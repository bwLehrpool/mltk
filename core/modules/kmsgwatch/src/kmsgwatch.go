package main

import (
	"bufio"
	"context"
	"crypto/tls"
	"crypto/x509"
	"flag"
	"fmt"
	"log"
	"net"
	"net/http"
	"net/url"
	"os"
	"regexp"
	"strings"
	"sync"
	"syscall"
	"time"
)

// matcher pairs a compiled regexp with a message template.
// In the template, $1, $2, ... are replaced with the corresponding capture groups.
type matcher struct {
	re      *regexp.Regexp
	message string
}

// matcherList implements flag.Value so that -match can be given multiple times.
type matcherList []matcher

func (ml *matcherList) String() string {
	return fmt.Sprintf("%d matchers", len(*ml))
}

// Set parses one -match value of the form REGEX@@MESSAGE.
func (ml *matcherList) Set(s string) error {
	const sep = "@@"
	idx := strings.Index(s, sep)
	if idx < 0 {
		return fmt.Errorf("missing %q separator in -match value %q", sep, s)
	}
	pattern := s[:idx]
	message := s[idx+len(sep):]
	re, err := regexp.Compile(pattern)
	if err != nil {
		return fmt.Errorf("invalid regexp %q: %w", pattern, err)
	}
	*ml = append(*ml, matcher{re: re, message: message})
	return nil
}

// rateLimiter tracks the last 100 sent messages and enforces:
// - no more than 100 messages in any 900-second window
// - a given message may appear at most 2 times among the last 5 sent messages
type rateLimiter struct {
	mu      sync.Mutex
	history [100]struct {
		msg string
		at  time.Time
	}
	count int // total messages ever accepted (wraps into history via mod 100)
}

// allow returns true if the message should be sent, and records it if so.
func (r *rateLimiter) allow(msg string) bool {
	r.mu.Lock()
	defer r.mu.Unlock()

	now := time.Now()
	window := now.Add(-900 * time.Second)

	// If oldest is newer than 900 seconds, bail out
	if r.history[r.count%100].at.After(window) {
		log.Printf("Rate limit: dropping message (100 msgs in 900s): %s", msg)
		return false
	}

	// Additional last 5 duplicate check
	last5 := 0
	off := r.count + 100
	for i := 0; i < 5; i++ {
		e := r.history[(off-i)%100]
		if e.at.Before(window) {
			continue
		}
		if e.msg == msg {
			last5++
		}
	}

	if last5 >= 2 {
		log.Printf("Rate limit: dropping duplicate message (2 of last 5): %s", msg)
		return false
	}

	r.history[r.count%100] = struct {
		msg string
		at  time.Time
	}{msg: msg, at: now}
	r.count++
	return true
}

