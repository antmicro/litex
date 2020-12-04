# Copyright (c) 2020 Antmicro <www.antmicro.com>

from migen import *

from litex.soc.interconnect import wishbone
from litex.soc.interconnect.csr import *


in_layout = [
    ("in0", 1),
    ("in1", 2),
    ("in2", 3),
    ("in3", 4),
    ("in4", 5),
]

out_layout = [
    ("out0", 1),
    ("out1", 2),
    ("out2", 3),
    ("out3", 4),
    ("out4", 5),
]

class PRIOInterfacer(Module):
    def __init__(self, bus_pads, input_pads, output_pads):
        self.bus = wishbone.Interface()

        additional_in, in_csrs = self.connect_additional_io(input_pads, in_layout, "input")
        additional_out, out_csrs = self.connect_additional_io(output_pads, out_layout, "output")

        # Connect bus signals to pads but also insert SYN_BUFS
        self.comb += self.connect_to_pads(bus_pads, "master")

    def connect_additional_io(self, pads, layout, direction):
        assert direction in ["input", "output"]
        extend_io = []
        csr = []

        for name, width in layout:
            sig = Signal(width, name)
            extend_io.append(sig)
            if (direction == "input"):
                reg = CSRStorage(width, name="csr_"+name)
                csr.append(reg)
                self.sync += sig.eq(reg.storage)
            else:
                reg = CSRStatus(width, name="csr_"+name)
                csr.append(reg)
                self.sync += reg.status.eq(sig)

        return extend_io, csr


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
                    fd_inst = Instance("FD", i_C=clk, i_D=sig[i], o_Q=self.bus_bte[i])
                    fd_inst.attr.add("keep")
                    self.specials += fd_inst
                    self.specials += Instance("SYN_OBUF", name=name+str(i), i_I=self.bus_bte[i], o_O=pad[i])
                continue

            #if (name == "err"):
            #    self.bus_err = Signal(width)
            #    self.bus_err.attr.add("keep")
            #    self.bus.err.attr.add("keep")
            #    for i in range(width):
            #        err_lut = Instance("LUT1", name='k_'+name+str(i), i_I0=self.bus_err[i], o_O=self.bus_err[i], p_INIT=2)
            #        err_lut.attr.add("keep")
            #        self.specials += err_lut
            #        self.specials += Instance("SYN_IBUF", name=name+str(i), i_I=pad[i], o_O=self.bus_err[i])
            #    continue

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
