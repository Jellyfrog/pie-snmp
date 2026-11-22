--TEST--
DISPLAY-HINT without an octet length uses one octet (net-snmp PR #1177)
--EXTENSIONS--
snmp
--SKIPIF--
<?php
require_once(__DIR__.'/skipif.inc');
if (PHP_OS_FAMILY === 'Windows') die('xfail SNMP tests might possibly fail on Windows');
?>
--FILE--
<?php
require_once(__DIR__.'/snmp_include.inc');

snmp_set_quick_print(false);
snmp_set_valueretrieval(SNMP_VALUE_LIBRARY);

/* Only the test MIB: with UCD-SNMP-MIB loaded, its extNames (a
 * DisplayString) would be used instead. */
snmp_init_mib(null, []);
var_dump(snmp_read_mib(__DIR__.'/PHP-SNMP-HINT-TEST-MIB.txt'));

/* UCD-SNMP-MIB::extNames.1 is "HexTest" (the exec line in snmpd.conf). The
 * test MIB gives it DISPLAY-HINT "t", a spec without an octet length, which
 * prints one octet at a time instead of an empty string. */
var_dump(snmp2_get($hostname, $community, '.1.3.6.1.4.1.2021.8.1.2.1', $timeout, $retries));
?>
--EXPECT--
bool(true)
string(15) "STRING: HexTest"
