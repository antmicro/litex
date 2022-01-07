from litex.soc.cores import led
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
    def __init__(self, pads, dma_busy_signal, leds_pads=None, mode="single_frame", delay_pclk=0):

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
        self._vsync   = _vsync   = getattr(pads, "vsync")
        self._vsync_d = _vsync_d = Signal()

        self.sync.pclk += _vsync_d.eq(_vsync)

        # Use PCLK as clock signal
        self.clock_domains.cd_pclk = ClockDomain()
        self.specials += [
            Instance("IBUFG", i_I=_pclk, o_O=self.cd_pclk.clk),
        ]

        self.first_pixel  = first_pixel  = Signal()
        self.counter      = counter      = Signal(32)
        last_data = Signal(len(data[3]))

        # FSM
        self.submodules.fsm = fsm = ClockDomainsRenamer("pclk")(FSM(reset_state="WAIT_START_DMA"))
        self.submodules.fifo = fifo = ClockDomainsRenamer({"write": "pclk", "read": "sys"})(AsyncFIFO([("data", 32)], depth=512))

        if isinstance(leds_pads, Cat):
            fifo_status = Signal(len(leds_pads))
            self.comb += leds_pads.eq(fifo_status)

            fsm.act("WAIT_START_DMA",
                If(fifo.source.valid,
                    NextValue(fifo_status, 0xff),
                ).Else(
                    NextValue(fifo_status, 0),
                ),
            )
            fsm.act("CAPTURE",
                If(fifo.source.valid,
                    NextValue(fifo_status, 0xff),
                ).Else(
                    NextValue(fifo_status, 0),
                ),
            )

        fsm.act("WAIT_START_DMA",
            NextValue(fifo.sink.valid, 0),
            If(_vsync & dma_busy_signal,
                NextValue(counter, 0),
                NextState("CAPTURE")
            )
        )
        fsm.act("CAPTURE",
            # inner_valid.eq(vsync[3] & href[3] & pclk[3 + delay_pclk] & pclk_valid),
            NextValue(first_pixel, 0),
            NextValue(fifo.sink.valid, 0),
            If(~dma_busy_signal,
                NextState("WAIT_START_DMA")
            ),
            If (_vsync & _href,
                If(counter == 1,
                    NextValue(first_pixel, 1),
                ),
                If(~counter & 1, #if counter % 2 == 0:
                    NextValue(last_data, _data),
                ).Else(
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
                    NextValue(counter, 0),
                )
            )
