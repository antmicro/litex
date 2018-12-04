#!/usr/bin/env python3

import argparse

from migen import *

from litex.boards.platforms import arty

from litex.soc.cores.clock import *
from litex.soc.integration.soc_core import mem_decoder
from litex.soc.integration.soc_sdram import *
from litex.soc.integration.builder import *

from litedram.modules import MT41K128M16
from litedram.phy import s7ddrphy

from liteeth.phy.mii import LiteEthPHYMII
from liteeth.core.mac import LiteEthMAC


class _CRG(Module):
    def __init__(self, platform, sys_clk_freq):
        self.clock_domains.cd_sys = ClockDomain()
        self.clock_domains.cd_sys4x = ClockDomain(reset_less=True)
        self.clock_domains.cd_sys4x_dqs = ClockDomain(reset_less=True)
        self.clock_domains.cd_clk200 = ClockDomain()

        self.submodules.pll = pll = S7PLL()
        self.comb += pll.reset.eq(~platform.request("cpu_reset"))
        pll.register_clkin(platform.request("clk100"), 100e6)
        pll.create_clkout(self.cd_sys, sys_clk_freq)
        pll.create_clkout(self.cd_sys4x, 4*sys_clk_freq)
        pll.create_clkout(self.cd_sys4x_dqs, 4*sys_clk_freq, phase=90)
        pll.create_clkout(self.cd_clk200, 200e6)

        self.submodules.idelayctrl = S7IDELAYCTRL(self.cd_clk200)

        eth_clk = Signal()
        self.specials += [
            Instance("BUFR", p_BUFR_DIVIDE="4", i_CE=1, i_CLR=0, i_I=self.cd_sys.clk, o_O=eth_clk),
            Instance("BUFG", i_I=eth_clk, o_O=platform.request("eth_ref_clk")),
        ]


class BaseSoC(SoCSDRAM):
    csr_map = {
        "ddrphy":    16,
    }
    csr_map.update(SoCSDRAM.csr_map)
    def __init__(self, **kwargs):
        platform = arty.Platform()
        sys_clk_freq = int(100e6)
        SoCSDRAM.__init__(self, platform, clk_freq=sys_clk_freq,
                         integrated_rom_size=0x8000,
                         integrated_sram_size=0x8000,
                         **kwargs)

        self.submodules.crg = _CRG(platform, sys_clk_freq)

        # sdram
        self.submodules.ddrphy = s7ddrphy.A7DDRPHY(platform.request("ddram"), sys_clk_freq=sys_clk_freq)
        sdram_module = MT41K128M16(sys_clk_freq, "1:4")
        self.register_sdram(self.ddrphy,
                            sdram_module.geom_settings,
                            sdram_module.timing_settings)

        self.clock_domains.cd_jtag_tck = ClockDomain(reset_less=True)

        jtag_tdi = Signal()
        jtag_tdo = Signal()
        jtag_tms = Signal()
        jtag_tck = Signal()
        jtag_sel = [Signal(), Signal()]

        bscane_sel = Signal()

        self.comb += bscane_sel.eq(jtag_sel[0] | jtag_sel[1])

        # jtag debug signals

        jtag_capture = Signal()
        jtag_drck = Signal()
        jtag_reset = Signal()
        jtag_runtest = Signal()
        jtag_shift = Signal()
        jtag_update = Signal()

        self.specials += Instance("BSCANE2",
                                  p_JTAG_CHAIN = 1,
                                  i_TDO = jtag_tdo,
                                  o_TDI = jtag_tdi,
                                  o_TCK = jtag_tck,
                                  o_TMS = jtag_tms,
                                  o_SEL = jtag_sel[0],
                                  o_CAPTURE = jtag_capture,
                                  o_DRCK = jtag_drck,
                                  o_RESET = jtag_reset,
                                  o_RUNTEST = jtag_runtest,
                                  o_SHIFT = jtag_shift,
                                  o_UPDATE = jtag_update)

        self.specials += Instance("BSCANE2",
                                  p_JTAG_CHAIN = 2,
                                  i_TDO = jtag_tdo,
                                  o_SEL = jtag_sel[1])

        self.specials += Instance("BUFGCE", i_CE = bscane_sel, i_I=jtag_tck, o_O=self.cd_jtag_tck.clk)

        self.comb += self.cpu.jtag_tdi.eq(jtag_tdi)
        self.comb += self.cpu.jtag_tms.eq(jtag_tms)
        self.comb += self.cpu.jtag_tck.eq(self.cd_jtag_tck.clk)
        self.comb += jtag_tdo.eq(self.cpu.jtag_tdo)

        self.platform.add_period_constraint(self.cpu.jtag_tck, 100.0)

        #XXX: DBG route jtag to PMOD

        pmod_jtag = platform.request("jtag")

        pmod_jtag.tdi = jtag_tdi
        pmod_jtag.tdo = jtag_tdo
        pmod_jtag.tms = jtag_tms
        pmod_jtag.tck = self.cd_jtag_tck.clk

        self.comb += platform.request("user_led", 0).eq(jtag_capture)
        self.comb += platform.request("user_led", 1).eq(jtag_drck)
        self.comb += platform.request("user_led", 2).eq(jtag_reset)
        self.comb += platform.request("user_led", 3).eq(jtag_runtest)

        platform.request("rgb_led", 0).g = jtag_shift
        platform.request("rgb_led", 1).g = jtag_update
        platform.request("rgb_led", 2).g = jtag_sel[0]
        platform.request("rgb_led", 3).g = jtag_sel[1]


