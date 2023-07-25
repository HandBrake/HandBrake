###############################################################################
##
## This script is coded for Python 2.7 through Python 3.x
##
## Authors: konablend
##
###############################################################################

import json
import os
import random
import re
import string
import sys
import traceback

import argparse

###############################################################################

class Tool(object):
    LOG_QUIET   = 0
    LOG_INFO    = 1
    LOG_VERBOSE = 2
    LOG_DEBUG   = 3

    def __init__(self):
        self.name = os.path.splitext(os.path.basename(sys.argv[0]))[0]
        self.parser = argparse.ArgumentParser()
        self.parser.add_argument('-v', '--verbose', default=Tool.LOG_INFO, action='count', dest='verbosity', help='increase verbosity')
        self.parser.add_argument('--config', default=None, action='store_const', metavar='FILE', const=lambda:'self._load_config', help='specify configuration file')

    def _parse(self):
        self.options, self.args = self.parser.parse_known_args()

    ## be sure not to use any methods referencing self.options as we are still parsing args
    def _load_config(self, option, opt, value, parser):
        with open(value, 'r') as file:
            data = json.load(file)
        parser.values.verbosity = data['verbosity']
        extend = getattr(self, '_load_config2', None)
        if extend:
            extend(parser, data)

    ## newline not required
    def errln(self, format, *args, **kwargs):
        s = (format % args)
        if re.match('^.*[!?:;.]$', s):
            if kwargs.get('exit', None) != None:
                sys.stderr.write('ERROR: %s stop.\n' % (s))
                sys.exit(1)
            sys.stderr.write('ERROR: %s continuing\n' % (s))
        else:
            if kwargs.get('exit', None) != None:
                sys.stderr.write('ERROR: %s; stop.\n' % (s))
                sys.exit(1)
            sys.stderr.write('ERROR: %s; continuing.\n' % (s))

    ## newline not required
    def warnln(self, format, *args):
        s = (format % args)
        if re.match( '^.*[!?:;.]$', s ):
            sys.stdout.write('WARNING: %s continuing.\n' % (s))
        else:
            sys.stdout.write('WARNING: %s; continuing.\n' % (s))

    ## newline required
    def infof(self, format, *args):
        if self.options.verbosity >= Tool.LOG_INFO:
            sys.stdout.write(format % args)

    ## newline not required
    def progressf(self, percent, format, *args):
        if self.options.verbosity >= Tool.LOG_INFO:
            sys.stdout.write(format % args)
            if percent >= 0:
                sys.stdout.write("  [%-20s] %.1f%%" % ('='*int(percent*20), percent*100))
            sys.stdout.write('\n')      # needed to flush on some systems
            if self.options.jobs == 1 and os.name != 'nt':
                sys.stdout.write('\033[F')  # reuse line
            sys.stdout.flush()

    ## newline required
    def verbosef(self, format, *args):
        if self.options.verbosity >= Tool.LOG_VERBOSE:
            sys.stdout.write(format % args)

    ## newline required
    def debugf(self, format, *args):
        if self.options.verbosity >= Tool.LOG_DEBUG:
            sys.stdout.write(format % args)

    def debug_exception(self, xinfo=None):
        if self.options.verbosity >= Tool.LOG_DEBUG:
            if not xinfo:
                xinfo = sys.exc_info()
            traceback.print_exception(*xinfo)

    ## generate a temporary filename - not worried about race conditions
    def mktmpname(self, filename):
        return filename + '.' + ''.join(random.choice(string.ascii_lowercase + string.digits) for _ in range(8)) + '.tmp'

###############################################################################

class ToolError(Exception):
    def __init__(self, op='unknown', text=None):
        self.op = op
        self.text = text

    def __call__(self, text):
        self.text = text
        return self

    def __str__(self):
        return self.text
