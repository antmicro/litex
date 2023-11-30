include ../include/generated/variables.mak
include $(SOC_DIRECTORY)/software/common.mak

OBJECTS = sdram.o bist.o sdram_dbg.o sdram_spd.o utils.o accessors.o sdram_rcd.o
OBJECTS += ddr5/ddr5_spd_parse.o ddr5/eye_detection_helper.o ddr5_training.o ddr5_helpers.o


all: liblitedram.a

liblitedram.a: $(OBJECTS)
	$(AR) crs liblitedram.a $(OBJECTS)

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

ddr5/%.o: $(LIBLITEDRAM_DIRECTORY)/ddr5/%.c
	mkdir -p ddr5
	$(compile)

%.o: $(LIBLITEDRAM_DIRECTORY)/%.c
	$(compile)

%.o: %.S
	$(assemble)

.PHONY: all clean

clean:
	$(RM) $(OBJECTS) liblitedram.a .*~ *~
