AM_DEFAULT_SOURCE_EXT = .cpp

sbin_PROGRAMS = \
	phosphor-host-state-manager \
	phosphor-chassis-state-manager \
	phosphor-bmc-state-manager

phosphor_host_state_manager_SOURCES = \
	host_state_manager.cpp \
	xyz.openbmc_project.State.Host.cpp \
	host_state_manager_main.cpp

phosphor_chassis_state_manager_SOURCES = \
	chassis_state_manager.cpp \
	xyz.openbmc_project.State.Chassis.cpp \
	chassis_state_manager_main.cpp

phosphor_bmc_state_manager_SOURCES = \
	bmc_state_manager.cpp \
	xyz.openbmc_project.State.BMC.cpp \
	bmc_state_manager_main.cpp

phosphor_host_state_manager_LDFLAGS = $(SYSTEMD_LIBS)
phosphor_chassis_state_manager_LDFLAGS = $(SYSTEMD_LIBS)
phosphor_bmc_state_manager_LDFLAGS = $(SYSTEMD_LIBS)
