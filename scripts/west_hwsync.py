# Copyright (c) 2026 Piotr Foltyn
# SPDX-License-Identifier: Apache-2.0
"""`west hwsync ...`: thin wrapper around scripts/hwsync.py."""

import sys
from pathlib import Path

from west.commands import WestCommand

sys.path.insert(0, str(Path(__file__).parent))
import hwsync  # noqa: E402


class HwSync(WestCommand):
    def __init__(self):
        super().__init__("hwsync", "generate/check board files and twin from hardware.yaml",
                         hwsync.__doc__, accepts_unknown_args=True)

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(self.name, help=self.help, description=self.description, add_help=False)
        return parser

    def do_run(self, args, unknown):
        sys.exit(hwsync.main(unknown))
