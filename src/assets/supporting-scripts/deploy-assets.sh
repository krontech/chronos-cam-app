#!/bin/bash
set -euo pipefail #Strict mode. Catch errors early.

#Pre-deploy script for the camApp. Upload the camApp external assets to the
#camera. Note: This includes deploy scripts as well as run-time assets.

scp -roKexAlgorithms=+diffie-hellman-group1-sha1 \
	"./assets/" "root@192.168.12.1:/opt/camera/"