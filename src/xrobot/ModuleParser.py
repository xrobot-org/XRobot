"""Read thin package metadata and display interfaces declared in C++ source."""
import argparse
import copy
import json
import re
import subprocess
from pathlib import Path
from typing import Optional
import yaml
from xrobot.SourceSyntax import extract_interface
from xrobot.ConstructorModel import enrich_interface

MANIFEST_PATTERN = re.compile(r'/\*\s*=== MODULE MANIFEST(?: V\d+)? ===\s*(.*?)\s*=== END MANIFEST ===\s*\*/', re.S)
INTERFACE_FIELDS = {'constructor_args', 'template_args', 'required_hardware'}


class ModuleManifest:
    def __init__(self, manifest: dict, path: Optional[Path] = None):
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
    data = yaml.safe_load(matches[0].group(1)) if matches else {}
    if data is None:
        data = {}
    if not isinstance(data, dict):
        raise ValueError('%s: package manifest must be a mapping' % path)
    # These historical fields are never used as a C++ interface or defaults.
    return ModuleManifest({k: v for k, v in data.items() if k not in INTERFACE_FIELDS}, path)


def parse_manifest_from_header(header_path: Path) -> ModuleManifest:
    header_path = Path(header_path)
    return manifest_from_text(header_path.read_text(encoding='utf-8-sig'), header_path)


def parse_module_folder(folder: Path) -> ModuleManifest:
    folder = Path(folder)
    return parse_manifest_from_header(folder / (folder.name + '.hpp'))


def load_single_module(path: Path) -> ModuleManifest:
    path = Path(path)
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
            source = path.read_text(encoding='utf-8-sig')
            _INTERFACE_CACHE[key] = enrich_interface(
                source,
                extract_interface(source, path.stem, source_name=str(path)),
            )
        except ValueError as error:
            raise ValueError('%s: %s' % (path, error)) from error
    return copy.deepcopy(_INTERFACE_CACHE[key])


def _folder_identity(folder: Path) -> str:
    if (folder / '.git').exists():
        result = subprocess.run(['git', '-C', str(folder), 'config', '--get', 'remote.origin.url'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=10)
        match = re.search(r'(?:github\.com[/:])([^/]+/[^/]+?)(?:\.git)?/?$', result.stdout.strip(), re.I)
        if match:
            return match.group(1)
    return 'local/' + folder.name


def _locked_head(folder: Path) -> Optional[str]:
    result = subprocess.run(['git', '-C', str(folder), 'rev-parse', '--verify', 'HEAD'],
                            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
    return result.stdout.strip() if result.returncode == 0 else None


def _module_record(identity: str, folder: Path) -> dict:
    header = folder / (folder.name + '.hpp')
    return {'id': identity, 'name': folder.name, 'path': folder, 'header': header,
            'manifest': parse_manifest_from_header(header)}


def discover_modules(directory: Path, lock_path=None) -> dict:
    """Return canonical IDs and local source paths, not instantiated objects.

    With a project lock the lock is the only source of truth: unlisted folders
    (stale caches, manual clones) are ignored and every locked folder must be
    checked out at its locked commit, so interfaces are read from the sources
    the build will compile.
    """
    directory = Path(directory)
    lock_path = Path(lock_path) if lock_path else directory.parent / 'xrobot.lock'
    result = {}
    if lock_path.exists():
        lock = yaml.safe_load(lock_path.read_text(encoding='utf-8')) or {}
        for identity, record in (lock.get('modules') or {}).items():
            folder = (directory / identity).resolve()
            if directory.resolve() not in folder.parents:
                raise ValueError('Module path leaves directory: %s' % identity)
            if not (folder / (folder.name + '.hpp')).is_file():
                raise ValueError('%s from xrobot.lock is not checked out; run xrobot_setup --frozen' % identity)
            commit = (record or {}).get('commit')
            head = _locked_head(folder)
            if commit and head and head != commit:
                raise ValueError(
                    '%s is checked out at %s but xrobot.lock pins %s; run xrobot_setup --frozen to '
                    'restore the locked sources, or xrobot_setup --update to adopt the new ones'
                    % (identity, head[:12], commit[:12]))
            result[identity] = _module_record(identity, folder)
        return result
    if not directory.exists():
        return result
    folders = sorted(set(directory.glob('*')) | set(directory.glob('*/*')))
    for folder in folders:
        if not folder.is_dir() or folder.name.startswith('.'):
            continue
        if not (folder / (folder.name + '.hpp')).is_file():
            continue
        if not (folder / '.git').exists() and folder.parent != directory:
            identity = folder.parent.name + '/' + folder.name
        else:
            identity = _folder_identity(folder)
        if identity in result:
            raise ValueError('Duplicate local package %s' % identity)
        result[identity] = _module_record(identity, folder)
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--path', '-p', required=True)
    try:
        print_manifest(load_single_module(Path(parser.parse_args().path)))
    except (OSError, ValueError, yaml.YAMLError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
