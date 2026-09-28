"""Check a core release record against the candidate repositories; never build, tag or publish.

LibXR, XRobot and LibXR_CppCodeGenerator are versioned independently. The record
names each candidate's exact commit, the version of each Python package, the
xr-syntax version both packages pin, and the acceptance gates:

    {
      "xr-syntax": "0.2.0",
      "repositories": {
        "libxr":   {"commit": "<40-hex>"},
        "xrobot":  {"commit": "<40-hex>", "version": "1.0.0"},
        "codegen": {"commit": "<40-hex>", "version": "6.0.0"}
      },
      "checks": {"automatic": "pass", "backends": "pass", "packages": "pass",
                 "modules": "pass", "docs": "pass", "bsp": "pass"}
    }
"""
import argparse
import json
import re
import subprocess
from pathlib import Path

GATES = ('automatic', 'backends', 'packages', 'modules', 'docs', 'bsp')
REPOSITORIES = ('libxr', 'xrobot', 'codegen')
PACKAGES = ('xrobot', 'codegen')
VERSION = re.compile(r'\d+\.\d+\.\d+(?:(?:a|b|rc)\d+)?')
COMMIT = re.compile(r'[0-9a-f]{40}')


def project_table(repo):
    """Return the text of the [project] table of ``repo``/pyproject.toml."""
    text = (Path(repo) / 'pyproject.toml').read_text(encoding='utf-8-sig')
    match = re.search(r'^\[project\][ \t]*$(.*?)(?=^\[|\Z)', text, re.M | re.S)
    if not match:
        raise ValueError('Missing [project] in %s' % (Path(repo) / 'pyproject.toml'))
    return match.group(1)


def package_version(repo):
    match = re.search(r'''^version\s*=\s*["']([^"']+)["']\s*$''', project_table(repo), re.M)
    if not match:
        raise ValueError('Expected an explicit package version in %s' % repo)
    return match.group(1)


def package_dependencies(repo):
    match = re.search(r'^dependencies\s*=\s*\[(.*?)\]', project_table(repo), re.M | re.S)
    if not match:
        return []
    return re.findall(r'''["']([^"']+)["']''', match.group(1))


def normalized_name(requirement):
    name = re.match(r'\s*([A-Za-z0-9][A-Za-z0-9._-]*)', requirement)
    return re.sub(r'[-_.]+', '-', name.group(1)).lower() if name else ''


def xr_syntax_pin(repo):
    """The exact xr-syntax version ``repo`` pins, or None with the offending requirement."""
    requirements = [r for r in package_dependencies(repo) if normalized_name(r) == 'xr-syntax']
    if len(requirements) != 1:
        return None, 'no xr-syntax dependency' if not requirements else 'several xr-syntax dependencies'
    requirement = requirements[0]
    match = re.fullmatch(r'\s*xr[-_.]syntax\s*==\s*(%s)\s*' % VERSION.pattern, requirement, re.I)
    if not match:
        return None, requirement
    return match.group(1), requirement


def git(repo, *args):
    output = subprocess.run(['git', '-C', str(repo), *args], stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, encoding='utf-8', timeout=20)
    if output.returncode:
        raise ValueError(output.stderr.strip())
    return output.stdout.strip()


def check(repositories, record):
    """Return every mismatch between the candidate repositories and the record."""
    failures = []
    rows = record.get('repositories') or {}
    for name in REPOSITORIES:
        if name not in repositories:
            failures.append('%s repository path is missing' % name)
            continue
        path = repositories[name]
        row = rows.get(name) or {}
        commit = row.get('commit')
        if not isinstance(commit, str) or not COMMIT.fullmatch(commit):
            failures.append('%s: the record needs the full 40-hex commit' % name)
        if git(path, 'status', '--porcelain'):
            failures.append('%s is not a clean committed candidate' % name)
        head = git(path, 'rev-parse', 'HEAD')
        if commit != head:
            failures.append('%s: recorded commit %s is not HEAD %s' % (name, str(commit)[:12], head[:12]))
        if name not in PACKAGES:
            continue
        version = row.get('version')
        if not isinstance(version, str) or not VERSION.fullmatch(version):
            failures.append('%s: the record needs a release version such as 1.0.0 or 1.0.0rc1' % name)
        actual = package_version(path)
        if actual != version:
            failures.append('%s: pyproject.toml version %s differs from the recorded %s' % (name, actual, version))
    pinned = record.get('xr-syntax')
    if not isinstance(pinned, str) or not VERSION.fullmatch(pinned):
        failures.append('the record needs the pinned xr-syntax version')
    for name in PACKAGES:
        if name not in repositories:
            continue
        version, requirement = xr_syntax_pin(repositories[name])
        if version is None:
            failures.append('%s must pin xr-syntax exactly (xr-syntax==X); found %s' % (name, requirement))
        elif version != pinned:
            failures.append('%s pins xr-syntax==%s, the record says %s' % (name, version, pinned))
    for gate in GATES:
        if (record.get('checks') or {}).get(gate) != 'pass':
            failures.append('Candidate acceptance is missing: ' + gate)
    return failures


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--libxr', required=True)
    parser.add_argument('--xrobot', required=True)
    parser.add_argument('--codegen', required=True)
    parser.add_argument('--record', required=True)
    args = parser.parse_args(argv)
    try:
        record = json.loads(Path(args.record).read_text(encoding='utf-8-sig'))
        failures = check({name: Path(getattr(args, name)) for name in REPOSITORIES}, record)
        if failures:
            parser.exit(1, '\n'.join(failures) + '\n')
        print('Release candidates and the acceptance record agree; no tag or publication performed.')
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
