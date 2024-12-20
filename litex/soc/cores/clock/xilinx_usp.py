#
# This file is part of LiteX.
#
# Copyright (c) 2018-2020 Florent Kermarrec <florent@enjoy-digital.fr>
# Copyright (c) 2024 Antmicro <mdudek@antmicro.com>
# SPDX-License-Identifier: BSD-2-Clause


from litex.soc.cores.clock.common import *
from litex.soc.cores.clock.xilinx_common import *

# Xilinx / Ultrascale Plus -------------------------------------------------------------------------

class USPPLL(XilinxClocking):
    nclkouts_max = 2

    def __init__(self, speedgrade=-1, no_reset_delay=False):
        self.logger = logging.getLogger("USPPLL")
        self.logger.info(f"Creating USPPLL, {f'speedgrade {speedgrade}'}.")
        self.no_reset_delay= no_reset_delay
        XilinxClocking.__init__(self)
        self.divclk_divide_range = (1, 15+1)
        self.clkfbout_mult_frange = (2, 21+1)
        self.clkin_freq_range = {
            -1: (70e6,  800e6),
            -2: (70e6,  933e6),
            -3: (70e6, 1066e6),
        }[speedgrade]
        self.vco_freq_range = {
            -1: (750e6, 1500e6),
            -2: (750e6, 1500e6),
            -3: (750e6, 1500e6),
        }[speedgrade]
        self.phy_mode = None
        self.pll_phy_en = None
        self.pll_phy_clk = None
        self.alignment_required = False

    def create_clkout_phy(
        self,
        phy_mode,
        freq,
        pll_phy_en=None,
        pll_phy_clk=None,
        alignment_required=True
    ):
        self.phy_mode = phy_mode
        self.pll_phy_en = pll_phy_en
        self.pll_phy_clk = pll_phy_clk
        if self.pll_phy_en is None:
            self.pll_phy_en = Signal()
        if self.pll_phy_clk is None:
            self.pll_phy_clk = Signal()
        self.alignment_required = alignment_required
        # Create PHY output and mark it -1, this should remove conflicts with
        # other clock outputs
        self.clkouts[-1]=(self.pll_phy_clk, freq, 0, 1e-2, 0.50)

    def do_finalize(self):
        XilinxClocking.do_finalize(self)
        config = self.compute_config()
        pll_fb = Signal()
        self.params.update(
            # Global.
            p_STARTUP_WAIT = "FALSE",
            i_RST          = self.reset,
            i_PWRDWN       = self.power_down,
            o_LOCKED       = self.locked,

            # VCO.
            p_REF_JITTER    = 0.01,
            p_CLKIN_PERIOD  = 1e9/self.clkin_freq,
            p_CLKFBOUT_MULT = config["clkfbout_mult"],
            p_DIVCLK_DIVIDE = config["divclk_divide"],
            i_CLKIN         = self.clkin,
            i_CLKFBIN       = pll_fb,
            o_CLKFBOUT      = pll_fb,
        )
        for n, (clk, _, _, _, _) in sorted(self.clkouts.items()):
            if n == -1:
                continue
            self.params[f"p_CLKOUT{n}_DIVIDE"] = config[f"clkout{n}_divide"]
            self.params[f"p_CLKOUT{n}_PHASE"] = config[f"clkout{n}_phase"]
            self.params[f"p_CLKOUT{n}_DUTY_CYCLE"] = config[f"clkout{n}_duty_cycle"]
            self.params[f"o_CLKOUT{n}"] = clk
        if self.phy_mode is not None:
            self.params["p_COMPENSATION"] = "PHY_ALIGN"
            self.params["p_CLKOUTPHY_MODE"] = self.phy_mode
            self.params["o_CLKOUTPHY"] = self.pll_phy_clk
            self.params["i_CLKOUTPHYEN"] = self.pll_phy_en
            if (self.alignment_required and
                (
                    config["clkfbout_mult"] not in [1, 2, 4, 8] or
                    config["divclk_divide"] not in [1, 2, 4, 8]
                )
            ):
                raise AssertionError("CLKOUTPHY alignment is required, but either of "
                                     "clkfbout_mult or divclk_divide is not equal to "
                                     "1, 2, 4, or 8")

        self.specials += Instance("PLLE4_ADV", **self.params)


