#!/bin/bash
SRCDIR="$(cd $(dirname ${BASH_SOURCE[0]})/.. && pwd)"
# Get the version number from git.
if [ -d "${SRCDIR}/.git" ]; then
    VERS=$(cd ${SRCDIR} && git describe --always --tags --dirty)
elif [ -e "${SRCDIR}/debian/changelog" ]; then
    VERS=$(head -1 ${SRCDIR}/debian/changelog | awk '{print $2}')
else
    VERS="unknown"
fi
cat << EOF
const char *git_version_str = "${VERS}";
EOF

