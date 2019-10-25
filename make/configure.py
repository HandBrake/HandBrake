###############################################################################
##
## This script is coded for Python 2.7 through Python 3.x
##
## Authors: konablend
##
###############################################################################

import fnmatch
import glob
import json
import os
import platform
import random
import re
import string
import subprocess
import sys
import time
from datetime import datetime, timedelta

import argparse
from sys import stderr
from sys import stdout

class AbortError( Exception ):
    def __init__( self, format, *args ):
        self.value = format % args
    def __str__( self ):
        return self.value

###############################################################################
##
## Main configure object.
##
## dir = containing this configure script
## cwd = current working dir at time of script launch
##
class Configure( object ):
    OUT_QUIET   = 0
    OUT_INFO    = 1
    OUT_VERBOSE = 2

    def __init__( self, verbose ):
        self._log_info    = []
        self._log_verbose = []
        self._record      = False

        self.verbose = verbose
        self.dir = os.path.dirname( sys.argv[0] )
        self.cwd = os.getcwd()

        self.build_dir = '.'

        ## compute src dir which is 2 dirs up from this script
        self.src_dir = os.path.normpath( sys.argv[0] )
        for i in range( 2 ):
            self.src_dir = os.path.dirname( self.src_dir )
        if len( self.src_dir ) == 0:
            self.src_dir = os.curdir

    def _final_dir( self, chdir, dir ):
        dir = os.path.normpath( dir )
        if not os.path.isabs( dir ):
            if os.path.isabs( chdir ):
                dir = os.path.normpath( os.path.abspath(dir ))
            else:
                dir = os.path.normpath( self.relpath( dir, chdir ))
        return dir

    ## output functions
    def errln( self, format, *args ):
        s = (format % args)
        if re.match( '^.*[!?:;.]$', s ):
            stderr.write( 'ERROR: %s configure stop.\n' % (s) )
        else:
            stderr.write( 'ERROR: %s; configure stop.\n' % (s) )
        self.record_log()
        sys.exit( 1 )
    def infof( self, format, *args ):
        line = format % args
        self._log_verbose.append( line )
        if self.verbose >= Configure.OUT_INFO:
            self._log_info.append( line )
            stdout.write( line )
    def verbosef( self, format, *args ):
        line = format % args
        self._log_verbose.append( line )
        if self.verbose >= Configure.OUT_VERBOSE:
            stdout.write( line )

    ## doc is ready to be populated
    def doc_ready( self ):
        ## compute final paths as they are after chdir into build
        self.build_final  = os.curdir
        self.src_final    = self._final_dir( self.build_dir, self.src_dir )
        self.prefix_final = self._final_dir( self.build_dir, self.prefix_dir )
        if build_tuple.match( '*-*-darwin*' ):
            self.xcode_prefix_final = self._final_dir( self.build_dir, self.xcode_prefix_dir )

        self.infof( 'compute: makevar SRC/    = %s\n', self.src_final )
        self.infof( 'compute: makevar BUILD/  = %s\n', self.build_final )
        self.infof( 'compute: makevar PREFIX/ = %s\n', self.prefix_final )
        if build_tuple.match( '*-*-darwin*' ):
            self.infof( 'compute: makevar XCODE.prefix/ = %s\n', self.xcode_prefix_final )

    ## perform chdir and enable log recording
    def chdir( self ):
        if os.path.abspath( self.build_dir ) == os.path.abspath( self.src_dir ):
            self.errln( 'build (scratch) directory must not be the same as top-level source root!' )

        if self.build_dir != os.curdir:
            if os.path.exists( self.build_dir ):
                if not options.force:
                    self.errln( 'build directory already exists: %s (use --force to overwrite)', self.build_dir )
            else:
                self.mkdirs( self.build_dir )
            self.infof( 'chdir: %s\n', self.build_dir )
            os.chdir( self.build_dir )

        ## enable logging
        self._record = True

    def mkdirs( self, dir ):
        if len(dir) and not os.path.exists( dir ):
            self.infof( 'mkdir: %s\n', dir )
            os.makedirs( dir )

    def open( self, *args ):
        dir = os.path.dirname( args[0] )
        if len(args) > 1 and args[1].find('w') != -1:
            self.mkdirs( dir )
        m = re.match( '^(.*)\.tmp\..{8}$', args[0] )
        if m:
            self.infof( 'write: %s\n', m.group(1) )
        else:
            self.infof( 'write: %s\n', args[0] )

        try:
            return open( *args )
        except Exception as x:
            self.errln( 'open failure: %s', x )

    def record_log( self ):
        if not self._record:
            return
        self._record = False
        self.verbose = Configure.OUT_QUIET
        log_info_file = self.open( 'log/config.info.txt', 'w' )
        for line in self._log_info:
            log_info_file.write( line )
        log_info_file.close()
        log_verbose_file = self.open( 'log/config.verbose.txt', 'w' )
        for line in self._log_verbose:
            log_verbose_file.write( line )
        log_verbose_file.close()

    ## Find executable by searching path.
    ## On success, returns full pathname of executable.
    ## On fail, returns None.
    def findExecutable( self, name ):
        if len( os.path.split(name)[0] ):
            if os.access( name, os.X_OK ):
                return name
            return None

        path = os.getenv( 'PATH' ) or os.defpath
        for dir in path.split( os.pathsep ):
            f = os.path.join( dir, name )
            if os.access( f, os.X_OK ):
                return f
        return None

    ## taken from python2.6 -- we need it
    def relpath( self, path, start=os.curdir ):
        """Return a relative version of a path"""

        if not path:
            raise ValueError("no path specified")

        start_list = os.path.abspath(start).split(os.sep)
        path_list = os.path.abspath(path).split(os.sep)

        # Work out how much of the filepath is shared by start and path.
        i = len(os.path.commonprefix([start_list, path_list]))

        rel_list = [os.pardir] * (len(start_list)-i) + path_list[i:]
        if not rel_list:
            return os.curdir
        return os.path.join(*rel_list)

    ## update with parsed cli options
    def update_cli( self, options ):
        self.src_dir    = os.path.normpath( options.src )
        self.build_dir  = os.path.normpath( options.build )
        self.prefix_dir = os.path.normpath( options.prefix )
        if build_tuple.match( '*-*-darwin*' ) and options.cross is None:
            self.xcode_prefix_dir = os.path.normpath( options.xcode_prefix )
        if options.sysroot != None:
                self.sysroot_dir = os.path.normpath( options.sysroot )
        else:
                self.sysroot_dir = ""

        try:
                self.minver = options.minver
        except:
                self.minver = ""

        ## special case if src == build: add build subdir
        if os.path.abspath( self.src_dir ) == os.path.abspath( self.build_dir ):
            self.build_dir = os.path.join( self.build_dir, 'build' )

    ## generate a temporary filename - not worried about race conditions
    def mktmpname( self, filename ):
        return filename + '.tmp.' + ''.join(random.choice(string.ascii_lowercase + string.digits) for _ in range(8))

###############################################################################
##
## abstract action
##
## pretext = text which immediately follows 'probe:' output prefix
## abort   = if true configure will exit on probe fail
## head    = if true probe session is stripped of all but first line
## session = output from command, including stderr
## fail    = true if probe failed
##
class Action( object ):
    actions = []

    def __init__( self, category, pretext='unknown', abort=False, head=False ):
        if self not in Action.actions:
            Action.actions.append( self )

        self.category = category
        self.pretext  = pretext
        self.abort    = abort
        self.head     = head
        self.session  = None

        self.run_done = False
        self.fail     = True
        self.msg_fail = 'fail'
        self.msg_pass = 'pass'
        self.msg_end  = 'end'

    def _actionBegin( self ):
        cfg.infof( '%s: %s...', self.category, self.pretext )

    def _actionEnd( self ):
        if self.fail:
            cfg.infof( '(%s) %s\n', self.msg_fail, self.msg_end )
            if self.abort:
                self._dumpSession( cfg.infof )
                cfg.errln( 'unable to continue' )
            self._dumpSession( cfg.verbosef )
            self._failSession()
        else:
            cfg.infof( '(%s) %s\n', self.msg_pass, self.msg_end )
            self._dumpSession( cfg.verbosef )

    def _dumpSession( self, printf ):
        if self.session and len(self.session):
            for line in self.session:
                printf( '  : %s\n', line )
        else:
            printf( '  : <NO-OUTPUT>\n' )

    def _parseSession( self ):
        pass

    def _failSession( self ):
        pass

    def run( self ):
        if self.run_done:
            return
        self.run_done = True
        self._actionBegin()
        self._action()
        if not self.fail:
            self._parseSession()
        self._actionEnd()

###############################################################################
##
## base probe: anything which runs in shell.
##
## pretext = text which immediately follows 'probe:' output prefix
## command = full command and arguments to pipe
## abort   = if true configure will exit on probe fail
## head    = if true probe session is stripped of all but first line
## session = output from command, including stderr
## fail    = true if probe failed
##
class ShellProbe( Action ):
    def __init__( self, pretext, command, abort=False, head=False ):
        super( ShellProbe, self ).__init__( 'probe', pretext, abort, head )
        self.command = command

    def _action( self ):
        ## pipe and redirect stderr to stdout; effects communicate result
        pipe = subprocess.Popen( self.command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT )

        ## read data into memory buffers, only first element (stdout) data is used
        data = pipe.communicate()
        self.fail = pipe.returncode != 0

        if data[0]:
            self.session = data[0].splitlines()
        else:
            self.session = []

        if pipe.returncode:
            self.msg_end = 'code %d' % (pipe.returncode)

    def _dumpSession( self, printf ):
        printf( '  + %s\n', self.command )
        super( ShellProbe, self )._dumpSession( printf )

