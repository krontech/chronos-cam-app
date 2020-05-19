#!/bin/bash
set -euf -o pipefail

ssh root@192.168.12.1 "cat /dev/fb0 | head --bytes=1536000" | #Use head to limit fb0 to actual screen data, convert gets very confused otherwise. Use remote's head because ssh dies with 255 otherwise.
convert -size 800x480 -depth 8 bgra:- ~/Work/Chronos2.1/screenshot.png

