#!/bin/bash
# Copy the debian directory if different
SRCDIR="$(cd $(dirname ${BASH_SOURCE[0]})/.. && pwd)"
if [ "${SRCDIR}" != "$(pwd)" ]; then
    rm -rfv $(pwd)/debian
    cp -r ${SRCDIR}/debian $(pwd)
fi

# Lookup the sysroot from the makefile.
QT_CFLAGS=$(make -f Makefile -f - print <<< 'print: ; @echo \$(CFLAGS)')
QT_SYSROOT=$(echo ${QT_CFLAGS} | tr '[:space:]' '\n' | sed -n 's/--sysroot=\(.*\)/\1/p')

# Update the rules file to just to simple packaging of qmake's output
cat << EOF > $(pwd)/debian/rules
#!/usr/bin/make -f
# See debhelper(7) (uncomment to enable)
# output every command that modifies files on the build system.
#export DH_VERBOSE = 1

export INSTALL_ROOT=\$(CURDIR)/debian/chronos-gui

%:
	dh \$@ --with systemd

override_dh_auto_clean:

override_dh_shlibdeps:
	dh_shlibdeps -- --ignore-missing-info \
		-l${QT_SYSROOT}/lib/\${DEB_HOST_MULTIARCH} \
		-l${QT_SYSROOT}/usr/lib/\${DEB_HOST_MULTIARCH}
EOF

# Update the changelog
${SRCDIR}/scripts/debchangelog.sh > debian/changelog

# Build the debian package.
debuild -d -b -us -uc --host-arch=armel

