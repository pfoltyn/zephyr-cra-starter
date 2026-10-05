# Minimal model of the nRF52840 NVMC (flash controller) for the twin.
#
# Renode's nRF52840 platform maps flash as plain memory without an NVMC, so
# Zephyr's flash driver would spin forever on READY. This model reports READY
# and implements page erase (fill with 0xFF), which settings storage (ZMS) and
# MCUboot need. Write-enable/bit-clearing semantics are not modelled.
#
# SPDX-License-Identifier: Apache-2.0

READY = 0x400
READYNEXT = 0x408
CONFIG = 0x504
ERASEPAGE = 0x508
ERASEALL = 0x50C
ERASEUICR = 0x514
ERASEPAGEPARTIAL = 0x518
PAGE_SIZE = 0x1000
FLASH_SIZE = 0x100000

if request.IsInit:
    config = 0
    from Antmicro.Renode.Core import EmulationManager
    from Antmicro.Renode.Peripherals import IPeripheralExtensions
    from System import Array, Byte
    erased_page = Array[Byte]([0xFF] * PAGE_SIZE)
    sysbus = None
elif request.IsRead:
    if request.Offset in (READY, READYNEXT):
        request.Value = 1
    elif request.Offset == CONFIG:
        request.Value = config
    else:
        request.Value = 0
elif request.IsWrite:
    if request.Offset == CONFIG:
        config = request.Value & 0x3
    elif request.Offset in (ERASEPAGE, ERASEPAGEPARTIAL):
        if sysbus is None:
            sysbus = IPeripheralExtensions.GetMachine(self).SystemBus
        base = request.Value & ~(PAGE_SIZE - 1)
        if base < FLASH_SIZE:
            sysbus.WriteBytes(erased_page, base)
    elif request.Offset == ERASEALL and request.Value & 1:
        if sysbus is None:
            sysbus = IPeripheralExtensions.GetMachine(self).SystemBus
        for base in range(0, FLASH_SIZE, PAGE_SIZE):
            sysbus.WriteBytes(erased_page, base)
