#!/usr/bin/env python
import argparse
import re
import os
import sys

parser = argparse.ArgumentParser()
parser.add_argument("-l", "--line", action="store_true", help="output #line directive")
parser.add_argument("files", nargs="+", help="input source file")
parser.add_argument("-e", "--external", action="append", help="external headers")
parser.add_argument("-C", "--chdir", help="change working directory")
parser.add_argument("--header", nargs="*", help="file to be inserted to the head of output")
parser.add_argument("--footer", nargs="*", help="file to be inserted to the tail of output")
args = parser.parse_args()

def parse(path, args):
    if not "ext_list" in args:
        args.ext_list = []
    if not "int_list" in args:
        args.int_list = []
    f = open(path, "r")
    l = args.line
    n = 0
    p = re.compile(r"^\s*#\s*include\s*([<\"])([^>\"]+)[>\"]")
    for line in f:
        line = line.rstrip("\r\n")
        n += 1
        m = p.match(line)
        if m:
            h = m.group(2)
            if (m.group(1) == "<") or (args.external and h in args.external):
                if not h in args.ext_list:
                    args.ext_list.append(h)
            else:
                h = os.path.normpath(os.path.join(os.path.dirname(path), h))
                if not h in args.int_list:
                    args.int_list.append(h)
                    parse(h, args)
                    l = args.line
                    continue
                else:
                    line = "/* include removed: " + h + " */"
        if l:
            print("#line " + str(n) + ' "' + path + '"')
            l = False
        print(line)
    f.close()

if args.chdir:
    os.chdir(args.chdir)

if args.header:
    for path in args.header:
        f = open(path, "r")
        print(f.read())
        f.close()

for path in args.files:
    parse(path, args)

if args.footer:
    for path in args.footer:
        f = open(path, "r")
        print(f.read())
        f.close()

