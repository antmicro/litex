# Copyright (c) 2020 Antmicro <www.antmicro.com>

from migen import *

from litex.soc.interconnect import wishbone
from litex.soc.interconnect.csr import *


in_layout = [
    ("roi_in0", 1),
    ("roi_in1", 2),
    ("roi_in2", 3),
    ("roi_in3", 4),
    ("roi_in4", 4),
]

out_layout = [
    ("roi_out0", 1),
    ("roi_out1", 2),
    ("roi_out2", 3),
    ("roi_out3", 4),
    ("roi_out4", 4),
]

class PRIOInterfacer(Module):
    def __init__(self, bus_pads, input_pads, output_pads, mode):
        self.bus = wishbone.Interface()

        self.connect_additional_io(input_pads, output_pads,
                                   in_layout, out_layout)

        # Connect bus signals to pads but also insert SYN_BUFS
        self.comb += self.connect_to_pads(bus_pads, mode)

    def connect_additional_io(self, ipads, opads, ilayout, olayout):
        self.additional_in = []
        self.additional_out = []

        for name, width in ilayout:
            clk = ClockSignal("sys")
            pad  = getattr(ipads, name)
            sig = Signal(width, name)
            bufsig = Signal(width, name=name+"_bufsig")

            self.additional_in.append(sig)

            for i in range(width):
                fd_inst = Instance("FD",
                                    i_C=clk,
                                    i_D=self.additional_in[-1][i],
                                    o_Q=bufsig[i])
                fd_inst.attr.add("keep")
                self.specials += fd_inst
                self.specials += Instance("SYN_OBUF",
                                            name=name+str(i),
                                            i_I=bufsig[i],
                                            o_O=pad[i])

        for name, width in olayout:
            pad  = getattr(opads, name)
            sig = Signal(width, name)
            self.additional_out.append(sig)
            for i in range(width):
                self.specials += Instance("SYN_IBUF",
                                            name=name+str(i),
                                            i_I=pad[i],
                                            o_O=self.additional_out[-1][i])


    def connect_to_pads(self, pads, mode="master"):
        assert mode in ["slave", "master"]
        r = []

        for name, width, direction in self.bus.layout:
            static_sig = Signal(width, name)
            sig  = getattr(self.bus, name)
            pad  = getattr(pads, name)

            ### Use some active logic to avoid direct routing to GND
            if (name == "bte"):
                clk = ClockSignal("sys")
                self.bus_bte = Signal(width)
                for i in range(width):
                    fd_inst = Instance("FD",
                                        i_C=clk,
                                        i_D=sig[i],
                                        o_Q=self.bus_bte[i])
                    fd_inst.attr.add("keep")
                    self.specials += fd_inst
                    self.specials += Instance("SYN_OBUF",
                                                name=name+str(i),
                                                i_I=self.bus_bte[i],
                                                o_O=pad[i])
                continue

            if mode == "master":
                if direction == DIR_M_TO_S:
                    for i in range(width):
                        self.specials += Instance("SYN_OBUF",
                                                    name=name+str(i),
                                                    i_I=sig[i],
                                                    o_O=pad[i])
                else:
                    for i in range(width):
                        self.specials += Instance("SYN_IBUF",
                                                    name=name+str(i),
                                                    i_I=pad[i],
                                                    o_O=sig[i])
            else:
                if direction == DIR_S_TO_M:
                    for i in range(width):
                        self.specials += Instance("SYN_OBUF",
                                                    name=name+str(i),
                                                    i_I=sig[i],
                                                    o_O=pad[i])
                else:
                    for i in range(width):
                        self.specials += Instance("SYN_IBUF",
                                                    name=name+str(i),
                                                    i_I=pad[i],
                                                    o_O=sig[i])

        return r
