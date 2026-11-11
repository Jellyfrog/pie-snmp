# snmp: PHP SNMP extension for PHP 8.5+

This is php-src's `ext/snmp`, taken from master (bd8e73473ee) and packaged
for [PIE](https://github.com/php/pie). It builds on PHP 8.5 and newer.

It includes the PHP 8.6 SNMP improvements: the `Snmp\OidOutput`,
`Snmp\StringOutput`, `Snmp\Output` and `Snmp\Mib` enums,
`snmp_set_output_option()`, `snmp_set_mib_option()`, `snmp_init_mib()`,
AES-192/256 (including the Cisco variants) and the extra output properties.

It also adds the following, which upstream does not have:

| Feature | API |
|---|---|
| Turn off GETBULK in walks (GETNEXT is used instead) | `SNMP::$use_bulk` (default `true`) |

SNMPv3 algorithms (`setSecurity()`, `snmp3_*()`):

| | |
|---|---|
| Authentication | `MD5`, `SHA`, `SHA256`, `SHA512` |
| Privacy | `DES`, `AES` (= `AES128`), `AES192`, `AES256`, `AES192C`, `AES256C` (Cisco key extension) |

`phpversion('snmp')` returns this build's version (e.g. `0.0.1`, the release
tag without the `v`), not the PHP version. To detect this build, check
`defined('SNMP_LIBRENMS_VERSION')`; the constant holds the same version.

## Install

Net-SNMP is bundled: `net-snmp/` is a git submodule (upstream master), and
`./configure` builds it as a static, PIC library-only `libnetsnmp.a`, applies
any `patches/*.patch` first, and links it into `snmp.so`. The Net-SNMP
symbols are kept private to `snmp.so`. No system Net-SNMP package is needed;
OpenSSL is still linked dynamically.

Build dependencies: a C compiler, `make`, the PHP dev files (`phpize`) and
the OpenSSL dev files (Debian/Ubuntu: `build-essential php8.5-dev libssl-dev`).
The MIB files are not bundled; install them separately (e.g.
`snmp-mibs-downloader` or `snmp`) if you use MIB names. The library looks in
the standard places (`/usr/share/snmp/mibs`, `/etc/snmp/snmp.conf`).
Extra Net-SNMP configure arguments can be passed in `NETSNMP_CONFIGURE_ARGS`.

The snmp extension shipped with PHP must not also be loaded. On
distributions that ship it as a separate package (e.g. Arch `php-snmp`,
Debian `php8.5-snmp`), remove that package or disable its ini file first.

```sh
pie repository:add vcs https://github.com/jellyfrog/pie-snmp
pie install jellyfrog/snmp
```

PIE installs from the release assets: a prebuilt `snmp.so` for Linux
(x86_64/arm64, glibc, NTS/ZTS) when one matches, otherwise the source
package `php_snmp-<version>-src.tgz`, which includes the Net-SNMP submodule
(GitHub's own source archives do not). The assets are built by
`.github/workflows/release.yml` when a tag is pushed. Unreleased versions
(`dev-main`) cannot be installed with PIE; build them manually.

Manual build:

```sh
git clone --recurse-submodules https://github.com/jellyfrog/pie-snmp
cd pie-snmp
phpize && ./configure && make
php -d extension=$PWD/modules/snmp.so -m | grep snmp
```

## Tests

The tests need an SNMP agent configured with `tests/snmpd.conf`. To run one
without root, change the port in `agentaddress` (e.g. to 1161) and the path
to `bigtest`, then:

```sh
snmpd -f -Le -C -c ./snmpd.conf --persistentDir=/tmp/snmpd-persist
SNMP_PORT=1161 NO_INTERACTION=1 make test TESTS=tests/
```

## Known issues

- Net-SNMP caches USM users per process. If one security name is used with
  different security parameters in the same process, later requests may use
  the cached keys.