###############################################################################
##
## Compile test probe: determine if compile time feature is supported
##
## returns true if feature successfully compiles
##
##
class CCProbe( Action ):
    def __init__( self, pretext, command, test_file ):
        super( CCProbe, self ).__init__( 'probe', pretext )
        self.command = command
        self.test_file = test_file

    def _action( self ):
        ## write program file
        with open( 'conftest.c', 'w' ) as out_file:
            out_file.write( self.test_file )
        ## pipe and redirect stderr to stdout; effects communicate result
        pipe = subprocess.Popen( '%s -c -o conftest.o conftest.c' % self.command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT )

        ## read data into memory buffers, only first element (stdout) data is used
        data = pipe.communicate()
        self.fail = pipe.returncode != 0

        if data[0]:
            self.session = data[0].splitlines()
        else:
            self.session = []

        if pipe.returncode:
            self.msg_end = 'code %d' % (pipe.returncode)
        os.remove( 'conftest.c' )
        if not self.fail:
            os.remove( 'conftest.o' )

    def _dumpSession( self, printf ):
        printf( '  + %s\n', self.command )
        super( CCProbe, self )._dumpSession( printf )


###############################################################################
##
## Compile test probe: determine if compile time feature is supported
##
## returns true if feature successfully compiles
##
##
def PkgConfigTest(args, lib):
    msg_end = ''
    if Tools.pkgconfig.fail:
        fail = True
        session = []
        msg_end = 'No pkg-config'
        return fail, msg_end, session

    ## pipe and redirect stderr to stdout; effects communicate result
    pipe = subprocess.Popen( '%s %s %s' %
            (Tools.pkgconfig.pathname, args, lib),
            shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT )

    ## read data into memory buffers, only first element (stdout)
    ## data is used
    data = pipe.communicate()
    fail = pipe.returncode != 0

    if data[0]:
        session = data[0].splitlines()
    else:
        session = []

    if pipe.returncode:
        msg_end = 'code %d' % (pipe.returncode)

    return fail, msg_end, session

class PkgConfigProbe( Action ):
    def __init__( self, pretext, args, lib ):
        super( PkgConfigProbe, self ).__init__( 'probe', pretext )
        self.args = args
        self.lib = lib

    def _action( self ):
        self.fail, self.msg_end, self.session = PkgConfigTest(self.args,
                                                              self.lib)

    def _dumpSession( self, printf ):
        printf( '  + %s %s\n', Tools.pkgconfig.pathname, self.args )
        super( PkgConfigProbe, self )._dumpSession( printf )


###############################################################################
##
## Compile test probe: determine if compile time feature is supported
##
## returns true if feature successfully compiles
##
##
def LDTest(command, lib, test_file):
    ## write program file
    with open( 'conftest.c', 'w' ) as out_file:
        out_file.write( test_file )
    ## pipe and redirect stderr to stdout; effects communicate result
    pipe = subprocess.Popen( '%s -o conftest conftest.c %s' % (command, lib), shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT )

    ## read data into memory buffers, only first element (stdout) data is used
    data = pipe.communicate()
    fail = pipe.returncode != 0

    if data[0]:
        session = data[0].splitlines()
    else:
        session = []

    msg_end = ''
    if pipe.returncode:
        msg_end = 'code %d' % (pipe.returncode)

    os.remove( 'conftest.c' )
    if not fail:
        try:
            os.remove( 'conftest.exe' )
        except:
            pass
        try:
            os.remove( 'conftest' )
        except:
            pass
    return (fail, msg_end, session)

class LDProbe( Action ):
    def __init__( self, pretext, command, lib, test_file ):
        super( LDProbe, self ).__init__( 'probe', pretext )
        self.command = command
        self.test_file = test_file
        self.lib = lib

    def _action( self ):
        self.fail, self.msg_end, self.session = LDTest(
                            self.command, self.lib, self.test_file)

    def _dumpSession( self, printf ):
        printf( '  + %s\n', self.command )
        super( LDProbe, self )._dumpSession( printf )


###############################################################################
##
## Basic library existence check
##
## returns true if feature successfully compiles
##
##
class ChkLib( Action ):
    def __init__( self, pretext, command, lib, test_file, abort=False ):
        super( ChkLib, self ).__init__( 'probe', pretext, abort=abort )
        self.command = command
        self.test_file = test_file
        self.lib = lib

    def _action( self ):
        ## First try pkg-config
        if not Tools.pkgconfig.fail:
            self.fail, self.msg_end, self.session = PkgConfigTest(
                                                        '--libs', self.lib)
            if not self.fail:
                return

        ## If pkg-config fails, try compiling and linking test file
        self.fail, self.msg_end, session = LDTest(
                            self.command, '-l%s' % self.lib, self.test_file)
        self.session.append(session)

###############################################################################
##
## GNU build tuple probe: determine canonical platform type
##
## example results from various platforms:
##
##   x86_64-apple-darwin15.6.0  (Mac OS X 10.11.6 Intel)
##   x86_64-apple-darwin16.6.0  (macOS 10.12.6 Intel)
##   i686-pc-cygwin             (Cygwin, Microsoft Vista)
##   x86_64-unknown-linux-gnu   (Linux, Fedora 10 x86_64)
##
class BuildTupleProbe( ShellProbe, list ):
    GNU_TUPLE_RE = '([^-]+)-?([^-]*)-([^0-9-]+)([^-]*)-?([^-]*)'

    def __init__( self ):
        super( BuildTupleProbe, self ).__init__( 'build tuple', '%s/config.guess' % (cfg.dir), abort=True, head=True )

    def _parseSession( self ):
        self.spec = self.session[0].decode('utf-8') if self.session else ''

        ## grok GNU build tuples
        m = re.match( BuildTupleProbe.GNU_TUPLE_RE, self.spec )
        if not m:
            self.fail = True
            self.msg_end = 'invalid build tuple: %s' % (self.spec)
            return

        self.msg_end = self.spec

        ## assign tuple from regex
        self[:] = m.groups()

        ## for clarity
        self.machine = self[0]
        self.vendor  = self[1]
        self.system  = self[2]
        self.release = self[3]
        self.extra   = self[4]

        ## nice formal name for 'system'
        self.systemf = platform.system()

        if self.match( '*-*-cygwin*' ):
            self.systemf = self[2][0].upper() + self[2][1:]

    ## glob-match against spec
    def match( self, *specs ):
        for spec in specs:
            if fnmatch.fnmatch( self.spec, spec ):
                return True
        return False

###############################################################################

class HostTupleAction( Action, list ):
    def __init__( self, cross=None ):
        super( HostTupleAction, self ).__init__( 'compute', 'host tuple', abort=True )
        # Initialize, but allow to be reset by options
        self.setHost(cross)

    def setHost( self, cross ):
        ## check if --cross spec was used; must maintain 5-tuple compatibility with regex
        if cross is not None:
            self.spec = os.path.basename( cross ).rstrip( '-' )
        else:
            self.spec = build_tuple.spec

        ## grok GNU build tuples
        m = re.match( BuildTupleProbe.GNU_TUPLE_RE, self.spec )
        if not m:
            self.msg_end = 'invalid build tuple: %s' % (self.spec)
            return

        self.msg_end = self.spec

        ## assign tuple from regex
        self[:] = m.groups()

        ## for clarity
        self.machine = self[0]
        self.vendor  = self[1]
        self.system  = self[2]
        self.release = self[3]
        self.extra   = self[4]
        self.systemf = build_tuple.systemf

        try:
            self.machine = arch.mode.mode
        except NameError:
            pass

        ## when cross we need switch for platforms
        if cross is not None:
            if self.match( '*mingw*' ):
                self.systemf = 'MinGW'
            elif self.systemf:
                self.systemf = self.systemf.capitalize()

        self.title = '%s %s' % (self.systemf,self.machine)
        self.fail = False

        self.spec = ('%s-%s-%s%s-%s' % (self.machine, self.vendor, self.system,
                                        self.release, self.extra)).rstrip('-')

    def _action( self ):
        try:
            self.setHost(options.cross)
        except NameError:
            self.setHost(None)

    ## glob-match against spec
    def match( self, *specs ):
        for spec in specs:
            if fnmatch.fnmatch( self.spec, spec ):
                return True
        return False

###############################################################################
##
## value wrapper; value is accepted only if one of host specs matches
## otherwise it is None (or a keyword-supplied val)
##
## result is attribute 'value'
##
class IfHost( object ):
    def __init__( self, value, *specs, **kwargs ):
        self.value = kwargs.get('none',None)
        for spec in specs:
            if host_tuple.match( spec ):
                self.value = value
                break

    def __nonzero__( self ):
        return self.value != None

    def __str__( self ):
        return self.value

class IfBuild( object ):
    def __init__( self, value, *specs, **kwargs ):
        self.value = kwargs.get('none',None)
        for spec in specs:
            if build_tuple.match( spec ):
                self.value = value
                break

    def __nonzero__( self ):
        return self.value != None

    def __str__( self ):
        return self.value

###############################################################################
##
## platform conditional value; loops through list of tuples comparing
## to first host match and sets value accordingly; the first value is
## always default.
##
class ForHost( object ):
    def __init__( self, default, *tuples ):
        self.value = default
        for tuple in tuples:
            if host_tuple.match( tuple[1] ):
                self.value = tuple[0]
                break

    def __str__( self ):
        return self.value

###############################################################################

class ArchAction( Action ):
    def __init__( self ):
        super( ArchAction, self ).__init__( 'compute', 'available architectures', abort=True )
        self.mode = SelectMode( 'architecture', (host_tuple.machine,host_tuple.spec) )

    def _action( self ):
        self.fail = False

        ## some match on system should be made here; otherwise we signal a warning.
        if host_tuple.match( '*-*-cygwin*' ):
            pass
        elif host_tuple.match( '*-*-mingw*' ):
            pass
        elif host_tuple.match( '*-*-darwin*' ):
            self.mode['x86_64'] = 'x86_64-apple-darwin%s'    % (host_tuple.release)
        elif host_tuple.match( '*-*-linux*' ):
            pass
        elif host_tuple.match( '*-*-solaris*' ):
            pass
        elif host_tuple.match( '*-*-freebsd*' ):
            self.mode['i386']   = 'i386-portsbuild-freebsd%s' % (host_tuple.release)
            self.mode['amd64'] = 'amd64-portsbuild-freebsd%s' % (host_tuple.release)
        else:
            self.msg_pass = 'WARNING'

        self.msg_end = self.mode.toString()

    ## glob-match against spec
    def match( self, spec ):
        return fnmatch.fnmatch( self.spec, spec )

