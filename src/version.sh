#!/bin/bash
SRCDIR="$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)"
VERS=$(cd ${SRCDIR} && git describe --always --tags --dirty)
cat << EOF
const char *git_version_str = "${VERS}";
EOF

