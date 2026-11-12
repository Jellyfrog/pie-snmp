--TEST--
snmp_translate()
--EXTENSIONS--
snmp
--SKIPIF--
<?php
if (snmp_translate('SNMPv2-MIB::sysDescr.0', Snmp\OidOutput::Numeric) !== '.1.3.6.1.2.1.1.1.0') die('skip SNMPv2-MIB not available');
?>
--FILE--
<?php
foreach (['sysDescr.0', 'SNMPv2-MIB::sysDescr.0', '.1.3.6.1.2.1.1.1.0', '1.3.6.1.2.1.1.1.0'] as $oid) {
    foreach (Snmp\OidOutput::cases() as $format) {
        if ($format === Snmp\OidOutput::None) {
            continue;
        }
        echo $oid, ' ', $format->name, ': ', snmp_translate($oid, $format), "\n";
    }
}

echo "Default uses the global format\n";
snmp_set_oid_output_format(Snmp\OidOutput::Numeric);
var_dump(snmp_translate('sysDescr.0'));
var_dump(snmp_translate('sysDescr.0', Snmp\OidOutput::Suffix));
echo "Global format restored\n";
var_dump(snmp_translate('sysDescr.0'));

echo "Array keeps keys\n";
var_dump(snmp_translate(['a' => 'sysName.0', 7 => 'NO-SUCH-MIB::foo', 8 => '.1.3.6.1.2.1.1.5.0'], Snmp\OidOutput::Module));

echo "Invalid\n";
var_dump(snmp_translate('noSuchObjectAnywhere'));

try {
    snmp_translate(['sysName.0', 42]);
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
sysDescr.0 Suffix: sysDescr.0
sysDescr.0 Module: SNMPv2-MIB::sysDescr.0
sysDescr.0 Full: .iso.org.dod.internet.mgmt.mib-2.system.sysDescr.0
sysDescr.0 Numeric: .1.3.6.1.2.1.1.1.0
sysDescr.0 Ucd: system.sysDescr.0
SNMPv2-MIB::sysDescr.0 Suffix: sysDescr.0
SNMPv2-MIB::sysDescr.0 Module: SNMPv2-MIB::sysDescr.0
SNMPv2-MIB::sysDescr.0 Full: .iso.org.dod.internet.mgmt.mib-2.system.sysDescr.0
SNMPv2-MIB::sysDescr.0 Numeric: .1.3.6.1.2.1.1.1.0
SNMPv2-MIB::sysDescr.0 Ucd: system.sysDescr.0
.1.3.6.1.2.1.1.1.0 Suffix: sysDescr.0
.1.3.6.1.2.1.1.1.0 Module: SNMPv2-MIB::sysDescr.0
.1.3.6.1.2.1.1.1.0 Full: .iso.org.dod.internet.mgmt.mib-2.system.sysDescr.0
.1.3.6.1.2.1.1.1.0 Numeric: .1.3.6.1.2.1.1.1.0
.1.3.6.1.2.1.1.1.0 Ucd: system.sysDescr.0
1.3.6.1.2.1.1.1.0 Suffix: sysDescr.0
1.3.6.1.2.1.1.1.0 Module: SNMPv2-MIB::sysDescr.0
1.3.6.1.2.1.1.1.0 Full: .iso.org.dod.internet.mgmt.mib-2.system.sysDescr.0
1.3.6.1.2.1.1.1.0 Numeric: .1.3.6.1.2.1.1.1.0
1.3.6.1.2.1.1.1.0 Ucd: system.sysDescr.0
Default uses the global format
string(18) ".1.3.6.1.2.1.1.1.0"
string(10) "sysDescr.0"
Global format restored
string(18) ".1.3.6.1.2.1.1.1.0"
Array keeps keys
array(3) {
  ["a"]=>
  string(21) "SNMPv2-MIB::sysName.0"
  [7]=>
  bool(false)
  [8]=>
  string(21) "SNMPv2-MIB::sysName.0"
}
Invalid
bool(false)
snmp_translate(): Argument #1 ($object_id) must contain only strings, int given
