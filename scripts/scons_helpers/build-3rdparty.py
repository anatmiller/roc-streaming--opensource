from __future__ import print_function

import sys
import os
import os.path
import re
import shutil
import glob
import fnmatch
import ssl
import tarfile
import fileinput
import subprocess
import multiprocessing

try:
    from shlex import quote
except:
    from pipes import quote

try:
    from urllib.request import urlopen
except ImportError:
    from urllib2 import urlopen

try:
    # workaround for SSL certificate error
    ssl._create_default_https_context = ssl._create_unverified_context
except:
    pass

printdir = os.path.abspath('.')
devnull = open(os.devnull, 'w+')

def mkpath(path):
    try:
        os.makedirs(path)
    except:
        pass

def rmpath(path):
    try:
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except:
        pass

def rm_emptydir(path):
    try:
        os.rmdir(path)
    except:
        pass

def fetch_vendored(url, path, log, vendordir):
    for subdir, _, _ in os.walk(vendordir):
        distfile = os.path.join(subdir, os.path.basename(path))
        if os.path.exists(distfile):
            print('[found vendored] %s' % os.path.basename(distfile))
            with open(distfile, 'rb') as rp:
                with open(path, 'wb') as wp:
                    wp.write(rp.read())
                    return
    raise

def fetch_urlopen(url, path, log, vendordir):
    rp = urlopen(url)
    with open(path, 'wb') as wp:
        wp.write(rp.read())

def fetch_tool(url, path, log, vendordir, tool, cmd):
    print('[trying %s] %s' % (tool, url))
    with open(log, 'a+') as fp:
        print('>>> %s' % cmd, file=fp)
    if os.system(cmd) != 0:
        raise

def fetch_wget(url, path, log, vendordir):
    fetch_tool(url, path, log, vendordir, 'wget', 'wget "%s" --quiet -O "%s"' % (url, path))

def fetch_curl(url, path, log, vendordir):
    fetch_tool(url, path, log, vendordir, 'curl', 'curl -Ls "%s" -o "%s"' % (url, path))

def download(url, name, log, vendordir):
    path_res = 'src/' + name
    path_tmp = 'tmp/' + name

    if os.path.exists(path_res):
        print('[found downloaded] %s' % name)
        return

    rmpath(path_res)
    rmpath(path_tmp)
    mkpath('tmp')

    error = None
    for fn in [fetch_vendored, fetch_urlopen, fetch_curl, fetch_wget]:
        try:
            fn(url, path_tmp, log, vendordir)
            shutil.move(path_tmp, path_res)
            rm_emptydir('tmp')
            return
        except Exception as e:
            if fn == fetch_vendored:
                print('[download] %s' % url)
            if fn == fetch_urlopen:
                error = e

    print("error: can't download '%s': %s" % (url, error), file=sys.stderr)
    exit(1)

def unpack(filename, dirname):
    dirname_res = 'src/' + dirname
    dirname_tmp = 'tmp/' + dirname

    if os.path.exists(dirname_res):
        print('[found unpacked] %s' % dirname)
        return

    print('[unpack] %s' % filename)

    rmpath(dirname_res)
    rmpath(dirname_tmp)
    mkpath('tmp')

    tar = tarfile.open('src/'+filename, 'r')
    tar.extractall('tmp')
    tar.close()

    shutil.move(dirname_tmp, dirname_res)
    rm_emptydir('tmp')

def which(tool):
    proc = subprocess.Popen(['which', tool], stdout=subprocess.PIPE, stderr=devnull)
    out = read_stdout(proc).strip()
    if out:
        return out

def try_execute(cmd):
    return subprocess.call(
        cmd, stdout=devnull, stderr=subprocess.STDOUT, shell=True) == 0

def execute(cmd, log, ignore_error=False, clear_env=False):
    print('[execute] %s' % cmd)

    with open(log, 'a+') as fp:
        print('>>> %s' % cmd, file=fp)

    env = None
    if clear_env:
        env = {'HOME': os.environ['HOME'], 'PATH': os.environ['PATH']}

    code = subprocess.call('%s >>%s 2>&1' % (cmd, log), shell=True, env=env)
    if code != 0:
        if ignore_error:
            with open(log, 'a+') as fp:
                print('command exited with status %s' % code, file=fp)
        else:
            exit(1)

