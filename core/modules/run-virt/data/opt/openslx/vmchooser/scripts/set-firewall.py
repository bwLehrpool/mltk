#!/usr/bin/env python3

import os
import sys
import subprocess
import re
import shlex
import argparse
import tempfile
import shutil
import threading
import time
import ipaddress
import urllib.request
from typing import List, Dict, Set, Optional

# Constants
DNS_IPT_FILE = "/opt/openslx/iptables/rules.d/00-dnsblock"
ILLEGAL_DNS = re.compile(r'[?@:*/ ]')

def is_root():
    """Check if the current user has root privileges."""
    return os.getuid() == 0

def run_command(cmd: List[str], check: bool = True, capture_output: bool = False):
    """Run a system command and return the result."""
    try:
        result = subprocess.run(cmd, check=check, capture_output=capture_output, text=True)
        return result
    except subprocess.CalledProcessError as e:
        if check:
            print(f"Command failed: {' '.join(cmd)}", file=sys.stderr)
            if e.stderr:
                print(e.stderr, file=sys.stderr)
        return e

def parse_shell_config(path: str) -> Dict[str, str]:
    """Parse a shell configuration file and return a dictionary of variables."""
    config = {}
    if not os.path.exists(path):
        return config
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            try:
                parts = shlex.split(line)
                if not parts:
                    continue
                assignment = parts[0]
                if assignment == 'export' and len(parts) > 1:
                    assignment = parts[1]
                if '=' in assignment:
                    var, val = assignment.split('=', 1)
                    config[var] = val
            except ValueError:
                # Fallback to simple split if shlex fails
                if '=' in line:
                    var, val = line.split('=', 1)
                    config[var.strip()] = val.strip()
    return config

def get_ip_version(address: str) -> Optional[int]:
    """Return the IP version (4 or 6) of the given address, or None if invalid."""
    try:
        # Remove CIDR if present
        addr = address.split('/')[0]
        return ipaddress.ip_address(addr).version
    except ValueError:
        return None

def net_split(host_port: str, default_port: Optional[str] = None):
    """Split a host:port string into a (host, port) tuple."""
    host = host_port
    port = default_port
    
    if ':' in host_port:
        # Check if it's an IPv6 with port like [2001:db8::1]:80
        if host_port.startswith('[') and ']:' in host_port:
            parts = host_port.rsplit(']:', 1)
            host = parts[0][1:] # Remove [
            port = parts[1]
        elif host_port.count(':') == 1:
            # Simple host:port or ipv4:port
            parts = host_port.split(':')
            host = parts[0]
            port = parts[1]
        else:
            # IPv6 without brackets and port, or just IPv6
            # In bash: port=${1##*:}
            potential_port = host_port.split(':')[-1]
            if potential_port.isdigit() and 0 < int(potential_port) < 65536:
                host = host_port.rsplit(':', 1)[0]
                port = potential_port
            else:
                host = host_port
                port = default_port

    if host.startswith('[') and host.endswith(']'):
        host = host[1:-1]
        
    return host, port

def net_parse_uri(uri: str, fallback_port: Optional[str] = None):
    """Parse a URI and return a (host, port) tuple based on the scheme."""
    if '://' not in uri:
        return None, None
    
    scheme, rest = uri.split('://', 1)
    host_part = rest.split('/', 1)[0]
    
    default_ports = {
        'ldaps': '636',
        'ldap': '389',
        'https': '443',
        'http': '80',
        'smb': '445',
        'nfs': '2049'
    }
    
    port = default_ports.get(scheme, fallback_port)
    return net_split(host_part, port)

