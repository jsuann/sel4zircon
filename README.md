# Zircon on seL4

Zircon API on seL4.

Zircon objects currently supported:
* Channels, Sockets, FIFOs
* VMOs, VMARs
* Threads, Processes, Jobs
* Events, Eventpairs, Timers

## Setup

Most dependencies should be covered
[here](https://docs.sel4.systems/HostDependencies), although note that this
project uses an older kernel and
[build system](https://docs.sel4.systems/Developing/Building/OldBuildSystem).

```
mkdir sel4zircon
cd sel4zircon
repo init -u https://github.com/jsuann/sel4zircon-manifest.git
repo sync
./projects/sel4zircon/apply-patches.sh
```

## Build

```
make x64_simulation_release_xml_defconfig
make
```

## Run

Run simulation with Qemu:
```
make simulate-x86_64
```

Information for running on actual hardware can be found
[here](https://docs.sel4.systems/Hardware/IA32).
