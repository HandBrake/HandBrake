###############################################################################
##
## This script is coded for minimum version of Python 2.4 .
## Pyhthon3 is incompatible.
##
## Authors: konablend
##
###############################################################################

import fnmatch
import glob
import optparse
import os
import platform
import re
import subprocess
import sys
import time
from datetime import datetime, timedelta

from optparse import OptionGroup
from optparse import OptionGroup
from optparse import OptionParser
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
        if cfg.verbose >= Configure.OUT_INFO:
            self._log_info.append( line )
            stdout.write( line )
    def verbosef( self, format, *args ):
        line = format % args
        self._log_verbose.append( line )
        if cfg.verbose >= Configure.OUT_VERBOSE:
            stdout.write( line )

    ## doc is ready to be populated
    def doc_ready( self ):
        ## compute final paths as they are after chdir into build
        self.build_final  = os.curdir
        self.src_final    = self._final_dir( self.build_dir, self.src_dir )
        self.prefix_final = self._final_dir( self.build_dir, self.prefix_dir )

        cfg.infof( 'compute: makevar SRC/    = %s\n', self.src_final )
        cfg.infof( 'compute: makevar BUILD/  = %s\n', self.build_final )
        cfg.infof( 'compute: makevar PREFIX/ = %s\n', self.prefix_final )

    ## perform chdir and enable log recording
    def chdir( self ):
        if os.path.abspath( self.build_dir ) == os.path.abspath( self.src_dir ):
            cfg.errln( 'build (scratch) directory must not be the same as top-level source root!' )

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
        m = re.match( '^(.*)\.tmp$', args[0] )
        if m:
            self.infof( 'write: %s\n', m.group(1) )
        else:
            self.infof( 'write: %s\n', args[0] )

        try:
            return open( *args )
        except Exception, x:
            cfg.errln( 'open failure: %s', x )

    def record_log( self ):
        if not self._record:
            return
        self._record = False
        self.verbose = Configure.OUT_QUIET
        file = cfg.open( 'log/config.info.txt', 'w' )
        for line in self._log_info:
            file.write( line )
        file.close()
        file = cfg.open( 'log/config.verbose.txt', 'w' )
        for line in self._log_verbose:
            file.write( line )
        file.close()

    ## Find executable by searching path.
    ## On success, returns full pathname of executable.
    ## On fail, returns None.
    def findExecutable( self, name ):
        if len( os.path.split(name)[0] ):
            if os.access( name, os.X_OK ):
                return name
            return None

        if not os.environ.has_key( 'PATH' ) or os.environ[ 'PATH' ] == '':
            path = os.defpath
        else:
            path = os.environ['PATH']

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
        if options.sysroot != None:
                self.sysroot_dir = os.path.normpath( options.sysroot )
        else:
                self.sysroot_dir = ""

        if options.minver != None:
                self.minver = options.minver
        else:
                self.minver = ""

        ## special case if src == build: add build subdir
        if os.path.abspath( self.src_dir ) == os.path.abspath( self.build_dir ):
            self.build_dir = os.path.join( self.build_dir, 'build' )

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
        file = open( 'conftest.c', 'w' )
        file.write( self.test_file )
        file.close()
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
class LDProbe( Action ):
    def __init__( self, pretext, command, lib, test_file ):
        super( LDProbe, self ).__init__( 'probe', pretext )
        self.command = command
        self.test_file = test_file
        self.lib = lib

    def _action( self ):
        ## write program file
        file = open( 'conftest.c', 'w' )
        file.write( self.test_file )
        file.close()
        ## pipe and redirect stderr to stdout; effects communicate result
        pipe = subprocess.Popen( '%s -o conftest conftest.c %s' % (self.command, self.lib), shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT )

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
            os.remove( 'conftest' )

    def _dumpSession( self, printf ):
        printf( '  + %s\n', self.command )
        super( LDProbe, self )._dumpSession( printf )


###############################################################################
##
## GNU host tuple probe: determine canonical platform type
##
## example results from various platforms:
##
##   powerpc-apple-darwin9.6.0  (Mac OS X 10.5.6 PPC)
##   i386-apple-darwin9.6.0     (Mac OS X 10.5.6 Intel)
##   x86_64-apple-darwin10.8.0  (Mac OS X 10.6.8 Intel)
##   x86_64-apple-darwin11.2.0  (Mac OS X 10.7.2 Intel)
##   i686-pc-cygwin             (Cygwin, Microsoft Vista)
##   x86_64-unknown-linux-gnu   (Linux, Fedora 10 x86_64)
##
class HostTupleProbe( ShellProbe, list ):
    GNU_TUPLE_RE = '([^-]+)-?([^-]*)-([^0-9-]+)([^-]*)-?([^-]*)'

    def __init__( self ):
        super( HostTupleProbe, self ).__init__( 'host tuple', '%s/config.guess' % (cfg.dir), abort=True, head=True )

    def _parseSession( self ):
        if len(self.session):
            self.spec = self.session[0]
        else:
            self.spec = ''

        ## grok GNU host tuples
        m = re.match( HostTupleProbe.GNU_TUPLE_RE, self.spec )
        if not m:
            self.fail = True
            self.msg_end = 'invalid host tuple: %s' % (self.spec)
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