###############################################################################

class CoreProbe( Action ):
    def __init__( self ):
        super( CoreProbe, self ).__init__( 'probe', 'number of CPU cores' )
        self.count = 1

    def _action( self ):
        if self.fail:
            ## good for darwin9.6.0 and linux
            try:
                self.count = os.sysconf( 'SC_NPROCESSORS_ONLN' )
                if self.count < 1:
                    self.count = 1
                self.fail = False
            except:
                pass

        if self.fail:
            ## windows
            try:
                self.count = int( os.environ['NUMBER_OF_PROCESSORS'] )
                if self.count < 1:
                    self.count = 1
                self.fail = False
            except:
                pass

        ## clamp
        if self.count < 1:
            self.count = 1
        elif self.count > 64:
            self.count = 64

        if options.launch:
            if options.launch_jobs == 0:
                self.jobs = core.count
            else:
                self.jobs = options.launch_jobs
        else:
            self.jobs = core.count

        self.msg_end = str(self.count)

###############################################################################

class StoreCallbackAction(argparse.Action):
    def __init__(self, option_strings, dest, nargs=None, **kwargs):
        self.callback = kwargs.pop('callback', None)

        super(StoreCallbackAction, self).__init__(
            option_strings, dest, nargs, **kwargs)

    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, values)
        if self.callback != None:
            self.callback(self, values)

class SelectMode( dict ):
    def __init__( self, descr, *modes, **kwargs ):
        super( SelectMode, self ).__init__( modes )
        self.descr = descr
        self.modes = modes
        self.what  = kwargs.get('what',' mode')
        if modes:
            self.default = kwargs.get('default',modes[0][0])
        else:
            self.default = None
        self.mode = self.default

    def cli_add_argument( self, parser, option ):
        parser.add_argument(option, nargs='?', metavar='MODE',
            default=self.mode, const=self.mode,
            help='select %s%s: %s' % (self.descr,self.what,self.toString()),
            action=StoreCallbackAction, callback=self.cli_callback)

    def cli_callback( self, action, value ):
        if value not in self:
            raise argparse.ArgumentError(action,
                'invalid %s%s: %s (choose from: %s)'
                % (self.descr, self.what, value, self.toString(True)))
        self.mode = value

    def toString( self, nodefault=False ):
        keys = list(self.copy().keys())
        keys.sort()
        if len(self) == 1:
            value = self.mode
        elif nodefault:
            value = ' '.join( keys )
        else:
            value = '%s [%s]' % (' '.join( keys ), self.mode )
        return value

###############################################################################
##
## Repository object.
## Holds information gleaned from subversion working dir.
##
## Builds are classed into one of the following types:
##
##  release
##      must be built from official git at version tag
##  developer
##      must be built from official git but is not a release
##
class RepoProbe( ShellProbe ):
    def __init__( self ):
        # Find script that creates repo info
        try:
            repo_info = os.path.join( cfg.src_dir, 'scripts', 'repo-info.sh' )
            if not os.path.isfile( repo_info ):
                cfg.errln( 'Missing required script %s\n', repo_info )
                sys.exit( 1 )
        except:
            sys.exit( 1 )

        super( RepoProbe, self ).__init__( 'repo info', '%s %s' %
                                            (repo_info, cfg.src_dir) )

        self.url       = 'git://nowhere.com/project/unknown'
        self.tag       = ''
        self.tag_hash  = 'deadbeaf'
        self.branch    = 'unknown'
        self.remote    = 'unknown'
        self.rev       = 0
        self.hash      = 'deadbeaf'
        self.shorthash = 'deadbea'
        self.date      = None
        self.official  = 0
        self.type      = 'developer'

    def _parseSession( self ):
        for line in self.session:
            if isinstance(line, bytes):
                line = line.decode('utf-8')

            ## grok fields
            m = re.match( '([^\=]+)\=(.*)', line )
            if not m:
                continue

            (name,value) = m.groups()
            if name == 'URL' and value != '':
                self.url = value
            elif name == 'TAG':
                self.tag = value
            elif name == 'TAG_HASH':
                self.tag_hash = value
            elif name == 'BRANCH':
                self.branch = value
            elif name == 'REMOTE':
                self.remote = value
            elif name == 'REV':
                self.rev = int( value )
            elif name == 'DATE':
                self.date = datetime.strptime(value[0:19], "%Y-%m-%d %H:%M:%S")

                # strptime can't handle UTC offset
                m = re.match( '^([-+]?[0-9]{2})([0-9]{2})$', value[20:])
                (hh, mn) = m.groups()
                utc_off_hour   = int(hh)
                utc_off_minute = int(mn)
                if utc_off_hour >= 0:
                    utc_off = utc_off_hour * 60 + utc_off_minute
                else:
                    utc_off = utc_off_hour * 60 - utc_off_minute
                delta = timedelta(minutes=utc_off)
                self.date = self.date - delta

            elif name == 'HASH':
                self.hash = value
                self.shorthash = value[:7]

        # type-classification via repository URL
        if self.url == project.url_repo_ssh:
            self.url = project.url_repo  # official repo, SSH to HTTPS

        if self.url == project.url_repo:
            self.official = 1
            if not options.snapshot and self.hash == self.tag_hash:
                self.type = 'release'
            else:
                self.type = 'developer'

        self.msg_end = self.url

    def _failSession( self ):
        # Look for repo info in version file.
        #
        # Version file would be created manually by source packager.
        # e.g.
        # $ HandBrake/scripts/repo-info.sh HandBrake > HandBrake/version.txt
        # $ tar -czf handbrake-source.tgz --exclude .git HandBrake
        cfg.infof( 'probe: version.txt...' )
        try:
            hvp = os.path.join( cfg.src_dir, 'version.txt' )
            if os.path.isfile( hvp ) and os.path.getsize( hvp ) > 0:
                with open( hvp, 'r' ) as in_file:
                    self.session = in_file.readlines()
                if self.session:
                    self._parseSession()
            if self.hash and self.hash != 'deadbeaf':
                cfg.infof( '(pass)\n' )
            else:
                cfg.infof( '(fail)\n' )

        except:
            cfg.infof( '(fail)\n' )

###############################################################################
##
## project object.
##
## Contains manually updated version numbers consistent with HB releases
## and other project metadata.
##
class Project( Action ):
    def __init__( self ):
        super( Project, self ).__init__( 'compute', 'project data' )

        self.name          = 'HandBrake'
        self.acro_lower    = 'hb'
        self.acro_upper    = 'HB'
        self.url_website   = 'https://handbrake.fr'
        self.url_repo      = 'https://github.com/HandBrake/HandBrake.git'
        self.url_repo_ssh  = 'git@github.com:HandBrake/HandBrake.git'
        self.url_community = 'https://forum.handbrake.fr'
        self.url_irc       = 'irc://irc.freenode.net/handbrake'

        self.name_lower = self.name.lower()
        self.name_upper = self.name.upper()

        self.vmajor = 0
        self.vminor = 0
        self.vpoint = 0
        self.spoint = 0
        self.suffix = ''
        self.special = ''

    def _action( self ):
        ## add architecture to URL only for Mac
        if fnmatch.fnmatch( host_tuple.spec, '*-*-darwin*' ):
            url_arch = '.%s' % (arch.mode.mode)
        else:
            url_arch = ''

        if repo.date is None:
            cfg.errln( '%s is missing version information it needs to build properly.\nClone the official git repository at %s\nor download an official source archive from %s\n', self.name, self.url_repo, self.url_website )
            sys.exit( 1 )

        if repo.tag != '':
            m = re.match( '^([0-9]+)\.([0-9]+)\.([0-9]+)-?(.+)?$', repo.tag )
            if not m:
                cfg.errln( 'Invalid repo tag format %s\n', repo.tag )
                sys.exit( 1 )
            (vmajor, vminor, vpoint, suffix) = m.groups()
            self.vmajor = int(vmajor)
            self.vminor = int(vminor)
            self.vpoint = int(vpoint)
            if suffix:
                self.suffix = suffix

        if repo.type != 'release' or options.snapshot:
            self.version = repo.date.strftime("%Y%m%d%H%M%S")
            self.version += '-%s' % (repo.shorthash)
            if repo.branch != '':
                self.version += '-%s' % (repo.branch)

            self.debversion = repo.date.strftime("%Y%m%d%H%M%S")
            self.debversion += '-%s' % (repo.shorthash)
            if repo.branch != '':
                self.debversion += '-%s' % (repo.branch)

            url_ctype = '_unstable'
            url_ntype = 'unstable'
            self.build = time.strftime('%Y%m%d', now) + '01'
            self.title = '%s %s (%s)' % (self.name,self.version,self.build)
        else:
            m = re.match('^([a-zA-Z]+)\.([0-9]+)$', self.suffix)
            if not m:
                # Regular release
                self.version = '%d.%d.%d' % (self.vmajor,self.vminor,self.vpoint)
                self.debversion = '%d.%d.%d' % (self.vmajor, self.vminor, self.vpoint)
                url_ctype = ''
                url_ntype = 'stable'
            else:
                (special, spoint,) = m.groups()
                self.special = special
                self.spoint = int(spoint)
                self.version = '%d.%d.%d-%s.%d' % (self.vmajor,self.vminor,self.vpoint, self.special, self.spoint)
                self.debversion = '%d.%d.%d~%s.%d' % (self.vmajor, self.vminor, self.vpoint, self.special, self.spoint)
                url_ctype = '_unstable'
                url_ntype = 'unstable'

            self.build = time.strftime('%Y%m%d', now) + '00'
            self.title = '%s %s (%s)' % (self.name,self.version,self.build)

        self.url_appcast = 'https://handbrake.fr/appcast%s%s.xml' % (url_ctype,url_arch)
        self.url_appnote = 'https://handbrake.fr/appcast/%s.html' % (url_ntype)

        self.msg_end = '%s (%s)' % (self.name,repo.type)
        self.fail = False

