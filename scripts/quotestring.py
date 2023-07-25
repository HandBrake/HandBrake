#!/usr/bin/env python3

import re
import argparse
import sys


def main():
    parser = argparse.ArgumentParser(description='Creates a quoted string suitable for inclusion in a C char*')
    parser.add_argument('infile', metavar='<input>', type=argparse.FileType('r'), help='Input file to quote')
    parser.add_argument('outfile', metavar='<output>', type=argparse.FileType('w'), nargs='?',
                        default=sys.stdout, help='Output quoted string [stdout]')
    args = parser.parse_args()

    ss = args.infile.read()
    ss = re.sub(r'\\', r'\\\\', ss)
    ss = re.sub(r'"', r'\\"', ss)
    pattern = re.compile("$", re.M)
    ss = re.sub(pattern, r'\\n"', ss)
    pattern = re.compile("^", re.M)
    ss = re.sub(pattern, "\"", ss)
    args.outfile.write(ss)

main()
