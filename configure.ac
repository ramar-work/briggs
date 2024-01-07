# 
AC_PREREQ([2.71])
AC_INIT([briggs], [1.0.1c], [ramar@ramar.work], [], [https://briggsdb.com])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_STRUCT_TIMEZONE
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRNLEN
AC_CHECK_FUNCS([alarm clock_gettime fdatasync floor gethrtime gettimeofday localtime_r memchr memmove memset rint socket strcasecmp strdup strerror strspn strtol strtoul strtoull])

# Tags
ld_flags="-ldl -lpthread -lm"
disable_sqlite3=0
disable_mysql=1
disable_pgsql=1
disable_sqlserver=1
disable_oracle=1

# Checks for libraries.
# FIXME: Replace 'main' with a function in '-lcrypto':
AC_CHECK_LIB([crypto], [main])
# FIXME: Replace 'main' with a function in '-ldl':
AC_CHECK_LIB([dl], [main])
# FIXME: Replace 'main' with a function in '-lgnutls':
AC_CHECK_LIB([gnutls], [main])
# FIXME: Replace 'main' with a function in '-lm':
AC_CHECK_LIB([m], [main])
# FIXME: Replace 'main' with a function in '-lpthread':
AC_CHECK_LIB([pthread], [main])
# FIXME: Replace 'main' with a function in '-lssl':
AC_CHECK_LIB([ssl], [main])
# FIXME: Replace 'main' with a function in '-lz':
AC_CHECK_LIB([z], [main])
# FIXME: Replace 'main' with a function in '-lmariadb':
AC_CHECK_LIB([mariadb], [main])
# FIXME: Replace 'main' with a function in '-lpq':
AC_CHECK_LIB([pq], [main])

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([arpa/inet.h fcntl.h fenv.h libintl.h malloc.h netdb.h netinet/in.h stdint.h strings.h sys/ioctl.h sys/socket.h sys/time.h syslog.h unistd.h])


# Change C compiler
AC_ARG_WITH([cc], AS_HELP_STRING([--with-cc=CC],[Set a different C compiler as the default]), [cc=${withval}])
AC_SUBST(cc)


# Find SQLite3 headers or use the local version
AC_ARG_ENABLE([local-sqlite3], AS_HELP_STRING([--enable-local-sqlite3],[Use the provided SQLite3 library]), [local_sqlite3=1])
if test ${local_sqlite3} == 1; then
	objdeps+="vendor/sqlite3.o "
else
	# Find SQLite3 header
	SQLITE3_HEADER_ERRMSG="SQLite3 headers were not found"
	AC_CHECK_HEADER([sqlite3.h], [], AC_MSG_FAILURE([${SQLITE3_HEADER_ERRMSG}]))
	SQLITE3_LIB_ERRMSG="SQLite3 library not found"
	# FIXME: Replace `main' with a function in `-lsqlite3':
	AC_CHECK_LIB([sqlite3], [main], [], AC_MSG_FAILURE(${SQLITE3_LIB_ERRMSG}))
	ld_flags+=" -lsqlite3"
fi


AC_CONFIG_FILES([Makefile])
#                 tests/Makefile
#                 vendor/Makefile])


#
AC_SUBST(ld_flags)
AC_SUBST(disable_sqlite3)
AC_SUBST(disable_mysql)
AC_SUBST(disable_pgsql)
AC_SUBST(disable_sqlserver)
AC_SUBST(disable_oracle)

AC_OUTPUT