class FirewallManager:
    def __init__(self, uuid, dnscfg_path=None, dnsport=None, parent_pid=None):
        """Initialize the FirewallManager with session details."""
        self.uuid = uuid
        self.dnscfg_path = dnscfg_path
        self.dnsport = dnsport
        self.parent_pid = parent_pid
        self.auto_rules = []
        self.remote_rules = []
        self.dns_list = {} # domain -> list of (action, dir, port)
        self.dns_servers = []
        self.block_all = None
        self.config = {}
        
    def load_configs(self):
        """Load configuration from standard system paths."""
        # This is where all the SLX_* variables come from
        self.config.update(parse_shell_config('/opt/openslx/config'))
        # This is currently only used for url_lecture_netrules
        resource_urls_path = '/opt/openslx/vmchooser/config/resource_urls.conf'
        if os.path.exists(resource_urls_path):
            self.config.update(parse_shell_config(resource_urls_path))

    def setup_chains(self):
        """Initialize and hook custom iptables chains."""
        if os.path.exists(DNS_IPT_FILE):
            os.remove(DNS_IPT_FILE)
            
        for tool in ['iptables', 'ip6tables']:
            for suffix in ['INPUT', 'OUTPUT', 'INPUT-sub', 'OUTPUT-sub']:
                chain = f"runvirt-{suffix}"
                # Flush or create
                if run_command([tool, '-w', '-F', chain], check=False).returncode != 0:
                    if run_command([tool, '-w', '-N', chain], check=False).returncode != 0:
                        return False
            
            # Hooks
            for hook in [('INPUT', '-i', 'br0', 'runvirt-INPUT'),
                         ('OUTPUT', '-o', 'br0', 'runvirt-OUTPUT'),
                         ('FORWARD', '-i', 'br0', 'runvirt-INPUT'),
                         ('FORWARD', '-o', 'br0', 'runvirt-OUTPUT')]:
                base_chain, flag, interface, target = hook
                check_cmd = [tool, '-w', '-C', base_chain, flag, interface, '-j', target]
                if run_command(check_cmd, check=False).returncode != 0:
                    run_command([tool, '-w', '-A', base_chain, flag, interface, '-j', target])
            
            # Allow loopback
            run_command([tool, '-w', '-A', 'runvirt-INPUT', '-i', 'lo', '-j', 'ACCEPT'])
            run_command([tool, '-w', '-A', 'runvirt-OUTPUT', '-o', 'lo', '-j', 'ACCEPT'])
            
            # Jump to subchains
            run_command([tool, '-w', '-A', 'runvirt-INPUT', '-j', 'runvirt-INPUT-sub'])
            run_command([tool, '-w', '-A', 'runvirt-OUTPUT', '-j', 'runvirt-OUTPUT-sub'])
            
        return True

    def discover_infrastructure(self):
        """Identify DNS servers and other infrastructure components."""
        # DNS
        dns_ips = set()
        slx_dns = self.config.get('SLX_DNS')
        if slx_dns:
            dns_ips.update(slx_dns.split())
        
        if os.path.exists('/etc/resolv.conf'):
            with open('/etc/resolv.conf', 'r') as f:
                for line in f:
                    parts = line.split()
                    if len(parts) >= 2 and parts[0] == 'nameserver':
                        dns_ips.add(parts[1])
        
        res = run_command(['resolvectl', 'dns'], check=False, capture_output=True)
        if isinstance(res, subprocess.CompletedProcess) and res.returncode == 0:
            # resolvectl output can be multi-line and contains interface names/labels
            # e.g.: Link 2 (eth0): 132.230.200.200 132.230.201.111
            for line in res.stdout.splitlines():
                if ':' not in line:
                    continue
                _, servers = line.split(':', 1)
                for part in servers.split():
                    if get_ip_version(part):
                        dns_ips.add(part)
        
        dnsmasq_configs = ['/etc/dnsmasq.conf']
        if os.path.isdir('/etc/dnsmasq.d'):
            try:
                for f in os.listdir('/etc/dnsmasq.d'):
                    dnsmasq_configs.append(os.path.join('/etc/dnsmasq.d', f))
            except OSError:
                pass
        
        for cfg in dnsmasq_configs:
            if os.path.exists(cfg):
                try:
                    with open(cfg, 'r') as f:
                        for line in f:
                            match = re.search(r'^server=([^/].*)', line.strip())
                            if match:
                                dns_ips.update(re.findall(r'\S+', match.group(1)))
                except OSError:
                    pass
        
        self.dns_servers = sorted(list(dns_ips))
        
        # Create dns allow script
        try:
            with tempfile.NamedTemporaryFile(mode='w', delete=False) as tf:
                tf.write("#!/bin/ash\n")
                for ip in self.dns_servers:
                    version = get_ip_version(ip)
                    if version == 4:
                        tool = 'iptables'
                    elif version == 6:
                        tool = 'ip6tables'
                    else:
                        continue
                    tf.write(f"{tool} -w -A OUTPUT -d {ip} -p udp --dport 53 -j ACCEPT\n")
                    tf.write(f"{tool} -w -A OUTPUT -d {ip} -p tcp --dport 53 -j ACCEPT\n")
                temp_name = tf.name
            
            os.chmod(temp_name, 0o755)
            # Use a more descriptive name
            target_path = "/opt/openslx/iptables/rules.d/10-dns-allow"
            shutil.move(temp_name, target_path)
        except Exception as e:
            print(f"Failed to create DNS allow rules: {e}", file=sys.stderr)
            # Not fatal enough to exit 7? Bash doesn't check this specifically.

        # SSSD
        if os.path.exists('/etc/sssd/sssd.conf'):
            with open('/etc/sssd/sssd.conf', 'r') as f:
                for line in f:
                    match = re.match(r'^\s*ldap_(?:backup_)?uri\s*=\s*(.*)', line.strip())
                    if match:
                        uris = match.group(1).replace(',', ' ').split()
                        for uri in uris:
                            host, port = net_parse_uri(uri)
                            if host:
                                self.add_auto_rule("OUT", host, port or "0", "ACCEPT")

        # PAM LDAP
        pam_dir = '/opt/openslx/pam/slx-ldap.d'
        if os.path.isdir(pam_dir):
            for f in os.listdir(pam_dir):
                fpath = os.path.join(pam_dir, f)
                if os.path.isfile(fpath):
                    ldap_cfg = parse_shell_config(fpath)
                    ldap_uri = ldap_cfg.get('LDAP_URI')
                    if ldap_uri:
                        for uri in ldap_uri.split():
                            host, port = net_parse_uri(uri)
                            if host:
                                self.add_auto_rule("OUT", host, port or "0", "ACCEPT")

        # NFS
        slx_vm_nfs = self.config.get('SLX_VM_NFS')
        if slx_vm_nfs:
            ip = None
            if slx_vm_nfs.startswith('//'): # Assume CIFS/SMB syntax
                ip = slx_vm_nfs[2:].split('/')[0]
            else: # Assume NFS syntax
                ip = slx_vm_nfs.split(':')[0]
            if ip:
                self.add_auto_rule("OUT", ip, "0", "ACCEPT")

    def add_auto_rule(self, direction, host, port, action):
        """Add a rule to the internal list of automatic rules."""
        self.auto_rules.append((direction, host, port, action))

    def download_rules(self):
        """Fetch network rules for the current session from the management server."""
        base_url = self.config.get('SLX_VMCHOOSER_BASE_URL', '').rstrip('/')
        url = self.config.get('url_lecture_netrules', '').replace('%UUID%', self.uuid)
        if not url:
            url = f"{base_url}/lecture/{self.uuid}/netrules"
            
        try:
            with urllib.request.urlopen(url, timeout=8) as response:
                content = response.read().decode('utf-8')
                for line in content.splitlines():
                    parts = line.strip().split()
                    if len(parts) >= 4:
                        self.remote_rules.append(tuple(parts[:4]))
            return True
        except urllib.error.HTTPError as e:
            if e.code == 404:
                return True # Old satellite, no rules
            print(f"HTTP Error: {e.code}", file=sys.stderr)
            return False
        except Exception as e:
            print(f"Download error: {e}", file=sys.stderr)
            return False

    def process_rules(self):
        """Compile and apply all gathered rules to the system firewall and DNS."""
        all_rules = []
        # Add auto rules (deduplicated)
        seen = set()
        for r in self.auto_rules:
            if r not in seen:
                all_rules.append(r)
                seen.add(r)
        # Add remote rules
        all_rules.extend(self.remote_rules)
        
        dnsmasq_available = shutil.which('dnsmasq') is not None
        dig_available = shutil.which('dig') is not None
        
        dns_config_lines = []
        
        for rule in all_rules:
            if len(rule) < 4: continue
            direction, dest, port, action = rule
            
            # Prepare iptables
            ip_version = get_ip_version(dest)
            
            is_hostname = (dest != '*' and ip_version is None)
            
            # Bash logic for 'both'
            both = False
            if is_hostname:
                if direction != "OUT" or not dnsmasq_available or port != "0" or ILLEGAL_DNS.search(dest):
                    both = True
                if not dig_available:
                    both = True
            
            handle_via_dns = False
            if is_hostname:
                if not both or action == "ACCEPT":
                    handle_via_dns = True
            elif dest == "*":
                handle_via_dns = True # Always handle '*' in DNS if possible
                both = True # And always in iptables

            if handle_via_dns and self.dnscfg_path:
                if action == "ACCEPT":
                    if dest == "*":
                        if self.block_all is None: self.block_all = False
                        both = True
                    else:
                        for dnsip in self.dns_servers:
                            dns_config_lines.append(f"server=/{dest}/{dnsip}")
                else: # REJECT/DROP
                    if dest == "*":
                        if self.block_all is None: self.block_all = True
                        both = True
                    else:
                        dns_config_lines.append(f"address=/{dest}/")

            if is_hostname and not ILLEGAL_DNS.search(dest):
                if both:
                    if dest not in self.dns_list:
                        self.dns_list[dest] = []
                    self.dns_list[dest].append((action, direction, port))
            
            # Apply to iptables
            if not is_hostname or both:
                if dest == "*" or ip_version == 4 or is_hostname:
                    self.apply_iptables("iptables", direction, dest, port, action)
                if dest == "*" or ip_version == 6 or is_hostname:
                    self.apply_iptables("ip6tables", direction, dest, port, action)

        # Finalize DNS config
        if self.dnscfg_path:
            with open(self.dnscfg_path, 'a') as f:
                f.write("address=/use-application-dns.net/\n")
                for line in dns_config_lines:
                    f.write(line + "\n")
                
                if self.block_all:
                    for dnsip in self.dns_servers:
                        f.write(f"server=/in-addr.arpa/{dnsip}\n")
                        f.write(f"server=/ip6.arpa/{dnsip}\n")
                    f.write("address=/#/\n")
                else:
                    for dnsip in self.dns_servers:
                        f.write(f"server={dnsip}\n")
            
            # DNS NAT Redirection
            with open(DNS_IPT_FILE, 'w') as f:
                f.write(f"iptables -t nat -A PREROUTING -p tcp --dport 53 -j REDIRECT --to-port {self.dnsport}\n")
                f.write(f"iptables -t nat -A PREROUTING -p udp --dport 53 -j REDIRECT --to-port {self.dnsport}\n")
                f.write(f"ip6tables -t nat -A PREROUTING -p tcp --dport 53 -j REDIRECT --to-port {self.dnsport} || ip6tables -A FORWARD -p tcp --dport 53 -j DROP\n")
                f.write(f"ip6tables -t nat -A PREROUTING -p udp --dport 53 -j REDIRECT --to-port {self.dnsport} || ip6tables -A FORWARD -p udp --dport 53 -j DROP\n")
            os.chmod(DNS_IPT_FILE, 0o755)

            # Block DoH servers
            doh_servers_path = '/opt/openslx/vmchooser/data/doh-servers'
            if os.path.exists(doh_servers_path):
                with open(doh_servers_path, 'r') as f:
                    for server_ip in f:
                        server_ip = server_ip.strip()
                        if not server_ip: continue
                        version = get_ip_version(server_ip)
                        tool = "ip6tables" if version == 6 else "iptables"
                        run_command([tool, '-w', '-I', 'runvirt-OUTPUT', '1', '-d', server_ip, '-p', 'tcp', '--dport', '443', '-j', 'REJECT', '--reject-with', 'tcp-reset'], check=False)

    def apply_iptables(self, tool, direction, dest, port, action, front=False):
        """Execute iptables/ip6tables commands to apply a specific rule."""
        if dest == "*":
            chain = f"runvirt-{direction}"
        else:
            chain = f"runvirt-{direction}-sub"
            
        args = ['-w']
        if front:
            args += ['-I', chain, '1']
        else:
            args += ['-A', chain]
            
        if dest != "*":
            if direction == "OUT":
                args += ['-d', dest]
            else:
                args += ['-s', dest]
        
        # Action and Port
        if port == "0":
            if action == "REJECT":
                # For REJECT, we add a tcp-reset rule first
                tcp_args = args + ['-p', 'tcp', '-j', 'REJECT', '--reject-with', 'tcp-reset']
                run_command([tool] + tcp_args, check=False)
            run_command([tool] + args + ['-j', action], check=False)
        else:
            # TCP with reset if REJECT
            tcp_args = args + ['-p', 'tcp', '--dport', port, '-j', action]
            if action == "REJECT":
                tcp_args += ['--reject-with', 'tcp-reset']
            run_command([tool] + tcp_args, check=False)
            
            # UDP
            udp_args = args + ['-p', 'udp', '--dport', port, '-j', action]
            run_command([tool] + udp_args, check=False)

    def dns_monitor_loop(self):
        """Periodically resolve hostnames and update iptables rules for identified IPs."""
        known_ips = {} # (ip, action, direction, port) -> bool
        
        dig_args = ['-p', str(self.dnsport), '@127.0.0.1']
        
        ctr = 0
        while True:
            # Check if parent process still exists
            if not os.path.exists(f"/proc/{self.parent_pid}"):
                break
            
            if self.dns_list and ctr % 6 == 0:
                for domain, rules in self.dns_list.items():
                    for qtype, tool in [('A', 'iptables'), ('AAAA', 'ip6tables')]:
                        cmd = ['dig'] + dig_args + ['+short', domain, qtype]
                        res = run_command(cmd, check=False, capture_output=True)
                        if isinstance(res, subprocess.CompletedProcess) and res.returncode == 0:
                            ips = res.stdout.splitlines()
                            for ip in ips:
                                if not ip: continue
                                for action, direction, port in rules:
                                    key = (ip, action, direction, port)
                                    if key not in known_ips:
                                        self.apply_iptables(tool, direction, ip, port, action)
                                        known_ips[key] = True
            
            ctr += 1
            time.sleep(5)
        
        if os.path.exists(DNS_IPT_FILE):
            try:
                os.remove(DNS_IPT_FILE)
            except OSError:
                pass

