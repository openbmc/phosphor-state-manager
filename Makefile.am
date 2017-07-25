AM_DEFAULT_SOURCE_EXT = .cpp

sbin_PROGRAMS = \
	phosphor-host-state-manager \
	phosphor-chassis-state-manager \
	phosphor-bmc-state-manager \
	phosphor-discover-system-state \
	phosphor-host-check

phosphor_host_state_manager_SOURCES = \
	host_state_manager.cpp \
	host_state_manager_main.cpp \
	host_state_serialize.cpp \
	settings.cpp

phosphor_chassis_state_manager_SOURCES = \
	chassis_state_manager.cpp \
	chassis_state_manager_main.cpp

phosphor_bmc_state_manager_SOURCES = \
	bmc_state_manager.cpp \
	bmc_state_manager_main.cpp

phosphor_discover_system_state_SOURCES = \
	discover_system_state.cpp

phosphor_host_check_SOURCES = \
	host_check_main.cpp

generic_cxxflags = \
	$(SYSTEMD_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)

generic_ldflags = \
	$(SYSTEMD_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(SDBUSPLUS_LIBS)
	$(PHOSPHOR_LOGGING_LIBS)

phosphor_host_state_manager_CXXFLAGS = $(generic_cxxflags)
phosphor_host_state_manager_LDFLAGS = $(generic_ldflags) -lstdc++fs

phosphor_chassis_state_manager_CXXFLAGS = $(generic_cxxflags)
phosphor_chassis_state_manager_LDFLAGS = $(generic_ldflags)

phosphor_bmc_state_manager_CXXFLAGS = $(generic_cxxflags)
phosphor_bmc_state_manager_LDFLAGS = $(generic_ldflags)

phosphor_discover_system_state_CXXFLAGS = $(generic_cxxflags)
phosphor_discover_system_state_LDFLAGS = $(generic_ldflags)

phosphor_host_check_CXXFLAGS = $(generic_cxxflags)
phosphor_host_check_LDFLAGS = $(generic_ldflags)
