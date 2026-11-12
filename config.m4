PHP_ARG_WITH([snmp],
  [for SNMP support],
  [AS_HELP_STRING([--with-snmp],
    [Include SNMP support. Net-SNMP is built from the bundled net-snmp/
    submodule (plus patches/*.patch) and linked statically. Extra Net-SNMP
    configure arguments can be passed in NETSNMP_CONFIGURE_ARGS.])],
  [yes])

if test "$PHP_SNMP" != "no"; then
  AS_IF([test "$PHP_SNMP" != "yes"],
    [AC_MSG_WARN([--with-snmp=DIR is ignored; the bundled Net-SNMP is used])])

  dnl Build the bundled Net-SNMP now, so the checks below run against it.
  AC_MSG_NOTICE([building bundled Net-SNMP (this takes a while)])
  AS_IF([! $SHELL "$abs_srcdir/scripts/build-netsnmp.sh" "$abs_srcdir" "$abs_builddir"],
    [AC_MSG_ERROR([building the bundled Net-SNMP failed])])

  SNMP_DIR="$abs_builddir/netsnmp-build"
  SNMP_CFLAGS="-I$SNMP_DIR/include"
  SNMP_LIBS="$SNMP_DIR/snmplib/.libs/libnetsnmp.a $($SHELL "$SNMP_DIR/net-snmp-config" --external-libs)"

  PHP_EVAL_INCLINE([$SNMP_CFLAGS])
  SNMP_SHARED_LIBADD="$SNMP_LIBS"

  dnl Keep the Net-SNMP symbols private to snmp.so, so they cannot clash
  dnl with a shared libnetsnmp loaded by something else in the process.
  AS_CASE([$host_os], [linux*|*gnu*|freebsd*],
    [SNMP_SHARED_LIBADD="$SNMP_SHARED_LIBADD -Wl,--exclude-libs,libnetsnmp.a"])

  LIBS_SAVE=$LIBS
  LIBS="$SNMP_LIBS $LIBS"
  AC_CHECK_FUNC([init_snmp],
    [AC_DEFINE([HAVE_SNMP], [1],
      [Define to 1 if the PHP extension 'snmp' is available.])],
    [AC_MSG_FAILURE([SNMP sanity check failed.])])
  AC_CHECK_FUNCS([netsnmp_init_mib shutdown_snmp_logging])
  LIBS=$LIBS_SAVE

  CFLAGS_SAVE=$CFLAGS
  LIBS_SAVE=$LIBS
  CFLAGS="$CFLAGS $SNMP_CFLAGS"
  LIBS="$LIBS $SNMP_LIBS"

  AC_CHECK_DECL([usmHMAC128SHA224AuthProtocol],
    [AC_DEFINE([HAVE_SNMP_SHA224], [1],
      [Define to 1 if SNMP library has the 'usmHMAC128SHA224AuthProtocol'
      array.])],
    [],
    [
      #include <net-snmp/net-snmp-config.h>
      #include <net-snmp/net-snmp-includes.h>
    ])

  AC_CHECK_DECL([usmHMAC192SHA256AuthProtocol],
    [AC_DEFINE([HAVE_SNMP_SHA256], [1],
      [Define to 1 if SNMP library has the 'usmHMAC192SHA256AuthProtocol'
      array.])],
    [],
    [
      #include <net-snmp/net-snmp-config.h>
      #include <net-snmp/net-snmp-includes.h>
    ])

  AC_CHECK_DECL([usmHMAC384SHA512AuthProtocol],
    [AC_DEFINE([HAVE_SNMP_SHA512], [1],
      [Define to 1 if SNMP library has the 'usmHMAC384SHA512AuthProtocol'
      array.])],
    [],
    [
      #include <net-snmp/net-snmp-config.h>
      #include <net-snmp/net-snmp-includes.h>
    ])

  AC_CHECK_DECL([usmHMAC256SHA384AuthProtocol],
    [AC_DEFINE([HAVE_SNMP_SHA384], [1],
      [Define to 1 if SNMP library has the 'usmHMAC256SHA384AuthProtocol'
      array.])],
    [],
    [
      #include <net-snmp/net-snmp-config.h>
      #include <net-snmp/net-snmp-includes.h>
    ])

  CFLAGS=$CFLAGS_SAVE
  LIBS=$LIBS_SAVE

  PHP_NEW_EXTENSION([snmp], [snmp.c], [$ext_shared])
  PHP_ADD_EXTENSION_DEP(snmp, spl)
  PHP_SUBST([SNMP_SHARED_LIBADD])
fi
