#!/bin/bash
writelog "+ apply 'allow_root' Option for gvfsd-fuse"

# exec dummy call to start gvfsd and gvfsd-fuse
gio mount

# create gvfs user dir mybe it does not exist at this point
mkdir -p "/run/user/$(id -u)/gvfs"

fusermount -zu "/run/user/$(id -u)/gvfs"
killall gvfsd-fuse
/usr/lib/gvfs/gvfsd-fuse -o allow_root "/run/user/$(id -u)/gvfs"
