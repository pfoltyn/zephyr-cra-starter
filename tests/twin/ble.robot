*** Settings ***
Documentation     Bluetooth LE comes up and advertises on the twin's radio model.
Resource          common.resource
Test Setup        Reset Emulation

*** Test Cases ***
Device Advertises Environmental Sensing
    Create Twin
    Start Emulation
    Wait For Line On Uart     bt_hci_core: Identity:    timeout=10
    Wait For Line On Uart     advertising as "Twin Sensor"
