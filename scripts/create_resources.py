#! /usr/bin/python
#

import types
import os
import sys
import time
import datetime
import json
import plistlib
import argparse
from xml.parsers import expat

resources = dict()
stack = list()
stack.append(resources)

def top(ss):
    return ss[len(ss)-1]

def end_element_handler(tag):
    global stack

    if tag == "section":
        stack.pop()

def start_element_handler(tag, attr):
    global resources, stack

    current = top(stack)
    key = val = None
    if tag == "section":
        key = attr["name"]
        val = dict()
        stack.append(val)
    elif tag == "integer":
        key = attr["name"]
        val = int(attr["value"])
    elif tag == "json":
        fbase = attr["file"]
        fname = find_file(fbase)
        key = attr["name"]
        if fname != None and key != None:
            try:
                fp = open(fname)
            except Exception, err:
                print >> sys.stderr, ( "Error: %s" % str(err) )
            val = json.load(fp)
        elif fname == None:
            print >> sys.stderr, ( "Error: No such json file %s" % fbase )
            sys.exit(1)
    elif tag == "plist":
        fbase = attr["file"]
        fname = find_file(fbase)
        key = attr["name"]
        if fname != None and key != None:
            val = plistlib.readPlist(fname)
        elif fname == None:
            print >> sys.stderr, ( "Error: No such plist file %s" % fbase )
            sys.exit(1)
    elif tag == "text":
        fbase = attr["file"]
        fname = find_file(fbase)
        key = attr["name"]
        if fname != None and key != None:
            try:
                fp = open(fname)
                val = fp.read()
            except Exception, err:
                print >> sys.stderr, ( "Error: %s"  % str(err) )
                sys.exit(1)
        elif fname == None:
            print >> sys.stderr, ( "Error: No such string file %s" % fbase )
            sys.exit(1)
    elif tag == "string":
        key = attr["name"]
        val = attr["value"]

    if val != None:
        if type(current) == types.DictType:
            current[key] = val
        elif type(current) == types.TupleType:
            current.append(val)


def cdata_handler(str):
    return

def resource_parse_file(infile):
    parser = expat.ParserCreate()
    parser.StartElementHandler = start_element_handler
    parser.EndElementHandler = end_element_handler
    parser.CharacterDataHandler = cdata_handler
    parser.ParseFile(infile)

inc_list = list()

def find_file(name):
    global inc_list

    for inc_dir in inc_list:
        inc = "%s/%s" % (inc_dir, name)
        if os.path.isfile(inc):
            return inc

    if os.path.isfile(name):
        return name

    return None

def main():
    global inc_list

    parser = argparse.ArgumentParser(description='Creates a resource json from a resource list')
    parser.add_argument('-I', metavar='<inc path>', help='Include path to search for files')
    parser.add_argument('infile', metavar='<resource list>', type=argparse.FileType('r'), help='Input resources file')
    parser.add_argument('outfile', metavar='<resource json>', type=argparse.FileType('w'), nargs='?',
                        default=sys.stdout, help='Output resources json file [stdout]')
    args = parser.parse_args()

    if args.I:
        inc_list.append(args.I)

    resource_parse_file(args.infile)
    json.dump(resources, args.outfile, indent=4, sort_keys=True)

main()

