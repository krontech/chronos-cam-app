#!/bin/bash
VERS=$(git describe --always --tags --dirty)
cat << EOF
const char *git_version_str = "${VERS}";
EOF
