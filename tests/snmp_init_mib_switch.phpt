--TEST--
snmp_init_mib() switching between module lists gives the same tree as a fresh build
--EXTENSIONS--
snmp
--SKIPIF--
<?php
snmp_init_mib(null, ['RFC1213-MIB', 'SNMPv2-MIB', 'IF-MIB', 'HOST-RESOURCES-MIB']);
foreach (['RFC1213-MIB::ipForwarding.0', 'SNMPv2-MIB::sysORLastChange.0', 'IF-MIB::ifHCInOctets.1', 'HOST-RESOURCES-MIB::hrSystemUptime.0'] as $oid) {
    if (snmp_translate($oid, Snmp\OidOutput::Numeric) === false) die("skip $oid not available");
}
?>
--FILE--
<?php
// Lists switched in place reuse loaded modules. Every result below is what
// a fresh build of the same list gives. RFC1213-MIB and SNMPv2-MIB both
// define sysDescr: the module read first owns the node.
$lists = [
    ['RFC1213-MIB'],
    ['SNMPv2-MIB', 'RFC1213-MIB'],
    ['RFC1213-MIB'],
    ['IF-MIB'],
    ['SNMPv2-MIB'],
    ['HOST-RESOURCES-MIB', 'IF-MIB'],
    ['IF-MIB'],
    [],
    ['IF-MIB'],
];
foreach ($lists as $list) {
    snmp_init_mib(null, $list);
    echo str_pad(implode(',', $list) ?: '(none)', 30), json_encode([
        snmp_translate('.1.3.6.1.2.1.1.1.0', Snmp\OidOutput::Module),
        snmp_translate('ifDescr.1'),
        snmp_translate('ifHCInOctets.1'),
        snmp_translate('hrSystemUptime.0'),
    ]), "\n";
}
?>
--EXPECT--
RFC1213-MIB                   ["RFC1213-MIB::sysDescr.0","RFC1213-MIB::ifDescr.1",false,false]
SNMPv2-MIB,RFC1213-MIB        ["SNMPv2-MIB::sysDescr.0","RFC1213-MIB::ifDescr.1",false,false]
RFC1213-MIB                   ["RFC1213-MIB::sysDescr.0","RFC1213-MIB::ifDescr.1",false,false]
IF-MIB                        ["SNMPv2-MIB::sysDescr.0","IF-MIB::ifDescr.1","IF-MIB::ifHCInOctets.1",false]
SNMPv2-MIB                    ["SNMPv2-MIB::sysDescr.0",false,false,false]
HOST-RESOURCES-MIB,IF-MIB     ["SNMPv2-MIB::sysDescr.0","IF-MIB::ifDescr.1","IF-MIB::ifHCInOctets.1","HOST-RESOURCES-MIB::hrSystemUptime.0"]
IF-MIB                        ["SNMPv2-MIB::sysDescr.0","IF-MIB::ifDescr.1","IF-MIB::ifHCInOctets.1",false]
(none)                        ["iso.3.6.1.2.1.1.1.0",false,false,false]
IF-MIB                        ["SNMPv2-MIB::sysDescr.0","IF-MIB::ifDescr.1","IF-MIB::ifHCInOctets.1",false]