###############################################################################

class ToolProbe( Action ):
    tools = []

    def __init__( self, var, option, *names, **kwargs ):
        super( ToolProbe, self ).__init__( 'find', abort=kwargs.get('abort',True) )
        if not self in ToolProbe.tools:
            ToolProbe.tools.append( self )
        self.var    = var
        self.option = option
        self.names  = []
        self.kwargs = kwargs
        for name in names:
            try:
                name = str(name)
            except:
                name = None
            if name is not None:
                self.names.append( name )
        self.name = self.names[0]
        self.pretext = self.name
        self.pathname = self.names[0]
        self.abort = kwargs.get('abort', True)
        self.versionopt = kwargs.get('versionopt', '--version')
        self.minversion = kwargs.get('minversion', None)
        self.rexpr = kwargs.get('rexpr', None)

    def _action( self ):
        self.session = []
        for i,name in enumerate(self.names):
            self.session.append( 'name[%d] = %s' % (i,name) )
        for name in self.names:
            f = cfg.findExecutable( name )
            if f:
                self.pathname = f
                self.fail = False
                self.msg_end = f
                break
        if self.fail:
            self.msg_end = 'not found'
        elif self.minversion:
            self.version = VersionProbe( self.name, [self.pathname, self.versionopt], abort=self.abort, minversion=self.minversion, rexpr=self.rexpr )

    def cli_add_argument( self, parser ):
        parser.add_argument( '--'+self.option, nargs=1, metavar='PROG',
            help='[%s]' % (self.pathname),
            action=StoreCallbackAction, callback=self.cli_callback )

    def cli_callback( self, action, value ):
        # set pool to include only the user specified tool
        self.__init__( self.var, self.option, value[0] )
        self.run()

    def doc_add( self, doc ):
        doc.add( self.var, self.pathname )

###############################################################################

###############################################################################
##
## version probe: passes --version to command and only cares about first line
## of output. If probe fails, a default version of '0.0.0' results.
## The default rexpr is useful for some very simple version strings. A Custom
## expression would be required for more complex version strings.
##
## command = full command and arguments to pipe
## rexpr   = a regular expression which must return named subgroups:
##              name: mandatory. The tool name.
##              svers: mandatory. The whole version tuple to be represented as string.
##              i0: mandatory. First element of version tuple to be parsed as int.
##              i1: optional. Second element of version tuple to be parsed as int.
##              i2: optional. Third element of version tuple to be parsed as int.
##           All matching is case-insensitive.
## abort   = if true configure will exit on probe fail
## session = result. array of lines (stdout/stderr) from command
## fail    = result. true if probe failed
## svers   = result. string of version tuple
## ivers   = result. int[3] of version tuple
##
class VersionProbe( Action ):
    def __init__( self, name, command, minversion=None, rexpr=None, abort=False ):
        super( VersionProbe, self ).__init__( 'version probe', '%s %s' % (os.path.basename(command[0]),'.'.join([str(i) for i in minversion])), abort )
        self.name = name
        self.command = command
        self.abort = abort
        self.minversion = minversion
        self.rexprs = [ '(?P<name>[^.]+)\s+(?P<svers>(?P<i0>\d+)(\.(?P<i1>\d+))?(\.(?P<i2>\d+))?)',
                        '(?P<svers>(?P<i0>\d+)(\.(?P<i1>\d+))?(\.(?P<i2>\d+))?)' ]
        if rexpr:
            self.rexprs.insert(0,rexpr)

    def _action( self ):
        ## pipe and redirect stderr to stdout; effects communicate result
        pipe = subprocess.Popen( self.command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT )

        ## read data into memory buffers
        data = pipe.communicate()
        self.fail = pipe.returncode != 0
        self.session = data[0].splitlines() if data[0] else []
        self.svers = '0.0.0'
        self.ivers = [0,0,0]

        try:
            if not self.fail and self.session and len(self.session):
                self.fail = True
                self._parse()
            self.fail = False
            self.msg_end = self.svers
        except Exception as x:
            self.svers = '0.0.0'
            self.ivers = [0,0,0]
            self.msg_end = str(x)

        if self.inadequate():
            self.fail = True
            if self.abort is True:
                stdout.write('(%s) %s\n' % (self.msg_fail,self.svers))
                raise AbortError( 'minimum required %s version is %s and %s is %s\n' % (self.name,'.'.join([str(i) for i in self.minversion]),self.command[0],self.svers) )

    def _dumpSession( self, printf ):
        printf( '  + %s\n', ' '.join(self.command) )
        super( VersionProbe, self )._dumpSession( printf )

    def _parse( self ):
        for expression in self.rexprs:
            mo = re.match( expression, self.session[0].decode('utf-8'), re.IGNORECASE )
            if mo is None:
                continue
            md = mo.groupdict()
            self.svers = md['svers']
            if 'i0' in md and md['i0']:
                self.ivers[0] = int(md['i0'])
            if 'i1' in md and md['i1']:
                self.ivers[1] = int(md['i1'])
            if 'i2' in md and md['i2']:
                self.ivers[2] = int(md['i2'])
            break

    def inadequate( self ):
        if not self.minversion:
            return False
        return self.lesser( self.minversion )

    def lesser( self, ivers ):
        for i in range(0,3):
            if self.ivers[i] < ivers[i]:
                return True
            elif self.ivers[i] > ivers[i]:
                return False
        return False

###############################################################################
##
## config object used to output gnu-make or gnu-m4 output.
##
## - add() to add NAME/VALUE pairs suitable for both make/m4.
## - addBlank() to add a linefeed for both make/m4.
## - addMake() to add a make-specific line.
## - addM4() to add a m4-specific line.
##
class ConfigDocument:
    def __init__( self ):
        self._elements = []

    def _outputMake( self, out_file, namelen, name, value, append ):
        if append:
            if value == None or len(str(value)) == 0:
                out_file.write( '%-*s +=\n' % (namelen, name) )
            else:
                out_file.write( '%-*s += %s\n' % (namelen, name, value) )
        else:
            if value == None or len(str(value)) == 0:
                out_file.write( '%-*s  =\n' % (namelen, name) )
            else:
                out_file.write( '%-*s  = %s\n' % (namelen, name, value) )

    def _outputM4( self, out_file, namelen, name, value ):
        namelen += 7
        name = '<<__%s>>,' % name.replace( '.', '_' )
        out_file.write( 'define(%-*s  <<%s>>)dnl\n' % (namelen, name, value ))

    def add( self, name, value, append=False ):
        self._elements.append( [name,value,append] )

    def addBlank( self ):
        self._elements.append( None )

    def addComment( self, format, *args ):
        self.addMake( '## ' + format % args )
        self.addM4( 'dnl ' + format % args )

    def addMake( self, line ):
        self._elements.append( ('?make',line) )

    def addM4( self, line ):
        self._elements.append( ('?m4',line) )

    def output( self, out_file, type ):
        namelen = 0
        for item in self._elements:
            if item == None or item[0].find( '?' ) == 0:
                continue
            if len(item[0]) > namelen:
                namelen = len(item[0])
        for item in self._elements:
            if item == None:
                if type == 'm4':
                    out_file.write( 'dnl\n' )
                else:
                    out_file.write( '\n' )
                continue
            if item[0].find( '?' ) == 0:
                if item[0].find( type, 1 ) == 1:
                    out_file.write( '%s\n' % (item[1]) )
                continue

            if type == 'm4':
                self._outputM4( out_file, namelen, item[0], item[1] )
            else:
                self._outputMake( out_file, namelen, item[0], item[1], item[2] )

    def update( self, name, value ):
        for item in self._elements:
            if item == None:
                continue
            if item[0] == name:
                item[1] = value
                return
        raise ValueError( 'element not found: %s' % (name) )

    def write( self, type ):
        if type == 'make':
            fname = 'GNUmakefile'
        elif type == 'm4':
            fname = os.path.join( 'project', project.name_lower + '.m4' )
        else:
            raise ValueError('unknown file type: ' + type)

        ftmp = cfg.mktmpname(fname)
        try:
            try:
                out_file = cfg.open( ftmp, 'w' )
                self.output( out_file, type )
            finally:
                try:
                    out_file.close()
                except:
                    pass
        except Exception as x:
            try:
                os.remove( ftmp )
            except Exception as x:
                pass
            cfg.errln( 'failed writing to %s\n%s', ftmp, x )

        try:
            os.rename( ftmp, fname )
        except Exception as x:
            cfg.errln( 'failed writing to %s\n%s', fname, x )

###############################################################################

def encodeDistfileConfig():
    fname = 'distfile.cfg'
    ftmp = cfg.mktmpname(fname)
    data = {
        'disable-fetch':  options.disable_df_fetch,
        'disable-verify': options.disable_df_verify,
        'jobs':           options.df_jobs,
        'verbosity':      options.df_verbosity,
        'accept-url':     options.df_accept_url,
        'deny-url':       options.df_deny_url,
    }
    try:
        try:
            out_file = cfg.open( ftmp, 'w' )
            json.dump(data, out_file)
            out_file.write('\n')
        finally:
            try:
                out_file.close()
            except:
                pass
    except Exception as x:
        try:
            os.remove( ftmp )
        except Exception as x:
            pass
        cfg.errln( 'failed writing to %s\n%s', ftmp, x )

    try:
        os.rename( ftmp, fname )
    except Exception as x:
        cfg.errln( 'failed writing to %s\n%s', fname, x )

