# snmp: PHP SNMP extension for PHP 8.5+

This is php-src's `ext/snmp`, taken from master (bd8e73473ee) and packaged
for [PIE](https://github.com/php/pie). It builds on PHP 8.5 and newer.

It includes the PHP 8.6 SNMP improvements: the `Snmp\OidOutput`,
`Snmp\StringOutput`, `Snmp\Output` and `Snmp\Mib` enums,
`snmp_set_output_option()`, `snmp_set_mib_option()`, `snmp_init_mib()`,
AES-192/256 and the extra output properties.

It also adds the following, which upstream does not have:

| Feature | API |
|---|---|
| Turn off GETBULK in walks (GETNEXT is used instead) | `SNMP::$use_bulk` (default `true`) |
| SHA-224 and SHA-384 SNMPv3 authentication | `"SHA224"`, `"SHA384"` in `setSecurity()` / `snmp3_*()` |
| Don't apply MIB display hints (Net-SNMP `-Ih`) | `SNMP::$no_display_hint`, `Snmp\Output::NoDisplayHint` |
| TCP transport | `tcp:host[:port]`, `tcp6:[addr][:port]` (and `udp:`, `udp6:`) in the hostname |

`phpversion('snmp')` returns `1.0.0`, not the PHP version. Use that, or
`property_exists(SNMP::class, 'use_bulk')`, to detect this build.

## Install

You need the Net-SNMP development files (`net-snmp-config`), and the
bundled snmp extension must not also be loaded. On distributions that ship
it as a separate package (e.g. Arch `php-snmp`, Debian `php8.5-snmp`),
remove that package or disable its ini file first.

```sh
pie repository:add vcs https://github.com/<you>/php-ext-snmp
pie install jellyfrog/snmp:dev-main
```

To use a non-default Net-SNMP prefix: `pie install jellyfrog/snmp --with-snmp=/opt/net-snmp`.

Manual build:

```sh
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

- **TCP connect ignores the SNMP timeout.** Net-SNMP's TCP transport calls a
  blocking `connect()`. For an unreachable host, the first request on Linux
  waits for the kernel's SYN retries, about 2 minutes, whatever `$timeout` is
  set to. Refused connections fail immediately.
- Net-SNMP caches USM users per process. If one security name is used with
  different security parameters in the same process, later requests may use
  the cached keys.
- Against Net-SNMP 5.9.5 snmpd with `tests/snmpd.conf`, SNMPv2c over
  `tcp6:` times out while SNMPv3 over `tcp6:` works. The same happens with
  the `snmpget` CLI, so the cause is on the agent side.

This build also works around a Net-SNMP 5.9.5 bug (net-snmp issue #1055)
that broke the second request on a reused `SNMP` object.
