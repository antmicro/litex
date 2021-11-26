module AXIS_to_WB_WishboneCSR( // @[:@3.2]
  input         clock, // @[:@4.4]
  input         reset, // @[:@5.4]
  input  [31:0] io_ctl_dat_i, // @[:@6.4]
  output [31:0] io_ctl_dat_o, // @[:@6.4]
  input         io_ctl_cyc_i, // @[:@6.4]
  input         io_ctl_stb_i, // @[:@6.4]
  input         io_ctl_we_i, // @[:@6.4]
  input  [29:0] io_ctl_adr_i, // @[:@6.4]
  output        io_ctl_ack_o, // @[:@6.4]
  output [3:0]  io_bus_addr, // @[:@6.4]
  output [31:0] io_bus_dataOut, // @[:@6.4]
  input  [31:0] io_bus_dataIn, // @[:@6.4]
  output        io_bus_write, // @[:@6.4]
  output        io_bus_read // @[:@6.4]
);
  reg  state; // @[WishboneCSR.scala 40:22:@8.4]
  reg [31:0] _RAND_0;
  reg  ack; // @[WishboneCSR.scala 42:21:@9.4]
  reg [31:0] _RAND_1;
  wire  valid; // @[WishboneCSR.scala 44:37:@10.4]
  wire  _T_40; // @[Conditional.scala 37:30:@13.4]
  wire  _GEN_0; // @[WishboneCSR.scala 49:40:@17.6]
  wire  _GEN_2; // @[Conditional.scala 39:67:@24.6]
  wire  _GEN_3; // @[Conditional.scala 39:67:@24.6]
  wire  _GEN_4; // @[Conditional.scala 40:58:@14.4]
  wire  _GEN_5; // @[Conditional.scala 40:58:@14.4]
  wire  _T_50; // @[WishboneCSR.scala 65:24:@33.4]
  assign valid = io_ctl_stb_i & io_ctl_cyc_i; // @[WishboneCSR.scala 44:37:@10.4]
  assign _T_40 = 1'h0 == state; // @[Conditional.scala 37:30:@13.4]
  assign _GEN_0 = valid ? 1'h1 : state; // @[WishboneCSR.scala 49:40:@17.6]
  assign _GEN_2 = state ? 1'h0 : ack; // @[Conditional.scala 39:67:@24.6]
  assign _GEN_3 = state ? 1'h0 : state; // @[Conditional.scala 39:67:@24.6]
  assign _GEN_4 = _T_40 ? valid : _GEN_2; // @[Conditional.scala 40:58:@14.4]
  assign _GEN_5 = _T_40 ? _GEN_0 : _GEN_3; // @[Conditional.scala 40:58:@14.4]
  assign _T_50 = io_ctl_we_i == 1'h0; // @[WishboneCSR.scala 65:24:@33.4]
  assign io_ctl_dat_o = io_bus_dataIn; // @[WishboneCSR.scala 68:16:@37.4]
  assign io_ctl_ack_o = ack; // @[WishboneCSR.scala 63:16:@30.4]
  assign io_bus_addr = io_ctl_adr_i[3:0]; // @[WishboneCSR.scala 69:15:@38.4]
  assign io_bus_dataOut = io_ctl_dat_i; // @[WishboneCSR.scala 67:18:@36.4]
  assign io_bus_write = ack & io_ctl_we_i; // @[WishboneCSR.scala 64:16:@32.4]
  assign io_bus_read = ack & _T_50; // @[WishboneCSR.scala 65:15:@35.4]
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE
  integer initvar;
  initial begin
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      #0.002 begin end
    `endif
  `ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  state = _RAND_0[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  ack = _RAND_1[0:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if (reset) begin
      state <= 1'h0;
    end else begin
      if (_T_40) begin
        if (valid) begin
          state <= 1'h1;
        end
      end else begin
        if (state) begin
          state <= 1'h0;
        end
      end
    end
    if (reset) begin
      ack <= 1'h0;
    end else begin
      if (_T_40) begin
        ack <= valid;
      end else begin
        if (state) begin
          ack <= 1'h0;
        end
      end
    end
  end
endmodule
module AXIStreamSlave( // @[:@40.2]
  input         clock, // @[:@41.4]
  input         reset, // @[:@42.4]
  input  [31:0] io_bus_tdata, // @[:@43.4]
  input         io_bus_tvalid, // @[:@43.4]
  output        io_bus_tready, // @[:@43.4]
  input         io_dataOut_ready, // @[:@43.4]
  output        io_dataOut_valid, // @[:@43.4]
  output [31:0] io_dataOut_bits, // @[:@43.4]
  output        io_xfer_done, // @[:@43.4]
  input  [31:0] io_xfer_length, // @[:@43.4]
  input         io_xfer_valid // @[:@43.4]
);
  reg [1:0] state; // @[AXIStreamSlave.scala 42:22:@45.4]
  reg [31:0] _RAND_0;
  reg  done; // @[AXIStreamSlave.scala 44:21:@46.4]
  reg [31:0] _RAND_1;
  reg  enable; // @[AXIStreamSlave.scala 46:23:@47.4]
  reg [31:0] _RAND_2;
  reg [31:0] length; // @[AXIStreamSlave.scala 48:23:@48.4]
  reg [31:0] _RAND_3;
  wire  ready; // @[AXIStreamSlave.scala 50:41:@49.4]
  wire  valid; // @[AXIStreamSlave.scala 51:38:@52.4]
  wire  _T_66; // @[Conditional.scala 37:30:@59.4]
  wire [1:0] _GEN_0; // @[AXIStreamSlave.scala 65:26:@63.6]
  wire [31:0] _GEN_1; // @[AXIStreamSlave.scala 65:26:@63.6]
  wire  _T_70; // @[Conditional.scala 37:30:@70.6]
  wire  _T_71; // @[AXIStreamSlave.scala 72:18:@72.8]
  wire [32:0] _T_73; // @[AXIStreamSlave.scala 73:26:@74.10]
  wire [32:0] _T_74; // @[AXIStreamSlave.scala 73:26:@75.10]
  wire [31:0] _T_75; // @[AXIStreamSlave.scala 73:26:@76.10]
  wire  _T_77; // @[AXIStreamSlave.scala 74:21:@78.10]
  wire [1:0] _GEN_3; // @[AXIStreamSlave.scala 74:29:@79.10]
  wire  _GEN_4; // @[AXIStreamSlave.scala 74:29:@79.10]
  wire [31:0] _GEN_5; // @[AXIStreamSlave.scala 72:27:@73.8]
  wire [1:0] _GEN_6; // @[AXIStreamSlave.scala 72:27:@73.8]
  wire  _GEN_7; // @[AXIStreamSlave.scala 72:27:@73.8]
  wire  _T_79; // @[Conditional.scala 37:30:@86.8]
  wire [1:0] _GEN_8; // @[Conditional.scala 39:67:@87.8]
  wire  _GEN_9; // @[Conditional.scala 39:67:@87.8]
  wire [31:0] _GEN_10; // @[Conditional.scala 39:67:@71.6]
  wire [1:0] _GEN_11; // @[Conditional.scala 39:67:@71.6]
  wire  _GEN_12; // @[Conditional.scala 39:67:@71.6]
  wire  _GEN_13; // @[Conditional.scala 39:67:@71.6]
  wire  _GEN_14; // @[Conditional.scala 40:58:@60.4]
  wire  _GEN_15; // @[Conditional.scala 40:58:@60.4]
  wire [1:0] _GEN_16; // @[Conditional.scala 40:58:@60.4]
  wire [31:0] _GEN_17; // @[Conditional.scala 40:58:@60.4]
  assign ready = io_dataOut_ready & enable; // @[AXIStreamSlave.scala 50:41:@49.4]
  assign valid = io_bus_tvalid & enable; // @[AXIStreamSlave.scala 51:38:@52.4]
  assign _T_66 = 2'h0 == state; // @[Conditional.scala 37:30:@59.4]
  assign _GEN_0 = io_xfer_valid ? 2'h1 : state; // @[AXIStreamSlave.scala 65:26:@63.6]
  assign _GEN_1 = io_xfer_valid ? io_xfer_length : length; // @[AXIStreamSlave.scala 65:26:@63.6]
  assign _T_70 = 2'h1 == state; // @[Conditional.scala 37:30:@70.6]
  assign _T_71 = ready & valid; // @[AXIStreamSlave.scala 72:18:@72.8]
  assign _T_73 = length - 32'h1; // @[AXIStreamSlave.scala 73:26:@74.10]
  assign _T_74 = $unsigned(_T_73); // @[AXIStreamSlave.scala 73:26:@75.10]
  assign _T_75 = _T_74[31:0]; // @[AXIStreamSlave.scala 73:26:@76.10]
  assign _T_77 = length == 32'h1; // @[AXIStreamSlave.scala 74:21:@78.10]
  assign _GEN_3 = _T_77 ? 2'h2 : state; // @[AXIStreamSlave.scala 74:29:@79.10]
  assign _GEN_4 = _T_77 ? 1'h0 : enable; // @[AXIStreamSlave.scala 74:29:@79.10]
  assign _GEN_5 = _T_71 ? _T_75 : length; // @[AXIStreamSlave.scala 72:27:@73.8]
  assign _GEN_6 = _T_71 ? _GEN_3 : state; // @[AXIStreamSlave.scala 72:27:@73.8]
  assign _GEN_7 = _T_71 ? _GEN_4 : enable; // @[AXIStreamSlave.scala 72:27:@73.8]
  assign _T_79 = 2'h2 == state; // @[Conditional.scala 37:30:@86.8]
  assign _GEN_8 = _T_79 ? 2'h0 : state; // @[Conditional.scala 39:67:@87.8]
  assign _GEN_9 = _T_79 ? 1'h1 : done; // @[Conditional.scala 39:67:@87.8]
  assign _GEN_10 = _T_70 ? _GEN_5 : length; // @[Conditional.scala 39:67:@71.6]
  assign _GEN_11 = _T_70 ? _GEN_6 : _GEN_8; // @[Conditional.scala 39:67:@71.6]
  assign _GEN_12 = _T_70 ? _GEN_7 : enable; // @[Conditional.scala 39:67:@71.6]
  assign _GEN_13 = _T_70 ? done : _GEN_9; // @[Conditional.scala 39:67:@71.6]
  assign _GEN_14 = _T_66 ? 1'h0 : _GEN_13; // @[Conditional.scala 40:58:@60.4]
  assign _GEN_15 = _T_66 ? io_xfer_valid : _GEN_12; // @[Conditional.scala 40:58:@60.4]
  assign _GEN_16 = _T_66 ? _GEN_0 : _GEN_11; // @[Conditional.scala 40:58:@60.4]
  assign _GEN_17 = _T_66 ? _GEN_1 : _GEN_10; // @[Conditional.scala 40:58:@60.4]
  assign io_bus_tready = io_dataOut_ready & enable; // @[AXIStreamSlave.scala 54:17:@56.4]
  assign io_dataOut_valid = io_bus_tvalid & enable; // @[AXIStreamSlave.scala 53:20:@55.4]
  assign io_dataOut_bits = io_bus_tdata; // @[AXIStreamSlave.scala 56:19:@57.4]
  assign io_xfer_done = done; // @[AXIStreamSlave.scala 58:16:@58.4]
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE
  integer initvar;
  initial begin
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      #0.002 begin end
    `endif
  `ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  state = _RAND_0[1:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  done = _RAND_1[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_2 = {1{`RANDOM}};
  enable = _RAND_2[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_3 = {1{`RANDOM}};
  length = _RAND_3[31:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if (reset) begin
      state <= 2'h0;
    end else begin
      if (_T_66) begin
        if (io_xfer_valid) begin
          state <= 2'h1;
        end
      end else begin
        if (_T_70) begin
          if (_T_71) begin
            if (_T_77) begin
              state <= 2'h2;
            end
          end
        end else begin
          if (_T_79) begin
            state <= 2'h0;
          end
        end
      end
    end
    if (reset) begin
      done <= 1'h0;
    end else begin
      if (_T_66) begin
        done <= 1'h0;
      end else begin
        if (!(_T_70)) begin
          if (_T_79) begin
            done <= 1'h1;
          end
        end
      end
    end
    if (reset) begin
      enable <= 1'h0;
    end else begin
      if (_T_66) begin
        enable <= io_xfer_valid;
      end else begin
        if (_T_70) begin
          if (_T_71) begin
            if (_T_77) begin
              enable <= 1'h0;
            end
          end
        end
      end
    end
    if (reset) begin
      length <= 32'h0;
    end else begin
      if (_T_66) begin
        if (io_xfer_valid) begin
          length <= io_xfer_length;
        end
      end else begin
        if (_T_70) begin
          if (_T_71) begin
            length <= _T_75;
          end
        end
      end
    end
  end
endmodule
module WishboneClassicWriter( // @[:@92.2]
  input         clock, // @[:@93.4]
  input         reset, // @[:@94.4]
  output [31:0] io_bus_dat_o, // @[:@95.4]
  output        io_bus_cyc_o, // @[:@95.4]
  output        io_bus_stb_o, // @[:@95.4]
  output [29:0] io_bus_adr_o, // @[:@95.4]
  input         io_bus_ack_i, // @[:@95.4]
  output        io_dataIn_ready, // @[:@95.4]
  input         io_dataIn_valid, // @[:@95.4]
  input  [31:0] io_dataIn_bits, // @[:@95.4]
  output        io_xfer_done, // @[:@95.4]
  input  [31:0] io_xfer_address, // @[:@95.4]
  input  [31:0] io_xfer_length, // @[:@95.4]
  input         io_xfer_valid // @[:@95.4]
);
  reg  state; // @[WishboneClassicWriter.scala 40:22:@97.4]
  reg [31:0] _RAND_0;
  reg [31:0] stbCnt; // @[WishboneClassicWriter.scala 44:23:@100.4]
  reg [31:0] _RAND_1;
  reg [31:0] adr; // @[WishboneClassicWriter.scala 45:20:@101.4]
  reg [31:0] _RAND_2;
  wire  _T_66; // @[WishboneClassicWriter.scala 46:29:@102.4]
  wire  stb; // @[WishboneClassicWriter.scala 46:37:@103.4]
  wire  _T_71; // @[WishboneClassicWriter.scala 50:28:@110.4]
  wire  ready; // @[WishboneClassicWriter.scala 50:35:@111.4]
  reg  done; // @[WishboneClassicWriter.scala 52:21:@114.4]
  reg [31:0] _RAND_3;
  wire  _T_80; // @[Conditional.scala 37:30:@125.4]
  wire  _GEN_0; // @[WishboneClassicWriter.scala 69:27:@128.6]
  wire [31:0] _GEN_1; // @[WishboneClassicWriter.scala 69:27:@128.6]
  wire [31:0] _GEN_2; // @[WishboneClassicWriter.scala 69:27:@128.6]
  wire  _T_84; // @[WishboneClassicWriter.scala 76:19:@137.8]
  wire  _GEN_3; // @[WishboneClassicWriter.scala 76:27:@138.8]
  wire  _GEN_4; // @[WishboneClassicWriter.scala 76:27:@138.8]
  wire  _GEN_5; // @[Conditional.scala 39:67:@136.6]
  wire  _GEN_6; // @[Conditional.scala 39:67:@136.6]
  wire  _GEN_7; // @[Conditional.scala 40:58:@126.4]
  wire  _GEN_8; // @[Conditional.scala 40:58:@126.4]
  wire [31:0] _GEN_9; // @[Conditional.scala 40:58:@126.4]
  wire [31:0] _GEN_10; // @[Conditional.scala 40:58:@126.4]
  wire  _T_88; // @[WishboneClassicWriter.scala 83:23:@144.4]
  wire [32:0] _T_90; // @[WishboneClassicWriter.scala 84:16:@146.6]
  wire [31:0] _T_91; // @[WishboneClassicWriter.scala 84:16:@147.6]
  wire [32:0] _T_93; // @[WishboneClassicWriter.scala 85:22:@149.6]
  wire [32:0] _T_94; // @[WishboneClassicWriter.scala 85:22:@150.6]
  wire [31:0] _T_95; // @[WishboneClassicWriter.scala 85:22:@151.6]
  wire [31:0] _GEN_11; // @[WishboneClassicWriter.scala 83:32:@145.4]
  wire [31:0] _GEN_12; // @[WishboneClassicWriter.scala 83:32:@145.4]
  assign _T_66 = stbCnt != 32'h0; // @[WishboneClassicWriter.scala 46:29:@102.4]
  assign stb = _T_66 & io_dataIn_valid; // @[WishboneClassicWriter.scala 46:37:@103.4]
  assign _T_71 = stb & stb; // @[WishboneClassicWriter.scala 50:28:@110.4]
  assign ready = _T_71 & io_bus_ack_i; // @[WishboneClassicWriter.scala 50:35:@111.4]
  assign _T_80 = 1'h0 == state; // @[Conditional.scala 37:30:@125.4]
  assign _GEN_0 = io_xfer_valid ? 1'h1 : state; // @[WishboneClassicWriter.scala 69:27:@128.6]
  assign _GEN_1 = io_xfer_valid ? io_xfer_length : stbCnt; // @[WishboneClassicWriter.scala 69:27:@128.6]
  assign _GEN_2 = io_xfer_valid ? io_xfer_address : adr; // @[WishboneClassicWriter.scala 69:27:@128.6]
  assign _T_84 = stbCnt == 32'h0; // @[WishboneClassicWriter.scala 76:19:@137.8]
  assign _GEN_3 = _T_84 ? 1'h0 : state; // @[WishboneClassicWriter.scala 76:27:@138.8]
  assign _GEN_4 = _T_84 ? 1'h1 : done; // @[WishboneClassicWriter.scala 76:27:@138.8]
  assign _GEN_5 = state ? _GEN_3 : state; // @[Conditional.scala 39:67:@136.6]
  assign _GEN_6 = state ? _GEN_4 : done; // @[Conditional.scala 39:67:@136.6]
  assign _GEN_7 = _T_80 ? 1'h0 : _GEN_6; // @[Conditional.scala 40:58:@126.4]
  assign _GEN_8 = _T_80 ? _GEN_0 : _GEN_5; // @[Conditional.scala 40:58:@126.4]
  assign _GEN_9 = _T_80 ? _GEN_1 : stbCnt; // @[Conditional.scala 40:58:@126.4]
  assign _GEN_10 = _T_80 ? _GEN_2 : adr; // @[Conditional.scala 40:58:@126.4]
  assign _T_88 = _T_66 & ready; // @[WishboneClassicWriter.scala 83:23:@144.4]
  assign _T_90 = adr + 32'h4; // @[WishboneClassicWriter.scala 84:16:@146.6]
  assign _T_91 = adr + 32'h4; // @[WishboneClassicWriter.scala 84:16:@147.6]
  assign _T_93 = stbCnt - 32'h1; // @[WishboneClassicWriter.scala 85:22:@149.6]
  assign _T_94 = $unsigned(_T_93); // @[WishboneClassicWriter.scala 85:22:@150.6]
  assign _T_95 = _T_94[31:0]; // @[WishboneClassicWriter.scala 85:22:@151.6]
  assign _GEN_11 = _T_88 ? _T_91 : _GEN_10; // @[WishboneClassicWriter.scala 83:32:@145.4]
  assign _GEN_12 = _T_88 ? _T_95 : _GEN_9; // @[WishboneClassicWriter.scala 83:32:@145.4]
  assign io_bus_dat_o = io_dataIn_bits; // @[WishboneClassicWriter.scala 54:16:@115.4]
  assign io_bus_cyc_o = _T_66 & io_dataIn_valid; // @[WishboneClassicWriter.scala 61:16:@122.4]
  assign io_bus_stb_o = _T_66 & io_dataIn_valid; // @[WishboneClassicWriter.scala 62:16:@123.4]
  assign io_bus_adr_o = adr[31:2]; // @[WishboneClassicWriter.scala 60:16:@121.4]
  assign io_dataIn_ready = _T_71 & io_bus_ack_i; // @[WishboneClassicWriter.scala 55:19:@116.4]
  assign io_xfer_done = done; // @[WishboneClassicWriter.scala 64:16:@124.4]
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE
  integer initvar;
  initial begin
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      #0.002 begin end
    `endif
  `ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  state = _RAND_0[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  stbCnt = _RAND_1[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_2 = {1{`RANDOM}};
  adr = _RAND_2[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_3 = {1{`RANDOM}};
  done = _RAND_3[0:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if (reset) begin
      state <= 1'h0;
    end else begin
      if (_T_80) begin
        if (io_xfer_valid) begin
          state <= 1'h1;
        end
      end else begin
        if (state) begin
          if (_T_84) begin
            state <= 1'h0;
          end
        end
      end
    end
    if (reset) begin
      stbCnt <= 32'h0;
    end else begin
      if (_T_88) begin
        stbCnt <= _T_95;
      end else begin
        if (_T_80) begin
          if (io_xfer_valid) begin
            stbCnt <= io_xfer_length;
          end
        end
      end
    end
    if (reset) begin
      adr <= 32'h0;
    end else begin
      if (_T_88) begin
        adr <= _T_91;
      end else begin
        if (_T_80) begin
          if (io_xfer_valid) begin
            adr <= io_xfer_address;
          end
        end
      end
    end
    if (reset) begin
      done <= 1'h0;
    end else begin
      if (_T_80) begin
        done <= 1'h0;
      end else begin
        if (state) begin
          if (_T_84) begin
            done <= 1'h1;
          end
        end
      end
    end
  end
endmodule
module AXIS_to_WB_CSR( // @[:@155.2]
  output [31:0] io_csr_0_dataOut, // @[:@158.4]
  output        io_csr_0_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_0_dataIn, // @[:@158.4]
  input  [31:0] io_csr_1_dataIn, // @[:@158.4]
  output [31:0] io_csr_2_dataOut, // @[:@158.4]
  output        io_csr_2_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_2_dataIn, // @[:@158.4]
  output [31:0] io_csr_3_dataOut, // @[:@158.4]
  output        io_csr_3_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_3_dataIn, // @[:@158.4]
  output [31:0] io_csr_4_dataOut, // @[:@158.4]
  output        io_csr_4_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_4_dataIn, // @[:@158.4]
  output [31:0] io_csr_5_dataOut, // @[:@158.4]
  output        io_csr_5_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_5_dataIn, // @[:@158.4]
  output [31:0] io_csr_6_dataOut, // @[:@158.4]
  output        io_csr_6_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_6_dataIn, // @[:@158.4]
  output [31:0] io_csr_7_dataOut, // @[:@158.4]
  output        io_csr_7_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_7_dataIn, // @[:@158.4]
  output [31:0] io_csr_8_dataOut, // @[:@158.4]
  output        io_csr_8_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_8_dataIn, // @[:@158.4]
  output [31:0] io_csr_9_dataOut, // @[:@158.4]
  output        io_csr_9_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_9_dataIn, // @[:@158.4]
  output [31:0] io_csr_10_dataOut, // @[:@158.4]
  output        io_csr_10_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_10_dataIn, // @[:@158.4]
  output [31:0] io_csr_11_dataOut, // @[:@158.4]
  output        io_csr_11_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_11_dataIn, // @[:@158.4]
  output [31:0] io_csr_12_dataOut, // @[:@158.4]
  output        io_csr_12_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_12_dataIn, // @[:@158.4]
  output [31:0] io_csr_13_dataOut, // @[:@158.4]
  output        io_csr_13_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_13_dataIn, // @[:@158.4]
  output [31:0] io_csr_14_dataOut, // @[:@158.4]
  output        io_csr_14_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_14_dataIn, // @[:@158.4]
  output [31:0] io_csr_15_dataOut, // @[:@158.4]
  output        io_csr_15_dataWrite, // @[:@158.4]
  input  [31:0] io_csr_15_dataIn, // @[:@158.4]
  input  [3:0]  io_bus_addr, // @[:@158.4]
  input  [31:0] io_bus_dataOut, // @[:@158.4]
  output [31:0] io_bus_dataIn, // @[:@158.4]
  input         io_bus_write, // @[:@158.4]
  input         io_bus_read // @[:@158.4]
);
  wire  _T_178; // @[CSR.scala 40:22:@163.4]
  wire  _T_179; // @[CSR.scala 40:30:@164.4]
  wire [31:0] _GEN_0; // @[CSR.scala 40:45:@165.4]
  wire  _T_184; // @[CSR.scala 47:30:@173.4]
  wire  _T_189; // @[CSR.scala 40:22:@182.4]
  wire  _T_190; // @[CSR.scala 40:30:@183.4]
  wire [31:0] _GEN_4; // @[CSR.scala 40:45:@184.4]
  wire  _T_200; // @[CSR.scala 40:22:@201.4]
  wire  _T_201; // @[CSR.scala 40:30:@202.4]
  wire [31:0] _GEN_8; // @[CSR.scala 40:45:@203.4]
  wire  _T_206; // @[CSR.scala 47:30:@211.4]
  wire  _T_211; // @[CSR.scala 40:22:@220.4]
  wire  _T_212; // @[CSR.scala 40:30:@221.4]
  wire [31:0] _GEN_12; // @[CSR.scala 40:45:@222.4]
  wire  _T_217; // @[CSR.scala 47:30:@230.4]
  wire  _T_222; // @[CSR.scala 40:22:@239.4]
  wire  _T_223; // @[CSR.scala 40:30:@240.4]
  wire [31:0] _GEN_16; // @[CSR.scala 40:45:@241.4]
  wire  _T_228; // @[CSR.scala 47:30:@249.4]
  wire  _T_233; // @[CSR.scala 40:22:@258.4]
  wire  _T_234; // @[CSR.scala 40:30:@259.4]
  wire [31:0] _GEN_20; // @[CSR.scala 40:45:@260.4]
  wire  _T_239; // @[CSR.scala 47:30:@268.4]
  wire  _T_244; // @[CSR.scala 40:22:@277.4]
  wire  _T_245; // @[CSR.scala 40:30:@278.4]
  wire [31:0] _GEN_24; // @[CSR.scala 40:45:@279.4]
  wire  _T_250; // @[CSR.scala 47:30:@287.4]
  wire  _T_255; // @[CSR.scala 40:22:@296.4]
  wire  _T_256; // @[CSR.scala 40:30:@297.4]
  wire [31:0] _GEN_28; // @[CSR.scala 40:45:@298.4]
  wire  _T_261; // @[CSR.scala 47:30:@306.4]
  wire  _T_266; // @[CSR.scala 40:22:@315.4]
  wire  _T_267; // @[CSR.scala 40:30:@316.4]
  wire [31:0] _GEN_32; // @[CSR.scala 40:45:@317.4]
  wire  _T_272; // @[CSR.scala 47:30:@325.4]
  wire  _T_277; // @[CSR.scala 40:22:@334.4]
  wire  _T_278; // @[CSR.scala 40:30:@335.4]
  wire [31:0] _GEN_36; // @[CSR.scala 40:45:@336.4]
  wire  _T_283; // @[CSR.scala 47:30:@344.4]
  wire  _T_288; // @[CSR.scala 40:22:@353.4]
  wire  _T_289; // @[CSR.scala 40:30:@354.4]
  wire [31:0] _GEN_40; // @[CSR.scala 40:45:@355.4]
  wire  _T_294; // @[CSR.scala 47:30:@363.4]
  wire  _T_299; // @[CSR.scala 40:22:@372.4]
  wire  _T_300; // @[CSR.scala 40:30:@373.4]
  wire [31:0] _GEN_44; // @[CSR.scala 40:45:@374.4]
  wire  _T_305; // @[CSR.scala 47:30:@382.4]
  wire  _T_310; // @[CSR.scala 40:22:@391.4]
  wire  _T_311; // @[CSR.scala 40:30:@392.4]
  wire [31:0] _GEN_48; // @[CSR.scala 40:45:@393.4]
  wire  _T_316; // @[CSR.scala 47:30:@401.4]
  wire  _T_321; // @[CSR.scala 40:22:@410.4]
  wire  _T_322; // @[CSR.scala 40:30:@411.4]
  wire [31:0] _GEN_52; // @[CSR.scala 40:45:@412.4]
  wire  _T_327; // @[CSR.scala 47:30:@420.4]
  wire  _T_332; // @[CSR.scala 40:22:@429.4]
  wire  _T_333; // @[CSR.scala 40:30:@430.4]
  wire [31:0] _GEN_56; // @[CSR.scala 40:45:@431.4]
  wire  _T_338; // @[CSR.scala 47:30:@439.4]
  wire  _T_343; // @[CSR.scala 40:22:@448.4]
  wire  _T_344; // @[CSR.scala 40:30:@449.4]
  wire  _T_349; // @[CSR.scala 47:30:@458.4]
  assign _T_178 = io_bus_addr == 4'h0; // @[CSR.scala 40:22:@163.4]
  assign _T_179 = _T_178 & io_bus_read; // @[CSR.scala 40:30:@164.4]
  assign _GEN_0 = _T_179 ? io_csr_0_dataIn : 32'h0; // @[CSR.scala 40:45:@165.4]
  assign _T_184 = _T_178 & io_bus_write; // @[CSR.scala 47:30:@173.4]
  assign _T_189 = io_bus_addr == 4'h1; // @[CSR.scala 40:22:@182.4]
  assign _T_190 = _T_189 & io_bus_read; // @[CSR.scala 40:30:@183.4]
  assign _GEN_4 = _T_190 ? io_csr_1_dataIn : _GEN_0; // @[CSR.scala 40:45:@184.4]
  assign _T_200 = io_bus_addr == 4'h2; // @[CSR.scala 40:22:@201.4]
  assign _T_201 = _T_200 & io_bus_read; // @[CSR.scala 40:30:@202.4]
  assign _GEN_8 = _T_201 ? io_csr_2_dataIn : _GEN_4; // @[CSR.scala 40:45:@203.4]
  assign _T_206 = _T_200 & io_bus_write; // @[CSR.scala 47:30:@211.4]
  assign _T_211 = io_bus_addr == 4'h3; // @[CSR.scala 40:22:@220.4]
  assign _T_212 = _T_211 & io_bus_read; // @[CSR.scala 40:30:@221.4]
  assign _GEN_12 = _T_212 ? io_csr_3_dataIn : _GEN_8; // @[CSR.scala 40:45:@222.4]
  assign _T_217 = _T_211 & io_bus_write; // @[CSR.scala 47:30:@230.4]
  assign _T_222 = io_bus_addr == 4'h4; // @[CSR.scala 40:22:@239.4]
  assign _T_223 = _T_222 & io_bus_read; // @[CSR.scala 40:30:@240.4]
  assign _GEN_16 = _T_223 ? io_csr_4_dataIn : _GEN_12; // @[CSR.scala 40:45:@241.4]
  assign _T_228 = _T_222 & io_bus_write; // @[CSR.scala 47:30:@249.4]
  assign _T_233 = io_bus_addr == 4'h5; // @[CSR.scala 40:22:@258.4]
  assign _T_234 = _T_233 & io_bus_read; // @[CSR.scala 40:30:@259.4]
  assign _GEN_20 = _T_234 ? io_csr_5_dataIn : _GEN_16; // @[CSR.scala 40:45:@260.4]
  assign _T_239 = _T_233 & io_bus_write; // @[CSR.scala 47:30:@268.4]
  assign _T_244 = io_bus_addr == 4'h6; // @[CSR.scala 40:22:@277.4]
  assign _T_245 = _T_244 & io_bus_read; // @[CSR.scala 40:30:@278.4]
  assign _GEN_24 = _T_245 ? io_csr_6_dataIn : _GEN_20; // @[CSR.scala 40:45:@279.4]
  assign _T_250 = _T_244 & io_bus_write; // @[CSR.scala 47:30:@287.4]
  assign _T_255 = io_bus_addr == 4'h7; // @[CSR.scala 40:22:@296.4]
  assign _T_256 = _T_255 & io_bus_read; // @[CSR.scala 40:30:@297.4]
  assign _GEN_28 = _T_256 ? io_csr_7_dataIn : _GEN_24; // @[CSR.scala 40:45:@298.4]
  assign _T_261 = _T_255 & io_bus_write; // @[CSR.scala 47:30:@306.4]
  assign _T_266 = io_bus_addr == 4'h8; // @[CSR.scala 40:22:@315.4]
  assign _T_267 = _T_266 & io_bus_read; // @[CSR.scala 40:30:@316.4]
  assign _GEN_32 = _T_267 ? io_csr_8_dataIn : _GEN_28; // @[CSR.scala 40:45:@317.4]
  assign _T_272 = _T_266 & io_bus_write; // @[CSR.scala 47:30:@325.4]
  assign _T_277 = io_bus_addr == 4'h9; // @[CSR.scala 40:22:@334.4]
  assign _T_278 = _T_277 & io_bus_read; // @[CSR.scala 40:30:@335.4]
  assign _GEN_36 = _T_278 ? io_csr_9_dataIn : _GEN_32; // @[CSR.scala 40:45:@336.4]
  assign _T_283 = _T_277 & io_bus_write; // @[CSR.scala 47:30:@344.4]
  assign _T_288 = io_bus_addr == 4'ha; // @[CSR.scala 40:22:@353.4]
  assign _T_289 = _T_288 & io_bus_read; // @[CSR.scala 40:30:@354.4]
  assign _GEN_40 = _T_289 ? io_csr_10_dataIn : _GEN_36; // @[CSR.scala 40:45:@355.4]
  assign _T_294 = _T_288 & io_bus_write; // @[CSR.scala 47:30:@363.4]
  assign _T_299 = io_bus_addr == 4'hb; // @[CSR.scala 40:22:@372.4]
  assign _T_300 = _T_299 & io_bus_read; // @[CSR.scala 40:30:@373.4]
  assign _GEN_44 = _T_300 ? io_csr_11_dataIn : _GEN_40; // @[CSR.scala 40:45:@374.4]
  assign _T_305 = _T_299 & io_bus_write; // @[CSR.scala 47:30:@382.4]
  assign _T_310 = io_bus_addr == 4'hc; // @[CSR.scala 40:22:@391.4]
  assign _T_311 = _T_310 & io_bus_read; // @[CSR.scala 40:30:@392.4]
  assign _GEN_48 = _T_311 ? io_csr_12_dataIn : _GEN_44; // @[CSR.scala 40:45:@393.4]
  assign _T_316 = _T_310 & io_bus_write; // @[CSR.scala 47:30:@401.4]
  assign _T_321 = io_bus_addr == 4'hd; // @[CSR.scala 40:22:@410.4]
  assign _T_322 = _T_321 & io_bus_read; // @[CSR.scala 40:30:@411.4]
  assign _GEN_52 = _T_322 ? io_csr_13_dataIn : _GEN_48; // @[CSR.scala 40:45:@412.4]
  assign _T_327 = _T_321 & io_bus_write; // @[CSR.scala 47:30:@420.4]
  assign _T_332 = io_bus_addr == 4'he; // @[CSR.scala 40:22:@429.4]
  assign _T_333 = _T_332 & io_bus_read; // @[CSR.scala 40:30:@430.4]
  assign _GEN_56 = _T_333 ? io_csr_14_dataIn : _GEN_52; // @[CSR.scala 40:45:@431.4]
  assign _T_338 = _T_332 & io_bus_write; // @[CSR.scala 47:30:@439.4]
  assign _T_343 = io_bus_addr == 4'hf; // @[CSR.scala 40:22:@448.4]
  assign _T_344 = _T_343 & io_bus_read; // @[CSR.scala 40:30:@449.4]
  assign _T_349 = _T_343 & io_bus_write; // @[CSR.scala 47:30:@458.4]
  assign io_csr_0_dataOut = _T_184 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@175.6 CSR.scala 52:25:@180.6]
  assign io_csr_0_dataWrite = _T_178 & io_bus_write; // @[CSR.scala 49:27:@176.6 CSR.scala 51:27:@179.6]
  assign io_csr_2_dataOut = _T_206 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@213.6 CSR.scala 52:25:@218.6]
  assign io_csr_2_dataWrite = _T_200 & io_bus_write; // @[CSR.scala 49:27:@214.6 CSR.scala 51:27:@217.6]
  assign io_csr_3_dataOut = _T_217 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@232.6 CSR.scala 52:25:@237.6]
  assign io_csr_3_dataWrite = _T_211 & io_bus_write; // @[CSR.scala 49:27:@233.6 CSR.scala 51:27:@236.6]
  assign io_csr_4_dataOut = _T_228 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@251.6 CSR.scala 52:25:@256.6]
  assign io_csr_4_dataWrite = _T_222 & io_bus_write; // @[CSR.scala 49:27:@252.6 CSR.scala 51:27:@255.6]
  assign io_csr_5_dataOut = _T_239 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@270.6 CSR.scala 52:25:@275.6]
  assign io_csr_5_dataWrite = _T_233 & io_bus_write; // @[CSR.scala 49:27:@271.6 CSR.scala 51:27:@274.6]
  assign io_csr_6_dataOut = _T_250 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@289.6 CSR.scala 52:25:@294.6]
  assign io_csr_6_dataWrite = _T_244 & io_bus_write; // @[CSR.scala 49:27:@290.6 CSR.scala 51:27:@293.6]
  assign io_csr_7_dataOut = _T_261 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@308.6 CSR.scala 52:25:@313.6]
  assign io_csr_7_dataWrite = _T_255 & io_bus_write; // @[CSR.scala 49:27:@309.6 CSR.scala 51:27:@312.6]
  assign io_csr_8_dataOut = _T_272 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@327.6 CSR.scala 52:25:@332.6]
  assign io_csr_8_dataWrite = _T_266 & io_bus_write; // @[CSR.scala 49:27:@328.6 CSR.scala 51:27:@331.6]
  assign io_csr_9_dataOut = _T_283 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@346.6 CSR.scala 52:25:@351.6]
  assign io_csr_9_dataWrite = _T_277 & io_bus_write; // @[CSR.scala 49:27:@347.6 CSR.scala 51:27:@350.6]
  assign io_csr_10_dataOut = _T_294 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@365.6 CSR.scala 52:25:@370.6]
  assign io_csr_10_dataWrite = _T_288 & io_bus_write; // @[CSR.scala 49:27:@366.6 CSR.scala 51:27:@369.6]
  assign io_csr_11_dataOut = _T_305 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@384.6 CSR.scala 52:25:@389.6]
  assign io_csr_11_dataWrite = _T_299 & io_bus_write; // @[CSR.scala 49:27:@385.6 CSR.scala 51:27:@388.6]
  assign io_csr_12_dataOut = _T_316 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@403.6 CSR.scala 52:25:@408.6]
  assign io_csr_12_dataWrite = _T_310 & io_bus_write; // @[CSR.scala 49:27:@404.6 CSR.scala 51:27:@407.6]
  assign io_csr_13_dataOut = _T_327 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@422.6 CSR.scala 52:25:@427.6]
  assign io_csr_13_dataWrite = _T_321 & io_bus_write; // @[CSR.scala 49:27:@423.6 CSR.scala 51:27:@426.6]
  assign io_csr_14_dataOut = _T_338 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@441.6 CSR.scala 52:25:@446.6]
  assign io_csr_14_dataWrite = _T_332 & io_bus_write; // @[CSR.scala 49:27:@442.6 CSR.scala 51:27:@445.6]
  assign io_csr_15_dataOut = _T_349 ? io_bus_dataOut : 32'h0; // @[CSR.scala 48:25:@460.6 CSR.scala 52:25:@465.6]
  assign io_csr_15_dataWrite = _T_343 & io_bus_write; // @[CSR.scala 49:27:@461.6 CSR.scala 51:27:@464.6]
  assign io_bus_dataIn = _T_344 ? io_csr_15_dataIn : _GEN_56; // @[CSR.scala 37:17:@162.4]
