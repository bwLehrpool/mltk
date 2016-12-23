### Variablen #####################################
LOCALUSER=student
USERHOME=/home/"$LOCALUSER"
USERHOMEDIR=/home/"$LOCALUSER"/PERSISTENT
DISKMOUNTDIR=/mnt/diskmount
SLXCONFIGFILE="$DISKMOUNTDIR"/openslx.ini
CONFIGFILE="$DISKMOUNTDIR"/shares.dat
KEYTEMP=$(mktemp -t XXXXXXXXXX.dat)
RAWKEYTEMP=$(mktemp -t XXXXXXXXXX.dat)
BYTES=256
declare -a ROHSHARES

if [ ! $(mount 2>/dev/null | grep -q "$DISKMOUNTDIR") ]; then
	[ ! -d "$DISKMOUNTDIR" ]        && mkdir -p "$DISKMOUNTDIR"
	mount /dev/fd1 "$DISKMOUNTDIR" 2>/dev/null 1>&2		# Zu erl.: Fehlerfangen
fi

NATADDR=$(head -n 1 "$CONFIGFILE" | cut -f 1 -d$'\t')
PORT=$(head -n 1 "$CONFIGFILE" | cut -f 2 -d$'\t')
SCHLUESSEL=$(head -n 1 "$CONFIGFILE" | cut -f 4 -d$'\t')
GLOBALDOMAINUSER=$(head -n 1 "$CONFIGFILE" | cut -f 5 -d$'\t')
GLOBALUSER=$(echo "$GLOBALDOMAINUSER" | cut -d '\' -f 2)

REMAPMODE=$(grep 'remapMode=' "$SLXCONFIGFILE" | cut -d '=' -f 2)
CREATEMISSING=$(grep 'createMissingRemap=' "$SLXCONFIGFILE" | cut -d '=' -f 2)

MOUNTOPTSCIFS="-v -t cifs -o uid=$(id --user "$LOCALUSER"),gid=$(id --group "$LOCALUSER"),forceuid,forcegid,file_mode=0700,dir_mode=0700,nobrl,noacl"
MOUNTOPTSNFS="-v -t nfs4 -o rw,nosuid,nodev,nolock,intr,hard,sloppy"
### Variablen Ende ################################
