AC_INIT([unrar],[3.7.8], [bug-report@lundman.net])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CXX

AC_TYPE_SIGNAL

AC_HEADER_TIME

AC_CHECK_HEADERS([stdarg.h varargs.h sys/time.h sys/types.h dirent.h sys/file.h sys/param.h sys/statfs.h sys/vfs.h sys/statvfs.h errno.h])

dnl # check if mount depends on param
AC_CHECK_HEADERS([sys/mount.h], [], [], [#if HAVE_SYS_PARAM_H
                                         #include <sys/param.h>
                                         #endif])

AC_STRUCT_DIRENT_D_TYPE

AH_TEMPLATE([NEED_SYS_ERRLIST_DECLARED],
        [Define this if sys_errlist must be declared (if it exists).])

AC_SYS_LARGEFILE



AC_CHECK_FUNCS([strtoul strtouq strtoull strtoll])

AC_DEFINE([UNRAR], 1, [Compile unrar])


AC_C_BIGENDIAN([AC_DEFINE([BIG_ENDIAN], 1, [Defined if host is big endian])],
               [AC_DEFINE([LITTLE_ENDIAN], 1, [Defined if host is little endian])], 
			   [Could not determine endianness of the target arthitecture, that aint too good])

case "${host}" in
  mips-sgi-irix*)
    DEFINES="$DEFINES -D_BSD_COMPAT -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED=1"
  ;;
esac


AM_CONFIG_HEADER(config.h)

AC_CONFIG_FILES([Makefile win32/Makefile win32/unrar/Makefile win32/unrar/unrar/Makefile])
AC_OUTPUT

