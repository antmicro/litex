# Copyright (c) 2020 Antmicro <www.antmicro.com>

from migen import *

from litex.soc.interconnect import wishbone

class PRIOInterfacer(Module):
    def __init__(self, pads):
        self.bus = wishbone.Interface()

        ###

        # Connect bus signals to pads but also insert SYN_BUFS
        self.comb += self.connect_to_pads(pads, "master")

    def connect_to_pads(self, pads, mode="master"):
        assert mode in ["slave", "master"]
        r = []

        for name, width, direction in self.bus.layout:
            static_sig = Signal(width, name)
            sig  = getattr(self.bus, name)
            pad  = getattr(pads, name)

            if mode == "master":
                if direction == DIR_M_TO_S:
                    for i in range(width):
                        self.specials += Instance("SYN_OBUF", name=name+str(i), i_I=sig[i], o_O=pad[i])
                    #r.append(sig.eq(pad))
                else:
                    for i in range(width):
                        self.specials += Instance("SYN_IBUF", name=name+str(i), i_I=pad[i], o_O=sig[i])
                    #r.append(static_sig.eq(pad))
            else:
                if direction == DIR_S_TO_M:
                    r.append(pad.eq(sig))
                else:
                    r.append(sig.eq(pad))

        return r
