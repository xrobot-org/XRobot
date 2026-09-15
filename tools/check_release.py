"""Validate a coordinated core release record; never build, tag or publish."""
import argparse
import json
import re
import subprocess
from pathlib import Path

GATES = ('automatic', 'backends', 'packages', 'modules', 'docs', 'bsp')


def package_version(repo):
    text = (Path(repo)/'pyproject.toml').read_text(encoding='utf-8-sig')
    project = re.search(r'^\[project\]\s*$(.*?)(?=^\[|\Z)', text, re.M | re.S)
    if not project:
        raise ValueError('Missing [project] in %s' % repo)
    version = re.search(r'^version\s*=\s*[\"\']([^\"\']+)[\"\']\s*$', project.group(1), re.M)
    if not version:
        raise ValueError('Expected an explicit package version in %s' % repo)
    return version.group(1)


def git(repo, *args):
    output = subprocess.run(['git', '-C', str(repo), *args], stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, encoding='utf-8', timeout=20)
    if output.returncode:
        raise ValueError(output.stderr.strip())
    return output.stdout.strip()


def check(version, repositories, record):
    failures = []
    if not re.fullmatch(r'\d+\.\d+\.\d+(?:(?:a|b|rc)\d+)?', version):
        failures.append('Use one numeric release version, optionally with a numbered prerelease suffix')
    if record.get('version') != version:
        failures.append('Acceptance record version does not match')
    for name, path in repositories.items():
        if git(path, 'status', '--porcelain'):
            failures.append('%s is not a clean committed candidate' % name)
        head = git(path, 'rev-parse', 'HEAD')
        if record.get('commits', {}).get(name) != head:
            failures.append('%s acceptance does not match HEAD' % name)
        if name != 'libxr':
            actual = package_version(path)
            if actual != version:
                failures.append('%s package version %s differs from %s' % (name, actual, version))
    for gate in GATES:
        if record.get('checks', {}).get(gate) != 'pass':
            failures.append('Candidate acceptance is missing: ' + gate)
    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', required=True)
    parser.add_argument('--libxr', required=True)
    parser.add_argument('--xrobot', required=True)
    parser.add_argument('--codegen', required=True)
    parser.add_argument('--record', required=True)
    args = parser.parse_args()
    try:
        record = json.loads(Path(args.record).read_text(encoding='utf-8-sig'))
        failures = check(args.version, {'libxr': Path(args.libxr), 'xrobot': Path(args.xrobot), 'codegen': Path(args.codegen)}, record)
        if failures:
            parser.exit(1, '\n'.join(failures)+'\n')
        print('Coordinated candidate metadata and recorded acceptance agree; no tag or publication performed.')
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        parser.exit(1, str(error)+'\n')


if __name__ == '__main__':
    main()
