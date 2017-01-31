AM_DEFAULT_SOURCE_EXT = .cpp

sbin_PROGRAMS = \
	phosphor-host-state-manager \
	phosphor-chassis-state-manager \
	phosphor-bmc-state-manager

phosphor_host_state_manager_SOURCES = \
	host_state_manager.cpp \
	host_state_manager_main.cpp

phosphor_chassis_state_manager_SOURCES = \
	chassis_state_manager.cpp \
	chassis_state_manager_main.cpp

phosphor_bmc_state_manager_SOURCES = \
	bmc_state_manager.cpp \
	bmc_state_manager_main.cpp

generic_cxxflags = $(SYSTEMD_CFLAGS) $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) $(SDBUSPLUS_CFLAGS)
generic_ldflags = $(SYSTEMD_LIBS) $(PHOSPHOR_DBUS_INTERFACES_LIBS) $(SDBUSPLUS_LIBS)

phosphor_host_state_manager_CXXFLAGS = $(generic_cxxflags)
phosphor_host_state_manager_LDFLAGS = $(generic_ldflags)

phosphor_chassis_state_manager_CXXFLAGS = $(generic_cxxflags)
phosphor_chassis_state_manager_LDFLAGS = $(generic_ldflags)

phosphor_bmc_state_manager_CXXFLAGS = $(generic_cxxflags)
phosphor_bmc_state_manager_LDFLAGS = $(generic_ldflags)
