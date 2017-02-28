#!/bin/bash
# TODO this currently is not used.
# This should be continued when we have a clear idea
# how we will embed network shares in VMs
set_network_shares() {

	# Add rw share for home dir
	HOME_SHARE_PATH="${HOME}/PERSISTENT"
	HOME_SHARE_NAME="home"

	# Add common share
	COMMON_SHARE_PATH="${HOME}/SHARE"
	COMMON_SHARE_NAME="share"
}

reg_feature_handler "netshares" "set_network_shares"