def execute_make(log, cpu_count=None):
    if cpu_count is None:
        try:
            cpu_count = len(os.sched_getaffinity(0))
        except:
            pass

    if cpu_count is None:
        try:
            cpu_count = multiprocessing.cpu_count()
        except:
            pass

    cmd = ['make']
    if cpu_count:
        cmd += ['-j' + str(cpu_count)]

    execute(' '.join(cmd), log)

def execute_cmake(srcdir, variant, toolchain, env, log, args=None):
    if not args:
        args = []

    compiler = getvar(env, 'CC', toolchain, 'gcc')
    sysroot = getsysroot(toolchain, compiler)

    need_sysroot = bool(sysroot)
    need_tools = True

    # cross-compiling for yocto linux
    if 'OE_CMAKE_TOOLCHAIN_FILE' in os.environ:
        need_tools = False

    # cross-compiling for android
    if 'android' in toolchain:
        args += ['-DCMAKE_SYSTEM_NAME=Android']

        api = detect_android_api(compiler)
        abi = detect_android_abi(toolchain)

        toolchain_file = find_android_toolchain_file(compiler)

        if toolchain_file:
            need_sysroot = False
            need_tools = False
            args += [quote('-DCMAKE_TOOLCHAIN_FILE=%s' % toolchain_file)]
            if api:
                args += ['-DANDROID_NATIVE_API_LEVEL=%s' % api]
            if abi:
                args += ['-DANDROID_ABI=%s' % abi]
        else:
            sysroot = find_android_sysroot(compiler)
            need_sysroot = bool(sysroot)
            need_tools = True
            if api:
                args += ['-DCMAKE_SYSTEM_VERSION=%s' % api]
            if abi:
                args += ['-DCMAKE_ANDROID_ARCH_ABI=%s' % abi]

    if need_sysroot:
        args += [
            '-DCMAKE_FIND_ROOT_PATH=%s' % quote(sysroot),
            '-DCMAKE_SYSROOT=%s'        % quote(sysroot),
        ]

    if need_tools:
        if not 'android' in toolchain:
            args += [
                '-DCMAKE_C_COMPILER=%s' % quote(getpath(compiler)),
            ]
        args += [
            '-DCMAKE_LINKER=%s' % quote(getpath(getvar(env, 'CCLD', toolchain, 'gcc'))),
            '-DCMAKE_AR=%s'     % quote(getpath(getvar(env, 'AR', toolchain, 'ar'))),
            '-DCMAKE_RANLIB=%s' % quote(getpath(getvar(env, 'RANLIB', toolchain, 'ranlib'))),
        ]

    cc_flags = [
        '-fPIC', # -fPIC should be set explicitly in older cmake versions
        '-fvisibility=hidden', # hide private symbols
    ]

    if variant == 'debug':
        cc_flags += [
            '-ggdb', # -ggdb is required for sanitizer backtrace
        ]
        args += [
            '-DCMAKE_BUILD_TYPE=Debug',
            '-DCMAKE_C_FLAGS_DEBUG:STRING=%s' % quote(' '.join(cc_flags)),
        ]
    else:
        args += [
            '-DCMAKE_BUILD_TYPE=Release',
            '-DCMAKE_C_FLAGS_RELEASE:STRING=%s' % quote(' '.join(cc_flags)),
        ]

    args += [
        '-DCMAKE_POSITION_INDEPENDENT_CODE=ON',
    ]

    execute('cmake ' + srcdir + ' ' + ' '.join(args), log)

def install_tree(src, dst, match=None, ignore=None):
    print('[install] %s' % os.path.relpath(dst, printdir))

    def match_patterns(src, names):
        ignorenames = []
        for n in names:
            if os.path.isdir(os.path.join(src, n)):
                continue
            matched = False
            for m in match:
                if fnmatch.fnmatch(n, m):
                    matched = True
                    break
            if not matched:
                ignorenames.append(n)
        return set(ignorenames)

    if match:
        ignorefn = match_patterns
    elif ignore:
        ignorefn = shutil.ignore_patterns(*ignore)
    else:
        ignorefn = None

    mkpath(os.path.dirname(dst))
    rmpath(dst)
    shutil.copytree(src, dst, ignore=ignorefn)