class BuildAction( Action, list ):
    def __init__( self ):
        super( BuildAction, self ).__init__( 'compute', 'build tuple', abort=True )

    def _action( self ):
        ## check if --cross spec was used; must maintain 5-tuple compatibility with regex
        if options.cross:
            self.spec = os.path.basename( options.cross ).rstrip( '-' )
        else:
            self.spec = arch.mode[arch.mode.mode]

        ## grok GNU host tuples
        m = re.match( HostTupleProbe.GNU_TUPLE_RE, self.spec )
        if not m:
            self.msg_end = 'invalid host tuple: %s' % (self.spec)
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
        self.systemf = host.systemf

        ## when cross we need switch for platforms
        if options.cross:
            if self.match( '*mingw*' ):
                self.systemf = 'MinGW'
            elif self.systemf:
                self.systemf[0] = self.systemf[0].upper()
            self.title = '%s %s' % (build.systemf,self.machine)
        else:
            self.title = '%s %s' % (build.systemf,arch.mode.mode)
        self.fail = False

    ## glob-match against spec
    def match( self, *specs ):
        for spec in specs:
            if fnmatch.fnmatch( self.spec, spec ):
                return True
        return False

###############################################################################
##
## value wrapper; value is accepted only if one of host specs matcheds
## otherwise it is None (or a keyword-supplied val)
##
## result is attribute 'value'
##
class IfHost( object ):
    def __init__( self, value, *specs, **kwargs ):
        self.value = kwargs.get('none',None)
        for spec in specs:
            if host.match( spec ):
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
            if host.match( tuple[1] ):
                self.value = tuple[0]
                break

    def __str__( self ):
        return self.value

###############################################################################

class ArchAction( Action ):
    def __init__( self ):
        super( ArchAction, self ).__init__( 'compute', 'available architectures', abort=True )
        self.mode = SelectMode( 'architecture', (host.machine,host.spec) )

    def _action( self ):
        self.fail = False

        ## some match on system should be made here; otherwise we signal a warning.
        if host.match( '*-*-cygwin*' ):
            pass
        elif host.match( '*-*-darwin11.*' ):
            self.mode['i386']   = 'i386-apple-darwin%s'      % (host.release)
            self.mode['x86_64'] = 'x86_64-apple-darwin%s'    % (host.release)
        elif host.match( '*-*-darwin*' ):
            self.mode['i386']   = 'i386-apple-darwin%s'      % (host.release)
            self.mode['x86_64'] = 'x86_64-apple-darwin%s'    % (host.release)
            self.mode['ppc']    = 'powerpc-apple-darwin%s'   % (host.release)
            self.mode['ppc64']  = 'powerpc64-apple-darwin%s' % (host.release)

            ## special cases in that powerpc does not match gcc -arch value
            ## which we like to use; so it has to be removed.
            ## note: we don't know if apple will release Ssnow Leopad/ppc64 yet; just a guess.
            if 'powerpc' in self.mode:
                del self.mode['powerpc']
                self.mode.mode = 'ppc'
            elif 'powerpc64' in self.mode:
                del self.mode['powerpc64']
                self.mode.mode = 'ppc64'
        elif host.match( '*-*-linux*' ):
            pass
        elif host.match( '*-*-solaris*' ):
            pass
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
        elif self.count > 32:
            self.count = 32

        if options.launch:
            if options.launch_jobs == 0:
                self.jobs = core.count
            else:
                self.jobs = options.launch_jobs
        else:
            self.jobs = core.count

        self.msg_end = str(self.count)

