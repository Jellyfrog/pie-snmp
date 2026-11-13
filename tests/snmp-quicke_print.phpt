--TEST--
SNMP::$quicke_print and Snmp\Output::QuickEPrint
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

/* A MIB that declares sysDescr.0's OID as an INTEGER. The agent returns an
 * OCTET STRING, so Net-SNMP prefixes the value with "Wrong Type" unless
 * QUICKE_PRINT is set. */
$dir = __DIR__ . '/snmp-quicke_print.mibs';
@mkdir($dir);
file_put_contents($dir . '/PHP-QUICKE-TEST-MIB', <<<'MIB'
PHP-QUICKE-TEST-MIB DEFINITIONS ::= BEGIN
quickeTestValue OBJECT-TYPE
    SYNTAX      INTEGER
    ACCESS      read-only
    STATUS      mandatory
    ::= { iso 3 6 1 2 1 1 1 }
END
MIB);
snmp_init_mib($dir, ['PHP-QUICKE-TEST-MIB']);

$oid = '.1.3.6.1.2.1.1.1.0';
snmp_set_valueretrieval(SNMP_VALUE_LIBRARY);
snmp_set_quick_print(true);

$s = new SNMP(SNMP::VERSION_2c, $hostname, $community, $timeout, $retries);
var_dump($s->quicke_print);
echo $s->get($oid), "\n";

$s = new SNMP(SNMP::VERSION_2c, $hostname, $community, $timeout, $retries);
$s->quicke_print = true;
var_dump($s->quicke_print);
echo $s->get($oid), "\n";

echo "quick_print alone does not set QUICKE_PRINT\n";
$s = new SNMP(SNMP::VERSION_2c, $hostname, $community, $timeout, $retries);
$s->quick_print = true;
var_dump($s->quicke_print);

echo "Object setting does not leak into global state\n";
echo snmp2_get($hostname, $community, $oid, $timeout, $retries), "\n";

echo "Global option\n";
snmp_set_output_option(Snmp\Output::QuickEPrint, true);
echo snmp2_get($hostname, $community, $oid, $timeout, $retries), "\n";
$s = new SNMP(SNMP::VERSION_2c, $hostname, $community, $timeout, $retries);
var_dump($s->quicke_print);
snmp_set_output_option(Snmp\Output::QuickEPrint, false);
echo snmp2_get($hostname, $community, $oid, $timeout, $retries), "\n";
?>
--CLEAN--
<?php
@unlink(__DIR__ . '/snmp-quicke_print.mibs/PHP-QUICKE-TEST-MIB');
@rmdir(__DIR__ . '/snmp-quicke_print.mibs');
?>
--EXPECTF--
bool(false)
Wrong Type (should be INTEGER): %s
bool(true)
%r(?!Wrong Type)%r%s
quick_print alone does not set QUICKE_PRINT
bool(false)
Object setting does not leak into global state
Wrong Type (should be INTEGER): %s
Global option
%r(?!Wrong Type)%r%s
bool(true)
Wrong Type (should be INTEGER): %s
