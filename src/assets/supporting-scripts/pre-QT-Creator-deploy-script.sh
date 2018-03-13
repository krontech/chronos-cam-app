#!/bin/busybox ash
set -eu #Strict mode. Catch errors early. Note: -o pipefail not supported.

#Pre-deploy script for the camApp. These following needs to be in its own
#file or else stdout gets cut off early in QT Creator

cd /opt/camera/assets/supporting-scripts/
./shut-down-camApp.sh & ./back-up-camApp.sh