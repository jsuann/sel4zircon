# Zircon on seL4

Zircon API on seL4.

## Setup

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
