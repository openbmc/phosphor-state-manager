inc_dirs = [ include_directories('../') ]

packagedir = join_paths(
    get_option('prefix'),
    get_option('datadir'),
    meson.project_name(),
    'host_condition',
)

gpioplus = dependency('gpioplus')
gpiodcxx = dependency(
    'libgpiodcxx',
    fallback: ['libgpiod', 'gpiodcxx_dep']
)

executable(
    'phosphor-host-condition',
    'host_condition.cpp',
    'host_condition_main.cpp',
    dependencies: [
        gpioplus, sdbusplus, sdeventplus, phosphorlogging,
        phosphordbusinterfaces, gpiodcxx
    ],
    include_directories : inc_dirs,
    implicit_include_directories: true,
    install: true
)

configure_file(
    input: 'phosphor-host-condition.service',
    output: 'phosphor-host-condition.service',
    copy: true,
    install_dir: systemd_system_unit_dir,
    install: true,
)
