#!/bin/python

import os
import sys

def linux_extract():
    code = str()
    exports = list()
    driver_path = os.path.join("linux", "drivers", "mtd", "spi-nor", "spi-nor.c")

    # Constant parts of code
    header = \
"""#include <json-c/json.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;
typedef signed char		s8;
typedef short			s16;
typedef int			s32;
typedef long long		s64;

#ifndef BIT
	#define BIT(x)	(1 << (x))
#endif

#define SPI_NOR_MAX_ID_LEN	6

"""

    function = \
"""
int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("Please pass <file.json> argument\\n\\r");
		return 1;
	}
	char *dump_output = argv[1];
	FILE *fp;
	printf("Dumping configurations of SPI devices to %s.\\n", dump_output);
	fp = fopen(dump_output, "w+");
	const struct flash_info *chip = NULL;
	for (chip = spi_nor_ids; chip && chip->name; chip++) {
		struct json_object *chip_cfg = json_object_new_object();
		struct json_object *chip_name = json_object_new_string(chip->name);
		int64_t chip_id =
			((int64_t)chip->id[0] << 40)
			| ((int64_t)chip->id[1] << 32)
			| ((int64_t)chip->id[2] << 24)
			| ((int64_t)chip->id[3] << 16)
			| ((int64_t)chip->id[4] << 8)
			| (int64_t)chip->id[5];
		struct json_object *id = json_object_new_int64(chip_id);
		struct json_object *total_size = json_object_new_int64(chip->sector_size * chip->n_sectors);
		struct json_object *page_size = json_object_new_int64(chip->page_size);
		bool dualsupport = (chip->flags & SPI_NOR_DUAL_READ);
		bool quadsupport = (chip->flags & SPI_NOR_QUAD_READ);
		bool octalsupport = (chip->flags & SPI_NOR_OCTAL_READ);
		bool fastreadsupport = !(chip->flags & SPI_NOR_NO_FR);
		bool addr32b_support = (chip->flags & SPI_NOR_4B_OPCODES);
		struct json_object *dual_rw_support = json_object_new_boolean(dualsupport);
		struct json_object *quad_rw_support = json_object_new_boolean(quadsupport);
		struct json_object *octal_rw_support = json_object_new_boolean(octalsupport);
		struct json_object *fast_read_support = json_object_new_boolean(fastreadsupport);
		struct json_object *addr32_support = json_object_new_boolean(addr32b_support);

		json_object_object_add(chip_cfg, "chip_name", chip_name);
		json_object_object_add(chip_cfg, "id", id);
		json_object_object_add(chip_cfg, "total_size", total_size);
		json_object_object_add(chip_cfg, "page_size", page_size);
		json_object_object_add(chip_cfg, "dual_support", dual_rw_support);
		json_object_object_add(chip_cfg, "quad_support", quad_rw_support);
		json_object_object_add(chip_cfg, "octal_support", octal_rw_support);
		json_object_object_add(chip_cfg, "fast_read_support", fast_read_support);
		json_object_object_add(chip_cfg, "addr32_support", addr32_support);
		fputs(json_object_to_json_string(chip_cfg), fp);
	}
	fclose(fp);
	return 0;
}

"""

    structs = ["spi_nor_ids[]", "struct flash_info {"]
    defines = ["INFO"]
    cutting_out = False

    # Cut out defines and structs
    with open(driver_path) as f:
        terminator = str()
        for line in f:
            if (cutting_out):
                if (terminator == "\\"):
                    if (not terminator in line):
                        cutting_out = False
                else:
                    if (terminator in line):
                        cutting_out = False
                exports.append(line)

            for s in structs:
                if (not cutting_out and s in line):
                    exports.append(line)
                    cutting_out = True
                    terminator = "};"
                    break

            for d in defines:
                if (not cutting_out and d in line and "define" in line):
                    exports.append(line)
                    cutting_out = True
                    terminator = "\\"
                    break

    # Remove non-relevant fixups
    for line in exports:
        if (".fixups" in line):
            if ("}," in line):
                code += \
"""        },
"""
            else:
                continue
        else:
            code += line

    code = header + code + function

    if not os.path.exists("src"):
        os.mkdir("src")

    # Save generated code
    f = open(os.path.join("src", "json_gen.c"), "w+")
    f.write(code)
    f.close()

    # Create a Makefile
    make = """all:
\tgcc json_gen.c -o json_gen $(LIBS)"""
    f = open(os.path.join("src", "Makefile"), "w+")
    f.write(make)
    f.close()

    print("Generated code from Linux stored in src/json_gen.c file.")
