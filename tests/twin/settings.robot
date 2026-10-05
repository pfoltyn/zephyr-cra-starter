*** Settings ***
Documentation     Settings storage (ZMS on the storage partition) survives a reboot.
Resource          common.resource
Test Setup        Reset Emulation

*** Test Cases ***
Sampling Interval Persists Across Reboot
    Boot Twin
    Write Line To Uart        app interval 500    waitForEcho=false
    Wait For Line On Uart     interval set to 500 ms (saved)
    Write Line To Uart        kernel reboot cold    waitForEcho=false
    Wait For Line On Uart     Starting bootloader    timeout=10
    Wait For Line On Uart     sample interval 500 ms
