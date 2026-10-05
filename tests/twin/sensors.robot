*** Settings ***
Documentation     Sensor blocks: values injected into the simulated parts reach the application.
Resource          common.resource
Test Setup        Reset Emulation

*** Test Cases ***
Environment Sensor Reports Simulated Climate
    Boot Twin
    Execute Command           sysbus.twi0.env Temperature 30
    Execute Command           sysbus.twi0.env Humidity 60
    Wait For Line On Uart     env: t=30.00 C rh=60.    timeout=5

Accelerometer Reports Simulated Acceleration
    Boot Twin
    Execute Command           sysbus.twi0.accel AccelerationX 1
    Execute Command           sysbus.twi0.accel AccelerationZ 0
    Wait For Line On Uart     accel: x=999 y=0 z=0 mg    timeout=5
