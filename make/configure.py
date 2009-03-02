import fnmatch
import optparse
import os
import platform
import re
import subprocess
import sys
import time

from optparse import OptionGroup
from optparse import OptionGroup
from optparse import OptionParser
from sys import stderr
from sys import stdout

###############################################################################

def errf( format, *args ):
    stderr.write( ('ERROR: ' + format + '\n') % args )
    sys.exit( 1 )

def outf( format, *args ):
    stdout.write( (format + '\n') % args )

###############################################################################

## Expand values of iterable object into a decent string representation.
##
def expandValues( obj ):
    buf = ''
    for v in obj:
        buf += ', ' + v
    return '{ ' + buf[2:] + ' }'

###############################################################################

## Find executable by searching path.
## On success, returns full pathname of executable.
## On fail, returns None.
##
def findExecutable( name ):
    if len( os.path.split(name)[0] ):
        return name if os.access( name, os.X_OK ) else None

    if not os.environ.has_key( 'PATH' ) or os.environ[ 'PATH' ] == '':
        path = os.defpath
    else:
        path = os.environ['PATH']

    for dir in path.split( os.pathsep ):
        f = os.path.join( dir, name )
        if os.access( f, os.X_OK ):
            return f
    return None

###############################################################################

def computeDefaultMakeJobs():
    ## good for darwin9.6.0 and linux
    try:
        n = os.sysconf( 'SC_NPROCESSORS_ONLN' )
        if n < 1:
            n = 1
        return n
    except:
        pass
    ## windows
    try:
        n = int( os.environ['NUMBER_OF_PROCESSORS'] )
        if n < 1:
            n = 1
        return n
    except:
        pass
    return 1

###############################################################################

## taken from python2.6 -- we need it
def relpath(path, start=os.path.curdir):
    """Return a relative version of a path"""

    if not path:
        raise ValueError("no path specified")

    start_list = os.path.abspath(start).split(os.sep)
    path_list = os.path.abspath(path).split(os.sep)

    # Work out how much of the filepath is shared by start and path.
    i = len(os.path.commonprefix([start_list, path_list]))

    rel_list = [os.pardir] * (len(start_list)-i) + path_list[i:]
    if not rel_list:
        return os.path.curdir
    return os.path.join(*rel_list)

###############################################################################

# compute project dir which should be 2 dirs below this script
build_dir = os.curdir
project_dir = os.path.normpath( sys.argv[0] )
for i in range( 2 ):
    project_dir = os.path.dirname( project_dir )
if len( project_dir ) == 0:
    project_dir = os.curdir

###############################################################################

## model gnu-autotools platform guess
##
## native format:
##   (PROC)-(VENDOR)-(SYSTEM)(RELEASE)-(EXTRA)
##
## examples:
##   i386-apple-darwin9.6.0    (Mac OS X 10.5.6 Intel)
##   powerpc-apple-darwin9.6.0 (Mac OS X 10.5.6 PPC)
##   i686-pc-cygwin            (Cygwin, Microsoft Vista)
##   x86_64-unknown-linux-gnu  (Linux, Fedora 10 x86_64)
##
class Guess:
    def __init__( self ):
        self.machine = 'unknown'
        self.vendor  = 'unknown'
        self.system  = 'unknown'
        self.systemc = 'Unknown'
        self.release = '0.0.0'
        self.extra   = ''

        p_system    = platform.system().lower()
        p_systemc   = platform.system()
        p_release   = platform.release().lower()
        p_processor = platform.processor().lower()
        p_machine   = platform.machine().lower()

        if re.match( 'cygwin', p_system ):
            self.machine = p_machine
            self.vendor  = 'pc'
            self.system  = 'cygwin'
            self.systemc = 'Cygwin'
            self.release = ''
            self.extra   = ''
        elif re.match( 'darwin', p_system ):
            self.machine = p_machine
            self.vendor  = 'apple'
            self.system  = p_system
            self.systemc = p_systemc
            self.release = p_release
            self.extra   = ''
        elif re.match( 'linux', p_system ):
            self.machine = p_machine
            self.vendor  = 'unknown'
            self.system  = p_system
            self.systemc = p_systemc
            self.release = ''
            self.extra   = 'gnu'
            self.title   = 'Linux %s' % (p_machine)
        else:
            errf( 'unrecognized host system: %s', p_system )

    def __str__( self ):
        if len(self.extra):
            return '%s-%s-%s%s-%s' % (self.machine,self.vendor,self.system,self.release,self.extra)
        else:
            return '%s-%s-%s%s' % (self.machine,self.vendor,self.system,self.release)

    def match( self, spec ):
        return fnmatch.fnmatch( str(self), spec )

