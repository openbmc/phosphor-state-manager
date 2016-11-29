# export these headers
include_HEADERS =

bin_PROGRAMS = host-state-manager
host_state_manager_SOURCES = host_state_manager.cpp \
                             xyz.openbmc_project.State.Host.cpp \
                             host_state_manager_main.cpp

# systemd required
host_state_manager_LDFLAGS = $(SYSTEMD_LIBS)