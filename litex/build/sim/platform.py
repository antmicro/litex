# This file is Copyright (c) 2015-2018 Florent Kermarrec <florent@enjoy-digital.fr>
# This file is Copyright (c) 2017 Pierre-Olivier Vauboin <po@lambdaconcept>
# License: BSD

import os

from migen.fhdl.structure import Signal, _Fragment
from migen.genlib.record import Record

from litex.build.generic_platform import GenericPlatform
from litex.build.sim import common, verilator
from litex.build import edalize


class SimPlatform(GenericPlatform):
    def __init__(self, *args, name="sim", toolchain="verilator", use_edalize=False, **kwargs):
        GenericPlatform.__init__(self, *args, name=name, **kwargs)
        self.sim_requested = []

        if use_edalize:
            self.toolchain = edalize.EdalizeToolchain(toolchain=toolchain)
        else:
            if toolchain == "verilator":
                self.toolchain = verilator.SimVerilatorToolchain()
            else:
                raise ValueError("Unknown toolchain")

    def request(self, name, number=None):
        index = ""
        if number is not None:
            index = str(number)
        obj = GenericPlatform.request(self, name, number=number)
        siglist = []
        if isinstance(obj, Signal):
            siglist.append((name, obj.nbits, name))
        elif isinstance(obj, Record):
            for subsignal, dummy in obj.iter_flat():
                subfname = subsignal.backtrace[-1][0]
                prefix = "{}{}_".format(name, index)
                subname = subfname.split(prefix)[1]
                siglist.append((subname, subsignal.nbits, subfname))
        self.sim_requested.append((name, index, siglist))
        return obj

    def get_verilog(self, *args, special_overrides=dict(), **kwargs):
        so = dict(common.sim_special_overrides)
        so.update(special_overrides)
        return GenericPlatform.get_verilog(self, *args, special_overrides=so, **kwargs)

    def build(self, fragment, build_dir="build", build_name="sim", run=True, *args, **kwargs):
        # Create build directory
        os.makedirs(build_dir, exist_ok=True)
        cwd = os.getcwd()
        os.chdir(build_dir)

        # FIXME: is this condition really required?
        if kwargs.get("build", True):
            # Finalize design
            if not isinstance(fragment, _Fragment):
                fragment = fragment.get_fragment()
            self.finalize(fragment)

        # Run toolchain
        vns = None
        try:
            vns = self.toolchain.build(self, fragment, build_dir, build_name, run, *args, **kwargs)
        finally:
            os.chdir(cwd)

        return vns

def sim_platform_args(parser):
    platform_group = parser.add_argument_group('Platform options')
    platform_group.add_argument("--use-edalize", action="store_true", help="Use Edalize toolchain backend")

def sim_platform_argdict(args):
    r = {}
    r.update({
        "use_edalize": args.use_edalize
    })
    return r
