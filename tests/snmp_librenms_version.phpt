--TEST--
SNMP_LIBRENMS_VERSION is the extension version
--EXTENSIONS--
snmp
--FILE--
<?php
var_dump(defined('SNMP_LIBRENMS_VERSION'));
var_dump(SNMP_LIBRENMS_VERSION === phpversion('snmp'));
var_dump((bool) preg_match('/^\d+\.\d+\.\d+$/', SNMP_LIBRENMS_VERSION));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
