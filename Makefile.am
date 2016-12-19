AM_DEFAULT_SOURCE_EXT = .cpp

sbin_PROGRAMS = \
	op-host-state-manager \
	op-bmc-state-manager

op_host_state_manager_SOURCES = \
	host_state_manager.cpp \
	xyz.openbmc_project.State.Host.cpp \
	host_state_manager_main.cpp

op_bmc_state_manager_SOURCES = \
	BMC_state_manager.cpp \
	xyz.openbmc_project.State.BMC.cpp \
	BMC_state_manager_main.cpp

op_host_state_manager_LDFLAGS = $(SYSTEMD_LIBS)
op_bmc_state_manager_LDFLAGS = $(SYSTEMD_LIBS)
