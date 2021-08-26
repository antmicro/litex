import os

from migen import *

from litex.soc.interconnect import wishbone

class FastVDMA(Module):    
    def __init__(self, platform, data_width=32, adr_width=30):

        # Add verilog file with generated netlist for FastVDMA -------------------------------------
        platform.add_sources(os.path.abspath(os.path.dirname(__file__)), "fastvdma.v")
        
        # Attach FastVDMA signals to Wishbone buses ------------------------------------------------
        self.wb_control_bus = wishbone.Interface(data_width, adr_width)
        self.wb_read_bus = wishbone.Interface(data_width, adr_width)
        self.wb_write_bus = wishbone.Interface(data_width, adr_width)
        
        self.irq_readerDone = Signal()
        self.irq_writerDone = Signal()
        
        self.sync_readerSync = Signal()
        self.sync_writerSync = Signal()
        
        self.specials += Instance("fastvdma",
                                  i_clock = ClockSignal(),
                                  i_reset = ResetSignal(),
                                  i_io_control_dat_i = self.wb_control_bus.dat_w,
                                  o_io_control_dat_o = self.wb_control_bus.dat_r,
                                  i_io_control_cyc_i = self.wb_control_bus.cyc,
                                  i_io_control_stb_i = self.wb_control_bus.stb,
                                  i_io_control_we_i = self.wb_control_bus.we,
                                  i_io_control_adr_i = self.wb_control_bus.adr,
                                  i_io_control_sel_i = self.wb_control_bus.sel,
                                  o_io_control_ack_o = self.wb_control_bus.ack,
                                  #o_io_control_stall_o = 0,
                                  o_io_control_err_o = self.wb_control_bus.err,

                                  i_io_read_dat_i = self.wb_read_bus.dat_r,
                                  o_io_read_dat_o = self.wb_read_bus.dat_w,
                                  o_io_read_cyc_o = self.wb_read_bus.cyc,
                                  o_io_read_stb_o = self.wb_read_bus.stb,
                                  o_io_read_we_o = self.wb_read_bus.we,
                                  o_io_read_adr_o = self.wb_read_bus.adr,
                                  o_io_read_sel_o = self.wb_read_bus.sel,
                                  i_io_read_ack_i = self.wb_read_bus.ack,
                                  #i_io_read_stall_i = 0,
                                  i_io_read_err_i = self.wb_read_bus.err,

                                  i_io_write_dat_i = self.wb_write_bus.dat_r,
                                  o_io_write_dat_o = self.wb_write_bus.dat_w,
                                  o_io_write_cyc_o = self.wb_write_bus.cyc,
                                  o_io_write_stb_o = self.wb_write_bus.stb,
                                  o_io_write_we_o = self.wb_write_bus.we,
                                  o_io_write_adr_o = self.wb_write_bus.adr,
                                  o_io_write_sel_o = self.wb_write_bus.sel,
                                  i_io_write_ack_i = self.wb_write_bus.ack,
                                  #i_io_write_stall_i = 0,
                                  i_io_write_err_i = self.wb_write_bus.err,

                                  o_io_irq_readerDone = self.irq_readerDone,
                                  o_io_irq_writerDone = self.irq_writerDone,

                                  i_io_sync_readerSync = self.sync_readerSync,
                                  i_io_sync_writerSync = self.sync_writerSync,
        )