###############################################################################
##
## create cli parser
##
def createCLI( cross = None ):
    cli = argparse.ArgumentParser( usage='%s [OPTIONS...] [TARGETS...]' % os.path.basename(__file__), description='Configure %s build system' % project.name )

    ## add hidden options
    cli.add_argument( '--xcode-driver', default='bootstrap', action='store', help=argparse.SUPPRESS )

    ## add general options
    grp = cli.add_argument_group( 'General Options' )
    grp.add_argument( '--force', default=False, action='store_true', help='overwrite existing build config' )
    grp.add_argument( '--verbose', default=False, action='store_true', help='increase verbosity' )

    ## add distfile options
    grp = cli.add_argument_group( 'Distfile Options' )
    grp.add_argument( '--disable-df-fetch', default=False, action='store_true', help='disable distfile downloads' )
    grp.add_argument( '--disable-df-verify', default=False, action='store_true', help='disable distfile data verification' )
    grp.add_argument( '--df-jobs', action='store', metavar='N', type=int, help='allow N distfile downloads at once' )
    grp.add_argument( '--df-verbose', action='count', dest='df_verbosity', help='increase distfile tools verbosity' )
    grp.add_argument( '--df-accept-url', default=[], action='append', metavar='SPEC', help='accept URLs matching regex pattern' )
    grp.add_argument( '--df-deny-url', default=[], action='append', metavar='SPEC', help='deny URLs matching regex pattern' )
    cli.add_argument_group( grp )

    ## add tool locations
    grp = cli.add_argument_group( 'Tool Basenames and Locations' )
    for tool in ToolProbe.tools:
        tool.cli_add_argument( grp )
    cli.add_argument_group( grp )

    ## add directory options
    grp = cli.add_argument_group( 'Directory Locations' )
    h = 'specify Xcode SDK sysroot (macOS only)' if (build_tuple.match('*-*-darwin*') and cross is None) else argparse.SUPPRESS
    grp.add_argument( '--sysroot', default=None, action='store', metavar='DIR',
        help=h )
    grp.add_argument( '--src', default=cfg.src_dir, action='store', metavar='DIR',
        help='specify top-level source dir [%s]' % (cfg.src_dir) )
    grp.add_argument( '--build', default=cfg.build_dir, action='store', metavar='DIR',
        help='specify build scratch/output dir [%s]' % (cfg.build_dir) )
    grp.add_argument( '--prefix', default=cfg.prefix_dir, action='store', metavar='DIR',
        help='specify install dir for products [%s]' % (cfg.prefix_dir) )
    cli.add_argument_group( grp )

    ## add build options
    grp = cli.add_argument_group( 'Build Options' )
    grp.add_argument( '--snapshot', default=False, action='store_true', help='Force a snapshot build' )
    h = IfHost( 'Build extra contribs for flatpak packaging', '*-*-linux*', '*-*-freebsd*', none=argparse.SUPPRESS ).value
    grp.add_argument( '--flatpak', default=False, action='store_true', help=h )
    cli.add_argument_group( grp )

    ## add compiler options
    grp = cli.add_argument_group( 'Compiler Options' )
    debugMode.cli_add_argument( grp, '--debug' )
    optimizeMode.cli_add_argument( grp, '--optimize' )
    arch.mode.cli_add_argument( grp, '--arch' )
    grp.add_argument( '--cross', default=None, action='store', metavar='SPEC',
        help='specify GCC cross-compilation spec' )
    cli.add_argument_group( grp )

    ## add security options
    grp = cli.add_argument_group( 'Security Options' )
    grp.add_argument( '--harden', dest="enable_harden", default=False, action='store_true', help='harden app to protect against buffer overflows' )
    h = IfHost( 'sandbox app to limit host system access (macOS only)', '*-*-darwin*', none=argparse.SUPPRESS).value
    grp.add_argument( '--sandbox', dest="enable_sandbox", default=False, action='store_true', help=(( '%s' %h ) if h != argparse.SUPPRESS else h) )
    cli.add_argument_group( grp )

    ## add Xcode options
    if (build_tuple.match('*-*-darwin*') and cross is None):
        grp = cli.add_argument_group( 'Xcode Options (macOS only)' )
        grp.add_argument( '--disable-xcode', default=False, action='store_true',
            help='disable Xcode' )
        grp.add_argument( '--xcode-prefix', default=cfg.xcode_prefix_dir, action='store', metavar='DIR',
            help='specify install dir for Xcode products [%s]' % (cfg.xcode_prefix_dir) )
        grp.add_argument( '--xcode-symroot', default='xroot', action='store', metavar='DIR',
            help='specify root of the directory hierarchy that contains product files and intermediate build files' )
        xcconfigMode.cli_add_argument( grp, '--xcode-config' )
        grp.add_argument( '--minver', default=None, action='store', metavar='VER',
            help='specify deployment target for Xcode builds' )
        cli.add_argument_group( grp )

    ## add feature options
    grp = cli.add_argument_group( 'Feature Options' )

    h = IfHost( 'enable assembly code in non-contrib modules', 'NOMATCH*-*-darwin*', 'NOMATCH*-*-linux*', none=argparse.SUPPRESS ).value
    grp.add_argument( '--enable-asm', default=False, action='store_true', help=h )

    h = IfHost( 'disable GTK GUI', '*-*-linux*', '*-*-freebsd*', none=argparse.SUPPRESS ).value
    grp.add_argument( '--disable-gtk', default=False, action='store_true', help=h )

    h = IfHost( 'disable GTK GUI update checks', '*-*-linux*', '*-*-freebsd*', none=argparse.SUPPRESS ).value
    grp.add_argument( '--disable-gtk-update-checks', default=False, action='store_true', help=h )

    h = 'enable GTK GUI for Windows' if (cross is not None and 'mingw' in cross) else argparse.SUPPRESS
    grp.add_argument( '--enable-gtk-mingw', default=False, action='store_true', help=h )

    h = IfHost( 'Build GUI with GTK4', '*-*-linux*', '*-*-freebsd*', none=argparse.SUPPRESS ).value
    grp.add_argument( '--enable-gtk4', default=False, action='store_true', help=h )

    h = IfHost( 'disable GStreamer (live preview)', '*-*-linux*', '*-*-freebsd*', none=argparse.SUPPRESS ).value
    grp.add_argument( '--disable-gst', default=False, action='store_true', help=h )

    h = IfHost( 'x265 video encoder', '*-*-*', none=argparse.SUPPRESS ).value
    grp.add_argument( '--enable-x265', dest="enable_x265", default=True, action='store_true', help=(( 'enable %s' %h ) if h != argparse.SUPPRESS else h) )
    grp.add_argument( '--disable-x265', dest="enable_x265", action='store_false', help=(( 'disable %s' %h ) if h != argparse.SUPPRESS else h) )

    h = IfHost( 'x265 NUMA support', '*-*-linux*', none=argparse.SUPPRESS ).value
    grp.add_argument( '--enable-numa', dest="enable_numa", default=IfHost(True, '*-*-linux*', none=False).value, action='store_true', help=(( 'enable %s' %h ) if h != argparse.SUPPRESS else h) )
    grp.add_argument( '--disable-numa', dest="enable_numa", action='store_false', help=(( 'disable %s' %h ) if h != argparse.SUPPRESS else h) )

    h = IfHost( 'FDK AAC audio encoder', '*-*-*', none=argparse.SUPPRESS ).value
    grp.add_argument( '--enable-fdk-aac', dest="enable_fdk_aac", default=False, action='store_true', help=(( 'enable %s' %h ) if h != argparse.SUPPRESS else h) )
    grp.add_argument( '--disable-fdk-aac', dest="enable_fdk_aac", action='store_false', help=(( 'disable %s' %h ) if h != argparse.SUPPRESS else h) )

    h = 'FFmpeg AAC audio encoder' if (host_tuple.match( '*-*-darwin*' )) else argparse.SUPPRESS
    grp.add_argument( '--enable-ffmpeg-aac', dest="enable_ffmpeg_aac", default=not host_tuple.match( '*-*-darwin*' ), action='store_true', help=(( 'enable %s' %h ) if h != argparse.SUPPRESS else h) )
    grp.add_argument( '--disable-ffmpeg-aac', dest="enable_ffmpeg_aac", action='store_false', help=(( 'disable %s' %h ) if h != argparse.SUPPRESS else h) )

    h = IfHost( 'Nvidia NVENC video encoder', '*-*-linux*', '*-*-mingw*', none=argparse.SUPPRESS).value
    grp.add_argument( '--enable-nvenc', dest="enable_nvenc", default=IfHost( True, '*-*-linux*', '*-*-mingw*', none=False).value, action='store_true', help=(( 'enable %s' %h ) if h != argparse.SUPPRESS else h) )
    grp.add_argument( '--disable-nvenc', dest="enable_nvenc", action='store_false', help=(( 'disable %s' %h ) if h != argparse.SUPPRESS else h) )

    h = IfHost( 'Intel QSV video encoder/decoder', '*-*-linux*', '*-*-mingw*', none=argparse.SUPPRESS).value
    grp.add_argument( '--enable-qsv', dest="enable_qsv", default=IfHost(True, "*-*-mingw*", none=False).value, action='store_true', help=(( 'enable %s' %h ) if h != argparse.SUPPRESS else h) )
    grp.add_argument( '--disable-qsv', dest="enable_qsv", action='store_false', help=(( 'disable %s' %h ) if h != argparse.SUPPRESS else h) )

    h = IfHost( 'AMD VCE video encoder', '*-*-linux*', '*-*-mingw*', none=argparse.SUPPRESS).value
    grp.add_argument( '--enable-vce', dest="enable_vce", default=IfHost(True, '*-*-mingw*', none=False).value, action='store_true', help=(( 'enable %s' %h ) if h != argparse.SUPPRESS else h) )
    grp.add_argument( '--disable-vce', dest="enable_vce", action='store_false', help=(( 'disable %s' %h ) if h != argparse.SUPPRESS else h) )

    cli.add_argument_group( grp )

    ## add launch options
    grp = cli.add_argument_group( 'Launch Options' )
    grp.add_argument( '--launch', default=False, action='store_true',
        help='launch build, capture log and wait for completion' )
    grp.add_argument( '--launch-jobs', default=1, action='store', metavar='N', type=int,
        help='allow N jobs at once; 0 to match CPU count [1]' )
    grp.add_argument( '--launch-args', default=None, action='store', metavar='ARGS',
        help='specify additional ARGS for launch command' )
    grp.add_argument( '--launch-quiet', default=False, action='store_true',
        help='do not echo build output while waiting' )
    cli.add_argument_group( grp )

    return cli

