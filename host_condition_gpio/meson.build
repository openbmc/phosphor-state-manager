inc_dirs = [ include_directories('../') ]

gpioplus = dependency('gpioplus')
gpiodcxx = dependency(
    'libgpiodcxx',
    fallback: ['libgpiod', 'gpiodcxx_dep']
)

executable(
    'phosphor-host-condition-gpio',
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
    input: 'phosphor-host-condition-gpio@.service',
    output: 'phosphor-host-condition-gpio@.service',
    copy: true,
    install_dir: systemd_system_unit_dir,
    install: true,
)
