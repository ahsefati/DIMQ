#!/bin/sh
# Wrapper to check for custom config in $SNAP_USER_COMMON or $SNAP_COMMON and
# use it otherwise fall back to the included basic config which will at least
# allow dimq to run and do something.
# This script will also copy the full example config in to SNAP_USER_COMMON or
# SNAP_COMMON so that people can refer to it.
#
# The decision about whether to use SNAP_USER_COMMON or SNAP_COMMON is taken
# based on the user that runs the command. If the user is root, it is assumed
# that dimq is being run as a system daemon, and SNAP_COMMON will be used.
# If a non-root user runs the command, then SNAP_USER_COMMON will be used.

case "$SNAP_USER_COMMON" in
	*/root/snap/dimq/common*) COMMON=$SNAP_COMMON ;;
	*)                             COMMON=$SNAP_USER_COMMON ;;
esac

CONFIG_FILE="$SNAP/default_config.conf"
CUSTOM_CONFIG="$COMMON/dimq.conf"


# Copy the example config if it doesn't exist
if [ ! -e "$COMMON/dimq_example.conf" ]
then
  echo "Copying example config to $COMMON/dimq_example.conf"
  echo "You can create a custom config by creating a file called $CUSTOM_CONFIG"
  cp $SNAP/dimq.conf $COMMON/dimq_example.conf
fi


# Does the custom config exist?  If so use it.
if [ -e "$CUSTOM_CONFIG" ]
then
  echo "Found config in $CUSTOM_CONFIG"
  CONFIG_FILE=$CUSTOM_CONFIG
else
  echo "Using default config from $CONFIG_FILE"
fi

# Launch the snap
$SNAP/usr/sbin/dimq -c $CONFIG_FILE $@
