#
# This file is part of LiteX.
#
# Copyright (c) 2019 Florent Kermarrec <florent@enjoy-digital.fr>
# Copyright (c) 2024 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: BSD-2-Clause

import unittest

from migen import Record, run_simulation

from litex.gen.sim import passive
from litex.soc.cores.bitbang import I2CMaster, SPIMaster


@passive
def loopback(pads):
    while True:
        if (yield pads.scl.oe):
            yield pads.scl.i.eq(pads.scl.o)
        else:
            yield pads.scl.i.eq(1)
        if (yield pads.sda.oe):
            yield pads.sda.i.eq(pads.sda.o)
        else:
            yield pads.sda.i.eq(1)
        yield


class TestBitBangI2C(unittest.TestCase):

    def _master_output_bitbang(self, val):
        return (0, (val ^ 1) & 1, 0, ((val >> 1) & ~(val >> 2)) & 1)

    def test_i2c_master_syntax(self):
        i2c_master = I2CMaster()
        self.assertEqual(hasattr(i2c_master, "pads"), 1)
        i2c_master = I2CMaster(Record(I2CMaster.pads_layout))

    def test_i2c_master(self):
        def generator(i2c):
            yield
            yield i2c.pads.sda.i.eq(1)
            yield
            self.assertEqual((yield i2c._r.fields.sda), 1)
            yield i2c.pads.sda.i.eq(0)
            yield
            self.assertEqual((yield i2c._r.fields.sda), 0)
            for i in range(8):
                yield from i2c._w.write(i)
                scl_o, scl_oe, sda_o, sda_oe = self._master_output_bitbang(i)
                self.assertEqual((yield i2c.pads.scl.o), scl_o)
                self.assertEqual((yield i2c.pads.scl.oe), scl_oe)
                self.assertEqual((yield i2c.pads.sda.o), sda_o)
                self.assertEqual((yield i2c.pads.sda.oe), sda_oe)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(i2c_master, [generator(i2c_master)])
        self.assertEqual(hasattr(i2c_master, "pads"), 1)