###############################################################################

# a tool represents a command-line tool which may be searched for in PATH
class Tool:
    def __init__( self, parent, optional, var, *pool ):
        self.name     = pool[0]
        self.optional = optional
        self.var      = var
        self.pool     = pool
        self.found    = None
        if parent:
            parent.register( self )

    def addToConfig( self, config ):
        config.add( self.var, self.found )

    def addToGroup( self, group ):
        group.add_option( '', '--' + self.name, help='specify %s location' % (self.name), default=None, metavar='EXE' )

    def locate( self, options ):
        spec = options.__dict__[self.name]
        pool = self.pool if not spec else [spec]
        for p in pool:
            self.found = findExecutable( p )
            if self.found:
                outf( 'located %s: %s', self.name, self.found )
                return
        if self.optional:
            outf( 'missing: %s (optional)', self.name )
        else:
            errf( 'unable to locate tool: %s', self.name )

## a select tool picks first found from a list of tools
class SelectTool( Tool ):
    def __init__( self, parent, var, name, *pool ):
        self.var     = var
        self.name    = name
        self.pool    = pool
        self.found   = None

        self.poolMap = {}
        for p in self.pool:
            self.poolMap[p.name] = p
        if parent:
            parent.register( self )

    def addToConfig( self, config ):
        config.add( self.var, self.found )

    def addToGroup( self, group ):
        group.add_option( '', '--' + self.name, help='select %s mode: %s' % (self.name,expandValues(self.poolMap)),
            default=self.name, metavar='MODE' )

    def locate( self, options ):
        spec = options.__dict__[self.name]
        if spec in self.poolMap:
            self.found = spec
            return
        for p in self.pool:
            if p.found:
                self.found = p.name
                outf( 'selected %s: %s', self.name, self.found )
                return
        errf( 'require at least one location of: %s', expandValues( self.poolMap ))

###############################################################################

class ToolSet:
    def __init__( self ):
        self.items = []
        Tool( self, False, 'AR.exe',    'ar' )
        Tool( self, False, 'CP.exe',    'cp' )
        Tool( self, True,  'CURL.exe',  'curl' )
        Tool( self, False, 'GCC.gcc',   'gcc', 'gcc-4' )
        Tool( self, False, 'M4.exe',    'm4' )
        Tool( self, False, 'MKDIR.exe', 'mkdir' )
        Tool( self, False, 'PATCH.exe', 'patch' )
        Tool( self, False, 'RM.exe',    'rm' )
        Tool( self, False, 'TAR.exe',   'tar' )
        Tool( self, True,  'WGET.exe',  'wget' )

        SelectTool( self, 'FETCH.select', 'fetch', self.wget, self.curl )

    def register( self, item ):
        self.__dict__[item.name] = item
        self.items.append( item )

###############################################################################

class OptionMode( list ):
    def __init__( self, default, *items ):
        super( OptionMode, self ).__init__( items )
        self.default = items[default]
        self.mode = self.default

    def __str__( self ):
        return ' '.join( self ).replace( self.mode, '*'+self.mode )

    def addToGroup( self, group, option, name ):
        group.add_option( '', option, help='select %s mode: %s' % (name,self), default=self.mode, metavar='MODE' )

    def setFromOption( self, name, mode ):
        if mode not in self:
            errf( 'invalid %s mode: %s', name, mode )
        self.mode = mode

###############################################################################

## create singletons
guessHost  = Guess()
guessBuild = Guess()

makeTool = Tool( None, False, 'CONF.make', 'gmake', 'make' )
tools = ToolSet()

debugMode    = OptionMode( 0, 'none', 'min', 'std', 'max' )
optimizeMode = OptionMode( 1, 'none', 'speed', 'size' )

## populate platform-specific architecture modes
if guessHost.match( 'i386-*-darwin8.*' ):
    archMode = OptionMode( 0, 'i386', 'ppc' )
