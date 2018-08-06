function get_creds()
{
	# udp-Socket erzeugen Kanal erzeugen::
	exec 13<>/dev/udp/"$NATADDR"/"$PORT"
	
	# temp. Datei - wer hat Angst vor Nullbytes? Ich! Ich! Ich!
	head -n 1 "$CONFIGFILE" |cut -f3 -d$'\t' | while read -n 2 CODE; do [ -n "$CODE" ] && printf "\x$CODE"; done >"$KEYTEMP"
	# Auf Socket fuer Grundsystem schreiben:
	cat "$KEYTEMP" >&13
	
	# Serverantwort pwdaemon in temp. Datei schreiben:
	timeout 2s dd bs=$BYTES count=1 of="$RAWKEYTEMP" <&13 2>/dev/null

	# Kanal zu Socket schließen:
	exec 13<&-
	exec 13>&-
	
	# Wieviele Zeichen (Bytes 1 und 2 der Paketnutzlast)?
	ANZAHL=$((16#$(dd if=$RAWKEYTEMP bs=1 count=2 2>/dev/null|hexdump -e '1/1 "%02x"')))
	
	# Und raus-xor-en:
	PW=$( xor	"$(dd if="$RAWKEYTEMP" bs=1 skip=2 count="$ANZAHL" 2>/dev/null|hexdump -v -e '1/1 "%02x"')" \
			"$(head -n 1 "$CONFIGFILE" | cut -f 4 -d$'\t')" | sed 's/../\\x&/g')
	
	PW=$(echo -e "$PW")     # \x...-Ausdrücke zu ASCII
#	PW="${PW:0:${ANZAHL}}"  # ...und Rest nach ANZAHL abhacken.
}
