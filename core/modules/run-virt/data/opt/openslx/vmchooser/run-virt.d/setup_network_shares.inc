# TODO this currently is not used.
# This should be continued when we have a clear idea
# how we will embed network shares in VMs
set_network_shares() {

	# Add rw share for home dir
	homesharepath="${HOME}/PERSISTENT"
	homesharename="home"

	# Add common share
	commonsharepath="${HOME}/SHARE"
	commonsharename="share"
}

reg_feature_handler "netshares" "set_network_shares"