elif guessHost.match( 'powerpc-*-darwin8.*' ):
    archMode = OptionMode( 1, 'i386', 'ppc' )
elif guessHost.match( 'i386-*-darwin9.*' ):
    archMode = OptionMode( 0, 'i386', 'x86_64', 'ppc', 'ppc64' )
elif guessHost.match( 'powerpc-*-darwin9.*' ):
    archMode = OptionMode( 2, 'i386', 'x86_64', 'ppc', 'ppc64' )
else:
    archMode = OptionMode( 0, guessHost.machine )

if guessHost.match( '*-*-darwin*' ):
    d_prefix = '/Applications'
else: 
    d_prefix = '/usr/local'

## create parser
parser = OptionParser( 'Usage: %prog' )

group = OptionGroup( parser, 'Installation Options' )
group.add_option( '', '--prefix', default=d_prefix, action='store',
    help='specify destination for final products (%s)' % (d_prefix) )
parser.add_option_group( group )

group = OptionGroup( parser, 'Feature Options' )
group.add_option( '', '--disable-xcode', default=False, action='store_true',
    help='disable Xcode (Darwin only)' )
group.add_option( '', '--disable-gtk', default=False, action='store_true',
    help='disable GTK GUI (Linux only)' )
parser.add_option_group( group )

## add launch options
group = OptionGroup( parser, 'Launch Options' )
group.add_option( '', '--launch', default=False, action='store_true',
    help='launch build, capture log and wait for completion' )
group.add_option( '', '--launch-jobs', default=1, action='store', metavar='N',
    help='allow N jobs at once; 0 to match CPU count (1)' )
group.add_option( '', '--launch-args', default=None, action='store', metavar='ARGS',
    help='specify additional ARGS for launch command' )
group.add_option( '', '--launch-dir', default='build', action='store', metavar='DIR',
    help='specify scratch DIR to use for build (build)' )
group.add_option( '', '--launch-force', default=False, action='store_true',
    help='force use of scratch directory even if exists' )
group.add_option( '', '--launch-log', default='log.txt', action='store', metavar='FILE',
    help='specify log FILE (log.txt)' )
group.add_option( '', '--launch-quiet', default=False, action='store_true',
    help='do not echo build output' )
parser.add_option_group( group )

## add compile options
group = OptionGroup( parser, 'Compiler Options' )
debugMode.addToGroup( group, '--debug', 'debug' )
optimizeMode.addToGroup( group, '--optimize', 'optimize' )
archMode.addToGroup( group, '--arch', 'architecutre' )
parser.add_option_group( group )

## add tool options
group = OptionGroup( parser, 'Tool Options' )
makeTool.addToGroup( group )
for tool in tools.items:
    tool.addToGroup( group )
parser.add_option_group( group )

(options, args) = parser.parse_args()

## recompute values when launch mode
if options.launch:
    options.launch_jobs = int(options.launch_jobs)
    build_dir = options.launch_dir
    if os.path.isabs( build_dir ):
        project_dir = os.getcwd() 
    else:
        project_dir = os.path.normpath( relpath( project_dir, build_dir ))
    if options.launch_jobs == 0:
        options.launch_jobs = computeDefaultMakeJobs()
    if options.launch_jobs < 1:
        options.launch_jobs = 1
    elif options.launch_jobs > 8:
        options.launch_jobs = 8

## make sure configure does not run in source root
if os.path.abspath( project_dir ) == os.path.abspath( build_dir ):
    errf( 'scratch (build) directory must not be the same as source root' )

## validate modes
debugMode.setFromOption( 'debug', options.debug )
optimizeMode.setFromOption( 'optimize', options.optimize )
archMode.setFromOption( 'architecture', options.arch )

## update guessBuild as per architecture mode
if guessHost.match( '*-*-darwin*' ):
    if archMode.mode == 'i386':
        guessBuild.machine = 'i386'
    elif archMode.mode == 'x86_64':
        guessBuild.machine = 'x86_64'
    elif archMode.mode == 'ppc':
        guessBuild.machine = 'powerpc'
    elif archMode.mode == 'ppc64':
        guessBuild.machine = 'powerpc64'
else:
    guessBuild.machine = archMode.mode
guessBuild.cross = 0 if archMode.default == archMode.mode else 1

# locate tools
makeTool.locate( options )
for tool in tools.items:
    tool.locate( options )

