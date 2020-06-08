# This file is Copyright (c) 2014-2020 Florent Kermarrec <florent@enjoy-digital.fr>
# License: BSD

import os
import subprocess
import sys
import math

import edalize

from migen.fhdl.structure import _Fragment

from litex.build.generic_platform import *
from litex.build import tools
from litex.build.xilinx import common

from pprint import pprint

# Constraints (.xdc) -------------------------------------------------------------------------------

def _xdc_separator(msg):
    r =  "#"*80 + "\n"
    r += "# " + msg + "\n"
    r += "#"*80 + "\n"
    return r

def _format_xdc_constraint(c):
    if isinstance(c, Pins):
        return "set_property LOC " + c.identifiers[0]
    elif isinstance(c, IOStandard):
        return "set_property IOSTANDARD " + c.name
    elif isinstance(c, Drive):
        return "set_property DRIVE " + str(c.strength)
    elif isinstance(c, Misc):
        return "set_property " + c.misc.replace("=", " ")
    elif isinstance(c, Inverted):
        return None
    else:
        raise ValueError("unknown constraint {}".format(c))


def _format_xdc(signame, resname, *constraints):
    fmt_c = [_format_xdc_constraint(c) for c in constraints]
    fmt_r = resname[0] + ":" + str(resname[1])
    if resname[2] is not None:
        fmt_r += "." + resname[2]
    r = "# {}\n".format(fmt_r)
    for c in fmt_c:
        if c is not None:
            r += c + " [get_ports {" + signame + "}]\n"
    r += "\n"
    return r


def _build_xdc(named_sc, named_pc):
    r = _xdc_separator("IO constraints")
    for sig, pins, others, resname in named_sc:
        if len(pins) > 1:
            for i, p in enumerate(pins):
                r += _format_xdc(sig + "[" + str(i) + "]", resname, Pins(p), *others)
        elif pins:
            r += _format_xdc(sig, resname, Pins(pins[0]), *others)
        else:
            r += _format_xdc(sig, resname, *others)
    if named_pc:
        r += _xdc_separator("Design constraints")
        r += "\n" + "\n\n".join(named_pc)
    return r

# XilinxVivadoToolchain ----------------------------------------------------------------------------

