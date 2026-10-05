# Twin findings and compromises

Things the twin taught us while building the starter, and where it differs
from hardware. Keep this list short and current: each item either gets fixed
or stays documented here.

| # | Finding | Status |
|---|---|---|
| 1 | Renode 1.17.0's nRF52840 I2C model lacks TWI SUSPEND handling, so Zephyr's write-then-read transfers go to the wrong device. Fixed upstream on 2026-09-23 (renode-infrastructure `NRF52840_I2C: Implement SUSPEND logic`). | CI pins Renode nightly `1.17.0+20261002git0e9eea194`; move to the next release |
| 2 | Renode models only the legacy TWI peripheral, not TWIM (EasyDMA). | I2C uses `nordic,nrf-twi` (set in `socs/nrf52840_qiaa.yaml`). Works on hardware; TWIM would cost fewer interrupts |
| 3 | With `CONFIG_PM_DEVICE_RUNTIME=y` the TWI interrupt ends up disabled before the first transfer and both sensors fail to probe. | Disabled in `prj.conf`. **Reproduce on an nRF52840 DK** to tell a driver issue from a twin artifact |
| 4 | Renode's nRF52840 has no NVMC (flash controller); Zephyr's flash driver would hang. | Minimal Python model `renode/nrf52840_nvmc.py` (READY + page erase) |
| 5 | FICR `CODEPAGESIZE`/`CODESIZE` read as 0, so MCUboot can't compute the flash layout. | Tagged with silicon values in `renode/twin_sensor.resc` |
| 6 | LIS2DW12 INT1 isn't wired in the twin (model has no interrupt output). | Shown in `docs/hardware/pinout.md`; interrupt-driven motion wake is untested in simulation |
| 7 | A zero-length button pulse isn't seen as an edge (GPIOTE sense). | Tests hold the button in simulated time. Real buttons need debouncing anyway |
| 8 | SAADC isn't modelled, so the battery measurement block is not offered yet. | Pin P0.03 reserved in `hardware.yaml` |
