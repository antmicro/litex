# This file is Copyright (c) 2018-2019 Florent Kermarrec <florent@enjoy-digital.fr>
# This file is Copyright (c) 2020 Antmicro <www.antmicro.com>
# License: BSD

"""AXI4 Full/Lite support for LiteX"""

from migen import *
from migen.genlib import roundrobin
from migen.genlib.misc import split, displacer, chooser, WaitTimer

from litex.soc.interconnect import stream
from litex.build.generic_platform import *

from litex.soc.interconnect import csr_bus

# AXI Definition -----------------------------------------------------------------------------------

BURST_FIXED    = 0b00
BURST_INCR     = 0b01
BURST_WRAP     = 0b10
BURST_RESERVED = 0b11

RESP_OKAY      = 0b00
RESP_EXOKAY    = 0b01
RESP_SLVERR    = 0b10
RESP_DECERR    = 0b11

def ax_description(address_width, id_width):
    return [
        ("addr",  address_width),
        ("burst", 2), # Burst type
        ("len",   8), # Number of data (-1) transfers (up to 256)
        ("size",  4), # Number of bytes (-1) of each data transfer (up to 1024 bits)
        ("lock",  2), # *
        ("prot",  3), # *
        ("cache", 4), # *
        ("qos",   4), # *
        ("id",    id_width)
    ]
    # * present for interconnect with others cores but not used by LiteX.

