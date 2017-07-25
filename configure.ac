AC_PREREQ([2.69])
AC_INIT([phosphor-state-manager], [1.0], [https://github.com/openbmc/phosphor-state-manager/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs
AC_PROG_CXX
AC_PROG_INSTALL #Checks/sets the install variable to be used
AC_PROG_MAKE_SET

# Check for libraries
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces],,\
    AC_MSG_ERROR(["Requires phosphor-dbus-interfaces package."]))
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus],,
    AC_MSG_ERROR(["Requires sdbusplus package."]))
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging],,\
    AC_MSG_ERROR(["Requires phosphor-logging package."]))

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_14([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# Checks for library functions
LT_INIT # Removes WARNING: unrecognized options: --with-libtool-sysroot

AC_ARG_VAR(HOST_BUSNAME, [The Host Dbus busname to own])
AS_IF([test "x$HOST_BUSNAME" == "x"], [HOST_BUSNAME="xyz.openbmc_project.State.Host"])
AC_DEFINE_UNQUOTED([HOST_BUSNAME], ["$HOST_BUSNAME"], [The Host DBus busname to own])

AC_ARG_VAR(HOST_OBJPATH, [The host state manager Dbus root])
AS_IF([test "x$HOST_OBJPATH" == "x"], [HOST_OBJPATH="/xyz/openbmc_project/state/host"])
AC_DEFINE_UNQUOTED([HOST_OBJPATH], ["$HOST_OBJPATH"], [The host state manager Dbus root])

AC_ARG_VAR(CHASSIS_BUSNAME, [The Chassis Dbus busname to own])
AS_IF([test "x$CHASSIS_BUSNAME" == "x"], [CHASSIS_BUSNAME="xyz.openbmc_project.State.Chassis"])
AC_DEFINE_UNQUOTED([CHASSIS_BUSNAME], ["$CHASSIS_BUSNAME"], [The Chassis DBus busname to own])

AC_ARG_VAR(CHASSIS_OBJPATH, [The chassis state manager Dbus root])
AS_IF([test "x$CHASSIS_OBJPATH" == "x"], [CHASSIS_OBJPATH="/xyz/openbmc_project/state/chassis"])
AC_DEFINE_UNQUOTED([CHASSIS_OBJPATH], ["$CHASSIS_OBJPATH"], [The chassis state manager Dbus root])

AC_ARG_VAR(BMC_BUSNAME, [The BMC Dbus busname to own])
AS_IF([test "x$BMC_BUSNAME" == "x"], [BMC_BUSNAME="xyz.openbmc_project.State.BMC"])
AC_DEFINE_UNQUOTED([BMC_BUSNAME], ["$BMC_BUSNAME"], [The BMC DBus busname to own])

AC_ARG_VAR(BMC_OBJPATH, [The BMC state manager Dbus root])
AS_IF([test "x$BMC_OBJPATH" == "x"], [BMC_OBJPATH="/xyz/openbmc_project/state/bmc"])
AC_DEFINE_UNQUOTED([BMC_OBJPATH], ["$BMC_OBJPATH"], [The BMC state manager Dbus root])

AC_ARG_VAR(HOST_RUNNING_FILE, [File to create if host is running])
AS_IF([test "x$HOST_RUNNING_FILE" == "x"], [HOST_RUNNING_FILE="/run/openbmc/host@%u-on"])
AC_DEFINE_UNQUOTED([HOST_RUNNING_FILE], ["$HOST_RUNNING_FILE"], [File to create if host is running])

# Check for header files.
AC_CHECK_HEADER(systemd/sd-bus.h, ,[AC_MSG_ERROR([Could not find systemd/sd-bus.h...systemd developement package required])])
AC_CHECK_HEADER(sdbusplus/server.hpp, ,[AC_MSG_ERROR([Could not find sdbusplus/server.hpp...openbmc/sdbusplus package required])])
AC_CHECK_HEADER(phosphor-logging/log.hpp, ,[AC_MSG_ERROR([Could not find phosphor-logging/log.hpp...openbmc/phosphor-logging package required])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