###############################################################################

## Repository object.
## Holds information gleaned from subversion working dir.
##
## Builds are classed into one of the following types:
##
##  release
##      must be built from official svn with '/tags/' in the url
##  developer
##      must be built from official svn but is not a release
##  unofficial
##      all other builds
##
class Repository:
    def __init__( self ):
        self.url       = 'svn://nowhere.com/project/unknown'
        self.root      = 'svn://nowhere.com/project'
        self.branch    = 'unknown'
        self.uuid      = '00000000-0000-0000-0000-000000000000';
        self.rev       = 0
        self.date      = '0000-00-00 00:00:00 -0000'
        self.wcversion = 'exported'
        self.official  = 0
        self.type      = 'unofficial'

        # parse output: svnversion PROJECT_DIR
        cmd = 'svnversion ' + project_dir
        print 'running: %s' % (cmd)
        try:
            p = subprocess.Popen( cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE )
            p.wait();
            if p.returncode == 0:
                self.wcversion = p.stdout.readline().rstrip()
        except:
            pass

        # parse output: svn info PROJECT_DIR
        cmd = 'svn info ' + project_dir
        print 'running: %s' % (cmd)
        try:
            p = subprocess.Popen( cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE )
            p.wait();
            if p.returncode == 0:
                for line in p.stdout:
                    (name,value) = re.match( '([^:]+):\\s+(.+)', line.rstrip() ).groups()
                    if name == 'URL':
                        self.url = value
                    elif name == 'Repository Root':
                        self.root = value
                    elif name == 'Repository UUID':
                        self.uuid = value
                    elif name == 'Revision':
                        self.rev = int( value )
                    elif name == 'Last Changed Date':
                        # strip chars in parens
                        if value.find( ' (' ):
                            self.date = value[0:value.find(' (')]
                        else:
                            self.date = value
        except:
            pass

        i = self.url.rfind( '/' )
        if i != -1 and i < len(self.url)-1:
            self.branch = self.url[i+1:]

        # official UUID behavior
        if self.uuid == 'b64f7644-9d1e-0410-96f1-a4d463321fa5':
            self.official = 1
            m = re.match( '([^:]+)://([^/]+)/(.+)', self.url )
            if m and re.match( 'tags/', m.group( 3 )):
                self.type = 'release'
            else:
                self.type = 'developer'

###############################################################################

## Project object.
## Contains manually updated version numbers consistent with HB releases
## and other project metadata.
##
class Project:
    def __init__( self ):
        if repo.type == 'unofficial':
            self.name          = 'NoNameBrand'
            self.acro_lower    = 'nnb'
            self.acro_upper    = 'NNB'
            self.url_website   = 'http://nonamebrand.com'
            self.url_community = 'http://forum.nonamebrand.com'
            self.url_irc       = 'irc://irc.freenode.net/nonamebrand'
        else:
            self.name          = 'HandBrake'
            self.acro_lower    = 'hb'
            self.acro_upper    = 'HB'
            self.url_website   = 'http://handbrake.fr'
            self.url_community = 'http://forum.handbrake.fr'
            self.url_irc       = 'irc://irc.freenode.net/handbrake'

        self.name_lower = self.name.lower()
        self.name_upper = self.name.upper()

        self.vmajor = 0
        self.vminor = 9
        self.vpoint = 4

        appcastfmt = 'http://handbrake.fr/appcast%s.xml'

        if repo.type == 'release':
            self.version = '%d.%d.%d' % (self.vmajor,self.vminor,self.vpoint)
            self.url_appcast = appcastfmt % ('')
            self.build = time.strftime('%Y%m%d') + '00'
            self.title = '%s %s (%s)' % (self.name,self.version,self.build)
        elif repo.type == 'developer':
            self.version = 'svn%d' % (repo.rev)
            self.url_appcast = appcastfmt % ('_unstable')
            self.build = time.strftime('%Y%m%d') + '01'
            self.title = '%s svn%d (%s)' % (self.name,repo.rev,self.build)
        else:
            self.version = 'svn%d' % (repo.rev)
            self.version = '%d.%d.%d' % (self.vmajor,self.vminor,self.vpoint)
            self.url_appcast = appcastfmt % ('_unofficial')
            self.build = time.strftime('%Y%m%d') + '99'
            self.title = 'Unofficial svn%d (%s)' % (repo.rev,self.build)