def w_description(data_width, id_width):
    return [
        ("data", data_width),
        ("strb", data_width//8),
        ("id",   id_width)
    ]

def b_description(id_width):
    return [
        ("resp", 2),
        ("id",   id_width)
    ]

def r_description(data_width, id_width):
    return [
        ("resp", 2),
        ("data", data_width),
        ("id",   id_width)
    ]

def _connect_axi(master, slave):
    channel_modes = {
        "aw": "master",
        "w" : "master",
        "b" : "slave",
        "ar": "master",
        "r" : "slave",
    }
    r = []
    for channel, mode in channel_modes.items():
        if mode == "master":
            m, s = getattr(master, channel), getattr(slave, channel)
        else:
            s, m = getattr(master, channel), getattr(slave, channel)
        r.extend(m.connect(s))
    return r

def _axi_layout_flat(axi):
    # yields tuples (channel, name, direction)
    def get_dir(channel, direction):
        if channel in ["b", "r"]:
            return {DIR_M_TO_S: DIR_S_TO_M, DIR_S_TO_M: DIR_M_TO_S}[direction]
        return direction
    for ch in ["aw", "w", "b", "ar", "r"]:
        channel = getattr(axi, ch)
        for group in channel.layout:
            if len(group) == 3:
                name, _, direction = group
                yield ch, name, get_dir(ch, direction)
            else:
                _, subgroups = group
                for subgroup in subgroups:
                    name, _, direction = subgroup
                    yield ch, name, get_dir(ch, direction)

class AXIInterface:
    def __init__(self, data_width=32, address_width=32, id_width=1, clock_domain="sys"):
        self.data_width    = data_width
        self.address_width = address_width
        self.id_width      = id_width
        self.clock_domain  = clock_domain

        self.aw = stream.Endpoint(ax_description(address_width, id_width))
        self.w  = stream.Endpoint(w_description(data_width, id_width))
        self.b  = stream.Endpoint(b_description(id_width))
        self.ar = stream.Endpoint(ax_description(address_width, id_width))
        self.r  = stream.Endpoint(r_description(data_width, id_width))

    def connect(self, slave):
        return _connect_axi(self, slave)

    def layout_flat(self):
        return list(_axi_layout_flat(self))

# AXI Lite Definition ------------------------------------------------------------------------------

def ax_lite_description(address_width):
    return [("addr",  address_width)]

def w_lite_description(data_width):
    return [
        ("data", data_width),
        ("strb", data_width//8)
    ]

def b_lite_description():
    return [("resp", 2)]

def r_lite_description(data_width):
    return [
        ("resp", 2),
        ("data", data_width)
    ]

class AXILiteInterface:
    def __init__(self, data_width=32, address_width=32, clock_domain="sys"):
        self.data_width    = data_width
        self.address_width = address_width
        self.clock_domain  = clock_domain

        self.aw = stream.Endpoint(ax_lite_description(address_width))
        self.w  = stream.Endpoint(w_lite_description(data_width))
        self.b  = stream.Endpoint(b_lite_description())
        self.ar = stream.Endpoint(ax_lite_description(address_width))
        self.r  = stream.Endpoint(r_lite_description(data_width))

    def get_ios(self, bus_name="wb"):
        subsignals = []
        for channel in ["aw", "w", "b", "ar", "r"]:
            for name in ["valid", "ready"]:
                subsignals.append(Subsignal(channel + name, Pins(1)))
            for name, width in getattr(self, channel).description.payload_layout:
                subsignals.append(Subsignal(channel + name, Pins(width)))
        ios = [(bus_name , 0) + tuple(subsignals)]
        return ios

    def connect_to_pads(self, pads, mode="master"):
        assert mode in ["slave", "master"]
        r = []
        def swap_mode(mode): return "master" if mode == "slave" else "slave"
        channel_modes = {
            "aw": mode,
            "w" : mode,
            "b" : swap_mode(mode),
            "ar": mode,
            "r" : swap_mode(mode),
        }
        for channel, mode in channel_modes.items():
            for name, width in [("valid", 1)] + getattr(self, channel).description.payload_layout:
                sig  = getattr(getattr(self, channel), name)
                pad  = getattr(pads, channel + name)
                if mode == "master":
                    r.append(pad.eq(sig))
                else:
                    r.append(sig.eq(pad))
            for name, width in [("ready", 1)]:
                sig  = getattr(getattr(self, channel), name)
                pad  = getattr(pads, channel + name)
                if mode == "master":
                    r.append(sig.eq(pad))
                else:
                    r.append(pad.eq(sig))
        return r

    def connect(self, slave):
        return _connect_axi(self, slave)

    def layout_flat(self):
        return list(_axi_layout_flat(self))

    def write(self, addr, data, strb=None):
        if strb is None:
            strb = 2**len(self.w.strb) - 1
        yield self.aw.valid.eq(1)
        yield self.aw.addr.eq(addr)
        yield self.w.data.eq(data)
        yield self.w.valid.eq(1)
        yield self.w.strb.eq(strb)
        yield
        while not (yield self.aw.ready):
            yield
        yield self.aw.valid.eq(0)
        while not (yield self.w.ready):
            yield
        yield self.w.valid.eq(0)
        yield self.b.ready.eq(1)
        while not (yield self.b.valid):
            yield
        resp = (yield self.b.resp)
        yield self.b.ready.eq(0)
        return resp

    def read(self, addr):
        yield self.ar.valid.eq(1)
        yield self.ar.addr.eq(addr)
        yield
        while not (yield self.ar.ready):
            yield
        yield self.ar.valid.eq(0)
        yield self.r.ready.eq(1)
        while not (yield self.r.valid):
            yield
        data = (yield self.r.data)
        resp = (yield self.r.resp)
        yield self.r.ready.eq(0)
        return (data, resp)

# AXI Stream Definition ----------------------------------------------------------------------------

class AXIStreamInterface(stream.Endpoint):
    def __init__(self, data_width=32, user_width=0):
        self.data_width = data_width
        self.user_width = user_width
        axi_layout = [("data", data_width)]
        if self.user_width:
            axi_layout += [("user", user_width)]
        stream.Endpoint.__init__(self, axi_layout)

    def get_ios(self, bus_name="axi"):
        subsignals = [
            Subsignal("tvalid", Pins(1)),
            Subsignal("tlast",  Pins(1)),
            Subsignal("tready", Pins(1)),
            Subsignal("tdata",  Pins(self.data_width)),
        ]
        if self.user_width:
            subsignals += [Subsignal("tuser", Pins(self.user_width))]
        ios = [(bus_name , 0) + tuple(subsignals)]
        return ios

    def connect_to_pads(self, pads, mode="master"):
        assert mode in ["slave", "master"]
        r = []
        if mode == "master":
            r.append(pads.tvalid.eq(self.valid))
            r.append(self.ready.eq(pads.tready))
            r.append(pads.tlast.eq(self.last))
            r.append(pads.tdata.eq(self.data))
            if self.user_width:
                r.append(pads.tuser.eq(self.user))
        if mode == "slave":
            r.append(self.valid.eq(pads.tvalid))
            r.append(pads.tready.eq(self.ready))
            r.append(self.last.eq(pads.tlast))
            r.append(self.data.eq(pads.tdata))
            if self.user_width:
                r.append(self.user.eq(pads.tuser))
        return r

# AXI Bursts to Beats ------------------------------------------------------------------------------

class AXIBurst2Beat(Module):
    def __init__(self, ax_burst, ax_beat, capabilities={BURST_FIXED, BURST_INCR, BURST_WRAP}):
        assert BURST_FIXED in capabilities

        # # #

        beat_count  = Signal(8)
        beat_size   = Signal(8 + 4)
        beat_offset = Signal(8 + 4)
        beat_wrap   = Signal(8 + 4)

        # compute parameters
        self.comb += beat_size.eq(1 << ax_burst.size)
        self.comb += beat_wrap.eq(ax_burst.len << ax_burst.size)

        # combinatorial logic
        self.comb += [
            ax_beat.valid.eq(ax_burst.valid | ~ax_beat.first),
            ax_beat.first.eq(beat_count == 0),
            ax_beat.last.eq(beat_count == ax_burst.len),
            ax_beat.addr.eq(ax_burst.addr + beat_offset),
            ax_beat.id.eq(ax_burst.id),
            If(ax_beat.ready,
                If(ax_beat.last,
                    ax_burst.ready.eq(1)
                )
            )
        ]

        # synchronous logic
        self.sync += [
            If(ax_beat.valid & ax_beat.ready,
                If(ax_beat.last,
                    beat_count.eq(0),
                    beat_offset.eq(0)
                ).Else(
                    beat_count.eq(beat_count + 1),
                    If(((ax_burst.burst == BURST_INCR) & (BURST_INCR in capabilities)) |
                       ((ax_burst.burst == BURST_WRAP) & (BURST_WRAP in capabilities)),
                        beat_offset.eq(beat_offset + beat_size)
                    )
                ),
                If((ax_burst.burst == BURST_WRAP) & (BURST_WRAP in capabilities),
                    If(beat_offset == beat_wrap,
                        beat_offset.eq(0)
                    )
                )
            )
        ]


# AXI to AXI Lite ----------------------------------------------------------------------------------

class AXI2AXILite(Module):
    # Note: Since this AXI bridge will mostly be used to target buses that are not supporting
    # simultaneous writes/reads, to reduce ressource usage the AXIBurst2Beat module is shared
    # between writes/reads.
    def __init__(self, axi, axi_lite):
        assert axi.data_width    == axi_lite.data_width
        assert axi.address_width == axi_lite.address_width

        ax_buffer = stream.Buffer(ax_description(axi.address_width, axi.id_width))
        ax_burst  = stream.Endpoint(ax_description(axi.address_width, axi.id_width))
        ax_beat   = stream.Endpoint(ax_description(axi.address_width, axi.id_width))
        self.comb += ax_burst.connect(ax_buffer.sink)
        ax_burst2beat = AXIBurst2Beat(ax_buffer.source, ax_beat)
        self.submodules += ax_buffer, ax_burst2beat

        _data         = Signal(axi.data_width)
        _cmd_done     = Signal()
        _last_ar_aw_n = Signal()

        self.submodules.fsm = fsm = FSM(reset_state="IDLE")
        fsm.act("IDLE",
            NextValue(_cmd_done, 0),
            If(axi.ar.valid & axi.aw.valid,
                # If last access was a read, do a write
                If(_last_ar_aw_n,
                    axi.aw.connect(ax_burst),
                    NextValue(_last_ar_aw_n, 0),
                    NextState("WRITE")
                # If last access was a write, do a read
                ).Else(
                    axi.ar.connect(ax_burst),
                    NextValue(_last_ar_aw_n, 1),
                    NextState("READ"),
                )
            ).Elif(axi.ar.valid,
                axi.ar.connect(ax_burst),
                NextValue(_last_ar_aw_n, 1),
                NextState("READ"),
            ).Elif(axi.aw.valid,
                axi.aw.connect(ax_burst),
                NextValue(_last_ar_aw_n, 0),
                NextState("WRITE")
            )
        )
        fsm.act("READ",
            # cmd
            axi_lite.ar.valid.eq(ax_beat.valid & ~_cmd_done),
            axi_lite.ar.addr.eq(ax_beat.addr),
            ax_beat.ready.eq(axi_lite.ar.ready & ~_cmd_done),
            If(ax_beat.valid & ax_beat.last,
                If(axi_lite.ar.ready,
                    ax_beat.ready.eq(0),
                    NextValue(_cmd_done, 1)
                )
            ),
            # data
            axi.r.valid.eq(axi_lite.r.valid),
            axi.r.last.eq(_cmd_done),
            axi.r.resp.eq(RESP_OKAY),
            axi.r.id.eq(ax_beat.id),
            axi.r.data.eq(axi_lite.r.data),
            axi_lite.r.ready.eq(axi.r.ready),
            # exit
            If(axi.r.valid & axi.r.last & axi.r.ready,
                ax_beat.ready.eq(1),
                NextState("IDLE")
            )
        )
        # always accept write responses
        self.comb += axi_lite.b.ready.eq(1)
        fsm.act("WRITE",
            # cmd
            axi_lite.aw.valid.eq(ax_beat.valid & ~_cmd_done),
            axi_lite.aw.addr.eq(ax_beat.addr),
            ax_beat.ready.eq(axi_lite.aw.ready & ~_cmd_done),
            If(ax_beat.valid & ax_beat.last,
                If(axi_lite.aw.ready,
                    ax_beat.ready.eq(0),
                    NextValue(_cmd_done, 1)
                )
            ),
            # data
            axi_lite.w.valid.eq(axi.w.valid),
            axi_lite.w.data.eq(axi.w.data),
            axi_lite.w.strb.eq(axi.w.strb),
            axi.w.ready.eq(axi_lite.w.ready),
            # exit
            If(axi.w.valid & axi.w.last & axi.w.ready,
                NextState("WRITE-RESP")
            )
        )
        fsm.act("WRITE-RESP",
            axi.b.valid.eq(1),
            axi.b.resp.eq(RESP_OKAY),
            axi.b.id.eq(ax_beat.id),
            If(axi.b.ready,
                ax_beat.ready.eq(1),
                NextState("IDLE")
            )
        )

# AXI Lite to Wishbone -----------------------------------------------------------------------------

class AXILite2Wishbone(Module):
    def __init__(self, axi_lite, wishbone, base_address=0x00000000):
        wishbone_adr_shift = log2_int(axi_lite.data_width//8)
        assert axi_lite.data_width    == len(wishbone.dat_r)
        assert axi_lite.address_width == len(wishbone.adr) + wishbone_adr_shift

        _data         = Signal(axi_lite.data_width)
        _r_addr       = Signal(axi_lite.address_width)
        _w_addr       = Signal(axi_lite.address_width)
        _last_ar_aw_n = Signal()
        self.comb += _r_addr.eq(axi_lite.ar.addr - base_address)
        self.comb += _w_addr.eq(axi_lite.aw.addr - base_address)

        self.submodules.fsm = fsm = FSM(reset_state="IDLE")
        fsm.act("IDLE",
            If(axi_lite.ar.valid & axi_lite.aw.valid,
                # If last access was a read, do a write
                If(_last_ar_aw_n,
                    NextValue(_last_ar_aw_n, 0),
                    NextState("DO-WRITE")
                # If last access was a write, do a read
                ).Else(
                    NextValue(_last_ar_aw_n, 1),
                    NextState("DO-READ")
                )
            ).Elif(axi_lite.ar.valid,
                NextValue(_last_ar_aw_n, 1),
                NextState("DO-READ")
            ).Elif(axi_lite.aw.valid,
                NextValue(_last_ar_aw_n, 0),
                NextState("DO-WRITE")
            )
        )
        fsm.act("DO-READ",
            wishbone.stb.eq(1),
            wishbone.cyc.eq(1),
            wishbone.adr.eq(_r_addr[wishbone_adr_shift:]),
            wishbone.sel.eq(2**len(wishbone.sel) - 1),
            If(wishbone.ack,
                axi_lite.ar.ready.eq(1),
                NextValue(_data, wishbone.dat_r),
                NextState("SEND-READ-RESPONSE")
            )
        )
        fsm.act("SEND-READ-RESPONSE",
            axi_lite.r.valid.eq(1),
            axi_lite.r.resp.eq(RESP_OKAY),
            axi_lite.r.data.eq(_data),
            If(axi_lite.r.ready,
                NextState("IDLE")
            )
        )
        fsm.act("DO-WRITE",
            wishbone.stb.eq(axi_lite.w.valid),
            wishbone.cyc.eq(axi_lite.w.valid),
            wishbone.we.eq(1),
            wishbone.adr.eq(_w_addr[wishbone_adr_shift:]),
            wishbone.sel.eq(axi_lite.w.strb),
            wishbone.dat_w.eq(axi_lite.w.data),
            If(wishbone.ack,
                axi_lite.aw.ready.eq(1),
                axi_lite.w.ready.eq(1),
                NextState("SEND-WRITE-RESPONSE")
            )
        )
        fsm.act("SEND-WRITE-RESPONSE",
            axi_lite.b.valid.eq(1),
            axi_lite.b.resp.eq(RESP_OKAY),
            If(axi_lite.b.ready,
                NextState("IDLE")
            )
        )

# AXI to Wishbone ----------------------------------------------------------------------------------

class AXI2Wishbone(Module):
    def __init__(self, axi, wishbone, base_address=0x00000000):
        axi_lite          = AXILiteInterface(axi.data_width, axi.address_width)
        axi2axi_lite      = AXI2AXILite(axi, axi_lite)
        axi_lite2wishbone = AXILite2Wishbone(axi_lite, wishbone, base_address)
        self.submodules += axi2axi_lite, axi_lite2wishbone

# Wishbone to AXILite ------------------------------------------------------------------------------

class Wishbone2AXILite(Module):
    def __init__(self, wishbone, axi_lite, base_address=0x00000000):
        wishbone_adr_shift = log2_int(axi_lite.data_width//8)
        assert axi_lite.data_width    == len(wishbone.dat_r)
        assert axi_lite.address_width == len(wishbone.adr) + wishbone_adr_shift

        _cmd_done  = Signal()
        _data_done = Signal()
        _addr      = Signal(len(wishbone.adr))
        self.comb += _addr.eq(wishbone.adr - base_address//4)

        self.submodules.fsm = fsm = FSM(reset_state="IDLE")
        fsm.act("IDLE",
            NextValue(_cmd_done,  0),
            NextValue(_data_done, 0),
            If(wishbone.stb & wishbone.cyc,
                If(wishbone.we,
                    NextState("WRITE")
                ).Else(
                    NextState("READ")
                )
            )
        )
        fsm.act("WRITE",
            # cmd
            axi_lite.aw.valid.eq(~_cmd_done),
            axi_lite.aw.addr[wishbone_adr_shift:].eq(_addr),
            If(axi_lite.aw.valid & axi_lite.aw.ready,
                NextValue(_cmd_done, 1)
            ),
            # data
            axi_lite.w.valid.eq(~_data_done),
            axi_lite.w.data.eq(wishbone.dat_w),
            axi_lite.w.strb.eq(wishbone.sel),
            If(axi_lite.w.valid & axi_lite.w.ready,
                NextValue(_data_done, 1),
            ),
            # resp
            axi_lite.b.ready.eq(_cmd_done & _data_done),
            If(axi_lite.b.valid & axi_lite.b.ready,
                If(axi_lite.b.resp == RESP_OKAY,
                    wishbone.ack.eq(1),
                    NextState("IDLE")
                ).Else(
                    NextState("ERROR")
                )
            )
        )
        fsm.act("READ",
            # cmd
            axi_lite.ar.valid.eq(~_cmd_done),
            axi_lite.ar.addr[wishbone_adr_shift:].eq(_addr),
            If(axi_lite.ar.valid & axi_lite.ar.ready,
                NextValue(_cmd_done, 1)
            ),
            # data & resp
            axi_lite.r.ready.eq(_cmd_done),
            If(axi_lite.r.valid & axi_lite.r.ready,
                If(axi_lite.r.resp == RESP_OKAY,
                    wishbone.dat_r.eq(axi_lite.r.data),
                    wishbone.ack.eq(1),
                    NextState("IDLE"),
                ).Else(
                    NextState("ERROR")
                )
            )
        )
        fsm.act("ERROR",
            wishbone.ack.eq(1),
            wishbone.err.eq(1),
            NextState("IDLE")
        )

# AXILite to CSR -----------------------------------------------------------------------------------

def axi_lite_to_simple(axi_lite, port_adr, port_dat_r, port_dat_w=None, port_we=None):
    """Connection of AXILite to simple bus with 1-cycle latency, such as CSR bus or Memory port"""
    bus_data_width = axi_lite.data_width
    adr_shift = log2_int(bus_data_width//8)
    do_read = Signal()
    do_write = Signal()
    last_was_read = Signal()

    comb = []
    if port_dat_w is not None:
        comb.append(port_dat_w.eq(axi_lite.w.data))
    if port_we is not None:
        if len(port_we) > 1:
            for i in range(bus_data_width//8):
                comb.append(port_we[i].eq(axi_lite.w.valid & axi_lite.w.ready & axi_lite.w.strb[i]))
        else:
            comb.append(port_we.eq(axi_lite.w.valid & axi_lite.w.ready & (axi_lite.w.strb != 0)))

    fsm = FSM()
    fsm.act("START-TRANSACTION",
        # If the last access was a read, do a write, and vice versa
        If(axi_lite.aw.valid & axi_lite.ar.valid,
            do_write.eq(last_was_read),
            do_read.eq(~last_was_read),
        ).Else(
            do_write.eq(axi_lite.aw.valid),
            do_read.eq(axi_lite.ar.valid),
        ),
        # Start reading/writing immediately not to waste a cycle
        If(do_write,
            port_adr.eq(axi_lite.aw.addr[adr_shift:]),
            If(axi_lite.w.valid,
                axi_lite.aw.ready.eq(1),
                axi_lite.w.ready.eq(1),
                NextState("SEND-WRITE-RESPONSE")
            )
        ).Elif(do_read,
            port_adr.eq(axi_lite.ar.addr[adr_shift:]),
            axi_lite.ar.ready.eq(1),
            NextState("SEND-READ-RESPONSE"),
        )
    )
    fsm.act("SEND-READ-RESPONSE",
        NextValue(last_was_read, 1),
        # As long as we have correct address port.dat_r will be valid
        port_adr.eq(axi_lite.ar.addr[adr_shift:]),
        axi_lite.r.data.eq(port_dat_r),
        axi_lite.r.resp.eq(RESP_OKAY),
        axi_lite.r.valid.eq(1),
        If(axi_lite.r.ready,
            NextState("START-TRANSACTION")
        )
    )
    fsm.act("SEND-WRITE-RESPONSE",
        NextValue(last_was_read, 0),
        axi_lite.b.valid.eq(1),
        axi_lite.b.resp.eq(RESP_OKAY),
        If(axi_lite.b.ready,
            NextState("START-TRANSACTION")
        )
    )
    return fsm, comb

class AXILite2CSR(Module):
    def __init__(self, axi_lite=None, bus_csr=None):
        if axi_lite is None:
            axi_lite = AXILiteInterface()
        if bus_csr is None:
            bus_csr = csr_bus.Interface()

        self.axi_lite = axi_lite
        self.csr = bus_csr

        fsm, comb = axi_lite_to_simple(self.axi_lite,
                                       port_adr=self.csr.adr, port_dat_r=self.csr.dat_r,
                                       port_dat_w=self.csr.dat_w, port_we=self.csr.we)
        self.submodules.fsm = fsm
        self.comb += comb

# AXILite SRAM -------------------------------------------------------------------------------------

class AXILiteSRAM(Module):
    def __init__(self, mem_or_size, read_only=None, init=None, bus=None):
        if bus is None:
            bus = AXILiteInterface()
        self.bus = bus

        bus_data_width = len(self.bus.r.data)
        if isinstance(mem_or_size, Memory):
            assert(mem_or_size.width <= bus_data_width)
            self.mem = mem_or_size
        else:
            self.mem = Memory(bus_data_width, mem_or_size//(bus_data_width//8), init=init)

        if read_only is None:
            if hasattr(self.mem, "bus_read_only"):
                read_only = self.mem.bus_read_only
            else:
                read_only = False

        ###

        # Create memory port
        port = self.mem.get_port(write_capable=not read_only, we_granularity=8,
            mode=READ_FIRST if read_only else WRITE_FIRST)
        self.specials += self.mem, port

        # Generate write enable signal
        if not read_only:
            self.comb += port.dat_w.eq(self.bus.w.data),
            self.comb += [port.we[i].eq(self.bus.w.valid & self.bus.w.ready & self.bus.w.strb[i])
                for i in range(bus_data_width//8)]

        # Transaction logic
        fsm, comb = axi_lite_to_simple(self.bus,
                                       port_adr=port.adr, port_dat_r=port.dat_r,
                                       port_dat_w=port.dat_w if not read_only else None,
                                       port_we=port.we if not read_only else None)
        self.submodules.fsm = fsm
        self.comb += comb

# AXILite Data Width Converter ---------------------------------------------------------------------

class AXILiteDownConverter(Module):
    def __init__(self, master, slave):
        assert isinstance(master, AXILiteInterface) and isinstance(slave, AXILiteInterface)
        dw_from = len(master.r.data)
        dw_to   = len(slave.r.data)
        ratio   = dw_from//dw_to

        # # #

        skip          = Signal()
        counter       = Signal(max=ratio)
        do_read       = Signal()
        do_write      = Signal()
        last_was_read = Signal()
        aw_ready      = Signal()
        w_ready       = Signal()
        resp          = Signal.like(master.b.resp)

        # Slave address counter
        master_align = log2_int(master.data_width//8)
        slave_align = log2_int(slave.data_width//8)
        addr_counter = Signal(master_align)
        self.comb += addr_counter[slave_align:].eq(counter)

        # Write path
        self.comb += [
            slave.aw.addr.eq(Cat(addr_counter, master.aw.addr[master_align:])),
            Case(counter, {i: slave.w.data.eq(master.w.data[i*dw_to:]) for i in range(ratio)}),
            Case(counter, {i: slave.w.strb.eq(master.w.strb[i*dw_to//8:]) for i in range(ratio)}),
            master.b.resp.eq(resp),
        ]

        # Read path
        # shift the data word
        r_data = Signal(dw_from, reset_less=True)
        self.sync += If(slave.r.ready, r_data.eq(master.r.data))
        self.comb += master.r.data.eq(Cat(r_data[dw_to:], slave.r.data))
        # address, resp
        self.comb += [
            slave.ar.addr.eq(Cat(addr_counter, master.ar.addr[master_align:])),
            master.r.resp.eq(resp),
        ]

        # Control Path
        fsm = FSM(reset_state="IDLE")
        fsm = ResetInserter()(fsm)
        self.submodules.fsm = fsm
        self.comb += fsm.reset.eq(~(master.aw.valid | master.ar.valid))

        fsm.act("IDLE",
            NextValue(counter, 0),
            NextValue(resp, RESP_OKAY),
            # If the last access was a read, do a write, and vice versa
            If(master.aw.valid & master.ar.valid,
                do_write.eq(last_was_read),
                do_read.eq(~last_was_read),
            ).Else(
                do_write.eq(master.aw.valid),
                do_read.eq(master.ar.valid),
            ),
            # Start reading/writing immediately not to waste a cycle
            If(do_write & master.w.valid,
                NextValue(last_was_read, 0),
                NextState("WRITE")
            ).Elif(do_read,
                NextValue(last_was_read, 1),
                NextState("READ")
            )
        )

        # Write conversion
        fsm.act("WRITE",
            skip.eq(slave.w.strb == 0),
            slave.aw.valid.eq(~skip & ~aw_ready),
            slave.w.valid.eq(~skip & ~w_ready),
            If(slave.aw.ready,
                NextValue(aw_ready, 1)
            ),
            If(slave.w.ready,
                NextValue(w_ready, 1)
            ),
            # When skipping, we just increment the counter
            If(skip,
                NextValue(counter, counter + 1),
                # Corner-case: when the last word is being skipped, we must send the response
                If(counter == (ratio - 1),
                    master.aw.ready.eq(1),
                    master.w.ready.eq(1),
                    NextState("WRITE-RESPONSE-MASTER")
                )
            # Write current word and wait for write response
            ).Elif((slave.aw.ready | aw_ready) & (slave.w.ready | w_ready),
                NextState("WRITE-RESPONSE-SLAVE")
            )
        )
        fsm.act("WRITE-RESPONSE-SLAVE",
            NextValue(aw_ready, 0),
            NextValue(w_ready, 0),
            If(slave.b.valid,
                slave.b.ready.eq(1),
                # Any errors is sticky, so the first one is always sent
                If((resp == RESP_OKAY) & (slave.b.resp != RESP_OKAY),
                    NextValue(resp, slave.b.resp)
                ),
                If(counter == (ratio - 1),
                    master.aw.ready.eq(1),
                    master.w.ready.eq(1),
                    NextState("WRITE-RESPONSE-MASTER")
                ).Else(
                    NextValue(counter, counter + 1),
                    NextState("WRITE")
                )
            )
        )
        fsm.act("WRITE-RESPONSE-MASTER",
            NextValue(aw_ready, 0),
            NextValue(w_ready, 0),
            master.b.valid.eq(1),
            If(master.b.ready,
                NextState("IDLE")
            )
        )

        # Read conversion
        fsm.act("READ",
            slave.ar.valid.eq(1),
            If(slave.ar.ready,
                NextState("READ-RESPONSE-SLAVE")
            )
        )
        fsm.act("READ-RESPONSE-SLAVE",
            If(slave.r.valid,
                # Any errors is sticky, so the first one is always sent
                If((resp == RESP_OKAY) & (slave.b.resp != RESP_OKAY),
                    NextValue(resp, slave.b.resp)
                ),
                # On last word acknowledge ar and hold slave.r.valid until we get master.r.ready
                If(counter == (ratio - 1),
                    master.ar.ready.eq(1),
                    NextState("READ-RESPONSE-MASTER")
                # Acknowledge the response and continue conversion
                ).Else(
                    slave.r.ready.eq(1),
                    NextValue(counter, counter + 1),
                    NextState("READ")
                )
            )
        )
        fsm.act("READ-RESPONSE-MASTER",
            master.r.valid.eq(1),
            If(master.r.ready,
                slave.r.ready.eq(1),
                NextState("IDLE")
            )
        )

class AXILiteConverter(Module):
    """AXILite data width converter"""
    def __init__(self, master, slave):
        self.master = master
        self.slave = slave

        # # #

        dw_from = len(master.r.data)
        dw_to = len(slave.r.data)
        if dw_from > dw_to:
            self.submodules += AXILiteDownConverter(master, slave)
        elif dw_from < dw_to:
            raise NotImplementedError("AXILiteUpConverter")
        else:
            self.comb += master.connect(slave)

# AXILite Timeout ----------------------------------------------------------------------------------

class AXILiteTimeout(Module):
    """Protect master against slave timeouts (master _has_ to respond correctly)"""
    def __init__(self, master, cycles):
        self.error = Signal()

        # # #

        timer = WaitTimer(int(cycles))
        self.submodules += timer
        is_write = Signal()
        is_read  = Signal()

        self.submodules.fsm = fsm = FSM()
        fsm.act("WAIT",
            is_write.eq((master.aw.valid & ~master.aw.ready) | (master.w.valid & ~master.w.ready)),
            is_read.eq(master.ar.valid & ~master.ar.ready),
            timer.wait.eq(is_write | is_read),
            # done is updated in `sync`, so we must make sure that `ready` has not been issued
            # by slave during that single cycle, by checking `timer.wait`
            If(timer.done & timer.wait,
                self.error.eq(1),
                If(is_write,
                    NextState("RESPOND-WRITE")
                ).Else(
                    NextState("RESPOND-READ")
                )
            )
        )
        fsm.act("RESPOND-WRITE",
            master.aw.ready.eq(master.aw.valid),
            master.w.ready.eq(master.w.valid),
            master.b.valid.eq(~master.aw.valid & ~master.w.valid),
            master.b.resp.eq(RESP_SLVERR),
            If(master.b.valid & master.b.ready,
                NextState("WAIT")
            )
        )
        fsm.act("RESPOND-READ",
            master.ar.ready.eq(master.ar.valid),
            master.r.valid.eq(~master.ar.valid),
            master.r.resp.eq(RESP_SLVERR),
            master.r.data.eq(2**len(master.r.data) - 1),
            If(master.r.valid & master.r.ready,
                NextState("WAIT")
            )
        )

# AXILite Interconnect -----------------------------------------------------------------------------

class AXILiteInterconnectPointToPoint(Module):
    def __init__(self, master, slave):
        self.comb += master.connect(slave)

