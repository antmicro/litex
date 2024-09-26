#
# This file is part of LiteX.
#
# Copyright (c) 2014-2015 Sebastien Bourdeauducq <sb@m-labs.hk>
# Copyright (c) 2014-2020 Florent Kermarrec <florent@enjoy-digital.fr>
# Copyright (c) 2016-2018 Robert Jordens <jordens@gmail.com>
# Copyright (c) 2015 William D. Jones <thor0505@comcast.net>
# SPDX-License-Identifier: BSD-2-Clause

import os
import sys
import subprocess

from migen.fhdl.structure import *
from migen.fhdl.specials import Instance, Tristate
from migen.fhdl.module import Module
from migen.fhdl.verilog import _printexpr as verilog_printexpr
from migen.genlib.cdc import *
from migen.genlib.resetsync import AsyncResetSynchronizer

from litex.build.io import *
from litex.build import tools
from litex.build.yosys_wrapper import YosysWrapper

# Colorama -----------------------------------------------------------------------------------------

try:
    import colorama
    colorama.init()  # install escape sequence translation on Windows
    _have_colorama = True
except ImportError:
    _have_colorama = False

colors = []
if _have_colorama:
    colors += [
        ("^ERROR:.*$", colorama.Fore.RED + colorama.Style.BRIGHT +
         r"\g<0>" + colorama.Style.RESET_ALL),
        ("^CRITICAL WARNING:.*$", colorama.Fore.RED +
         r"\g<0>" + colorama.Style.RESET_ALL),
        ("^WARNING:.*$", colorama.Fore.YELLOW +
         r"\g<0>" + colorama.Style.RESET_ALL),
        ("^INFO:.*$", colorama.Fore.GREEN +
         r"\g<0>" + colorama.Style.RESET_ALL),
    ]

# Common MultiReg ----------------------------------------------------------------------------------

class XilinxMultiRegImpl(MultiRegImpl):
    def __init__(self, *args, **kwargs):
        MultiRegImpl.__init__(self, *args, **kwargs)
        i = self.i
        if not hasattr(i, "attr"):
            i0, i = i, Signal()
            self.comb += i.eq(i0)
        if len(self.regs):
            self.regs[0].attr.add("mr_ff")
        for r in self.regs:
            r.attr.add("async_reg")
            r.attr.add("no_shreg_extract")


class XilinxMultiReg:
    @staticmethod
    def lower(dr):
        return XilinxMultiRegImpl(dr.i, dr.o, dr.odomain, dr.n)

# Common AsyncResetSynchronizer --------------------------------------------------------------------

class XilinxAsyncResetSynchronizerImpl(Module):
    def __init__(self, cd, async_reset, bufg=False):
        if not hasattr(async_reset, "attr"):
            i, async_reset = async_reset, Signal()
            self.comb += async_reset.eq(i)
        rst_meta = Signal()
        rst_meta2 = Signal()
        self.specials += [
            Instance("FDPE",
                attr   = {"async_reg", "ars_ff1"},
                p_INIT = 1,
                i_PRE  = async_reset,
                i_CE   = 1,
                i_C    = cd.clk,
                i_D    = 0,
                o_Q    = rst_meta,
            ),
            Instance("FDPE",
                attr   = {"async_reg", "ars_ff2"},
                p_INIT = 1,
                i_PRE  = async_reset,
                i_CE   = 1,
                i_C    = cd.clk,
                i_D    = rst_meta,
                o_Q    = cd.rst if not bufg else rst_meta2
            ),
        ]
        if bufg:
            self.specials += [
                Instance("BUFG",
                    attr = {"ars_bufg"},
                    i_I = rst_meta2,
                    o_O = cd.rst
                )
            ]


class XilinxAsyncResetSynchronizer:
    @staticmethod
    def lower(dr):
        bufg = False
        if hasattr(dr, "bufg"):
            bufg = dr.bufg
        return XilinxAsyncResetSynchronizerImpl(dr.cd, dr.async_reset, bufg)

# Common DifferentialInput -------------------------------------------------------------------------

class XilinxDifferentialInputImpl(Module):
    def __init__(self, i_p, i_n, o):
        self.specials += Instance("IBUFDS",
            i_I  = i_p,
            i_IB = i_n,
            o_O  = o
        )


class XilinxDifferentialInput:
    @staticmethod
    def lower(dr):
        return XilinxDifferentialInputImpl(dr.i_p, dr.i_n, dr.o)

# Common DifferentialOutput ------------------------------------------------------------------------

