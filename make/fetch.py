###############################################################################
##
## This script is coded for minimum version of Python 2.7 .
##
## Python3 is incompatible.
##
## Authors: konablend
##
###############################################################################

import errno
import hashlib
import json
import os
import re
import tempfile
import traceback
import urllib2

from optparse import OptionGroup
from optparse import OptionParser
from sys import stderr
from sys import stdout
from urlparse import urlparse

###############################################################################

class Fetch(object):
    def __init__(self, options, urls):
        if options.disable:
            self.errln('fetching files from the network is disabled.')
        self.options = options
        self.urls = urls
        if len(self.urls) > 1:
            self.infof('\n')
        self.verbosef('OPTIONS:\n')
        self.verbosef('  disable:        %s\n' % self.options.disable)
        self.verbosef('  disable_md5:    %s\n' % self.options.disable_md5)
        self.verbosef('  config:         %s\n' % self.options.config)
        self.verbosef('  md5:            %s\n' % self.options.md5)
        self.verbosef('  output_dir:     %s\n' % self.options.output_dir)
        index = 0
        for spec in self.options.accept_url:
            self.verbosef('  accept_url[%d]:  %s\n' % (index,spec))
            index += 1
        if not self.options.accept_url:
            self.verbosef('  accept_url:     %s\n' % None)
        index = 0
        for spec in self.options.deny_url:
            self.verbosef('  deny_url[%d]:    %s\n' % (index,spec))
            index += 1
        if not self.options.deny_url:
            self.verbosef('  deny_url:       %s\n' % None)

    def run(self):
        if not self.urls:
            self.errln('no URLs specified')
        files = []
        for url in self.urls:
            files.append(self._process_url(url))
        index = 0
        for file in files:
            file.dump(index)
            index += 1
        canon = files[0].filename
        for file in files:
            if file.filename != canon:
                self.errln('URL basename is not consistent')
        scan = os.access(canon, os.F_OK)
        for file in files:
            if file.run(scan):
                return
        self.errln('%s failed.' % ('scan' if scan else 'download'))

    def errln(self, format, *args):
        s = (format % args)
        if re.match( '^.*[!?:;.]$', s ):
            stderr.write('ERROR: %s fetch stop.\n' % (s))
        else:
            stderr.write('ERROR: %s; fetch stop.\n' % (s))
        exit(1)

    def warnln(self, format, *args):
        s = (format % args)
        if re.match( '^.*[!?:;.]$', s ):
            stderr.write('WARNING: %s fetch continuing.\n' % (s))
        else:
            stderr.write('WARNING: %s; fetch continuing.\n' % (s))

    def infof(self, format, *args):
        stdout.write(format % args)

    def verbosef(self, format, *args):
        if self.options.verbose:
            stdout.write(format % args)

    def _process_url(self, url):
        props = {}
        index = 0
        while True:
            ## optional per-URL properties
            ## [key=value][...]URL
            m = re.match('(\[(\w+)=([^]]+)\])?(.*)', url[index:])
            if not m.group(1):
                break
            props[m.group(2)] = m.group(3)
            index += len(m.group(1))
        return File(url[index:], **props)

###############################################################################

