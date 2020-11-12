# Copyright (c) 2020 Antmicro LTD

from migen import *

from litex.soc.interconnect.csr import *

from litex.soc.interconnect import wishbone

class PRIOInterfacer(Module, AutoCSR):
    def __init__(self, bus, pads, inputs, outputs):
        # assert isinstance(bus, wishbone.Interface)
        self.ina = []
        self.csr_i = []
        self.oua = []
        self.csr_o = []

        self.bus = wishbone.Interface(data_width=32)
        # prm_region = bus.alloc_region("prm", 1, False)
        # bus.add_region("prm", prm_region)
        # print("PRM ITERFACE: BUS: " + bus.__str__())
        # bus.add_slave(name="prm", slave=bus, region=prm_region)
        bus_io = self.bus.get_ios()
        print("PRM ITERFACE: WISHBONE IO: ")
        print(bus_io)
        self.comb += self.bus.connect_to_pads(pads=pads, mode="slave")

        # for i in range(inputs):
            # self.ina.append(Signal())
            # self.ina[-1].attr.add("keep")
            # self.csr_i.append(CSRStorage(name="csr_storage_" + str(i)))
            # self.sync += self.ina[-1].eq(self.csr_i[-1].storage)

        # for o in range(outputs):
            # self.oua.append(Signal())
            # self.oua[-1].attr.add("keep")
            # self.csr_o.append(CSRStatus(name="csr_status_" + str(i)))
            # self.sync += self.csr_o[-1].status.eq(self.oua[-1])
            # self.comb += o_pads[o].eq(self.csr_o[-1].status)

        ###

        # self.comb += oua[0].eq(ina[0] | ina[1])
        # for i in range(inputs):
            # self.ios += Signal(name="input_" + str(i))
        # for o in range(outputs):
            # self.ios += Signal(name="output_" + str(i))