###############################################################################

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

    def cli_add_option( self, parser, option ):
        parser.add_option( option, default=self.mode, metavar='MODE',
            help='select %s%s: %s' % (self.descr,self.what,self.toString()),
            action='callback', callback=self.cli_callback, type='str' )

    def cli_callback( self, option, opt_str, value, parser, *args, **kwargs ):
        if value not in self:
            raise optparse.OptionValueError( 'invalid %s%s: %s (choose from: %s)'
                % (self.descr,self.what,value,self.toString( True )) )
        self.mode = value

    def toString( self, nodefault=False ):
        keys = self.keys()
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
        self.date      = datetime(1, 1, 1)
        self.official  = 0
        self.type      = 'developer'

    def _parseSession( self ):
        for line in self.session:
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
        official_url = 'https://github.com/HandBrake/HandBrake.git' # HTTPS
        if self.url == 'git@github.com:HandBrake/HandBrake.git':    # SSH
            self.url = official_url

        if self.url == official_url:
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
                file = open( hvp, 'r' )
                self.session = file.readlines()
                file.close()
                if self.session:
                    self._parseSession()
            if self.rev != 0:
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
        if fnmatch.fnmatch( build.spec, '*-*-darwin*' ):
            url_arch = '.%s' % (arch.mode.mode)
        else:
            url_arch = ''

        suffix = ''
        if repo.tag != '':
            m = re.match( '^([0-9]+)\.([0-9]+)\.([0-9]+)-?(.*)?$', repo.tag )
            if not m:
                cfg.errln( 'Invalid repo tag format %s\n', repo.tag )
                sys.exit( 1 )
            (vmajor, vminor, vpoint, suffix) = m.groups()
            self.vmajor = int(vmajor)
            self.vminor = int(vminor)
            self.vpoint = int(vpoint)
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
            self.build = time.strftime('%Y%m%d') + '01'
            self.title = '%s %s (%s)' % (self.name,self.version,self.build)
        else:
            m = re.match('^([a-zA-Z]+)\.([0-9]+)$', suffix)
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

            self.build = time.strftime('%Y%m%d') + '00'
            self.title = '%s %s (%s)' % (self.name,self.version,self.build)

        self.url_appcast = 'https://handbrake.fr/appcast%s%s.xml' % (url_ctype,url_arch)
        self.url_appnote = 'https://handbrake.fr/appcast/%s.html' % (url_ntype)

        self.msg_end = '%s (%s)' % (self.name,repo.type)
        self.fail = False

###############################################################################

class ToolProbe( Action ):
    tools = []

    def __init__( self, var, *names, **kwargs ):
        super( ToolProbe, self ).__init__( 'find', abort=kwargs.get('abort',True) )
        if not self in ToolProbe.tools:
            ToolProbe.tools.append( self )
        self.var    = var
        self.names  = []
        self.kwargs = kwargs
        for name in names:
            if name:
                self.names.append( str(name) )
        self.name = self.names[0]
        self.pretext = self.name
        self.pathname = self.names[0]
        self.minversion = kwargs.get('minversion', None)

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
            self.version = VersionProbe( [self.pathname, '--version'], minversion=self.minversion )

    def cli_add_option( self, parser ):
        parser.add_option( '--'+self.name, metavar='PROG',
            help='[%s]' % (self.pathname),
            action='callback', callback=self.cli_callback, type='str' )

    def cli_callback( self, option, opt_str, value, parser, *args, **kwargs ):
        self.__init__( self.var, value, **self.kwargs )
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
    def __init__( self, command, minversion=None, rexpr=None, abort=False ):
        super( VersionProbe, self ).__init__( 'version probe', os.path.basename(command[0]), abort )
        self.command = command
        self.minversion = minversion
        if not rexpr:
            rexpr = '(?P<name>[^.]+)\s+(?P<svers>(?P<i0>\d+)(\.(?P<i1>\d+))?(\.(?P<i2>\d+))?)'
        self.rexpr = rexpr

    def _action( self ):
        ## pipe and redirect stderr to stdout; effects communicate result
        pipe = subprocess.Popen( self.command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT )

        ## read data into memory buffers
        data = pipe.communicate()
        self.fail = pipe.returncode != 0

        if data[0]:
            self.session = data[0].splitlines()
        else:
            self.session = []

        self.svers = '0.0.0'
        self.ivers = [0,0,0]

        try:
            if not self.fail and self.session and len(self.session):
                self.fail = True
                self._parse()
            self.fail = False
            self.msg_end = self.svers
        except Exception, x:
            self.svers = '0.0.0'
            self.ivers = [0,0,0]
            self.msg_end = str(x)

    def _dumpSession( self, printf ):
        printf( '  + %s\n', ' '.join(self.command) )
        super( VersionProbe, self )._dumpSession( printf )

    def _parse( self ):
        mo = re.match( self.rexpr, self.session[0], re.IGNORECASE )
        md = mo.groupdict()
        self.svers = md['svers']
        if 'i0' in md and md['i0']:
            self.ivers[0] = int(md['i0'])
        if 'i1' in md and md['i1']:
            self.ivers[1] = int(md['i1'])
        if 'i2' in md and md['i2']:
            self.ivers[2] = int(md['i2'])

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

