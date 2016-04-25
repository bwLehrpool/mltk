
# usage: VAR=$(trim "   string ")
trim() {
	local var=$1
	var="${var#"${var%%[![:space:]]*}"}"   # remove leading whitespace characters
	var="${var%"${var##*[![:space:]]}"}"   # remove trailing whitespace characters
	echo -n "$var"
}

# Inline version of trim, use when piping
itrim () {
	sed -r 's/^\s+//g;s/\s+$//g'
}

escape_search() {
	sed -e 's/[]\/()$*.^|[]/\\&/g'

}

escape_replace() {
	sed -e 's/[\/&]/\\&/g'
}

tolower () {
	tr '[A-Z]' '[a-z]'
}

toupper () {
	tr '[a-z]' '[A-Z]'
}

