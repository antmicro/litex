from litex.build.generic_platform import *
from litex.build.lattice import LatticePlatform

# IOs ----------------------------------------------------------------------------------------------

_io = [
    ("clk8", 0, Pins("U18"), IOStandard("LVCMOS33")),

    ("user_led", 0, Pins("E3"), IOStandard("LVCMOS33")),
    ("user_led", 1, Pins("D3"), IOStandard("LVCMOS33")),
    ("user_led", 2, Pins("C3"), IOStandard("LVCMOS33")),
    ("user_led", 3, Pins("C4"), IOStandard("LVCMOS33")),
    ("user_led", 4, Pins("C2"), IOStandard("LVCMOS33")),
    ("user_led", 5, Pins("B1"), IOStandard("LVCMOS33")),
    ("user_led", 6, Pins("B20"), IOStandard("LVCMOS33")),
    ("user_led", 7, Pins("B19"), IOStandard("LVCMOS33")),
    ("user_led", 8, Pins("A18"), IOStandard("LVCMOS33")),
    ("user_led", 9, Pins("K20"), IOStandard("LVCMOS33")),
    ("user_led", 10, Pins("K19"), IOStandard("LVCMOS33")),
    ("user_led", 11, Pins("P19"), IOStandard("LVCMOS33")),
    ("user_led", 12, Pins("L18"), IOStandard("LVCMOS33")),
    ("user_led", 13, Pins("K18"), IOStandard("LVCMOS33")),

    ("user_btn", 0, Pins("E1"), IOStandard("LVCMOS33"), Misc("PULLMODE=UP")), #start
    ("user_btn", 1, Pins("D2"), IOStandard("LVCMOS33"), Misc("PULLMODE=UP")), #select
    ("user_btn", 2, Pins("D1"), IOStandard("LVCMOS33"), Misc("PULLMODE=UP")), #a
    ("user_btn", 3, Pins("E2"), IOStandard("LVCMOS33"), Misc("PULLMODE=UP")), #b
    ("user_btn", 4, Pins("F2"), IOStandard("LVCMOS33"), Misc("PULLMODE=UP")), #right
    ("user_btn", 5, Pins("G2"), IOStandard("LVCMOS33"), Misc("PULLMODE=UP")), #left
    ("user_btn", 6, Pins("C1"), IOStandard("LVCMOS33"), Misc("PULLMODE=UP")), #down
    ("user_btn", 7, Pins("F1"), IOStandard("LVCMOS33"), Misc("PULLMODE=UP")), #up

    ("pwmout", 0, Pins("T1"), IOStandard("LVCMOS33")),

    ("programn", 0, Pins("R1"), IOStandard("LVCMOS33")),

    ("usb", 0,
        Subsignal("d_p", Pins("F3")),
        Subsignal("d_n", Pins("G3")),
        Subsignal("pullup", Pins("E4")),
        Subsignal("vdet", Pins("F4")),
        IOStandard("LVCMOS33"),
    ),

    ("serial", 0,
        Subsignal("tx", Pins("U1")),
        Subsignal("rx", Pins("U2")),
        IOStandard("LVCMOS33"),
    ),

    ("fsel_d", 0, Pins("D5")),
    ("fsel_c", 0, Pins("E5")),

    ("gdpi", 0,
        Subsignal("dp", Pins("N19 L20 L16")),
        Subsignal("dn", Pins("N20 M20 L17")),
        Subsignal("ckp", Pins("P20")),
        Subsignal("ckn", Pins("R20")),
        Subsignal("ethp", Pins("T19")),
        Subsignal("ethn", Pins("R18")),
    ),

    ("lcd", 0,
        Subsignal("rd", Pins("P2")),
        Subsignal("wr", Pins("P4")),
        Subsignal("rs", Pins("P1")),
        Subsignal("rst", Pins("H2")),
        Subsignal("cs", Pins("P3")),
        Subsignal("id", Pins("J4")),
        Subsignal("fmark", Pins("G1")),
        Subsignal("blen", Pins("P5")),
        Subsignal("db", Pins(
            "J3 H1 K4 J1 K3 K2 L4 K1 L3",
            "L2 M4 L1 M3 M1 N4 N2 N3 N1")),
        IOStandard("LVCMOS33"),
    ),

    ("adc", 0,
        Subsignal("adcref1", Pins("H18"), IOStandard("LVCMOS33")),
        Subsignal("adcref2", Pins("F17"), IOStandard("LVCMOS33")),
        Subsignal("adcref3", Pins("D18"), IOStandard("LVCMOS33")),
        Subsignal("adcref4", Pins("C18"), IOStandard("LVDS")),
        Subsignal("adc4", Pins("D17"), IOStandard("LVDS")),
        Subsignal("adcrefout", Pins("A19"), IOStandard("LVCMOS33")),
    ),

    ("spiflash4x", 0,
        Subsignal("cs_n", Pins("R2")),
        Subsignal("clk", Pins("U3")),
        Subsignal("dq", Pins("W2", "V2", "Y2", "W1")),
        IOStandard("LVCMOS33"),
    ),

    ("spiflash", 0,
        Subsignal("cs_n", Pins("R2")),
        Subsignal("clk", Pins("U3")),
        Subsignal("mosi", Pins("W2")),
        Subsignal("miso", Pins("V2")),
        Subsignal("wp", Pins("Y2")),
        Subsignal("hold", Pins("W1")),
        IOStandard("LVCMOS33"),
    ),

    ("spipsram4x_a", 0,
        Subsignal("cs_n", Pins("D20")),
        Subsignal("clk", Pins("E20")),
        Subsignal("dq", Pins("E19", "D19", "C20", "F19")),
        IOStandard("LVCMOS33"),
    ),

    ("spipsram_a", 0,
        Subsignal("cs_n", Pins("D20")),
        Subsignal("clk", Pins("E20")),
        Subsignal("mosi", Pins("E19")),
        Subsignal("miso", Pins("D19")),
        IOStandard("LVCMOS33"),
    ),

    ("spipsram4x_b", 0,
        Subsignal("cs_n", Pins("F20")),
        Subsignal("clk", Pins("J19")),
        Subsignal("dq", Pins("J20", "G19", "G20", "H20")),
        IOStandard("LVCMOS33"),
    ),

    ("spipsram_b", 0,
        Subsignal("cs_n", Pins("F20")),
        Subsignal("clk", Pins("J19")),
        Subsignal("mosi", Pins("J20")),
        Subsignal("miso", Pins("G19")),
        IOStandard("LVCMOS33"),
    ),

    ("irda", 0,
        Subsignal("tx", Pins("R16"), IOStandard("LVCMOS33")),
        Subsignal("rx", Pins("U16"), IOStandard("LVCMOS33"), Misc("PULLMODE=UP")),
        Subsignal("sd", Pins("T16"), IOStandard("LVCMOS33"), Misc("PULLMODE=UP")),
    ),

]

