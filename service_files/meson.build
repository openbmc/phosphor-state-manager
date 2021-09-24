unit_files = [
    'phosphor-bmc-service-fail.service',
    'phosphor-systemd-target-monitor.service',
    'phosphor-discover-system-state@.service',
    'phosphor-reboot-host@.service',
    'phosphor-reset-host-reboot-attempts@.service',
    'phosphor-reset-host-recovery@.service',
    'phosphor-reset-host-running@.service',
    'phosphor-reset-sensor-states@.service',
    'xyz.openbmc_project.State.BMC.service',
    'xyz.openbmc_project.State.Chassis.service',
    'xyz.openbmc_project.State.Host.service',
    'xyz.openbmc_project.State.Hypervisor.service',
    'xyz.openbmc_project.State.ScheduledHostTransition.service',
    'phosphor-clear-one-time@.service',
    'phosphor-set-host-transition-to-off@.service',
    'phosphor-set-host-transition-to-running@.service'
]

foreach u : unit_files
    configure_file(
        copy: true,
        input: u,
        install: true,
        install_dir: systemd_system_unit_dir,
        output: u,
        )
endforeach
