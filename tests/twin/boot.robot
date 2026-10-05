*** Settings ***
Documentation     Secure boot, self-test and basic I/O of the Twin Sensor Node.
Resource          common.resource
Test Setup        Reset Emulation

*** Test Cases ***
MCUboot Verifies And Starts The Signed Image
    Create Twin
    Start Emulation
    Wait For Line On Uart     Starting bootloader
    Wait For Line On Uart     Jumping to the first image slot
    Wait For Line On Uart     twin_sensor v0.1.0
    Wait For Line On Uart     self-test passed

Tampered Image Is Rejected By MCUboot
    [Documentation]           CRA "secure by design": a modified application must not run.
    Create Twin
    # Flip a word inside the application code in slot 0 after it was loaded.
    Execute Command           sysbus WriteDoubleWord 0x10000 0xDEADBEEF
    Start Emulation
    Wait For Line On Uart     Unable to find bootable image
    Should Not Be On Uart     twin_sensor v0.1.0    timeout=2

Image Signed With Another Key Is Rejected
    [Documentation]           Only images signed with the product key boot, whatever their version.
    ${key}=                   Set Variable    ${TEMPDIR}/twin-attacker-key.pem
    ${image}=                 Set Variable    ${TEMPDIR}/twin-attacker-signed.hex
    Run Imgtool               keygen    -k    ${key}    -t    ecdsa-p256
    Run Imgtool               sign    --version    9.9.9    --header-size    0x200    --align    4
    ...                       --slot-size    ${SLOT_SIZE}    --key    ${key}
    ...                       ${BUILD}/app/zephyr/zephyr.hex    ${image}
    Create Twin               app=${image}
    Start Emulation
    Wait For Line On Uart     Unable to find bootable image
    Should Not Be On Uart     twin_sensor v    timeout=2

Status LED Blinks
    Create Twin
    Create LED Tester         sysbus.gpio0.led_status    defaultTimeout=0
    Start Emulation
    Wait For Line On Uart     self-test passed
    Assert LED Is Blinking    testDuration=4  onDuration=1  offDuration=1  tolerance=0.1  pauseEmulation=true

Button Press Is Reported
    Boot Twin
    # Hold the button for real (simulated) time: a zero-length pulse is not an edge.
    Execute Command           sysbus.gpio0.button_user Press
    Wait For Line On Uart     button: pressed
    Execute Command           sysbus.gpio0.button_user Release

Shell Reports Version
    Boot Twin
    Write Line To Uart        app status    waitForEcho=false
    Wait For Line On Uart     version: 0.1.0
