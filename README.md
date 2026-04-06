# Phosphor State Manager Documentation

This repository contains the software responsible for tracking and controlling
the state of different objects within OpenBMC. This currently includes the BMC,
Chassis, Host, and Hypervisor. The most critical feature of
phosphor-state-manager (PSM) software is its support for requests to power on
and off the system by the user.

This software also enforces any restore policy (i.e. auto power on system after
a system power event or bmc reset) and ensures its states are updated correctly
in situations where the BMC is rebooted and the chassis or host are in
on/running states.

This repository also provides a command line tool, obmcutil, which provides
basic command line support to query and control phosphor-state-manager
applications running within an OpenBMC system. This tool itself runs within an
OpenBMC system and utilizes D-Bus APIs. These D-Bus APIs are used for
development and debug and are not intended for end users.

As with all OpenBMC applications, interfaces and properties within
phosphor-state-manager are D-Bus interfaces. These interfaces are then used by
external interface protocols, such as Redfish and IPMI, to report and control
state to/by the end user.

## State Tracking and Control

phosphor-state-manager makes extensive use of systemd. There is a [writeup][1]
with an overview of systemd and its use by OpenBMC.

phosphor-state-manager monitors for systemd targets to complete as a trigger to
updating the its corresponding D-Bus property. When using PSM, a user must
ensure all generic services installed within the PSM targets complete
successfully in order to have PSM properly report states.

phosphor-state-manager follows some basics design guidelines in its
implementation and use of systemd:

- Keep the different objects as independent as possible (host, chassis, bmc)
- Use systemd targets for everything and keep the code within PSM minimal
- Ensure it can support required external interfaces, but don't necessarily
  create 1x1 mappings otherwise every external interface will end up with its
  own special chassis or host state request
- If something like a hard power off can be done by just turning off the
  chassis, don't provide a command in the host to do the same thing

phosphor-state-manager implements states and state requests as defined in
phosphor-dbus-interfaces for each object it supports.

- [bmc][2]: The BMC has very minimal states. It is `NotReady` when first started
  and `Ready` once all services within the default.target have executed. It is
  `Quiesced` when a critical service has entered the failed state. The only
  state change request you can make of the BMC is for it to reboot itself.
  - CurrentBMCState: NotReady, Ready, Quiesced
  - RequestedBMCTransition: Reboot
  - Monitored systemd targets: multi-user.target and
    obmc-bmc-service-quiesce\@.target
- [chassis][3]: The chassis represents the physical hardware in which the system
  is contained. It usually has the power supplies, fans, and other hardware
  associated with it. It can be either `On`, `Off`, or in a fail state. A
  `BrownOut` state indicates there is not enough chassis power to fully power on
  and `UninterruptiblePowerSupply` indicates the chassis is running on a UPS.
  - CurrentPowerState: On, Off, BrownOut, UninterruptiblePowerSupply
  - RequestedPowerTransition: On, Off
  - Monitored systemd targets: obmc-chassis-poweron\@.target,
    obmc-chassis-poweroff\@.target
- [host][4]: The host represents the software running on the system. In most
  cases this is an operating system of some sort. The host can be `Off`,
  `Running`, `TransitioningToRunning`, `TransitioningToOff`, `Quiesced`(error
  condition), or in `DiagnosticMode`(collecting diagnostic data for a failure)
  - CurrentHostState: Off, Running, TransitioningToRunning, TransitioningToOff,
    Quiesced, DiagnosticMode
  - RequestedHostTransition: Off, On, Reboot, GracefulWarmReboot,
    ForceWarmReboot
  - Monitored systemd targets: obmc-host-startmin\@.target,
    obmc-host-stop\@.target, obmc-host-quiesce\@.target,
    obmc-host-diagnostic-mode\@.target
- [hypervisor][4]: The hypervisor is an optional package systems can install
  which tracks the state of the hypervisor on the system. This state manager
  object implements a limited subset of the host D-Bus interface.
  - CurrentHostState: Standby, TransitionToRunning, Running, Off, Quiesced
  - RequestedHostTransition: On

As noted above, PSM provides a command line tool, [obmcutil][5], which takes a
`state` parameter. This will use D-Bus commands to retrieve the above states and
present them to the user. It also provides other commands which will send the
appropriate D-Bus commands to the above properties to power on/off the chassis
and host (see `obmcutil --help` within an OpenBMC system).

The above objects also implement other D-Bus objects like power on hours, boot
progress, reboot attempts, and operating system status. These D-Bus objects are
also defined out in the phosphor-dbus-interfaces repository.

## Restore Policy on Power Events

The [RestorePolicy][6] defines the behavior the user wants when the BMC is
reset. If the chassis or host is on/running then this service will not run. If
they are off then the `RestorePolicy` will be read and executed by PSM code.

The `PowerRestoreDelay` property within the interface defines a maximum time the
service will wait for the BMC to enter the `Ready` state before issuing the
power on request, this allows host to be powered on as early as the BMC is
ready.

## Only Allow System Boot When BMC Ready

There is an optional `only-allow-boot-when-bmc-ready` feature which can be
enabled within PSM that will not allow chassis or host operations (other then
`Off` requests) if the BMC is not in a `Ready` state. Care should be taken to
ensure `PowerRestoreDelay` is set to a suitable value to ensure the BMC reaches
`Ready` before the power restore function requests the power on.