class ArgumentParser(argparse.ArgumentParser):
    def error(self, message):
        """Handle argument parsing errors."""
        sys.exit(3)

def main():
    """Main entry point for the firewall setup script."""
    parser = ArgumentParser(description="Set up VM firewall rules")
    parser.add_argument("uuid", help="Lecture identifier")
    parser.add_argument("dnscfg", nargs="?", help="Path to dnsmasq configuration block")
    parser.add_argument("dnsport", nargs="?", help="Port where local dnsmasq is listening")
    parser.add_argument("parentpid", nargs="?", help="PID of the VM process")
    
    args = parser.parse_args()
    
    if not is_root():
        sys.exit(1)
        
    try:
        with tempfile.NamedTemporaryFile():
            pass
    except Exception:
        sys.exit(2)
        
    if not (10 <= len(args.uuid) < 40):
        sys.exit(4 if len(args.uuid) < 10 else 5)
        
    manager = FirewallManager(args.uuid, args.dnscfg, args.dnsport, args.parentpid)
    manager.load_configs()
    
    if not manager.setup_chains():
        sys.exit(7)
        
    manager.discover_infrastructure()
    
    if not manager.download_rules():
        sys.exit(6)
        
    try:
        manager.process_rules()
    except Exception as e:
        print(f"Error applying rules: {e}", file=sys.stderr)
        sys.exit(8)
        
    if args.parentpid and (manager.dns_list or manager.dnscfg_path):
        # Start background monitor
        # In bash it was: ... &
        # We can use a daemon thread if the main process is supposed to exit, 
        # but wait, if the main process exits, the daemon thread also exits.
        # So we should fork if we want it to stay alive.
        
        try:
            pid = os.fork()
            if pid > 0:
                # Parent exits
                sys.exit(0)
        except OSError as e:
            print(f"fork failed: {e}", file=sys.stderr)
            sys.exit(0) # Still success for the main part

        # Child process (Daemon)
        # Decouple from parent environment
        os.setsid()
        os.umask(0)
        # Redirect standard file descriptors
        sys.stdout.flush()
        sys.stderr.flush()
        with open('/dev/null', 'r') as f:
            os.dup2(f.fileno(), sys.stdin.fileno())
        with open('/tmp/dns-monitor', 'w') as f:
            os.dup2(f.fileno(), sys.stdout.fileno())
            os.dup2(f.fileno(), sys.stderr.fileno())
            
        manager.dns_monitor_loop()
    
    sys.exit(0)

if __name__ == "__main__":
    main()
