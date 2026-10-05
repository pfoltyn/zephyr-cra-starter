# SPDX-License-Identifier: Apache-2.0
"""Unit tests for scripts/hwsync.py validation and generation (run: pytest tests/hwsync)."""

import shutil
import sys
from pathlib import Path

import pytest
import yaml

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "scripts"))
import hwsync  # noqa: E402


@pytest.fixture
def repo(tmp_path):
    for item in ("hardware.yaml", "west.yml", "socs", "blocks", "boards", "renode", "docs", "hardware.lock.yaml",
                 "tests/twin"):
        src = REPO / item
        dst = tmp_path / item
        dst.parent.mkdir(parents=True, exist_ok=True)
        (shutil.copytree if src.is_dir() else shutil.copy)(src, dst)
    return tmp_path


def edit(repo, fn):
    path = repo / "hardware.yaml"
    hw = yaml.safe_load(path.read_text())
    fn(hw)
    path.write_text(yaml.safe_dump(hw, sort_keys=False))
    return hwsync.Design(repo)


def errors_of(design):
    return "\n".join(design.errors)


def test_reference_design_is_valid_and_in_sync(repo):
    d = hwsync.Design(repo)
    assert d.errors == []
    for rel, content in d.outputs().items():
        assert (repo / rel).read_text() == content, f"{rel} is stale: run `west hwsync generate`"


def test_pin_used_twice(repo):
    d = edit(repo, lambda hw: hw["leds"]["status"].update(pin="P0.26"))
    assert "P0.26 already used by i2c0 (SDA)" in errors_of(d)


def test_pin_outside_package(repo):
    d = edit(repo, lambda hw: hw["leds"]["status"].update(pin="P1.16"))
    assert "P1.16 does not exist" in errors_of(d)


def test_special_pin_refused(repo):
    d = edit(repo, lambda hw: hw["leds"]["status"].update(pin="P0.09"))
    assert "NFC1" in errors_of(d)


def test_reserved_pin_refused(repo):
    d = edit(repo, lambda hw: hw["buttons"]["user"].update(pin="P0.03"))
    assert "reserved in hardware.yaml" in errors_of(d)


def test_address_must_match_straps(repo):
    d = edit(repo, lambda hw: hw["devices"]["env"].update(address=0x40))
    assert "straps allow 0x76, 0x77" in errors_of(d)


def test_address_conflict_on_bus(repo):
    d = edit(repo, lambda hw: hw["devices"]["accel"].update(address=0x18) or hw["devices"].update(
        accel2={"block": "st_lis2dw12", "bus": "i2c0", "address": 0x18}))
    assert "0x18 on i2c0 already used by accel" in errors_of(d)


def test_shared_instance_conflict(repo):
    d = edit(repo, lambda hw: hw["buses"].update(spi0={"sck": "P0.19", "mosi": "P0.20", "miso": "P0.21"}))
    assert "shares a hardware instance with i2c0" in errors_of(d)


def test_unknown_block_is_new_work(repo):
    d = edit(repo, lambda hw: hw["devices"]["env"].update(block="ti_hdc2080"))
    assert "not in the supported menu" in errors_of(d)


def test_block_module_must_be_in_manifest(repo):
    west = repo / "west.yml"
    west.write_text(west.read_text().replace("          - hal_st\n", ""))
    d = hwsync.Design(repo)
    assert "needs west module 'hal_st'" in errors_of(d)


def test_bus_without_twin_model_warns(repo):
    def change(hw):
        del hw["buses"]["i2c0"]
        hw["buses"]["spi3"] = {"sck": "P0.19", "mosi": "P0.20", "miso": "P0.21"}
        hw["devices"] = {}
    d = edit(repo, change)
    assert d.errors == []
    assert any("spi3: no Renode model" in w for w in d.warnings)


def test_check_detects_hand_edit(repo, monkeypatch, capsys):
    dtsi = repo / "boards/boardtwin/twin_sensor/twin_sensor-pinctrl.dtsi"
    dtsi.write_text(dtsi.read_text().replace("TWIM_SDA, 0, 26", "TWIM_SDA, 0, 30"))
    with pytest.raises(hwsync.HwError, match="out of date or hand-edited"):
        hwsync.cmd_check(hwsync.Design(repo))
    assert "STALE: boards/boardtwin/twin_sensor/twin_sensor-pinctrl.dtsi" in capsys.readouterr().out


def test_block_evidence_must_exist(repo):
    robot = repo / "tests/twin/sensors.robot"
    robot.write_text(robot.read_text().replace("Environment Sensor Reports Simulated Climate", "Renamed Test"))
    d = hwsync.Design(repo)
    assert "block bosch_bme280: evidence 'tests/twin/sensors.robot::Environment Sensor Reports Simulated Climate' not found" in errors_of(d)
