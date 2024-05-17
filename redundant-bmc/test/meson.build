test('rbmc-state-manager-tests',
  executable('rbmc-state-manager-tests',
    'role_determination_test.cpp',
    dependencies: [
      gtest,
      phosphordbusinterfaces,
      sdbusplus,
      phosphorlogging,
    ],
    link_with: [
      librbmc_lib
    ],
    include_directories: '../'
  )
)
