### Variablen #####################################

DISKMOUNTDIR=/mnt/diskmount
ADMINRUNDIR="$DISKMOUNTDIR"/adminrun
SLXCONFIGFILE="$DISKMOUNTDIR"/openslx.ini
CONFIGFILE="$DISKMOUNTDIR"/shares.dat
LOGINUSER=$(grep '^username=' "$SLXCONFIGFILE" | cut -d '=' -f 2)

##### User related#################
LOCALUSER=student
USERHOME=/home/"$LOCALUSER"
USERHOMEDIR=/home/"$LOCALUSER"/PERSISTENT
USERDESKTOP=$(su -c - $LOCALUSER "xdg-user-dir DESKTOP")
[ "$USERDESKTOP" == "" ] && USERDESKTOP="$USERHOME"
USERLOGFILE="$USERDESKTOP"/openslx.log


##### Mounting, network, PW #######
KEYTEMP=$(mktemp -t XXXXXXXXXX.dat)
RAWKEYTEMP=$(mktemp -t XXXXXXXXXX.dat)
BYTES=256
declare -a ROHSHARES

# Just to be sure...
if [ ! $(mount 2>/dev/null | grep -q "$DISKMOUNTDIR") ]; then
	[ ! -d "$DISKMOUNTDIR" ]        && mkdir -p "$DISKMOUNTDIR"
	mount /dev/fd1 "$DISKMOUNTDIR" 2>/dev/null 1>&2		# Zu erl.: Fehlerfangen
fi

# CIFSVERSIONS="3.11 3.0 2.1 2.0 1.0"
#CIFSVERSIONS="3.0 2.1 1.0 3.11 2.0"
CIFSVERSIONS="vers=3.0,sec=ntlmssp vers=2.1,sec=ntlmssp vers=1.0,sec=ntlm vers=3.0,sec=ntlmv2 vers=1.0,sec=ntlmv2 vers=3.0,sec=ntlm vers=2.0,sec=ntlmssp"
MOUNTCIFSPRAE="-v -t cifs -o "
MOUNTCIFSOPTS="uid=$(id --user "$LOCALUSER"),gid=$(id --group "$LOCALUSER"),forceuid,forcegid,file_mode=0700,dir_mode=0700,nobrl,noacl"
MOUNTNFSPRAE="-v -t nfs4 -o "
MOUNTNFSOPTS="rw,nosuid,nodev,nolock,intr,hard,sloppy"

NATADDR=$(head -n 1 "$CONFIGFILE" | cut -f 1 -d$'\t')
PORT=$(head -n 1 "$CONFIGFILE" | cut -f 2 -d$'\t')
SCHLUESSEL=$(head -n 1 "$CONFIGFILE" | cut -f 4 -d$'\t')
GLOBALDOMAINUSER=$(head -n 1 "$CONFIGFILE" | cut -f 5 -d$'\t')
GLOBALDOMAIN=$(echo "$GLOBALDOMAINUSER" | cut -d '\' -f 1)
GLOBALUSER=$(echo "$GLOBALDOMAINUSER" | cut -d '\' -f 2)

##### Remap #######################
REMAPMODE=$(grep '^remapMode=' "$SLXCONFIGFILE" | cut -d '=' -f 2)
CREATEMISSING=$(grep '^createMissingRemap=' "$SLXCONFIGFILE" | cut -d '=' -f 2)

##### Script (Autostart & admin) #########
SCRIPTUSER=$(grep '^scriptUser=' "$SLXCONFIGFILE" | cut -d '=' -f 2)
SCRIPTVISIBILITY=$(grep '^scriptVisibility=' "$SLXCONFIGFILE" | cut -d '=' -f 2)
SCRIPTEXT=$(grep scriptExt "$DISKMOUNTDIR/openslx.ini" | cut -f 2 -d "=")

##### Sound #######################
MUTESOUND=$(grep muteSound "$DISKMOUNTDIR/openslx.ini" | cut -f 2 -d "=")
SOUNDVOL="100%"


### Variablen Ende ################################