def install_files(src, dst):
    print('[install] %s' % os.path.join(
        os.path.relpath(dst, printdir),
        os.path.basename(src)))

    for f in glob.glob(src):
        mkpath(dst)
        shutil.copy(f, dst)

def replace_files(path, from_, to):
    print('[patch] %s' % path)
    for line in fileinput.input(path, inplace=True):
        print(line.replace(from_, to), end='')

def replace_tree(dirpath, filepats, from_, to):
    def match(path):
        try:
            with open(path) as fp:
                for line in fp:
                    if from_ in line:
                        return True
        except:
            pass

    for filepat in filepats:
        for root, dirnames, filenames in os.walk(dirpath):
            for filename in fnmatch.filter(filenames, filepat):
                filepath = os.path.join(root, filename)
                if match(filepath):
                    replace_files(filepath, from_, to)

def try_patch(dirname, patchurl, patchname, logfile, vendordir):
    if not try_execute('patch --version'):
        return
    download(patchurl, patchname, logfile, vendordir)
    execute('patch -p1 -N -d %s -i %s' % (
        'src/' + dirname,
        '../' + patchname), logfile, ignore_error=True)

def touch(path):
    open(path, 'w').close()

def traverse_parents(path, search_file):
    while True:
        parent = os.path.dirname(path)
        if parent == path:
            break # root
        path = parent
        child_path = os.path.join(path, search_file)
        if os.path.exists(child_path):
            return child_path

def read_stdout(proc):
    out = proc.stdout.read()
    try:
        out = out.decode()
    except:
        pass
    try:
        out = str(out)
    except:
        pass
    return out

def getvar(env, var, toolchain, default):
    if var in env:
        return env[var]
    return '-'.join([s for s in [toolchain, default] if s])

def getpath(tool):
    if '/' in tool:
        return tool
    p = which(tool)
    if not p:
        return tool
    return p

def getsysroot(toolchain, compiler):
    if not toolchain:
        return ""

    if not compiler:
        compiler = '%s-gcc' % toolchain

    try:
        cmd = [compiler, '-print-sysroot']
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=devnull)
        sysroot = read_stdout(proc).strip()
        if os.path.isdir(sysroot):
            return sysroot
    except:
        pass

    return None

def find_android_toolchain_file(compiler):
    compiler_exe = which(compiler)
    if compiler_exe:
        return traverse_parents(compiler_exe, 'build/cmake/android.toolchain.cmake')

def find_android_sysroot(compiler):
    compiler_exe = which(compiler)
    if compiler_exe:
        return traverse_parents(compiler_exe, 'sysroot')

def detect_android_abi(toolchain):
    try:
        ta = toolchain.split('-')[0]
    except:
        return
    if ta == 'arm' or ta == 'armv7a':
        return 'armeabi-v7a'
    if ta == 'aarch64':
        return 'arm64-v8a'
    if ta == 'i686':
        return 'x86'
    if ta == 'x86_64':
        return 'x86_64'
    return ta

def detect_android_api(compiler):
    try:
        cmd = [compiler, '-dM', '-E', '-']
        proc = subprocess.Popen(cmd, stdin=devnull, stdout=subprocess.PIPE, stderr=devnull)
        for line in read_stdout(proc).splitlines():
            m = re.search(r'__ANDROID_API__\s+(\d+)', line)
            if m:
                return m.group(1)
    except:
        pass

