*** Settings ***
Documentation     Shared setup for twin tests: boots MCUboot + the signed app on the
...               Renode twin generated from hardware.yaml.
Library           Process

*** Variables ***
${UART}           sysbus.uart0
${RESC}           ${CURDIR}/../../renode/twin_sensor.resc
# Built by `west build --sysbuild -d build app` from the workspace root.
${BUILD}          ${CURDIR}/../../../build
${BOOTLOADER}     ${BUILD}/mcuboot/zephyr/zephyr.elf
${APP}            ${BUILD}/app/zephyr/zephyr.signed.hex
${IMGTOOL}        ${CURDIR}/../../../bootloader/mcuboot/scripts/imgtool.py
# Must match slot0_partition size (nrf52840_partition.dtsi: 0x76000).
${SLOT_SIZE}      0x76000

*** Keywords ***
Create Twin
    [Arguments]               ${app}=${APP}
    Execute Command           $bootloader=@${BOOTLOADER}
    Execute Command           $app=@${app}
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}    defaultPauseEmulation=true

Boot Twin
    Create Twin
    Start Emulation
    Wait For Line On Uart     self-test passed    timeout=10

Run Imgtool
    [Arguments]               @{args}
    ${python}=                Evaluate    sys.executable    modules=sys
    ${result}=                Run Process    ${python}    ${IMGTOOL}    @{args}
    Should Be Equal As Integers    ${result.rc}    0    imgtool failed: ${result.stderr}
