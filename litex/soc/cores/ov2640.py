from migen import *

from litex.soc.interconnect.csr import *
from migen.genlib.cdc import MultiReg

# OV2640 pads layout:
# "data":  8
# "pclk":  1
# "href":  1
# "vsync": 1

class OV2640(Module, AutoCSR):
    def __init__(self, pads, mode="single_frame", delay_pclk=0):

        assert mode in ["single_frame", "continous_stream"]

        self.pclk_valid = pclk_valid = Signal()

        self.data    = data    = [Signal(8) for _ in range(4)]
        self.pclk    = pclk    = [Signal()  for _ in range(4 + delay_pclk)]
        self.href    = href    = [Signal()  for _ in range(4)]
        self.vsync   = vsync   = [Signal()  for _ in range(5)] # Additional signal to check edge changes

        _data  = getattr(pads, "data")
        _pclk  = getattr(pads, "pclk")
        _href  = getattr(pads, "href")
        _vsync = getattr(pads, "vsync")

        # Connect pads to signals
        self.specials += [
            MultiReg(_pclk, pclk[0]),
            MultiReg(_href, href[0]),
            MultiReg(_vsync, vsync[0]),
        ]
        for i in range(len(data[3])):
            self.specials += MultiReg(_data[i], data[0][i])

        # Throw all signals into pipeline of 4 multiregs to stabilize tem
        for i in range(3):
            self.specials += [
                MultiReg(href[i], href[i + 1]),
                MultiReg(vsync[i], vsync[i + 1]),
            ]

            for j in range(len(data[3])):
                self.specials += MultiReg(data[i][j], data[i + 1][j])

        # Delayed vsync to check edge changes
        self.specials += MultiReg(vsync[3], vsync[4]),

        # For some reason pclk needs additional delay of 2 cycles so it's 6 in total
        for i in range(3 + delay_pclk):
            self.specials += MultiReg(pclk[i], pclk[i + 1]),

        self.outer_valid  = outer_valid  = Signal()
        self.pixel_rgb565 = pixel_rgb565 = Signal(16)
        self.pixel_rgba   = pixel_rgba   = Signal(32)
        self.first_pixel  = first_pixel  = Signal()
        inner_valid = Signal()
        last_data   = Signal(len(data[3]))
        counter     = Signal(32)

        # FSM
        self.submodules.fsm = fsm = FSM(reset_state="WAIT_START_VSYNC")

        fsm.act("WAIT_START_VSYNC",
            If(vsync[3],
                NextValue(pclk_valid, 0),
                NextValue(counter, 0),
                NextState("CAPTURE")
            )
        )
        fsm.act("CAPTURE",
            inner_valid.eq(vsync[3] & href[3] & pclk[3 + delay_pclk] & pclk_valid),
            If (inner_valid,
                If(counter == 0,
                    NextValue(first_pixel, 1),
                ),
                If(~counter & 1, #if counter % 2 == 0:
                    NextValue(last_data, data[3]),
                ).Else(
                    pixel_rgb565.eq((last_data << 8) | data[3]),
                    pixel_rgba.eq(                                          # Save RGBA pixel as Little Endian (ABGR32)
                        (last_data[3:] << 3) |                              # red   = pixel[11:16]
                        ((last_data[:3] << 5) | (data[3][5:] << 2)) << 8 |  # green = pixel[5:11]
                        (data[3][:5] << 3) << 16 |                          # blue  = pixel[:5]
                        0xff << 24                                          # alpha = 255 (not transparent)
                    ),
                    outer_valid.eq(1),
                    NextValue(first_pixel, 0),
                ),
                NextValue(counter, counter + 1),
                NextValue(pclk_valid, 0),
            ),
            If(~pclk[3 + delay_pclk],
                NextValue(pclk_valid, 1),
            ),
        )

        if mode == "single_frame":
            fsm.act("CAPTURE",
                If(~vsync[3],
                    NextState("WAIT_START_VSYNC")
                )
            )
        elif mode == "continous_stream":
            fsm.act("CAPTURE",
                If(~vsync[3],
                    NextValue(pclk_valid, 0),
                    NextValue(counter, 0),
                )
            )