###############################################################################
##
## launcher - used for QuickStart method; launch; build and capture log.
##
class Launcher:
    def __init__( self, targets ):
        # open build logfile
        self._file = cfg.open( 'log/build.txt', 'w' )

        cmd = '%s -j%d' % (Tools.gmake.pathname,core.jobs)
        if options.launch_args:
            cmd += ' ' + options.launch_args
        if len(targets):
            cmd += ' ' + ' '.join(targets)

        ## record begin
        timeBegin = time.time()
        self.infof( 'time begin: %s\n', time.asctime() )
        self.infof( 'launch: %s\n', cmd )
        if options.launch_quiet:
            stdout.write( 'building to %s ...\n' % (os.path.abspath( cfg.build_final )))
        else:
            stdout.write( '%s\n' % ('-' * 79) )

        ## launch/pipe
        try:
            pipe = subprocess.Popen( cmd, shell=True, bufsize=1, stdout=subprocess.PIPE, stderr=subprocess.STDOUT )
        except Exception as x:
            cfg.errln( 'launch failure: %s', x )
        for line in pipe.stdout:
            if not isinstance(line, str):
                line = line.decode()
            self.echof( '%s', line )
        pipe.wait()

        ## record end
        timeEnd = time.time()
        elapsed = timeEnd - timeBegin

        if pipe.returncode:
            result = 'FAILURE (code %d)' % pipe.returncode
        else:
            result = 'SUCCESS'

        ## present duration in decent format
        seconds = elapsed
        hours = int(seconds / 3600)
        seconds -= hours * 3600
        minutes = int(seconds / 60)
        seconds -= minutes * 60

        segs = []
        duration = ''

        if hours == 1:
            segs.append( '%d hour' % hours )
        elif hours > 1:
            segs.append( '%d hours' % hours )

        if len(segs) or minutes == 1:
            segs.append( '%d minute' % minutes )
        elif len(segs) or  minutes > 1:
            segs.append( '%d minutes' % minutes )

        if seconds == 1:
            segs.append( '%d second' % seconds )
        else:
            segs.append( '%d seconds' % seconds )

        if not options.launch_quiet:
            stdout.write( '%s\n' % ('-' * 79) )
        self.infof( 'time end: %s\n', time.asctime() )
        self.infof( 'duration: %s (%.2fs)\n', ', '.join(segs), elapsed )
        self.infof( 'result: %s\n', result )

        ## cleanup
        self._file.close()

    def echof( self, format, *args ):
        line = format % args
        self._file.write( line )
        if not options.launch_quiet:
            stdout.write( '  : %s' % line )
            stdout.flush()

    def infof( self, format, *args ):
        line = format % args
        self._file.write( line )
        cfg.infof( '%s', line )

