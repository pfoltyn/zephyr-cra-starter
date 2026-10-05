# Zephyr CRA starter: firmware before the PCB exists

A complete, opinionated starting point for a connected product on **Zephyr**:
custom board, **MCUboot secure boot with signed updates**, settings storage,
Bluetooth LE, CI, **SBOM**, and a **Renode digital twin** that runs the real
firmware in CI. All of it generated from one file, `hardware.yaml`, and kept in
sync with it.

The reference product is a BLE environmental sensor node (nRF52840, Bosch BME280,
ST LIS2DW12, LED, button). Its pins match the nRF52840 DK, so the same firmware
also runs on a DK while your PCB is being designed.

```
hardware.yaml ──west hwsync generate──► board pinctrl + devicetree
      │                                 Renode twin platform
      │                                 hardware.lock.yaml (resolved pins)
      │                                 docs/hardware/pinout.md (for the hardware engineer)
      └──CI: hwsync check ─► build (MCUboot + signed app) ─► verify-build ─► SBOM ─► twin tests
```

## What you get

| | |
|---|---|
| **Board** | `boards/boardtwin/twin_sensor`: plain Zephyr HWMv2 board; pins/devices come from `hardware.yaml` |
| **Firmware** | `app/`: sensors, LED heartbeat, button, UART shell (`app status`, `app interval`), ZMS settings, BLE Environmental Sensing Service, watchdog, self-test + image confirm |
| **Secure boot / updates** | MCUboot via sysbuild, ECDSA P-256 signed images, A/B slots with revert, release signing from a CI secret |
| **Twin** | `renode/`: generated platform + test suite (`tests/twin/`). Runs MCUboot and the signed app exactly as flashed |
| **Hardware contract** | `west hwsync`: validates pins (package, conflicts, special pins, shared peripheral instances, strap addresses), generates files, checks a build against the lock file |
| **CI / release** | `.github/workflows/`: check → build → verify → SBOM → twin tests; tagged releases publish signed images, SBOM, checksums |
| **CRA basics** | `docs/cra/README.md`: requirement-by-requirement mapping, `SECURITY.md` template |

## Twin tests (all run in CI)

| Test | Proves |
|---|---|
| MCUboot Verifies And Starts The Signed Image | the boot chain works end to end |
| Tampered Image Is Rejected By MCUboot | integrity check: one changed word, no boot |
| Image Signed With Another Key Is Rejected | only your key can produce bootable firmware |
| Status LED Blinks / Button Press Is Reported | GPIO blocks wired as `hardware.yaml` says |
| Environment Sensor / Accelerometer report simulated values | I2C blocks at the right bus/address, drivers configured |
| Sampling Interval Persists Across Reboot | settings storage on the flash partition works |
| Device Advertises Environmental Sensing | Bluetooth LE stack comes up on the radio model |
| Shell Reports Version | console/shell works |

## Quick start

Requirements: Python ≥ 3.12, CMake, Ninja, [Zephyr SDK](https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html)
(arm toolchain), and [Renode](https://renode.io) for the twin.

```sh
mkdir my-product && cd my-product
git clone <this repo> zephyr-cra-starter
python3 -m venv .venv && . .venv/bin/activate
pip install west
west init -l zephyr-cra-starter && west update --narrow -o=--depth=1
west packages pip --install
pip install -r bootloader/mcuboot/scripts/requirements.txt

zephyr-cra-starter/scripts/build.sh          # check, build, verify, SBOM -> build/
zephyr-cra-starter/scripts/run_twin_tests.sh # Renode twin tests -> build/twin-results/
renode zephyr-cra-starter/renode/twin_sensor.resc   # interactive twin (start, then use the shell)
```

On the interactive twin, change the world and watch the firmware react:

```
(twin_sensor) start
(twin_sensor) sysbus.twi0.env Temperature 30
(twin_sensor) sysbus.gpio0.button_user Press
```

## Changing the hardware

1. Edit `hardware.yaml` (move a pin, add a device from `blocks/`).
2. `west hwsync generate`: errors explain what's wrong
   (`led_status (LED): P0.26 already used by i2c0 (SDA)`).
3. Commit `hardware.yaml` **and** the generated files. CI fails if they disagree
   (`west hwsync check`) or if an overlay moved a pin behind your back
   (`west hwsync verify-build`):

   ```
   MISMATCH: i2c0 SDA: hardware.lock.yaml says P0.26, built devicetree says P0.30
   ```

Ownership: before a schematic exists, `hardware.yaml` is the source of truth.
Hardware approves pin changes, firmware approves devicetree changes (set this up
in `CODEOWNERS`).

## The block menu

`blocks/*/block.yaml` describes each supported part: devicetree compatible,
allowed I2C addresses (straps), signals, required west modules, Renode model,
reference circuit, known issues, and **the twin tests that validate it**.
`hwsync` refuses a block whose cited tests don't exist. Anything not in the menu
is new block work, not a configuration change.

## What is *not* simulated

Listed honestly in [`docs/hardware/pinout.md`](docs/hardware/pinout.md) and
[`docs/twin-findings.md`](docs/twin-findings.md). Today: accelerometer interrupt
line, ADC/battery measurement, real radio RF behaviour, power consumption. The
twin also uses legacy TWI instead of TWIM for I2C (functionally identical on
hardware).

## Leaving is free

Everything here is standard Zephyr, CMake, west and Renode. The generated files
are committed, so the project builds without `hwsync`. Delete `scripts/hwsync.py`
and you have an ordinary Zephyr application with a custom board.

## License

Apache-2.0. `renode/nrf52840_nvmc.py` and all generated files are under the same license.