def checkfamily(env, toolchain, family):
    if family == 'gcc':
        keys = ['GNU', 'gnu', 'gcc', 'g++']
    elif family == 'clang':
        keys = ['clang']

    def _checktool(toolchain, tool):
        if toolchain:
            tool = '%s-%s' % (toolchain, tool)
        try:
            proc = subprocess.Popen([tool, '-v'],
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            out = str(read_stdout(proc).strip())
            for k in keys:
                if k in out:
                    return True
        except:
            pass
        return False

    for var in ['CC', 'CCLD', 'CXX', 'CXXLD']:
        if var in env:
            if not _checktool('', env[var]):
                return False

    if not 'gnu' in toolchain:
        if 'CC' not in env:
            for tool in ['cc', 'gcc', 'clang']:
                if _checktool(toolchain, tool):
                    break
            else:
                return False

        if 'CCLD' not in env:
            for tool in ['ld', 'gcc', 'clang']:
                if _checktool(toolchain, tool):
                    break
            else:
                return False

        if 'CXX' not in env or 'CXXLD' not in env:
            for tool in ['g++', 'clang++']:
                if _checktool(toolchain, tool):
                    break
            else:
                return False

    return True

def makeflags(workdir, toolchain, env, deplist, cflags='', ldflags='', variant='', pthread=False):
    incdirs=[]
    libdirs=[]
    rpathdirs=[]

    for dep in deplist:
        incdirs += [os.path.join(workdir, dep, 'include')]
        libdirs += [os.path.join(workdir, dep, 'lib')]
        rpathdirs += [os.path.join(workdir, dep, 'rpath')]

    cflags = ([cflags] if cflags else []) + ['-I%s' % path for path in incdirs]
    ldflags = ['-L%s' % path for path in libdirs] + ([ldflags] if ldflags else [])

    is_android = 'android' in toolchain
    is_gnu = checkfamily(env, toolchain, 'gcc')
    is_clang = checkfamily(env, toolchain, 'clang')

    if variant == 'debug':
        if is_gnu or is_clang:
            cflags += ['-ggdb']
        else:
            cflags += ['-g']
    elif variant == 'release':
        cflags += ['-O2']

    if pthread and not is_android:
        if is_gnu or is_clang:
            cflags += ['-pthread']
        if is_gnu:
            ldflags += ['-pthread']
        else:
            ldflags += ['-lpthread']

    if is_gnu:
        ldflags += ['-Wl,-rpath-link=%s' % path for path in rpathdirs]

    return ' '.join([
        'CXXFLAGS=%s' % quote(' '.join(cflags)),
        'CFLAGS=%s'   % quote(' '.join(cflags)),
        'LDFLAGS=%s'  % quote(' '.join(ldflags)),
    ])

def makeenv(envlist):
    ret = []
    for e in envlist:
        ret.append(quote(e))
    return ' '.join(ret)

if len(sys.argv) < 7:
    print("error: usage: 3rdparty.py workdir vendordir toolchain variant package deplist [env]",
          file=sys.stderr)
    exit(1)

workdir = os.path.abspath(sys.argv[1])
vendordir = os.path.abspath(sys.argv[2])
toolchain = sys.argv[3]
variant = sys.argv[4]
fullname = sys.argv[5]
deplist = sys.argv[6].split(':')
envlist = sys.argv[7:]

env = dict()
for e in envlist:
    k, v = e.split('=', 1)
    env[k] = v

m = re.match('^(.*?)-([0-9][a-z0-9.-]+)$', fullname)
if not m:
    print("error: can't determine version of '%s'" % fullname, file=sys.stderr)
    exit(1)
name, ver = m.group(1), m.group(2)

build_dir = os.path.join(workdir, fullname)
lib_dir = os.path.join(build_dir, 'lib')
include_dir = os.path.join(build_dir, 'include')
rpath_dir = os.path.join(build_dir, 'rpath')

logfile = os.path.join(build_dir, 'build.log')

rmpath(os.path.join(build_dir, 'commit'))
mkpath(os.path.join(build_dir, 'src'))

os.chdir(os.path.join(build_dir))

if name == 'libuv':
    download('https://dist.libuv.org/dist/v%s/libuv-v%s.tar.gz' % (ver, ver),
             'libuv-v%s.tar.gz' % ver,
             logfile,
             vendordir)
    unpack('libuv-v%s.tar.gz' % ver,
            'libuv-v%s' % ver)
    os.chdir('src/libuv-v%s' % ver)
    replace_files('include/uv.h', '__attribute__((visibility("default")))', '')
    if 'android' in toolchain:
        mkpath('build')
        os.chdir('build')
        execute_cmake('..', variant, toolchain, env, logfile, args=[
            '-DLIBUV_BUILD_TESTS=OFF',
            '-DANDROID_PLATFORM=android-21',
            ])
        execute_make(logfile)
        shutil.copy('libuv_a.a', 'libuv.a')
        os.chdir('..')
        install_files('build/libuv.a', lib_dir)
    else:
        execute('./autogen.sh', logfile)
        execute('./configure --host=%s %s %s %s' % (
            toolchain,
            makeenv(envlist),
            makeflags(workdir, toolchain, env, [], cflags='-fvisibility=hidden'),
            ' '.join([
                '--with-pic',
                '--enable-static',
            ])), logfile)
        execute_make(logfile)
        install_files('.libs/libuv.a', lib_dir)
    install_tree('include', include_dir)
elif name == 'libunwind':
    download(
        'http://download.savannah.nongnu.org/releases/libunwind/libunwind-%s.tar.gz' % ver,
        'libunwind-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('libunwind-%s.tar.gz' % ver,
            'libunwind-%s' % ver)
    os.chdir('src/libunwind-%s' % ver)
    execute('./configure --host=%s %s %s %s' % (
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, deplist, cflags='-fcommon -fPIC', variant=variant),
        ' '.join([
            '--enable-static',
            '--disable-shared',
            '--disable-coredump',
            '--disable-ptrace',
            '--disable-setjmp',
            '--disable-minidebuginfo',
           ])), logfile)
    execute_make(logfile)
    install_files('include/*.h', include_dir)
    install_files('src/.libs/libunwind.a', lib_dir)
