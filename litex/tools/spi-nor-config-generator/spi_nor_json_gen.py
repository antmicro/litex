#!/usr/bin/env python3

# This file is Copyright (c) 2020 Jan Kowalewski <jkowalewski@antmicro.com>

import argparse
import json
import os
import sys
import re
from git import Git, Repo
import progressbar
import time
from multiprocessing import Process
import subprocess
from linux_spi_nor_extract import linux_extract

# This file produces JSONs with SPI NOR configs from Flashrom,
# OpenOCD and Linux SPI NOR driver and generate single JSON file
# later used by LiteSPI controller.

# List of JSON keys
json_keys = [
        'chip_name',
        'id',
        'total_size',
        'page_size',
        'dummy_bits',
        'dual_support',
        'quad_support',
        'octal_support',
        'fast_read_support',
        'addr32_support',
]

# Some features could be not mentioned in modules we parse from.
# In case there is a chip which supports a feature but it was not
# mentioned, we can override config here.
override_chip_cfg = {
        'mx25lm51245' : {
            'octal_support' : True,
        },
        'mx25r512f' : {
            'dual_support' : True,
            'quad_support' : True,

        },
        'mx25r1035f' :  {
            'dual_support' : True,
            'quad_support' : True,

        },
        'mx25r2035f' :  {
            'dual_support' : True,
            'quad_support' : True,

        },
        'mx25r4035f' :  {
            'dual_support' : True,
            'quad_support' : True,

        },
        'mx25r8035f' :  {
            'dual_support' : True,
            'quad_support' : True,

        },
        'mx25r1635f' :  {
            'dual_support' : True,
            'quad_support' : True,

        },
        'w25q64jv' :  {
            'dual_support' : True,
            'quad_support' : True,

        },
        'gd25q512mc' :  {
            'dual_support' : True,
            'quad_support' : True,

        },
        'is25lp512m' :  {
            'dual_support' : True,
            'quad_support' : True,

        },
        'is25wp512m' :  {
            'dual_support' : True,
            'quad_support' : True,

        },
}

supported_modules = {
        'linux' : {
            'url' : 'https://github.com/torvalds/linux.git',
            'patch' : 'linux_json_gen.patch',
            'build_cmds' : ['make'],
            'build_path' : 'src',
            'gen_cmd' : ['./json_gen', 'cfgs.json']
        },
        'flashrom' : {
            'url' : 'https://review.coreboot.org/cgit/flashrom.git',
            'patch' : 'flashrom_json_gen.patch',
            'build_cmds' : ['make'],
            'build_path' : 'flashrom',
            'gen_cmd' : ['./flashrom', '--dump-json', 'cfgs.json']
        },
        'openocd' : {
            'url' : 'https://git.code.sf.net/p/openocd/code',
            'patch' : 'openocd_json_gen.patch',
            'build_cmds' : ['./bootstrap', './configure', 'make'],
            'build_path' : 'openocd',
            'gen_cmd' : ['./src/openocd', '--dump_json', 'cfgs.json']
        },
}

class LogInfo(object):
    def __init__(self):
        rows, columns = os.popen('stty size', 'r').read().split()
        self.term_width = int(columns)
        self.stage_separator = "=" * self.term_width

    def std_print(self, string):
        print(string)

    def stage_print(self, string):
        print(self.stage_separator)
        print(string)
        print(self.stage_separator)

    def mark_done(self, string):
        print('\x1b[1A', end='\r')
        print(string + '[DONE]')

    def clear_line(self):
        print('\x1b[2K', end='\r')


class Progress(object):
    def _infinity(self):
        val = 0
        while True:
            yield val
            val += 1

    def _progress_func(self):
        widget = [progressbar.BouncingBar()]
        bar = progressbar.ProgressBar(widgets=widget)
        while True:
            for i in bar(self._infinity()):
                time.sleep(0.1)

    def start(self):
        self.proc = Process(target=self._progress_func,)
        self.proc.start()

    def stop(self):
        self.proc.terminate()


def prepare_modules(logger, bar, update):
    for module in supported_modules:
        if os.path.exists(module):
            if update:
                r = Repo(module)
                # Remove patches to avoid conflicts
                prepare_info = ('Updating repository: %s (%s)' % (module,
                    supported_modules[module]['url']))
                bar.start()
                logger.std_print(prepare_info)
                if supported_modules[module]['patch'] is not None:
                    r.git.execute(['git', 'reset', '--hard', 'HEAD^'])

                r.git.execute(['git', 'pull'])

                # Patch again after pull
                if supported_modules[module]['patch'] is not None:
                    r.git.execute(['git', 'am',
                        os.path.realpath(supported_modules[module]['patch'])])
                bar.stop()
                logger.clear_line()
                logger.mark_done(prepare_info)
                continue
            else:
                continue
        prepare_info = ('Cloning repository: %s (%s)' % (module,
            supported_modules[module]['url']))
        bar.start()
        logger.std_print(prepare_info)
        if module == 'linux':
            # Shallow clone Linux
            r = Repo.clone_from(supported_modules[module]['url'], module, depth=1)
        else:
            r = Repo.clone_from(supported_modules[module]['url'], module)
        if supported_modules[module]['patch'] is not None:
            r.git.execute(['git', 'am',
                os.path.realpath(supported_modules[module]['patch'])])
        bar.stop()
        logger.clear_line()
        logger.mark_done(prepare_info)

    # For Linux we need to cut out data from driver and generate new code
    # to extract configs and dump them as JSON
    logger.std_print("Generating code from Linux SPI NOR driver")
    linux_extract()


