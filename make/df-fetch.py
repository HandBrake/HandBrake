###############################################################################
##
## This script is coded for Python 2.7 through Python 3.x
##
## Authors: konablend
##
###############################################################################

import glob
import hashlib
import random
import re
import os
import signal
import sys
import time

try:
    from urllib.request import urlopen
except ImportError:
    from urllib2 import urlopen

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

## track resources and ensure cleanup
##
## - items are lambdas accepting no args
## - item order of insertion is important
## - cleanup will run in reverse order of insertion
## - item update does not effect order
##
class Ensure(object):
    def __init__(self):
        super(Ensure, self).__setattr__('_items', [])

    def __delattr__(self, key):
        if key in self.__dict__:
            self._items.remove(self.__dict__[key])
        super(Ensure, self).__delattr__(key)

    def __setattr__(self, key, value):
        if not key in self.__dict__:
            self._items.insert(0, value)
        super(Ensure, self).__setattr__(key, value)

    def run(self):
        for item in self._items:
            try:
                item()
            except Exception:
                pass

###############################################################################

class Tool(hb_distfile.Tool):
    def __init__(self):
        super(Tool, self).__init__()
        self.parser.prog = self.name
        self.parser.usage = '%(prog)s [OPTIONS] URL...'
        self.parser.description = 'Fetch and verify distfile data integrity.'
        self.parser.add_argument('--disable', default=False, action='store_true', help='do nothing and exit with error')
        self.parser.add_argument('--jobs', default=1, action='store', metavar='N', help='allow N download jobs at once')
        self.parser.add_argument('--sha256', default=None, action='store', metavar='HASH', help='verify sha256 HASH against data')
        self.parser.add_argument('--accept-url', default=[], action='append', metavar='SPEC', help='accept URL regex pattern')
        self.parser.add_argument('--deny-url', default=[], action='append', metavar='SPEC', help='deny URL regex pattern')
        self.parser.add_argument('--exhaust-url', default=None, action='store_true', help='try all active distfiles')
        self.parser.add_argument('--output', default=None, action='store', metavar='FILE', help='write to FILE')
        self._parse()

    def _load_config2(self, parser, data):
        parser.values.disable     = data['disable-fetch']
        parser.values.jobs        = data['jobs']
        parser.values.accept_url  = data['accept-url']
        parser.values.deny_url    = data['deny-url']

    def _run(self, error):
        # throttle instances
        if tool.options.jobs < 1:
            tool.options.jobs = 1
        if tool.options.jobs > 20:
            tool.options.jobs = 20
        dirname = os.path.dirname(tool.options.output)
        time.sleep(random.uniform(0.1,2))
        active = len(glob.glob(dirname + '/*.tmp'))
        while active >= tool.options.jobs:
            time.sleep(2)
            active = len(glob.glob(dirname + '/*.tmp'))
        # handle disabled
        if self.options.disable:
            raise error('administratively disabled')
        ## create URL objects and keep active
        urls = []
        i = 0
        for arg in self.args[1:]:
            url = URL(arg, i)
            if url.active:
                urls.append(url)
            i += 1
        ## try each URL until first success
        error.op = 'download'
        if not urls:
            raise error('nothing to download')
        while urls:
            url = urls.pop(0)
            try:
                url.download(error)
                if not self.options.exhaust_url:
                    break
            except Exception as x:
                ## propagate exception if no remaining urls
                if not urls:
                    raise
                self.errln('%s failure; %s' % (error.op,x))

    def run(self):
        error = hb_distfile.ToolError(self.name)
        try:
            self._run(error)
        except Exception as x:
            self.debug_exception()
            self.errln('%s failure; %s' % (error.op,x), exit=1)

###############################################################################

class URL(object):
    def __init__(self, url, index):
        self.index = index
        self.url = url
        self.active = True
        self.rule = 'none'
        self._accept()
        self._deny()
        tool.verbosef('URL[%d]: %s\n' % (self.index,self.url))
        tool.verbosef('  active: %s\n' % ('yes' if self.active else 'no'))
        tool.verbosef('  rule:   %s\n' % (self.rule))

    def _accept(self):
        if not tool.options.accept_url:
            return
        index = 0
        for spec in tool.options.accept_url:
            if re.search(spec, self.url):
                self.rule = 'via accept rule[%d]: %s' % (index,spec)
                return
            index += 1
        self.active = False
        self.rule = 'no matching accept rule'

    def _deny(self):
        index = 0
        for spec in tool.options.deny_url:
            if re.search(spec, self.url):
                self.active = False
                self.rule = 'via deny rule[%d]: %s' % (index,spec)
                return
            index += 1

    def _download(self, error, ensure):
        filename = tool.options.output
        hasher = hashlib.sha256()
        if filename:
            tool.infof('downloading %s to %s\n' % (self.url,filename))
            ftmp = tool.mktmpname(filename)
            hout = open(ftmp, 'wb')
            ensure.unlink_ftmp = lambda: os.unlink(ftmp)
            ensure.close_hout = lambda: hout.close()
        else:
            tool.infof('downloading %s\n' % (self.url))
        hin = urlopen(self.url, None, 30)
        ensure.close_hin = lambda: hin.close()
        info = hin.info()
        try:
            content_length = int(info.getheader('Content-Length'))
        except:
            content_length = None
        data_total = 0
        data_total_percent = 0.0
        while True:
            data = hin.read(65536)
            if not data:
                break
            if filename:
                hout.write(data)
            hasher.update(data)
            data_total += len(data)
            if content_length and content_length > 0:
                data_total_percent = float(data_total) / content_length
                if data_total_percent >= 1 and data_total < content_length:
                    data_total_percent = 0.999999
            else:
                data_total_percent = -1
            tool.progressf(data_total_percent, 'downloading...  %9d bytes' % data_total)
        if content_length and content_length != data_total:
            raise error('expected %d bytes, got %d bytes' % (content_length,data_total))
        s = 'download total: %9d bytes\n' % data_total
        if filename:
            s += 'sha256 (%s) = %s' % (filename,hasher.hexdigest())
        else:
            s += 'sha256 = %s' % (hasher.hexdigest())
        if tool.options.sha256:
            sha256_pass = tool.options.sha256 == hasher.hexdigest()
            s += ' (%s)' % ('pass' if sha256_pass else 'fail; expecting %s' % tool.options.sha256)
        tool.infof('%s\n' % s)
        if filename and tool.options.sha256:
            if sha256_pass:
                if os.access(filename, os.F_OK) and not os.access(filename, os.W_OK):
                    raise error("permission denied: '%s'" % filename)
            else:
                raise error("expected sha256 hash '%s', got '%s'" % (tool.options.sha256, hasher.hexdigest()))
            os.rename(ftmp,filename)
            del ensure.unlink_ftmp

    def download(self, error):
        ensure = Ensure()
        try:
            self._download(error, ensure)
        finally:
            ensure.run()

###############################################################################

tool = Tool()
tool.run()
