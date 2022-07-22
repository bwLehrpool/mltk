#!/bin/ash

# Compact memory now once at start of graphical session,
# So VM will have as much contiguous blocks available
# as possible.
if [ "$PAM_TTY" = ":0" ]; then
	echo 1 > /proc/sys/vm/compact_memory
fi
exit 0
