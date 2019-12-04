#!/bin/bash
# Do all of these things from the git checkout.
cd $(dirname ${BASH_SOURCE[0]})

# Get the source name from the git remote.
GITREMOTE=$(git remote get-url origin)
DEBSOURCE=$(basename ${GITREMOTE} | cut -d. -f1)

# Get the user name and e-mail from the last commit.
DEBFULLNAME=$(git log -1 --format='%cn')
DEBEMAIL=$(git log -1 --format='%ce')

# Generate the Version Number from the most recent tag.
TAGGED=$(git describe --tags --abbrev=0)
REVISION=$(git rev-list ${TAGGED}..HEAD --count)
VERSION=$(echo $TAGGED | sed -e 's/^[^0-9]*//g' | sed -e 's/-/~/g')
if [ $REVISION -gt 0 ]; then
	VERSION="${VERSION}${REVISION}"
fi

# Convert a list of change descriptions (one per line) and convert
# them into a bullet-point list with sensible word-wrapping.
gen_changelog() {
	while read -r change; do
		echo -n "  * "
		echo "$change" | fold -s -w 100 | sed '1 ! s/^/    /g' 
	done
}

# Output the Debian changelog
cat << EOF
${DEBSOURCE} (${VERSION}) unstable; urgency=medium

$(git log --oneline ${TAGGED}..HEAD | sed -e 's/^[0-9a-f]*\s*//g' | gen_changelog)
  * Upstream release ${TAGGED}

 -- ${DEBFULLNAME} <${DEBEMAIL}>  $(date -R)
EOF

