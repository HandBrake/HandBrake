###############################################################################
##
## This script is coded for Python 2.7 through Python 3.x
##
## Authors: konablend
##
###############################################################################

import hashlib
import os
import signal
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(sys.path[0], 'lib'))
import hb_distfile

###############################################################################

def signal_handler(signal, frame):
    sys.stderr.write('^C')
    sys.exit(1)

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

###############################################################################

## simple structure object
class Struct(object):
    pass

###############################################################################

class Tool(hb_distfile.Tool):
    def __init__(self):
        super(Tool, self).__init__()
        self.parser.prog = self.name
        self.parser.usage = '%(prog)s [OPTIONS] FILE'
        self.parser.description = 'Verify distfile data integrity.'
        self.parser.add_argument('--disable', default=False, action='store_true', help='do nothing and exit without error')
        self.parser.add_argument('--sha256', default=None, action='store', metavar='HASH', help='verify sha256 HASH against data')
        self._parse()

    def _load_config2(self, parser, data):
        parser.values.disable = data['disable-verify']

    def _scan(self, filename):
        self.verbosef('scanning %s\n' % filename)
        hasher = hashlib.sha256()
        with open(filename, 'rb') as o:
            data_total = 0
            while True:
                data = o.read(65536)
                if not data:
                    break
                hasher.update(data)
                data_total += len(data)
        self.verbosef('scanned %d bytes\n' % data_total)
        r = Struct()
        r.sha256 = hasher.hexdigest()
        r.size = data_total
        return r

    def _verify(self, filename):
        r = Struct()
        r.scan = self._scan(filename)
        r.status = self.options.sha256 == r.scan.sha256
        return r

    def _run(self, error):
        if self.options.disable:
            self.infof('%s failure; administratively disabled.\n' % self.name)
            sys.exit(0)
        if len(self.args) < 1:
            raise error('no file specified')
        if len(self.args) > 1:
            filename = self.args[1]
        else:
            filename = self.args[0]
        if self.options.sha256:
            error.op = 'verify'
            r = self._verify(filename)
            self.infof('sha256 (%s) = %s (%s)\n', filename, r.scan.sha256, 'pass' if r.status else 'fail; expecting %s' % self.options.sha256)
        else:
            error.op = 'scan'
            r = self._scan(filename)
            self.infof('sha256 (%s) = %s (%d bytes)\n', filename, r.sha256, r.size)

    def run(self):
        error = hb_distfile.ToolError(self.name)
        try:
            self._run(error)
        except Exception as x:
            self.debug_exception()
            self.errln('%s failure; %s' % (error.op,x), exit=1)

###############################################################################

tool = Tool()
tool.run()
