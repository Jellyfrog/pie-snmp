/*
   +----------------------------------------------------------------------+
   | Copyright © The PHP Group and Contributors.                          |
   +----------------------------------------------------------------------+
   | This source file is subject to the Modified BSD License that is      |
   | bundled with this package in the file LICENSE, and is available      |
   | through the World Wide Web at <https://www.php.net/license/>.        |
   |                                                                      |
   | SPDX-License-Identifier: BSD-3-Clause                                |
   +----------------------------------------------------------------------+
*/

/* Shims that let the PHP 8.6+ ext/snmp sources build against PHP 8.5. */

#ifndef PHP_SNMP_COMPAT_H
#define PHP_SNMP_COMPAT_H

#include "php_version.h"

#if PHP_VERSION_ID < 80600

#include "zend_enum.h"

/* PHP 8.6 added ZEND_CONTAINER_OF() to zend_portability.h */
#ifndef ZEND_CONTAINER_OF
# define ZEND_CONTAINER_OF(ptr, Type, member) ((Type*)((char*)(ptr) - offsetof(Type, member)))
#endif

/* PHP 8.6 dropped the unused "params" argument of php_verror() */
#define php_verror(docref, type, format, args) php_verror((docref), "", (type), (format), (args))

/* PHP 8.6 stores a 1-based case id in every enum object. Older versions do
 * not, so derive it from the position of the case in the enum's constant
 * table, which matches the order in which gen_stub registers the cases. */
static inline int php_snmp_compat_enum_case_id(zend_object *zobj)
{
	zend_string *case_name = Z_STR_P(zend_enum_fetch_case_name(zobj));
	zend_string *key;
	zend_class_constant *c;
	int id = 0;

	ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(CE_CONSTANTS_TABLE(zobj->ce), key, c) {
		if (!(ZEND_CLASS_CONST_FLAGS(c) & ZEND_CLASS_CONST_IS_CASE)) {
			continue;
		}
		id++;
		if (zend_string_equals(key, case_name)) {
			return id;
		}
	} ZEND_HASH_FOREACH_END();

	ZEND_UNREACHABLE();
	return 0;
}
#define zend_enum_fetch_case_id(zobj) php_snmp_compat_enum_case_id(zobj)

#define Z_PARAM_ENUM(dest, _ce) \
	{ \
		zend_object *__##dest = NULL; \
		Z_PARAM_OBJ_OF_CLASS(__##dest, _ce); \
		dest = zend_enum_fetch_case_id(__##dest); \
	}

#endif /* PHP_VERSION_ID < 80600 */

#endif /* PHP_SNMP_COMPAT_H */
