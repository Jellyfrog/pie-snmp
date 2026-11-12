/*
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/3_01.txt                                 |
   +----------------------------------------------------------------------+
*/

/* Switching the Net-SNMP MIB tree between lists of modules. No PHP
 * dependencies, so it can be tested on its own against the library. */

#ifndef PHP_SNMP_MIB_H
#define PHP_SNMP_MIB_H

#include <stdbool.h>
#include <stddef.h>

/* Make the MIB tree hold exactly what a fresh build from the current MIB
 * directories with MIBS=<mibs> would hold, reusing the modules that are
 * already loaded when it is safe to. mibs is an ENV_SEPARATOR list ("" for
 * none). Returns true if the tree was switched in place, false if it was
 * rebuilt. */
bool php_snmp_mib_switch(const char *mibs);

/* Forget the recorded state: the next php_snmp_mib_switch() rebuilds. Call
 * whenever the tree is rebuilt or changed in another way. */
void php_snmp_mib_forget(void);

#endif /* PHP_SNMP_MIB_H */
