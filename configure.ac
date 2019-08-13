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
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces])
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus])
PKG_CHECK_MODULES([SDEVENTPLUS], [sdeventplus])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])

# We need the header only CLI library
AC_CHECK_HEADERS(
    [CLI/CLI.hpp],
    [],
    [AC_MSG_ERROR([Could not find CLI11 CLI/CLI.hpp])]
)
AC_CHECK_HEADER(nlohmann/json.hpp, ,
                [AC_MSG_ERROR([Could not find nlohmann/json.hpp... nlohmann/json package required])])

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
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

AC_ARG_VAR(HOST_STATE_PERSIST_PATH, [Path of file for storing requested host state.])
AS_IF([test "x$HOST_STATE_PERSIST_PATH" == "x"], \
    [HOST_STATE_PERSIST_PATH="/var/lib/phosphor-state-manager/requestedHostTransition"])
AC_DEFINE_UNQUOTED([HOST_STATE_PERSIST_PATH], ["$HOST_STATE_PERSIST_PATH"], \
    [Path of file for storing requested host state.])

AC_ARG_VAR(POH_COUNTER_PERSIST_PATH, [Path of file for storing POH counter.])
AS_IF([test "x$POH_COUNTER_PERSIST_PATH" == "x"], \
    [POH_COUNTER_PERSIST_PATH="/var/lib/phosphor-state-manager/POHCounter"])
AC_DEFINE_UNQUOTED([POH_COUNTER_PERSIST_PATH], ["$POH_COUNTER_PERSIST_PATH"], \
    [Path of file for storing POH counter.])

AC_ARG_VAR(CHASSIS_STATE_CHANGE_PERSIST_PATH, [Path of file for storing the state change time.])
AS_IF([test "x$CHASSIS_STATE_CHANGE_PERSIST_PATH" == "x"], \
    [CHASSIS_STATE_CHANGE_PERSIST_PATH="/var/lib/phosphor-state-manager/chassisStateChangeTime"])
AC_DEFINE_UNQUOTED([CHASSIS_STATE_CHANGE_PERSIST_PATH], ["$CHASSIS_STATE_CHANGE_PERSIST_PATH"], \
    [Path of file for storing the state change time.])

AC_ARG_VAR(BOOT_COUNT_MAX_ALLOWED, [The maximum allowed reboot count])
AS_IF([test "x$BOOT_COUNT_MAX_ALLOWED" == "x"], [BOOT_COUNT_MAX_ALLOWED=3])
AC_DEFINE_UNQUOTED([BOOT_COUNT_MAX_ALLOWED], [$BOOT_COUNT_MAX_ALLOWED], [The maximum allowed reboot count])

# Check for header files.
AC_CHECK_HEADER(systemd/sd-bus.h, ,[AC_MSG_ERROR([Could not find systemd/sd-bus.h...systemd development package required])])
AC_CHECK_HEADER(sdbusplus/server.hpp, ,[AC_MSG_ERROR([Could not find sdbusplus/server.hpp...openbmc/sdbusplus package required])])
AC_CHECK_HEADER(phosphor-logging/log.hpp, ,[AC_MSG_ERROR([Could not find phosphor-logging/log.hpp...openbmc/phosphor-logging package required])])

AC_ARG_VAR(CLASS_VERSION, [Class version to register with Cereal])
AS_IF([test "x$CLASS_VERSION" == "x"], [CLASS_VERSION=1])
AC_DEFINE_UNQUOTED([CLASS_VERSION], [$CLASS_VERSION], [Class version to register with Cereal])

# Make it possible for users to choose if they want test support
# explicitly or not at all
AC_ARG_ENABLE([tests], AC_HELP_STRING([--disable-tests],
                                      [Build test cases]))

 # Check/set gtest specific functions
AS_IF([test "x$enable_tests" != "xno"], [
    PKG_CHECK_MODULES([GTEST], [gtest], [], [true])
    PKG_CHECK_MODULES([GMOCK], [gmock], [], [true])
    AX_PTHREAD

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GTEST_CFLAGS], [CPPFLAGS])
    AC_CHECK_HEADERS([gtest/gtest.h], [
        AS_IF([test "x$GTEST_CFLAGS" = "x"], [
            AS_IF([test "x$PTHREAD_CFLAGS" = "x"], [
                AX_APPEND_COMPILE_FLAGS(["-DGTEST_HAS_PTHREAD=0"], [GTEST_CFLAGS])
            ], [
                AX_APPEND_COMPILE_FLAGS(["-DGTEST_HAS_PTHREAD=1"], [GTEST_CFLAGS])
                AX_APPEND_COMPILE_FLAGS([$PTHREAD_CFLAGS], [GTEST_CFLAGS])
            ])
        ])
    ], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but could not find gtest/gtest.h])
        ])
    ])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GMOCK_CFLAGS], [CPPFLAGS])
    AC_CHECK_HEADERS([gmock/gmock.h], [], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but could not find gmock/gmock.h])
        ])
    ])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [CPPFLAGS])

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GTEST_LIBS], [LDFLAGS])
    AC_CHECK_LIB([gtest], [main], [
        AS_IF([test "x$GTEST_LIBS" = "x"], [
            AX_APPEND_COMPILE_FLAGS([-lgtest], [GTEST_LIBS])
        ])
    ], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but couldn't find gtest libs])
        ])
    ])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])

    AX_SAVE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])
    AX_APPEND_COMPILE_FLAGS([$GMOCK_LIBS], [LDFLAGS])
    AC_CHECK_LIB([gmock], [main], [
        AS_IF([test "x$GMOCK_LIBS" = "x"], [
            AX_APPEND_COMPILE_FLAGS([-lgmock], [GMOCK_LIBS])
        ])
    ], [
        AS_IF([test "x$enable_tests" = "xyes"], [
            AC_MSG_ERROR([Testing enabled but couldn't find gmock libs])
        ])
    ])
    AX_RESTORE_FLAGS_WITH_PREFIX(OLD, [LDFLAGS])
])

# Check for valgrind
AS_IF([test "x$enable_tests" = "xno"], [enable_valgrind=no])
m4_foreach([vgtool], [valgrind_tool_list],
    [AX_VALGRIND_DFLT(vgtool, [off])])
AX_VALGRIND_DFLT([memcheck], [on])
AX_VALGRIND_CHECK
AM_EXTRA_RECURSIVE_TARGETS([check-valgrind])
m4_foreach([vgtool], [valgrind_tool_list],
    [AM_EXTRA_RECURSIVE_TARGETS([check-valgrind-]vgtool)])

# Code coverage
AX_CODE_COVERAGE

m4_ifdef([_AX_CODE_COVERAGE_RULES],
    [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [true])],
    [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [false])])
AX_ADD_AM_MACRO_STATIC([])

PKG_PROG_PKG_CONFIG
AC_ARG_WITH([systemdsystemunitdir],
     [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files])],,
     [with_systemdsystemunitdir=auto])
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"], [
     def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)
     AS_IF([test "x$def_systemdsystemunitdir" = "x"],
   [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
    [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])])
    with_systemdsystemunitdir=no],
   [with_systemdsystemunitdir="$def_systemdsystemunitdir"])])
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
[AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([phosphor-systemd-target-monitor.service])
AC_OUTPUT