class XilinxDifferentialOutputImpl(Module):
    def __init__(self, i, o_p, o_n):
        self.specials += Instance("OBUFDS",
            i_I  = i,
            o_O  = o_p,
            o_OB = o_n
        )


class XilinxDifferentialOutput:
    @staticmethod
    def lower(dr):
        return XilinxDifferentialOutputImpl(dr.i, dr.o_p, dr.o_n)

# Common SDRTristate -------------------------------------------------------------------------------

class XilinxSDRTristateImpl(Module):
    def __init__(self, io, o, oe, i, clk):
        _o    = Signal()
        _oe_n = Signal()
        _i    = Signal()
        self.specials += SDROutput(o, _o)
        self.specials += SDROutput(~oe, _oe_n)
        self.specials += SDRInput(_i, i)
        self.specials += Instance("IOBUF",
            io_IO = io,
            o_O   = _i,
            i_I   = _o,
            i_T   = _oe_n,
        )

class XilinxSDRTristate:
    @staticmethod
    def lower(dr):
        return XilinxSDRTristateImpl(dr.io, dr.o, dr.oe, dr.i, dr.clk)

# Common DDRTristate -------------------------------------------------------------------------------

class XilinxDDRTristateImpl(Module):
    def __init__(self, io, o1, o2, oe1, oe2, i1, i2, clk):
        _o    = Signal()
        _oe_n = Signal()
        _i    = Signal()
        self.specials += DDROutput(o1, o2, _o, clk)
        self.specials += DDROutput(~oe1, ~oe2, _oe_n, clk)
        self.specials += DDRInput(_i, i1, i2, clk)
        self.specials += Instance("IOBUF",
            io_IO = io,
            o_O   = _i,
            i_I   = _o,
            i_T   = _oe_n,
        )

class XilinxDDRTristate:
    @staticmethod
    def lower(dr):
        return XilinxDDRTristateImpl(dr.io, dr.o1, dr.o2, dr.oe1, dr.oe2, dr.i1, dr.i2, dr.clk)

# Common Memory ------------------------------------------------------------------------------------