elif name == 'libatomic_ops':
    download(
        'https://github.com/ivmai/libatomic_ops/releases/download/v%s/libatomic_ops-%s.tar.gz' % (
            ver, ver),
        'libatomic_ops-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('libatomic_ops-%s.tar.gz' % ver,
            'libatomic_ops-%s' % ver)
    os.chdir('src/libatomic_ops-%s' % ver)
    execute('./configure --host=%s %s %s %s' % (
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, deplist, cflags='-fPIC', variant=variant),
        ' '.join([
            '--enable-static',
            '--disable-shared',
            '--disable-docs',
           ])), logfile)
    execute_make(logfile)
    install_tree('src', include_dir, match=['*.h'])
    install_files('src/.libs/libatomic_ops.a', lib_dir)
elif name == 'openfec':
    if variant == 'debug':
        dist = 'bin/Debug'
    else:
        dist = 'bin/Release'
    download(
      'https://github.com/roc-streaming/openfec/archive/v%s.tar.gz' % ver,
      'openfec_v%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('openfec_v%s.tar.gz' % ver,
            'openfec-%s' % ver)
    os.chdir('src/openfec-%s' % ver)
    mkpath('build')
    os.chdir('build')
    execute_cmake('..', variant, toolchain, env, logfile, args=[
        '-DBUILD_STATIC_LIBS=ON',
        '-DDEBUG:STRING=%s' % ('ON' if variant == 'debug' else 'OFF'),
        ])
    execute_make(logfile)
    os.chdir('..')
    install_tree('src', include_dir, match=['*.h'])
    install_files('%s/libopenfec.a' % dist, lib_dir)
elif name == 'speexdsp':
    if ver.split('.', 1) > ['1', '2'] and (
            not re.match('^1.2[a-z]', ver) or ver == '1.2rc3'):
        speex = 'speexdsp'
    else:
        speex = 'speex'
    download('http://downloads.xiph.org/releases/speex/%s-%s.tar.gz' % (speex, ver),
            '%s-%s.tar.gz' % (speex, ver),
            logfile,
            vendordir)
    unpack('%s-%s.tar.gz' % (speex, ver),
            '%s-%s' % (speex, ver))
    os.chdir('src/%s-%s' % (speex, ver))
    execute('./configure --host=%s %s %s %s' % (
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, deplist, cflags='-fPIC', variant=variant),
        ' '.join([
            '--enable-static',
            '--disable-shared',
            '--disable-examples',
           ])), logfile)
    execute_make(logfile)
    install_tree('include', include_dir)
    install_files('lib%s/.libs/libspeexdsp.a' % speex, lib_dir)