class File(object):
    def __init__(self, url, **kwargs):
        self.url = url
        self.props = kwargs # not currently used
        self.filename = os.path.join(fetch.options.output_dir,os.path.basename(urlparse(self.url).path))
        self.active = True
        self.active_descr = 'default'
        self._accept()
        self._deny()

    def dump(self, index):
        fetch.verbosef('URL[%d]: %s\n' % (index,self.url))
        fetch.verbosef('  filename: %s\n' % self.filename)
        fetch.verbosef('  active: %s (%s)\n' % ('yes' if self.active else 'no',self.active_descr))

    def run(self, scan):
        if not self.active:
            return False
        try:
            if (self._scan() if scan else self._download()):
                return True
        except Exception, x:
            if fetch.options.verbose:
                traceback.print_exc()
            fetch.warnln('%s' % x)
        return False

    def _accept(self):
        if not fetch.options.accept_url:
            return
        index = 0
        for spec in fetch.options.accept_url:
            if re.match(spec, self.url):
                self.active_descr = 'via accept rule %d: %s' % (index,spec)
                return
            index += 1
        self.active = False
        self.active_descr = 'no matching spec'

    def _deny(self):
        index = 0
        for spec in fetch.options.deny_url:
            if re.match(spec, self.url):
                self.active = False
                self.active_descr = 'via deny rule %d: %s' % (index,spec)
                return
            index += 1

    def _download(self):
        fetch.infof('downloading %s to %s\n' % (self.url,self.filename))
        hasher = hashlib.md5()
        ftmp = self.filename + '.' + os.path.basename(tempfile.mktemp())
        r = urllib2.urlopen(self.url, None, 30)
        try:
            o = open(ftmp, 'w')
            info = r.info()
            try:
                content_length = int(info.getheader('Content-Length'))
            except:
                content_length = None
            data_total = 0
            while True:
                data = r.read(65536)
                if not data:
                    break
                o.write(data)
                hasher.update(data)
                data_total += len(data)
        except:
            os.unlink(ftmp)
        finally:
            for closeable in [r,o]:
                try:
                    if not closeable:
                        continue
                    closeable.close()
                except:
                    pass

        if content_length and content_length != data_total:
            fetch.warnln('expected %d bytes, got %d bytes' % (content_length,data_total))
            os.unlink(ftmp)
            return False

        if not fetch.options.disable_md5 and fetch.options.md5 and fetch.options.md5 == hasher.hexdigest():
            s = ' (verified)'
        else:
            s = ''

        fetch.infof("downloaded '%s' - %d bytes - %s%s\n" % (self.filename,data_total,hasher.hexdigest(),s))
        if not fetch.options.disable_md5 and fetch.options.md5 and fetch.options.md5 != hasher.hexdigest():
            os.unlink(ftmp)
            raise RuntimeError("expected MD5 hash '%s', got '%s'" % (fetch.options.md5, hasher.hexdigest()))

        if os.access(self.filename, os.F_OK) and not os.access(self.filename, os.W_OK):
            os.unlink(ftmp)
            raise IOError(errno.EACCES, "Permission denied: '%s'" % self.filename)

        try:
            os.rename(ftmp,self.filename)
        except:
            os.unlink(ftmp)
        return True

    def _scan(self):
        fetch.infof('scanning %s\n' % self.filename)
        hasher = hashlib.md5()
        try:
            o = open(self.filename, 'r')
            data_total = 0
            while True:
                data = o.read(65536)
                if not data:
                    break
                hasher.update(data)
                data_total += len(data)
        finally:
            o.close()

        if not fetch.options.disable_md5 and fetch.options.md5 and fetch.options.md5 == hasher.hexdigest():
            s = ' (verified)'
        else:
            s = ''

        fetch.infof("scanned '%s' - %d bytes - %s%s\n" % (self.filename,data_total,hasher.hexdigest(),s))
        if not fetch.options.disable_md5 and fetch.options.md5 and fetch.options.md5 != hasher.hexdigest():
            raise RuntimeError("expected MD5 hash '%s', got '%s'" % (fetch.options.md5, hasher.hexdigest()))
        return True

###############################################################################

def load_config(option, opt, value, parser):
    with open(value, 'r') as file:
        data = json.load(file)
        parser.values.verbose     = data[0]
        parser.values.disable     = data[1]
        parser.values.disable_md5 = data[2]
        parser.values.accept_url  = data[3]
        parser.values.deny_url    = data[4]

###############################################################################

parser = OptionParser('usage: %prog [OPTIONS...] [URL...]')

parser.description = 'Fetch files from the network.'

parser.add_option('--verbose', default=False, action='store_true', help='increase verbosity')
parser.add_option('--config', default=None, action='callback', metavar='FILE', type='str', callback=load_config, help='specify configuration file')
parser.add_option('--disable', default=False, action='store_true', help='print disabled message and exit with error')
parser.add_option('--disable-md5', default=False, action='store_true', help='disable MD5 data error detection')
parser.add_option('--md5', default=None, action='store', metavar='HASH', help='set default MD5 hash value')
parser.add_option('--accept-url', default=[], action='append', metavar='SPEC', help='accept URL regex pattern')
parser.add_option('--deny-url', default=[], action='append', metavar='SPEC', help='deny URL regex pattern')
parser.add_option('--output-dir', default='', action='store', help='specify output directory')

fetch = Fetch(*parser.parse_args())
fetch.run()
