inc_dirs = [ include_directories('../') ]

filesystem = import('fs')
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

filesystem.copyfile(
  'phosphor-host-condition-gpio@.service',
  install_dir: systemd_system_unit_dir,
  install: true
)