class TestI2C(unittest.TestCase):

    def _master_output_bitbang(self, val):
        return (0, (val ^ 1) & 1, 0, ((val >> 1) & ~(val >> 2)) & 1)

    @passive
    def check_start(self, pads):
        self.detect_start = False
        self.start_seen = False
        data_transition = False
        yield
        old_scl = yield pads.scl.i
        old_sda = yield pads.sda.i
        while not self.detect_start:
            old_scl = yield pads.scl.i
            old_sda = yield pads.sda.i
            yield
        while True:
            if old_sda != (yield pads.sda.i) and old_sda == 1:
                data_transition = True
                self.assertEqual(old_scl, (yield pads.scl.i))
                self.assertEqual(old_scl, 1)
            if data_transition and old_scl != (yield pads.scl.i):
                self.assertEqual(old_sda, (yield pads.sda.i))
                self.assertEqual(old_sda, 0)
                self.start_seen = True
            old_scl = yield pads.scl.i
            old_sda = yield pads.sda.i
            yield

    @passive
    def check_stop(self, pads):
        self.detect_stop = False
        self.stop_seen = False
        clk_transition = False
        yield
        old_scl = yield pads.scl.i
        old_sda = yield pads.sda.i
        while not self.detect_stop:
            old_scl = yield pads.scl.i
            old_sda = yield pads.sda.i
            yield
        while True:
            if old_scl != (yield pads.scl.i) and old_scl == 0:
                clk_transition = True
                self.assertEqual(old_sda, (yield pads.sda.i))
                self.assertEqual(old_sda, 0)
            if clk_transition and old_sda != (yield pads.sda.i):
                self.assertEqual(old_scl, (yield pads.scl.i))
                self.assertEqual(old_scl, 1)
                self.stop_seen = True
            old_scl = yield pads.scl.i
            old_sda = yield pads.sda.i
            yield

    @passive
    def check_data(self, pads, _bytes):
        yield
        old_scl = yield pads.scl.i
        for byte, nack in _bytes:
            for i in range(8):
                while not (old_scl == 0 and (yield pads.scl.i) == 1):
                    old_scl = yield pads.scl.i
                    yield
                old_scl = yield pads.scl.i
                bit = yield pads.sda.i
                self.assertEqual((byte >> (7 - i)) & 1, bit)
                yield
            while not (old_scl == 0 and (yield pads.scl.i) == 1):
                old_scl = yield pads.scl.i
                yield
            old_scl = yield pads.scl.i
            bit = yield pads.sda.i
            self.assertEqual(nack, bit)
            yield

    @passive
    def send_data(self, pads, _bytes):
        yield
        old_scl = yield pads.scl.i
        for byte, nack in _bytes:
            for i in range(8):
                while not (old_scl == 1 and (yield pads.scl.i) == 0):
                    old_scl = yield pads.scl.i
                    yield
                old_scl = yield pads.scl.i
                yield pads.sda.i.eq((byte >> (7 - i)) & 1)
                yield
            while not (old_scl == 1 and (yield pads.scl.i) == 0):
                old_scl = yield pads.scl.i
                yield
            old_scl = yield pads.scl.i
            yield pads.sda.i.eq(nack)
            yield

    @passive
    def check_clk(self, pads):
        yield
        old_sda = yield pads.sda.i
        while True:
            if (yield pads.scl.i) == 1:
                self.assertEqual(old_sda, (yield pads.sda.i))
            old_sda = yield pads.sda.i
            yield

    @passive
    def clk_pullup(self, pads):
        while True:
            if (yield pads.scl.oe):
                yield pads.scl.i.eq(pads.scl.o)
            else:
                yield pads.scl.i.eq(1)
            yield

    def test_i2c_master_worker_dis(self):
        def generator(i2c):
            yield
            yield from i2c._sel.write(0)
            yield i2c.pads.sda.i.eq(1)
            yield
            self.assertEqual((yield i2c._r.fields.sda), 1)
            yield i2c.pads.sda.i.eq(0)
            yield
            self.assertEqual((yield i2c._r.fields.sda), 0)
            for i in range(8):
                yield from i2c._w.write(i)
                scl_o, scl_oe, sda_o, sda_oe = self._master_output_bitbang(i)
                self.assertEqual((yield i2c.pads.scl.o), scl_o)
                self.assertEqual((yield i2c.pads.scl.oe), scl_oe)
                self.assertEqual((yield i2c.pads.sda.o), sda_o)
                self.assertEqual((yield i2c.pads.sda.oe), sda_oe)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(i2c_master, [generator(i2c_master)])

    def test_i2c_master_worker_en(self):
        def generator(i2c):
            yield
            yield from i2c._sel.write(1)
            yield i2c.pads.sda.i.eq(1)
            yield
            self.assertEqual((yield i2c._r.fields.sda), 1)
            yield i2c.pads.sda.i.eq(0)
            yield
            self.assertEqual((yield i2c._r.fields.sda), 0)
            for i in range(8):
                yield from i2c._w.write(i)
                self.assertEqual((yield i2c.pads.scl.o), 0)
                self.assertEqual((yield i2c.pads.scl.oe), 0)
                self.assertEqual((yield i2c.pads.sda.o), 0)
                self.assertEqual((yield i2c.pads.sda.oe), 0)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(i2c_master, generator(i2c_master))

    def test_i2c_master_worker_write_fifo_fill(self):
        def generator(i2c):
            yield
            yield from i2c._sel.write(1)
            fifo_depth = (yield i2c.i2c_worker._fifo_w.fields.fifo_depth)
            self.assertEqual(fifo_depth, 128)
            self.assertEqual((yield i2c.i2c_worker._fifo_w.fields.fifo_entries), 0)
            for i in range(fifo_depth):
                yield from i2c.i2c_worker._fifo.write(0)
                yield
                self.assertEqual((yield i2c.i2c_worker._fifo_w.fields.fifo_depth), 128)
                self.assertEqual((yield i2c.i2c_worker._fifo_w.fields.fifo_entries), i+1)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(i2c_master, generator(i2c_master))

    def test_i2c_master_worker_write_fifo_clr(self):
        def generator(i2c):
            yield
            yield from i2c._sel.write(1)
            fifo_depth = (yield i2c.i2c_worker._fifo_w.fields.fifo_depth)
            for i in range(fifo_depth):
                yield from i2c.i2c_worker._fifo.write(0)
            while ((yield i2c.i2c_worker._fifo_w.fields.fifo_entries) != 128):
                yield
            yield i2c.i2c_worker._ctrl.fields.clr_fifos.eq(1)
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            while not ((yield i2c.i2c_worker._state.fields.ready)):
                yield
            self.assertEqual((yield i2c.i2c_worker._fifo_w.fields.fifo_entries), 0)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(i2c_master, generator(i2c_master))

    def test_i2c_master_worker_state_rst(self):
        def generator(i2c):
            yield
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            yield i2c.i2c_worker._ctrl.fields.reset_fsm.eq(1)
            yield
            yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 0)
            yield i2c.i2c_worker._ctrl.fields.reset_fsm.eq(0)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(i2c_master, generator(i2c_master))

    def test_i2c_master_worker_start(self):
        def generator(i2c):
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._fifo.write(1 << 16)
            yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.detect_start = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 4)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 4):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.assertEqual(self.start_seen, True)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(i2c_master, [generator(i2c_master), loopback(pads), self.check_start(pads)])

    def test_i2c_master_worker_start_stop(self):
        def generator(i2c):
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._fifo.write(1 << 16 | 1 << 18)
            yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.detect_start = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 4)
            self.detect_stop = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 4):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 8)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 8):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.assertEqual(self.start_seen, True)
            self.assertEqual(self.stop_seen, True)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), loopback(pads), self.check_start(pads), self.check_stop(pads)],
        )

    def test_i2c_master_worker_start_then_stop(self):
        def generator(i2c):
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._fifo.write(1 << 16)
            yield from i2c.i2c_worker._fifo.write(1 << 18)
            yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.detect_start = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 4)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 4):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.detect_stop = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 8)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 8):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.assertEqual(self.start_seen, True)
            self.assertEqual(self.stop_seen, True)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), loopback(pads), self.check_start(pads), self.check_stop(pads)],
        )

    def test_i2c_master_worker_start_stop_end(self):
        def generator(i2c):
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._fifo.write(1 << 16 | 1 << 18 | 1 << 19)
            yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.detect_start = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 4)
            self.detect_stop = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 4):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 8)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 8):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 0)
            self.assertEqual(self.start_seen, True)
            self.assertEqual(self.stop_seen, True)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), loopback(pads), self.check_start(pads), self.check_stop(pads)],
        )

    def test_i2c_master_worker_start_stop_then_end(self):
        def generator(i2c):
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._fifo.write(1 << 16 | 1 << 18)
            yield from i2c.i2c_worker._fifo.write(1 << 19)
            yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.detect_start = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 4)
            self.detect_stop = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 4):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 8)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 8):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 0)
            self.assertEqual(self.start_seen, True)
            self.assertEqual(self.stop_seen, True)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), loopback(pads), self.check_start(pads), self.check_stop(pads)],
        )

    def test_i2c_master_worker_start_then_stop_end(self):
        def generator(i2c):
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._fifo.write(1 << 16)
            yield from i2c.i2c_worker._fifo.write(1 << 18 | 1 << 19)
            yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.detect_start = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 4)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 4):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.detect_stop = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 8)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 8):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 0)
            self.assertEqual(self.start_seen, True)
            self.assertEqual(self.stop_seen, True)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), loopback(pads), self.check_start(pads), self.check_stop(pads)],
        )

    def test_i2c_master_worker_start_then_stop_then_end(self):
        def generator(i2c):
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._fifo.write(1 << 16)
            yield from i2c.i2c_worker._fifo.write(1 << 18)
            yield from i2c.i2c_worker._fifo.write(1 << 19)
            yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.detect_start = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 4)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 4):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.detect_stop = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 8)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 8):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 0)
            self.assertEqual(self.start_seen, True)
            self.assertEqual(self.stop_seen, True)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), loopback(pads), self.check_start(pads), self.check_stop(pads)],
        )

    def test_i2c_master_worker_send_data(self):
        def generator(i2c, _bytes):
            yield from i2c._sel.write(1)
            for byte in _bytes:
                yield from i2c.i2c_worker._fifo.write(byte | 1 << 17)
                yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            for _ in range(len(_bytes)):
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                    yield
                for _ in range(9):
                    self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
                    while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                        yield
                    self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 5)
                    while ((yield i2c.i2c_worker._state.fields.fsm_state) == 5):
                        yield
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                    yield
        data = [0x55, 0x55, 0x96, 0x96, 0x42, 0x42, 0xAA, 0xAA, 0xFF, 0xFF, 0x00, 0x00]
        expected_output = []
        for i in range(len(data)):
            expected_output.append((data[i], i & 1))
            data[i] |= (i & 1) << 8  # Allow for response

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master, data), loopback(pads),
             self.check_clk(pads), self.check_data(pads, expected_output)],
        )

    def test_i2c_master_worker_recv_data(self):
        def generator(i2c, _bytes):
            yield from i2c._sel.write(1)
            for _ in range(len(_bytes)):
                yield from i2c.i2c_worker._fifo.write(0x1ff | 1 << 17)
                yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            for _ in range(len(_bytes)):
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                    yield
                for _ in range(9):
                    self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
                    while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                        yield
                    self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 5)
                    while ((yield i2c.i2c_worker._state.fields.fsm_state) == 5):
                        yield
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                    yield
            self.assertEqual((yield i2c.i2c_worker._fifo_r.fields.fifo_entries), len(_bytes))
            i = 0
            while (yield i2c.i2c_worker._fifo_r.fields.fifo_entries) > 0:
                data = yield from i2c.i2c_worker._fifo.read()
                self.assertEqual(data, _bytes[i])
                i += 1
                yield

        data = [0x55, 0x55, 0x96, 0x96, 0x42, 0x42, 0xAA, 0xAA, 0xFF, 0xFF, 0x00, 0x00]
        expected_output = []
        for i in range(len(data)):
            expected_output.append(data[i] | (i & 1) << 8)
            data[i] = (data[i], (i & 1))

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master, expected_output), self.clk_pullup(pads),
             self.check_clk(pads), self.send_data(pads, data)],
        )

    def test_i2c_master_worker_read_fifo_fill(self):
        def generator(i2c, _bytes):
            yield from i2c._sel.write(1)
            self.assertEqual((yield i2c.i2c_worker._fifo_r.fields.fifo_depth), 128)
            self.assertEqual((yield i2c.i2c_worker._fifo_r.fields.fifo_entries), 0)
            for byte in _bytes:
                yield from i2c.i2c_worker._fifo.write(byte | 1 << 17)
                yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            for _ in range(len(_bytes)):
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                    yield
                for _ in range(9):
                    self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
                    while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                        yield
                    self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 5)
                    while ((yield i2c.i2c_worker._state.fields.fsm_state) == 5):
                        yield
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                    yield
            self.assertEqual((yield i2c.i2c_worker._fifo_r.fields.fifo_entries), len(_bytes))
            i = 0
            while (yield i2c.i2c_worker._fifo_r.fields.fifo_entries) > 0:
                data = yield from i2c.i2c_worker._fifo.read()
                self.assertEqual(data, _bytes[i])
                i += 1
                yield

        data = []
        for _ in range(32):
            for val in [0x55, 0xAA, 0xFF, 0x00]:
                data.append(val)
        expected_output = []
        for i in range(len(data)):
            expected_output.append((data[i], i & 1))
            data[i] |= (i & 1) << 8  # Allow for response

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master, data), loopback(pads),
             self.check_clk(pads), self.check_data(pads, expected_output)],
        )

    def test_i2c_master_worker_read_fifo_over_fill(self):
        @passive
        def fill(i2c, _bytes):
            i = 0
            while i < len(_bytes):
                if (yield i2c.i2c_worker._fifo_w.fields.fifo_entries) < 128:
                    yield from i2c.i2c_worker._fifo.write(_bytes[i] | 1 << 17)
                    i += 1
                yield

        def generator(i2c):
            yield from i2c._sel.write(1)
            while (yield i2c.i2c_worker._fifo_w.fields.fifo_entries) < 8:
                yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            for _ in range(128):
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                    yield
                for _ in range(9):
                    while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                        yield
                    while ((yield i2c.i2c_worker._state.fields.fsm_state) == 5):
                        yield
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                    yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.assertEqual((yield i2c.i2c_worker._fifo_r.fields.fifo_entries), 128)
            self.assertEqual((yield i2c.i2c_worker._fifo_w.fields.fifo_entries), 4)

        data = []
        for _ in range(33):
            for val in [0x55, 0xAA, 0xFF, 0x00]:
                data.append(val)
        for i in range(len(data)):
            data[i] |= (i & 1) << 8  # Allow for response

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), fill(i2c_master, data), loopback(pads)],
        )

    def test_i2c_master_worker_read_fifo_clr(self):
        @passive
        def fill(i2c, _bytes):
            for byte in _bytes:
                if (yield i2c.i2c_worker._fifo_w.fields.fifo_entries) < 128:
                    yield from i2c.i2c_worker._fifo.write(byte | 1 << 17)
                yield

        def generator(i2c):
            yield from i2c._sel.write(1)
            while (yield i2c.i2c_worker._fifo_w.fields.fifo_entries) < 8:
                yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            for _ in range(128):
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                    yield
                for _ in range(9):
                    while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                        yield
                    while ((yield i2c.i2c_worker._state.fields.fsm_state) == 5):
                        yield
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                    yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.assertEqual((yield i2c.i2c_worker._fifo_r.fields.fifo_entries), 128)
            self.assertEqual((yield i2c.i2c_worker._fifo_w.fields.fifo_entries), 0)
            yield i2c.i2c_worker._ctrl.fields.reset_fsm.eq(1)
            yield
            yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 0)
            yield i2c.i2c_worker._ctrl.fields.reset_fsm.eq(0)
            yield i2c.i2c_worker._ctrl.fields.clr_fifos.eq(1)
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            while not ((yield i2c.i2c_worker._state.fields.ready)):
                yield
            self.assertEqual((yield i2c.i2c_worker._fifo_r.fields.fifo_entries), 0)

        data = []
        for _ in range(32):
            for val in [0x55, 0xAA, 0xFF, 0x00]:
                data.append(val)
        expected_output = []
        for i in range(len(data)):
            expected_output.append(data[i] << 1 | (i & 1))
            data[i] |= (i & 1) << 8  # Allow for response

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), fill(i2c_master, data), loopback(pads)],
        )

    def test_i2c_master_worker_data_stop(self):
        def generator(i2c):
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._fifo.write(1 << 17 | 1 << 18)
            yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            for _ in range(9):
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                    yield
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 5)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 5):
                    yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
            self.detect_stop = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 8)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 8):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.assertEqual(self.stop_seen, True)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), loopback(pads), self.check_stop(pads)],
        )

    def test_i2c_master_worker_data_then_stop(self):
        def generator(i2c):
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._fifo.write(1 << 17)
            yield from i2c.i2c_worker._fifo.write(1 << 18)
            yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            for _ in range(9):
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                    yield
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 5)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 5):
                    yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
            self.detect_stop = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 8)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 8):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.assertEqual(self.stop_seen, True)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), loopback(pads), self.check_stop(pads)],
        )

    def test_i2c_master_worker_data_nack_then_stop(self):
        def generator(i2c):
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._fifo.write(1 << 8 | 1 << 17)
            yield from i2c.i2c_worker._fifo.write(1 << 18)
            yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            for _ in range(9):
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                    yield
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 5)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 5):
                    yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
            self.detect_stop = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 7)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 7):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 8)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 8):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.assertEqual(self.stop_seen, True)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), loopback(pads), self.check_stop(pads)],
        )

    def test_i2c_master_worker_data_then_start(self):
        def generator(i2c):
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._fifo.write(1 << 17)
            yield from i2c.i2c_worker._fifo.write(1 << 16)
            yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            for _ in range(9):
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                    yield
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 5)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 5):
                    yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
            self.detect_start = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 2)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 2):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 3)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 3):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 4)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 4):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.assertEqual(self.start_seen, True)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), loopback(pads), self.check_start(pads)],
        )

    def test_i2c_master_worker_data_nack_then_start(self):
        def generator(i2c):
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._fifo.write(1 << 8 | 1 << 17)
            yield from i2c.i2c_worker._fifo.write(1 << 16)
            yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            for _ in range(9):
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                    yield
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 5)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 5):
                    yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
            self.detect_start = True
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 3)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 3):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 4)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 4):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            self.assertEqual(self.start_seen, True)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), loopback(pads), self.check_start(pads)],
        )

    def test_i2c_master_worker_data_nack_abort(self):
        def generator(i2c):
            yield from i2c._sel.write(1)
            yield from i2c.i2c_worker._fifo.write(1 << 8 | 1 << 17 | 1 << 20)
            yield
            yield from i2c.i2c_worker._start.write(1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 0):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 1)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 1):
                yield
            for _ in range(9):
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                    yield
                self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 5)
                while ((yield i2c.i2c_worker._state.fields.fsm_state) == 5):
                    yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 6)
            while ((yield i2c.i2c_worker._state.fields.fsm_state) == 6):
                yield
            self.assertEqual((yield i2c.i2c_worker._state.fields.fsm_state), 9)

        pads = Record([("scl", [("o", 1), ("oe", 1), ("i", 1)]),
                       ("sda", [("o", 1), ("oe", 1), ("i", 1)])])
        i2c_master = I2CMaster(pads=pads, sys_freq=100e6, bus_freq=400e3)
        run_simulation(
            i2c_master,
            [generator(i2c_master), loopback(pads)],
        )


class TestBitBangSPI(unittest.TestCase):
    def test_spi_master_syntax(self):
        spi_master = SPIMaster()
        self.assertEqual(hasattr(spi_master, "pads"), 1)
        spi_master = SPIMaster(Record(SPIMaster.pads_layout))
        self.assertEqual(hasattr(spi_master, "pads"), 1)
