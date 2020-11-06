# Copyright (c) 2020 Antmicro LTD

from migen import *

from litex.soc.interconnect.csr import *


class PRIOInterfacer(Module, AutoCSR):
    def __init__(self, i_pads, o_pads, inputs, outputs):
        self.ina = []
        self.csr_i = []
        self.oua = []
        self.csr_o = []

        for i in range(inputs):
            self.ina.append(Signal())
            self.ina[-1].attr.add("keep")
            self.csr_i.append(CSRStorage(name="csr_storage_" + str(i)))
            self.sync += self.ina[-1].eq(self.csr_i[-1].storage)

        for o in range(outputs):
            self.oua.append(Signal())
            self.oua[-1].attr.add("keep")
            self.csr_o.append(CSRStatus(name="csr_status_" + str(i)))
            self.sync += self.csr_o[-1].status.eq(self.oua[-1])
            self.comb += o_pads[o].eq(self.csr_o[-1].status)

        ###

        # self.comb += oua[0].eq(ina[0] | ina[1])
        # for i in range(inputs):
            # self.ios += Signal(name="input_" + str(i))
        # for o in range(outputs):
            # self.ios += Signal(name="output_" + str(i))
