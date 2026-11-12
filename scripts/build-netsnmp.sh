#!/bin/sh
# Build the bundled Net-SNMP library (net-snmp/ submodule + patches/*.patch)
# as a static, position-independent libnetsnmp.a for linking into snmp.so.
#
# Usage: build-netsnmp.sh SRCDIR BUILDDIR
#   SRCDIR    the extension source directory (contains net-snmp/ and patches/)
#   BUILDDIR  where to build; the result is BUILDDIR/netsnmp-build
#
# Rebuilds only when the Net-SNMP source, the patches or this script change.

set -eu

srcdir=$1
builddir=$2
netsnmp_src="$srcdir/net-snmp"
patchdir="$srcdir/patches"
out="$builddir/netsnmp-build"
stamp="$out/.php-snmp-stamp"

if [ ! -f "$netsnmp_src/configure" ]; then
    echo "error: $netsnmp_src is empty; run: git submodule update --init" >&2
    exit 1
fi

# Fingerprint of everything that affects the build.
fingerprint=$(
    {
        git -C "$netsnmp_src" rev-parse HEAD 2>/dev/null || echo no-git
        git -C "$netsnmp_src" status --porcelain --untracked-files=no 2>/dev/null || true
        cat "$0"
        for p in "$patchdir"/*.patch; do
            [ -f "$p" ] && cat "$p"
        done
    } | cksum
)

if [ -f "$stamp" ] && [ "$(cat "$stamp")" = "$fingerprint" ] \
    && [ -f "$out/snmplib/.libs/libnetsnmp.a" ]; then
    echo "bundled net-snmp is up to date"
    exit 0
fi

rm -rf "$out"
mkdir -p "$out"
# Copy the source so the submodule stays clean and patches can be applied.
(cd "$netsnmp_src" && tar --exclude=.git -cf - .) | (cd "$out" && tar -xf -)

for p in "$patchdir"/*.patch; do
    [ -f "$p" ] || continue
    echo "applying $(basename "$p")"
    patch -d "$out" -p1 -N --batch < "$p"
done

cd "$out"
# Without the agent, Net-SNMP derives an empty default MIB list from the
# (absent) agent modules. Use the list a stock build loads instead.
default_mibs="SNMPv2-MIB:IF-MIB:IP-MIB:TCP-MIB:UDP-MIB:HOST-RESOURCES-MIB:\
NOTIFICATION-LOG-MIB:DISMAN-EVENT-MIB:DISMAN-SCHEDULE-MIB:HOST-RESOURCES-TYPES:\
UCD-SNMP-MIB:UCD-DEMO-MIB:SNMP-TARGET-MIB:NET-SNMP-AGENT-MIB:\
SNMP-FRAMEWORK-MIB:SNMP-MPD-MIB:SNMP-USER-BASED-SM-MIB:SNMP-VIEW-BASED-ACM-MIB:\
SNMP-COMMUNITY-MIB:IPV6-ICMP-MIB:IPV6-MIB:IPV6-TCP-MIB:IPV6-UDP-MIB:\
IP-FORWARD-MIB:NET-SNMP-PASS-MIB:NET-SNMP-EXTEND-MIB:UCD-DLMOD-MIB:\
SNMP-NOTIFICATION-MIB:SNMPv2-TM:NET-SNMP-VACM-MIB"
# Library only: no agent, apps, Perl or Python. The runtime paths
# (MIB dirs, snmp.conf, persistent dir) match a normal /usr install so
# the system MIBs and config are found.
CFLAGS="${NETSNMP_CFLAGS:--O2 -g} -fPIC" ./configure \
    --prefix=/usr \
    --sysconfdir=/etc \
    --with-persistent-directory=/var/lib/net-snmp \
    --with-defaults \
    --with-mibs="$default_mibs" \
    --disable-shared \
    --enable-static \
    --with-pic \
    --disable-agent \
    --disable-applications \
    --disable-manuals \
    --disable-scripts \
    --disable-mibs \
    --disable-embedded-perl \
    --without-perl-modules \
    --without-python-modules \
    --with-openssl \
    --enable-blumenthal-aes \
    --enable-ipv6 \
    ${NETSNMP_CONFIGURE_ARGS:-} \
    > configure.out 2>&1 || { tail -50 configure.out >&2; exit 1; }

jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
make -C snmplib -j"$jobs" > make.out 2>&1 || { tail -50 make.out >&2; exit 1; }

echo "$fingerprint" > "$stamp"
echo "built bundled net-snmp in $out"
