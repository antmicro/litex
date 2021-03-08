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
        self.pixel = pixel = Signal (16)
        pixel_d = Signal (16)

        self.sync += pixel_d.eq(pixel)

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
            pixel.eq(pixel_d),
            If (inner_valid,
                If(~counter & 1, #if counter % 2 == 0:
                    NextValue(last_data, data_d),
                ).Else(
                    pixel.eq((last_data << 8) | data_d),
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
