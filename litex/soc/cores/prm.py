# Copyright (c) 2020 Antmicro <www.antmicro.com>

from migen import *

from litex.soc.interconnect import wishbone

class PRIOInterfacer(Module):
    def __init__(self, pads):
        bus = wishbone.Interface()
        bus_io = bus.get_ios()

        ###

        # Connect bus signals to pads but also insert SYN_BUFS
        self.comb += self.connect_to_pads(bus, pads, "master")

    def connect_to_pads(self, bus, pads, mode="master"):
        assert mode in ["slave", "master"]
        r = []

        for name, width, direction in bus.layout:
            static_sig = Signal(width, name)
            sig  = getattr(bus, name)
            pad  = getattr(pads, name)

            if mode == "master":
                if direction == DIR_M_TO_S:
                    for i in range(width):
                        self.specials += Instance("SYN_OBUF", name=name+str(i), i_I=static_sig[i], o_O=sig[i])
                    r.append(pad.eq(sig))
                else:
                    for i in range(width):
                        self.specials += Instance("SYN_IBUF", name=name+str(i), i_I=pad, o_O=static_sig[i])
                    r.append(sig.eq(pad))
            else:
                if direction == DIR_S_TO_M:
                    r.append(pad.eq(sig))
                else:
                    r.append(sig.eq(pad))

        return r
