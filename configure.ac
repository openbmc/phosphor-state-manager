AC_PREREQ([2.69])
AC_INIT([phosphor-state-manager], [1.0], [https://github.com/openbmc/phosphor-state-manager/issues])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_14([noext])
AM_PROG_AR
AC_PROG_INSTALL #Checks/sets the install variable to be used
AC_PROG_MAKE_SET

# Surpress the --with-libtool-sysroot error
LT_INIT

# Check for libraries
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])

# Check for header files.
AC_CHECK_HEADER(systemd/sd-bus.h, ,[AC_MSG_ERROR([Could not find systemd/sd-bus.h...systemd developement package required])])

# Checks for typedefs, structures, and compiler characteristics.
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])
AC_ARG_VAR(BUSNAME, [The Dbus busname to own])
AC_ARG_VAR(OBJPATH, [The host state manager Dbus root])
AS_IF([test "x$BUSNAME" == "x"], [BUSNAME="xyz.openbmc_project.State.Host"])
AS_IF([test "x$OBJPATH" == "x"], [OBJPATH="/xyz/openbmc_project/state/host"])
AC_DEFINE_UNQUOTED([BUSNAME], ["$BUSNAME"], [The DBus busname to own])
AC_DEFINE_UNQUOTED([OBJPATH], ["$OBJPATH"], [The host state manager Dbus root])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT