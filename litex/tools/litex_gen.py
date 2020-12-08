#!/usr/bin/env python3

#
# This file is part of LiteX.
#
# Copyright (c) 2020 Antmicro <www.antmicro.com>
# Copyright (c) 2020 Florent Kermarrec <florent@enjoy-digital.fr>
# SPDX-License-Identifier: BSD-2-Clause

import argparse

from migen import *

from litex.build.generic_platform import *

from litex.soc.integration.soc_core import *
from litex.soc.integration.builder import *
from litex.soc.interconnect import wishbone
from litex.soc.interconnect import axi

from litex.soc.cores.pwm import PWM
from litex.soc.cores.gpio import GPIOTristate
from litex.soc.cores.spi import SPIMaster, SPISlave
from litex.soc.cores.clock import S7MMCM
from litex.soc.cores.prm import *

# Platform -----------------------------------------------------------------------------------------

_io = [
    ("sys_clk", 0, Pins(1)),
    ("sys_rst", 0, Pins(1)),
]

# Platform -----------------------------------------------------------------------------------------

class Platform(GenericPlatform):
    def __init__(self, io):
        GenericPlatform.__init__(self, "", io)

    def build(self, fragment, build_dir, **kwargs):
        os.makedirs(build_dir, exist_ok=True)
        os.chdir(build_dir)
        top_output = self.get_verilog(fragment)
        top_output.write("litex_core.v")

# LiteXCore ----------------------------------------------------------------------------------------

class LiteXCore(SoCMini):
    SoCMini.mem_map["csr"] = 0x00000000
    def __init__(self, sys_clk_freq=int(100e6),
        with_pwm        = False,
        with_mmcm       = False,
        with_gpio       = False, gpio_width=32,
        with_spi_master = False, spi_master_data_width=8, spi_master_clk_freq=8e6,
        with_prm        = False,
        **kwargs):

        platform = Platform(_io)

        # UART
        if kwargs["with_uart"]:
            platform.add_extension([
                ("serial", 0,
                    Subsignal("tx",  Pins(1)),
                    Subsignal("rx", Pins(1)),
                )
            ])

        # CRG --------------------------------------------------------------------------------------
        self.submodules.crg = CRG(platform.request("sys_clk"), rst=platform.request("sys_rst"))

        # SoCMini ----------------------------------------------------------------------------------
        print(kwargs)
        SoCMini.__init__(self, platform, clk_freq=sys_clk_freq, **kwargs)

        # MMCM
        if with_mmcm:
            platform.add_extension([
                ("clkgen", 0,
                    Subsignal("ref", Pins(1)),
                    Subsignal("out0", Pins(1)),
                    Subsignal("out1", Pins(1)),
                    Subsignal("locked", Pins(1)),
                )
            ])

            self.clock_domains.cd_out0 = ClockDomain(reset_less=True)
            self.clock_domains.cd_out1 = ClockDomain(reset_less=True)
            self.submodules.mmcm = mmcm = S7MMCM()
            mmcm.expose_drp()
            self.add_csr("mmcm")

            clkgen = platform.request("clkgen")

            mmcm.register_clkin(clkgen.ref, 100e6)
            mmcm.create_clkout(self.cd_out0, 148.5e6, with_reset=False)
            mmcm.create_clkout(self.cd_out1, 742.5e6, with_reset=False)

            self.comb += [
                clkgen.out0.eq(self.cd_out0.clk),
                clkgen.out1.eq(self.cd_out1.clk),
                clkgen.locked.eq(mmcm.locked),
            ]

        # SPI Master
        if with_spi_master:
            platform.add_extension([
                ("spi_master", 0,
                    Subsignal("clk",  Pins(1)),
                    Subsignal("cs_n", Pins(1)),
                    Subsignal("mosi", Pins(1)),
                    Subsignal("miso", Pins(1)),
                )
            ])
            self.submodules.spi_master = SPIMaster(
                pads         = platform.request("spi_master"),
                data_width   = spi_master_data_width,
                sys_clk_freq = sys_clk_freq,
                spi_clk_freq = spi_master_clk_freq,
            )
            self.add_csr("spi_master")

        # PRM
        if with_prm:
            platform.add_extension([
                 ("synthio_bus", 0,
                     Subsignal("adr",   Pins("SYN2 SYN3 SYN4 SYN5 SYN6 SYN7 SYN8 SYN9 SYN10 SYN11 SYN12 SYN13 SYN14 SYN15 SYN16 SYN17 SYN18 SYN19 SYN20 SYN21 SYN22 SYN23 SYN24 SYN25 SYN26 SYN27 SYN28 SYN29 SYN30 SYN31 ")),
                     Subsignal("dat_w",   Pins("SYN32 SYN33 SYN34 SYN35 SYN36 SYN37 SYN38 SYN39 SYN40 SYN41 SYN42 SYN43 SYN44 SYN45 SYN46 SYN47 SYN48 SYN49 SYN50 SYN51 SYN52 SYN53 SYN54 SYN55 SYN56 SYN57 SYN58 SYN59 SYN60 SYN61 SYN62 SYN63 ")),
                     Subsignal("dat_r",   Pins("SYN90 SYN91 SYN92 SYN93 SYN94 SYN95 SYN96 SYN97 SYN98 SYN99 SYN100 SYN101 SYN102 SYN103 SYN104 SYN105 SYN106 SYN107 SYN108 SYN109 SYN110 SYN111 SYN112 SYN113 SYN114 SYN115 SYN116 SYN117 SYN118 SYN119 SYN120 SYN121 ")),
                     Subsignal("sel",   Pins("SYN64 SYN65 SYN66 SYN67 ")),
                     Subsignal("cyc",   Pins("SYN68 ")),
                     Subsignal("stb",   Pins("SYN69 ")),
                     Subsignal("ack",   Pins("SYN122 ")),
                     Subsignal("we",   Pins("SYN70 ")),
                     Subsignal("cti",   Pins("SYN71 SYN72 SYN73 ")),
                     Subsignal("bte",   Pins("SYN74 SYN75 ")),
                     Subsignal("err",   Pins("SYN123 ")),
                 ),
                 ("synthio_in", 0,
                     Subsignal("roi_in0",   Pins("SYN76 ")),
                     Subsignal("roi_in1",   Pins("SYN77 SYN78 ")),
                     Subsignal("roi_in2",   Pins("SYN79 SYN80 SYN81 ")),
                     Subsignal("roi_in3",   Pins("SYN82 SYN83 SYN84 SYN85 ")),
                     Subsignal("roi_in4",   Pins("SYN86 SYN87 SYN88 SYN89 ")),
                 ),
                 ("synthio_out", 0,
                     Subsignal("roi_out0",   Pins("SYN124 ")),
                     Subsignal("roi_out1",   Pins("SYN125 SYN126 ")),
                     Subsignal("roi_out2",   Pins("SYN127 SYN128 SYN129 ")),
                     Subsignal("roi_out3",   Pins("SYN130 SYN131 SYN132 SYN133 ")),
                     Subsignal("roi_out4",   Pins("SYN134 SYN135 SYN136 SYN137 ")),
                 ),
            ])
            self.submodules.prm = PRIOInterfacer(
            bus_pads         = platform.request("synthio_bus"),
            roi_input_pads   = platform.request("synthio_in"),
            roi_output_pads  = platform.request("synthio_out"),
            mode             = "slave")
            self.add_csr("prm")

        # PWM
        if with_pwm:
            platform.add_extension([("pwm", 0, Pins(1))])
            self.submodules.pwm = PWM(platform.request("pwm"))
            self.add_csr("pwm")

        # GPIO
        if with_gpio:
            platform.add_extension([("gpio", 0, Pins(gpio_width))])
            self.submodules.gpio = GPIOTristate(platform.request("gpio"))
            self.add_csr("gpio")

        # Wishbone Master
        if kwargs["bus"] == "wishbone":
            wb_bus = wishbone.Interface()
            self.bus.add_master(master=wb_bus)
            platform.add_extension(wb_bus.get_ios("wb"))
            wb_pads = platform.request("wb")
            self.comb += wb_bus.connect_to_pads(wb_pads, mode="slave")

        # AXI-Lite Master
        if kwargs["bus"] == "axi":
            axi_bus = axi.AXILiteInterface(data_width=32, address_width=32)
            wb_bus = wishbone.Interface()
            axi2wb = axi.AXILite2Wishbone(axi_bus, wb_bus)
            self.submodules += axi2wb
            self.bus.add_master(master=wb_bus)
            platform.add_extension(axi_bus.get_ios("axi"))
            axi_pads = platform.request("axi")
            self.comb += axi_bus.connect_to_pads(axi_pads, mode="slave")

        # IRQs
        for name, loc in sorted(self.irq.locs.items()):
            module = getattr(self, name)
            platform.add_extension([("irq_"+name, 0, Pins(1))])
            irq_pin = platform.request("irq_"+name)
            self.comb += irq_pin.eq(module.ev.irq)

