<?php

declare(strict_types=1);

/**
 * Usage:
 *   php filter_ips.php <url_source_file> <cidr_file>
 *
 * Example:
 *   php filter_ips.php urls.txt cidrs.txt
 *
 * URLs could come from here:
 * https://github.com/curl/curl/wiki/DNS-over-HTTPS
 * CIDRs to ignore could come from here:
 * https://github.com/rezmoss/cloud-provider-ip-addresses/raw/refs/heads/main/all_providers/all_providers_ips.txt
 * (Make sure these are still maintained)
 */

if ($argc !== 3) {
    fwrite(STDERR, "Usage: php {$argv[0]} <url_source_file> <cidr_file>\n");
    exit(1);
}

$urlFile  = $argv[1];
$cidrFile = $argv[2];

if (!is_readable($urlFile)) {
    fwrite(STDERR, "Cannot read URL source file: {$urlFile}\n");
    exit(1);
}

if (!is_readable($cidrFile)) {
    fwrite(STDERR, "Cannot read CIDR file: {$cidrFile}\n");
    exit(1);
}

/**
 * Extract all https:// URLs from text.
 */
function extractHttpsUrls(string $content): array
{
    preg_match_all('~https://[^\s\'"<>]+~i', $content, $matches);

    return array_values(array_unique($matches[0]));
}

function extractIps(string $content): array
{
    preg_match_all('~^(\d+\.\d+\.\d+\.\d+|[0-9a-f:]+)$~im', $content, $matches);
    return array_values($matches[0]);
}

/**
 * Extract unique hostnames from URLs.
 */
function extractHostnames(array $urls): array
{
    $hosts = [];

    foreach ($urls as $url) {
        $host = parse_url($url, PHP_URL_HOST);

        if ($host !== null && $host !== false) {
            $hosts[strtolower($host)] = true;
        }
    }

    return array_keys($hosts);
}

/**
 * Resolve all IPv4 and IPv6 addresses for a hostname.
 */
function resolveAllIps(string $hostname): array
{
    $ips = [];

    $records = dns_get_record($hostname, DNS_A + DNS_AAAA);

    if ($records === false) {
        return [];
    }

    foreach ($records as $record) {
        if (isset($record['ip'])) {
            $ips[$record['ip']] = true;
        }

        if (isset($record['ipv6'])) {
            $ips[$record['ipv6']] = true;
        }
    }

    return array_keys($ips);
}

/**
 * Check whether an IP belongs to a CIDR range.
 */
function ipInCidr(string $ip, string $cidr): bool
{
    if (!str_contains($cidr, '/')) {
        return false;
    }

    [$subnet, $prefixLength] = explode('/', $cidr, 2);

    $ipBin     = inet_pton($ip);
    $subnetBin = inet_pton($subnet);

    if ($ipBin === false || $subnetBin === false) {
        return false;
    }

    // IPv4 vs IPv6 mismatch
    if (strlen($ipBin) !== strlen($subnetBin)) {
        return false;
    }

    $prefixLength = (int)$prefixLength;
    $bytes = strlen($ipBin);

    $fullBytes = intdiv($prefixLength, 8);
    $remainingBits = $prefixLength % 8;

    // Compare complete bytes
    for ($i = 0; $i < $fullBytes; $i++) {
        if ($ipBin[$i] !== $subnetBin[$i]) {
            return false;
        }
    }

    // Compare remaining bits
    if ($remainingBits > 0) {
        $mask = (~(0xFF >> $remainingBits)) & 0xFF;

        if (
            (ord($ipBin[$fullBytes]) & $mask) !==
            (ord($subnetBin[$fullBytes]) & $mask)
        ) {
            return false;
        }
    }

    return true;
}

/**
 * Load CIDRs from file.
 */
function loadCidrs(string $filename): array
{
    $result = [];

    $lines = file($filename, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    $lines[] = '127.0.0.0/8';
    $lines[] = '10.0.0.0/8';
    $lines[] = '192.168.0.0/16';
    $lines[] = '169.254.0.0/16';
    $lines[] = '172.16.0.0/12';
    $lines[] = '0.0.0.0/32';
    $lines[] = '255.255.255.255/32';
    $lines[] = '::/128';
    $lines[] = '::1/128';
    $lines[] = '5f00::/16';
    $lines[] = 'fc00::/7';
    $lines[] = 'fe80::/64';
    $lines[] = 'ff00::/8';
    $lines = array_unique($lines);

    foreach ($lines as $line) {
        $line = trim($line);

        if ($line === '' || str_starts_with($line, '#')) {
            continue;
        }

        if (!str_contains($line, '/')) {
            error_log("Not CIDR: $line");
            continue;
        }

        [$subnet, $prefix] = explode('/', $line, 2);

        $subnetBin = inet_pton($subnet);

        if ($subnetBin === false) {
            error_log("Not inet_pton: $subnet");
            continue;
        }

        $prefix = (int)$prefix;

        $fullBytes = intdiv($prefix, 8);
        $remainingBits = $prefix % 8;

        $mask = null;

        if ($remainingBits > 0) {
            $mask = (~(0xFF >> $remainingBits)) & 0xFF;
        }

        $result[] = [
            'family_len'    => strlen($subnetBin),
            'subnet_bin'    => $subnetBin,
            'full_bytes'    => $fullBytes,
            'remaining_bits'=> $remainingBits,
            'mask'          => $mask,
        ];
    }

    return $result;
}

/**
 * Check IP against preprocessed CIDRs.
 */
function ipMatchesCidrs(string $ip, array $cidrs): bool
{
    $ipBin = inet_pton($ip);

    if ($ipBin === false) {
        return false;
    }

    $ipLen = strlen($ipBin);

    foreach ($cidrs as $cidr) {
        // IPv4 vs IPv6 mismatch
        if ($ipLen !== $cidr['family_len']) {
            continue;
        }

        // Compare full bytes
        $matched = true;

        for ($i = 0; $i < $cidr['full_bytes']; $i++) {
            if ($ipBin[$i] !== $cidr['subnet_bin'][$i]) {
                $matched = false;
                break;
            }
        }

        if (!$matched) {
            continue;
        }

        // Compare remaining bits
        if ($cidr['remaining_bits'] > 0) {
            $mask = $cidr['mask'];
            $idx  = $cidr['full_bytes'];

            if (
                (ord($ipBin[$idx]) & $mask) !==
                (ord($cidr['subnet_bin'][$idx]) & $mask)
            ) {
                continue;
            }
        }

        return true;
    }

    return false;
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

$content = file_get_contents($urlFile);

if ($content === false) {
    fwrite(STDERR, "Failed to read {$urlFile}\n");
    exit(1);
}

$urls = extractHttpsUrls($content);
$hostnames = extractHostnames($urls);

// Resolve all IPs
$allIps = [];

foreach ($hostnames as $hostname) {
    foreach (resolveAllIps($hostname) as $ip) {
        $allIps[$ip] = true;
    }
}

$allIps = array_merge(array_keys($allIps), extractIps($content));
$allIps = array_unique($allIps);

// Load CIDRs
$cidrs = loadCidrs($cidrFile);

// Filter matching IPs
$matchedIps = [];

foreach ($allIps as $ip) {
    if (!ipMatchesCidrs($ip, $cidrs)) {
        echo $ip, PHP_EOL;
    }
}