elif name == 'alsa':
    download(
      'ftp://ftp.alsa-project.org/pub/lib/alsa-lib-%s.tar.bz2' % ver,
        'alsa-lib-%s.tar.bz2' % ver,
        logfile,
        vendordir)
    unpack('alsa-lib-%s.tar.bz2' % ver,
            'alsa-lib-%s' % ver)
    os.chdir('src/alsa-lib-%s' % ver)
    execute('./configure --host=%s %s %s' % (
        toolchain,
        makeenv(envlist),
        ' '.join([
            '--enable-shared',
            '--disable-static',
            '--disable-python',
        ])), logfile)
    execute_make(logfile)
    install_tree('include/alsa',
            os.path.join(build_dir, 'include', 'alsa'),
            ignore=['alsa'])
    install_files('src/.libs/libasound.so', lib_dir)
    install_files('src/.libs/libasound.so.*', rpath_dir)
elif name == 'ltdl':
    download(
      'ftp://ftp.gnu.org/gnu/libtool/libtool-%s.tar.gz' % ver,
        'libtool-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('libtool-%s.tar.gz' % ver,
            'libtool-%s' % ver)
    os.chdir('src/libtool-%s' % ver)
    execute('./configure --host=%s %s %s' % (
        toolchain,
        makeenv(envlist),
        ' '.join([
            '--enable-shared',
            '--disable-static',
        ])), logfile)
    execute_make(logfile)
    install_files('libltdl/ltdl.h', include_dir)
    install_tree('libltdl/libltdl', os.path.join(build_dir, 'include', 'libltdl'))
    install_files('libltdl/.libs/libltdl.so', lib_dir)
    install_files('libltdl/.libs/libltdl.so.*', rpath_dir)
elif name == 'json-c':
    download(
      'https://github.com/json-c/json-c/archive/json-c-%s.tar.gz' % ver,
        'json-c-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('json-c-%s.tar.gz' % ver,
            'json-c-json-c-%s' % ver)
    os.chdir('src/json-c-json-c-%s' % ver)
    execute('%s --host=%s %s %s %s' % (
        ' '.join(filter(None, [
            # workaround for outdated config.sub
            'ac_cv_host=%s' % toolchain if toolchain else '',
            # disable rpl_malloc and rpl_realloc
            'ac_cv_func_malloc_0_nonnull=yes',
            'ac_cv_func_realloc_0_nonnull=yes',
            # configure
            './configure',
        ])),
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, [], cflags='-w -fPIC -fvisibility=hidden'),
        ' '.join([
            '--enable-static',
            '--disable-shared',
        ])), logfile)
    execute_make(logfile, cpu_count=0) # -j is buggy for json-c
    install_tree('.', include_dir, match=['*.h'])
    install_files('.libs/libjson-c.a', lib_dir)
elif name == 'sndfile':
    download(
      'http://www.mega-nerd.com/libsndfile/files/libsndfile-%s.tar.gz' % ver,
        'libsndfile-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('libsndfile-%s.tar.gz' % ver,
            'libsndfile-%s' % ver)
    os.chdir('src/libsndfile-%s' % ver)
    execute('%s --host=%s %s %s %s' % (
        ' '.join(filter(None, [
            # workaround for outdated config.sub
            'ac_cv_host=%s' % toolchain if toolchain else '',
            # configure
            './configure',
        ])),
        toolchain,
        makeenv(envlist),
        # explicitly enable -pthread because libtool doesn't add it on some platforms
        makeflags(workdir, toolchain, env, [], cflags='-fPIC -fvisibility=hidden', pthread=True),
        ' '.join([
            '--enable-static',
            '--disable-shared',
            '--disable-external-libs',
        ])), logfile)
    execute_make(logfile)
    install_files('src/sndfile.h', include_dir)
    install_files('src/.libs/libsndfile.a', lib_dir)