class XilinxMemoryImpl:
    @staticmethod
    def emit_verilog(memory, namespace, add_data_file):
        # Helpers.
        # --------

        def _get_name(e):
            if isinstance(e, Memory):
                return namespace.get_name(e)
            else:
                return verilog_printexpr(namespace, e)[0]

        # Parameters.
        # -----------
        r         = ""
        adr_regs  = {}
        data_regs = {}

        # Ports Transformations.
        # ----------------------

        # Set Port Mode to Read-First when several Ports with different Clocks.
        # FIXME: Verify behaviour with the different FPGA toolchains, try to avoid it.
        clocks = [port.clock for port in memory.ports]
        if clocks.count(clocks[0]) != len(clocks):
            for port in memory.ports:
                port.mode = READ_FIRST

        # Set Port Granularity when 0.
        for port in memory.ports:
            if port.we_granularity == 0:
                port.we_granularity = memory.width

        # Memory Description.
        # -------------------
        r += "//" + "-"*78 + "\n"
        r += f"// Memory {_get_name(memory)}: {memory.depth}-words x {memory.width}-bit\n"
        r += "//" + "-"*78 + "\n"
        for n, port in enumerate(memory.ports):
            r += f"// Port {n} | "
            if port.async_read:
                r += "Read: Async | "
            else:
                r += "Read: Sync  | "
            if port.we is None:
                r += "Write: ---- | "
            else:
                r += "Write: Sync | "
                r += "Mode: "
                if port.mode == WRITE_FIRST:
                    r += "Write-First | "
                elif port.mode == READ_FIRST:
                    r += "Read-First  | "
                elif port.mode == NO_CHANGE:
                    r += "No-Change | "
                r += f"Write-Granularity: {port.we_granularity} "
            r += "\n"

        # Memory Logic Declaration/Initialization.
        # ----------------------------------------
        r += f"reg [{memory.width-1}:0] {_get_name(memory)}[0:{memory.depth-1}];\n"
        if memory.init is not None:
            content = ""
            formatter = f"{{:0{int(memory.width/4)}x}}\n"
            for d in memory.init:
                content += formatter.format(d)
            memory_filename = add_data_file(f"{memory.LiteX_name}_{_get_name(memory)}.init", content)

            r += "initial begin\n"
            r += f"\t$readmemh(\"{memory_filename}\", {_get_name(memory)});\n"
            r += "end\n"

        # Port Intermediate Signals.
        # --------------------------
        for n, port in enumerate(memory.ports):
            # No Intermediate Signal for Async Read.
            if port.async_read:
                continue

            # Create Address Register in Write-First mode.
            if port.mode in [WRITE_FIRST]:
                adr_regs[n] = Signal(name_override=f"{_get_name(memory)}_adr{n}")
                r += f"reg [{bits_for(memory.depth-1)-1}:0] {_get_name(adr_regs[n])};\n"

            # Create Data Register in Read-First/No Change mode.
            if port.mode in [READ_FIRST, NO_CHANGE]:
                data_regs[n] = Signal(name_override=f"{_get_name(memory)}_dat{n}")
                r += f"reg [{memory.width-1}:0] {_get_name(data_regs[n])};\n"

        # Ports Write/Read Logic.
        # -----------------------
        for n, port in enumerate(memory.ports):
            loop_var_name = f"{_get_name(memory)}_{n}_loop_var"
            # Declare loop variable before always block
            if port.we is not None and memory.width != port.we_granularity:
                r += f"integer {loop_var_name};\n"
            r += f"always @(posedge {_get_name(port.clock)}) begin\n"
            # Write Logic.
            if port.we is not None:
                # Create for loop with we.
                preamble = ""
                postamble = ""
                if memory.width != port.we_granularity:
                    preamble += f"for({loop_var_name} = 0; "
                    preamble += f"{loop_var_name} < {memory.width//port.we_granularity}; "
                    preamble += f"{loop_var_name}={loop_var_name}+1) begin\n"
                    postamble = "end\n"
                r += preamble
                wbit = f"[{loop_var_name}]" if memory.width != port.we_granularity else ""
                r += f"\tif ({_get_name(port.we)}{wbit}) begin\n"
                dslc = f"[{loop_var_name}*{port.we_granularity}+: {port.we_granularity}]" if (memory.width != port.we_granularity) else ""
                r += f"\t\t{_get_name(memory)}[{_get_name(port.adr)}]{dslc} <= {_get_name(port.dat_w)}{dslc};\n"
                r += "\tend\n"
                r += postamble

            # Read Logic.
            if not port.async_read:
                # In Write-First mode, Read from Address Register.
                if port.mode in [WRITE_FIRST]:
                    rd = f"\t{_get_name(adr_regs[n])} <= {_get_name(port.adr)};\n"

                # In Read-First/No Change mode:
                if port.mode in [READ_FIRST, NO_CHANGE]:
                    rd = ""
                    # Only Read in No-Change mode when no Write.
                    if port.mode == NO_CHANGE:
                        rd += f"\tif (!{_get_name(port.we)})\n\t"
                    # Read-First/No-Change Read logic.
                    rd += f"\t{_get_name(data_regs[n])} <= {_get_name(memory)}[{_get_name(port.adr)}];\n"

                # Add Read-Enable Logic.
                if port.re is None:
                    r += rd
                else:
                    r += f"\tif ({_get_name(port.re)})\n"
                    r += "\t" + rd.replace("\n\t", "\n\t\t")
            r += "end\n"

        # Ports Read Mapping.
        # -------------------
        for n, port in enumerate(memory.ports):
            # Direct (Asynchronous) Read on Async-Read mode.
            if port.async_read:
                r += f"assign {_get_name(port.dat_r)} = {_get_name(memory)}[{_get_name(port.adr)}];\n"
                continue

            # Write-First mode: Do Read through Address Register.
            if port.mode in [WRITE_FIRST]:
                r += f"assign {_get_name(port.dat_r)} = {_get_name(memory)}[{_get_name(adr_regs[n])}];\n"

            # Read-First/No-Change mode: Data already Read on Data Register.
            if port.mode in [READ_FIRST, NO_CHANGE]:
                 r += f"assign {_get_name(port.dat_r)} = {_get_name(data_regs[n])};\n"
        r += "\n\n"

        return r

# Common Special Overrides -------------------------------------------------------------------------

xilinx_special_overrides = {
    MultiReg:               XilinxMultiReg,
    AsyncResetSynchronizer: XilinxAsyncResetSynchronizer,
    DifferentialInput:      XilinxDifferentialInput,
    DifferentialOutput:     XilinxDifferentialOutput,
    SDRTristate:            XilinxSDRTristate,
    DDRTristate:            XilinxDDRTristate,
    Memory:                 XilinxMemoryImpl,
}