# Build -------------------------------------------------------------------------------------------

def soc_argdict(args):
    ret = {}
    for arg in [
        "bus",
        "with_pwm",
        "with_mmcm",
        "with_uart",
        "uart_fifo_depth",
        "with_ctrl",
        "with_timer",
        "with_prm",
        "with_gpio",
        "gpio_width",
        "with_spi_master",
        "spi_master_data_width",
        "spi_master_clk_freq",
        "csr_data_width",
        "csr_address_width",
        "csr_paging"]:
        ret[arg] = getattr(args, arg)
    return ret

def main():
    parser = argparse.ArgumentParser(description="LiteX standalone core generator")
    builder_args(parser)

    # Bus
    parser.add_argument("--bus",                   default="wishbone",    type=str, help="Type of Bus (wishbone, axi)")

    # Cores
    parser.add_argument("--with-pwm",              action="store_true",   help="Add PWM core")
    parser.add_argument("--with-mmcm",             action="store_true",   help="Add MMCM (Xilinx 7-series) core")
    parser.add_argument("--with-uart",             action="store_true",   help="Add UART core")
    parser.add_argument("--uart-fifo-depth",       default=16, type=int,  help="UART FIFO depth (default=16)")
    parser.add_argument("--with-ctrl",             action="store_true",   help="Add bus controller core")
    parser.add_argument("--with-timer",            action="store_true",   help="Add timer core")
    parser.add_argument("--with-spi-master",       action="store_true",   help="Add SPI master core")
    parser.add_argument("--spi-master-data-width", default=8,   type=int, help="SPI master data width")
    parser.add_argument("--spi-master-clk-freq",   default=8e6, type=int, help="SPI master output clock frequency")
    parser.add_argument("--with-prm",              action="store_true",   help="Add PRM core")
    parser.add_argument("--with-gpio",             action="store_true",   help="Add GPIO core")
    parser.add_argument("--gpio-width",            default=32,  type=int, help="GPIO signals width")

    # CSR settings
    parser.add_argument("--csr-data-width",    default=8,     type=int, help="CSR bus data-width (8 or 32, default=8)")
    parser.add_argument("--csr-address-width", default=14,    type=int, help="CSR bus address-width")
    parser.add_argument("--csr-paging",        default=0x800, type=int, help="CSR bus paging")

    args = parser.parse_args()

    soc     = LiteXCore(**soc_argdict(args))
    builder = Builder(soc, **builder_argdict(args))
    builder.build()


if __name__ == "__main__":
    main()