def build_modules(logger, bar, nproc):
    logfile = open('build.log', 'w')
    os.environ['LIBS'] = '-ljson-c'
    for module in supported_modules:
        build_info = ("Building %s..." % (module))
        logger.std_print(build_info)
        bar.start()
        for cmd in supported_modules[module]['build_cmds']:
            exe = [cmd]
            if cmd == 'make':
                exe.append('-j%s' % (nproc))
            b = subprocess.Popen(exe,
                                 cwd=os.path.realpath(
                                     supported_modules[module]['build_path']),
                                 stdout=logfile,
                                 stderr=subprocess.STDOUT)
            exitcode = b.wait()
            if exitcode != 0:
                p.terminate()
                logger.clear_line()
                logger.std_print("Build failed for target %s with exit code %d, please check build.log" % (module, exitcode))
                sys.exit(exitcode)
        bar.stop()
        logger.clear_line()
        logger.mark_done(build_info)


def generate_jsons(logger):
    logfile = open('gen.log', 'w')
    formater_cmd = ['jq', '-c', '.', 'cfgs.json']
    for module in supported_modules:
        gen_info = ("Generating JSON from %s..." % (module))
        logger.std_print(gen_info)
        b = subprocess.Popen(supported_modules[module]['gen_cmd'],
                             cwd=os.path.realpath(
                                 supported_modules[module]['build_path']),
                             stdout=logfile,
                             stderr=subprocess.STDOUT)
        exitcode = b.wait()
        if exitcode != 0:
            logger.std_print("JSON generation failed for target %s" % (module))
            sys.exit(exitcode)

        with open(module + '.json', 'w') as f:
            b = subprocess.Popen(formater_cmd,
                                 cwd=os.path.realpath(
                                     supported_modules[module]['build_path']),
                                 stdout=f,
                                 stderr=subprocess.STDOUT)
            exitcode = b.wait()
            if exitcode != 0:
                logger.std_print("JSON formating failed for target %s" % (module))
                sys.exit(exitcode)

        logger.mark_done(gen_info)


def generate_final_json(logger, output):
    # Merge JSONs
    all_entries = list()
    for module in supported_modules:
        with open(module + '.json') as json_file:
            for entry in json_file:
                data = json.loads(entry)
                all_entries.append(data)

    # Separate variations (e.g. w25q128fv/w25q128jv)
    plain_all_entries = list()
    for entry in all_entries:
        list_of_variations = entry["chip_name"].split('/')
        if len(list_of_variations) > 1:
            for var in list_of_variations:
                new_entry = dict()
                new_entry["chip_name"] = var
                new_entry["id"] = entry["id"]
                new_entry["total_size"] = entry["total_size"]
                new_entry["page_size"] = entry["page_size"]
                new_entry["dual_support"] = entry["dual_support"]
                new_entry["quad_support"] = entry["quad_support"]
                new_entry["octal_support"] = entry["octal_support"]
                new_entry["fast_read_support"] = entry["fast_read_support"]
                new_entry["addr32_support"] = entry["addr32_support"]
                plain_all_entries.append(new_entry)
        else:
            plain_all_entries.append(entry)


    # Search for duplicates
    seen_id = set()
    seen_name = set()
    seen_vendor = set()
    uniqs = list()
    for entry in plain_all_entries:
        if entry["id"] not in seen_id and entry["chip_name"] not in seen_name:
            uniqs.append(entry)
            seen_id.add(entry["id"])
            seen_name.add(entry["chip_name"])
        elif entry["chip_name"] not in seen_name:
            duplicate = False
            # Check if it has '.' characters
            if '.' in entry["chip_name"]:
                pattern = re.compile(entry["chip_name"])
                for u in uniqs:
                    result = pattern.match(u["chip_name"])
                    if result != None:
                        duplicate = True
                        break
            else:
                for u in uniqs:
                    pattern = re.compile(u["chip_name"])
                    result = pattern.match(entry["chip_name"])
                    if result != None:
                        duplicate = True
                        break

            if not duplicate:
                uniqs.append(entry)
                seen_name.add(entry["chip_name"])

    # Override config if possible
    for entry in uniqs:
        entry["dummy_bits"] = 8
        if entry["chip_name"] in override_chip_cfg.keys():
            for key in override_chip_cfg[entry["chip_name"]]:
                entry[key] = override_chip_cfg[entry["chip_name"]][key]

    with open(output, "w") as json_file:
        for entry in uniqs:
            json.dump(entry, json_file)

    os.system('jq -c . %s > %s' % (output, output + ".pretty"))
    os.remove(output)
    os.rename(output + ".pretty", output)

def main():
    parser = argparse.ArgumentParser(description='\
        This tool downloads modules: \
        Flashrom, OpenOCD, Linux SPI NOR driver patch them \
        and use JSONs generated from them to generate final JSON \
        used later on to configure LiteX SPI NOR controller.')
    parser.add_argument('--json-out', required=True, action='store',
            help='JSON output file name.')
    parser.add_argument('--nproc', required=False, action='store',
            help='Number of jobs to use.')
    parser.add_argument('--update-modules', required=False, action='store_true',
            help='Update cloned modules.')

    args = parser.parse_args()

    bar = Progress()
    logger = LogInfo()
    output = ("%s.json" % (args.json_out))

    logger.stage_print("Prepare modules")
    prepare_modules(logger, bar, args.update_modules)
    logger.stage_print("Build modules")
    build_modules(logger, bar, args.nproc)
    logger.stage_print("Generate JSONs")
    generate_jsons(logger)
    logger.stage_print("Create final JSON")
    generate_final_json(logger, output)
    logger.std_print("SPI NOR cofigs generated to %s" % (output))

if __name__ == "__main__":
    main()
