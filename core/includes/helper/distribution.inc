# Get simple distribution name
# This is also being used via ./openslx, not only via ./mltk,
# so it has not been moved to remote/includes.
SYS_DISTRIBUTION=$(lsb_release -is | tr '[A-Z]' '[a-z]' | sed -r 's/[^a-z0-9]//g;s/project$//g;s/scientificsl$/scientific/g')