## BMC Reset with Host and/or Chassis On

In situations where the BMC is reset and the chassis and host are on and
running, its critical that the BMC software do two things:

- Never impact the state of the system (causing a power off of a running system
  is very bad)
- Ensure the BMC, Chassis, and Host states accurately represent the state of the
  system.

Note that some of this logic is provided via service files in system-specific
meta layers. That is because the logic to determine if the chassis is on or if
the host is running can vary from system to system. The requirement to create
the files defined below and ensure the common targets go active is a must for
anyone wishing to enable this feature.

phosphor-state-manager discovers state vs. trying to cache and save states. This
ensure it's always getting the most accurate state information. It discovers the
chassis state by checking the `pgood` value from the power application. If it
determines that power is on then it will do the following:

- Create a file called /run/openbmc/chassis@0-on
  - The presence of this file tells the services to alter their behavior because
    the chassis is already powered on
- Start the obmc-chassis-poweron\@0.target
  - The majority of services in this target will "fake start" due to the file
    being present. They will report to systemd that they started and ran
    successfully but they actually do nothing. This is what you would want in
    this case. Power is already on so you don't want to run the services to turn
    power on. You do want to get the obmc-chassis-poweron\@0.target in the
    Active state though so that the chassis object within PSM will correctly
    report that the chassis is `On`
- Start a service to check if the host is on

The chassis@0-on file is removed once the obmc-chassis-poweron\@0.target becomes
active (i.e. all service have been successfully started which are wanted or
required by this target).

The logic to check if the host is on sends a command to the host, and if a
response is received then similar logic to chassis is done:

- Create a file called /run/openbmc/host@0-on
- Start the obmc-host-start\@0.target
  - Similar to above, most services will not run due to the file being created
    and their service files implementing a
    "ConditionPathExists=!/run/openbmc/host@0-request"

The host@0-on file is removed once the obmc-host-start\@0.target and
obmc-host-startmin\@0.target become active (i.e. all service have been
successfully started which are wanted or required by these targets).

## Multiple Chassis in a Symmetric Multi-Processor (SMP) System

phosphor-state-manager supports an optional multi-chassis SMP feature for
systems with multiple chassis instances that need to be managed as a single
logical unit. This is useful in systems where multiple compute nodes or chassis
work together in a symmetric multi-processor configuration.

### Overview

When the multi-chassis SMP feature is enabled, chassis instance 0 acts as an
aggregator that monitors and controls chassis instances 1 through N. The
aggregator does not monitor any local chassis 0 hardware; instead, it aggregates
state information from the other chassis instances and presents a unified view.

### Key Features

- **Event-Driven Monitoring**: Uses D-Bus property change signals to monitor all
  chassis instances in real-time (no polling overhead)
- **Power State Aggregation**: Reports `Off` if ANY chassis is off, `On` only if
  ALL chassis are on
- **Power Status Aggregation**: Reports worst-case power status across all
  chassis (BrownOut > UninterruptiblePowerSupply > Good)
- **Coordinated Power Control**: Power transition requests are forwarded to all
  chassis instances, and the systemd target for chassis 0 is also triggered.
  This allows users who have any global service to run on all power on or off's
  to put them in the instance 0 obmc-chassis-power(off/on) targets.

### Configuration

The feature is enabled by default in CI but disabled by default within the
bitbake recipe.

Configuration options:

- `multi-chassis-smp`: Enable/disable the feature (default: enabled)
- `num-chassis-smp`: Maximum number of chassis instances to aggregate, 1-N
  (default: 12)

### Usage

Start the chassis state manager instances:

```bash
# Chassis 0 (aggregator) - monitors chassis 1-N, no local hardware monitoring
phosphor-chassis-state-manager --chassis 0

# Chassis 1-N (normal operation) - each monitors its own local hardware
phosphor-chassis-state-manager --chassis 1
phosphor-chassis-state-manager --chassis 2
...
phosphor-chassis-state-manager --chassis N
```

### D-Bus Interface

Chassis 0 presents the standard chassis D-Bus interface at:

- Bus name: `xyz.openbmc_project.State.Chassis0`
- Object path: `/xyz/openbmc_project/state/chassis0`

The aggregated properties include:

- `CurrentPowerState`: Aggregated power state from all chassis
- `CurrentPowerStatus`: Worst-case power status from all chassis
- `RequestedPowerTransition`: Forwards requests to all chassis instances

### Implementation Details

When a power transition is requested on chassis 0:

1. The systemd target for chassis 0 is started (e.g.,
   `obmc-chassis-poweron@0.target`)
2. The transition request is forwarded to all chassis instances 1-N
3. Each chassis instance processes the request independently
4. Chassis 0 aggregates the resulting states from all instances

This ensures that both the aggregator and individual chassis instances maintain
proper systemd target states and can execute any necessary system-specific
services.

## Building the Code

To build this package, do the following steps:

1. `meson setup build`
2. `ninja -C build`

To clean the repository again run `rm -rf build`.

[1]: https://github.com/openbmc/docs/blob/master/architecture/openbmc-systemd.md
[2]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/BMC.interface.yaml
[3]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Chassis.interface.yaml
[4]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Host.interface.yaml
[5]: https://github.com/openbmc/phosphor-state-manager/blob/master/obmcutil
[6]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Control/Power/RestorePolicy.interface.yaml
