from migen import *

from litex.soc.interconnect.csr import *
from litedram.frontend.dma import LiteDRAMDMAWriter

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
        self.data_d = data_d = Signal(8)
        self.pclk_d = pclk_d = Signal()
        self.href_d = href_d = Signal()
        self.vsync_d = vsync_d = Signal()

        self.comb += [
            camera_pads.data.eq(getattr(pads, "data")),
            camera_pads.pclk.eq(getattr(pads, "pclk")),
            camera_pads.href.eq(getattr(pads, "href")),
            camera_pads.vsync.eq(getattr(pads, "vsync")),
        ]

        self.sync += [
            data_d.eq(camera_pads.data),
            pclk_d.eq(camera_pads.pclk),
            href_d.eq(camera_pads.href),
            vsync_d.eq(camera_pads.vsync),
        ]

        inner_valid = Signal ()
        self.outer_valid = outer_valid = Signal ()
        last_data = Signal (8)
        counter = Signal (32)
        self.pixel_rgb565 = pixel_rgb565 = Signal (16)
        self.pixel_rgba = pixel_rgba     = Signal (32)

        # FSM
        self.submodules.fsm = fsm = FSM(reset_state="WAIT_END_VSYNC")

        fsm.act("WAIT_END_VSYNC",
            If(~vsync_d,
                NextState("WAIT_START_VSYNC")
            )
        )
        fsm.act("WAIT_START_VSYNC",
            If(vsync_d,
                NextValue(pclk_valid, 0),
                NextValue(counter, 0),
                NextState("CAPTURE")
            )
        )
        fsm.act("CAPTURE",
            inner_valid.eq(vsync_d & href_d & pclk_d & pclk_valid),
            If (inner_valid,
                If(~counter & 1, #if counter % 2 == 0:
                    NextValue(last_data, data_d),
                ).Else(
                    pixel_rgb565.eq((last_data << 8) | data_d),
                    pixel_rgba.eq(                                          # Save RGBA pixel as Little Endian (ABGR32)
                        (last_data[3:] << 3) |                              # red   = pixel[11:16]
                        ((last_data[:3] << 5) | (data_d[5:] << 2)) << 8 |   # green = pixel[5:11]
                        (data_d[:5] << 3) << 16 |                           # blue  = pixel[:5]
                        0xff << 24                                          # alpha = 255 (not transparent)
                    ),
                    outer_valid.eq(1),
                ),
                NextValue(counter, counter + 1),
                NextValue(pclk_valid, 0),
            ),
            If(~pclk_d,
                NextValue(pclk_valid, 1),
            ),
            If(~vsync_d,
                NextState("WAIT_END_VSYNC")
            )
        )
