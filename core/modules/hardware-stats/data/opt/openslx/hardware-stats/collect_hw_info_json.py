from dmiparser import DmiParser
from subprocess import PIPE
from os import listdir, path
import argparse
import json
import re
import requests
import shlex
import subprocess
import sys

__debug = False

# Run dmi command as subprocess and get the stdout
def run_subprocess(_cmd):
    global __debug
    proc = subprocess.run(_cmd, shell=True, stdout=PIPE, stderr=PIPE)
    stdout = proc.stdout
    stderr = proc.stderr

    if __debug:
        eprint(_cmd + ':')
        eprint()
        eprint('Errors:')
        eprint(stderr.decode())
        eprint()
    # stderr len instead of proc.returncode > 0 is used because some have returncode 2 but are still valid
    if len(stderr.decode()) > 0:
        eprint(_cmd + ' Errors:')
        eprint(stderr.decode())
        if proc.returncode != 0:
            eprint('Error Return Code: ' + str(proc.returncode))
            eprint()
            return False
        else:
            return stdout.decode()
    else:
        return stdout.decode() 

# Get and parse dmidecode using dmiparser
def get_dmidecode():
    _dmiraw = run_subprocess('dmidecode')
    _dmiparsed = ''
    if _dmiraw:
        # Parse dmidecode
        _dmiparsed = DmiParser(_dmiraw)
        return json.loads(str(_dmiparsed))
    else:
        return []

# Get the readlink -f output
def get_readlink(link):
    _readlink = run_subprocess('readlink -f ' + link)
    return _readlink

# Try parsing string to json
def parse_to_json(program_name, raw_string):
    parsed = {}
    if isinstance(raw_string, str):
        try:
            parsed = json.loads(raw_string)
        except ValueError as e:
            eprint(program_name + ' output couldn\'t be parsed')
            eprint(e)
            eprint('Output of ' + program_name + 'was:')
            eprint(raw_string)
            eprint()
    return parsed

# Get smartctl output in json format
def get_smartctl(disk_path, disk_name):
    disk_path_full = disk_path + disk_name
    output = run_subprocess('smartctl -x --json ' + disk_path_full)
    smartctl = parse_to_json('smartctl', output)
    return smartctl

# Get sfdisk info in json format
def get_sfdisk(disk_path, disk_name):
    output = run_subprocess('sfdisk --json ' + disk_path + disk_name)
    sfdisk = parse_to_json('sfdisk', output)
    return sfdisk

# Get lsblk info in json format
def get_lsblk(disk_path, disk_name):
    output = run_subprocess('lsblk --json -b --output-all ' + disk_path + disk_name)
    lsblk = parse_to_json('lsblk', output)
    return lsblk

# Get CD/DVD Information
def get_cdrom():
    cdromdir = '/proc/sys/dev/cdrom/'
    cdrom = []
    if path.exists(cdromdir):
        cdrom_raw = run_subprocess('cat ' + cdromdir + 'info')

        # Skip first two entries because of useless information and empty row
        for row in cdrom_raw.split('\n')[2:]:
            if row == '':
                continue
            # Split at one or more tabs
            values = re.split('\t+', row)
            key = values[0][:-1].replace('drive ', '').replace(' ', '_')
            for index, val in enumerate(values[1:]):
                if len(cdrom) < index + 1:
                    cdrom.append({ 'read': [], 'write': [], 'functions': [] })

                if 'Can_read_' in key:
                    if val == '1':
                        cdrom[index]['read'].append(key[9:])
                elif 'Can_write_' in key:
                    cdrom[index]['write'].append(key[10:])
                elif 'Can_' in key:
                    cdrom[index]['functions'].append(key[4:])
                else:
                    cdrom[index][key] = val
    return {cd['name']:cd for cd in cdrom}

