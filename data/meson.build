target_monitor_data = [
    'phosphor-target-monitor-default.json',
    'phosphor-service-monitor-default.json',
]

foreach d : target_monitor_data
    install_data(d, install_dir: datadir)
endforeach

install_data(
    'phosphor-chassis-availability-default.json',
    install_dir: join_paths(
        get_option('datadir'),
        'phosphor-state-manager',
        'chassis-availability',
    ),
)
