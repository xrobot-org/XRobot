"""Read thin package metadata and display interfaces declared in C++ source."""
import copy
import json
import re
import subprocess
from pathlib import Path

import yaml

from xrobot import __version__
from xrobot.constructor_model import enrich_interface
from xrobot.source_syntax import extract_interface

MANIFEST_PATTERN = re.compile(r'/\*\s*=== MODULE MANIFEST(?: V(\d+))? ===\s*(.*?)\s*=== END MANIFEST ===\s*\*/', re.S)
# Newest manifest format this version reads; a Module that declares a newer one
# needs a newer xrobot.
MANIFEST_VERSION = 2
MANIFEST_KEYS = ('module_description', 'description', 'depends', 'standalone')


class ModuleManifest:
    def __init__(self, manifest: dict, path: Path | None = None):
        self.manifest = manifest
        self.path = path

    @property
    def description(self):
        return self.manifest.get('module_description', self.manifest.get('description', ''))

    @property
    def depends(self):
        value = self.manifest.get('depends', [])
        if isinstance(value, str):
            return [value]
        if not isinstance(value, list):
            raise ValueError('%s: depends must be a list' % self.path)
        return value

    @property
    def standalone(self):
        return self.manifest.get('standalone', True) is not False

    def as_dict(self):
        return dict(self.manifest)


def manifest_from_text(text: str, path=None) -> ModuleManifest:
    matches = list(MANIFEST_PATTERN.finditer(text))
    if len(matches) > 1:
        raise ValueError('%s: multiple package manifests' % path)
    if matches and matches[0].group(1) and int(matches[0].group(1)) > MANIFEST_VERSION:
        raise ValueError('%s: MODULE MANIFEST V%s needs a newer xrobot; xrobot %s reads manifests up to V%d'
                         % (path, matches[0].group(1), __version__, MANIFEST_VERSION))
    data = yaml.safe_load(matches[0].group(2)) if matches else {}
    if data is None:
        data = {}
    if not isinstance(data, dict):
        raise ValueError('%s: package manifest must be a mapping' % path)
    unknown = [k for k in data if k not in MANIFEST_KEYS]
    if unknown:
        raise ValueError('%s: unsupported manifest key(s) %s; the manifest holds only %s (the '
                         'constructor in C++ is the interface)' % (path, ', '.join(map(str, unknown)),
                                                                   ', '.join(MANIFEST_KEYS)))
    return ModuleManifest(data, path)


def parse_manifest_from_header(header_path: Path) -> ModuleManifest:
    header_path = Path(header_path)
    return manifest_from_text(header_path.read_text(encoding='utf-8-sig'), header_path)


def parse_module_folder(folder: Path) -> ModuleManifest:
    folder = Path(folder).resolve()
    return parse_manifest_from_header(folder / (folder.name + '.hpp'))


def load_single_module(path: Path) -> ModuleManifest:
    path = Path(path).resolve()
    return parse_module_folder(path) if path.is_dir() else parse_manifest_from_header(path)


_INTERFACE_CACHE = {}


def source_interface(path: Path) -> dict:
    """Read a Module's constructor interface; cached per file content state."""
    path = Path(path)
    if path.is_dir():
        path = path / (path.name + '.hpp')
    stat = path.stat()
    key = (str(path.resolve()), stat.st_mtime_ns, stat.st_size)
    if key not in _INTERFACE_CACHE:
        try:
            # Same text as TypeIndex reads, so both share one parsed document.
            source = path.read_text(encoding='utf-8', errors='surrogateescape')
            _INTERFACE_CACHE[key] = enrich_interface(
                source,
                extract_interface(source, path.stem, source_name=str(path.resolve())),
            )
        except ValueError as error:
            raise ValueError('%s: %s' % (path, error)) from error
    return copy.deepcopy(_INTERFACE_CACHE[key])


