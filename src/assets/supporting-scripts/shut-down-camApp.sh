#!/bin/busybox ash
set -eu #Strict mode. Catch errors early. Note: -o pipefail not supported.

#Pre-deploy script for the camApp. Qt Creator pushes a new version of the camApp, but
#it will not work if the camApp is already running. However, if we just force-quit
#the camApp, then the fpga is left in an inconsistent state. However, we can't just
#quit the camApp and expect it to work, because sometimes it's frozen and will never
#exit gracefully. This script tries to solve that problem by waiting a little while,
#and then restarting the camera if the camApp isn't responsive.

camAppPID="$(pidof camApp || true)"
if [ -n "$camAppPID" ]; then
	killall camApp;
	
	iter=0
	while [ -n "$camAppPID" ] && [ "$iter" -lt 50 ]; do #Wait at least 1 second for the camApp to gracefully close itself.
		sleep 0.01
		camAppPID="$(pidof camApp || true)"
		iter="$(( $iter + 1))"
	done

	if [ -n "$camAppPID" ]; then #If the camApp is still running now, it must be frozen. Kill it ungracefully.
		killall -KILL camApp
		echo "WARNING - camApp frozen - video will not work until after camera reboot"
		wait 0.1
	fi
fi