elif name == 'pulseaudio':
    download(
      'https://freedesktop.org/software/pulseaudio/releases/pulseaudio-%s.tar.gz' % ver,
        'pulseaudio-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('pulseaudio-%s.tar.gz' % ver,
            'pulseaudio-%s' % ver)
    pa_ver = tuple(map(int, ver.split('.')))
    if (8, 99, 1) <= pa_ver < (11, 99, 1):
        try_patch(
            'pulseaudio-%s' % ver,
            'https://bugs.freedesktop.org/attachment.cgi?id=136927',
            '0001-memfd-wrappers-only-define-memfd_create-if-not-alrea.patch',
            logfile,
            vendordir)
    if pa_ver < (12, 99, 1):
        replace_tree('src/pulseaudio-%s' % ver, ['*.h', '*.c'],
                      '#include <asoundlib.h>',
                      '#include <alsa/asoundlib.h>')
    os.chdir('src/pulseaudio-%s' % ver)
    if pa_ver < (14, 99, 1):
        # workaround for "missing acolocal-1.15" and "missing automake-1.15" errors
        # on some systems; since we're not modifying any autotools stuff, it's safe
        # to replace corresponding commands with "true" command
        if os.path.exists('Makefile.in'):
            replace_files('Makefile.in', '@ACLOCAL@', 'true')
            replace_files('Makefile.in', '@AUTOMAKE@', 'true')
        execute('./configure --host=%s %s %s %s %s' % (
            toolchain,
            makeenv(envlist),
            makeflags(workdir, toolchain, env, deplist, cflags='-w -fomit-frame-pointer -O2'),
            ' '.join([
                'LIBJSON_CFLAGS=" "',
                'LIBJSON_LIBS="-ljson-c"',
                'LIBSNDFILE_CFLAGS=" "',
                'LIBSNDFILE_LIBS="-lsndfile"',
            ]),
            ' '.join([
                '--enable-shared',
                '--disable-static',
                '--disable-tests',
                '--disable-manpages',
                '--disable-orc',
                '--disable-webrtc-aec',
                '--disable-openssl',
                '--disable-neon-opt',
                '--without-caps',
            ])), logfile)
        execute_make(logfile)
        install_files('config.h', include_dir)
        install_tree('src/pulse', os.path.join(build_dir, 'include', 'pulse'),
                     match=['*.h'])
        install_files('src/.libs/libpulse.so', lib_dir)
        install_files('src/.libs/libpulse.so.0', rpath_dir)
        install_files('src/.libs/libpulse-simple.so', lib_dir)
        install_files('src/.libs/libpulse-simple.so.0', rpath_dir)
        install_files('src/.libs/libpulsecommon-*.so', lib_dir)
        install_files('src/.libs/libpulsecommon-*.so', rpath_dir)
    else:
        mkpath('builddir')
        os.chdir('builddir')
        execute('%s %s meson .. %s' % (
            makeenv(envlist),
            makeflags(workdir, toolchain, env, deplist),
            ' '.join([
                '-Ddoxygen=false',
                '-Dman=false',
                '-Dgcov=false',
                '-Dtests=false',
                '-Ddatabase=simple',
                '-Dorc=disabled',
                '-Dwebrtc-aec=disabled',
                '-Dopenssl=disabled',
            ])), logfile)
        execute('ninja', logfile)
        execute('DESTDIR=../instdir ninja install', logfile)
        os.chdir('..')
        install_tree('instdir/usr/local/include/pulse',
                     os.path.join(build_dir, 'include', 'pulse'),
                     match=['*.h'])
        install_files('builddir/src/pulse/libpulse.so', lib_dir)
        install_files('builddir/src/pulse/libpulse.so.0', rpath_dir)
        install_files('builddir/src/pulse/libpulse-simple.so', lib_dir)
        install_files('builddir/src/pulse/libpulse-simple.so.0', rpath_dir)
        install_files('builddir/src/libpulsecommon-*.so', lib_dir)
        install_files('builddir/src/libpulsecommon-*.so', rpath_dir)
elif name == 'sox':
    download(
      'https://downloads.sourceforge.net/project/sox/sox/%s/sox-%s.tar.gz' % (ver, ver),
      'sox-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('sox-%s.tar.gz' % ver,
            'sox-%s' % ver)
    os.chdir('src/sox-%s' % ver)
    execute('./configure --host=%s %s %s %s' % (
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, deplist, cflags='-fvisibility=hidden', variant=variant),
        ' '.join([
            '--enable-static',
            '--disable-shared',
            '--disable-openmp',
            '--without-libltdl',
            '--without-magic',
            '--without-png',
            '--without-ladspa',
            '--without-mad',
            '--without-id3tag',
            '--without-lame',
            '--without-twolame',
            '--without-ao',
            '--without-opus',
            '--with-oggvorbis=no',
            '--with-opus=no',
            '--with-flac=no',
            '--with-amrwb=no',
            '--with-amrnb=no',
            '--with-wavpack=no',
            '--with-ao=no',
            '--with-pulseaudio=no',
            '--with-sndfile=no',
            '--with-mp3=no',
            '--with-gsm=no',
            '--with-lpc10=no',
        ])), logfile)
    execute_make(logfile)
    install_files('src/sox.h', include_dir)
    install_files('src/.libs/libsox.a', lib_dir)
