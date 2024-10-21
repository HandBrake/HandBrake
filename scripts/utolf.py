#/usr/bin/env python3

import os
import sys

def main():
    filenames = sys.argv[1:]
    if filenames:
        convert(filenames)
    else:
        print("Usage: <command> filename [filename ...]")

def convert(files):
    for file in files:
        print(file)
        with open(file, 'r') as infile, \
             open(file + '.tmp', 'w', newline='\n') as outfile:
            outfile.writelines(infile.readlines())
        os.replace(file + '.tmp', file)

if __name__ == "__main__":
    main()