class EdalizeToolchain:
    def __init__(self):
        self.bitstream_commands                   = []
        self.additional_commands                  = []
        self.pre_synthesis_commands               = []
        self.pre_placement_commands               = []
        self.pre_routing_commands                 = []
        self.incremental_implementation           = False
        self.vivado_synth_directive               = "default"
        self.opt_directive                        = "default"
        self.vivado_place_directive               = "default"
        self.vivado_post_place_phys_opt_directive = None
        self.vivado_route_directive               = "default"
        self.vivado_post_route_phys_opt_directive = "default"
        self.clocks      = dict()
        self.false_paths = set()

    def _build_clock_constraints(self, platform):
        platform.add_platform_command(_xdc_separator("Clock constraints"))
        for clk, period in sorted(self.clocks.items(), key=lambda x: x[0].duid):
            platform.add_platform_command(
                "create_clock -name {clk} -period " + str(period) +
                " [get_nets {clk}]", clk=clk)
        for from_, to in sorted(self.false_paths,
                                key=lambda x: (x[0].duid, x[1].duid)):
            platform.add_platform_command(
                "set_clock_groups "
                "-group [get_clocks -include_generated_clocks -of [get_nets {from_}]] "
                "-group [get_clocks -include_generated_clocks -of [get_nets {to}]] "
                "-asynchronous",
                from_=from_, to=to)
        # Make sure add_*_constraint cannot be used again
        del self.clocks
        del self.false_paths

    def _build_false_path_constraints(self, platform):
        platform.add_platform_command(_xdc_separator("False path constraints"))
        # The asynchronous input to a MultiReg is a false path
        platform.add_platform_command(
            "set_false_path -quiet "
            "-through [get_nets -hierarchical -filter {{mr_ff == TRUE}}]"
        )
        # The asychronous reset input to the AsyncResetSynchronizer is a false path
        platform.add_platform_command(
            "set_false_path -quiet "
            "-to [get_pins -filter {{REF_PIN_NAME == PRE}} "
                "-of_objects [get_cells -hierarchical -filter {{ars_ff1 == TRUE || ars_ff2 == TRUE}}]]"
        )
        # clock_period-2ns to resolve metastability on the wire between the AsyncResetSynchronizer FFs
        platform.add_platform_command(
            "set_max_delay 2 -quiet "
            "-from [get_pins -filter {{REF_PIN_NAME == C}} "
                "-of_objects [get_cells -hierarchical -filter {{ars_ff1 == TRUE}}]] "
            "-to [get_pins -filter {{REF_PIN_NAME == D}} "
                "-of_objects [get_cells -hierarchical -filter {{ars_ff2 == TRUE}}]]"
        )


    def build(self, platform, fragment, build_dir, build_name, run,
        synth_mode = "vivado",
        enable_xpm = False,
        **kwargs):

        # Generate timing constraints
        self._build_clock_constraints(platform)
        self._build_false_path_constraints(platform)

        # Generate verilog
        v_output = platform.get_verilog(fragment, name=build_name, **kwargs)
        named_sc, named_pc = platform.resolve_signals(v_output.ns)
        v_file = build_name + ".v"
        v_output.write(v_file)
        platform.add_source(v_file)

        # Generate design constraints (.xdc)
        tools.write_to_file(build_name + ".xdc", _build_xdc(named_sc, named_pc))

        # Edalize
        toolchain = "vivado"
        edam = {
            "files":        [],
            "hooks":        [],
            "name":         build_name,
            "parameters":   {},
            "tool_options": {
                "vivado": {
                    "part":  platform.device,
                    "synth": synth_mode,
                }
            },
            "toplevel":     build_name,
            "vpi":          []
        }

        file_type_map = {
            "systemverilog": "systemVerilogSource",
            "verilog":       "verilogSource",
            "vhdl":          "vhdlSource",
        }
        for filename, language, library in platform.sources:
            edam["files"].append({ "name": filename, "file_type": file_type_map.get(language, "unknown") })

        for filename, disable_constraints in platform.ips.items():
            edam["files"].append({ "name": filename, "file_type": "xci" })
            # FIXME:
            #if disable_constraints:
            #    tcl.append("set_property is_enabled false [get_files -of_objects [get_files {}] -filter {{FILE_TYPE == XDC}}]".format(filename_tcl))

        # FIXME: custom edifs not supported in edalize
        #for filename in platform.edifs:
        #    edam["files"].append({ "name": filename, "file_type": "user" })

        for path in platform.verilog_include_paths:
            # FIXME: this is pretty hackish.
            # name and file_type are not used, but documentation marks them as mandatory.
            edam["files"].append({ "name": "", "file_type": "", "is_include_file": True, "include_path": path })

        edam["files"].append({ "name": build_name + ".xdc", "file_type": "xdc" })

        backend = edalize.get_edatool(toolchain)(edam=edam, work_root=build_dir)
        backend.configure()

        # Run
        if run:
            backend.build()

        return v_output.ns

    def add_period_constraint(self, platform, clk, period):
        clk.attr.add("keep")
        period = math.floor(period*1e3)/1e3 # round to lowest picosecond
        if clk in self.clocks:
            if period != self.clocks[clk]:
                raise ValueError("Clock already constrained to {:.2f}ns, new constraint to {:.2f}ns"
                    .format(self.clocks[clk], period))
        self.clocks[clk] = period

    def add_false_path_constraint(self, platform, from_, to):
        from_.attr.add("keep")
        to.attr.add("keep")
        if (to, from_) not in self.false_paths:
            self.false_paths.add((from_, to))

def edalize_build_args(parser):
    parser.add_argument("--synth-mode", default="vivado", help="synthesis mode (vivado or yosys, default=vivado)")


def edalize_build_argdict(args):
    return {"synth_mode": args.synth_mode}


#------------------------------------------------

class VivadoEdalizeToolchain(EdalizeToolchain):
    attr_translate = {
        "keep":            ("dont_touch", "true"),
        "no_retiming":     ("dont_touch", "true"),
        "async_reg":       ("async_reg",  "true"),
        "mr_ff":           ("mr_ff",      "true"), # user-defined attribute
        "ars_ff1":         ("ars_ff1",    "true"), # user-defined attribute
        "ars_ff2":         ("ars_ff2",    "true"), # user-defined attribute
        "no_shreg_extract": None
    }
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)