elif name == 'gengetopt':
    download('ftp://ftp.gnu.org/gnu/gengetopt/gengetopt-%s.tar.gz' % ver,
             'gengetopt-%s.tar.gz' % ver,
             logfile,
             vendordir)
    unpack('gengetopt-%s.tar.gz' % ver,
            'gengetopt-%s' % ver)
    os.chdir('src/gengetopt-%s' % ver)
    execute('./configure', logfile, clear_env=True)
    execute_make(logfile, cpu_count=0) # -j is buggy for gengetopt
    install_files('src/gengetopt', os.path.join(build_dir, 'bin'))
elif name == 'ragel':
    download('https://www.colm.net/files/ragel/ragel-%s.tar.gz' % ver,
             'ragel-%s.tar.gz' % ver,
             logfile,
             vendordir)
    unpack('ragel-%s.tar.gz' % ver,
            'ragel-%s' % ver)
    os.chdir('src/ragel-%s' % ver)
    execute('./configure', logfile, clear_env=True)
    execute_make(logfile)
    install_files('ragel/ragel', os.path.join(build_dir, 'bin'))
elif name == 'cpputest':
    download(
        'https://github.com/cpputest/cpputest/releases/download/v%s/cpputest-%s.tar.gz' % (
            ver, ver),
        'cpputest-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('cpputest-%s.tar.gz' % ver,
            'cpputest-%s' % ver)
    os.chdir('src/cpputest-%s' % ver)
    execute('./configure --host=%s %s %s %s' % (
            toolchain,
            makeenv(envlist),
            # disable warnings, since CppUTest uses -Werror and may fail to
            # build on old GCC versions
            makeflags(workdir, toolchain, env, [], cflags='-w'),
            ' '.join([
                '--enable-static',
                # disable memory leak detection which is too hard to use properly
                '--disable-memory-leak-detection',
            ])), logfile)
    execute_make(logfile)
    install_tree('include', include_dir)
    install_files('lib/libCppUTest.a', lib_dir)
elif name == 'google-benchmark':
    download(
        'https://github.com/google/benchmark/archive/v%s.tar.gz' % ver,
        'benchmark_v%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('benchmark_v%s.tar.gz' % ver,
            'benchmark-%s' % ver)
    os.chdir('src/benchmark-%s' % ver)
    mkpath('build')
    os.chdir('build')
    execute_cmake('..', variant, toolchain, env, logfile, args=[
        '-DBENCHMARK_ENABLE_GTEST_TESTS=OFF',
        '-DCMAKE_CXX_FLAGS=-w',
        ])
    execute_make(logfile)
    os.chdir('..')
    install_tree('include', include_dir, match=['*.h'])
    install_files('build/src/libbenchmark.a', lib_dir)
elif name == 'openssl':
    archive = 'openssl-{}.tar.gz'.format(ver)
    dir = 'openssl-' + ver
    url = 'https://www.openssl.org/source/' + archive

    download(url, archive, logfile, vendordir)
    unpack(archive, dir)
    os.chdir('src/' + dir)
    # see https://github.com/openssl/openssl/blob/master/INSTALL.md#configuration-options
    # for options:
    execute('./Configure', logfile)
    execute_make(logfile)
    install_tree('include', include_dir)
    install_files('libssl.a', lib_dir)
    install_files('libssl.so', lib_dir)
    install_files('libssl.so.*', rpath_dir)
    install_files('libcrypto.a', lib_dir)
    install_files('libcrypto.so', lib_dir)
    install_files('libcrypto.so.*', rpath_dir)
# end of deps
else:
    print("error: unknown 3rdparty '%s'" % fullname, file=sys.stderr)
    exit(1)

touch(os.path.join(build_dir, 'commit'))