###############################################################################
##
## main program
##
try:
    ## we need to pre-check argv for -h or --help or --verbose to deal with
    ## initializing Configure correctly.
    verbose = Configure.OUT_INFO
    for arg in sys.argv:
        if arg == '-h' or arg == '--help':
            verbose = Configure.OUT_QUIET
            break
        if arg == '--verbose':
            verbose = Configure.OUT_VERBOSE

    now = time.gmtime(int(os.environ.get('SOURCE_DATE_EPOCH', time.time())))

    ## create main objects; actions/probes run() is delayed.
    ## if any actions must be run earlier (eg: for configure --help purposes)
    ## then run() must be invoked earlier. subsequent run() invocations
    ## are ignored.
    cfg         = Configure( verbose )
    build_tuple = BuildTupleProbe(); build_tuple.run()

    cfg.prefix_dir = '/usr/local'
    if build_tuple.match( '*-*-darwin*' ):
        cfg.xcode_prefix_dir = '/Applications'

    ## create remaining main objects
    core    = CoreProbe()
    repo    = RepoProbe()
    project = Project()

    # options is created by parse_known_args(), which is called directly after
    # createCLI(). we need cross info earlier and cannot parse args twice, so
    # extract the info we need here
    cross = None
    for i in range(len(sys.argv)):
        cross_pattern = re.compile( '^--cross=(.+)$' )
        m = cross_pattern.match( sys.argv[i] )
        if m:
            cross = sys.argv[i][8:]
            continue
        cross_pattern = re.compile( '^--cross$' )
        m = cross_pattern.match( sys.argv[i] )
        if m and ((i + 1) < len(sys.argv)):
            cross = sys.argv[i+1]
            cross = None if cross == '' else cross
            continue

    ## create tools in a scope
    class Tools:
        ar         = ToolProbe( 'AR.exe',         'ar',         'ar', abort=True )
        cp         = ToolProbe( 'CP.exe',         'cp',         'cp', abort=True )
        m4         = ToolProbe( 'M4.exe',         'm4',         'gm4', 'm4', abort=True )
        mkdir      = ToolProbe( 'MKDIR.exe',      'mkdir',      'mkdir', abort=True )
        patch      = ToolProbe( 'PATCH.exe',      'patch',      'gpatch', 'patch', abort=True )
        rm         = ToolProbe( 'RM.exe',         'rm',         'rm', abort=True )
        ranlib     = ToolProbe( 'RANLIB.exe',     'ranlib',     'ranlib', abort=True )
        strip      = ToolProbe( 'STRIP.exe',      'strip',      'strip', abort=True )
        tar        = ToolProbe( 'TAR.exe',        'tar',        'gtar', 'tar', abort=True )
        python     = ToolProbe( 'PYTHON.exe',     'python',     os.path.basename(sys.executable), abort=True )

        gcc_tools  = ['GCC.gcc',
                      'cc',
                      os.environ.get('CC', None),
                      'gcc',
                      IfBuild( 'clang', '*-*-freebsd*' ),
                      IfBuild( 'gcc-4', '*-*-cygwin*' )]
        gcc        = ToolProbe(*filter(None, gcc_tools))
        if build_tuple.match( '*-*-darwin*' ):
            gmake  = ToolProbe( 'GMAKE.exe',      'make',       'make', 'gmake', abort=True )
        else:
            gmake  = ToolProbe( 'GMAKE.exe',      'make',       'gmake', 'make', abort=True )

        autoconf   = ToolProbe( 'AUTOCONF.exe',   'autoconf',   'autoconf', abort=True, minversion=[2,69,0] )
        automake   = ToolProbe( 'AUTOMAKE.exe',   'automake',   'automake', abort=True, minversion=[1,15,0] )
        libtool    = ToolProbe( 'LIBTOOL.exe',    'libtool',    'libtool', abort=True )
        lipo       = ToolProbe( 'LIPO.exe',       'lipo',       'lipo', abort=False )
        pkgconfig  = ToolProbe( 'PKGCONFIG.exe',  'pkgconfig',  'pkg-config', abort=True, minversion=[0,29,0] )

        meson      = ToolProbe( 'MESON.exe',      'meson',      'meson', abort=True, minversion=[0,47,0] )
        nasm       = ToolProbe( 'NASM.exe',       'asm',        'nasm', abort=True, minversion=[2,13,0] )
        ninja      = ToolProbe( 'NINJA.exe',      'ninja',      'ninja-build', 'ninja', abort=True )

        xcodebuild = ToolProbe( 'XCODEBUILD.exe', 'xcodebuild', 'xcodebuild', abort=(True if (build_tuple.match('*-*-darwin*') and cross is None) else False), versionopt='-version', minversion=[10,3,0] )

    ## run tool probes
    for tool in ToolProbe.tools:
        tool.run()

    debugMode = SelectMode( 'debug', ('none','none'), ('min','min'), ('std','std'), ('max','max') )
    optimizeMode = SelectMode( 'optimize', ('none','none'), ('speed','speed'), ('size','size'), default='speed' )

    ## find xcconfig values
    xcconfigMode = SelectMode( 'xcconfig', ('none',None), what='' )
    if build_tuple.match( '*-*-darwin*' ):
        for xc in glob.glob( os.path.join(cfg.dir, '../macosx/xcconfig/*.xcconfig') ):
            bname = os.path.basename( xc )
            xname = os.path.splitext( bname )
            if xname and xname[0]:
                xcconfigMode[xname[0]] = bname
        if not 'native' in xcconfigMode:
            raise Exception( 'native xcconfig not found' )
        xcconfigMode.default = 'native'
        xcconfigMode.mode = xcconfigMode.default

    ## re-run tools with cross-compilation needs
    if cross:
        for tool in ( Tools.ar, Tools.gcc, Tools.ranlib, Tools.strip ):
            tool.__init__( tool.var, tool.option, '%s-%s' % (cross,tool.name), **tool.kwargs )
            tool.run()

    # run host tuple and arch actions
    host_tuple = HostTupleAction(cross)
    arch       = ArchAction(); arch.run()

    # create CLI and parse
    cli = createCLI( cross )
    options, args = cli.parse_known_args()

    ## update cfg with cli directory locations
    cfg.update_cli( options )

    ## prepare list of targets and NAME=VALUE args to pass to make
    targets = []
    exports = []
    rx_exports = re.compile( '([^=]+)=(.*)' )
    for arg in args:
        m = rx_exports.match( arg )
        if m:
            exports.append( m.groups() )
        else:
            targets.append( arg )

    ## run delayed actions
    for action in Action.actions:
        action.run()

    ## Sanitize options
    # Sandboxing is currently only implemented on macOS
    options.enable_sandbox    = IfHost(options.enable_sandbox, '*-*-darwin*',
                                       none=False).value
    # Require FFmpeg AAC on Linux and Windows
    options.enable_ffmpeg_aac = IfHost(options.enable_ffmpeg_aac, '*-*-darwin*',
                                       none=True).value
    # Allow GTK mingw only on mingw
    options.enable_gtk_mingw  = IfHost(options.enable_gtk_mingw, '*-*-mingw*',
                                       none=False).value
    # Disable NVENC on unsupported platforms
    options.enable_nvenc      = IfHost(options.enable_nvenc, '*-*-linux*',
                                       '*-*-mingw*', none=False).value
    # NUMA is linux only and only needed with x265
    options.enable_numa       = (IfHost(options.enable_numa, '*-*-linux*',
                                        none=False).value
                                 and options.enable_x265)
    # Disable QSV on unsupported platforms
    options.enable_qsv        = IfHost(options.enable_qsv, '*-*-linux*',
                                       '*-*-mingw*', none=False).value
    # Disable VCE on unsupported platforms
    options.enable_vce        = IfHost(options.enable_vce, '*-*-linux*', '*-*-mingw*',
                                       none=False).value

    #####################################
    ## Additional library and tool checks
    #####################################
    if options.flatpak or host_tuple.match('*-*-darwin*', '*-*-mingw'):
        # Requires Jansson which requires CMake 3.1.0 or later
        Tools.cmake = ToolProbe('CMAKE.exe', 'cmake', 'cmake', abort=True, minversion=[3,1,0])
    else:
        Tools.cmake = ToolProbe('CMAKE.exe', 'cmake', 'cmake', abort=True, minversion=[2,8,12])
    Tools.cmake.__init__( Tools.cmake.var, Tools.cmake.option, Tools.cmake.name, **Tools.cmake.kwargs )
    Tools.cmake.run()
    for action in Action.actions:
        action.run()

    #########################################
    ## MinGW specific library and tool checks
    #########################################
    if host_tuple.system == 'mingw':
        dlfcn_test = """
#include <dlfcn.h>
#include <stdio.h>

void fnord() { int i=42;}
int main ()
{
  void *self = dlopen (0, RTLD_GLOBAL|RTLD_NOW);
  int status = 1;

  if (self)
    {
      if (dlsym (self,"fnord"))       status = 0;
      else if (dlsym( self,"_fnord")) status = 0;
      /* dlclose (self); */
    }
  else
    puts (dlerror ());

  return status;
}
"""
        dlfcn = LDProbe( 'static dlfcn', '%s -static' % Tools.gcc.pathname, '-ldl', dlfcn_test )
        dlfcn.run()

        pthread_test = """
#include <stdio.h>
#include <pthread.h>
int main ()
{
  pthread_t thread;
  pthread_create (&thread, NULL, NULL, NULL);
  return 0;
}
"""
        pthread = LDProbe( 'static pthread', '%s -static' % Tools.gcc.pathname, '-lpthread', pthread_test )
        pthread.run()

        bz2_test = """
#include <stdio.h>
#include <bzlib.h>
int main ()
{
  BZ2_bzReadOpen(NULL, NULL, 0, 0, NULL, 0);
  return 0;
}
"""
        bz2 = LDProbe( 'static bz2', '%s -static' % Tools.gcc.pathname, '-lbz2', bz2_test )
        bz2.run()

        libz_test = """
#include <stdio.h>
#include <zlib.h>
int main ()
{
  compress(NULL, NULL, NULL, 0);
  return 0;
}
"""
        libz = LDProbe( 'static zlib', '%s -static' % Tools.gcc.pathname, '-lz', libz_test )
        libz.run()

        xz_test = """
#include <stdio.h>
#include <lzma.h>
int main ()
{
  lzma_stream_decoder(NULL, 0, 0);
  return 0;
}
"""
        xz = LDProbe( 'static xz', '%s -static' % Tools.gcc.pathname, '-llzma', xz_test )
        xz.run()

        iconv_test = """
#include <stdio.h>
#include <iconv.h>
int main ()
{
  iconv_open(NULL, NULL);
  return 0;
}
"""
        iconv = LDProbe( 'static iconv', '%s -static' % Tools.gcc.pathname, '-liconv', iconv_test )
        iconv.run()

        regex_test = """
#include <stdio.h>
#include <regex.h>

int match(regex_t *x, char *s)
{
    regmatch_t matches[1];
    return regexec(x, s, 1, matches, 0);
}

int main()
{
    int     rv;
    regex_t exp;

    rv = regcomp(&exp, "^[0-9]+$", REG_EXTENDED);
    if (rv != 0) {
        return 1;
    }

    if (match(&exp, "7") != 0)
    {
        return 1;
    }
    if (match(&exp, "foo") == 0)
    {
        return 1;
    }

    regfree(&exp);
    return 0;
}
"""
        regex = LDProbe( 'static regex', '%s -static' % Tools.gcc.pathname, '-lregex', regex_test )
        regex.run()

        strtok_r_test = """
#include <string.h>

int main ()
{
  char *saveptr;
  strtok_r("String tok string", "tok", &saveptr);
  return 0;
}
"""
        strtok_r = LDProbe( 'static strtok_r', '%s -static' % Tools.gcc.pathname, '', strtok_r_test )
        strtok_r.run()

    #########################################
    ## Linux specific library and tool checks
    #########################################
    if host_tuple.system == 'linux':
        if options.enable_numa:
            numa_test = """
#include <numa.h>

int main()
{
struct bitmask *bm = numa_allocate_cpumask();
return 0;
}
"""

            numa = ChkLib( 'numa', '%s' % Tools.gcc.pathname,
                           'numa', numa_test, abort=True )
            numa.run()

    #########################################
    ## Common library and tool checks
    #########################################
    strerror_r_test = """
#include <string.h>

int main()
{
    /* some implementations fail if buf is less than 80 characters
       so size it appropriately */
    char errstr[128];
    /* some implementations fail if err == 0 */
    strerror_r(1, errstr, 127);
    return 0;
}
"""

    strerror_r = LDProbe( 'strerror_r', '%s' % Tools.gcc.pathname, '', strerror_r_test )
    strerror_r.run()

    #########################################
    ## Compiler option checks
    #########################################
    gcc_w_extra = []

    Wno_format_truncation_command = '%s -O2 -Werror -Wno-format-truncation -S -o /dev/null -xc /dev/null > /dev/null 2>&1' % Tools.gcc.pathname
    Wno_format_truncation = ShellProbe('checking for -Wno-format-truncation', '%s' % Wno_format_truncation_command)
    Wno_format_truncation.run()
    if Wno_format_truncation.fail is False:
        gcc_w_extra.append('no-format-truncation')

    ## cfg hook before doc prep
    cfg.doc_ready()

    ## create document object
    doc = ConfigDocument()
    doc.addComment( 'generated by configure on %s', time.strftime( '%c' ))

    ## add configure line for reconfigure purposes
    doc.addBlank()
    conf_args = []
    for arg in sys.argv[1:]:
        if re.match( '^--(force|launch).*$', arg ):
            continue
        conf_args.append(arg)
    doc.add( 'CONF.args', ' '.join(conf_args).replace('$','$$') )

    doc.addBlank()
    doc.add( 'HB.title',       project.title )
    doc.add( 'HB.name',        project.name )
    doc.add( 'HB.name.lower',  project.name_lower )
    doc.add( 'HB.name.upper',  project.name_upper )
    doc.add( 'HB.acro.lower',  project.acro_lower )
    doc.add( 'HB.acro.upper',  project.acro_upper )

    doc.add( 'HB.url.website',    project.url_website )
    doc.add( 'HB.url.community',  project.url_community )
    doc.add( 'HB.url.irc',        project.url_irc )
    doc.add( 'HB.url.appcast',    project.url_appcast )
    doc.add( 'HB.url.appnote',    project.url_appnote )

    doc.add( 'HB.version.major',  project.vmajor )
    doc.add( 'HB.version.minor',  project.vminor )
    doc.add( 'HB.version.point',  project.vpoint )
    doc.add( 'HB.version.suffix', project.suffix )
    doc.add( 'HB.version',        project.version )
    doc.add( 'HB.debversion',     project.debversion )
    doc.add( 'HB.version.hex',    '%04x%02x%02x%08x' % (project.vmajor,project.vminor,project.vpoint,repo.rev) )

    doc.add( 'HB.build', project.build )

    doc.add( 'HB.repo.url',       repo.url )
    doc.add( 'HB.repo.tag',       repo.tag )
    doc.add( 'HB.repo.rev',       repo.rev )
    doc.add( 'HB.repo.hash',      repo.hash )
    doc.add( 'HB.repo.shorthash', repo.shorthash )
    doc.add( 'HB.repo.branch',    repo.branch )
    doc.add( 'HB.repo.remote',    repo.remote )
    doc.add( 'HB.repo.type',      repo.type )
    doc.add( 'HB.repo.official',  repo.official )
    doc.add( 'HB.repo.date',      repo.date.strftime("%Y-%m-%d %H:%M:%S") )

    doc.addBlank()
    doc.add( 'BUILD.spec',    build_tuple.spec )
    doc.add( 'BUILD.machine', build_tuple.machine )
    doc.add( 'BUILD.vendor',  build_tuple.vendor )
    doc.add( 'BUILD.system',  build_tuple.system )
    doc.add( 'BUILD.systemf', build_tuple.systemf )
    doc.add( 'BUILD.release', build_tuple.release )
    doc.add( 'BUILD.extra',   build_tuple.extra )
    doc.add( 'BUILD.title',   '%s %s' % (build_tuple.systemf,arch.mode.default) )
    doc.add( 'BUILD.ncpu',    core.count )
    doc.add( 'BUILD.jobs',    core.jobs )
    doc.add( 'BUILD.date',    time.strftime('%c', now) ),

    doc.addBlank()
    doc.add( 'HOST.spec',    host_tuple.spec )
    doc.add( 'HOST.machine', host_tuple.machine )
    doc.add( 'HOST.vendor',  host_tuple.vendor )
    doc.add( 'HOST.system',  host_tuple.system )
    doc.add( 'HOST.systemf', host_tuple.systemf )
    doc.add( 'HOST.release', host_tuple.release )
    doc.add( 'HOST.extra',   host_tuple.extra )
    doc.add( 'HOST.title',   host_tuple.title )

    doc.add( 'HOST.cross', int(options.cross != None or arch.mode.mode != arch.mode.default) )
    if options.cross:
        doc.add( 'HOST.cross.prefix', '%s-' % (options.cross) )
    else:
        doc.add( 'HOST.cross.prefix', '' )

    doc.add( 'HOST.arch',   arch.mode.mode )

    doc.addBlank()
    doc.add( 'SRC',     cfg.src_final )
    doc.add( 'SRC/',    cfg.src_final + os.sep )
    doc.add( 'BUILD',   cfg.build_final )
    doc.add( 'BUILD/',  cfg.build_final + os.sep )
    doc.add( 'PREFIX',  cfg.prefix_final )
    doc.add( 'PREFIX/', cfg.prefix_final + os.sep )

    doc.addBlank()
    doc.add( 'SECURITY.sandbox',    int( options.enable_sandbox ))
    doc.add( 'SECURITY.harden',     int( options.enable_harden ))

    doc.addBlank()
    doc.add( 'FEATURE.asm',        int( 0 ))
    doc.add( 'FEATURE.fdk_aac',    int( options.enable_fdk_aac ))
    doc.add( 'FEATURE.ffmpeg_aac', int( options.enable_ffmpeg_aac ))
    doc.add( 'FEATURE.flatpak',    int( options.flatpak ))
    doc.add( 'FEATURE.gtk4',       int( options.enable_gtk4 ))
    doc.add( 'FEATURE.gtk',        int( not options.disable_gtk ))
    doc.add( 'FEATURE.gtk.mingw',  int( options.enable_gtk_mingw ))
    doc.add( 'FEATURE.gtk.update.checks', int( not options.disable_gtk_update_checks ))
    doc.add( 'FEATURE.gst',        int( not options.disable_gst ))
    doc.add( 'FEATURE.nvenc',      int( options.enable_nvenc ))
    doc.add( 'FEATURE.qsv',        int( options.enable_qsv ))
    doc.add( 'FEATURE.vce',        int( options.enable_vce ))
    doc.add( 'FEATURE.x265',       int( options.enable_x265 ))
    doc.add( 'FEATURE.numa',       int( options.enable_numa ))

    if build_tuple.match( '*-*-darwin*' ) and options.cross is None:
        doc.add( 'FEATURE.xcode',      int( not (Tools.xcodebuild.fail or options.disable_xcode) ))
        if not Tools.xcodebuild.fail and not options.disable_xcode:
            doc.addBlank()
            doc.add( 'XCODE.prefix',  cfg.xcode_prefix_final )
            doc.add( 'XCODE.prefix/', cfg.xcode_prefix_final + os.sep )
            doc.add( 'XCODE.driver', options.xcode_driver )
            if os.path.isabs(options.xcode_symroot):
                doc.add( 'XCODE.symroot', options.xcode_symroot )
            else:
                doc.add( 'XCODE.symroot', os.path.abspath(os.path.join(cfg.build_dir,options.xcode_symroot)) )
            doc.add( 'XCODE.xcconfig', xcconfigMode[xcconfigMode.mode] )

    if host_tuple.system == 'mingw':
        doc.addBlank()
        if not dlfcn.fail:
            doc.add( 'HAS.dlfcn', 1 )
        elif not pthread.fail:
            doc.add( 'HAS.pthread', 1 )
        if not bz2.fail:
            doc.add( 'HAS.bz2', 1 )
        if not libz.fail:
            doc.add( 'HAS.libz', 1 )
        if not xz.fail:
            doc.add( 'HAS.xz', 1 )
        if not iconv.fail:
            doc.add( 'HAS.iconv', 1 )
        if not regex.fail:
            doc.add( 'HAS.regex', 1 )
        if strtok_r.fail:
            doc.add( 'COMPAT.strtok_r', 1 )

    else:
        doc.addBlank()
        if host_tuple.system == 'freebsd':
            doc.add( 'HAS.pthread', 1 )
        if not strerror_r.fail:
            doc.add( 'HAS.strerror_r', 1 )

    doc.addMake( '' )
    doc.addMake( '## define debug mode and optimize before other includes' )
    doc.addMake( '## since it is tested in some module.defs' )
    doc.add( 'GCC.g', debugMode.mode )
    doc.add( 'GCC.O', optimizeMode.mode )
    doc.addBlank()
    doc.addMake( '## include definitions' )
    doc.addMake( 'include $(SRC/)make/include/main.defs' )

    doc.addBlank()
    for tool in ToolProbe.tools:
        tool.doc_add( doc )

    doc.addBlank()
    doc.add( 'GCC.archs', arch.mode.mode )
    if host_tuple.match( '*-*-darwin*' ):
        doc.add( 'GCC.sysroot', cfg.sysroot_dir )
        doc.add( 'GCC.minver', cfg.minver )
    else:
        doc.add( 'GCC.sysroot', '' )
        doc.add( 'GCC.minver', '' )
    doc.add( 'GCC.W.extra', " ".join(gcc_w_extra) )

    if host_tuple.match( 'i?86-*' ):
        doc.add( 'LIBHB.GCC.D', 'ARCH_X86_32', append=True )
    elif host_tuple.match( 'x86_64-*' ):
        doc.add( 'LIBHB.GCC.D', 'ARCH_X86_64', append=True )
    elif host_tuple.match( 'amd64-*' ):
        doc.add( 'LIBHB.GCC.D', 'ARCH_X86_64', append=True )

    if options.enable_asm and ( not Tools.nasm.fail ):
        asm = ''
        if host_tuple.match( 'i?86-*' ):
            asm = 'x86'
            doc.add( 'LIBHB.GCC.D', 'HAVE_MMX', append=True )
        elif host_tuple.match( 'x86_64-*' ) or host_tuple.match( 'amd64-*' ):
            asm = 'x86'
            doc.add( 'LIBHB.GCC.D', 'HAVE_MMX ARCH_X86_64', append=True )
        doc.update( 'FEATURE.asm', asm )

    ## add exports to make
    if len(exports):
        doc.addBlank()
        doc.addComment( 'overrides via VARIABLE=VALUE on command-line' )
        for nv in exports:
            doc.add( nv[0], nv[1] )

    doc.addMake( '' )
    doc.addMake( '## include custom definitions' )
    doc.addMake( '-include $(SRC/)custom.defs' )
    doc.addMake( '-include $(BUILD/)GNUmakefile.custom.defs' )

    doc.addMake( '' )
    doc.addMake( '## include rules' )
    doc.addMake( 'include $(SRC/)make/include/main.rules' )
    doc.addMake( '-include $(SRC/)custom.rules' )
    doc.addMake( '-include $(BUILD/)GNUmakefile.custom.rules' )

    ## chdir
    cfg.chdir()

    ## perform
    doc.write( 'make' )
    doc.write( 'm4' )
    encodeDistfileConfig()

    note_required    = 'required on target platform'
    note_unsupported = 'not supported on target platform'

    stdout.write( '%s\n' % ('-' * 79) )
    stdout.write( 'Build system:       %s\n' % build_tuple.spec.rstrip('-') )
    stdout.write( 'Host system:        %s\n' % host_tuple.spec.rstrip('-') )
    stdout.write( 'Target platform:    %s' % host_tuple.system )
    stdout.write( ' (cross-compile)\n' ) if options.cross else stdout.write( '\n' )
    stdout.write( 'Harden:             %s\n' % options.enable_harden )
    stdout.write( 'Sandbox:            %s' % options.enable_sandbox )
    stdout.write( ' (%s)\n' % note_unsupported ) if not host_tuple.system == 'darwin' else stdout.write( '\n' )
    stdout.write( 'Enable FDK-AAC:     %s\n' % options.enable_fdk_aac )
    stdout.write( 'Enable FFmpeg AAC:  %s' % options.enable_ffmpeg_aac )
    stdout.write( '  (%s)\n' % note_required ) if host_tuple.system != 'darwin' else stdout.write( '\n' )
    stdout.write( 'Enable NVENC:       %s' % options.enable_nvenc )
    stdout.write( ' (%s)\n' % note_unsupported ) if not (host_tuple.system == 'linux' or host_tuple.system == 'mingw') else stdout.write( '\n' )
    stdout.write( 'Enable QSV:         %s' % options.enable_qsv )
    stdout.write( ' (%s)\n' % note_unsupported ) if not (host_tuple.system == 'linux' or host_tuple.system == 'mingw') else stdout.write( '\n' )
    stdout.write( 'Enable VCE:         %s' % options.enable_vce )
    stdout.write( ' (%s)\n' % note_unsupported ) if not (host_tuple.system == 'linux' or host_tuple.system == 'mingw') else stdout.write( '\n' )

    if options.launch:
        stdout.write( '%s\n' % ('-' * 79) )
        Launcher( targets )

    cfg.record_log()

    if os.path.normpath( cfg.build_dir ) == os.curdir:
        nocd = True
    else:
        nocd = False

    stdout.write( '%s\n' % ('-' * 79) )
    if options.launch:
        stdout.write( 'Build is finished!\n' )
        if nocd:
            stdout.write( 'You may now examine the output.\n' )
        else:
            stdout.write( 'You may now cd into %s and examine the output.\n' % (cfg.build_dir) )
    else:
        stdout.write( 'Build is configured!\n' )
        if nocd:
            stdout.write( 'You may now run make (%s).\n' % (Tools.gmake.pathname) )
        else:
            stdout.write( 'You may now cd into %s and run make (%s).\n' % (cfg.build_dir,Tools.gmake.pathname) )

except AbortError as x:
    stderr.write( 'ERROR: %s\n' % (x) )
    try:
        cfg.record_log()
    except:
        pass
    sys.exit( 1 )

sys.exit( 0 )
