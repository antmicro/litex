# Copyright (c) 2020 Antmicro LTD

from migen import *

from litex.soc.interconnect import wishbone

class PRIOInterfacer(Module):
    def __init__(self, pads):
        self.bus = wishbone.Interface()
        bus_io = self.bus.get_ios()

        ###

        self.comb += self.bus.connect_to_pads(pads=pads, mode="master")
