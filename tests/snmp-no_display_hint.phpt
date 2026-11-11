--TEST--
SNMP::$no_display_hint and Snmp\Output::NoDisplayHint
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

/* HOST-RESOURCES-MIB::hrSystemDate.0 is a DateAndTime, which has a
 * DISPLAY-HINT. A new object is used per request. */
$oid = '.1.3.6.1.2.1.25.1.2.0';

snmp_set_valueretrieval(SNMP_VALUE_LIBRARY);
snmp_set_quick_print(false);

$s = new SNMP(SNMP::VERSION_2c, $hostname, $community, $timeout, $retries);
var_dump($s->no_display_hint);
echo $s->get($oid), "\n";

$s = new SNMP(SNMP::VERSION_2c, $hostname, $community, $timeout, $retries);
$s->no_display_hint = true;
var_dump($s->no_display_hint);
echo $s->get($oid), "\n";

echo "Object setting does not leak into global state\n";
echo snmp2_get($hostname, $community, $oid, $timeout, $retries), "\n";

echo "Global option\n";
snmp_set_output_option(Snmp\Output::NoDisplayHint, true);
echo snmp2_get($hostname, $community, $oid, $timeout, $retries), "\n";
$s = new SNMP(SNMP::VERSION_2c, $hostname, $community, $timeout, $retries);
var_dump($s->no_display_hint);
snmp_set_output_option(Snmp\Output::NoDisplayHint, false);
echo snmp2_get($hostname, $community, $oid, $timeout, $retries), "\n";
?>
--EXPECTF--
bool(false)
STRING: %d-%d-%d,%d:%d:%d.%d,%s
bool(true)
Hex-STRING: %x %x %x %x %x %x %x %x%A
Object setting does not leak into global state
STRING: %d-%d-%d,%d:%d:%d.%d,%s
Global option
Hex-STRING: %x %x %x %x %x %x %x %x%A
bool(true)
STRING: %d-%d-%d,%d:%d:%d.%d,%s