# Connectors ---------------------------------------------------------------------------------------

_connectors = [
    ("pmod", "A15 C16 A14 D16 B15 C15 A13 B13"),
    ("sao0", {
        "gpio1" : "A2",
        "gpio2" : "A3",
        "gpio3" : "B4",
        "drm" : "A4",
        "sda" : "B2",
        "scl" : "B3",
    } ),
    ("sao1", {
        "gpio1" : "B18",
        "gpio2" : "A17",
        "gpio3" : "B16",
        "drm" : "C17",
        "sda" : "B17",
        "scl" : "A16",
    } ),
    ("genio", {
        "genio0" : "C5",
        "genio1" : "B5",
        "genio2" : "A5",
        "genio3" : "C6",
        "genio4" : "B6",
        "genio5" : "A6",
        "genio6" : "D6",
        "genio7" : "C7",
        "genio8" : "A7",
        "genio9" : "C8",
        "genio10" : "B8",
        "genio11" : "A8",
        "genio12" : "D9",
        "genio13" : "C9",
        "genio14" : "B9",
        "genio15" : "A9",
        "genio16" : "D10",
        "genio17" : "C10",
        "genio18" : "B10",
        "genio19" : "A10",
        "genio20" : "D11",
        "genio21" : "C11",
        "genio22" : "B11",
        "genio23" : "A11",
        "genio24" : "G18",
        "genio25" : "H17",
        "genio26" : "B12",
        "genio27" : "A12",
        "genio28" : "E17",
        "genio29" : "C14",
    } )
]

# Platform -----------------------------------------------------------------------------------------

class Platform(LatticePlatform):
    default_clk_name = "clk8"
    default_clk_period = 1e9/8e6

    def __init__(self, device="LFE5U-45F", **kwargs):
        LatticePlatform.__init__(self, device + "-8BG381C", _io, **kwargs)
