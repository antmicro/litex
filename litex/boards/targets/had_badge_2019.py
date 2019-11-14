#!/usr/bin/env python3

import argparse

from migen import *

from litex.boards.platforms import had_badge_2019

from litex.soc.cores.clock import *
from litex.soc.integration.soc_core import *
from litex.soc.integration.builder import *

# CRG ----------------------------------------------------------------------------------------------

class _CRG(Module):
    def __init__(self, platform, sys_clk_freq):
        self.clock_domains.cd_sys = ClockDomain()

        # # #

        self.cd_sys.clk.attr.add("keep")

        # pll
        self.submodules.pll = pll = ECP5PLL()
        pll.register_clkin(platform.request("clk8"), 8e6)
        pll.create_clkout(self.cd_sys, sys_clk_freq)

# BaseSoC ------------------------------------------------------------------------------------------

class BaseSoC(SoCCore):
    def __init__(self, toolchain="diamond", **kwargs):
        platform = had_badge_2019.Platform(device="LFE5U-45F", toolchain=toolchain)
        sys_clk_freq = int(50e6)
        SoCCore.__init__(self, platform, clk_freq=sys_clk_freq,
                         integrated_rom_size=0x8000,
                         **kwargs)

        self.submodules.crg = _CRG(platform, sys_clk_freq)

# Build --------------------------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="LiteX SoC on Hackaday Supercon 2019 badge")
    parser.add_argument("--gateware-toolchain", dest="toolchain", default="trellis",
        help='gateware toolchain to use, trellis (default) or  diamond')
    builder_args(parser)
    soc_core_args(parser)
    args = parser.parse_args()

    soc = BaseSoC(toolchain=args.toolchain, **soc_core_argdict(args))
    builder = Builder(soc, **builder_argdict(args))
    builder.build()

if __name__ == "__main__":
    main()