# Spartan6 DDROutput -------------------------------------------------------------------------------

class XilinxDDROutputImplS6(Module):
    def __init__(self, i1, i2, o, clk):
        self.specials += Instance("ODDR2",
            p_DDR_ALIGNMENT = "C0",
            p_INIT          = 0,
            p_SRTYPE        = "ASYNC",
            i_C0 = clk,
            i_C1 = ~clk,
            i_CE = 1,
            i_S  = 0,
            i_R  = 0,
            i_D0 = i1,
            i_D1 = i2,
            o_Q  = o
        )


class XilinxDDROutputS6:
    @staticmethod
    def lower(dr):
        return XilinxDDROutputImplS6(dr.i1, dr.i2, dr.o, dr.clk)

# Spartan6 DDRInput --------------------------------------------------------------------------------

class XilinxDDRInputImplS6(Module):
    def __init__(self, i, o1, o2, clk):
        self.specials += Instance("IDDR2",
            p_DDR_ALIGNMENT = "C0",
            p_INIT_Q0       = 0,
            p_INIT_Q1       = 0,
            p_SRTYPE        = "ASYNC",
            i_C0 = clk,
            i_C1 = ~clk,
            i_CE = 1,
            i_S  = 0,
            i_R  = 0,
            i_D  = i,
            o_Q0 = o1,
            o_Q1 = o2
        )


class XilinxDDRInputS6:
    @staticmethod
    def lower(dr):
        return XilinxDDRInputImplS6(dr.i, dr.o1, dr.o2, dr.clk)

# Spartan6 SDROutput -------------------------------------------------------------------------------

class XilinxSDROutputS6:
    @staticmethod
    def lower(dr):
        return XilinxDDROutputImplS6(dr.i, dr.i, dr.o, dr.clk)


# Spartan6 SDRInput --------------------------------------------------------------------------------

class XilinxSDRInputS6:
    @staticmethod
    def lower(dr):
        return XilinxDDRInputImplS6(dr.i, dr.o, Signal(), dr.clk)

# Spartan6 Special Overrides -----------------------------------------------------------------------

xilinx_s6_special_overrides = {
    DDROutput:   XilinxDDROutputS6,
    DDRInput:    XilinxDDRInputS6,
    SDROutput:   XilinxSDROutputS6,
    SDRInput:    XilinxSDRInputS6,
}

# 7-Series DDROutput -------------------------------------------------------------------------------

class XilinxDDROutputImplS7(Module):
    def __init__(self, i1, i2, o, clk):
        self.specials += Instance("ODDR",
            p_DDR_CLK_EDGE="SAME_EDGE",
            i_C  = clk,
            i_CE = 1,
            i_S  = 0,
            i_R  = 0,
            i_D1 = i1,
            i_D2 = i2,
            o_Q  = o
        )


class XilinxDDROutputS7:
    @staticmethod
    def lower(dr):
        return XilinxDDROutputImplS7(dr.i1, dr.i2, dr.o, dr.clk)

# 7-Series DDRInput --------------------------------------------------------------------------------

class XilinxDDRInputImplS7(Module):
    def __init__(self, i, o1, o2, clk):
        self.specials += Instance("IDDR",
            p_DDR_CLK_EDGE="SAME_EDGE",
            i_C  = clk,
            i_CE = 1,
            i_S  = 0,
            i_R  = 0,
            i_D  = i,
            o_Q1 = o1,
            o_Q2 = o2
        )


class XilinxDDRInputS7:
    @staticmethod
    def lower(dr):
        return XilinxDDRInputImplS7(dr.i, dr.o1, dr.o2, dr.clk)

# 7-Series SDROutput -------------------------------------------------------------------------------

class XilinxSDROutputS7:
    @staticmethod
    def lower(dr):
        return XilinxDDROutputImplS7(dr.i, dr.i, dr.o, dr.clk)


# 7-Series SDRInput --------------------------------------------------------------------------------

class XilinxSDRInputS7:
    @staticmethod
    def lower(dr):
        return XilinxDDRInputImplS7(dr.i, dr.o, Signal(), dr.clk)

# 7-Series Special Overrides -----------------------------------------------------------------------

xilinx_s7_special_overrides = {
    DDROutput: XilinxDDROutputS7,
    DDRInput:  XilinxDDRInputS7,
    SDROutput: XilinxSDROutputS7,
    SDRInput:  XilinxSDRInputS7,
}

# Ultrascale DDROutput -----------------------------------------------------------------------------