# Get informations about the disks
def get_disk_info():
    diskdir = '/dev/disk/by-path/'
    disk_informations = {}
    dupcheck = {}
    cdrom = get_cdrom()

    # Get and filter all disks
    if path.exists(diskdir):
        disks = listdir(diskdir)
        filtered_disks = [i for i in disks if (not '-part' in i) and (not '-usb-' in i)]

        # Call all disk specific tools
        for d in filtered_disks:
            disk_path = diskdir + d
            devpath = get_readlink(disk_path).rstrip()
            # Sometimes there are multiple links to the same disk, e.g. named
            # pci-0000:00:1f.2-ata-1.0 and pci-0000:00:1f.2-ata-1
            if devpath in dupcheck:
                continue
            dupcheck[devpath] = 1
            disk_info = {}
            disk_info['readlink']   = devpath

            # Check if it's a cd/dvd
            if disk_info['readlink'].split('/')[-1] in cdrom.keys():
                disk_info['type'] = 'cdrom'
                disk_info['info'] = cdrom[disk_info['readlink'].split('/')[-1]]
            else:
                disk_info['type'] = 'drive'
            disk_info['smartctl']   = get_smartctl(diskdir, d)
            disk_info['lsblk']      = get_lsblk(diskdir, d)

            if disk_info['type'] != 'cdrom':
                disk_info['sfdisk']     = get_sfdisk(diskdir, d)

            disk_informations[d]    = disk_info
    return disk_informations

# Get and process "lspci -mn" output
def get_lspci():
    lspci = []
    lspci_raw = run_subprocess('lspci -mmn').split('\n')

    # Prepare addition of iommu group
    iommu_groups = {}
    iommu_raw = run_subprocess('find /sys/kernel/iommu_groups/*/devices/*')
    if iommu_raw:
        iommu_split = iommu_raw.split('\n')
        for iommu_path in iommu_split:
            if iommu_path == "":
                continue
            iommu = iommu_path.split('/')
            iommu_groups[iommu[6][5:]] = iommu[4]

    for line in lspci_raw:
        if len(line) <= 0: continue

        # Parsing shell like command parameters
        parse = shlex.split(line)
        lspci_parsed = {}
        arguments = []
        values = []
        for parameter in parse:
            # Split values from arguments
            if parameter.startswith('-'):
                arguments.append(parameter)
            else:
                values.append(parameter)

        # Prepare values positions are in order
        if len(values) >= 6:
            lspci_parsed['slot']                = values[0]

            # The busybox version of lspci has "Class <class>" instead of "<class>"
            lspci_parsed['class']               = values[1].replace("Class ", "")
            lspci_parsed['vendor']              = values[2]
            lspci_parsed['device']              = values[3]
            lspci_parsed['subsystem_vendor']    = values[4]
            lspci_parsed['subsystem']           = values[5]

            # Additional add iommu group
            if values[0] in iommu_groups:
                lspci_parsed['iommu_group'] = iommu_groups[values[0]]

        # Prepare arguments
        if len(arguments) > 0:
            for arg in arguments:
                if arg.startswith('-p'):
                    lspci_parsed['progif'] = arg[2:]
                elif arg.startswith('-r'):
                    lspci_parsed['rev'] = arg[2:]
                else: continue

        lspci.append(lspci_parsed)
    return lspci

# Get ip data in json format
def get_ip():
    result = []
    ip_raw = run_subprocess('ip --json addr show')
    if isinstance(ip_raw, str):
        result = json.loads(ip_raw)
    return result

def get_net_fallback():
    netdir = '/sys/class/net/'
    result = {}

    # Get MAC address and speed
    if path.exists(netdir):
        interfaces = run_subprocess('ls ' + netdir).split('\n')
        for interface in interfaces:
            # Skip local stuff
            if interface == 'lo' or interface == '':
                continue
            net = {}
            speed = run_subprocess('cat ' + netdir + interface + '/speed')
            if isinstance(speed, str) and speed.endswith('\n'):
                net['speed'] = speed[:-1]

            duplex = run_subprocess('cat ' + netdir + interface + '/duplex')
            if isinstance(duplex, str) and duplex.endswith('\n'):
                net['duplex'] = duplex[:-1]

            mac = run_subprocess('cat ' + netdir + interface + '/address')
            if isinstance(mac, str) and mac.endswith('\n'):
                net['mac'] = mac[:-1]
            result[interface] = net

    # Get IP address
    interfaces = run_subprocess('ip -o addr show | awk \'/inet/ {print $2, $3, $4}\'').split('\n')
    for interface in interfaces:
        if interface == '':
            continue
        interf = interface.split(' ')
        if interf[0] == 'lo':
            continue
        else:
            if interf[0] not in result:
                result[interf[0]] = {}
            if interf[1] == 'inet':
                result[interf[0]]['ipv4'] = interf[2]
            elif interf[1] == 'inet6':
                result[interf[0]]['ipv6'] = interf[2]
    return result