###############################################################################

## Config object used to output gnu-make or gnu-m4 output.
##
## Use add() to add NAME/VALUE pairs suitable for both make/m4.
## Use addBlank() to add a linefeed for both make/m4.
## Use addMake() to add a make-specific line.
## Use addM4() to add a m4-specific line.
##
class Config:
    def __init__( self ):
        self._items = []

    def add( self, name, value ):
        self._items.append( (name,value) )

    def addBlank( self ):
        self._items.append( None )

    def addComment( self, format, *args ):
        self.addMake( '## ' + format % args )
        self.addM4( 'dnl ' + format % args )

    def addMake( self, line ):
        self._items.append( ('?make',line) )

    def addM4( self, line ):
        self._items.append( ('?m4',line) )

    def output( self, file, type ):
        namelen = 0
        for item in self._items:
            if item == None or item[0].find( '?' ) == 0:
                continue
            if len(item[0]) > namelen:
                namelen = len(item[0])
        for item in self._items:
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
                self._outputMake( file, namelen, item[0], item[1] )

    def _outputMake( self, file, namelen, name, value ):
        file.write( '%-*s = %s\n' % (namelen, name, value ))

    def _outputM4( self, file, namelen, name, value ):
        namelen += 7
        name = '<<__%s>>,' % name.replace( '.', '_' )
        file.write( 'define(%-*s  <<%s>>)dnl\n' % (namelen, name, value ))

###############################################################################

## create configure line, stripping arg --launch, quoting others
configure = []
for arg in sys.argv[1:]:
    #if arg.find( '--launch' ) == 0:
    #    continue
    if arg == '--launch':
        continue
    configure.append( '"%s"' % (arg.replace('"', '\\"')) )

## create singletones
repo = Repository()
project = Project()
config  = Config()

config.addComment( 'generated by configure on %s', time.strftime( '%c' ))

config.addBlank()
config.add( 'CONF.args', ' '.join( configure ))

config.addBlank()
config.add( 'HB.title',         project.title )
config.add( 'HB.name',          project.name )
config.add( 'HB.name.lower',    project.name_lower )
config.add( 'HB.name.upper',    project.name_upper )
config.add( 'HB.acro.lower',    project.acro_lower )
config.add( 'HB.acro.upper',    project.acro_upper )

config.add( 'HB.url.website',   project.url_website )
config.add( 'HB.url.community', project.url_community )
config.add( 'HB.url.irc',       project.url_irc )
config.add( 'HB.url.appcast',   project.url_appcast )

config.add( 'HB.version.major',  project.vmajor )
config.add( 'HB.version.minor',  project.vminor )
config.add( 'HB.version.point',  project.vpoint )
config.add( 'HB.version',        project.version )
config.add( 'HB.version.hex',    '%04x%02x%02x%02x%06x' % (project.vmajor,project.vminor,project.vpoint,0,repo.rev) )

config.add( 'HB.build', project.build )

config.add( 'HB.repo.url',       repo.url )
config.add( 'HB.repo.root',      repo.root )
config.add( 'HB.repo.branch',    repo.branch )
config.add( 'HB.repo.uuid',      repo.uuid )
config.add( 'HB.repo.rev',       repo.rev )
config.add( 'HB.repo.date',      repo.date )
config.add( 'HB.repo.wcversion', repo.wcversion )
config.add( 'HB.repo.official',  repo.official )
config.add( 'HB.repo.type',      repo.type )

config.addBlank()
config.add( 'HOST.spec',    guessHost )
config.add( 'HOST.machine', guessHost.machine )
config.add( 'HOST.vendor',  guessHost.vendor )
config.add( 'HOST.system',  guessHost.system )
config.add( 'HOST.systemc', guessHost.systemc )
config.add( 'HOST.release', guessHost.release )
config.add( 'HOST.title',   '%s %s' % (guessHost.systemc,archMode.default) )
config.add( 'HOST.extra',   guessHost.extra )

