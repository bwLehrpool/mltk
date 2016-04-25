#!/bin/bash
# Needs full bash

# region header

# Copyright Torben Sickert 16.12.2012

# License
#    This library written by Torben Sickert stand under a creative commons
#    naming 3.0 unported license.
#    see http://creativecommons.org/licenses/by/3.0/deed.de

# vim: set tabstop=4 shiftwidth=4 expandtab:
# vim: foldmethod=marker foldmarker=region,endregion:

# Dependencies:

# vmware or virtualbox

# Notes:

# USE "sudo bin/vmware-vmx --new-sn JJ237-G52E2-08X0C-C3306-0WCQ1"
# To activate wmware workstation!

# Abbreviation for "createLinkedClone".
__NAME__='clc'

# endregion

# Provides the main module scope.
function clc() {

# region configuration 

    # region private properties

        # region command line arguments

        local _VERBOSE='no'
        local _SUPPORTED_HYPERVISOR=('VMware' 'virtualBox')
        # NOTE: This value will be determined automatically. If no hypervisor
        # could be detected this value will be used as default.
        # The first value from supported Machines is taken as default.
        local _HYPERVISOR="$_SUPPORTED_HYPERVISOR"
        local _PERSISTENT_SHORT_DESCRIPTION_SUFFIX=' --persistent--'

        # endregion

        local _STANDARD_OUTPUT=/dev/null
        local _ERROR_OUTPUT=/dev/null
        local _BASIC_IMAGE_CONFIGURATION_FILE_PATH=''
        local _TARGET_PATH=''
        local _CREATE_PERSISTENT_CONFIG='no'

    # endregion

# endregion

# region functions

    # region command line interface

    # Prints a description about how to use this program.
    function clcPrintUsageMessage() {
        cat << EOF
    $__NAME__ Generates a linked clone from given machine description file in
    given target location.
EOF
        return $?
    }

    # Prints a description about how to use this program by providing examples.
    function clcPrintUsageExamples() {
        cat << EOF
    # Getting a help message.
    >>> $0 --help

    # Creating a linked clone.
    >>> $0 /path/to/config.xml ~/.persistentLinkedClones/

    # Creating a linked clone in verbose mode.
    >>> $0 /path/to/config.xml ~/.persistentLinkedClones/ --verbose

    # Creating a linked clone in verbose mode with debugging output.
    >>> $0 /path/to/config.xml ~/.persistentLinkedClones/ --verbose --debug

    # Creating a linked clone in verbose mode with debugging output.
    >>> $0 /path/to/config.xml ~/.persistentLinkedClones/ -v -d
EOF
        return $?
    }

    # Prints descriptions about each available command line option.
    function clcPrintCommandLineOptionDescriptions() {
        # NOTE; All letters are used for short options.
        cat << EOF
    -h --help Shows this help message.

    -v --verbose Tells you what is going on (default: "$_VERBOSE").

    -d --debug Gives you any output from all tools which are used
        (default: "$_DEBUG").

    -c --create-persistent-config If set an xml file for persistent openslx
        boot will be created (default: "$_CREATE_PERSISTENT_CONFIG").
EOF
        return $?
    }

    # Provides a help message for this module.
    function clcPrintHelpMessage() {
        echo -e \
            "\nUsage: $0 BASIC_IMAGE_CONFIGURATION_FILE_PATH TARGET_PATH [options]\n" && \
        clcPrintUsageMessage "$@" && \
        echo -e '\nExamples:\n' && \
        clcPrintUsageExamples "$@" && \
        echo -e '\nOption descriptions:\n' && \
        clcPrintCommandLineOptionDescriptions "$@" && \
        echo && \
        return $?
    }

    # Provides the command line interface and interactive questions.
    function clcCommandLineInterface() {
        while true; do
            case "$1" in
                -h|--help)
                    shift
                    clcPrintHelpMessage "$0"
                    exit 0
                    ;;
                -v|--verbose)
                    shift
                    _VERBOSE='yes'
                    ;;
                -d|--debug)
                    shift
                    _DEBUG='yes'
                    _STANDARD_OUTPUT=/dev/stdout
                    _ERROR_OUTPUT=/dev/stderr
                    ;;
                -c|--create-persistent-config)
                    shift
                    _CREATE_PERSISTENT_CONFIG='yes'
                    ;;

                '')
                    shift
                    break 2
                    ;;
                *)
                    if [[ ! "$_BASIC_IMAGE_CONFIGURATION_FILE_PATH" ]]; then
                        _BASIC_IMAGE_CONFIGURATION_FILE_PATH="$1"
                    elif [[ ! "$_TARGET_PATH" ]]; then
                        _TARGET_PATH="$1"
                    else
                        clcLog 'critical' \
                            "Given argument: \"$1\" is not available." '\n'
                        clcPrintHelpMessage "$0"
                        return 1
                    fi
                    shift
                    ;;
            esac
        done
        if [[ ! "$_BASIC_IMAGE_CONFIGURATION_FILE_PATH" ]] || \
           [[ ! "$_TARGET_PATH" ]]; then
            clcLog 'critical' \
                "You have to provide a basic image configuration file and a destination path."
            clcPrintHelpMessage "$0"
            return 1
        fi
        local supportedVirtualMachine
        for supportedVirtualMachine in ${_SUPPORTED_HYPERVISOR[*]}; do
            if [[ "$(clcGetXMLValue 'virtualMachine' | \
                  grep --ignore-case "$supportedVirtualMachine")" ]]; then
                _HYPERVISOR="$supportedVirtualMachine"
                clcLog 'debug' "Detected \"$_HYPERVISOR\" as hypervisor."
                break
            fi
        done
        clcLog 'info' "Using \"$_HYPERVISOR\" as hypervisor." && \
        return $?
    }

    # Grabs a value from currently loaded xml file.
    function clcGetXMLValue() {
        grep --ignore-case --only-matching "<$1 param=.*" \
            "$_BASIC_IMAGE_CONFIGURATION_FILE_PATH" | awk -F '"' '{ print $2 }'
        return $?
    }

    # Handles logging messages. Returns non zero and exit on log level error to
    # support chaining the message into toolchain.
    function clcLog() {
        local loggingType='info'
        local message="$1"
        if [ "$2" ]; then
            loggingType="$1"
            message="$2"
        fi
        if [ "$_VERBOSE" == 'yes' ] || [ "$loggingType" == 'error' ] || \
           [ "$loggingType" == 'critical' ]; then
            if [ "$3" ]; then
                echo -e -n "$3"
            fi
            echo -e "${loggingType}: $message"
        fi
        if [ "$loggingType" == 'error' ]; then
            exit 1
        fi
    }

    # endregion

    # region tools

    # Returns the minimal vmx vmware configuration file content to create a
    # snapshot.
    function clcGetTemporaryVMXContent() {
        cat << EOF
.encoding = "UTF-8"
config.version = "8"
virtualHW.version = "7"
ide0:0.present = "TRUE"
ide0:0.fileName = "$1"
displayName = ""
EOF
        return $?
    }

    # Creates a snapshot from VMware generated virtual machine.
    function clcCreateVMwareSnapshot() {
        local temporaryConfigurationPath="$(mktemp --directory)/" \
            1>"$_STANDARD_OUTPUT" 2>"$_ERROR_OUTPUT" && \
        local temporaryConfigurationFilePath="$(mktemp --suffix '.vmx')" \
            1>"$_STANDARD_OUTPUT" 2>"$_ERROR_OUTPUT" && \
        clcGetTemporaryVMXContent "/var/lib/virt/vmware/$(clcGetXMLValue 'image_name')" \
            1>"$temporaryConfigurationFilePath" 2>"$_ERROR_OUTPUT" && \
        mv "$temporaryConfigurationFilePath" "$temporaryConfigurationPath" \
            1>"$_STANDARD_OUTPUT" 2>"$_ERROR_OUTPUT" && \
        clcLog "Needed files generated in \"$temporaryConfigurationPath\" generated." && \
        vmrun snapshot "$temporaryConfigurationPath"*.vmx \
            persistentUserSnapshot 1>"$_STANDARD_OUTPUT" \
            2>"$_ERROR_OUTPUT" && \
        mv "$temporaryConfigurationPath"*.vmdk "$_TARGET_PATH" \
            1>"$_STANDARD_OUTPUT" 2>"$_ERROR_OUTPUT"
        local result=$?
        if [[ "$_DEBUG" == 'no' ]]; then
            rm --recursive "$temporaryConfigurationPath" 1>"$_STANDARD_OUTPUT" \
                2>"$_ERROR_OUTPUT"
        fi
        return $result
    }

    # Creates a snapshot from virtualBox generated virtual machine.
    function clcCreateVirtualBoxSnapshot() {
        local temporaryConfigurationPath="$(mktemp --directory)" \
            1>"$_STANDARD_OUTPUT" 2>"$_ERROR_OUTPUT" && \
        VBoxManage clonevm TODO ->(VMNAME) --snapshot base --options link \\
            --basefolder "$temporaryConfigurationPath" \
            1>"$_STANDARD_OUTPUT" 2>"$_ERROR_OUTPUT" && \
        mv "${temporaryConfigurationPath}/Snapshots/"*.vmdk "$_TARGET_PATH" \
            1>"$_STANDARD_OUTPUT" 2>"$_ERROR_OUTPUT" && \
        rm --recursive "$temporaryConfigurationPath" 1>"$_STANDARD_OUTPUT" \
            2>"$_ERROR_OUTPUT" && \
        return $?
    }

    # Creates a persistent version of given config file.
    function clcCreatePersistentConfig() {
        cp "$_BASIC_IMAGE_CONFIGURATION_FILE_PATH" "$_TARGET_PATH" && \
            1>"$_STANDARD_OUTPUT" 2>"$_ERROR_OUTPUT" && \
        sed --in-place --regexp-extended \
            "s/(< *short_description[^>]*param=\"[^\"]*)(\")/\\1$_PERSISTENT_SHORT_DESCRIPTION_SUFFIX\\2/g" \
            "$_TARGET_PATH" 1>"$_STANDARD_OUTPUT" 2>"$_ERROR_OUTPUT"
        return $?
    }

    # endregion

# endregion

# region controller

    clcCommandLineInterface "$@" || return $?
    if [[ "$_CREATE_PERSISTENT_CONFIG" == 'yes' ]]; then
        clcCreatePersistentConfig || \
            clcLog 'error' 'Creating persitent config failed.'
    else
        "clcCreate${_HYPERVISOR}Snapshot" || \
            clcLog 'error' 'Creating Linked Clone failed.'
    fi
    clcLog 'Program has successfully finished.' && \
    return $?

# endregion

}

# region footer

if [[ "$0" == *"${__NAME__}.bash" ]]; then
    "$__NAME__" "$@"
fi

# endregion