# Get and convert EDID data to hex
def get_edid():
    edid = {}
    display_paths = run_subprocess('ls /sys/class/drm/*/edid')
    if display_paths:
        display_paths = display_paths.split('\n')
        for dp in display_paths:
            if dp == '': continue
            edid_hex = open(dp, 'rb').read().hex()
            if len(edid_hex) > 0:
                # The path is always /sys/class/drm/[..]/edid, so cut the first 15 chars and the last 5 chars
                edid[dp[15:-5]] = edid_hex
    return edid

def get_lshw():
    result = []
    lshw_raw = run_subprocess('lshw -json')
    if isinstance(lshw_raw, str):
        result = json.loads(lshw_raw)
    return result

def get_uuid():
    uuid_path = '/sys/class/dmi/id/product_uuid'
    uuid = 'N/A'
    if path.exists(uuid_path):
        uuid_raw = run_subprocess('cat ' + uuid_path)
        # uuid_raw = False if no sudo permission:
        if uuid_raw:
            uuid = uuid_raw.rstrip()
    return uuid

def prepare_location(parent, bay, slot):
    location = {}
    if parent:
        location['parent']  = parent
    if bay:
        location['bay']     = int(bay)
    if slot:
        location['slot']    = int(slot)
    return location

def prepare_contacts(contact_list):
    contacts = []
    if contact_list == None:
        return contacts
    for contact in contact_list:
        contacts.append(contact[0])
    return contacts

def send_post(url, payload):
    # headers = { 'Content-type': 'application/json', 'Accept': 'text/plain' }
    # req = requests.post(url, json=payload, headers=headers)
    req = requests.post(url, json=payload)
    # Print the response
    print("POST-Request Response: \n")
    print(req.text)

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

def main():
    global __debug

    # Create and parse arguments
    parser = argparse.ArgumentParser(description='Collects hardware data from different tools and returns it as json.')
    parser.add_argument('-d', '--debug',    action='store_true',    help='Prints all STDERR messages. (Non critical included)')
    parser.add_argument('-u', '--url',      action='append',        help='[multiple] If given, a post request with the generated JSON is sent to the given URLs')
    parser.add_argument('-uu', '--uuidurl', action='append',        help='[multiple] Same as -u but UUID in the url is replaced with the actual uuid of the client')
    parser.add_argument('-p', '--print',    action='store_true',    help='Prints the generated JSON')
    parser.add_argument('-l', '--location', action='store',         help='Room-/Rackname where the client is located')
    parser.add_argument('-s', '--slot',     action='store',         help='The slot number (int) where the client is located in the rack')
    parser.add_argument('-b', '--bay',      action='store',         help='The bay number (int) where the client is located in the slot (segment)')
    parser.add_argument('-c', '--contact',  action='append',        help='[multiple] The idoit_username of the person responsible for this machine', nargs=1)
    parser.add_argument('-n', '--name',     action='store',         help='Name of the client')
    parser.add_argument('-S', '--SERVER',   action='store_true',    help='Defines the type of the client to be a server')
    args = parser.parse_args()

    if args.debug:
        __debug = True

    # Run the tools
    _collecthw = {}
    _collecthw['version']   = 2.0
    _collecthw['dmidecode'] = get_dmidecode()

    # Includes smartctl, lsblk, readlink and sfdisk
    _collecthw['drives']    = get_disk_info()

    # Includes iommu group
    _collecthw['lspci']     = get_lspci()

    _collecthw['ip']        = get_ip()
    _collecthw['edid']      = get_edid()
    _collecthw['lshw']      = get_lshw()
    _collecthw['net']       = get_net_fallback()

    _collecthw['location']  = prepare_location(args.location, args.bay, args.slot)
    _collecthw['contacts']  = prepare_contacts(args.contact)

    if args.name:
        _collecthw['name'] = args.name

    if args.SERVER:
        _collecthw['type'] = 'SERVER'

    collecthw_json = json.dumps(_collecthw)
    if args.url:
        for url in args.url:
            send_post(url, _collecthw)
    if args.uuidurl:
        uuid = get_uuid()
        for uuidurl in args.uuidurl:
            url = uuidurl.replace("UUID", uuid)
            send_post(url, _collecthw)

    # Print out the final json
    if args.print:
        print(json.dumps(_collecthw))

if __name__ == "__main__":
    main()