config.addBlank()
config.add( 'BUILD.spec',    guessBuild )
config.add( 'BUILD.machine', guessBuild.machine )
config.add( 'BUILD.vendor',  guessBuild.vendor )
config.add( 'BUILD.system',  guessBuild.system )
config.add( 'BUILD.systemc', guessBuild.systemc )
config.add( 'BUILD.release', guessBuild.release )
config.add( 'BUILD.title',   '%s %s' % (guessBuild.systemc,archMode.mode) )
config.add( 'BUILD.extra',   guessBuild.extra )
config.add( 'BUILD.cross',   guessBuild.cross )
config.add( 'BUILD.date',    time.strftime('%c') )
config.add( 'BUILD.arch',    archMode.mode )

config.addBlank()
config.add( 'BUILD/',   os.curdir + os.sep )
config.add( 'PROJECT/', project_dir + os.sep )

config.addBlank()
config.add( 'INSTALL.prefix', options.prefix )
config.add( 'INSTALL.prefix/', '$(INSTALL.prefix)/' )

config.addBlank()
config.add( 'FEATURE.xcode', 0 if options.disable_xcode else 1 )
config.add( 'FEATURE.gtk',   0 if options.disable_gtk   else 1 )

config.addMake( '' )
config.addMake( '## include main definitions' )
config.addMake( 'include $(PROJECT/)make/include/main.defs' )

config.addBlank()
for tool in tools.items:
    tool.addToConfig( config )

config.addBlank()
config.add( 'GCC.archs', archMode.mode if guessBuild.cross else '' )
config.add( 'GCC.g', options.debug )
config.add( 'GCC.O', options.optimize )

config.addMake( '' )
config.addMake( '## include (optional) customization file' )
config.addMake( '-include $(BUID/)GNUmakefile.custom' )

config.addMake( '' )
config.addMake( '## include main rules' )
config.addMake( 'include $(PROJECT/)make/include/main.rules' )

###############################################################################

# generate make or m4 file
def generate( type ):
    if type == 'make':
        fname = 'GNUmakefile'
    elif type == 'm4':
        fname = os.path.join( 'project', project.name_lower + '.m4' )
    else:
        raise ValueError, 'unknown file type: ' + type

    ftmp  = fname + '.tmp'

    pdir = os.path.dirname( fname )
    if pdir:
        if not os.path.exists( pdir ):
            os.makedirs( pdir )

    try:
        try:
            outf( 'generating %s', fname )
            file = open( ftmp, 'w' )
            config.output( file, type )
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
        errf( 'failed writing to %s\n%s', ftmp, x )

    try:
        os.rename( ftmp, fname )
    except Exception, x:
        errf( 'failed writing to %s\n%s', fname, x )

###############################################################################

if not options.launch:
    generate( 'make' )
    generate( 'm4' )
    sys.exit( 0 )

###############################################################################

if os.path.exists( options.launch_dir ):
    if not options.launch_force:
        errf( 'scratch directory already exists: %s', options.launch_dir )
else:
    outf( 'creating %s', options.launch_dir )
    os.makedirs( options.launch_dir )    

outf( 'chdir %s', options.launch_dir )
os.chdir( options.launch_dir )
generate( 'make' )
generate( 'm4' )

outf( 'opening %s', options.launch_log )
try:
    log = open( options.launch_log, 'w' )
except Exception, x:
    errf( 'open failure: %s', x )

cmd = '%s -j%d' % (makeTool.found,options.launch_jobs)
if options.launch_args:
    cmd += ' ' + options.launch_args

## record begin
timeBegin = time.time()
s = '###\n### TIME: %s\n### launch: %s\n###\n' % (time.asctime(),cmd)
stdout.write( s ); stdout.flush()
log.write( s ); log.flush()

## launch/pipe
try:
    pipe = subprocess.Popen( cmd, shell=True, bufsize=1, stdout=subprocess.PIPE, stderr=subprocess.STDOUT )
except Exception, x:
    errf( 'launch failure: %s', x )
for line in pipe.stdout:
    if not options.launch_quiet:
        stdout.write( line ); stdout.flush()
    log.write( line ); log.flush()
pipe.wait()

## record end
timeEnd = time.time()
elapsed = timeEnd - timeBegin
result = '%s (exit code %d)' % ('success' if pipe.returncode == 0 else 'failed',pipe.returncode)
s = '###\n### TIME: %s\n### finished: %.2f seconds\n### %s\n###\n' % (time.asctime(),elapsed,result)
stdout.write( s ); stdout.flush()
log.write( s ); log.flush()

log.close()
sys.exit( 0 )
