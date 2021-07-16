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
pre_exec() {
	:
}

# Called when this module is active in a target. This is called for each module
# once, before any other method is called, and before any packages are installed.
# Note that is does not honor any dependency ordering of modules. Consider this as
# being called in a random order. Also, this gets called on every run, there
# is no flag to remember this has already been called in a previous run.
module_init() {
	:
}
