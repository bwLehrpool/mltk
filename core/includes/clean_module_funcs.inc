# Called when the sources for this module need to be fetched.
# Not called if fetched_source.flag exists
fetch_source() {
	:
}

# Called when the module should be built.
# Not called if build_complete.flag exists
build() {
	:
}

# Called after the relevant files from the module's build dir
# have been copied to the target dir.
post_copy() {
	:
}

# Called when this module is about to be handled, before fetching the source.
module_load() {
	:
}

