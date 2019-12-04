#!/bin/sh
# Check for required arguments.
if [ $# -lt 1 ]; then
   echo "Missing argument: CHANGEFILE" >&2
   echo "" >&2
   echo "Usage: $1 CHANGEFILE" >&2
fi

# Parse out the package details
RELEASE=unstable
PACKAGE=$(basename $1)
PKGNAME=$(echo ${PACKAGE} | awk -F_ '{print $1}')
PKGVERS=$(echo ${PACKAGE} | awk -F_ '{print $2}')

# Update the packages if the version has changed.
OLDVERS=$(reprepro list ${RELEASE} ${PKGNAME} | awk '{print $3}')
if [ -z "${OLDVERS}" ]; then
   echo "Installing new package ${PKGNAME} version ${PKGVERS}"
elif [ "${OLDVERS}" != "${PKGVERS}" ]; then
   echo "Upgrading ${PKGNAME} from version ${OLDVERS} to ${PKGVERS}"
else
   echo "Package version ${PKGVERS} unchanged. Exiting..."
   exit 0
fi   
reprepro include ${RELEASE} $1

