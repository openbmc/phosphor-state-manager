AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -I$(top_srcdir)/../sdbusplus

sbin_PROGRAMS = \
	op-host-state-manager

op_host_state_manager_SOURCES = \
	host_state_manager.cpp \
	xyz.openbmc_project.State.Host.cpp \
	host_state_manager_main.cpp

op_host_state_manager_LDFLAGS = $(SYSTEMD_LIBS)

SUBDIRS = test
