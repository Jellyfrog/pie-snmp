--TEST--
snmp_init_mib() without arguments does not crash
--EXTENSIONS--
snmp
--FILE--
<?php
var_dump(snmp_init_mib());
var_dump(snmp_init_mib(null));
echo "done\n";
?>
--EXPECT--
NULL
NULL
done
