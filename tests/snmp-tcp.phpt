--TEST--
TCP transport and transport prefixes
--EXTENSIONS--
snmp
--SKIPIF--
<?php
require_once(__DIR__.'/skipif.inc');
if (PHP_OS_FAMILY === 'Windows') die('xfail SNMP tests might possibly fail on Windows');
if (@snmp2_get("tcp:$hostname", $community, '.1.3.6.1.2.1.1.1.0', $timeout, $retries) === false) {
    die('skip SNMP agent does not listen on TCP');
}
?>
--FILE--
<?php
require_once(__DIR__.'/snmp_include.inc');

snmp_set_valueretrieval(SNMP_VALUE_PLAIN);
$oid = '.1.3.6.1.2.1.1.1.0';

/* A new object is used per request. */
foreach (["udp:$hostname", "tcp:$hostname", "TCP:$hostname"] as $h) {
    $s = new SNMP(SNMP::VERSION_2c, $h, $community, $timeout, $retries);
    var_dump(is_string($s->get($oid)));
}

echo "Walk over TCP\n";
$s = new SNMP(SNMP::VERSION_2c, "tcp:$hostname", $community, $timeout, $retries);
$tcp = $s->walk('.1.3.6.1.2.1.1');
$s = new SNMP(SNMP::VERSION_2c, $hostname, $community, $timeout, $retries);
$udp = $s->walk('.1.3.6.1.2.1.1');
var_dump(count($tcp) > 0, array_keys($tcp) === array_keys($udp));

echo "SNMPv3 over TCP\n";
$s = new SNMP(SNMP::VERSION_3, "tcp:$hostname", $user_noauth, $timeout, $retries);
$s->setSecurity('noAuthNoPriv');
var_dump(is_string($s->get($oid)));

echo "Procedural API\n";
var_dump(is_string(snmp2_get("tcp:$hostname", $community, $oid, $timeout, $retries)));

echo "Refused connection\n";
$s = new SNMP(SNMP::VERSION_2c, "tcp:$hostname4:1", $community, $timeout, $retries);
var_dump($s->get($oid));
?>
--EXPECTF--
bool(true)
bool(true)
bool(true)
Walk over TCP
bool(true)
bool(true)
SNMPv3 over TCP
bool(true)
Procedural API
bool(true)
Refused connection

Warning: SNMP::get(): Could not open snmp connection: Unknown host (tcp:%s:1) (Connection refused) in %s on line %d
bool(false)
