# Copyright (c) 2020 Antmicro <www.antmicro.com>

from migen import *

from litex.soc.interconnect import wishbone
from litex.soc.interconnect.csr import *


io_layout = [
    ("roi_in0", 1, DIR_M_TO_S),
    ("roi_in1", 2, DIR_M_TO_S),
    ("roi_in2", 3, DIR_M_TO_S),
    ("roi_in3", 4, DIR_M_TO_S),
    ("roi_in4", 4, DIR_M_TO_S),
    ("roi_out0", 1, DIR_S_TO_M),
    ("roi_out1", 2, DIR_S_TO_M),
    ("roi_out2", 3, DIR_S_TO_M),
    ("roi_out3", 4, DIR_S_TO_M),
    ("roi_out4", 4, DIR_S_TO_M),
]

class PRIOInterfacer(Module):
    def __init__(self, bus_pads, roi_input_pads, roi_output_pads, mode):
        self.bus = wishbone.Interface()

        self.connect_addit_roi_io(roi_input_pads,
                                  roi_output_pads,
                                  io_layout,
                                  mode)

        # Connect bus signals to pads but also insert SYN_BUFS
        self.comb += self.connect_to_pads(bus_pads, mode)

    def connect_addit_roi_io(self, roi_ipads, roi_opads, io_layout, mode="master"):
        assert mode in ["slave", "master"]
        self.addit_roi_in = []
        self.addit_roi_out = []

        for name, width, direction in io_layout:
            sig = Signal(width, name)
            if mode == "master":
                if direction == DIR_M_TO_S:
                    self.addit_roi_in.append(sig)
                    pad  = getattr(roi_ipads, name)
                    clk = ClockSignal("sys")
                    bufsig = Signal(width, name=name+"_bufsig")
                    for i in range(width):
                        fd_inst = Instance("FD",
                                            i_C=clk,
                                            i_D=self.addit_roi_in[-1][i],
                                            o_Q=bufsig[i])
                        fd_inst.attr.add("keep")
                        self.specials += fd_inst
                        self.specials += Instance("SYN_OBUF",
                                                    name=name+str(i),
                                                    i_I=bufsig[i],
                                                    o_O=pad[i])
                else:
                    self.addit_roi_out.append(sig)
                    pad  = getattr(roi_opads, name)
                    for i in range(width):
                        self.specials += Instance("SYN_IBUF",
                                                    name=name+str(i),
                                                    i_I=pad[i],
                                                    o_O=self.addit_roi_out[-1][i])
            else:
                if direction == DIR_S_TO_M:
                    self.addit_roi_out.append(sig)
                    pad  = getattr(roi_opads, name)
                    clk = ClockSignal("sys")
                    bufsig = Signal(width, name=name+"_bufsig")
                    for i in range(width):
                        fd_inst = Instance("FD",
                                            i_C=clk,
                                            i_D=self.addit_roi_out[-1][i],
                                            o_Q=bufsig[i])
                        fd_inst.attr.add("keep")
                        self.specials += fd_inst
                        self.specials += Instance("SYN_OBUF",
                                                    name=name+str(i),
                                                    i_I=bufsig[i],
                                                    o_O=pad[i])
                else:
                    self.addit_roi_in.append(sig)
                    pad  = getattr(roi_ipads, name)
                    for i in range(width):
                        self.specials += Instance("SYN_IBUF",
                                                    name=name+str(i),
                                                    i_I=pad[i],
                                                    o_O=self.addit_roi_in[-1][i])


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