def _locked_head(folder: Path):
    """HEAD commit of a module checkout; raises when git cannot read it."""
    result = subprocess.run(['git', '-C', str(folder), 'rev-parse', '--verify', 'HEAD'],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode:
        raise ValueError('cannot read the commit of %s: %s' % (folder, result.stderr.strip()))
    return result.stdout.strip()


def _module_record(identity: str, folder: Path) -> dict:
    header = folder / (folder.name + '.hpp')
    return {'id': identity, 'name': folder.name, 'path': folder, 'header': header,
            'manifest': parse_manifest_from_header(header)}


def locked_modules(directory: Path, lock_path: Path) -> list:
    """State of every xrobot.lock entry: ok, missing, mismatch or broken (with a reason)."""
    directory = Path(directory)
    lock = yaml.safe_load(Path(lock_path).read_text(encoding='utf-8')) or {}
    states = []
    for identity, record in (lock.get('modules') or {}).items():
        folder = (directory / identity).resolve()
        if directory.resolve() not in folder.parents:
            raise ValueError('Module path leaves directory: %s' % identity)
        commit = (record or {}).get('commit')
        state = {'id': identity, 'folder': folder, 'commit': commit, 'head': None,
                 'status': 'ok', 'reason': None}
        if not re.fullmatch(r'[0-9a-f]{40}', str(commit or '')):
            state.update(status='broken', reason='xrobot.lock has no commit for %s' % identity)
        elif not (folder / (folder.name + '.hpp')).is_file():
            state['status'] = 'missing'
        elif not (folder / '.git').exists():
            state.update(status='broken', reason='%s is not a git checkout; run xrobot setup --frozen'
                         % identity)
        else:
            try:
                state['head'] = _locked_head(folder)
            except ValueError as error:
                state.update(status='broken', reason=str(error))
            else:
                if state['head'] != commit:
                    state['status'] = 'mismatch'
        states.append(state)
    return states


def lock_error(state: dict) -> str:
    if state['status'] == 'broken':
        return state['reason']
    if state['status'] == 'missing':
        return '%s from xrobot.lock is not checked out; run xrobot setup --frozen' % state['id']
    return ('%s is checked out at %s but xrobot.lock pins %s. While developing a module, keep '
            'your changes uncommitted; when they are ready, push them to a branch of the module '
            'and run `xrobot setup --update %s`. To return to the locked sources run '
            '`xrobot setup --frozen`.' % (state['id'], state['head'][:12], state['commit'][:12],
                                            state['id']))


def discover_modules(directory: Path, lock_path) -> dict:
    """Return canonical IDs and local source paths of the locked Modules.

    The lock is the only source of truth: unlisted folders (stale caches,
    manual clones) are ignored and every locked folder must be checked out at
    its locked commit, so interfaces are read from the sources the build
    compiles.
    """
    lock_path = Path(lock_path)
    if not lock_path.is_file():
        raise ValueError('%s does not exist; run `xrobot setup` to resolve the Modules' % lock_path.name)
    result = {}
    problems = []
    for state in locked_modules(directory, lock_path):
        if state['status'] != 'ok':
            problems.append(lock_error(state))
            continue
        result[state['id']] = _module_record(state['id'], state['folder'])
    if problems:
        raise ValueError('\n'.join(problems))
    return result


def select_module(modules: dict, requested: str) -> dict:
    candidates = [value for key, value in modules.items() if (key.casefold() == requested.casefold() if '/' in requested else value['name'] == requested)]
    if len(candidates) != 1:
        if not candidates:
            raise ValueError('Module not found: %s' % requested)
        raise ValueError('Ambiguous Module %s; specify %s' % (requested, ', '.join(v['id'] for v in candidates)))
    return candidates[0]


def print_manifest(manifest, name=None):
    print(json.dumps(manifest.as_dict(), ensure_ascii=False, indent=2))
    if manifest.standalone and manifest.path:
        interface = source_interface(manifest.path)
        if interface['template'] is not None:
            print('template <%s>' % interface['template'])
        for declaration in interface['constructors']:
            print('%s:%s: %s' % (manifest.path, declaration['line'], declaration['declaration']))