class XilinxDDROutputImplUS(Module):
    def __init__(self, i1, i2, o, clk):
        self.specials += Instance("ODDRE1",
            i_C  = clk,
            i_SR = 0,
            i_D1 = i1,
            i_D2 = i2,
            o_Q  = o
        )


class XilinxDDROutputUS:
    @staticmethod
    def lower(dr):
        return XilinxDDROutputImplUS(dr.i1, dr.i2, dr.o, dr.clk)

# Ultrascale DDRInput ------------------------------------------------------------------------------

class XilinxDDRInputImplUS(Module):
    def __init__(self, i, o1, o2, clk):
        self.specials += Instance("IDDRE1",
            p_DDR_CLK_EDGE="SAME_EDGE_PIPELINED",
            p_IS_C_INVERTED  = 0,
            p_IS_CB_INVERTED = 1,
            i_C  = clk,
            i_CB = clk,
            i_R  = 0,
            i_D  = i,
            o_Q1 = o1,
            o_Q2 = o2
        )


class XilinxDDRInputUS:
    @staticmethod
    def lower(dr):
        return XilinxDDRInputImplUS(dr.i, dr.o1, dr.o2, dr.clk)

# Ultrascale SDROutput -----------------------------------------------------------------------------

class XilinxSDROutputImplUS(Module):
    def __init__(self, i, o, clk):
        self.specials += Instance("FDCE",
            i_C   = clk,
            i_CE  = 1,
            i_CLR = 0,
            i_D   = i,
            o_Q   = o
        )

class XilinxSDROutputUS:
    @staticmethod
    def lower(dr):
        return XilinxSDROutputImplUS(dr.i, dr.o, dr.clk)

# Ultrascale SDRInput ------------------------------------------------------------------------------
class XilinxSDRInputImplUS(Module):
    def __init__(self, i, o, clk):
        self.specials += Instance("FDCE",
            i_C   = clk,
            i_CE  = 1,
            i_CLR = 0,
            i_D   = i,
            o_Q   = o
        )

class XilinxSDRInputUS:
    @staticmethod
    def lower(dr):
        return XilinxSDRInputImplUS(dr.i, dr.o, dr.clk)

# Ultrascale Specials Overrides --------------------------------------------------------------------

xilinx_us_special_overrides = {
    DDROutput: XilinxDDROutputUS,
    DDRInput:  XilinxDDRInputUS,
    SDROutput: XilinxSDROutputUS,
    SDRInput:  XilinxSDRInputUS,
}

# Yosys Run ----------------------------------------------------------------------------------------

def _build_yosys_project(platform, synth_opts="", build_name=""):
    family = ""
    device = platform.device
    if (device.startswith("xc7") or device.startswith("xa7") or device.startswith("xq7")):
        family = "xc7"
    elif (device.startswith("xc6s") or device.startswith("xa6s") or device.startswith("xq6s")):
        family = "xc6s"
    else:
        raise OSError("Unsupported device")

    yosys_cmd = [
        "hierarchy -top {build_name}",
        "# FIXME: Are these needed?",
        "# proc; memory; opt; fsm; opt",
        "# Map keep to keep=1 for yosys",
        "log",
        "log XX. Converting (* keep = \"xxxx\" *) attribute for Yosys",
        "log",
        "attrmap -tocase keep -imap keep=\"true\" keep=1 -imap keep=\"false\" keep=0 -remove keep=0",
        "select -list a:keep=1",
        "# Add keep=1 for yosys to objects which have dont_touch=\"true\" attribute.",
        "log",
        "log XX. Converting (* dont_touch = \"true\" *) attribute for Yosys",
        "log",
        "select -list a:dont_touch=true",
        "setattr -set keep 1 a:dont_touch=true",
        "# Convert (* async_reg = \"true\" *) to async registers for Yosys.",
        "# (* async_reg = \"true\", dont_touch = \"true\" *) reg xilinxmultiregimpl0_regs1 = 1'd0;",
        "log",
        "log XX. Converting (* async_reg = \"true\" *) attribute to async registers for Yosys",
        "log",
        "select -list a:async_reg=true",
        "setattr -set keep 1 a:async_reg=true",
    ]

    yosys = YosysWrapper(platform, build_name,
        target       = "xilinx",
        template     = [],
        yosys_cmds   = yosys_cmd,
        yosys_opts   = f"-family {family}",
        synth_format = "edif"
    )
    yosys.build_script()
    return yosys.get_yosys_call("script") + "\n"
