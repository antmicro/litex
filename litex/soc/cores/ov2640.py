from migen import *

from litex.soc.interconnect.csr import *
from migen.genlib.cdc import MultiReg

_ov2640_pads_layout = [
    ("data",    8),
    ("pclk",    1),
    ("href",    1),
    ("vsync",   1),
]

class OV2640(Module, AutoCSR):
    def __init__(self, pads):

        self.camera_pads = camera_pads = Record(_ov2640_pads_layout)

        self.pclk_valid = pclk_valid = Signal ()

        # delayed signals
        self.data  = data  = Signal(8)
        self.pclk  = pclk  = Signal()
        self.href  = href  = Signal()
        self.vsync = vsync = Signal()
        self.vsync_d = vsync_d = Signal()

        self.comb += [
            camera_pads.data.eq(getattr(pads, "data")),
            camera_pads.pclk.eq(getattr(pads, "pclk")),
            camera_pads.href.eq(getattr(pads, "href")),
            camera_pads.vsync.eq(getattr(pads, "vsync")),
        ]

        self.specials += [
            MultiReg(camera_pads.pclk, pclk),
            MultiReg(camera_pads.href, href),
            MultiReg(camera_pads.vsync, vsync),
            MultiReg(vsync, vsync_d),
        ]

        for i in range(8):
            self.specials += MultiReg(camera_pads.data[i], data[i])

        inner_valid = Signal ()
        self.outer_valid = outer_valid = Signal ()
        last_data = Signal (8)
        counter = Signal (32)
        self.pixel_rgb565 = pixel_rgb565 = Signal (16)
        self.pixel_rgba   = pixel_rgba   = Signal (32)

        # FSM
        self.submodules.fsm = fsm = FSM(reset_state="WAIT_END_VSYNC")

        fsm.act("WAIT_END_VSYNC",
            If(~vsync,
                NextState("WAIT_START_VSYNC")
            )
        )
        fsm.act("WAIT_START_VSYNC",
            If(vsync,
                NextValue(pclk_valid, 0),
                NextValue(counter, 0),
                NextState("CAPTURE")
            )
        )
        fsm.act("CAPTURE",
            inner_valid.eq(vsync & href & pclk & pclk_valid),
            If (inner_valid,
                If(~counter & 1, #if counter % 2 == 0:
                    NextValue(last_data, data),
                ).Else(
                    pixel_rgb565.eq((last_data << 8) | data),
                    pixel_rgba.eq(                                          # Save RGBA pixel as Little Endian (ABGR32)
                        (last_data[3:] << 3) |                              # red   = pixel[11:16]
                        ((last_data[:3] << 5) | (data[5:] << 2)) << 8 |     # green = pixel[5:11]
                        (data[:5] << 3) << 16 |                             # blue  = pixel[:5]
                        0xff << 24                                          # alpha = 255 (not transparent)
                    ),
                    outer_valid.eq(1),
                ),
                NextValue(counter, counter + 1),
                NextValue(pclk_valid, 0),
            ),
            If(~pclk,
                NextValue(pclk_valid, 1),
            ),
            If(~vsync,
                NextState("WAIT_END_VSYNC")
            )
        )
