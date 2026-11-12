--TEST--
snmp_init_mib() with a list of modules
--EXTENSIONS--
snmp
--SKIPIF--
<?php
if (@snmp_translate('HOST-RESOURCES-MIB::hrSystemUptime.0', Snmp\OidOutput::Numeric) === false) die('skip default MIBs not available');
?>
--FILE--
<?php
function show(string $label): void {
    echo $label, ': ', json_encode([
        @snmp_translate('sysDescr.0'),
        @snmp_translate('ifDescr.1'),
        @snmp_translate('hrSystemUptime.0'),
    ]), "\n";
}

show('default');

snmp_init_mib(null, ['IF-MIB']);
show('IF-MIB');

snmp_init_mib(null, ['IF-MIB', 'HOST-RESOURCES-MIB']);
show('IF-MIB, HOST-RESOURCES-MIB');

snmp_init_mib(mibs: []);
show('empty list');

snmp_init_mib();
show('default again');

var_dump(getenv('MIBS'));

try {
    snmp_init_mib(null, ['IF-MIB:HOST-RESOURCES-MIB']);
} catch (ValueError $e) {
    echo $e->getMessage(), "\n";
}
try {
    snmp_init_mib(null, ['']);
} catch (ValueError $e) {
    echo $e->getMessage(), "\n";
}
try {
    snmp_init_mib(null, [42]);
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}
?>
--EXPECT--
default: ["SNMPv2-MIB::sysDescr.0","IF-MIB::ifDescr.1","HOST-RESOURCES-MIB::hrSystemUptime.0"]
IF-MIB: ["SNMPv2-MIB::sysDescr.0","IF-MIB::ifDescr.1",false]
IF-MIB, HOST-RESOURCES-MIB: ["SNMPv2-MIB::sysDescr.0","IF-MIB::ifDescr.1","HOST-RESOURCES-MIB::hrSystemUptime.0"]
empty list: [false,false,false]
default again: ["SNMPv2-MIB::sysDescr.0","IF-MIB::ifDescr.1","HOST-RESOURCES-MIB::hrSystemUptime.0"]
bool(false)
snmp_init_mib(): Argument #2 ($mibs) must contain only module names, got "IF-MIB:HOST-RESOURCES-MIB"
snmp_init_mib(): Argument #2 ($mibs) must contain only module names, got ""
snmp_init_mib(): Argument #2 ($mibs) must contain only strings, int given