class SelectTool( Action ):
    selects = []

    def __init__( self, var, name, *pool, **kwargs ):
        super( SelectTool, self ).__init__( 'select', abort=kwargs.get('abort',True) )
        self.pretext = name
        if not self in SelectTool.selects:
            SelectTool.selects.append( self )
        self.var      = var
        self.name     = name
        self.pool     = pool
        self.kwargs   = kwargs

    def _action( self ):
        self.session = []
        for i,(name,tool) in enumerate(self.pool):
            self.session.append( 'tool[%d] = %s (%s)' % (i,name,tool.pathname) )
        for (name,tool) in self.pool:
            if not tool.fail:
                self.selected = name
                self.fail = False
                self.msg_end = '%s (%s)' % (name,tool.pathname)
                break
        if self.fail:
            self.msg_end = 'not found'

    def cli_add_option( self, parser ):
        parser.add_option( '--'+self.name, metavar='MODE',
            help='select %s mode: %s' % (self.name,self.toString()),
            action='callback', callback=self.cli_callback, type='str' )

    def cli_callback( self, option, opt_str, value, parser, *args, **kwargs ):
        found = False
        for (name,tool) in self.pool:
            if name == value:
                found = True
                self.__init__( self.var, self.name, [name,tool], **kwargs )
                self.run()
                break
        if not found:
            raise optparse.OptionValueError( 'invalid %s mode: %s (choose from: %s)'
                % (self.name,value,self.toString( True )) )

    def doc_add( self, doc ):
        doc.add( self.var, self.selected )

    def toString( self, nodefault=False ):
        if len(self.pool) == 1:
            value = self.pool[0][0]
        else:
            s = ''
            for key,value in self.pool:
                s += ' ' + key
            if nodefault:
                value = s[1:]
            else:
                value = '%s [%s]' % (s[1:], self.selected )
        return value

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

    def _outputMake( self, file, namelen, name, value, append ):
        if append:
            if value == None or len(str(value)) == 0:
                file.write( '%-*s +=\n' % (namelen, name) )
            else:
                file.write( '%-*s += %s\n' % (namelen, name, value) )
        else:
            if value == None or len(str(value)) == 0:
                file.write( '%-*s  =\n' % (namelen, name) )
            else:
                file.write( '%-*s  = %s\n' % (namelen, name, value) )

    def _outputM4( self, file, namelen, name, value ):
        namelen += 7
        name = '<<__%s>>,' % name.replace( '.', '_' )
        file.write( 'define(%-*s  <<%s>>)dnl\n' % (namelen, name, value ))

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

    def output( self, file, type ):
        namelen = 0
        for item in self._elements:
            if item == None or item[0].find( '?' ) == 0:
                continue
            if len(item[0]) > namelen:
                namelen = len(item[0])
        for item in self._elements:
            if item == None:
                if type == 'm4':
                    file.write( 'dnl\n' )
                else:
                    file.write( '\n' )
                continue
            if item[0].find( '?' ) == 0:
                if item[0].find( type, 1 ) == 1:
                    file.write( '%s\n' % (item[1]) )
                continue

            if type == 'm4':
                self._outputM4( file, namelen, item[0], item[1] )
            else:
                self._outputMake( file, namelen, item[0], item[1], item[2] )

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
            raise ValueError, 'unknown file type: ' + type

        ftmp  = fname + '.tmp'
        try:
            try:
                file = cfg.open( ftmp, 'w' )
                self.output( file, type )
            finally:
                try:
                    file.close()
                except:
                    pass
        except Exception, x:
            try:
                os.remove( ftmp )
            except Exception, x:
                pass
            cfg.errln( 'failed writing to %s\n%s', ftmp, x )

        try:
            os.rename( ftmp, fname )
        except Exception, x:
            cfg.errln( 'failed writing to %s\n%s', fname, x )

###############################################################################
##
## create cli parser
##

## class to hook options and create CONF.args list
class Option( optparse.Option ):
    conf_args = []

    def _conf_record( self, opt, value ):
        ## filter out non-applicable options
        if re.match( '^--(force|launch).*$', opt ):
            return

        ## remove duplicates (last duplicate wins)
        for i,arg in enumerate( Option.conf_args ):
            if opt == arg[0]:
                del Option.conf_args[i]
                break

        if value:
            Option.conf_args.append( [opt,'%s=%s' % (opt,value)] )
        else:
            Option.conf_args.append( [opt,'%s' % (opt)] )

    def take_action( self, action, dest, opt, value, values, parser ):
        self._conf_record( opt, value )
        return optparse.Option.take_action( self, action, dest, opt, value, values, parser )

