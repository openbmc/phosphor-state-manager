AM_DEFAULT_SOURCE_EXT = .cpp

sbin_PROGRAMS = \
	phosphor-host-state-manager \
	phosphor-chassis-state-manager \
	phosphor-bmc-state-manager \
	phosphor-discover-system-state \
	phosphor-host-check \
        phosphor-os-status \
        phosphor-boot-progress \
        phosphor-boot-count

phosphor_host_state_manager_SOURCES = \
	host_state_manager.cpp \
	host_state_manager_main.cpp

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

phosphor_os_status_SOURCES = \
	os_status_main.cpp

phosphor_boot_progress_SOURCES = \
	boot_progress_main.cpp

phosphor_boot_count_SOURCES = \
        boot_count_main.cpp

generic_cxxflags = $(SYSTEMD_CFLAGS) $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) $(SDBUSPLUS_CFLAGS)
generic_ldflags = $(SYSTEMD_LIBS) $(PHOSPHOR_DBUS_INTERFACES_LIBS) $(SDBUSPLUS_LIBS)

phosphor_host_state_manager_CXXFLAGS = $(generic_cxxflags)
phosphor_host_state_manager_LDFLAGS = $(generic_ldflags)

phosphor_chassis_state_manager_CXXFLAGS = $(generic_cxxflags)
phosphor_chassis_state_manager_LDFLAGS = $(generic_ldflags)

phosphor_bmc_state_manager_CXXFLAGS = $(generic_cxxflags)
phosphor_bmc_state_manager_LDFLAGS = $(generic_ldflags)

phosphor_discover_system_state_CXXFLAGS = $(generic_cxxflags)
phosphor_discover_system_state_LDFLAGS = $(generic_ldflags)

phosphor_host_check_CXXFLAGS = $(generic_cxxflags)
phosphor_host_check_LDFLAGS = $(generic_ldflags)

phosphor_os_status_CXXFLAGS = $(generic_cxxflags)
phosphor_os_status_LDFLAGS = $(generic_ldflags)

phosphor_boot_progress_CXXFLAGS = $(generic_cxxflags)
phosphor_boot_progress_LDFLAGS = $(generic_ldflags)

phosphor_boot_count_CXXFLAGS = $(generic_cxxflags)
phosphor_boot_count_LDFLAGS = $(generic_ldflags)
