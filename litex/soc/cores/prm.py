# Copyright (c) 2020 Antmicro LTD

from migen import *

from litex.soc.interconnect import wishbone
import sys

class PRIOInterfacer(Module):
    def __init__(self, bus_ios, ins=None, outs=None):
        bus = wishbone.Interface()
        bus_io = bus.get_ios()

        ###

        self.comb += self.connect_to_pads(bus, bus_ios, ins, outs, "master")

    def connect_to_pads(self, bus, bus_ios, ins=None, outs=None, mode="master"):
        assert mode in ["slave", "master"]
        r = []
        for name, width, direction in bus.layout:
            static_sig = Signal(width, name)
            sig  = getattr(bus, name)
            pad  = getattr(bus_ios, name)
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

        if ins is not None:
            for ipin in ins:
                print(ipin)

        if outs is not None:
            for opin in outs:
                print(opin)

        sys.exit(1)

        print("connect_to_pads:")
        print(r)
        return r
