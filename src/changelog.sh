#!/bin/sh
TAGGED=$(git describe --tags --abbrev=0)
REVISION=$(git rev-list ${TAGGED}..HEAD --count)
VERSION=$(echo $TAGGED | sed -e 's/^[^0-9]*//g' | sed -e 's/-/~/g')
if [ $REVISION -gt 0 ]; then
	VERSION="${VERSION}+${REVISION}"
fi
cat << EOF
chronos-gui (${VERSION}) unstable; urgency=medium

$(git log --oneline ${TAGGED}..HEAD | sed -e 's/^[0-9a-f]*/  \*/g')
  * Upstream release ${TAGGED}

 -- $(git config user.name) <$(git config user.email)>  $(date -R)
EOF
