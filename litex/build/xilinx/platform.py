# This file is Copyright (c) 2015 Sebastien Bourdeauducq <sb@m-labs.hk>
# This file is Copyright (c) 2015-2018 Florent Kermarrec <florent@enjoy-digital.fr>
# License: BSD

import os

from litex.build.generic_platform import GenericPlatform
from litex.build.xilinx import common, vivado, ise, symbiflow
from litex.build import edalize

from migen.fhdl.structure import _Fragment

# XilinxPlatform -----------------------------------------------------------------------------------

class XilinxPlatform(GenericPlatform):
    bitstream_ext = ".bit"

    def __init__(self, *args, toolchain="ise", **kwargs):
        GenericPlatform.__init__(self, *args, **kwargs)
        self.edifs = set()
        self.ips   = {}
        if toolchain == "ise":
            self.toolchain = ise.XilinxISEToolchain()
        elif toolchain == "vivado":
            self.toolchain = vivado.XilinxVivadoToolchain()
        elif toolchain == "symbiflow":
            self.toolchain = symbiflow.SymbiflowToolchain()
        elif toolchain == "edalize":
            self.toolchain = edalize.VivadoEdalizeToolchain()
        else:
            raise ValueError("Unknown toolchain")

    def add_edif(self, filename):
        self.edifs.add((os.path.abspath(filename)))

    def add_ip(self, filename, disable_constraints=False):
        self.ips.update({os.path.abspath(filename): disable_constraints})

    def get_verilog(self, *args, special_overrides=dict(), **kwargs):
        so = dict(common.xilinx_special_overrides)
        if self.device[:3] == "xc6":
            so.update(common.xilinx_s6_special_overrides)
        if self.device[:3] == "xc7":
            so.update(common.xilinx_s7_special_overrides)
        if self.device[:4] == "xcku":
            so.update(common.xilinx_us_special_overrides)
        so.update(special_overrides)
        return GenericPlatform.get_verilog(self, *args, special_overrides=so,
            attr_translate=self.toolchain.attr_translate, **kwargs)

    def get_edif(self, fragment, **kwargs):
        return GenericPlatform.get_edif(self, fragment, "UNISIMS", "Xilinx", self.device, **kwargs)

    def build(self, fragment, build_dir="build", build_name="top", run=True, **kwargs):
        # Create build directory
        os.makedirs(build_dir, exist_ok=True)
        cwd = os.getcwd()
        os.chdir(build_dir)

        # Finalize design
        if not isinstance(fragment, _Fragment):
            fragment = fragment.get_fragment()
        self.finalize(fragment)
        
        # Run toolchain
        vns = None
        try:
            vns = self.toolchain.build(self, fragment, build_dir, build_name, run, **kwargs)
        finally:
            os.chdir(cwd)

        return vns

    def add_period_constraint(self, clk, period):
        if clk is None: return
        if hasattr(clk, "p"):
            clk = clk.p
        clk.attr.add("keep")
        self.toolchain.add_period_constraint(self, clk, period)

    def add_false_path_constraint(self, from_, to):
        if hasattr(from_, "p"):
            from_ = from_.p
        if hasattr(to, "p"):
            to = to.p
        from_.attr.add("keep")
        to.attr.add("keep")
        self.toolchain.add_false_path_constraint(self, from_, to)