endmodule
module AXIS_to_WB_AddressGenerator( // @[:@468.2]
  input         clock, // @[:@469.4]
  input         reset, // @[:@470.4]
  input         io_ctl_start, // @[:@471.4]
  output        io_ctl_busy, // @[:@471.4]
  input  [31:0] io_ctl_startAddress, // @[:@471.4]
  input  [31:0] io_ctl_lineLength, // @[:@471.4]
  input  [31:0] io_ctl_lineCount, // @[:@471.4]
  input  [31:0] io_ctl_lineGap, // @[:@471.4]
  input         io_xfer_done, // @[:@471.4]
  output [31:0] io_xfer_address, // @[:@471.4]
  output [31:0] io_xfer_length, // @[:@471.4]
  output        io_xfer_valid // @[:@471.4]
);
  reg [1:0] state; // @[AddressGenerator.scala 37:22:@473.4]
  reg [31:0] _RAND_0;
  reg [31:0] lineCount; // @[AddressGenerator.scala 39:26:@474.4]
  reg [31:0] _RAND_1;
  reg [31:0] lineGap; // @[AddressGenerator.scala 40:24:@475.4]
  reg [31:0] _RAND_2;
  reg [31:0] address_o; // @[AddressGenerator.scala 42:26:@476.4]
  reg [31:0] _RAND_3;
  reg [31:0] address_i; // @[AddressGenerator.scala 43:26:@477.4]
  reg [31:0] _RAND_4;
  reg [31:0] length_o; // @[AddressGenerator.scala 44:25:@478.4]
  reg [31:0] _RAND_5;
  reg [31:0] length_i; // @[AddressGenerator.scala 45:25:@479.4]
  reg [31:0] _RAND_6;
  reg  valid; // @[AddressGenerator.scala 46:22:@480.4]
  reg [31:0] _RAND_7;
  reg  busy; // @[AddressGenerator.scala 48:21:@482.4]
  reg [31:0] _RAND_8;
  wire  _T_46; // @[AddressGenerator.scala 56:14:@488.4]
  wire  _GEN_0; // @[AddressGenerator.scala 56:24:@489.4]
  wire  _T_49; // @[Conditional.scala 37:30:@495.4]
  wire [1:0] _GEN_1; // @[AddressGenerator.scala 64:25:@497.6]
  wire [31:0] _GEN_2; // @[AddressGenerator.scala 64:25:@497.6]
  wire [31:0] _GEN_3; // @[AddressGenerator.scala 64:25:@497.6]
  wire [31:0] _GEN_4; // @[AddressGenerator.scala 64:25:@497.6]
  wire [31:0] _GEN_5; // @[AddressGenerator.scala 64:25:@497.6]
  wire  _T_51; // @[Conditional.scala 37:30:@507.6]
  wire [34:0] _T_54; // @[AddressGenerator.scala 79:42:@512.8]
  wire [34:0] _GEN_28; // @[AddressGenerator.scala 79:30:@513.8]
  wire [35:0] _T_55; // @[AddressGenerator.scala 79:30:@513.8]
  wire [34:0] _T_56; // @[AddressGenerator.scala 79:30:@514.8]
  wire [34:0] _T_58; // @[AddressGenerator.scala 79:74:@515.8]
  wire [35:0] _T_59; // @[AddressGenerator.scala 79:63:@516.8]
  wire [34:0] _T_60; // @[AddressGenerator.scala 79:63:@517.8]
  wire [32:0] _T_62; // @[AddressGenerator.scala 81:30:@519.8]
  wire [32:0] _T_63; // @[AddressGenerator.scala 81:30:@520.8]
  wire [31:0] _T_64; // @[AddressGenerator.scala 81:30:@521.8]
  wire  _T_65; // @[Conditional.scala 37:30:@526.8]
  wire  _T_69; // @[AddressGenerator.scala 88:24:@531.12]
  wire [1:0] _GEN_7; // @[AddressGenerator.scala 88:30:@532.12]
  wire [1:0] _GEN_8; // @[AddressGenerator.scala 87:25:@530.10]
  wire  _GEN_9; // @[Conditional.scala 39:67:@527.8]
  wire [1:0] _GEN_11; // @[Conditional.scala 39:67:@527.8]
  wire  _GEN_12; // @[Conditional.scala 39:67:@508.6]
  wire [31:0] _GEN_13; // @[Conditional.scala 39:67:@508.6]
  wire [31:0] _GEN_14; // @[Conditional.scala 39:67:@508.6]
  wire [34:0] _GEN_15; // @[Conditional.scala 39:67:@508.6]
  wire [31:0] _GEN_16; // @[Conditional.scala 39:67:@508.6]
  wire [1:0] _GEN_17; // @[Conditional.scala 39:67:@508.6]
  wire [1:0] _GEN_19; // @[Conditional.scala 40:58:@496.4]
  wire [34:0] _GEN_20; // @[Conditional.scala 40:58:@496.4]
  wire [31:0] _GEN_21; // @[Conditional.scala 40:58:@496.4]
  wire [31:0] _GEN_22; // @[Conditional.scala 40:58:@496.4]
  wire [31:0] _GEN_23; // @[Conditional.scala 40:58:@496.4]
  wire  _GEN_25; // @[Conditional.scala 40:58:@496.4]
  wire [31:0] _GEN_26; // @[Conditional.scala 40:58:@496.4]
  wire [31:0] _GEN_27; // @[Conditional.scala 40:58:@496.4]
  assign _T_46 = state == 2'h0; // @[AddressGenerator.scala 56:14:@488.4]
  assign _GEN_0 = _T_46 ? 1'h0 : 1'h1; // @[AddressGenerator.scala 56:24:@489.4]
  assign _T_49 = 2'h0 == state; // @[Conditional.scala 37:30:@495.4]
  assign _GEN_1 = io_ctl_start ? 2'h1 : state; // @[AddressGenerator.scala 64:25:@497.6]
  assign _GEN_2 = io_ctl_start ? io_ctl_startAddress : address_i; // @[AddressGenerator.scala 64:25:@497.6]
  assign _GEN_3 = io_ctl_start ? io_ctl_lineLength : length_i; // @[AddressGenerator.scala 64:25:@497.6]
  assign _GEN_4 = io_ctl_start ? io_ctl_lineCount : lineCount; // @[AddressGenerator.scala 64:25:@497.6]
  assign _GEN_5 = io_ctl_start ? io_ctl_lineGap : lineGap; // @[AddressGenerator.scala 64:25:@497.6]
  assign _T_51 = 2'h1 == state; // @[Conditional.scala 37:30:@507.6]
  assign _T_54 = length_i * 32'h4; // @[AddressGenerator.scala 79:42:@512.8]
  assign _GEN_28 = {{3'd0}, address_i}; // @[AddressGenerator.scala 79:30:@513.8]
  assign _T_55 = _GEN_28 + _T_54; // @[AddressGenerator.scala 79:30:@513.8]
  assign _T_56 = _GEN_28 + _T_54; // @[AddressGenerator.scala 79:30:@514.8]
  assign _T_58 = lineGap * 32'h4; // @[AddressGenerator.scala 79:74:@515.8]
  assign _T_59 = _T_56 + _T_58; // @[AddressGenerator.scala 79:63:@516.8]
  assign _T_60 = _T_56 + _T_58; // @[AddressGenerator.scala 79:63:@517.8]
  assign _T_62 = lineCount - 32'h1; // @[AddressGenerator.scala 81:30:@519.8]
  assign _T_63 = $unsigned(_T_62); // @[AddressGenerator.scala 81:30:@520.8]
  assign _T_64 = _T_63[31:0]; // @[AddressGenerator.scala 81:30:@521.8]
  assign _T_65 = 2'h2 == state; // @[Conditional.scala 37:30:@526.8]
  assign _T_69 = lineCount > 32'h0; // @[AddressGenerator.scala 88:24:@531.12]
  assign _GEN_7 = _T_69 ? 2'h1 : 2'h0; // @[AddressGenerator.scala 88:30:@532.12]
  assign _GEN_8 = io_xfer_done ? _GEN_7 : state; // @[AddressGenerator.scala 87:25:@530.10]
  assign _GEN_9 = _T_65 ? 1'h0 : valid; // @[Conditional.scala 39:67:@527.8]
  assign _GEN_11 = _T_65 ? _GEN_8 : state; // @[Conditional.scala 39:67:@527.8]
  assign _GEN_12 = _T_51 ? 1'h1 : _GEN_9; // @[Conditional.scala 39:67:@508.6]
  assign _GEN_13 = _T_51 ? address_i : address_o; // @[Conditional.scala 39:67:@508.6]
  assign _GEN_14 = _T_51 ? length_i : length_o; // @[Conditional.scala 39:67:@508.6]
  assign _GEN_15 = _T_51 ? _T_60 : {{3'd0}, address_i}; // @[Conditional.scala 39:67:@508.6]
  assign _GEN_16 = _T_51 ? _T_64 : lineCount; // @[Conditional.scala 39:67:@508.6]
  assign _GEN_17 = _T_51 ? 2'h2 : _GEN_11; // @[Conditional.scala 39:67:@508.6]
  assign _GEN_19 = _T_49 ? _GEN_1 : _GEN_17; // @[Conditional.scala 40:58:@496.4]
  assign _GEN_20 = _T_49 ? {{3'd0}, _GEN_2} : _GEN_15; // @[Conditional.scala 40:58:@496.4]
  assign _GEN_21 = _T_49 ? _GEN_3 : length_i; // @[Conditional.scala 40:58:@496.4]
  assign _GEN_22 = _T_49 ? _GEN_4 : _GEN_16; // @[Conditional.scala 40:58:@496.4]
  assign _GEN_23 = _T_49 ? _GEN_5 : lineGap; // @[Conditional.scala 40:58:@496.4]
  assign _GEN_25 = _T_49 ? valid : _GEN_12; // @[Conditional.scala 40:58:@496.4]
  assign _GEN_26 = _T_49 ? address_o : _GEN_13; // @[Conditional.scala 40:58:@496.4]
  assign _GEN_27 = _T_49 ? length_o : _GEN_14; // @[Conditional.scala 40:58:@496.4]
  assign io_ctl_busy = busy; // @[AddressGenerator.scala 54:15:@487.4]
  assign io_xfer_address = address_o; // @[AddressGenerator.scala 50:19:@483.4]
  assign io_xfer_length = length_o; // @[AddressGenerator.scala 51:18:@484.4]
  assign io_xfer_valid = valid; // @[AddressGenerator.scala 52:17:@485.4]
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE
  integer initvar;
  initial begin
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      #0.002 begin end
    `endif
  `ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  state = _RAND_0[1:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  lineCount = _RAND_1[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_2 = {1{`RANDOM}};
  lineGap = _RAND_2[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_3 = {1{`RANDOM}};
  address_o = _RAND_3[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_4 = {1{`RANDOM}};
  address_i = _RAND_4[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_5 = {1{`RANDOM}};
  length_o = _RAND_5[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_6 = {1{`RANDOM}};
  length_i = _RAND_6[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_7 = {1{`RANDOM}};
  valid = _RAND_7[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_8 = {1{`RANDOM}};
  busy = _RAND_8[0:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if (reset) begin
      state <= 2'h0;
    end else begin
      if (_T_49) begin
        if (io_ctl_start) begin
          state <= 2'h1;
        end
      end else begin
        if (_T_51) begin
          state <= 2'h2;
        end else begin
          if (_T_65) begin
            if (io_xfer_done) begin
              if (_T_69) begin
                state <= 2'h1;
              end else begin
                state <= 2'h0;
              end
            end
          end
        end
      end
    end
    if (reset) begin
      lineCount <= 32'h0;
    end else begin
      if (_T_49) begin
        if (io_ctl_start) begin
          lineCount <= io_ctl_lineCount;
        end
      end else begin
        if (_T_51) begin
          lineCount <= _T_64;
        end
      end
    end
    if (reset) begin
      lineGap <= 32'h0;
    end else begin
      if (_T_49) begin
        if (io_ctl_start) begin
          lineGap <= io_ctl_lineGap;
        end
      end
    end
    if (reset) begin
      address_o <= 32'h0;
    end else begin
      if (!(_T_49)) begin
        if (_T_51) begin
          address_o <= address_i;
        end
      end
    end
    if (reset) begin
      address_i <= 32'h0;
    end else begin
      address_i <= _GEN_20[31:0];
    end
    if (reset) begin
      length_o <= 32'h0;
    end else begin
      if (!(_T_49)) begin
        if (_T_51) begin
          length_o <= length_i;
        end
      end
    end
    if (reset) begin
      length_i <= 32'h0;
    end else begin
      if (_T_49) begin
        if (io_ctl_start) begin
          length_i <= io_ctl_lineLength;
        end
      end
    end
    if (reset) begin
      valid <= 1'h0;
    end else begin
      if (!(_T_49)) begin
        if (_T_51) begin
          valid <= 1'h1;
        end else begin
          if (_T_65) begin
            valid <= 1'h0;
          end
        end
      end
    end
    if (reset) begin
      busy <= 1'h0;
    end else begin
      if (_T_46) begin
        busy <= 1'h0;
      end else begin
        busy <= 1'h1;
      end
    end
  end
endmodule
module AXIS_to_WB_TransferSplitter( // @[:@541.2]
  output        io_xferIn_done, // @[:@544.4]
  input  [31:0] io_xferIn_length, // @[:@544.4]
  input         io_xferIn_valid, // @[:@544.4]
  input         io_xferOut_done, // @[:@544.4]
  output [31:0] io_xferOut_length, // @[:@544.4]
  output        io_xferOut_valid // @[:@544.4]
);
  assign io_xferIn_done = io_xferOut_done; // @[TransferSplitter.scala 132:16:@550.4]
  assign io_xferOut_length = io_xferIn_length; // @[TransferSplitter.scala 132:16:@548.4]
  assign io_xferOut_valid = io_xferIn_valid; // @[TransferSplitter.scala 132:16:@547.4]
endmodule
module AXIS_to_WB_TransferSplitter_1( // @[:@625.2]
  input         clock, // @[:@626.4]
  input         reset, // @[:@627.4]
  output        io_xferIn_done, // @[:@628.4]
  input  [31:0] io_xferIn_address, // @[:@628.4]
  input  [31:0] io_xferIn_length, // @[:@628.4]
  input         io_xferIn_valid, // @[:@628.4]
  input         io_xferOut_done, // @[:@628.4]
  output [31:0] io_xferOut_address, // @[:@628.4]
  output [31:0] io_xferOut_length, // @[:@628.4]
  output        io_xferOut_valid // @[:@628.4]
);
  reg [31:0] _T_42; // @[TransferSplitter.scala 43:28:@630.4]
  reg [31:0] _RAND_0;
  reg [31:0] _T_45; // @[TransferSplitter.scala 44:27:@631.4]
  reg [31:0] _RAND_1;
  reg [31:0] _T_48; // @[TransferSplitter.scala 46:28:@632.4]
  reg [31:0] _RAND_2;
  reg [31:0] _T_51; // @[TransferSplitter.scala 47:27:@633.4]
  reg [31:0] _RAND_3;
  reg  _T_60; // @[TransferSplitter.scala 52:23:@636.4]
  reg [31:0] _RAND_4;
  reg  _T_63; // @[TransferSplitter.scala 53:24:@637.4]
  reg [31:0] _RAND_5;
  reg [1:0] _T_65; // @[TransferSplitter.scala 55:24:@638.4]
  reg [31:0] _RAND_6;
  wire  _T_66; // @[Conditional.scala 37:30:@644.4]
  wire [31:0] _GEN_0; // @[TransferSplitter.scala 68:31:@647.6]
  wire [31:0] _GEN_1; // @[TransferSplitter.scala 68:31:@647.6]
  wire [1:0] _GEN_3; // @[TransferSplitter.scala 68:31:@647.6]
  wire  _T_68; // @[Conditional.scala 37:30:@655.6]
  wire  _T_71; // @[TransferSplitter.scala 81:23:@661.8]
  wire [32:0] _T_74; // @[TransferSplitter.scala 84:34:@664.10]
  wire [32:0] _T_75; // @[TransferSplitter.scala 84:34:@665.10]
  wire [31:0] _T_76; // @[TransferSplitter.scala 84:34:@666.10]
  wire [11:0] _T_79; // @[TransferSplitter.scala 85:50:@668.10]
  wire [31:0] _GEN_33; // @[TransferSplitter.scala 85:36:@669.10]
  wire [32:0] _T_80; // @[TransferSplitter.scala 85:36:@669.10]
  wire [31:0] _T_81; // @[TransferSplitter.scala 85:36:@670.10]
  wire [34:0] _T_84; // @[TransferSplitter.scala 103:47:@676.10]
  wire [34:0] _GEN_34; // @[TransferSplitter.scala 103:36:@677.10]
  wire [35:0] _T_85; // @[TransferSplitter.scala 103:36:@677.10]
  wire [34:0] _T_86; // @[TransferSplitter.scala 103:36:@678.10]
  wire [31:0] _GEN_4; // @[TransferSplitter.scala 81:38:@662.8]
  wire [31:0] _GEN_5; // @[TransferSplitter.scala 81:38:@662.8]
  wire [34:0] _GEN_6; // @[TransferSplitter.scala 81:38:@662.8]
  wire  _T_87; // @[Conditional.scala 37:30:@683.8]
  wire  _T_91; // @[TransferSplitter.scala 122:25:@688.12]
  wire [1:0] _GEN_7; // @[TransferSplitter.scala 122:32:@689.12]
  wire  _GEN_8; // @[TransferSplitter.scala 122:32:@689.12]
  wire [1:0] _GEN_9; // @[TransferSplitter.scala 121:31:@687.10]
  wire  _GEN_10; // @[TransferSplitter.scala 121:31:@687.10]
  wire  _GEN_11; // @[Conditional.scala 39:67:@684.8]
  wire [1:0] _GEN_13; // @[Conditional.scala 39:67:@684.8]
  wire  _GEN_14; // @[Conditional.scala 39:67:@684.8]
  wire [31:0] _GEN_15; // @[Conditional.scala 39:67:@656.6]
  wire  _GEN_17; // @[Conditional.scala 39:67:@656.6]
  wire [1:0] _GEN_18; // @[Conditional.scala 39:67:@656.6]
  wire [31:0] _GEN_19; // @[Conditional.scala 39:67:@656.6]
  wire [31:0] _GEN_20; // @[Conditional.scala 39:67:@656.6]
  wire [34:0] _GEN_21; // @[Conditional.scala 39:67:@656.6]
  wire  _GEN_23; // @[Conditional.scala 39:67:@656.6]
  wire  _GEN_24; // @[Conditional.scala 40:58:@645.4]
  wire [34:0] _GEN_25; // @[Conditional.scala 40:58:@645.4]
  wire [31:0] _GEN_26; // @[Conditional.scala 40:58:@645.4]
  wire [1:0] _GEN_28; // @[Conditional.scala 40:58:@645.4]
  wire [31:0] _GEN_29; // @[Conditional.scala 40:58:@645.4]
  wire  _GEN_31; // @[Conditional.scala 40:58:@645.4]
  wire [31:0] _GEN_32; // @[Conditional.scala 40:58:@645.4]
  assign _T_66 = 2'h0 == _T_65; // @[Conditional.scala 37:30:@644.4]
  assign _GEN_0 = io_xferIn_valid ? io_xferIn_address : _T_42; // @[TransferSplitter.scala 68:31:@647.6]
  assign _GEN_1 = io_xferIn_valid ? io_xferIn_length : _T_45; // @[TransferSplitter.scala 68:31:@647.6]
  assign _GEN_3 = io_xferIn_valid ? 2'h1 : _T_65; // @[TransferSplitter.scala 68:31:@647.6]
  assign _T_68 = 2'h1 == _T_65; // @[Conditional.scala 37:30:@655.6]
  assign _T_71 = _T_45 > 32'h100; // @[TransferSplitter.scala 81:23:@661.8]
  assign _T_74 = _T_45 - 32'h100; // @[TransferSplitter.scala 84:34:@664.10]
  assign _T_75 = $unsigned(_T_74); // @[TransferSplitter.scala 84:34:@665.10]
  assign _T_76 = _T_75[31:0]; // @[TransferSplitter.scala 84:34:@666.10]
  assign _T_79 = 9'h100 * 9'h4; // @[TransferSplitter.scala 85:50:@668.10]
  assign _GEN_33 = {{20'd0}, _T_79}; // @[TransferSplitter.scala 85:36:@669.10]
  assign _T_80 = _T_42 + _GEN_33; // @[TransferSplitter.scala 85:36:@669.10]
  assign _T_81 = _T_42 + _GEN_33; // @[TransferSplitter.scala 85:36:@670.10]
  assign _T_84 = _T_45 * 32'h4; // @[TransferSplitter.scala 103:47:@676.10]
  assign _GEN_34 = {{3'd0}, _T_42}; // @[TransferSplitter.scala 103:36:@677.10]
  assign _T_85 = _GEN_34 + _T_84; // @[TransferSplitter.scala 103:36:@677.10]
  assign _T_86 = _GEN_34 + _T_84; // @[TransferSplitter.scala 103:36:@678.10]
  assign _GEN_4 = _T_71 ? 32'h100 : _T_45; // @[TransferSplitter.scala 81:38:@662.8]
  assign _GEN_5 = _T_71 ? _T_76 : 32'h0; // @[TransferSplitter.scala 81:38:@662.8]
  assign _GEN_6 = _T_71 ? {{3'd0}, _T_81} : _T_86; // @[TransferSplitter.scala 81:38:@662.8]
  assign _T_87 = 2'h2 == _T_65; // @[Conditional.scala 37:30:@683.8]
  assign _T_91 = _T_45 > 32'h0; // @[TransferSplitter.scala 122:25:@688.12]
  assign _GEN_7 = _T_91 ? 2'h1 : 2'h0; // @[TransferSplitter.scala 122:32:@689.12]
  assign _GEN_8 = _T_91 ? _T_60 : 1'h1; // @[TransferSplitter.scala 122:32:@689.12]
  assign _GEN_9 = io_xferOut_done ? _GEN_7 : _T_65; // @[TransferSplitter.scala 121:31:@687.10]
  assign _GEN_10 = io_xferOut_done ? _GEN_8 : _T_60; // @[TransferSplitter.scala 121:31:@687.10]
  assign _GEN_11 = _T_87 ? 1'h0 : _T_63; // @[Conditional.scala 39:67:@684.8]
  assign _GEN_13 = _T_87 ? _GEN_9 : _T_65; // @[Conditional.scala 39:67:@684.8]
  assign _GEN_14 = _T_87 ? _GEN_10 : _T_60; // @[Conditional.scala 39:67:@684.8]
  assign _GEN_15 = _T_68 ? _T_42 : _T_48; // @[Conditional.scala 39:67:@656.6]
  assign _GEN_17 = _T_68 ? 1'h1 : _GEN_11; // @[Conditional.scala 39:67:@656.6]
  assign _GEN_18 = _T_68 ? 2'h2 : _GEN_13; // @[Conditional.scala 39:67:@656.6]
  assign _GEN_19 = _T_68 ? _GEN_4 : _T_51; // @[Conditional.scala 39:67:@656.6]
  assign _GEN_20 = _T_68 ? _GEN_5 : _T_45; // @[Conditional.scala 39:67:@656.6]
  assign _GEN_21 = _T_68 ? _GEN_6 : {{3'd0}, _T_42}; // @[Conditional.scala 39:67:@656.6]
  assign _GEN_23 = _T_68 ? _T_60 : _GEN_14; // @[Conditional.scala 39:67:@656.6]
  assign _GEN_24 = _T_66 ? 1'h0 : _GEN_23; // @[Conditional.scala 40:58:@645.4]
  assign _GEN_25 = _T_66 ? {{3'd0}, _GEN_0} : _GEN_21; // @[Conditional.scala 40:58:@645.4]
  assign _GEN_26 = _T_66 ? _GEN_1 : _GEN_20; // @[Conditional.scala 40:58:@645.4]
  assign _GEN_28 = _T_66 ? _GEN_3 : _GEN_18; // @[Conditional.scala 40:58:@645.4]
  assign _GEN_29 = _T_66 ? _T_48 : _GEN_15; // @[Conditional.scala 40:58:@645.4]
  assign _GEN_31 = _T_66 ? _T_63 : _GEN_17; // @[Conditional.scala 40:58:@645.4]
  assign _GEN_32 = _T_66 ? _T_51 : _GEN_19; // @[Conditional.scala 40:58:@645.4]
  assign io_xferIn_done = _T_60; // @[TransferSplitter.scala 57:20:@639.4]
  assign io_xferOut_address = _T_48; // @[TransferSplitter.scala 61:24:@642.4]
  assign io_xferOut_length = _T_51; // @[TransferSplitter.scala 62:23:@643.4]
  assign io_xferOut_valid = _T_63; // @[TransferSplitter.scala 58:22:@640.4]
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE
  integer initvar;
  initial begin
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      #0.002 begin end
    `endif
  `ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  _T_42 = _RAND_0[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  _T_45 = _RAND_1[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_2 = {1{`RANDOM}};
  _T_48 = _RAND_2[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_3 = {1{`RANDOM}};
  _T_51 = _RAND_3[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_4 = {1{`RANDOM}};
  _T_60 = _RAND_4[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_5 = {1{`RANDOM}};
  _T_63 = _RAND_5[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_6 = {1{`RANDOM}};
  _T_65 = _RAND_6[1:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if (reset) begin
      _T_42 <= 32'h0;
    end else begin
      _T_42 <= _GEN_25[31:0];
    end
    if (reset) begin
      _T_45 <= 32'h0;
    end else begin
      if (_T_66) begin
        if (io_xferIn_valid) begin
          _T_45 <= io_xferIn_length;
        end
      end else begin
        if (_T_68) begin
          if (_T_71) begin
            _T_45 <= _T_76;
          end else begin
            _T_45 <= 32'h0;
          end
        end
      end
    end
    if (reset) begin
      _T_48 <= 32'h0;
    end else begin
      if (!(_T_66)) begin
        if (_T_68) begin
          _T_48 <= _T_42;
        end
      end
    end
    if (reset) begin
      _T_51 <= 32'h0;
    end else begin
      if (!(_T_66)) begin
        if (_T_68) begin
          if (_T_71) begin
            _T_51 <= 32'h100;
          end else begin
            _T_51 <= _T_45;
          end
        end
      end
    end
    if (reset) begin
      _T_60 <= 1'h0;
    end else begin
      if (_T_66) begin
        _T_60 <= 1'h0;
      end else begin
        if (!(_T_68)) begin
          if (_T_87) begin
            if (io_xferOut_done) begin
              if (!(_T_91)) begin
                _T_60 <= 1'h1;
              end
            end
          end
        end
      end
    end
    if (reset) begin
      _T_63 <= 1'h0;
    end else begin
      if (!(_T_66)) begin
        if (_T_68) begin
          _T_63 <= 1'h1;
        end else begin
          if (_T_87) begin
            _T_63 <= 1'h0;
          end
        end
      end
    end
    if (reset) begin
      _T_65 <= 2'h0;
    end else begin
      if (_T_66) begin
        if (io_xferIn_valid) begin
          _T_65 <= 2'h1;
        end
      end else begin
        if (_T_68) begin
          _T_65 <= 2'h2;
        end else begin
          if (_T_87) begin
            if (io_xferOut_done) begin
              if (_T_91) begin
                _T_65 <= 2'h1;
              end else begin
                _T_65 <= 2'h0;
              end
            end
          end
        end
      end
    end
  end
endmodule
module AXIS_to_WB_ClearCSR( // @[:@699.2]
  input         clock, // @[:@700.4]
  input         reset, // @[:@701.4]
  input  [31:0] io_csr_dataOut, // @[:@702.4]
  input         io_csr_dataWrite, // @[:@702.4]
  output [31:0] io_csr_dataIn, // @[:@702.4]
  output [31:0] io_value, // @[:@702.4]
  input  [31:0] io_clear // @[:@702.4]
);
  reg [31:0] reg$; // @[ClearCSR.scala 36:20:@704.4]
  reg [31:0] _RAND_0;
  wire [31:0] _T_29; // @[ClearCSR.scala 44:19:@711.6]
  wire [31:0] _T_30; // @[ClearCSR.scala 44:16:@712.6]
  wire [31:0] _GEN_0; // @[ClearCSR.scala 41:25:@707.4]
  assign _T_29 = ~ io_clear; // @[ClearCSR.scala 44:19:@711.6]
  assign _T_30 = reg$ & _T_29; // @[ClearCSR.scala 44:16:@712.6]
  assign _GEN_0 = io_csr_dataWrite ? io_csr_dataOut : _T_30; // @[ClearCSR.scala 41:25:@707.4]
  assign io_csr_dataIn = reg$; // @[ClearCSR.scala 38:17:@705.4]
  assign io_value = reg$; // @[ClearCSR.scala 39:12:@706.4]
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE
  integer initvar;
  initial begin
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      #0.002 begin end
    `endif
  `ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  reg$ = _RAND_0[31:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if (reset) begin
      reg$ <= 32'h0;
    end else begin
      if (io_csr_dataWrite) begin
        reg$ <= io_csr_dataOut;
      end else begin
        reg$ <= _T_30;
      end
    end
  end
endmodule
module AXIS_to_WB_StatusCSR( // @[:@716.2]
  input         clock, // @[:@717.4]
  output [31:0] io_csr_dataIn, // @[:@719.4]
  input  [31:0] io_value // @[:@719.4]
);
  reg [31:0] reg$; // @[StatusCSR.scala 35:20:@721.4]
  reg [31:0] _RAND_0;
  assign io_csr_dataIn = reg$; // @[StatusCSR.scala 37:17:@723.4]
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE
  integer initvar;
  initial begin
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      #0.002 begin end
    `endif
  `ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  reg$ = _RAND_0[31:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    reg$ <= io_value;
  end
endmodule
module AXIS_to_WB_SimpleCSR( // @[:@725.2]
  input         clock, // @[:@726.4]
  input         reset, // @[:@727.4]
  input  [31:0] io_csr_dataOut, // @[:@728.4]
  input         io_csr_dataWrite, // @[:@728.4]
  output [31:0] io_csr_dataIn, // @[:@728.4]
  output [31:0] io_value // @[:@728.4]
);
  reg [31:0] reg$; // @[SimpleCSR.scala 35:20:@730.4]
  reg [31:0] _RAND_0;
  wire [31:0] _GEN_0; // @[SimpleCSR.scala 40:25:@733.4]
  assign _GEN_0 = io_csr_dataWrite ? io_csr_dataOut : reg$; // @[SimpleCSR.scala 40:25:@733.4]
  assign io_csr_dataIn = reg$; // @[SimpleCSR.scala 37:17:@731.4]
  assign io_value = reg$; // @[SimpleCSR.scala 38:12:@732.4]
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE
  integer initvar;
  initial begin
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      #0.002 begin end
    `endif
  `ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  reg$ = _RAND_0[31:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if (reset) begin
      reg$ <= 32'h0;
    end else begin
      if (io_csr_dataWrite) begin
        reg$ <= io_csr_dataOut;
      end
    end
  end
endmodule
module AXIS_to_WB_SetCSR( // @[:@737.2]
  input         clock, // @[:@738.4]
  input         reset, // @[:@739.4]
  input  [31:0] io_csr_dataOut, // @[:@740.4]
  input         io_csr_dataWrite, // @[:@740.4]
  output [31:0] io_csr_dataIn, // @[:@740.4]
  output [31:0] io_value, // @[:@740.4]
  input  [31:0] io_set // @[:@740.4]
);
  reg [31:0] reg$; // @[SetCSR.scala 36:20:@742.4]
  reg [31:0] _RAND_0;
  wire [31:0] _T_29; // @[SetCSR.scala 42:20:@746.6]
  wire [31:0] _T_30; // @[SetCSR.scala 42:17:@747.6]
  wire [31:0] _T_31; // @[SetCSR.scala 42:45:@748.6]
  wire [31:0] _T_32; // @[SetCSR.scala 44:16:@752.6]
  wire [31:0] _GEN_0; // @[SetCSR.scala 41:25:@745.4]
  assign _T_29 = ~ io_csr_dataOut; // @[SetCSR.scala 42:20:@746.6]
  assign _T_30 = reg$ & _T_29; // @[SetCSR.scala 42:17:@747.6]
  assign _T_31 = _T_30 | io_set; // @[SetCSR.scala 42:45:@748.6]
  assign _T_32 = reg$ | io_set; // @[SetCSR.scala 44:16:@752.6]
  assign _GEN_0 = io_csr_dataWrite ? _T_31 : _T_32; // @[SetCSR.scala 41:25:@745.4]
  assign io_csr_dataIn = reg$; // @[SetCSR.scala 38:17:@743.4]
  assign io_value = reg$; // @[SetCSR.scala 39:12:@744.4]
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE
  integer initvar;
  initial begin
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      #0.002 begin end
    `endif
  `ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  reg$ = _RAND_0[31:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if (reset) begin
      reg$ <= 32'h0;
    end else begin
      if (io_csr_dataWrite) begin
        reg$ <= _T_31;
      end else begin
        reg$ <= _T_32;
      end
    end
  end
endmodule
module AXIS_to_WB_InterruptController( // @[:@756.2]
  input         clock, // @[:@757.4]
  input         reset, // @[:@758.4]
  output        io_irq_readerDone, // @[:@759.4]
  output        io_irq_writerDone, // @[:@759.4]
  input         io_readBusy, // @[:@759.4]
  input         io_writeBusy, // @[:@759.4]
  input  [31:0] io_imr_dataOut, // @[:@759.4]
  input         io_imr_dataWrite, // @[:@759.4]
  output [31:0] io_imr_dataIn, // @[:@759.4]
  input  [31:0] io_isr_dataOut, // @[:@759.4]
  input         io_isr_dataWrite, // @[:@759.4]
  output [31:0] io_isr_dataIn // @[:@759.4]
);
  wire  SimpleCSR_clock; // @[SimpleCSR.scala 48:21:@761.4]
  wire  SimpleCSR_reset; // @[SimpleCSR.scala 48:21:@761.4]
  wire [31:0] SimpleCSR_io_csr_dataOut; // @[SimpleCSR.scala 48:21:@761.4]
  wire  SimpleCSR_io_csr_dataWrite; // @[SimpleCSR.scala 48:21:@761.4]
  wire [31:0] SimpleCSR_io_csr_dataIn; // @[SimpleCSR.scala 48:21:@761.4]
  wire [31:0] SimpleCSR_io_value; // @[SimpleCSR.scala 48:21:@761.4]
  wire  SetCSR_clock; // @[SetCSR.scala 51:21:@793.4]
  wire  SetCSR_reset; // @[SetCSR.scala 51:21:@793.4]
  wire [31:0] SetCSR_io_csr_dataOut; // @[SetCSR.scala 51:21:@793.4]
  wire  SetCSR_io_csr_dataWrite; // @[SetCSR.scala 51:21:@793.4]
  wire [31:0] SetCSR_io_csr_dataIn; // @[SetCSR.scala 51:21:@793.4]
  wire [31:0] SetCSR_io_value; // @[SetCSR.scala 51:21:@793.4]
  wire [31:0] SetCSR_io_set; // @[SetCSR.scala 51:21:@793.4]
  reg  readBusy; // @[InterruptController.scala 41:25:@770.4]
  reg [31:0] _RAND_0;
  reg  readBusyOld; // @[InterruptController.scala 42:28:@772.4]
  reg [31:0] _RAND_1;
  reg  writeBusy; // @[InterruptController.scala 44:26:@774.4]
  reg [31:0] _RAND_2;
  reg  writeBusyOld; // @[InterruptController.scala 45:29:@776.4]
  reg [31:0] _RAND_3;
  reg  writeBusyIrq; // @[InterruptController.scala 47:29:@778.4]
  reg [31:0] _RAND_4;
  reg  readBusyIrq; // @[InterruptController.scala 48:28:@779.4]
  reg [31:0] _RAND_5;
  wire  _T_59; // @[InterruptController.scala 50:35:@780.4]
  wire  _T_60; // @[InterruptController.scala 50:32:@781.4]
  wire [31:0] mask; // @[:@768.4 :@769.4]
  wire  _T_61; // @[InterruptController.scala 50:53:@782.4]
  wire  _T_62; // @[InterruptController.scala 50:46:@783.4]
  wire  _T_64; // @[InterruptController.scala 51:33:@785.4]
  wire  _T_65; // @[InterruptController.scala 51:30:@786.4]
  wire  _T_66; // @[InterruptController.scala 51:50:@787.4]
  wire  _T_67; // @[InterruptController.scala 51:43:@788.4]
  wire [1:0] irq; // @[Cat.scala 30:58:@790.4]
  wire [31:0] isr; // @[:@801.4 :@802.4]
  AXIS_to_WB_SimpleCSR SimpleCSR ( // @[SimpleCSR.scala 48:21:@761.4]
    .clock(SimpleCSR_clock),
    .reset(SimpleCSR_reset),
    .io_csr_dataOut(SimpleCSR_io_csr_dataOut),
    .io_csr_dataWrite(SimpleCSR_io_csr_dataWrite),
    .io_csr_dataIn(SimpleCSR_io_csr_dataIn),
    .io_value(SimpleCSR_io_value)
  );
  AXIS_to_WB_SetCSR SetCSR ( // @[SetCSR.scala 51:21:@793.4]
    .clock(SetCSR_clock),
    .reset(SetCSR_reset),
    .io_csr_dataOut(SetCSR_io_csr_dataOut),
    .io_csr_dataWrite(SetCSR_io_csr_dataWrite),
    .io_csr_dataIn(SetCSR_io_csr_dataIn),
    .io_value(SetCSR_io_value),
    .io_set(SetCSR_io_set)
  );
  assign _T_59 = writeBusy == 1'h0; // @[InterruptController.scala 50:35:@780.4]
  assign _T_60 = writeBusyOld & _T_59; // @[InterruptController.scala 50:32:@781.4]
  assign mask = SimpleCSR_io_value; // @[:@768.4 :@769.4]
  assign _T_61 = mask[0]; // @[InterruptController.scala 50:53:@782.4]
  assign _T_62 = _T_60 & _T_61; // @[InterruptController.scala 50:46:@783.4]
  assign _T_64 = readBusy == 1'h0; // @[InterruptController.scala 51:33:@785.4]
  assign _T_65 = readBusyOld & _T_64; // @[InterruptController.scala 51:30:@786.4]
  assign _T_66 = mask[1]; // @[InterruptController.scala 51:50:@787.4]
  assign _T_67 = _T_65 & _T_66; // @[InterruptController.scala 51:43:@788.4]
  assign irq = {readBusyIrq,writeBusyIrq}; // @[Cat.scala 30:58:@790.4]
  assign isr = SetCSR_io_value; // @[:@801.4 :@802.4]
  assign io_irq_readerDone = isr[1]; // @[InterruptController.scala 58:21:@806.4]
  assign io_irq_writerDone = isr[0]; // @[InterruptController.scala 57:21:@804.4]
  assign io_imr_dataIn = SimpleCSR_io_csr_dataIn; // @[SimpleCSR.scala 50:16:@764.4]
  assign io_isr_dataIn = SetCSR_io_csr_dataIn; // @[SetCSR.scala 55:16:@797.4]
  assign SimpleCSR_clock = clock; // @[:@762.4]
  assign SimpleCSR_reset = reset; // @[:@763.4]
  assign SimpleCSR_io_csr_dataOut = io_imr_dataOut; // @[SimpleCSR.scala 50:16:@766.4]
  assign SimpleCSR_io_csr_dataWrite = io_imr_dataWrite; // @[SimpleCSR.scala 50:16:@765.4]
  assign SetCSR_clock = clock; // @[:@794.4]
  assign SetCSR_reset = reset; // @[:@795.4]
  assign SetCSR_io_csr_dataOut = io_isr_dataOut; // @[SetCSR.scala 55:16:@799.4]
  assign SetCSR_io_csr_dataWrite = io_isr_dataWrite; // @[SetCSR.scala 55:16:@798.4]
  assign SetCSR_io_set = {{30'd0}, irq}; // @[SetCSR.scala 53:16:@796.4]
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE
  integer initvar;
  initial begin
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      #0.002 begin end
    `endif
  `ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  readBusy = _RAND_0[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  readBusyOld = _RAND_1[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_2 = {1{`RANDOM}};
  writeBusy = _RAND_2[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_3 = {1{`RANDOM}};
  writeBusyOld = _RAND_3[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_4 = {1{`RANDOM}};
  writeBusyIrq = _RAND_4[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_5 = {1{`RANDOM}};
  readBusyIrq = _RAND_5[0:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    readBusy <= io_readBusy;
    readBusyOld <= readBusy;
    writeBusy <= io_writeBusy;
    writeBusyOld <= writeBusy;
    if (reset) begin
      writeBusyIrq <= 1'h0;
    end else begin
      writeBusyIrq <= _T_62;
    end
    if (reset) begin
      readBusyIrq <= 1'h0;
    end else begin
      readBusyIrq <= _T_67;
    end
  end
endmodule
module AXIS_to_WB_WorkerCSRWrapper( // @[:@952.2]
  input         clock, // @[:@953.4]
  input         reset, // @[:@954.4]
  input  [31:0] io_csr_0_dataOut, // @[:@955.4]
  input         io_csr_0_dataWrite, // @[:@955.4]
  output [31:0] io_csr_0_dataIn, // @[:@955.4]
  output [31:0] io_csr_1_dataIn, // @[:@955.4]
  input  [31:0] io_csr_2_dataOut, // @[:@955.4]
  input         io_csr_2_dataWrite, // @[:@955.4]
  output [31:0] io_csr_2_dataIn, // @[:@955.4]
  input  [31:0] io_csr_3_dataOut, // @[:@955.4]
  input         io_csr_3_dataWrite, // @[:@955.4]
  output [31:0] io_csr_3_dataIn, // @[:@955.4]
  input  [31:0] io_csr_4_dataOut, // @[:@955.4]
  input         io_csr_4_dataWrite, // @[:@955.4]
  output [31:0] io_csr_4_dataIn, // @[:@955.4]
  input  [31:0] io_csr_5_dataOut, // @[:@955.4]
  input         io_csr_5_dataWrite, // @[:@955.4]
  output [31:0] io_csr_5_dataIn, // @[:@955.4]
  input  [31:0] io_csr_6_dataOut, // @[:@955.4]
  input         io_csr_6_dataWrite, // @[:@955.4]
  output [31:0] io_csr_6_dataIn, // @[:@955.4]
  input  [31:0] io_csr_7_dataOut, // @[:@955.4]
  input         io_csr_7_dataWrite, // @[:@955.4]
  output [31:0] io_csr_7_dataIn, // @[:@955.4]
  input  [31:0] io_csr_8_dataOut, // @[:@955.4]
  input         io_csr_8_dataWrite, // @[:@955.4]
  output [31:0] io_csr_8_dataIn, // @[:@955.4]
  input  [31:0] io_csr_9_dataOut, // @[:@955.4]
  input         io_csr_9_dataWrite, // @[:@955.4]
  output [31:0] io_csr_9_dataIn, // @[:@955.4]
  input  [31:0] io_csr_10_dataOut, // @[:@955.4]
  input         io_csr_10_dataWrite, // @[:@955.4]
  output [31:0] io_csr_10_dataIn, // @[:@955.4]
  input  [31:0] io_csr_11_dataOut, // @[:@955.4]
  input         io_csr_11_dataWrite, // @[:@955.4]
  output [31:0] io_csr_11_dataIn, // @[:@955.4]
  input  [31:0] io_csr_12_dataOut, // @[:@955.4]
  input         io_csr_12_dataWrite, // @[:@955.4]
  output [31:0] io_csr_12_dataIn, // @[:@955.4]
  input  [31:0] io_csr_13_dataOut, // @[:@955.4]
  input         io_csr_13_dataWrite, // @[:@955.4]
  output [31:0] io_csr_13_dataIn, // @[:@955.4]
  input  [31:0] io_csr_14_dataOut, // @[:@955.4]
  input         io_csr_14_dataWrite, // @[:@955.4]
  output [31:0] io_csr_14_dataIn, // @[:@955.4]
  input  [31:0] io_csr_15_dataOut, // @[:@955.4]
  input         io_csr_15_dataWrite, // @[:@955.4]
  output [31:0] io_csr_15_dataIn, // @[:@955.4]
  output        io_irq_readerDone, // @[:@955.4]
  output        io_irq_writerDone, // @[:@955.4]
  input         io_sync_readerSync, // @[:@955.4]
  input         io_sync_writerSync, // @[:@955.4]
  input         io_xferRead_done, // @[:@955.4]
  output [31:0] io_xferRead_length, // @[:@955.4]
  output        io_xferRead_valid, // @[:@955.4]
  input         io_xferWrite_done, // @[:@955.4]
  output [31:0] io_xferWrite_address, // @[:@955.4]
  output [31:0] io_xferWrite_length, // @[:@955.4]
  output        io_xferWrite_valid // @[:@955.4]
);
  wire  addressGeneratorRead_clock; // @[WorkerCSRWrapper.scala 41:36:@957.4]
  wire  addressGeneratorRead_reset; // @[WorkerCSRWrapper.scala 41:36:@957.4]
  wire  addressGeneratorRead_io_ctl_start; // @[WorkerCSRWrapper.scala 41:36:@957.4]
  wire  addressGeneratorRead_io_ctl_busy; // @[WorkerCSRWrapper.scala 41:36:@957.4]
  wire [31:0] addressGeneratorRead_io_ctl_startAddress; // @[WorkerCSRWrapper.scala 41:36:@957.4]
  wire [31:0] addressGeneratorRead_io_ctl_lineLength; // @[WorkerCSRWrapper.scala 41:36:@957.4]
  wire [31:0] addressGeneratorRead_io_ctl_lineCount; // @[WorkerCSRWrapper.scala 41:36:@957.4]
  wire [31:0] addressGeneratorRead_io_ctl_lineGap; // @[WorkerCSRWrapper.scala 41:36:@957.4]
  wire  addressGeneratorRead_io_xfer_done; // @[WorkerCSRWrapper.scala 41:36:@957.4]
  wire [31:0] addressGeneratorRead_io_xfer_address; // @[WorkerCSRWrapper.scala 41:36:@957.4]
  wire [31:0] addressGeneratorRead_io_xfer_length; // @[WorkerCSRWrapper.scala 41:36:@957.4]
  wire  addressGeneratorRead_io_xfer_valid; // @[WorkerCSRWrapper.scala 41:36:@957.4]
  wire  transferSplitterRead_io_xferIn_done; // @[WorkerCSRWrapper.scala 42:36:@960.4]
  wire [31:0] transferSplitterRead_io_xferIn_length; // @[WorkerCSRWrapper.scala 42:36:@960.4]
  wire  transferSplitterRead_io_xferIn_valid; // @[WorkerCSRWrapper.scala 42:36:@960.4]
  wire  transferSplitterRead_io_xferOut_done; // @[WorkerCSRWrapper.scala 42:36:@960.4]
  wire [31:0] transferSplitterRead_io_xferOut_length; // @[WorkerCSRWrapper.scala 42:36:@960.4]
  wire  transferSplitterRead_io_xferOut_valid; // @[WorkerCSRWrapper.scala 42:36:@960.4]
  wire  addressGeneratorWrite_clock; // @[WorkerCSRWrapper.scala 44:37:@963.4]
  wire  addressGeneratorWrite_reset; // @[WorkerCSRWrapper.scala 44:37:@963.4]
  wire  addressGeneratorWrite_io_ctl_start; // @[WorkerCSRWrapper.scala 44:37:@963.4]
  wire  addressGeneratorWrite_io_ctl_busy; // @[WorkerCSRWrapper.scala 44:37:@963.4]
  wire [31:0] addressGeneratorWrite_io_ctl_startAddress; // @[WorkerCSRWrapper.scala 44:37:@963.4]
  wire [31:0] addressGeneratorWrite_io_ctl_lineLength; // @[WorkerCSRWrapper.scala 44:37:@963.4]
  wire [31:0] addressGeneratorWrite_io_ctl_lineCount; // @[WorkerCSRWrapper.scala 44:37:@963.4]
  wire [31:0] addressGeneratorWrite_io_ctl_lineGap; // @[WorkerCSRWrapper.scala 44:37:@963.4]
  wire  addressGeneratorWrite_io_xfer_done; // @[WorkerCSRWrapper.scala 44:37:@963.4]
  wire [31:0] addressGeneratorWrite_io_xfer_address; // @[WorkerCSRWrapper.scala 44:37:@963.4]
  wire [31:0] addressGeneratorWrite_io_xfer_length; // @[WorkerCSRWrapper.scala 44:37:@963.4]
  wire  addressGeneratorWrite_io_xfer_valid; // @[WorkerCSRWrapper.scala 44:37:@963.4]
  wire  transferSplitterWrite_clock; // @[WorkerCSRWrapper.scala 45:37:@966.4]
  wire  transferSplitterWrite_reset; // @[WorkerCSRWrapper.scala 45:37:@966.4]
  wire  transferSplitterWrite_io_xferIn_done; // @[WorkerCSRWrapper.scala 45:37:@966.4]
  wire [31:0] transferSplitterWrite_io_xferIn_address; // @[WorkerCSRWrapper.scala 45:37:@966.4]
  wire [31:0] transferSplitterWrite_io_xferIn_length; // @[WorkerCSRWrapper.scala 45:37:@966.4]
  wire  transferSplitterWrite_io_xferIn_valid; // @[WorkerCSRWrapper.scala 45:37:@966.4]
  wire  transferSplitterWrite_io_xferOut_done; // @[WorkerCSRWrapper.scala 45:37:@966.4]
  wire [31:0] transferSplitterWrite_io_xferOut_address; // @[WorkerCSRWrapper.scala 45:37:@966.4]
  wire [31:0] transferSplitterWrite_io_xferOut_length; // @[WorkerCSRWrapper.scala 45:37:@966.4]
  wire  transferSplitterWrite_io_xferOut_valid; // @[WorkerCSRWrapper.scala 45:37:@966.4]
  wire  ClearCSR_clock; // @[ClearCSR.scala 50:21:@984.4]
  wire  ClearCSR_reset; // @[ClearCSR.scala 50:21:@984.4]
  wire [31:0] ClearCSR_io_csr_dataOut; // @[ClearCSR.scala 50:21:@984.4]
  wire  ClearCSR_io_csr_dataWrite; // @[ClearCSR.scala 50:21:@984.4]
  wire [31:0] ClearCSR_io_csr_dataIn; // @[ClearCSR.scala 50:21:@984.4]
  wire [31:0] ClearCSR_io_value; // @[ClearCSR.scala 50:21:@984.4]
  wire [31:0] ClearCSR_io_clear; // @[ClearCSR.scala 50:21:@984.4]
  wire  StatusCSR_clock; // @[StatusCSR.scala 42:21:@993.4]
  wire [31:0] StatusCSR_io_csr_dataIn; // @[StatusCSR.scala 42:21:@993.4]
  wire [31:0] StatusCSR_io_value; // @[StatusCSR.scala 42:21:@993.4]
  wire  InterruptController_clock; // @[InterruptController.scala 63:22:@1001.4]
  wire  InterruptController_reset; // @[InterruptController.scala 63:22:@1001.4]
  wire  InterruptController_io_irq_readerDone; // @[InterruptController.scala 63:22:@1001.4]
  wire  InterruptController_io_irq_writerDone; // @[InterruptController.scala 63:22:@1001.4]
  wire  InterruptController_io_readBusy; // @[InterruptController.scala 63:22:@1001.4]
  wire  InterruptController_io_writeBusy; // @[InterruptController.scala 63:22:@1001.4]
  wire [31:0] InterruptController_io_imr_dataOut; // @[InterruptController.scala 63:22:@1001.4]
  wire  InterruptController_io_imr_dataWrite; // @[InterruptController.scala 63:22:@1001.4]
  wire [31:0] InterruptController_io_imr_dataIn; // @[InterruptController.scala 63:22:@1001.4]
  wire [31:0] InterruptController_io_isr_dataOut; // @[InterruptController.scala 63:22:@1001.4]
  wire  InterruptController_io_isr_dataWrite; // @[InterruptController.scala 63:22:@1001.4]
  wire [31:0] InterruptController_io_isr_dataIn; // @[InterruptController.scala 63:22:@1001.4]
  wire  SimpleCSR_clock; // @[SimpleCSR.scala 48:21:@1038.4]
  wire  SimpleCSR_reset; // @[SimpleCSR.scala 48:21:@1038.4]
  wire [31:0] SimpleCSR_io_csr_dataOut; // @[SimpleCSR.scala 48:21:@1038.4]
  wire  SimpleCSR_io_csr_dataWrite; // @[SimpleCSR.scala 48:21:@1038.4]
  wire [31:0] SimpleCSR_io_csr_dataIn; // @[SimpleCSR.scala 48:21:@1038.4]
  wire [31:0] SimpleCSR_io_value; // @[SimpleCSR.scala 48:21:@1038.4]
  wire  SimpleCSR_1_clock; // @[SimpleCSR.scala 48:21:@1046.4]
  wire  SimpleCSR_1_reset; // @[SimpleCSR.scala 48:21:@1046.4]
  wire [31:0] SimpleCSR_1_io_csr_dataOut; // @[SimpleCSR.scala 48:21:@1046.4]
  wire  SimpleCSR_1_io_csr_dataWrite; // @[SimpleCSR.scala 48:21:@1046.4]
  wire [31:0] SimpleCSR_1_io_csr_dataIn; // @[SimpleCSR.scala 48:21:@1046.4]
  wire [31:0] SimpleCSR_1_io_value; // @[SimpleCSR.scala 48:21:@1046.4]
  wire  SimpleCSR_2_clock; // @[SimpleCSR.scala 48:21:@1054.4]
  wire  SimpleCSR_2_reset; // @[SimpleCSR.scala 48:21:@1054.4]
  wire [31:0] SimpleCSR_2_io_csr_dataOut; // @[SimpleCSR.scala 48:21:@1054.4]
  wire  SimpleCSR_2_io_csr_dataWrite; // @[SimpleCSR.scala 48:21:@1054.4]
  wire [31:0] SimpleCSR_2_io_csr_dataIn; // @[SimpleCSR.scala 48:21:@1054.4]
  wire [31:0] SimpleCSR_2_io_value; // @[SimpleCSR.scala 48:21:@1054.4]
  wire  SimpleCSR_3_clock; // @[SimpleCSR.scala 48:21:@1062.4]
  wire  SimpleCSR_3_reset; // @[SimpleCSR.scala 48:21:@1062.4]
  wire [31:0] SimpleCSR_3_io_csr_dataOut; // @[SimpleCSR.scala 48:21:@1062.4]
  wire  SimpleCSR_3_io_csr_dataWrite; // @[SimpleCSR.scala 48:21:@1062.4]
  wire [31:0] SimpleCSR_3_io_csr_dataIn; // @[SimpleCSR.scala 48:21:@1062.4]
  wire [31:0] SimpleCSR_3_io_value; // @[SimpleCSR.scala 48:21:@1062.4]
  wire  SimpleCSR_4_clock; // @[SimpleCSR.scala 48:21:@1071.4]
  wire  SimpleCSR_4_reset; // @[SimpleCSR.scala 48:21:@1071.4]
  wire [31:0] SimpleCSR_4_io_csr_dataOut; // @[SimpleCSR.scala 48:21:@1071.4]
  wire  SimpleCSR_4_io_csr_dataWrite; // @[SimpleCSR.scala 48:21:@1071.4]
  wire [31:0] SimpleCSR_4_io_csr_dataIn; // @[SimpleCSR.scala 48:21:@1071.4]
  wire [31:0] SimpleCSR_4_io_value; // @[SimpleCSR.scala 48:21:@1071.4]
  wire  SimpleCSR_5_clock; // @[SimpleCSR.scala 48:21:@1079.4]
  wire  SimpleCSR_5_reset; // @[SimpleCSR.scala 48:21:@1079.4]
  wire [31:0] SimpleCSR_5_io_csr_dataOut; // @[SimpleCSR.scala 48:21:@1079.4]
  wire  SimpleCSR_5_io_csr_dataWrite; // @[SimpleCSR.scala 48:21:@1079.4]
  wire [31:0] SimpleCSR_5_io_csr_dataIn; // @[SimpleCSR.scala 48:21:@1079.4]
  wire [31:0] SimpleCSR_5_io_value; // @[SimpleCSR.scala 48:21:@1079.4]
  wire  SimpleCSR_6_clock; // @[SimpleCSR.scala 48:21:@1087.4]
  wire  SimpleCSR_6_reset; // @[SimpleCSR.scala 48:21:@1087.4]
  wire [31:0] SimpleCSR_6_io_csr_dataOut; // @[SimpleCSR.scala 48:21:@1087.4]
  wire  SimpleCSR_6_io_csr_dataWrite; // @[SimpleCSR.scala 48:21:@1087.4]
  wire [31:0] SimpleCSR_6_io_csr_dataIn; // @[SimpleCSR.scala 48:21:@1087.4]
  wire [31:0] SimpleCSR_6_io_value; // @[SimpleCSR.scala 48:21:@1087.4]
  wire  SimpleCSR_7_clock; // @[SimpleCSR.scala 48:21:@1095.4]
  wire  SimpleCSR_7_reset; // @[SimpleCSR.scala 48:21:@1095.4]
  wire [31:0] SimpleCSR_7_io_csr_dataOut; // @[SimpleCSR.scala 48:21:@1095.4]
  wire  SimpleCSR_7_io_csr_dataWrite; // @[SimpleCSR.scala 48:21:@1095.4]
  wire [31:0] SimpleCSR_7_io_csr_dataIn; // @[SimpleCSR.scala 48:21:@1095.4]
  wire [31:0] SimpleCSR_7_io_value; // @[SimpleCSR.scala 48:21:@1095.4]
  wire  SimpleCSR_8_clock; // @[SimpleCSR.scala 48:21:@1103.4]
  wire  SimpleCSR_8_reset; // @[SimpleCSR.scala 48:21:@1103.4]
  wire [31:0] SimpleCSR_8_io_csr_dataOut; // @[SimpleCSR.scala 48:21:@1103.4]
  wire  SimpleCSR_8_io_csr_dataWrite; // @[SimpleCSR.scala 48:21:@1103.4]
  wire [31:0] SimpleCSR_8_io_csr_dataIn; // @[SimpleCSR.scala 48:21:@1103.4]
  wire [31:0] SimpleCSR_8_io_value; // @[SimpleCSR.scala 48:21:@1103.4]
  wire  SimpleCSR_9_clock; // @[SimpleCSR.scala 48:21:@1110.4]
  wire  SimpleCSR_9_reset; // @[SimpleCSR.scala 48:21:@1110.4]
  wire [31:0] SimpleCSR_9_io_csr_dataOut; // @[SimpleCSR.scala 48:21:@1110.4]
  wire  SimpleCSR_9_io_csr_dataWrite; // @[SimpleCSR.scala 48:21:@1110.4]
  wire [31:0] SimpleCSR_9_io_csr_dataIn; // @[SimpleCSR.scala 48:21:@1110.4]
  wire [31:0] SimpleCSR_9_io_value; // @[SimpleCSR.scala 48:21:@1110.4]
  wire  SimpleCSR_10_clock; // @[SimpleCSR.scala 48:21:@1117.4]
  wire  SimpleCSR_10_reset; // @[SimpleCSR.scala 48:21:@1117.4]
  wire [31:0] SimpleCSR_10_io_csr_dataOut; // @[SimpleCSR.scala 48:21:@1117.4]
  wire  SimpleCSR_10_io_csr_dataWrite; // @[SimpleCSR.scala 48:21:@1117.4]
  wire [31:0] SimpleCSR_10_io_csr_dataIn; // @[SimpleCSR.scala 48:21:@1117.4]
  wire [31:0] SimpleCSR_10_io_value; // @[SimpleCSR.scala 48:21:@1117.4]
  wire  SimpleCSR_11_clock; // @[SimpleCSR.scala 48:21:@1124.4]
  wire  SimpleCSR_11_reset; // @[SimpleCSR.scala 48:21:@1124.4]
  wire [31:0] SimpleCSR_11_io_csr_dataOut; // @[SimpleCSR.scala 48:21:@1124.4]
  wire  SimpleCSR_11_io_csr_dataWrite; // @[SimpleCSR.scala 48:21:@1124.4]
  wire [31:0] SimpleCSR_11_io_csr_dataIn; // @[SimpleCSR.scala 48:21:@1124.4]
  wire [31:0] SimpleCSR_11_io_value; // @[SimpleCSR.scala 48:21:@1124.4]
  reg [1:0] status; // @[WorkerCSRWrapper.scala 47:23:@970.4]
  reg [31:0] _RAND_0;
  reg  readerSync; // @[WorkerCSRWrapper.scala 49:27:@972.4]
  reg [31:0] _RAND_1;
  reg  readerSyncOld; // @[WorkerCSRWrapper.scala 50:30:@974.4]
  reg [31:0] _RAND_2;
  reg  writerSync; // @[WorkerCSRWrapper.scala 52:27:@976.4]
  reg [31:0] _RAND_3;
  reg  writerSyncOld; // @[WorkerCSRWrapper.scala 53:30:@978.4]
  reg [31:0] _RAND_4;
  reg  readerStart; // @[WorkerCSRWrapper.scala 55:28:@980.4]
  reg [31:0] _RAND_5;
  reg  writerStart; // @[WorkerCSRWrapper.scala 56:28:@981.4]
  reg [31:0] _RAND_6;
  wire [1:0] _T_203; // @[Cat.scala 30:58:@1016.4]
  wire [31:0] control; // @[WorkerCSRWrapper.scala 58:21:@982.4 WorkerCSRWrapper.scala 61:11:@992.4]
  wire  _T_204; // @[WorkerCSRWrapper.scala 68:56:@1017.4]
  wire  _T_205; // @[WorkerCSRWrapper.scala 68:68:@1018.4]
  wire [1:0] _T_206; // @[Cat.scala 30:58:@1019.4]
  wire [1:0] _T_207; // @[WorkerCSRWrapper.scala 68:44:@1020.4]
  wire [1:0] clear; // @[WorkerCSRWrapper.scala 68:42:@1021.4]
  wire  _T_210; // @[WorkerCSRWrapper.scala 70:20:@1023.4]
  wire  _T_211; // @[WorkerCSRWrapper.scala 70:35:@1024.4]
  wire  _T_212; // @[WorkerCSRWrapper.scala 70:60:@1025.4]
  wire  _T_213; // @[WorkerCSRWrapper.scala 70:50:@1026.4]
  wire  _T_214; // @[WorkerCSRWrapper.scala 70:75:@1027.4]
  wire  _T_215; // @[WorkerCSRWrapper.scala 70:65:@1028.4]
  wire  _T_217; // @[WorkerCSRWrapper.scala 71:20:@1030.4]
  wire  _T_218; // @[WorkerCSRWrapper.scala 71:35:@1031.4]
  wire  _T_219; // @[WorkerCSRWrapper.scala 71:60:@1032.4]
  wire  _T_220; // @[WorkerCSRWrapper.scala 71:50:@1033.4]
  wire  _T_221; // @[WorkerCSRWrapper.scala 71:75:@1034.4]
  wire  _T_222; // @[WorkerCSRWrapper.scala 71:65:@1035.4]
  AXIS_to_WB_AddressGenerator addressGeneratorRead ( // @[WorkerCSRWrapper.scala 41:36:@957.4]
    .clock(addressGeneratorRead_clock),
    .reset(addressGeneratorRead_reset),
    .io_ctl_start(addressGeneratorRead_io_ctl_start),
    .io_ctl_busy(addressGeneratorRead_io_ctl_busy),
    .io_ctl_startAddress(addressGeneratorRead_io_ctl_startAddress),
    .io_ctl_lineLength(addressGeneratorRead_io_ctl_lineLength),
    .io_ctl_lineCount(addressGeneratorRead_io_ctl_lineCount),
    .io_ctl_lineGap(addressGeneratorRead_io_ctl_lineGap),
    .io_xfer_done(addressGeneratorRead_io_xfer_done),
    .io_xfer_address(addressGeneratorRead_io_xfer_address),
    .io_xfer_length(addressGeneratorRead_io_xfer_length),
    .io_xfer_valid(addressGeneratorRead_io_xfer_valid)
  );
  AXIS_to_WB_TransferSplitter transferSplitterRead ( // @[WorkerCSRWrapper.scala 42:36:@960.4]
    .io_xferIn_done(transferSplitterRead_io_xferIn_done),
    .io_xferIn_length(transferSplitterRead_io_xferIn_length),
    .io_xferIn_valid(transferSplitterRead_io_xferIn_valid),
    .io_xferOut_done(transferSplitterRead_io_xferOut_done),
    .io_xferOut_length(transferSplitterRead_io_xferOut_length),
    .io_xferOut_valid(transferSplitterRead_io_xferOut_valid)
  );
  AXIS_to_WB_AddressGenerator addressGeneratorWrite ( // @[WorkerCSRWrapper.scala 44:37:@963.4]
    .clock(addressGeneratorWrite_clock),
    .reset(addressGeneratorWrite_reset),
    .io_ctl_start(addressGeneratorWrite_io_ctl_start),
    .io_ctl_busy(addressGeneratorWrite_io_ctl_busy),
    .io_ctl_startAddress(addressGeneratorWrite_io_ctl_startAddress),
    .io_ctl_lineLength(addressGeneratorWrite_io_ctl_lineLength),
    .io_ctl_lineCount(addressGeneratorWrite_io_ctl_lineCount),
    .io_ctl_lineGap(addressGeneratorWrite_io_ctl_lineGap),
    .io_xfer_done(addressGeneratorWrite_io_xfer_done),
    .io_xfer_address(addressGeneratorWrite_io_xfer_address),
    .io_xfer_length(addressGeneratorWrite_io_xfer_length),
    .io_xfer_valid(addressGeneratorWrite_io_xfer_valid)
  );
  AXIS_to_WB_TransferSplitter_1 transferSplitterWrite ( // @[WorkerCSRWrapper.scala 45:37:@966.4]
    .clock(transferSplitterWrite_clock),
    .reset(transferSplitterWrite_reset),
    .io_xferIn_done(transferSplitterWrite_io_xferIn_done),
    .io_xferIn_address(transferSplitterWrite_io_xferIn_address),
    .io_xferIn_length(transferSplitterWrite_io_xferIn_length),
    .io_xferIn_valid(transferSplitterWrite_io_xferIn_valid),
    .io_xferOut_done(transferSplitterWrite_io_xferOut_done),
    .io_xferOut_address(transferSplitterWrite_io_xferOut_address),
    .io_xferOut_length(transferSplitterWrite_io_xferOut_length),
    .io_xferOut_valid(transferSplitterWrite_io_xferOut_valid)
  );
  AXIS_to_WB_ClearCSR ClearCSR ( // @[ClearCSR.scala 50:21:@984.4]
    .clock(ClearCSR_clock),
    .reset(ClearCSR_reset),
    .io_csr_dataOut(ClearCSR_io_csr_dataOut),
    .io_csr_dataWrite(ClearCSR_io_csr_dataWrite),
    .io_csr_dataIn(ClearCSR_io_csr_dataIn),
    .io_value(ClearCSR_io_value),
    .io_clear(ClearCSR_io_clear)
  );
  AXIS_to_WB_StatusCSR StatusCSR ( // @[StatusCSR.scala 42:21:@993.4]
    .clock(StatusCSR_clock),
    .io_csr_dataIn(StatusCSR_io_csr_dataIn),
    .io_value(StatusCSR_io_value)
  );
  AXIS_to_WB_InterruptController InterruptController ( // @[InterruptController.scala 63:22:@1001.4]
    .clock(InterruptController_clock),
    .reset(InterruptController_reset),
    .io_irq_readerDone(InterruptController_io_irq_readerDone),
    .io_irq_writerDone(InterruptController_io_irq_writerDone),
    .io_readBusy(InterruptController_io_readBusy),
    .io_writeBusy(InterruptController_io_writeBusy),
    .io_imr_dataOut(InterruptController_io_imr_dataOut),
    .io_imr_dataWrite(InterruptController_io_imr_dataWrite),
    .io_imr_dataIn(InterruptController_io_imr_dataIn),
    .io_isr_dataOut(InterruptController_io_isr_dataOut),
    .io_isr_dataWrite(InterruptController_io_isr_dataWrite),
    .io_isr_dataIn(InterruptController_io_isr_dataIn)
  );
  AXIS_to_WB_SimpleCSR SimpleCSR ( // @[SimpleCSR.scala 48:21:@1038.4]
    .clock(SimpleCSR_clock),
    .reset(SimpleCSR_reset),
    .io_csr_dataOut(SimpleCSR_io_csr_dataOut),
    .io_csr_dataWrite(SimpleCSR_io_csr_dataWrite),
    .io_csr_dataIn(SimpleCSR_io_csr_dataIn),
    .io_value(SimpleCSR_io_value)
  );
  AXIS_to_WB_SimpleCSR SimpleCSR_1 ( // @[SimpleCSR.scala 48:21:@1046.4]
    .clock(SimpleCSR_1_clock),
    .reset(SimpleCSR_1_reset),
    .io_csr_dataOut(SimpleCSR_1_io_csr_dataOut),
    .io_csr_dataWrite(SimpleCSR_1_io_csr_dataWrite),
    .io_csr_dataIn(SimpleCSR_1_io_csr_dataIn),
    .io_value(SimpleCSR_1_io_value)
  );
  AXIS_to_WB_SimpleCSR SimpleCSR_2 ( // @[SimpleCSR.scala 48:21:@1054.4]
    .clock(SimpleCSR_2_clock),
    .reset(SimpleCSR_2_reset),
    .io_csr_dataOut(SimpleCSR_2_io_csr_dataOut),
    .io_csr_dataWrite(SimpleCSR_2_io_csr_dataWrite),
    .io_csr_dataIn(SimpleCSR_2_io_csr_dataIn),
    .io_value(SimpleCSR_2_io_value)
  );
  AXIS_to_WB_SimpleCSR SimpleCSR_3 ( // @[SimpleCSR.scala 48:21:@1062.4]
    .clock(SimpleCSR_3_clock),
    .reset(SimpleCSR_3_reset),
    .io_csr_dataOut(SimpleCSR_3_io_csr_dataOut),
    .io_csr_dataWrite(SimpleCSR_3_io_csr_dataWrite),
    .io_csr_dataIn(SimpleCSR_3_io_csr_dataIn),
    .io_value(SimpleCSR_3_io_value)
  );
  AXIS_to_WB_SimpleCSR SimpleCSR_4 ( // @[SimpleCSR.scala 48:21:@1071.4]
    .clock(SimpleCSR_4_clock),
    .reset(SimpleCSR_4_reset),
    .io_csr_dataOut(SimpleCSR_4_io_csr_dataOut),
    .io_csr_dataWrite(SimpleCSR_4_io_csr_dataWrite),
    .io_csr_dataIn(SimpleCSR_4_io_csr_dataIn),
    .io_value(SimpleCSR_4_io_value)
  );
  AXIS_to_WB_SimpleCSR SimpleCSR_5 ( // @[SimpleCSR.scala 48:21:@1079.4]
    .clock(SimpleCSR_5_clock),
    .reset(SimpleCSR_5_reset),
    .io_csr_dataOut(SimpleCSR_5_io_csr_dataOut),
    .io_csr_dataWrite(SimpleCSR_5_io_csr_dataWrite),
    .io_csr_dataIn(SimpleCSR_5_io_csr_dataIn),
    .io_value(SimpleCSR_5_io_value)
  );
  AXIS_to_WB_SimpleCSR SimpleCSR_6 ( // @[SimpleCSR.scala 48:21:@1087.4]
    .clock(SimpleCSR_6_clock),
    .reset(SimpleCSR_6_reset),
    .io_csr_dataOut(SimpleCSR_6_io_csr_dataOut),
    .io_csr_dataWrite(SimpleCSR_6_io_csr_dataWrite),
    .io_csr_dataIn(SimpleCSR_6_io_csr_dataIn),
    .io_value(SimpleCSR_6_io_value)
  );
  AXIS_to_WB_SimpleCSR SimpleCSR_7 ( // @[SimpleCSR.scala 48:21:@1095.4]
    .clock(SimpleCSR_7_clock),
    .reset(SimpleCSR_7_reset),
    .io_csr_dataOut(SimpleCSR_7_io_csr_dataOut),
    .io_csr_dataWrite(SimpleCSR_7_io_csr_dataWrite),
    .io_csr_dataIn(SimpleCSR_7_io_csr_dataIn),
    .io_value(SimpleCSR_7_io_value)
  );
  AXIS_to_WB_SimpleCSR SimpleCSR_8 ( // @[SimpleCSR.scala 48:21:@1103.4]
    .clock(SimpleCSR_8_clock),
    .reset(SimpleCSR_8_reset),
    .io_csr_dataOut(SimpleCSR_8_io_csr_dataOut),
    .io_csr_dataWrite(SimpleCSR_8_io_csr_dataWrite),
    .io_csr_dataIn(SimpleCSR_8_io_csr_dataIn),
    .io_value(SimpleCSR_8_io_value)
  );
  AXIS_to_WB_SimpleCSR SimpleCSR_9 ( // @[SimpleCSR.scala 48:21:@1110.4]
    .clock(SimpleCSR_9_clock),
    .reset(SimpleCSR_9_reset),
    .io_csr_dataOut(SimpleCSR_9_io_csr_dataOut),
    .io_csr_dataWrite(SimpleCSR_9_io_csr_dataWrite),
    .io_csr_dataIn(SimpleCSR_9_io_csr_dataIn),
    .io_value(SimpleCSR_9_io_value)
  );
  AXIS_to_WB_SimpleCSR SimpleCSR_10 ( // @[SimpleCSR.scala 48:21:@1117.4]
    .clock(SimpleCSR_10_clock),
    .reset(SimpleCSR_10_reset),
    .io_csr_dataOut(SimpleCSR_10_io_csr_dataOut),
    .io_csr_dataWrite(SimpleCSR_10_io_csr_dataWrite),
    .io_csr_dataIn(SimpleCSR_10_io_csr_dataIn),
    .io_value(SimpleCSR_10_io_value)
  );
  AXIS_to_WB_SimpleCSR SimpleCSR_11 ( // @[SimpleCSR.scala 48:21:@1124.4]
    .clock(SimpleCSR_11_clock),
    .reset(SimpleCSR_11_reset),
    .io_csr_dataOut(SimpleCSR_11_io_csr_dataOut),
    .io_csr_dataWrite(SimpleCSR_11_io_csr_dataWrite),
    .io_csr_dataIn(SimpleCSR_11_io_csr_dataIn),
    .io_value(SimpleCSR_11_io_value)
  );
  assign _T_203 = {readerStart,writerStart}; // @[Cat.scala 30:58:@1016.4]
  assign control = ClearCSR_io_value; // @[WorkerCSRWrapper.scala 58:21:@982.4 WorkerCSRWrapper.scala 61:11:@992.4]
  assign _T_204 = control[5]; // @[WorkerCSRWrapper.scala 68:56:@1017.4]
  assign _T_205 = control[4]; // @[WorkerCSRWrapper.scala 68:68:@1018.4]
  assign _T_206 = {_T_204,_T_205}; // @[Cat.scala 30:58:@1019.4]
  assign _T_207 = ~ _T_206; // @[WorkerCSRWrapper.scala 68:44:@1020.4]
  assign clear = _T_203 & _T_207; // @[WorkerCSRWrapper.scala 68:42:@1021.4]
  assign _T_210 = readerSyncOld == 1'h0; // @[WorkerCSRWrapper.scala 70:20:@1023.4]
  assign _T_211 = _T_210 & readerSync; // @[WorkerCSRWrapper.scala 70:35:@1024.4]
  assign _T_212 = control[3]; // @[WorkerCSRWrapper.scala 70:60:@1025.4]
  assign _T_213 = _T_211 | _T_212; // @[WorkerCSRWrapper.scala 70:50:@1026.4]
  assign _T_214 = control[1]; // @[WorkerCSRWrapper.scala 70:75:@1027.4]
  assign _T_215 = _T_213 & _T_214; // @[WorkerCSRWrapper.scala 70:65:@1028.4]
  assign _T_217 = writerSyncOld == 1'h0; // @[WorkerCSRWrapper.scala 71:20:@1030.4]
  assign _T_218 = _T_217 & writerSync; // @[WorkerCSRWrapper.scala 71:35:@1031.4]
  assign _T_219 = control[2]; // @[WorkerCSRWrapper.scala 71:60:@1032.4]
  assign _T_220 = _T_218 | _T_219; // @[WorkerCSRWrapper.scala 71:50:@1033.4]
  assign _T_221 = control[0]; // @[WorkerCSRWrapper.scala 71:75:@1034.4]
  assign _T_222 = _T_220 & _T_221; // @[WorkerCSRWrapper.scala 71:65:@1035.4]
  assign io_csr_0_dataIn = ClearCSR_io_csr_dataIn; // @[ClearCSR.scala 54:16:@988.4]
  assign io_csr_1_dataIn = StatusCSR_io_csr_dataIn; // @[StatusCSR.scala 44:16:@996.4]
  assign io_csr_2_dataIn = InterruptController_io_imr_dataIn; // @[InterruptController.scala 68:17:@1006.4]
  assign io_csr_3_dataIn = InterruptController_io_isr_dataIn; // @[InterruptController.scala 69:17:@1010.4]
  assign io_csr_4_dataIn = SimpleCSR_io_csr_dataIn; // @[SimpleCSR.scala 50:16:@1041.4]
  assign io_csr_5_dataIn = SimpleCSR_1_io_csr_dataIn; // @[SimpleCSR.scala 50:16:@1049.4]
  assign io_csr_6_dataIn = SimpleCSR_2_io_csr_dataIn; // @[SimpleCSR.scala 50:16:@1057.4]
  assign io_csr_7_dataIn = SimpleCSR_3_io_csr_dataIn; // @[SimpleCSR.scala 50:16:@1065.4]
  assign io_csr_8_dataIn = SimpleCSR_4_io_csr_dataIn; // @[SimpleCSR.scala 50:16:@1074.4]
  assign io_csr_9_dataIn = SimpleCSR_5_io_csr_dataIn; // @[SimpleCSR.scala 50:16:@1082.4]
  assign io_csr_10_dataIn = SimpleCSR_6_io_csr_dataIn; // @[SimpleCSR.scala 50:16:@1090.4]
  assign io_csr_11_dataIn = SimpleCSR_7_io_csr_dataIn; // @[SimpleCSR.scala 50:16:@1098.4]
  assign io_csr_12_dataIn = SimpleCSR_8_io_csr_dataIn; // @[SimpleCSR.scala 50:16:@1106.4]
  assign io_csr_13_dataIn = SimpleCSR_9_io_csr_dataIn; // @[SimpleCSR.scala 50:16:@1113.4]
  assign io_csr_14_dataIn = SimpleCSR_10_io_csr_dataIn; // @[SimpleCSR.scala 50:16:@1120.4]
  assign io_csr_15_dataIn = SimpleCSR_11_io_csr_dataIn; // @[SimpleCSR.scala 50:16:@1127.4]
  assign io_irq_readerDone = InterruptController_io_irq_readerDone; // @[WorkerCSRWrapper.scala 65:10:@1015.4]
  assign io_irq_writerDone = InterruptController_io_irq_writerDone; // @[WorkerCSRWrapper.scala 65:10:@1014.4]
  assign io_xferRead_length = transferSplitterRead_io_xferOut_length; // @[WorkerCSRWrapper.scala 90:15:@1138.4]
  assign io_xferRead_valid = transferSplitterRead_io_xferOut_valid; // @[WorkerCSRWrapper.scala 90:15:@1137.4]
  assign io_xferWrite_address = transferSplitterWrite_io_xferOut_address; // @[WorkerCSRWrapper.scala 93:16:@1149.4]
  assign io_xferWrite_length = transferSplitterWrite_io_xferOut_length; // @[WorkerCSRWrapper.scala 93:16:@1148.4]
  assign io_xferWrite_valid = transferSplitterWrite_io_xferOut_valid; // @[WorkerCSRWrapper.scala 93:16:@1147.4]
  assign addressGeneratorRead_clock = clock; // @[:@958.4]
  assign addressGeneratorRead_reset = reset; // @[:@959.4]
  assign addressGeneratorRead_io_ctl_start = readerStart; // @[WorkerCSRWrapper.scala 73:37:@1037.4]
  assign addressGeneratorRead_io_ctl_startAddress = SimpleCSR_io_value; // @[WorkerCSRWrapper.scala 74:44:@1045.4]
  assign addressGeneratorRead_io_ctl_lineLength = SimpleCSR_1_io_value; // @[WorkerCSRWrapper.scala 75:42:@1053.4]
  assign addressGeneratorRead_io_ctl_lineCount = SimpleCSR_2_io_value; // @[WorkerCSRWrapper.scala 76:41:@1061.4]
  assign addressGeneratorRead_io_ctl_lineGap = SimpleCSR_3_io_value; // @[WorkerCSRWrapper.scala 77:39:@1069.4]
  assign addressGeneratorRead_io_xfer_done = transferSplitterRead_io_xferIn_done; // @[WorkerCSRWrapper.scala 89:34:@1135.4]
  assign transferSplitterRead_io_xferIn_length = addressGeneratorRead_io_xfer_length; // @[WorkerCSRWrapper.scala 89:34:@1133.4]
  assign transferSplitterRead_io_xferIn_valid = addressGeneratorRead_io_xfer_valid; // @[WorkerCSRWrapper.scala 89:34:@1132.4]
  assign transferSplitterRead_io_xferOut_done = io_xferRead_done; // @[WorkerCSRWrapper.scala 90:15:@1140.4]
  assign addressGeneratorWrite_clock = clock; // @[:@964.4]
  assign addressGeneratorWrite_reset = reset; // @[:@965.4]
  assign addressGeneratorWrite_io_ctl_start = writerStart; // @[WorkerCSRWrapper.scala 79:38:@1070.4]
  assign addressGeneratorWrite_io_ctl_startAddress = SimpleCSR_4_io_value; // @[WorkerCSRWrapper.scala 80:45:@1078.4]
  assign addressGeneratorWrite_io_ctl_lineLength = SimpleCSR_5_io_value; // @[WorkerCSRWrapper.scala 81:43:@1086.4]
  assign addressGeneratorWrite_io_ctl_lineCount = SimpleCSR_6_io_value; // @[WorkerCSRWrapper.scala 82:42:@1094.4]
  assign addressGeneratorWrite_io_ctl_lineGap = SimpleCSR_7_io_value; // @[WorkerCSRWrapper.scala 83:40:@1102.4]
  assign addressGeneratorWrite_io_xfer_done = transferSplitterWrite_io_xferIn_done; // @[WorkerCSRWrapper.scala 92:35:@1145.4]
  assign transferSplitterWrite_clock = clock; // @[:@967.4]
  assign transferSplitterWrite_reset = reset; // @[:@968.4]
  assign transferSplitterWrite_io_xferIn_address = addressGeneratorWrite_io_xfer_address; // @[WorkerCSRWrapper.scala 92:35:@1144.4]
  assign transferSplitterWrite_io_xferIn_length = addressGeneratorWrite_io_xfer_length; // @[WorkerCSRWrapper.scala 92:35:@1143.4]
  assign transferSplitterWrite_io_xferIn_valid = addressGeneratorWrite_io_xfer_valid; // @[WorkerCSRWrapper.scala 92:35:@1142.4]
  assign transferSplitterWrite_io_xferOut_done = io_xferWrite_done; // @[WorkerCSRWrapper.scala 93:16:@1150.4]
  assign ClearCSR_clock = clock; // @[:@985.4]
  assign ClearCSR_reset = reset; // @[:@986.4]
  assign ClearCSR_io_csr_dataOut = io_csr_0_dataOut; // @[ClearCSR.scala 54:16:@990.4]
  assign ClearCSR_io_csr_dataWrite = io_csr_0_dataWrite; // @[ClearCSR.scala 54:16:@989.4]
  assign ClearCSR_io_clear = {{30'd0}, clear}; // @[ClearCSR.scala 52:18:@987.4]
  assign StatusCSR_clock = clock; // @[:@994.4]
  assign StatusCSR_io_value = {{30'd0}, status}; // @[StatusCSR.scala 46:18:@1000.4]
  assign InterruptController_clock = clock; // @[:@1002.4]
  assign InterruptController_reset = reset; // @[:@1003.4]
  assign InterruptController_io_readBusy = addressGeneratorRead_io_ctl_busy; // @[InterruptController.scala 65:22:@1004.4]
  assign InterruptController_io_writeBusy = addressGeneratorWrite_io_ctl_busy; // @[InterruptController.scala 66:23:@1005.4]
  assign InterruptController_io_imr_dataOut = io_csr_2_dataOut; // @[InterruptController.scala 68:17:@1008.4]
  assign InterruptController_io_imr_dataWrite = io_csr_2_dataWrite; // @[InterruptController.scala 68:17:@1007.4]
  assign InterruptController_io_isr_dataOut = io_csr_3_dataOut; // @[InterruptController.scala 69:17:@1012.4]
  assign InterruptController_io_isr_dataWrite = io_csr_3_dataWrite; // @[InterruptController.scala 69:17:@1011.4]
  assign SimpleCSR_clock = clock; // @[:@1039.4]
  assign SimpleCSR_reset = reset; // @[:@1040.4]
  assign SimpleCSR_io_csr_dataOut = io_csr_4_dataOut; // @[SimpleCSR.scala 50:16:@1043.4]
  assign SimpleCSR_io_csr_dataWrite = io_csr_4_dataWrite; // @[SimpleCSR.scala 50:16:@1042.4]
  assign SimpleCSR_1_clock = clock; // @[:@1047.4]
  assign SimpleCSR_1_reset = reset; // @[:@1048.4]
  assign SimpleCSR_1_io_csr_dataOut = io_csr_5_dataOut; // @[SimpleCSR.scala 50:16:@1051.4]
  assign SimpleCSR_1_io_csr_dataWrite = io_csr_5_dataWrite; // @[SimpleCSR.scala 50:16:@1050.4]
  assign SimpleCSR_2_clock = clock; // @[:@1055.4]
  assign SimpleCSR_2_reset = reset; // @[:@1056.4]
  assign SimpleCSR_2_io_csr_dataOut = io_csr_6_dataOut; // @[SimpleCSR.scala 50:16:@1059.4]
  assign SimpleCSR_2_io_csr_dataWrite = io_csr_6_dataWrite; // @[SimpleCSR.scala 50:16:@1058.4]
  assign SimpleCSR_3_clock = clock; // @[:@1063.4]
  assign SimpleCSR_3_reset = reset; // @[:@1064.4]
  assign SimpleCSR_3_io_csr_dataOut = io_csr_7_dataOut; // @[SimpleCSR.scala 50:16:@1067.4]
  assign SimpleCSR_3_io_csr_dataWrite = io_csr_7_dataWrite; // @[SimpleCSR.scala 50:16:@1066.4]
  assign SimpleCSR_4_clock = clock; // @[:@1072.4]
  assign SimpleCSR_4_reset = reset; // @[:@1073.4]
  assign SimpleCSR_4_io_csr_dataOut = io_csr_8_dataOut; // @[SimpleCSR.scala 50:16:@1076.4]
  assign SimpleCSR_4_io_csr_dataWrite = io_csr_8_dataWrite; // @[SimpleCSR.scala 50:16:@1075.4]
  assign SimpleCSR_5_clock = clock; // @[:@1080.4]
  assign SimpleCSR_5_reset = reset; // @[:@1081.4]
  assign SimpleCSR_5_io_csr_dataOut = io_csr_9_dataOut; // @[SimpleCSR.scala 50:16:@1084.4]
  assign SimpleCSR_5_io_csr_dataWrite = io_csr_9_dataWrite; // @[SimpleCSR.scala 50:16:@1083.4]
  assign SimpleCSR_6_clock = clock; // @[:@1088.4]
  assign SimpleCSR_6_reset = reset; // @[:@1089.4]
  assign SimpleCSR_6_io_csr_dataOut = io_csr_10_dataOut; // @[SimpleCSR.scala 50:16:@1092.4]
  assign SimpleCSR_6_io_csr_dataWrite = io_csr_10_dataWrite; // @[SimpleCSR.scala 50:16:@1091.4]
  assign SimpleCSR_7_clock = clock; // @[:@1096.4]
  assign SimpleCSR_7_reset = reset; // @[:@1097.4]
  assign SimpleCSR_7_io_csr_dataOut = io_csr_11_dataOut; // @[SimpleCSR.scala 50:16:@1100.4]
  assign SimpleCSR_7_io_csr_dataWrite = io_csr_11_dataWrite; // @[SimpleCSR.scala 50:16:@1099.4]
  assign SimpleCSR_8_clock = clock; // @[:@1104.4]
  assign SimpleCSR_8_reset = reset; // @[:@1105.4]
  assign SimpleCSR_8_io_csr_dataOut = io_csr_12_dataOut; // @[SimpleCSR.scala 50:16:@1108.4]
  assign SimpleCSR_8_io_csr_dataWrite = io_csr_12_dataWrite; // @[SimpleCSR.scala 50:16:@1107.4]
  assign SimpleCSR_9_clock = clock; // @[:@1111.4]
  assign SimpleCSR_9_reset = reset; // @[:@1112.4]
  assign SimpleCSR_9_io_csr_dataOut = io_csr_13_dataOut; // @[SimpleCSR.scala 50:16:@1115.4]
  assign SimpleCSR_9_io_csr_dataWrite = io_csr_13_dataWrite; // @[SimpleCSR.scala 50:16:@1114.4]
  assign SimpleCSR_10_clock = clock; // @[:@1118.4]
  assign SimpleCSR_10_reset = reset; // @[:@1119.4]
  assign SimpleCSR_10_io_csr_dataOut = io_csr_14_dataOut; // @[SimpleCSR.scala 50:16:@1122.4]
  assign SimpleCSR_10_io_csr_dataWrite = io_csr_14_dataWrite; // @[SimpleCSR.scala 50:16:@1121.4]
  assign SimpleCSR_11_clock = clock; // @[:@1125.4]
  assign SimpleCSR_11_reset = reset; // @[:@1126.4]
  assign SimpleCSR_11_io_csr_dataOut = io_csr_15_dataOut; // @[SimpleCSR.scala 50:16:@1129.4]
  assign SimpleCSR_11_io_csr_dataWrite = io_csr_15_dataWrite; // @[SimpleCSR.scala 50:16:@1128.4]
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE
  integer initvar;
  initial begin
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      #0.002 begin end
    `endif
  `ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  status = _RAND_0[1:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  readerSync = _RAND_1[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_2 = {1{`RANDOM}};
  readerSyncOld = _RAND_2[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_3 = {1{`RANDOM}};
  writerSync = _RAND_3[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_4 = {1{`RANDOM}};
  writerSyncOld = _RAND_4[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_5 = {1{`RANDOM}};
  readerStart = _RAND_5[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_6 = {1{`RANDOM}};
  writerStart = _RAND_6[0:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    status <= {addressGeneratorRead_io_ctl_busy,addressGeneratorWrite_io_ctl_busy};
    readerSync <= io_sync_readerSync;
    readerSyncOld <= readerSync;
    writerSync <= io_sync_writerSync;
    writerSyncOld <= writerSync;
    if (reset) begin
      readerStart <= 1'h0;
    end else begin
      readerStart <= _T_215;
    end
    if (reset) begin
      writerStart <= 1'h0;
    end else begin
      writerStart <= _T_222;
    end
  end
endmodule
module AXIS_to_WB_Queue( // @[:@1152.2]
  input         clock, // @[:@1153.4]
  input         reset, // @[:@1154.4]
  output        io_enq_ready, // @[:@1155.4]
  input         io_enq_valid, // @[:@1155.4]
  input  [31:0] io_enq_bits, // @[:@1155.4]
  input         io_deq_ready, // @[:@1155.4]
  output        io_deq_valid, // @[:@1155.4]
  output [31:0] io_deq_bits // @[:@1155.4]
);
  reg [31:0] ram [0:511]; // @[Decoupled.scala 215:24:@1157.4]
  reg [31:0] _RAND_0;
  wire [31:0] ram__T_63_data; // @[Decoupled.scala 215:24:@1157.4]
  wire [8:0] ram__T_63_addr; // @[Decoupled.scala 215:24:@1157.4]
  wire [31:0] ram__T_49_data; // @[Decoupled.scala 215:24:@1157.4]
  wire [8:0] ram__T_49_addr; // @[Decoupled.scala 215:24:@1157.4]
  wire  ram__T_49_mask; // @[Decoupled.scala 215:24:@1157.4]
  wire  ram__T_49_en; // @[Decoupled.scala 215:24:@1157.4]
  reg [8:0] value; // @[Counter.scala 26:33:@1158.4]
  reg [31:0] _RAND_1;
  reg [8:0] value_1; // @[Counter.scala 26:33:@1159.4]
  reg [31:0] _RAND_2;
  reg  maybe_full; // @[Decoupled.scala 218:35:@1160.4]
  reg [31:0] _RAND_3;
  wire  _T_41; // @[Decoupled.scala 220:41:@1161.4]
  wire  _T_43; // @[Decoupled.scala 221:36:@1162.4]
  wire  empty; // @[Decoupled.scala 221:33:@1163.4]
  wire  _T_44; // @[Decoupled.scala 222:32:@1164.4]
  wire  do_enq; // @[Decoupled.scala 37:37:@1165.4]
  wire  do_deq; // @[Decoupled.scala 37:37:@1168.4]
  wire [9:0] _T_52; // @[Counter.scala 35:22:@1175.6]
  wire [8:0] _T_53; // @[Counter.scala 35:22:@1176.6]
  wire [8:0] _GEN_5; // @[Decoupled.scala 226:17:@1171.4]
  wire [9:0] _T_56; // @[Counter.scala 35:22:@1181.6]
  wire [8:0] _T_57; // @[Counter.scala 35:22:@1182.6]
  wire [8:0] _GEN_6; // @[Decoupled.scala 230:17:@1179.4]
  wire  _T_58; // @[Decoupled.scala 233:16:@1185.4]
  wire  _GEN_7; // @[Decoupled.scala 233:28:@1186.4]
  assign ram__T_63_addr = value_1;
  assign ram__T_63_data = ram[ram__T_63_addr]; // @[Decoupled.scala 215:24:@1157.4]
  assign ram__T_49_data = io_enq_bits;
  assign ram__T_49_addr = value;
  assign ram__T_49_mask = 1'h1;
  assign ram__T_49_en = io_enq_ready & io_enq_valid;
  assign _T_41 = value == value_1; // @[Decoupled.scala 220:41:@1161.4]
  assign _T_43 = maybe_full == 1'h0; // @[Decoupled.scala 221:36:@1162.4]
  assign empty = _T_41 & _T_43; // @[Decoupled.scala 221:33:@1163.4]
  assign _T_44 = _T_41 & maybe_full; // @[Decoupled.scala 222:32:@1164.4]
  assign do_enq = io_enq_ready & io_enq_valid; // @[Decoupled.scala 37:37:@1165.4]
  assign do_deq = io_deq_ready & io_deq_valid; // @[Decoupled.scala 37:37:@1168.4]
  assign _T_52 = value + 9'h1; // @[Counter.scala 35:22:@1175.6]
  assign _T_53 = value + 9'h1; // @[Counter.scala 35:22:@1176.6]
  assign _GEN_5 = do_enq ? _T_53 : value; // @[Decoupled.scala 226:17:@1171.4]
  assign _T_56 = value_1 + 9'h1; // @[Counter.scala 35:22:@1181.6]
  assign _T_57 = value_1 + 9'h1; // @[Counter.scala 35:22:@1182.6]
  assign _GEN_6 = do_deq ? _T_57 : value_1; // @[Decoupled.scala 230:17:@1179.4]
  assign _T_58 = do_enq != do_deq; // @[Decoupled.scala 233:16:@1185.4]
  assign _GEN_7 = _T_58 ? do_enq : maybe_full; // @[Decoupled.scala 233:28:@1186.4]
  assign io_enq_ready = _T_44 == 1'h0; // @[Decoupled.scala 238:16:@1192.4]
  assign io_deq_valid = empty == 1'h0; // @[Decoupled.scala 237:16:@1190.4]
  assign io_deq_bits = ram__T_63_data; // @[Decoupled.scala 239:15:@1194.4]
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE
  integer initvar;
  initial begin
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      #0.002 begin end
    `endif
  _RAND_0 = {1{`RANDOM}};
  `ifdef RANDOMIZE_MEM_INIT
  for (initvar = 0; initvar < 512; initvar = initvar+1)
    ram[initvar] = _RAND_0[31:0];
  `endif // RANDOMIZE_MEM_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  value = _RAND_1[8:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_2 = {1{`RANDOM}};
  value_1 = _RAND_2[8:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_3 = {1{`RANDOM}};
  maybe_full = _RAND_3[0:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if(ram__T_49_en & ram__T_49_mask) begin
      ram[ram__T_49_addr] <= ram__T_49_data; // @[Decoupled.scala 215:24:@1157.4]
    end
    if (reset) begin
      value <= 9'h0;
    end else begin
      if (do_enq) begin
        value <= _T_53;
      end
    end
    if (reset) begin
      value_1 <= 9'h0;
    end else begin
      if (do_deq) begin
        value_1 <= _T_57;
      end
    end
    if (reset) begin
      maybe_full <= 1'h0;
    end else begin
      if (_T_58) begin
        maybe_full <= do_enq;
      end
    end
  end
endmodule
module AXIS_to_WB( // @[:@1203.2]
  input         clock, // @[:@1204.4]
  input         reset, // @[:@1205.4]
  input  [31:0] io_control_dat_i, // @[:@1206.4]
  output [31:0] io_control_dat_o, // @[:@1206.4]
  input         io_control_cyc_i, // @[:@1206.4]
  input         io_control_stb_i, // @[:@1206.4]
  input         io_control_we_i, // @[:@1206.4]
  input  [29:0] io_control_adr_i, // @[:@1206.4]
  input  [3:0]  io_control_sel_i, // @[:@1206.4]
  output        io_control_ack_o, // @[:@1206.4]
  output        io_control_stall_o, // @[:@1206.4]
  output        io_control_err_o, // @[:@1206.4]
  input  [31:0] io_read_tdata, // @[:@1206.4]
  input         io_read_tvalid, // @[:@1206.4]
  output        io_read_tready, // @[:@1206.4]
  input         io_read_tuser, // @[:@1206.4]
  input         io_read_tlast, // @[:@1206.4]
  input  [31:0] io_write_dat_i, // @[:@1206.4]
  output [31:0] io_write_dat_o, // @[:@1206.4]
  output        io_write_cyc_o, // @[:@1206.4]
  output        io_write_stb_o, // @[:@1206.4]
  output        io_write_we_o, // @[:@1206.4]
  output [29:0] io_write_adr_o, // @[:@1206.4]
  output [3:0]  io_write_sel_o, // @[:@1206.4]
  input         io_write_ack_i, // @[:@1206.4]
  input         io_write_stall_i, // @[:@1206.4]
  input         io_write_err_i, // @[:@1206.4]
  output        io_irq_readerDone, // @[:@1206.4]
  output        io_irq_writerDone, // @[:@1206.4]
  input         io_sync_readerSync, // @[:@1206.4]
  input         io_sync_writerSync // @[:@1206.4]
);
  wire  csrFrontend_clock; // @[DMATop.scala 42:27:@1208.4]
  wire  csrFrontend_reset; // @[DMATop.scala 42:27:@1208.4]
  wire [31:0] csrFrontend_io_ctl_dat_i; // @[DMATop.scala 42:27:@1208.4]
  wire [31:0] csrFrontend_io_ctl_dat_o; // @[DMATop.scala 42:27:@1208.4]
  wire  csrFrontend_io_ctl_cyc_i; // @[DMATop.scala 42:27:@1208.4]
  wire  csrFrontend_io_ctl_stb_i; // @[DMATop.scala 42:27:@1208.4]
  wire  csrFrontend_io_ctl_we_i; // @[DMATop.scala 42:27:@1208.4]
  wire [29:0] csrFrontend_io_ctl_adr_i; // @[DMATop.scala 42:27:@1208.4]
  wire  csrFrontend_io_ctl_ack_o; // @[DMATop.scala 42:27:@1208.4]
  wire [3:0] csrFrontend_io_bus_addr; // @[DMATop.scala 42:27:@1208.4]
  wire [31:0] csrFrontend_io_bus_dataOut; // @[DMATop.scala 42:27:@1208.4]
  wire [31:0] csrFrontend_io_bus_dataIn; // @[DMATop.scala 42:27:@1208.4]
  wire  csrFrontend_io_bus_write; // @[DMATop.scala 42:27:@1208.4]
  wire  csrFrontend_io_bus_read; // @[DMATop.scala 42:27:@1208.4]
  wire  readerFrontend_clock; // @[DMATop.scala 44:30:@1211.4]
  wire  readerFrontend_reset; // @[DMATop.scala 44:30:@1211.4]
  wire [31:0] readerFrontend_io_bus_tdata; // @[DMATop.scala 44:30:@1211.4]
  wire  readerFrontend_io_bus_tvalid; // @[DMATop.scala 44:30:@1211.4]
  wire  readerFrontend_io_bus_tready; // @[DMATop.scala 44:30:@1211.4]
  wire  readerFrontend_io_dataOut_ready; // @[DMATop.scala 44:30:@1211.4]
  wire  readerFrontend_io_dataOut_valid; // @[DMATop.scala 44:30:@1211.4]
  wire [31:0] readerFrontend_io_dataOut_bits; // @[DMATop.scala 44:30:@1211.4]
  wire  readerFrontend_io_xfer_done; // @[DMATop.scala 44:30:@1211.4]
  wire [31:0] readerFrontend_io_xfer_length; // @[DMATop.scala 44:30:@1211.4]
  wire  readerFrontend_io_xfer_valid; // @[DMATop.scala 44:30:@1211.4]
  wire  writerFrontend_clock; // @[DMATop.scala 46:30:@1214.4]
  wire  writerFrontend_reset; // @[DMATop.scala 46:30:@1214.4]
  wire [31:0] writerFrontend_io_bus_dat_o; // @[DMATop.scala 46:30:@1214.4]
  wire  writerFrontend_io_bus_cyc_o; // @[DMATop.scala 46:30:@1214.4]
  wire  writerFrontend_io_bus_stb_o; // @[DMATop.scala 46:30:@1214.4]
  wire [29:0] writerFrontend_io_bus_adr_o; // @[DMATop.scala 46:30:@1214.4]
  wire  writerFrontend_io_bus_ack_i; // @[DMATop.scala 46:30:@1214.4]
  wire  writerFrontend_io_dataIn_ready; // @[DMATop.scala 46:30:@1214.4]
  wire  writerFrontend_io_dataIn_valid; // @[DMATop.scala 46:30:@1214.4]
  wire [31:0] writerFrontend_io_dataIn_bits; // @[DMATop.scala 46:30:@1214.4]
  wire  writerFrontend_io_xfer_done; // @[DMATop.scala 46:30:@1214.4]
  wire [31:0] writerFrontend_io_xfer_address; // @[DMATop.scala 46:30:@1214.4]
  wire [31:0] writerFrontend_io_xfer_length; // @[DMATop.scala 46:30:@1214.4]
  wire  writerFrontend_io_xfer_valid; // @[DMATop.scala 46:30:@1214.4]
  wire [31:0] csr_io_csr_0_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_0_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_0_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_1_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_2_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_2_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_2_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_3_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_3_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_3_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_4_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_4_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_4_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_5_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_5_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_5_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_6_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_6_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_6_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_7_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_7_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_7_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_8_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_8_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_8_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_9_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_9_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_9_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_10_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_10_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_10_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_11_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_11_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_11_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_12_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_12_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_12_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_13_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_13_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_13_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_14_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_14_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_14_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_15_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_csr_15_dataWrite; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_csr_15_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire [3:0] csr_io_bus_addr; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_bus_dataOut; // @[DMATop.scala 48:19:@1217.4]
  wire [31:0] csr_io_bus_dataIn; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_bus_write; // @[DMATop.scala 48:19:@1217.4]
  wire  csr_io_bus_read; // @[DMATop.scala 48:19:@1217.4]
  wire  ctl_clock; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_reset; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_0_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_0_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_0_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_1_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_2_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_2_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_2_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_3_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_3_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_3_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_4_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_4_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_4_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_5_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_5_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_5_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_6_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_6_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_6_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_7_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_7_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_7_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_8_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_8_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_8_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_9_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_9_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_9_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_10_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_10_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_10_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_11_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_11_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_11_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_12_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_12_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_12_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_13_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_13_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_13_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_14_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_14_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_14_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_15_dataOut; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_csr_15_dataWrite; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_csr_15_dataIn; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_irq_readerDone; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_irq_writerDone; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_sync_readerSync; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_sync_writerSync; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_xferRead_done; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_xferRead_length; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_xferRead_valid; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_xferWrite_done; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_xferWrite_address; // @[DMATop.scala 50:19:@1220.4]
  wire [31:0] ctl_io_xferWrite_length; // @[DMATop.scala 50:19:@1220.4]
  wire  ctl_io_xferWrite_valid; // @[DMATop.scala 50:19:@1220.4]
  wire  queue_clock; // @[Decoupled.scala 294:21:@1223.4]
  wire  queue_reset; // @[Decoupled.scala 294:21:@1223.4]
  wire  queue_io_enq_ready; // @[Decoupled.scala 294:21:@1223.4]
  wire  queue_io_enq_valid; // @[Decoupled.scala 294:21:@1223.4]
  wire [31:0] queue_io_enq_bits; // @[Decoupled.scala 294:21:@1223.4]
  wire  queue_io_deq_ready; // @[Decoupled.scala 294:21:@1223.4]
  wire  queue_io_deq_valid; // @[Decoupled.scala 294:21:@1223.4]
  wire [31:0] queue_io_deq_bits; // @[Decoupled.scala 294:21:@1223.4]
  AXIS_to_WB_WishboneCSR csrFrontend ( // @[DMATop.scala 42:27:@1208.4]
    .clock(csrFrontend_clock),
    .reset(csrFrontend_reset),
    .io_ctl_dat_i(csrFrontend_io_ctl_dat_i),
    .io_ctl_dat_o(csrFrontend_io_ctl_dat_o),
    .io_ctl_cyc_i(csrFrontend_io_ctl_cyc_i),
    .io_ctl_stb_i(csrFrontend_io_ctl_stb_i),
    .io_ctl_we_i(csrFrontend_io_ctl_we_i),
    .io_ctl_adr_i(csrFrontend_io_ctl_adr_i),
    .io_ctl_ack_o(csrFrontend_io_ctl_ack_o),
    .io_bus_addr(csrFrontend_io_bus_addr),
    .io_bus_dataOut(csrFrontend_io_bus_dataOut),
    .io_bus_dataIn(csrFrontend_io_bus_dataIn),
    .io_bus_write(csrFrontend_io_bus_write),
    .io_bus_read(csrFrontend_io_bus_read)
  );
  AXIStreamSlave readerFrontend ( // @[DMATop.scala 44:30:@1211.4]
    .clock(readerFrontend_clock),
    .reset(readerFrontend_reset),
    .io_bus_tdata(readerFrontend_io_bus_tdata),
    .io_bus_tvalid(readerFrontend_io_bus_tvalid),
    .io_bus_tready(readerFrontend_io_bus_tready),
    .io_dataOut_ready(readerFrontend_io_dataOut_ready),
    .io_dataOut_valid(readerFrontend_io_dataOut_valid),
    .io_dataOut_bits(readerFrontend_io_dataOut_bits),
    .io_xfer_done(readerFrontend_io_xfer_done),
    .io_xfer_length(readerFrontend_io_xfer_length),
    .io_xfer_valid(readerFrontend_io_xfer_valid)
  );
  WishboneClassicWriter writerFrontend ( // @[DMATop.scala 46:30:@1214.4]
    .clock(writerFrontend_clock),
    .reset(writerFrontend_reset),
    .io_bus_dat_o(writerFrontend_io_bus_dat_o),
    .io_bus_cyc_o(writerFrontend_io_bus_cyc_o),
    .io_bus_stb_o(writerFrontend_io_bus_stb_o),
    .io_bus_adr_o(writerFrontend_io_bus_adr_o),
    .io_bus_ack_i(writerFrontend_io_bus_ack_i),
    .io_dataIn_ready(writerFrontend_io_dataIn_ready),
    .io_dataIn_valid(writerFrontend_io_dataIn_valid),
    .io_dataIn_bits(writerFrontend_io_dataIn_bits),
    .io_xfer_done(writerFrontend_io_xfer_done),
    .io_xfer_address(writerFrontend_io_xfer_address),
    .io_xfer_length(writerFrontend_io_xfer_length),
    .io_xfer_valid(writerFrontend_io_xfer_valid)
  );
  AXIS_to_WB_CSR csr ( // @[DMATop.scala 48:19:@1217.4]
    .io_csr_0_dataOut(csr_io_csr_0_dataOut),
    .io_csr_0_dataWrite(csr_io_csr_0_dataWrite),
    .io_csr_0_dataIn(csr_io_csr_0_dataIn),
    .io_csr_1_dataIn(csr_io_csr_1_dataIn),
    .io_csr_2_dataOut(csr_io_csr_2_dataOut),
    .io_csr_2_dataWrite(csr_io_csr_2_dataWrite),
    .io_csr_2_dataIn(csr_io_csr_2_dataIn),
    .io_csr_3_dataOut(csr_io_csr_3_dataOut),
    .io_csr_3_dataWrite(csr_io_csr_3_dataWrite),
    .io_csr_3_dataIn(csr_io_csr_3_dataIn),
    .io_csr_4_dataOut(csr_io_csr_4_dataOut),
    .io_csr_4_dataWrite(csr_io_csr_4_dataWrite),
    .io_csr_4_dataIn(csr_io_csr_4_dataIn),
    .io_csr_5_dataOut(csr_io_csr_5_dataOut),
    .io_csr_5_dataWrite(csr_io_csr_5_dataWrite),
    .io_csr_5_dataIn(csr_io_csr_5_dataIn),
    .io_csr_6_dataOut(csr_io_csr_6_dataOut),
    .io_csr_6_dataWrite(csr_io_csr_6_dataWrite),
    .io_csr_6_dataIn(csr_io_csr_6_dataIn),
    .io_csr_7_dataOut(csr_io_csr_7_dataOut),
    .io_csr_7_dataWrite(csr_io_csr_7_dataWrite),
    .io_csr_7_dataIn(csr_io_csr_7_dataIn),
    .io_csr_8_dataOut(csr_io_csr_8_dataOut),
    .io_csr_8_dataWrite(csr_io_csr_8_dataWrite),
    .io_csr_8_dataIn(csr_io_csr_8_dataIn),
    .io_csr_9_dataOut(csr_io_csr_9_dataOut),
    .io_csr_9_dataWrite(csr_io_csr_9_dataWrite),
    .io_csr_9_dataIn(csr_io_csr_9_dataIn),
    .io_csr_10_dataOut(csr_io_csr_10_dataOut),
    .io_csr_10_dataWrite(csr_io_csr_10_dataWrite),
    .io_csr_10_dataIn(csr_io_csr_10_dataIn),
    .io_csr_11_dataOut(csr_io_csr_11_dataOut),
    .io_csr_11_dataWrite(csr_io_csr_11_dataWrite),
    .io_csr_11_dataIn(csr_io_csr_11_dataIn),
    .io_csr_12_dataOut(csr_io_csr_12_dataOut),
    .io_csr_12_dataWrite(csr_io_csr_12_dataWrite),
    .io_csr_12_dataIn(csr_io_csr_12_dataIn),
    .io_csr_13_dataOut(csr_io_csr_13_dataOut),
    .io_csr_13_dataWrite(csr_io_csr_13_dataWrite),
    .io_csr_13_dataIn(csr_io_csr_13_dataIn),
    .io_csr_14_dataOut(csr_io_csr_14_dataOut),
    .io_csr_14_dataWrite(csr_io_csr_14_dataWrite),
    .io_csr_14_dataIn(csr_io_csr_14_dataIn),
    .io_csr_15_dataOut(csr_io_csr_15_dataOut),
    .io_csr_15_dataWrite(csr_io_csr_15_dataWrite),
    .io_csr_15_dataIn(csr_io_csr_15_dataIn),
    .io_bus_addr(csr_io_bus_addr),
    .io_bus_dataOut(csr_io_bus_dataOut),
    .io_bus_dataIn(csr_io_bus_dataIn),
    .io_bus_write(csr_io_bus_write),
    .io_bus_read(csr_io_bus_read)
  );
  AXIS_to_WB_WorkerCSRWrapper ctl ( // @[DMATop.scala 50:19:@1220.4]
    .clock(ctl_clock),
    .reset(ctl_reset),
    .io_csr_0_dataOut(ctl_io_csr_0_dataOut),
    .io_csr_0_dataWrite(ctl_io_csr_0_dataWrite),
    .io_csr_0_dataIn(ctl_io_csr_0_dataIn),
    .io_csr_1_dataIn(ctl_io_csr_1_dataIn),
    .io_csr_2_dataOut(ctl_io_csr_2_dataOut),
    .io_csr_2_dataWrite(ctl_io_csr_2_dataWrite),
    .io_csr_2_dataIn(ctl_io_csr_2_dataIn),
    .io_csr_3_dataOut(ctl_io_csr_3_dataOut),
    .io_csr_3_dataWrite(ctl_io_csr_3_dataWrite),
    .io_csr_3_dataIn(ctl_io_csr_3_dataIn),
    .io_csr_4_dataOut(ctl_io_csr_4_dataOut),
    .io_csr_4_dataWrite(ctl_io_csr_4_dataWrite),
    .io_csr_4_dataIn(ctl_io_csr_4_dataIn),
    .io_csr_5_dataOut(ctl_io_csr_5_dataOut),
    .io_csr_5_dataWrite(ctl_io_csr_5_dataWrite),
    .io_csr_5_dataIn(ctl_io_csr_5_dataIn),
    .io_csr_6_dataOut(ctl_io_csr_6_dataOut),
    .io_csr_6_dataWrite(ctl_io_csr_6_dataWrite),
    .io_csr_6_dataIn(ctl_io_csr_6_dataIn),
    .io_csr_7_dataOut(ctl_io_csr_7_dataOut),
    .io_csr_7_dataWrite(ctl_io_csr_7_dataWrite),
    .io_csr_7_dataIn(ctl_io_csr_7_dataIn),
    .io_csr_8_dataOut(ctl_io_csr_8_dataOut),
    .io_csr_8_dataWrite(ctl_io_csr_8_dataWrite),
    .io_csr_8_dataIn(ctl_io_csr_8_dataIn),
    .io_csr_9_dataOut(ctl_io_csr_9_dataOut),
    .io_csr_9_dataWrite(ctl_io_csr_9_dataWrite),
    .io_csr_9_dataIn(ctl_io_csr_9_dataIn),
    .io_csr_10_dataOut(ctl_io_csr_10_dataOut),
    .io_csr_10_dataWrite(ctl_io_csr_10_dataWrite),
    .io_csr_10_dataIn(ctl_io_csr_10_dataIn),
    .io_csr_11_dataOut(ctl_io_csr_11_dataOut),
    .io_csr_11_dataWrite(ctl_io_csr_11_dataWrite),
    .io_csr_11_dataIn(ctl_io_csr_11_dataIn),
    .io_csr_12_dataOut(ctl_io_csr_12_dataOut),
    .io_csr_12_dataWrite(ctl_io_csr_12_dataWrite),
    .io_csr_12_dataIn(ctl_io_csr_12_dataIn),
    .io_csr_13_dataOut(ctl_io_csr_13_dataOut),
    .io_csr_13_dataWrite(ctl_io_csr_13_dataWrite),
    .io_csr_13_dataIn(ctl_io_csr_13_dataIn),
    .io_csr_14_dataOut(ctl_io_csr_14_dataOut),
    .io_csr_14_dataWrite(ctl_io_csr_14_dataWrite),
    .io_csr_14_dataIn(ctl_io_csr_14_dataIn),
    .io_csr_15_dataOut(ctl_io_csr_15_dataOut),
    .io_csr_15_dataWrite(ctl_io_csr_15_dataWrite),
    .io_csr_15_dataIn(ctl_io_csr_15_dataIn),
    .io_irq_readerDone(ctl_io_irq_readerDone),
    .io_irq_writerDone(ctl_io_irq_writerDone),
    .io_sync_readerSync(ctl_io_sync_readerSync),
    .io_sync_writerSync(ctl_io_sync_writerSync),
    .io_xferRead_done(ctl_io_xferRead_done),
    .io_xferRead_length(ctl_io_xferRead_length),
    .io_xferRead_valid(ctl_io_xferRead_valid),
    .io_xferWrite_done(ctl_io_xferWrite_done),
    .io_xferWrite_address(ctl_io_xferWrite_address),
    .io_xferWrite_length(ctl_io_xferWrite_length),
    .io_xferWrite_valid(ctl_io_xferWrite_valid)
  );
  AXIS_to_WB_Queue queue ( // @[Decoupled.scala 294:21:@1223.4]
    .clock(queue_clock),
    .reset(queue_reset),
    .io_enq_ready(queue_io_enq_ready),
    .io_enq_valid(queue_io_enq_valid),
    .io_enq_bits(queue_io_enq_bits),
    .io_deq_ready(queue_io_deq_ready),
    .io_deq_valid(queue_io_deq_valid),
    .io_deq_bits(queue_io_deq_bits)
  );
  assign io_control_dat_o = csrFrontend_io_ctl_dat_o; // @[DMATop.scala 56:22:@1240.4]
  assign io_control_ack_o = csrFrontend_io_ctl_ack_o; // @[DMATop.scala 56:22:@1234.4]
  assign io_control_stall_o = 1'h0; // @[DMATop.scala 56:22:@1233.4]
  assign io_control_err_o = 1'h0; // @[DMATop.scala 56:22:@1232.4]
  assign io_read_tready = readerFrontend_io_bus_tready; // @[DMATop.scala 62:11:@1323.4]
  assign io_write_dat_o = writerFrontend_io_bus_dat_o; // @[DMATop.scala 63:12:@1334.4]
  assign io_write_cyc_o = writerFrontend_io_bus_cyc_o; // @[DMATop.scala 63:12:@1333.4]
  assign io_write_stb_o = writerFrontend_io_bus_stb_o; // @[DMATop.scala 63:12:@1332.4]
  assign io_write_we_o = 1'h1; // @[DMATop.scala 63:12:@1331.4]
  assign io_write_adr_o = writerFrontend_io_bus_adr_o; // @[DMATop.scala 63:12:@1330.4]
  assign io_write_sel_o = 4'hf; // @[DMATop.scala 63:12:@1329.4]
  assign io_irq_readerDone = ctl_io_irq_readerDone; // @[DMATop.scala 65:10:@1337.4]
  assign io_irq_writerDone = ctl_io_irq_writerDone; // @[DMATop.scala 65:10:@1336.4]
  assign csrFrontend_clock = clock; // @[:@1209.4]
  assign csrFrontend_reset = reset; // @[:@1210.4]
  assign csrFrontend_io_ctl_dat_i = io_control_dat_i; // @[DMATop.scala 56:22:@1241.4]
  assign csrFrontend_io_ctl_cyc_i = io_control_cyc_i; // @[DMATop.scala 56:22:@1239.4]
  assign csrFrontend_io_ctl_stb_i = io_control_stb_i; // @[DMATop.scala 56:22:@1238.4]
  assign csrFrontend_io_ctl_we_i = io_control_we_i; // @[DMATop.scala 56:22:@1237.4]
  assign csrFrontend_io_ctl_adr_i = io_control_adr_i; // @[DMATop.scala 56:22:@1236.4]
  assign csrFrontend_io_bus_dataIn = csr_io_bus_dataIn; // @[DMATop.scala 57:14:@1244.4]
  assign readerFrontend_clock = clock; // @[:@1212.4]
  assign readerFrontend_reset = reset; // @[:@1213.4]
  assign readerFrontend_io_bus_tdata = io_read_tdata; // @[DMATop.scala 62:11:@1325.4]
  assign readerFrontend_io_bus_tvalid = io_read_tvalid; // @[DMATop.scala 62:11:@1324.4]
  assign readerFrontend_io_dataOut_ready = queue_io_enq_ready; // @[Decoupled.scala 297:17:@1228.4]
  assign readerFrontend_io_xfer_length = ctl_io_xferRead_length; // @[DMATop.scala 59:26:@1313.4]
  assign readerFrontend_io_xfer_valid = ctl_io_xferRead_valid; // @[DMATop.scala 59:26:@1312.4]
  assign writerFrontend_clock = clock; // @[:@1215.4]
  assign writerFrontend_reset = reset; // @[:@1216.4]
  assign writerFrontend_io_bus_ack_i = io_write_ack_i; // @[DMATop.scala 63:12:@1328.4]
  assign writerFrontend_io_dataIn_valid = queue_io_deq_valid; // @[DMATop.scala 54:9:@1230.4]
  assign writerFrontend_io_dataIn_bits = queue_io_deq_bits; // @[DMATop.scala 54:9:@1229.4]
  assign writerFrontend_io_xfer_address = ctl_io_xferWrite_address; // @[DMATop.scala 60:26:@1319.4]
  assign writerFrontend_io_xfer_length = ctl_io_xferWrite_length; // @[DMATop.scala 60:26:@1318.4]
  assign writerFrontend_io_xfer_valid = ctl_io_xferWrite_valid; // @[DMATop.scala 60:26:@1317.4]
  assign csr_io_csr_0_dataIn = ctl_io_csr_0_dataIn; // @[DMATop.scala 58:14:@1247.4]
  assign csr_io_csr_1_dataIn = ctl_io_csr_1_dataIn; // @[DMATop.scala 58:14:@1251.4]
  assign csr_io_csr_2_dataIn = ctl_io_csr_2_dataIn; // @[DMATop.scala 58:14:@1255.4]
  assign csr_io_csr_3_dataIn = ctl_io_csr_3_dataIn; // @[DMATop.scala 58:14:@1259.4]
  assign csr_io_csr_4_dataIn = ctl_io_csr_4_dataIn; // @[DMATop.scala 58:14:@1263.4]
  assign csr_io_csr_5_dataIn = ctl_io_csr_5_dataIn; // @[DMATop.scala 58:14:@1267.4]
  assign csr_io_csr_6_dataIn = ctl_io_csr_6_dataIn; // @[DMATop.scala 58:14:@1271.4]
  assign csr_io_csr_7_dataIn = ctl_io_csr_7_dataIn; // @[DMATop.scala 58:14:@1275.4]
  assign csr_io_csr_8_dataIn = ctl_io_csr_8_dataIn; // @[DMATop.scala 58:14:@1279.4]
  assign csr_io_csr_9_dataIn = ctl_io_csr_9_dataIn; // @[DMATop.scala 58:14:@1283.4]
  assign csr_io_csr_10_dataIn = ctl_io_csr_10_dataIn; // @[DMATop.scala 58:14:@1287.4]
  assign csr_io_csr_11_dataIn = ctl_io_csr_11_dataIn; // @[DMATop.scala 58:14:@1291.4]
  assign csr_io_csr_12_dataIn = ctl_io_csr_12_dataIn; // @[DMATop.scala 58:14:@1295.4]
  assign csr_io_csr_13_dataIn = ctl_io_csr_13_dataIn; // @[DMATop.scala 58:14:@1299.4]
  assign csr_io_csr_14_dataIn = ctl_io_csr_14_dataIn; // @[DMATop.scala 58:14:@1303.4]
  assign csr_io_csr_15_dataIn = ctl_io_csr_15_dataIn; // @[DMATop.scala 58:14:@1307.4]
  assign csr_io_bus_addr = csrFrontend_io_bus_addr; // @[DMATop.scala 57:14:@1246.4]
  assign csr_io_bus_dataOut = csrFrontend_io_bus_dataOut; // @[DMATop.scala 57:14:@1245.4]
  assign csr_io_bus_write = csrFrontend_io_bus_write; // @[DMATop.scala 57:14:@1243.4]
  assign csr_io_bus_read = csrFrontend_io_bus_read; // @[DMATop.scala 57:14:@1242.4]
  assign ctl_clock = clock; // @[:@1221.4]
  assign ctl_reset = reset; // @[:@1222.4]
  assign ctl_io_csr_0_dataOut = csr_io_csr_0_dataOut; // @[DMATop.scala 58:14:@1249.4]
  assign ctl_io_csr_0_dataWrite = csr_io_csr_0_dataWrite; // @[DMATop.scala 58:14:@1248.4]
  assign ctl_io_csr_2_dataOut = csr_io_csr_2_dataOut; // @[DMATop.scala 58:14:@1257.4]
  assign ctl_io_csr_2_dataWrite = csr_io_csr_2_dataWrite; // @[DMATop.scala 58:14:@1256.4]
  assign ctl_io_csr_3_dataOut = csr_io_csr_3_dataOut; // @[DMATop.scala 58:14:@1261.4]
  assign ctl_io_csr_3_dataWrite = csr_io_csr_3_dataWrite; // @[DMATop.scala 58:14:@1260.4]
  assign ctl_io_csr_4_dataOut = csr_io_csr_4_dataOut; // @[DMATop.scala 58:14:@1265.4]
  assign ctl_io_csr_4_dataWrite = csr_io_csr_4_dataWrite; // @[DMATop.scala 58:14:@1264.4]
  assign ctl_io_csr_5_dataOut = csr_io_csr_5_dataOut; // @[DMATop.scala 58:14:@1269.4]
  assign ctl_io_csr_5_dataWrite = csr_io_csr_5_dataWrite; // @[DMATop.scala 58:14:@1268.4]
  assign ctl_io_csr_6_dataOut = csr_io_csr_6_dataOut; // @[DMATop.scala 58:14:@1273.4]
  assign ctl_io_csr_6_dataWrite = csr_io_csr_6_dataWrite; // @[DMATop.scala 58:14:@1272.4]
  assign ctl_io_csr_7_dataOut = csr_io_csr_7_dataOut; // @[DMATop.scala 58:14:@1277.4]
  assign ctl_io_csr_7_dataWrite = csr_io_csr_7_dataWrite; // @[DMATop.scala 58:14:@1276.4]
  assign ctl_io_csr_8_dataOut = csr_io_csr_8_dataOut; // @[DMATop.scala 58:14:@1281.4]
  assign ctl_io_csr_8_dataWrite = csr_io_csr_8_dataWrite; // @[DMATop.scala 58:14:@1280.4]
  assign ctl_io_csr_9_dataOut = csr_io_csr_9_dataOut; // @[DMATop.scala 58:14:@1285.4]
  assign ctl_io_csr_9_dataWrite = csr_io_csr_9_dataWrite; // @[DMATop.scala 58:14:@1284.4]
  assign ctl_io_csr_10_dataOut = csr_io_csr_10_dataOut; // @[DMATop.scala 58:14:@1289.4]
  assign ctl_io_csr_10_dataWrite = csr_io_csr_10_dataWrite; // @[DMATop.scala 58:14:@1288.4]
  assign ctl_io_csr_11_dataOut = csr_io_csr_11_dataOut; // @[DMATop.scala 58:14:@1293.4]
  assign ctl_io_csr_11_dataWrite = csr_io_csr_11_dataWrite; // @[DMATop.scala 58:14:@1292.4]
  assign ctl_io_csr_12_dataOut = csr_io_csr_12_dataOut; // @[DMATop.scala 58:14:@1297.4]
  assign ctl_io_csr_12_dataWrite = csr_io_csr_12_dataWrite; // @[DMATop.scala 58:14:@1296.4]
  assign ctl_io_csr_13_dataOut = csr_io_csr_13_dataOut; // @[DMATop.scala 58:14:@1301.4]
  assign ctl_io_csr_13_dataWrite = csr_io_csr_13_dataWrite; // @[DMATop.scala 58:14:@1300.4]
  assign ctl_io_csr_14_dataOut = csr_io_csr_14_dataOut; // @[DMATop.scala 58:14:@1305.4]
  assign ctl_io_csr_14_dataWrite = csr_io_csr_14_dataWrite; // @[DMATop.scala 58:14:@1304.4]
  assign ctl_io_csr_15_dataOut = csr_io_csr_15_dataOut; // @[DMATop.scala 58:14:@1309.4]
  assign ctl_io_csr_15_dataWrite = csr_io_csr_15_dataWrite; // @[DMATop.scala 58:14:@1308.4]
  assign ctl_io_sync_readerSync = io_sync_readerSync; // @[DMATop.scala 66:11:@1339.4]
  assign ctl_io_sync_writerSync = io_sync_writerSync; // @[DMATop.scala 66:11:@1338.4]
  assign ctl_io_xferRead_done = readerFrontend_io_xfer_done; // @[DMATop.scala 59:26:@1315.4]
  assign ctl_io_xferWrite_done = writerFrontend_io_xfer_done; // @[DMATop.scala 60:26:@1320.4]
  assign queue_clock = clock; // @[:@1224.4]
  assign queue_reset = reset; // @[:@1225.4]
  assign queue_io_enq_valid = readerFrontend_io_dataOut_valid; // @[Decoupled.scala 295:22:@1226.4]
  assign queue_io_enq_bits = readerFrontend_io_dataOut_bits; // @[Decoupled.scala 296:21:@1227.4]
  assign queue_io_deq_ready = writerFrontend_io_dataIn_ready; // @[DMATop.scala 54:9:@1231.4]
endmodule
