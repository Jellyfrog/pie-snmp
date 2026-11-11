--TEST--
SNMPv3 SHA-224 and SHA-384 authentication
--EXTENSIONS--
snmp
--SKIPIF--
<?php
require_once(__DIR__.'/skipif.inc');
if (PHP_OS_FAMILY === 'Windows') die('xfail SNMP tests might possibly fail on Windows');
try {
    $s = new SNMP(SNMP::VERSION_3, $hostname, 'x');
    $s->setSecurity('authNoPriv', 'SHA224', 'test1234');
    $s->setSecurity('authNoPriv', 'SHA384', 'test1234');
} catch (ValueError $e) {
    die('skip Net-SNMP has no SHA-224/SHA-384 support');
}
?>
--FILE--
<?php
require_once(__DIR__.'/snmp_include.inc');

/* net-snmp caches USM users per process, so each user is only used with
 * one set of security parameters here. */
snmp_set_valueretrieval(SNMP_VALUE_PLAIN);

foreach (['SHA224', 'SHA384'] as $auth) {
    echo "$auth authPriv AES\n";
    $s = new SNMP(SNMP::VERSION_3, $hostname, $user_auth_prefix . $auth, $timeout, $retries);
    var_dump($s->setSecurity('authPriv', $auth, $auth_pass, 'AES', $priv_pass));
    var_dump($s->get('.1.3.6.1.2.1.1.1.0'));
}
?>
--EXPECTF--
SHA224 authPriv AES
bool(true)
string(%d) "%s"
SHA384 authPriv AES
bool(true)
string(%d) "%s"
