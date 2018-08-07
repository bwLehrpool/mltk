### Variablen #####################################
LOCALUSER=student
USERHOME=/home/"$LOCALUSER"
USERHOMEDIR=/home/"$LOCALUSER"/PERSISTENT
USERDESKTOP=$(su -c - $LOCALUSER "xdg-user-dir DESKTOP")
[ "$USERDESKTOP" == "" ] && USERDESKTOP="$USERHOME"

DISKMOUNTDIR=/mnt/diskmount
SLXCONFIGFILE="$DISKMOUNTDIR"/openslx.ini
CONFIGFILE="$DISKMOUNTDIR"/shares.dat
KEYTEMP=$(mktemp -t XXXXXXXXXX.dat)
RAWKEYTEMP=$(mktemp -t XXXXXXXXXX.dat)
BYTES=256
declare -a ROHSHARES

# Just to be sure...
if [ ! $(mount 2>/dev/null | grep -q "$DISKMOUNTDIR") ]; then
	[ ! -d "$DISKMOUNTDIR" ]        && mkdir -p "$DISKMOUNTDIR"
	mount /dev/fd1 "$DISKMOUNTDIR" 2>/dev/null 1>&2		# Zu erl.: Fehlerfangen
fi

NATADDR=$(head -n 1 "$CONFIGFILE" | cut -f 1 -d$'\t')
PORT=$(head -n 1 "$CONFIGFILE" | cut -f 2 -d$'\t')
SCHLUESSEL=$(head -n 1 "$CONFIGFILE" | cut -f 4 -d$'\t')
GLOBALDOMAINUSER=$(head -n 1 "$CONFIGFILE" | cut -f 5 -d$'\t')
GLOBALUSER=$(echo "$GLOBALDOMAINUSER" | cut -d '\' -f 2)

REMAPMODE=$(grep '^remapMode=' "$SLXCONFIGFILE" | cut -d '=' -f 2)
CREATEMISSING=$(grep '^createMissingRemap=' "$SLXCONFIGFILE" | cut -d '=' -f 2)

SCRIPTUSER=$(grep '^scriptUser=' "$SLXCONFIGFILE" | cut -d '=' -f 2)
SCRIPTVISIBILITY=$(grep '^scriptVisibility=' "$SLXCONFIGFILE" | cut -d '=' -f 2)
SCRIPTEXT=$(grep scriptExt "$DISKMOUNTDIR/openslx.ini" | cut -f 2 -d "=")


# CIFSVERSIONS="3.11 3.0 2.1 2.0 1.0"
CIFSVERSIONS="3.0 2.1 1.0 3.11 2.0"

MOUNTCIFSPRAE="-v -t cifs -o "
MOUNTCIFSOPTS="uid=$(id --user "$LOCALUSER"),gid=$(id --group "$LOCALUSER"),forceuid,forcegid,file_mode=0700,dir_mode=0700,nobrl,noacl"

MOUNTNFSPRAE="-v -t nfs4 -o "
MOUNTNFSOPTS="rw,nosuid,nodev,nolock,intr,hard,sloppy"
### Variablen Ende ################################
