# This file is Copyright (c) 2020 Antmicro <www.antmicro.com>
# License: BSD

import math
import edalize

from litex.build.generic_platform import IOStandard, Drive, Misc, Inverted
from litex.build import edalize_ext

# EdalizeToolchain ---------------------------------------------------------------------------------

class EdalizeToolchain:
    def __init__(self, toolchain):
        # FIXME: pass to edalize as custom commands
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

        self._toolchain = toolchain

    def build(self, platform, fragment, build_dir, build_name, run,
        synth_mode = "vivado",
        enable_xpm = False,
        **kwargs):

        # Generate verilog
        v_output = platform.get_verilog(fragment, name=build_name, **kwargs)
        named_sc, named_pc = platform.resolve_signals(v_output.ns)
        v_file = build_name + ".v"
        v_output.write(v_file)
        platform.add_source(v_file)

        # Translate IO information
        io = []
        for sig, pins, properties, _ in named_sc:
            # Translate properties from Litex classes to generic tuples
            io_properties = []
            for p in properties:
                if isinstance(p, IOStandard):
                    io_properties.append(("iostandard", p.name))
                elif isinstance(p, Drive):
                    io_properties.append(("drive", str(p.strength)))
                elif isinstance(p, Inverted):
                    io_properties.append(("inverted"))
                elif isinstance(p, Misc):
                    io_properties.append(("custom", str(p.misc)))
                else:
                    raise ValueError(f"unknown constraint {p}")
            io.append({
                "signal": sig,
                "pins": pins,
                "properties": io_properties
            })

        # Resolve signals in constraints
        period_constraints = dict()
        for clk, period in self.clocks.items():
            clk_signal = v_output.ns.get_name(clk)
            period_constraints[clk_signal] = period
        false_paths = set()
        for from_, to in self.false_paths:
            from_sig = v_output.ns.get_name(from_)
            to_sig   = v_output.ns.get_name(to)
            false_paths.add((from_sig, to_sig))

        # Make sure add_*_constraint cannot be used again
        del self.clocks
        del self.false_paths

        # Edalize
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
            "vpi":          [],

            # XXX: edalize_ext stuff
            "constraints": {
                "period":     period_constraints,
                "false_path": false_paths,
                "io":         io,
                "custom":     named_pc
            }
        }

        file_type_map = {
            "systemverilog": "systemVerilogSource",
            "verilog":       "verilogSource",
            "vhdl":          "vhdlSource",
        }
        for filename, language, library in platform.sources:
            edam["files"].append({ "name": filename, "file_type": file_type_map.get(language, "unknown") })

        # FIXME: this is not generic thing, move to platform or edalize_ext or somewhere
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

        backend_ext = edalize_ext.get_edatool(self._toolchain)(edam=edam, work_root=build_dir)
        backend_ext.configure()

        # NOTE: backend_ext.configure() modifies edam, so call it before this
        backend = edalize.get_edatool(self._toolchain)(edam=edam, work_root=build_dir)
        backend.configure()

        # Run
        if run:
            backend.build()

        return v_output.ns

    # NOTE (mglb): this code is common for every toolchain
    def add_period_constraint(self, platform, clk, period):
        period = math.floor(period*1e3)/1e3 # round to lowest picosecond
        if clk in self.clocks:
            if period != self.clocks[clk]:
                raise ValueError("Clock already constrained to {:.2f}ns, new constraint to {:.2f}ns"
                    .format(self.clocks[clk], period))
        self.clocks[clk] = period

    # NOTE (mglb): this code is common for every toolchain
    def add_false_path_constraint(self, platform, from_, to):
        if (to, from_) not in self.false_paths:
            self.false_paths.add((from_, to))

def edalize_args(parser):
    parser.add_argument("--use-edalize", action="store_true", help="Use Edalize toolchain backend")

def edalize_argdict(args):
    return {"use_edalize": args.use_edalize}