def createCLI():
    cli = OptionParser( 'usage: %prog [OPTIONS...] [TARGETS...]' )
    cli.option_class = Option

    cli.description = ''
    cli.description += 'Configure %s build system.' % (project.name)

    ## add hidden options
    cli.add_option( '--xcode-driver', default='bootstrap', action='store', help=optparse.SUPPRESS_HELP )
    cli.add_option( '--force', default=False, action='store_true', help='overwrite existing build config' )
    cli.add_option( '--verbose', default=False, action='store_true', help='increase verbosity' )

    ## add install options
    grp = OptionGroup( cli, 'Directory Locations' )
    h = IfHost( 'specify sysroot of SDK for Xcode builds', '*-*-darwin*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--sysroot', default=None, action='store', metavar='DIR',
        help=h )
    grp.add_option( '--src', default=cfg.src_dir, action='store', metavar='DIR',
        help='specify top-level source dir [%s]' % (cfg.src_dir) )
    grp.add_option( '--build', default=cfg.build_dir, action='store', metavar='DIR',
        help='specify build scratch/output dir [%s]' % (cfg.build_dir) )
    grp.add_option( '--prefix', default=cfg.prefix_dir, action='store', metavar='DIR',
        help='specify install dir for products [%s]' % (cfg.prefix_dir) )
    cli.add_option_group( grp )

    ## add feature options
    grp = OptionGroup( cli, 'Feature Options' )

    h = IfHost( 'enable assembly code in non-contrib modules', 'NOMATCH*-*-darwin*', 'NOMATCH*-*-linux*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--enable-asm', default=False, action='store_true', help=h )

    h = IfHost( 'disable GTK GUI', '*-*-linux*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--disable-gtk', default=False, action='store_true', help=h )
    h = IfHost( 'disable GTK GUI update checks', '*-*-linux*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--disable-gtk-update-checks', default=False, action='store_true', help=h )
    h = IfHost( 'enable GTK GUI (mingw)', '*-*-mingw*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--enable-gtk-mingw', default=False, action='store_true', help=h )
    h = IfHost( 'disable gstreamer (live preview)', '*-*-linux*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--disable-gst', default=False, action='store_true', help=h )
    h = IfHost( 'enable use of Intel Quick Sync Video hardware acceleration', '*-*-*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--enable-qsv', default=False, action='store_true', help=h )

    h = IfHost( 'enable HWD features', '*-*-*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--enable-hwd', default=False, action='store_true', help=h )

    h = IfHost( 'enable use of x265 encoding', '*-*-*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--enable-x265', default=True, action='store_true', help=h )
    grp.add_option( '--disable-x265', dest="enable_x265", action='store_false' )

    h = IfHost( 'enable use of fdk-aac encoder', '*-*-*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--enable-fdk-aac', dest="enable_fdk_aac", default=not host.match( '*-*-darwin*' ), action='store_true', help=h )
    grp.add_option( '--disable-fdk-aac', dest="enable_fdk_aac", action='store_false' )

    h = IfHost( 'enable use of libav aac encoder', '*-*-*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--enable-libav-aac', dest="enable_libav_aac", default=not host.match( '*-*-darwin*' ), action='store_true', help=h )
    grp.add_option( '--disable-libav-aac', dest="enable_libav_aac", action='store_false' )

    cli.add_option_group( grp )

    ## add launch options
    grp = OptionGroup( cli, 'Launch Options' )
    grp.add_option( '--launch', default=False, action='store_true',
        help='launch build, capture log and wait for completion' )
    grp.add_option( '--launch-jobs', default=1, action='store', metavar='N', type='int',
        help='allow N jobs at once; 0 to match CPU count [1]' )
    grp.add_option( '--launch-args', default=None, action='store', metavar='ARGS',
        help='specify additional ARGS for launch command' )
    grp.add_option( '--launch-quiet', default=False, action='store_true',
        help='do not echo build output while waiting' )
    cli.add_option_group( grp )

    ## add compile options
    grp = OptionGroup( cli, 'Compiler Options' )
    debugMode.cli_add_option( grp, '--debug' )
    optimizeMode.cli_add_option( grp, '--optimize' )
    arch.mode.cli_add_option( grp, '--arch' )
    grp.add_option( '--cross', default=None, action='store', metavar='SPEC',
        help='specify GCC cross-compilation spec' )
    h = IfHost( 'specify Mac OS X deployment target for Xcode builds', '*-*-darwin*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--minver', default=None, action='store', metavar='VER',
        help=h )

    h = IfHost( 'Build and use local yasm', '*-*-*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--enable-local-yasm', default=False, action='store_true', help=h )
    h = IfHost( 'Build and use local autotools', '*-*-*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--enable-local-autotools', default=False, action='store_true', help=h )
    h = IfHost( 'Build and use local cmake', '*-*-*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--enable-local-cmake', default=False, action='store_true', help=h )
    h = IfHost( 'Build and use local pkg-config', '*-*-darwin*', none=optparse.SUPPRESS_HELP ).value
    grp.add_option( '--enable-local-pkgconfig', default=False, action='store_true', help=h )

    cli.add_option_group( grp )

    ## add Xcode options
    if host.match( '*-*-darwin*' ):
        grp = OptionGroup( cli, 'Xcode Options' )
        grp.add_option( '--disable-xcode', default=False, action='store_true',
            help='disable Xcode' )
        grp.add_option( '--xcode-symroot', default='xroot', action='store', metavar='DIR',
            help='specify root of the directory hierarchy that contains product files and intermediate build files' )
        xcconfigMode.cli_add_option( grp, '--xcode-config' )
        cli.add_option_group( grp )

    ## add tool locations
    grp = OptionGroup( cli, 'Tool Basenames and Locations' )
    for tool in ToolProbe.tools:
        tool.cli_add_option( grp )
    cli.add_option_group( grp )

    ## add tool modes
    grp = OptionGroup( cli, 'Tool Options' )
    for select in SelectTool.selects:
        select.cli_add_option( grp )
    cli.add_option_group( grp )

    ## add build options
    grp = OptionGroup( cli, 'Build Options' )
    grp.add_option( '--snapshot', default=False, action='store_true',
                    help='Force a snapshot build' )
    cli.add_option_group( grp )

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
        except Exception, x:
            cfg.errln( 'launch failure: %s', x )
        for line in pipe.stdout:
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

    ## create main objects; actions/probes run() is delayed.
    ## if any actions must be run earlier (eg: for configure --help purposes)
    ## then run() must be invoked earlier. subequent run() invocations
    ## are ignored.
    cfg   = Configure( verbose )
    host  = HostTupleProbe(); host.run()

    cfg.prefix_dir = ForHost( '/usr/local', ['/Applications','*-*-darwin*'] ).value

    build = BuildAction()
    arch  = ArchAction(); arch.run()

    ## create remaining main objects
    core    = CoreProbe()
    repo    = RepoProbe()
    project = Project()

    ## create tools in a scope
    class Tools:
        ar    = ToolProbe( 'AR.exe',    'ar' )
        cp    = ToolProbe( 'CP.exe',    'cp' )
        curl  = ToolProbe( 'CURL.exe',  'curl', abort=False )
        gcc   = ToolProbe( 'GCC.gcc',   'gcc', IfHost( 'gcc-4', '*-*-cygwin*' ))

        if host.match( '*-*-darwin*' ):
            gmake = ToolProbe( 'GMAKE.exe', 'make', 'gmake' )
        else:
            gmake = ToolProbe( 'GMAKE.exe', 'gmake', 'make' )

        m4       = ToolProbe( 'M4.exe',       'gm4', 'm4' )
        mkdir    = ToolProbe( 'MKDIR.exe',    'mkdir' )
        patch    = ToolProbe( 'PATCH.exe',    'gpatch', 'patch' )
        rm       = ToolProbe( 'RM.exe',       'rm' )
        ranlib   = ToolProbe( 'RANLIB.exe',   'ranlib' )
        strip    = ToolProbe( 'STRIP.exe',    'strip' )
        tar      = ToolProbe( 'TAR.exe',      'gtar', 'tar' )
        wget     = ToolProbe( 'WGET.exe',     'wget', abort=False )
        yasm     = ToolProbe( 'YASM.exe',     'yasm', abort=False, minversion=[1,2,0] )
        autoconf = ToolProbe( 'AUTOCONF.exe', 'autoconf', abort=False )
        automake = ToolProbe( 'AUTOMAKE.exe', 'automake', abort=False )
        cmake    = ToolProbe( 'CMAKE.exe',    'cmake', abort=False )
        libtool  = ToolProbe( 'LIBTOOL.exe',  'libtool', abort=False )
        pkgconfig = ToolProbe( 'PKGCONFIG.exe', 'pkg-config', abort=False )

        xcodebuild = ToolProbe( 'XCODEBUILD.exe', 'xcodebuild', abort=False )
        lipo       = ToolProbe( 'LIPO.exe',       'lipo', abort=False )

        fetch = SelectTool( 'FETCH.select', 'fetch', ['wget',wget], ['curl',curl] )

    ## run tool probes
    for tool in ToolProbe.tools:
        tool.run()
    for select in SelectTool.selects:
        select.run()

    debugMode = SelectMode( 'debug', ('none','none'), ('min','min'), ('std','std'), ('max','max') )
    optimizeMode = SelectMode( 'optimize', ('none','none'), ('speed','speed'), ('size','size'), default='speed' )

    ## find xcconfig values
    xcconfigMode = SelectMode( 'xcconfig', ('none',None), what='' )
    if host.match( '*-*-darwin*' ):
        for xc in glob.glob( os.path.join(cfg.dir, '../macosx/xcconfig/*.xcconfig') ):
            bname = os.path.basename( xc )
            xname = os.path.splitext( bname )
            if xname and xname[0]:
                xcconfigMode[xname[0]] = bname
        if not 'native' in xcconfigMode:
            raise Exception( 'native xcconfig not found' )
        xcconfigMode.default = 'native'
        xcconfigMode.mode = xcconfigMode.default

    ## create CLI and parse
    cli = createCLI()
    (options,args) = cli.parse_args()

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

    ## re-run tools with cross-compilation needs
    if options.cross:
        for tool in ( Tools.ar, Tools.gcc, Tools.ranlib, Tools.strip ):
            tool.__init__( tool.var, '%s-%s' % (options.cross,tool.name), **tool.kwargs )
            tool.run()

    ## run delayed actions
    for action in Action.actions:
        action.run()

    ## enable local yasm when yasm probe fails or version is too old
    ## x264 requires 1.2.0+
    if not options.enable_local_yasm:
        if Tools.yasm.fail:
            stdout.write( 'note: enabling local yasm: missing system yasm\n' )
            options.enable_local_yasm = True
        elif Tools.yasm.version.inadequate():
            stdout.write( 'note: enabling local yasm: minimum required version is %s and %s is %s\n' % ('.'.join([str(i) for i in Tools.yasm.version.minversion]),Tools.yasm.pathname,Tools.yasm.version.svers) )
            options.enable_local_yasm = True

    ## enable local autotools when any of { autoconf, automake, libtool } probe fails
    if not options.enable_local_autotools and (Tools.autoconf.fail or Tools.automake.fail or Tools.libtool.fail):
        stdout.write( 'note: enabling local autotools\n' )
        options.enable_local_autotools = True

    ## enable local cmake when cmake probe fails
    if not options.enable_local_cmake and (Tools.cmake.fail):
        stdout.write( 'note: enabling local cmake\n' )
        options.enable_local_cmake = True

    ## enable local pkg-config when probe fails
    if not options.enable_local_pkgconfig and Tools.pkgconfig.fail:
        stdout.write( 'note: enabling local pkgconfig\n' )
        options.enable_local_pkgconfig = True

    if build.system == 'mingw':
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
        pthreadGC2 = LDProbe( 'static pthread', '%s -static' % Tools.gcc.pathname, '-lpthreadGC2', pthread_test )
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

    rv = regcomp(&exp, "^[0-9]+$";", REG_EXTENDED);
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

    ## cfg hook before doc prep
    cfg.doc_ready()

    ## create document object
    doc = ConfigDocument()
    doc.addComment( 'generated by configure on %s', time.strftime( '%c' ))

    ## add configure line for reconfigure purposes
    doc.addBlank()
    args = []
    for arg in Option.conf_args:
        args.append( arg[1] )
    doc.add( 'CONF.args', ' '.join( args ))

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
    doc.add( 'HOST.spec',    host.spec )
    doc.add( 'HOST.machine', host.machine )
    doc.add( 'HOST.vendor',  host.vendor )
    doc.add( 'HOST.system',  host.system )
    doc.add( 'HOST.systemf', host.systemf )
    doc.add( 'HOST.release', host.release )
    doc.add( 'HOST.extra',   host.extra )
    doc.add( 'HOST.title',   '%s %s' % (host.systemf,arch.mode.default) )
    doc.add( 'HOST.ncpu',    core.count )

    doc.addBlank()
    doc.add( 'BUILD.spec',    build.spec )
    doc.add( 'BUILD.machine', build.machine )
    doc.add( 'BUILD.vendor',  build.vendor )
    doc.add( 'BUILD.system',  build.system )
    doc.add( 'BUILD.systemf', build.systemf )
    doc.add( 'BUILD.release', build.release )
    doc.add( 'BUILD.extra',   build.extra )
    doc.add( 'BUILD.title',   build.title )
    doc.add( 'BUILD.ncpu',    core.count )
    doc.add( 'BUILD.jobs',    core.jobs )

    doc.add( 'BUILD.cross', int(options.cross != None or arch.mode.mode != arch.mode.default) )
    if options.cross:
        doc.add( 'BUILD.cross.prefix', '%s-' % (options.cross) )
    else:
        doc.add( 'BUILD.cross.prefix', '' )

    doc.add( 'BUILD.date',   time.strftime('%c') )
    doc.add( 'BUILD.arch',   arch.mode.mode )

    doc.addBlank()
    doc.add( 'SRC',     cfg.src_final )
    doc.add( 'SRC/',    cfg.src_final + os.sep )
    doc.add( 'BUILD',   cfg.build_final )
    doc.add( 'BUILD/',  cfg.build_final + os.sep )
    doc.add( 'PREFIX',  cfg.prefix_final )
    doc.add( 'PREFIX/', cfg.prefix_final + os.sep )

    doc.addBlank()
    doc.add( 'FEATURE.local_yasm', int( options.enable_local_yasm ))
    doc.add( 'FEATURE.local_autotools', int( options.enable_local_autotools ))
    doc.add( 'FEATURE.local_cmake', int( options.enable_local_cmake ))
    doc.add( 'FEATURE.local_pkgconfig', int( options.enable_local_pkgconfig ))
    doc.add( 'FEATURE.asm',        'disabled' )
    doc.add( 'FEATURE.gtk',        int( not options.disable_gtk ))
    doc.add( 'FEATURE.gtk.update.checks', int( not options.disable_gtk_update_checks ))
    doc.add( 'FEATURE.gtk.mingw',  int( options.enable_gtk_mingw ))
    doc.add( 'FEATURE.gst',        int( not options.disable_gst ))
    doc.add( 'FEATURE.fdk_aac',    int( options.enable_fdk_aac ))
    doc.add( 'FEATURE.libav_aac',  int( options.enable_libav_aac ))
    doc.add( 'FEATURE.qsv',        int( options.enable_qsv ))
    doc.add( 'FEATURE.hwd',        int( options.enable_hwd ))
    doc.add( 'FEATURE.xcode',      int( not (Tools.xcodebuild.fail or options.disable_xcode or options.cross) ))
    doc.add( 'FEATURE.x265',       int( options.enable_x265 ))

    if not Tools.xcodebuild.fail and not options.disable_xcode:
        doc.addBlank()
        doc.add( 'XCODE.driver', options.xcode_driver )
        if os.path.isabs(options.xcode_symroot):
            doc.add( 'XCODE.symroot', options.xcode_symroot )
        else:
            doc.add( 'XCODE.symroot', os.path.abspath(os.path.join(cfg.build_dir,options.xcode_symroot)) )
        doc.add( 'XCODE.xcconfig', xcconfigMode[xcconfigMode.mode] )

    if build.system == 'mingw':
        doc.addBlank()
        if not dlfcn.fail:
            doc.add( 'HAS.dlfcn', 1 )
        if not pthreadGC2.fail:
            doc.add( 'HAS.pthreadGC2', 1 )
        elif not pthread.fail:
            doc.add( 'HAS.pthread', 1 )
        if not bz2.fail:
            doc.add( 'HAS.bz2', 1 )
        if not libz.fail:
            doc.add( 'HAS.libz', 1 )
        if not iconv.fail:
            doc.add( 'HAS.iconv', 1 )
        if not regex.fail:
            doc.add( 'HAS.regex', 1 )
        if strtok_r.fail:
            doc.add( 'COMPAT.strtok_r', 1 )

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
    for select in SelectTool.selects:
        select.doc_add( doc )

    doc.addBlank()
    doc.add( 'GCC.archs', arch.mode.mode )
    if build.match( '*-*-darwin*' ):
        doc.add( 'GCC.sysroot', cfg.sysroot_dir )
        doc.add( 'GCC.minver', cfg.minver )
    else:
        doc.add( 'GCC.sysroot', '' )
        doc.add( 'GCC.minver', '' )

    if build.match( 'i?86-*' ):
        doc.add( 'LIBHB.GCC.D', 'ARCH_X86_32', append=True )
    elif build.match( 'x86_64-*' ):
        doc.add( 'LIBHB.GCC.D', 'ARCH_X86_64', append=True )

    if options.enable_asm and ( not Tools.yasm.fail or options.enable_local_yasm ):
        asm = ''
        if build.match( 'i?86-*' ):
            asm = 'x86'
            doc.add( 'LIBHB.GCC.D', 'HAVE_MMX', append=True )
            doc.add( 'LIBHB.YASM.D', 'ARCH_X86', append=True )
            if build.match( '*-*-darwin*' ):
                doc.add( 'LIBHB.YASM.f', 'macho32' )
            else:
                doc.add( 'LIBHB.YASM.f', 'elf32' )
            doc.add( 'LIBHB.YASM.m', 'x86' )
        elif build.match( 'x86_64-*' ):
            asm = 'x86'
            doc.add( 'LIBHB.GCC.D', 'HAVE_MMX ARCH_X86_64', append=True )
            if build.match( '*-*-darwin*' ):
                doc.add( 'LIBHB.YASM.D', 'ARCH_X86_64 PIC', append=True )
                doc.add( 'LIBHB.YASM.f', 'macho64' )
            else:
                doc.add( 'LIBHB.YASM.D', 'ARCH_X86_64', append=True )
                doc.add( 'LIBHB.YASM.f', 'elf64' )
            doc.add( 'LIBHB.YASM.m', 'amd64' )
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
    if options.launch:
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

except AbortError, x:
    stderr.write( 'ERROR: %s\n' % (x) )
    try:
        cfg.record_log()
    except:
        pass
    sys.exit( 1 )

sys.exit( 0 )