func main() {
	var matchers matcherList
	flag.Var(&matchers, "match", "REGEX@@MESSAGE pair; may be given multiple times.\n"+
		"    REGEX is a Go regular expression; MESSAGE may reference capture groups as $1, $2, …\n"+
		"    Example: -match 'oom_kill_process.*comm=\"([^\"]+)\"@@OOM killed process: $1'")
	urlFlag := flag.String("url", "", "HTTPS URL to POST matching lines to")
	uuidFlag := flag.String("uuid", "", "UUID to send with each POST request")
	typeFlag := flag.String("type", "", "Type to send with each POST request")
	skipVerify := flag.Bool("k", false, "Skip TLS certificate verification (like curl -k)")
	chrootDir := flag.String("chroot", "/tmp/fant", "Directory to chroot into after startup")
	flag.Parse()

	if len(matchers) == 0 {
		fmt.Fprintln(os.Stderr, "Error: at least one -match flag is required")
		os.Exit(1)
	}
	if *urlFlag == "" {
		fmt.Fprintln(os.Stderr, "Error: -url flag is required")
		os.Exit(1)
	}
	if *uuidFlag == "" {
		fmt.Fprintln(os.Stderr, "Error: -uuid flag is required")
		os.Exit(1)
	}
	if *typeFlag == "" {
		fmt.Fprintln(os.Stderr, "Error: -type flag is required")
		os.Exit(1)
	}

	// Parse and resolve the target URL's IP before chroot.
	parsedURL, err := url.Parse(*urlFlag)
	if err != nil {
		log.Fatalf("Invalid URL: %v", err)
	}

	hostname := parsedURL.Hostname()
	ip, err := resolveIP(hostname)
	if err != nil {
		log.Fatalf("Failed to resolve hostname %q: %v", hostname, err)
	}
	log.Printf("Resolved %s to %s", hostname, ip)

	// Preload system CA certificates before chroot.
	caPool, err := loadCAs()
	if err != nil {
		log.Fatalf("Failed to load CA certificates: %v", err)
	}
	log.Println("Loaded system CA certificates")

	// Open /dev/kmsg before chroot.
	f, err := os.Open("/dev/kmsg")
	if err != nil {
		log.Fatalf("Failed to open /dev/kmsg: %v", err)
	}
	defer f.Close()

	// Enter chroot dir (must already exist).
	if _, err := os.Stat(*chrootDir); err != nil {
		log.Fatalf("Chroot directory %q does not exist or is not accessible: %v", *chrootDir, err)
	}
	if err := syscall.Chroot(*chrootDir); err != nil {
		log.Fatalf("Failed to chroot to %q: %v", *chrootDir, err)
	}
	log.Printf("Chrooted to %s — DNS files no longer accessible", *chrootDir)

	scanner := bufio.NewScanner(f)

	var wg sync.WaitGroup
	var rl rateLimiter

	// URL keeps the real hostname for Host header and SNI.
	// DialContext forces the connection to the pre-resolved IP.
	client := &http.Client{
		Transport: &http.Transport{
			DialContext: func(ctx context.Context, network, addr string) (net.Conn, error) {
				_, p, err := net.SplitHostPort(addr)
				if err != nil {
					return nil, fmt.Errorf("split hostport: %w", err)
				}
				return net.Dial(network, net.JoinHostPort(ip, p))
			},
			TLSClientConfig: &tls.Config{
				ServerName:         hostname,
				RootCAs:            caPool,
				InsecureSkipVerify: *skipVerify,
			},
		},
	}

	for scanner.Scan() {
		line := scanner.Text()
		for _, m := range matchers {
			match := m.re.FindStringSubmatchIndex(line)
			if match == nil {
				continue
			}
			// Expand $1, $2, … in the message template using the capture groups.
			msg := string(m.re.ExpandString(nil, m.message, line, match))

			if !rl.allow(msg) {
				continue
			}
			wg.Add(1)
			go func(body string) {
				defer wg.Done()
				log.Printf("Match: %s", body)

				formData := url.Values{
					"uuid":        {*uuidFlag},
					"type":        {*typeFlag},
					"description": {body},
				}
				resp, err := client.PostForm(*urlFlag, formData)
				if err != nil {
					log.Printf("POST failed: %v", err)
					return
				}
				defer resp.Body.Close()
				if resp.StatusCode < 200 || resp.StatusCode >= 300 {
					log.Printf("Unexpected status: %d", resp.StatusCode)
				}
			}(msg)
		}
	}

	if err := scanner.Err(); err != nil {
		log.Printf("Error reading /dev/kmsg: %v", err)
	}

	wg.Wait()
	log.Println("kmsg reader stopped")
}

func resolveIP(hostname string) (string, error) {
	ips, err := net.LookupIP(hostname)
	if err != nil {
		return "", err
	}
	if len(ips) == 0 {
		return "", fmt.Errorf("no IPs found for %s", hostname)
	}
	return ips[0].String(), nil
}

func loadCAs() (*x509.CertPool, error) {
	return x509.SystemCertPool()
}
