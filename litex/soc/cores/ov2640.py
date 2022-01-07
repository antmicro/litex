from litex.soc.interconnect.stream import AsyncFIFO
from migen import *
from migen.genlib.cdc import MultiReg
from migen.genlib.resetsync import AsyncResetSynchronizer

from litex.soc.interconnect.csr import *

from litex.soc.cores.clock import *

# OV2640 pads layout:
# "data":  8
# "pclk":  1
# "href":  1
# "vsync": 1

class OV2640(Module, AutoCSR):
    def __init__(self, pads, dma_busy_signal, mode="single_frame", delay_pclk=0):

        assert mode in ["single_frame", "continous_stream"]
        assert isinstance(dma_busy_signal, Signal)

        self.pclk_valid = pclk_valid = Signal()

        self.data    = data    = [Signal(8) for _ in range(4)]
        self.pclk    = pclk    = [Signal()  for _ in range(4 + delay_pclk)]
        self.href    = href    = [Signal()  for _ in range(4)]
        self.vsync   = vsync   = [Signal()  for _ in range(5)] # Additional signal to check edge changes

        _data  = getattr(pads, "data")
        _pclk  = getattr(pads, "pclk")
        _href  = getattr(pads, "href")
        _vsync = getattr(pads, "vsync")

        # Use PCLK as clock signal
        self.clock_domains.cd_pclk = ClockDomain()
        self.specials += [
            Instance("IBUFG", i_I=_pclk, o_O=self.cd_pclk.clk),
        ]
        self.specials += AsyncResetSynchronizer(self.cd_pclk, ResetSignal("sys"))

        self.comb += pclk[0].eq(self.cd_pclk.clk)

        # Connect pads to signals
        self.specials += [
            # MultiReg(self.cd_pclk.clk, pclk[0], odomain="pclk"),
            MultiReg(_href, href[0], odomain="pclk"),
            MultiReg(_vsync, vsync[0], odomain="pclk"),
        ]
        for i in range(len(data[3])):
            self.specials += MultiReg(_data[i], data[0][i], odomain="pclk")

        # Throw all signals into pipeline of 4 multiregs to stabilize tem
        for i in range(3):
            self.specials += [
                MultiReg(href[i], href[i + 1], odomain="pclk"),
                MultiReg(vsync[i], vsync[i + 1], odomain="pclk"),
            ]

            for j in range(len(data[3])):
                self.specials += MultiReg(data[i][j], data[i + 1][j], odomain="pclk")

        # Delayed vsync to check edge changes
        self.specials += MultiReg(vsync[3], vsync[4], odomain="pclk"),

        # For some reason pclk needs additional delay of 2 cycles so it's 6 in total
        for i in range(3 + delay_pclk):
            self.specials += MultiReg(pclk[i], pclk[i + 1], odomain="pclk"),

        self.outer_valid  = outer_valid  = Signal()
        self.pixel_rgb565 = pixel_rgb565 = Signal(16)
        self.pixel_rgba   = pixel_rgba   = Signal(32)
        self.first_pixel  = first_pixel  = Signal()
        self.inner_valid = inner_valid = Signal()
        last_data   = Signal(len(data[3]))
        self.counter = counter     = Signal(32)

        # FSM
        self.submodules.fsm = fsm = ClockDomainsRenamer("pclk")(FSM(reset_state="WAIT_START_DMA"))
        self.submodules.fifo = fifo = ClockDomainsRenamer({"write": "pclk", "read": "sys"})(AsyncFIFO([("data", 32)], depth=512))

        # self.comb += inner_valid.eq(vsync[3] & href[3] & pclk[3 + delay_pclk] & pclk_valid)

        fsm.act("WAIT_START_DMA",
            If(_vsync & dma_busy_signal,
                NextValue(pclk_valid, 0),
                NextValue(counter, 0),
                NextState("CAPTURE")
            )
        )
        fsm.act("CAPTURE",
            # inner_valid.eq(vsync[3] & href[3] & pclk[3 + delay_pclk] & pclk_valid),
            NextValue(outer_valid, 0),
            NextValue(first_pixel, 0),
            NextValue(fifo.sink.valid, 0),
            If(~dma_busy_signal,
                NextState("WAIT_START_DMA")
            ),
            If (_vsync & _href,
                If(counter == 0,
                    NextValue(first_pixel, 1),
                ),
                If(~counter & 1, #if counter % 2 == 0:
                    NextValue(last_data, _data),
                ).Else(
                    NextValue(pixel_rgb565, (last_data << 8) | data[0]),
                    NextValue(pixel_rgba,                                   # Save RGBA pixel as Little Endian (ABGR32)
                        (last_data[3:] << 3) |                              # red   = pixel[11:16]
                        ((last_data[:3] << 5) | (data[0][5:] << 2)) << 8 |  # green = pixel[5:11]
                        (data[0][:5] << 3) << 16 |                          # blue  = pixel[:5]
                        0xff << 24                                          # alpha = 255 (not transparent)
                    ),
                    NextValue(outer_valid, 1),
                    NextValue(fifo.sink.valid, 1),
                    NextValue(fifo.sink.data,                               # Save RGBA pixel as Little Endian (ABGR32)
                        (last_data[3:] << 3) |                              # red   = pixel[11:16]
                        ((last_data[:3] << 5) | (_data[5:] << 2)) << 8 |  # green = pixel[5:11]
                        (_data[:5] << 3) << 16 |                          # blue  = pixel[:5]
                        0xff << 24
                    ),
                ),
                NextValue(counter, counter + 1),
                NextValue(pclk_valid, 0),
            ),
            If(~pclk[0 + delay_pclk],
                NextValue(pclk_valid, 1),
            ),
        )

        if mode == "single_frame":
            fsm.act("CAPTURE",
                If(~_vsync,
                    NextState("WAIT_START_DMA")
                )
            )
        elif mode == "continous_stream":
            fsm.act("CAPTURE",
                If(~_vsync,
                    NextValue(pclk_valid, 0),
                    NextValue(counter, 0),
                )
            )
