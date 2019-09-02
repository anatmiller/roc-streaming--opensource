import re

def ParseVersion(env, command):
    text = env.CommandOutput(command)
    if not text:
        return None

    m = re.search(r'(\b[0-9][0-9.]+\b)', text)
    if not m:
        return None

    return m.group(1)

def ParseGitHead(env):
    with open('.git/HEAD') as hf:
        head_ref = hf.read().strip().split()[-1]
        with open('.git/%s' % (head_ref)) as hrf:
            sha = hrf.read().strip()[:10]

    return sha

def ParseCompilerVersion(env, compiler):
    def getverstr():
        try:
            version_formats = [
                r'(\b[0-9]+\.[0-9]+\.[0-9]+\b)',
                r'(\b[0-9]+\.[0-9]+\b)',
            ]

            full_text = env.CommandOutput([compiler, '--version'])

            for regex in version_formats:
                m = re.search(r'(?:LLVM|clang)\s+version\s+'+regex, full_text)
                if m:
                    return m.group(1)

            trunc_text = re.sub(r'\([^)]+\)', '', full_text)

            dump_text = env.CommandOutput([compiler, '-dumpversion'])

            for text in [dump_text, trunc_text, full_text]:
                for regex in version_formats:
                    m = re.search(regex, text)
                    if m:
                        return m.group(1)

            return None
        except:
            pass

    ver = getverstr()
    if ver:
        return tuple(map(int, ver.split('.')))
    else:
        return None

def ParseCompilerTarget(env, compiler):
    text = env.CommandOutput([compiler, '-v', '-E', '-'])
    if not text:
        return None

    for line in text.splitlines():
        m = re.search(r'\bTarget:\s*(\S+)', line)
        if m:
            parts = m.group(1).split('-')
            # "system" defaults to "pc" on recent config.guess versions
            # use the same newer format for all compilers
            if len(parts) == 3:
                parts = [parts[0]] + ['pc'] + parts[1:]
            elif len(parts) == 4:
                if parts[1] == 'unknown':
                    parts[1] = 'pc'
            return '-'.join(parts)

    return None

def ParseCompilerDirectory(env, compiler):
    text = env.CommandOutput([compiler, '--version'])
    if not text:
        return None

    for line in text.splitlines():
        m = re.search(r'\bInstalledDir:\s*(.*)', line)
        if m:
            return m.group(1)

    return None

def ParsePkgConfig(env, cmd):
    if 'PKG_CONFIG' in env.Dictionary():
        pkg_config = env['PKG_CONFIG']
    elif env.Which('pkg-config'):
        pkg_config = 'pkg-config'
    else:
        return False

    try:
        env.ParseConfig('%s %s' % (pkg_config, cmd))
        return True
    except:
        return False

def ParseConfigGuess(env, cmd):
    text = env.CommandOutput([cmd])
    if not text:
        return None

    if not re.match(r'^\S+-\S+$', text):
        return None

    return text

def ParseList(env, s, all):
    if not s:
        return []
    ret = []
    for name in s.split(','):
        if name == 'all':
            for name in all:
                if not name in ret:
                    ret.append(name)
        else:
            if not name in ret:
                ret.append(name)
    return ret

def init(env):
    env.AddMethod(ParseVersion, 'ParseVersion')
    env.AddMethod(ParseGitHead, 'ParseGitHead')
    env.AddMethod(ParseCompilerVersion, 'ParseCompilerVersion')
    env.AddMethod(ParseCompilerTarget, 'ParseCompilerTarget')
    env.AddMethod(ParseCompilerDirectory, 'ParseCompilerDirectory')
    env.AddMethod(ParsePkgConfig, 'ParsePkgConfig')
    env.AddMethod(ParseConfigGuess, 'ParseConfigGuess')
    env.AddMethod(ParseList, 'ParseList')