class EthernetSoC(BaseSoC):
    csr_map = {
        "ethphy": 18,
        "ethmac": 19
    }
    csr_map.update(BaseSoC.csr_map)

    interrupt_map = {
        "ethmac": 3,
    }
    interrupt_map.update(BaseSoC.interrupt_map)

    mem_map = {
        "ethmac": 0x30000000,  # (shadow @0xb0000000)
    }
    mem_map.update(BaseSoC.mem_map)

    def __init__(self, **kwargs):
        BaseSoC.__init__(self, **kwargs)

        self.submodules.ethphy = LiteEthPHYMII(self.platform.request("eth_clocks"),
                                               self.platform.request("eth"))
        self.submodules.ethmac = LiteEthMAC(phy=self.ethphy, dw=32,
            interface="wishbone", endianness=self.cpu.endianness)
        self.add_wb_slave(mem_decoder(self.mem_map["ethmac"]), self.ethmac.bus)
        self.add_memory_region("ethmac", self.mem_map["ethmac"] | self.shadow_base, 0x2000)

        self.crg.cd_sys.clk.attr.add("keep")
        self.ethphy.crg.cd_eth_rx.clk.attr.add("keep")
        self.ethphy.crg.cd_eth_tx.clk.attr.add("keep")
        self.platform.add_period_constraint(self.crg.cd_sys.clk, 10.0)
        self.platform.add_period_constraint(self.ethphy.crg.cd_eth_rx.clk, 80.0)
        self.platform.add_period_constraint(self.ethphy.crg.cd_eth_tx.clk, 80.0)
        self.platform.add_false_path_constraints(
            self.crg.cd_sys.clk,
            self.ethphy.crg.cd_eth_rx.clk,
            self.ethphy.crg.cd_eth_tx.clk)


def main():
    parser = argparse.ArgumentParser(description="LiteX SoC port to Arty")
    builder_args(parser)
    soc_sdram_args(parser)
    parser.add_argument("--with-ethernet", action="store_true",
                        help="enable Ethernet support")
    args = parser.parse_args()

    cls = EthernetSoC if args.with_ethernet else BaseSoC
    soc = cls(**soc_sdram_argdict(args))
    builder = Builder(soc, **builder_argdict(args))
    builder.build()


if __name__ == "__main__":
    main()
