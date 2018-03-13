#!/bin/busybox ash
set -eu #Strict mode. Catch errors early. Note: -o pipefail not supported.

#Pre-deploy script for the camApp. Make a backup of the existing camApp, so that
#QT Creator does not overwrite it when it deploys. Keep a few old backups around.
#This script exists and is auto-run by QT Creator because I'll never make backups
#if it's not automatic.

cd /opt/camera
if [ -f camApp ]; then
	if [ ! -d camApp.bak ]; then
		mkdir camApp.bak
		touch "camApp.bak/smaller numbered backups are newer"
	fi

	if [ -f camApp.bak/camApp.1.bak ]; then
		iter=99 #keep 100 backed up versions - the oldest doesn't get backed up, and is overwritten
		         #This does seem like a lot, but as we have the disk space there's no reason not to use it. Plus, if something's gone truly wrong, there's probably a bunch of broken builds between finding the problem, and giving up and restoring from backup.
		while [ "$iter" -gt 0 ]; do
			if [ -f "camApp.bak/camApp.$iter.bak" ]; then
				mv "camApp.bak/camApp.$iter.bak" "camApp.bak/camApp.$(expr $iter + 1).bak"
			fi
			iter="$(( $iter - 1 ))"
		done
	fi

	cp camApp camApp.bak/camApp.1.bak
fi