#
# This file is part of LiteX.
#
# Copyright (c) 2020 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: BSD-2-Clause

from migen import *

from litex.build.generic_platform import *
from litex.soc.interconnect import wishbone
from litex.soc.interconnect.csr import *

# Xilinx 7-series ----------------------------------------------------------------------------------

class PRMConnector(Module):
    def __init__(self, name, signal):
        self.name = name

        if type(signal) is Cat:
            self._signal = signal.l
            self._signal_width = 0
            for s in self._signal:
                self._signal_width += s.nbits
        else:
            self._signal = signal
            self._signal_width = signal.nbits

        self.width = self._signal_width

    def connect(self, intern_signal):
        assert self.width == intern_signal.nbits
        self._intern_signal = intern_signal

    def get_intern_io(self, i):
        return self._intern_signal[i]

    def get_intern_io_all(self):
        return self._intern_signal

    def get_core_io(self, i):
        if type(self._signal) is list:
            nbits = 0
            for sig in self._signal:
                nbits += sig.nbits
                if i < nbits:
                    return sig[sig.nbits - (nbits - i)]
        else:
            return self._signal[i]

    def get_core_io_all(self):
            return self._signal


class PRM(Module):
    """PRM (Partially Reconfigurable Module)

    PRM module is used to setup a design of an FPGA to enable partial reconfiguration
    in the SymbiFlow toolchain.

    It is assumed that PRM core is interfaced through a bus and additional IO signals which
    can be connected to e.g. physical pins. In order to make the partially reconfigurable
    design work correctly, the PRM must be defined for both static (overlay)
    and dynamic (ROI) design. In static part a bus slave with a reserved memory space
    must be defined and passed to the PRM along with any additional IOs (ROI core is not defined
    in the static part). In dynamic (ROI) part PRM must be defined accordingly and proper
    submodules (cores) must be used within the ROI. To prepare the ROI design
    it is recommended to use `litex_gen.py` tool, which is designated to generate
    cores with a bus and IOs routed to the top module.
    """
    def __init__(self, platform, bus, bus_type, mode, domain="sys", roi_ins=[], roi_outs=[]):
        assert mode in ["overlay", "roi"]
        assert bus_type in ["wishbone", "axi"]

        self.platform = platform
        self.bus = bus
        self.bus_io_name = "wb" if bus_type == "wishbone" else "axi"
        self.roi_ins = roi_ins
        self.roi_outs = roi_outs

        self._prepare_prm_signals(roi_ins, roi_outs)

        clk = ClockSignal(domain)
        if mode == "overlay":
            rst = ResetSignal(domain)
            self.specials += Instance("SYN_OBUF", i_I=clk, o_O=self.get_clk_pad())
            self.specials += Instance("SYN_OBUF", i_I=rst, o_O=self.get_rst_pad())

        self.connect_roi_ios(clk, roi_ins, roi_outs, mode)
        self.connect_roi_bus(clk, mode)

    def _prepare_prm_signals(self, roi_ins=[], roi_outs=[]):
        assert type(roi_ins) is list and type(roi_outs) is list

        # Set clock and reset as SYN0 and SYN1
        clk_pad = ("prm_clk", 0, Pins("SYN0"))
        rst_pad = ("prm_rst", 0, Pins("SYN1"))
        self.platform.add_extension([clk_pad])
        self.platform.add_extension([rst_pad])

        global_index = 2

        # Generate synth IOs for bus
        bus_constrs = ()
        for name, width, direction in self.bus.layout:
            constrs = str()
            for i in range(width):
                constrs += "SYN{} ".format(global_index + i)
            bus_constrs += (Subsignal(name, Pins(constrs[:-1])),)
            global_index += width
        bus_synth_ios = [(self.bus_io_name, 0) + bus_constrs]
        self.platform.add_extension(bus_synth_ios)

        # Generate synth IOs for roi_ins
        for sig_in in roi_ins:
            constrs = str()
            for i in range(sig_in.width):
                constrs += "SYN{} ".format(global_index + i)
            roi_ins_constrs = (Pins(constrs[:-1]),)
            global_index += width
            roi_ins_synth_ios = [(sig_in.name, 0) + roi_ins_constrs]
            self.platform.add_extension(roi_ins_synth_ios)
            sig_in.connect(self.platform.request(sig_in.name))

        # Generate synth IOs for roi_outs
        for sig_out in roi_outs:
            constrs = str()
            for i in range(sig_out.width):
                constrs += "SYN{} ".format(global_index + i)
            roi_outs_constrs = (Pins(constrs[:-1]),)
            global_index += width
            roi_outs_synth_ios = [(sig_out.name, 0) + roi_outs_constrs]
            self.platform.add_extension(roi_outs_synth_ios)
            sig_out.connect(self.platform.request(sig_out.name))

    def get_clk_pad(self):
        return self.platform.request("prm_clk")

    def get_rst_pad(self):
        return self.platform.request("prm_rst")

    def get_bus_pads(self):
        return self.platform.request(self.bus_io_name)

    def get_platform(self):
        return self.platform

    def connect_roi_ios(self, clk, roi_ins, roi_outs, mode):
        if roi_outs is not None:
            for io in roi_outs:
                sig = Signal(io.width)
                for i in range(io.width):
                    if mode == "roi":
                        fd_inst = Instance("FD",
                                            i_C=clk,
                                            i_D=io.get_core_io(i),
                                            o_Q=io.get_intern_io(i))
                        fd_inst.attr.add("keep")
                        self.specials += fd_inst
                    else:
                        self.specials += Instance("SYN_IBUF",
                                                   name=io.name+str(i),
                                                   i_I=io.get_intern_io(i),
                                                   o_O=io.get_core_io(i))
        if roi_ins is not None:
            for io in roi_ins:
                sig = Signal(io.width)
                for i in range(io.width):
                    if mode == "overlay":
                        fd_inst = Instance("FD",
                                            i_C=clk,
                                            i_D=io.get_core_io(i),
                                            o_Q=sig[i])
                        fd_inst.attr.add("keep")
                        self.specials += fd_inst
                        self.specials += Instance("SYN_OBUF",
                                                   name=io.name+str(i),
                                                   i_I=sig[i],
                                                   o_O=io.get_intern_io(i))

    def connect_roi_bus(self, clk, mode):
        bus_pads = self.get_bus_pads()
        for name, width, direction in self.bus.layout:
            sig  = getattr(self.bus, name)
            pad  = getattr(bus_pads, name)

            if mode == "overlay":
                # Use some active logic to avoid direct routing to GND
                if (name == "bte"):
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
                inter_sig = Signal(width)
                if direction == DIR_S_TO_M:
                    self.comb += inter_sig.eq(sig)
                    for i in range(width):
                        fd_inst = Instance("FD",
                                            i_C=clk,
                                            i_D=sig[i],
                                            o_Q=pad[i])
                        fd_inst.attr.add("keep")
                        self.specials += fd_inst
                else:
                    self.comb += sig.eq(pad)