class USPMMCM(XilinxClocking):
    nclkouts_max = 7

    def __init__(self, speedgrade=-1):
        self.logger = logging.getLogger("USPMMCM")
        self.logger.info(f"Creating USPMMCM, {f'speedgrade {speedgrade}'}.")
        XilinxClocking.__init__(self)
        self.divclk_divide_range = (1, 106+1)
        self.clkfbout_mult_frange = (2*8, (128+1)*8, 8)
        self.clkin_freq_range = {
            -1: (10e6,  800e6),
            -2: (10e6,  933e6),
            -3: (10e6, 1066e6),
        }[speedgrade]
        self.vco_freq_range = {
            -1: (800e6, 1600e6),
            -2: (800e6, 1600e6),
            -3: (800e6, 1600e6),
        }[speedgrade]

    def do_finalize(self):
        XilinxClocking.do_finalize(self)
        config = self.compute_config()
        mmcm_fb = Signal()
        _mult_int = config["clkfbout_mult"] // config["clkfbout_mult_divisor"]
        _mult_frac = 125*(config["clkfbout_mult"] % config["clkfbout_mult_divisor"])

        self.params.update(
            # Global.
            p_BANDWIDTH = "OPTIMIZED",
            i_RST       = self.reset,
            i_PWRDWN    = self.power_down,
            o_LOCKED    = self.locked,

            # VCO.
            p_REF_JITTER1     = 0.01,
            p_CLKIN1_PERIOD   = 1e9/self.clkin_freq,
            p_CLKFBOUT_MULT_F = float(f"{_mult_int}.{_mult_frac}"),
            p_DIVCLK_DIVIDE   = config["divclk_divide"],
            i_CLKIN1          = self.clkin,
            i_CLKFBIN         = mmcm_fb,
            o_CLKFBOUT        = mmcm_fb,
        )
        for n, (clk, _, _, _, _) in sorted(self.clkouts.items()):
            if n == 0:
                self.params[f"p_CLKOUT{n}_DIVIDE_F"] = config[f"clkout{n}_divide"]
            else:
                self.params[f"p_CLKOUT{n}_DIVIDE"] = config[f"clkout{n}_divide"]
            self.params[f"p_CLKOUT{n}_PHASE"] = config[f"clkout{n}_phase"]
            self.params[f"p_CLKOUT{n}_DUTY_CYCLE"] = config[f"clkout{n}_duty_cycle"]
            self.params[f"o_CLKOUT{n}"] = clk
        self.specials += Instance("MMCME4_ADV", **self.params)


class USPIDELAYCTRL(Module):
    def __init__(self, cd_ref, cd_sys, reset_cycles=64, ready_cycles=64):
        self.clock_domains.cd_ic = ClockDomain()
        ic_reset_counter = Signal(max=reset_cycles, reset=reset_cycles-1)
        ic_reset         = Signal(reset=1)
        cd_ref_sync      = getattr(self.sync, cd_ref.name)
        cd_ref_sync += [
            If(ic_reset_counter != 0,
                ic_reset_counter.eq(ic_reset_counter - 1)
            ).Else(
                ic_reset.eq(0)
            )
        ]
        ic_ready_counter = Signal(max=ready_cycles, reset=ready_cycles-1)
        ic_ready         = Signal()
        self.comb += self.cd_ic.clk.eq(cd_sys.clk)
        self.sync.ic += [
            cd_sys.rst.eq(1),
            If(ic_ready,
                If(ic_ready_counter != 0,
                    ic_ready_counter.eq(ic_ready_counter - 1)
                ).Else(
                    cd_sys.rst.eq(0)
                )
            )
        ]
        self.specials += [
            Instance("IDELAYCTRL",
                p_SIM_DEVICE = "ULTRASCALE",
                i_REFCLK     = cd_ref.clk,
                i_RST        = ic_reset,
                o_RDY        = ic_ready),
            AsyncResetSynchronizer(self.cd_ic, ic_reset)
        ]
