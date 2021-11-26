from migen import *

from litex.soc.interconnect.csr import *
from migen.genlib.cdc import MultiReg

# OV2640 pads layout:
# "data":  8
# "pclk":  1
# "href":  1
# "vsync": 1

class OV2640(Module, AutoCSR):
    def __init__(self, pads, mode="single_frame"):

        assert mode in ["single_frame", "continous_stream"]

        self.pclk_valid = pclk_valid = Signal()

        # delayed signals
        self.data  = data  = Signal(8)
        self.pclk  = pclk  = Signal()
        self.href  = href  = Signal()
        self.vsync = vsync = Signal()
        self.vsync_d = vsync_d = Signal()

        _data  = getattr(pads, "data")
        _pclk  = getattr(pads, "pclk")
        _href  = getattr(pads, "href")
        _vsync = getattr(pads, "vsync")

        self.specials += [
            MultiReg(_pclk, pclk),
            MultiReg(_href, href),
            MultiReg(_vsync, vsync),
            MultiReg(vsync, vsync_d),
        ]

        for i in range(len(data)):
            self.specials += MultiReg(_data[i], data[i])

        self.outer_valid  = outer_valid  = Signal()
        self.pixel_rgb565 = pixel_rgb565 = Signal(16)
        self.pixel_rgba   = pixel_rgba   = Signal(32)
        inner_valid = Signal()
        last_data   = Signal(len(data))
        counter     = Signal(32)

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
        )

        if mode == "single_frame":
            fsm.act("CAPTURE",
                If(~vsync,
                    NextState("WAIT_END_VSYNC")
                )
            )
        elif mode == "continous_stream":
            fsm.act("CAPTURE",
                If(~vsync,
                    NextValue(pclk_valid, 0),
                    NextValue(counter, 0),
                )
            )
