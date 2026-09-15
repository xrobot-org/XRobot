"""Resolve Module sources and exact commits; native CMake still owns the build."""
import argparse
import os
import re
import subprocess
from pathlib import Path
import yaml
from xrobot.SourceManager import SourceManager, load_yaml, validate_id
from xrobot.ModuleParser import manifest_from_text
from xrobot.GenerateMain import atomic_write

CONFIG_TEMPLATE = 'modules:\n  - xrobot-org/BlinkLED\n'
DEFAULT_CONFIG = Path('Modules/modules.yaml')
DEFAULT_SOURCES = Path('Modules/sources.yaml')


def git(path, *args, check=True):
    env = dict(os.environ, GIT_TERMINAL_PROMPT='0', GIT_OPTIONAL_LOCKS='0')
    command = ['git'] + (['-C', str(path)] if path else []) + list(args)
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8', errors='replace', env=env, timeout=120)
    if check and result.returncode:
        raise ValueError('Git failed in %s: %s\n%s' % (path, ' '.join(args), result.stderr.strip()))
    return result.stdout.strip() if result.returncode == 0 else None


def parse_modid(value):
    identity, separator, ref = str(value).partition('@')
    validate_id(identity)
    return (*identity.split('/'), ref if separator else None)


def request(value, canonical=False):
    if isinstance(value, str):
        identity, separator, ref = value.partition('@')
        result = {'id': identity, 'ref': ref if separator else None}
    elif isinstance(value, dict) and set(value) <= {'id', 'ref', 'context_ref'}:
        result = {'id': value.get('id'), 'ref': value.get('ref')}
        if value.get('context_ref'):
            result['context_ref'] = str(value['context_ref'])
    else:
        raise ValueError('Dependency must be owner/repo@ref or {id, ref, context_ref}')
    if not isinstance(result['id'], str) or not result['id']:
        raise ValueError('Dependency is missing its package id')
    if canonical:
        validate_id(result['id'])
    if result['ref'] is not None:
        result['ref'] = str(result['ref'])
        if not result['ref'] or result['ref'].startswith('-'):
            raise ValueError('Invalid dependency ref')
    return result


def context(value):
    if value.startswith('refs/heads/'):
        return ('branch', value[len('refs/heads/'):])
    if value.startswith('refs/tags/'):
        return ('tag', value[len('refs/tags/'):])
    raise ValueError('context_ref must explicitly start with refs/heads/ or refs/tags/')


class Resolver:
    def __init__(self, modules_dir, source_manager=None, offline=False):
        self.directory = Path(modules_dir).resolve()
        self.sources = source_manager
        self.offline = offline
        self.prepared = {}
        self.resolved = {}
        self.stack = []

    def prepare(self, identity, repo):
        validate_id(identity)
        folder = self.directory / identity
        if self.directory not in folder.resolve().parents:
            raise ValueError('Source path leaves the project module directory: ' + identity)
        if identity in self.prepared:
            if self.prepared[identity]['repo'] != repo:
                raise ValueError('Conflicting source repositories for ' + identity)
            return folder
        if folder.exists():
            if not (folder / '.git').exists():
                raise ValueError('Refusing to overwrite non-Git directory: %s' % folder)
            actual = git(folder, 'remote', 'get-url', 'origin')
            if actual.rstrip('/') != repo.rstrip('/'):
                raise ValueError('Source mismatch for %s: %s != %s' % (identity, actual, repo))
        elif self.offline:
            raise ValueError('Offline source missing: ' + identity)
        else:
            folder.parent.mkdir(parents=True, exist_ok=True)
            git(None, 'clone', '--', repo, str(folder))
        if not self.offline:
            git(folder, 'fetch', '--prune', '--tags', 'origin')
        self.prepared[identity] = {'repo': repo, 'folder': folder}
        return folder

    def resolve_ref(self, folder, ref, parent_context=None):
        if ref in ('same', 'same-or-dev'):
            if not parent_context or parent_context[0] not in ('branch', 'tag'):
                raise ValueError('%s requires an explicit parent branch/tag context (use context_ref for commit checkouts)' % ref)
            kind, name = parent_context
            full = ('refs/remotes/origin/' if kind == 'branch' else 'refs/tags/') + name
            sha = git(folder, 'rev-parse', '--verify', full + '^{commit}', check=False)
            if sha:
                return sha, kind, name
            if kind == 'tag' or ref == 'same':
                raise ValueError('Required same %s is missing: %s' % (kind, name))
            ref = 'refs/heads/dev'
        if ref is None:
            symbolic = git(folder, 'symbolic-ref', '--quiet', 'refs/remotes/origin/HEAD', check=False)
            if not symbolic:
                raise ValueError('No remote default branch; select an explicit ref')
            ref = 'refs/heads/' + symbolic[len('refs/remotes/origin/'):]
        if ref.startswith('refs/heads/'):
            name = ref[len('refs/heads/'):]
            lookup, kind = 'refs/remotes/origin/' + name, 'branch'
        elif ref.startswith('refs/tags/'):
            name = ref[len('refs/tags/'):]
            lookup, kind = ref, 'tag'
        else:
            branch = git(folder, 'rev-parse', '--verify', 'refs/remotes/origin/' + ref + '^{commit}', check=False)
            tag = git(folder, 'rev-parse', '--verify', 'refs/tags/' + ref + '^{commit}', check=False)
            if branch and tag:
                raise ValueError('Ref exists as branch and tag; qualify refs/heads/ or refs/tags/: ' + ref)
            if branch or tag:
                return branch or tag, 'branch' if branch else 'tag', ref
            if not re.fullmatch(r'[0-9a-fA-F]{7,40}', ref):
                raise ValueError('Unknown dependency ref: ' + ref)
            lookup, kind, name = ref, 'commit', ref
        sha = git(folder, 'rev-parse', '--verify', lookup + '^{commit}', check=False)
        if not sha:
            raise ValueError('Dependency ref does not exist: ' + ref)
        return sha, kind, name

    def visit(self, req, parent_context=None):
        identity = self.sources.resolve_id(req['id'])
        package = self.sources.packages[identity]
        if package['type'] != 'module':
            raise ValueError('%s is a BSP catalog entry, not a Module dependency' % identity)
        folder = self.prepare(identity, package['repo'])
        sha, kind, name = self.resolve_ref(folder, req['ref'], parent_context)
        logical = context(req['context_ref']) if req.get('context_ref') else (kind, name)
        if identity in self.stack:
            raise ValueError('Package dependency cycle: ' + ' -> '.join(self.stack + [identity]))
        if identity in self.resolved:
            previous = self.resolved[identity]
            if previous['commit'] != sha or previous['context'] != list(logical):
                raise ValueError('Dependency conflict for %s: %s vs %s (%s)' % (identity, previous['commit'], sha, ' -> '.join(self.stack)))
            return
        self.resolved[identity] = {'repo': package['repo'], 'source': package['source'], 'requested': req['ref'], 'resolved_ref': name, 'ref_kind': kind, 'context': list(logical), 'commit': sha, 'directory': identity}
        self.stack.append(identity)
        try:
            header = identity.rsplit('/', 1)[-1] + '.hpp'
            text = git(folder, 'show', sha + ':' + header, check=False)
            if text is None:
                raise ValueError('%s@%s has no primary %s' % (identity, name, header))
            for dep in manifest_from_text(text, str(folder / header)).depends:
                self.visit(request(dep, canonical=True), logical)
        finally:
            self.stack.pop()

    def materialize(self):
        # Complete conflict resolution and clean-tree checks before any checkout.
        before = {}
        for identity, entry in self.resolved.items():
            folder = self.prepared[identity]['folder']
            head = git(folder, 'rev-parse', '--verify', 'HEAD', check=False)
            # Never discard tracked or untracked work, including after a failed resolve.
            gitdir = Path(git(folder, 'rev-parse', '--absolute-git-dir'))
            has_index = (gitdir / 'index').exists()
            if git(folder, 'status', '--porcelain'):
                raise ValueError('Local changes preserved; cannot synchronize ' + identity)
            before[identity] = (head, git(folder, 'symbolic-ref', '--quiet', '--short', 'HEAD', check=False), has_index)
        by_name = {}
        for identity in self.resolved:
            name = identity.rsplit('/', 1)[-1]
            if name in by_name and by_name[name] != identity:
                raise ValueError('Source packages %s and %s define the same global Module; choose one implementation' % (by_name[name], identity))
            by_name[name] = identity
        applied = []
        try:
            for identity, entry in self.resolved.items():
                folder = self.prepared[identity]['folder']
                applied.append(identity)
                git(folder, 'checkout', '--detach', entry['commit'])
                if (folder / '.gitmodules').exists():
                    args = ['submodule', 'update', '--init', '--recursive']
                    if self.offline:
                        state = git(folder, 'submodule', 'status', '--recursive')
                        if any(line.startswith('-') for line in state.splitlines()):
                            raise ValueError('Offline submodule missing in ' + identity)
                        args.append('--no-fetch')
                    git(folder, *args)
        except Exception:
            for identity in reversed(applied):
                head, branch, has_index = before[identity]
                if has_index and head:
                    git(self.prepared[identity]['folder'], 'checkout', branch or head, check=False)
            raise


def validate_locked_graph(resolver, roots):
    """Validate lock closure from the pinned headers, without resolving moving refs."""
    records = resolver.resolved
    visited, active = set(), []

    def visit(req, parent=None):
        candidates = [key for key in records if (key.casefold() == req['id'].casefold() if '/' in req['id'] else key.rsplit('/', 1)[-1] == req['id'])]
        if len(candidates) != 1:
            raise ValueError('Project lock is missing or ambiguous for ' + req['id'])
        identity = candidates[0]
        row = records[identity]
        kind, name = row.get('ref_kind'), row.get('resolved_ref')
        logical = row.get('context')
        expected_context = list(context(req['context_ref'])) if req.get('context_ref') else [kind, name]
        if logical != expected_context:
            raise ValueError('Invalid logical ref context in lock for ' + identity)
        selector = req.get('ref')
        if selector in ('same', 'same-or-dev'):
            if not parent or parent[0] not in ('branch', 'tag'):
                raise ValueError('Relative dependency has no logical context in lock')
            allowed = [list(parent)]
            if selector == 'same-or-dev' and parent[0] == 'branch':
                allowed.append(['branch', 'dev'])
            if [kind, name] not in allowed:
                raise ValueError('Locked relative ref violates manifest for ' + identity)
        elif selector:
            if selector.startswith('refs/heads/') or selector.startswith('refs/tags/'):
                if [kind, name] != list(context(selector)):
                    raise ValueError('Locked ref does not match requested ref for ' + identity)
            elif re.fullmatch(r'[0-9a-fA-F]{7,40}', selector) and kind == 'commit':
                if not row['commit'].startswith(selector.lower()):
                    raise ValueError('Locked commit does not match requested SHA for ' + identity)
            elif name != selector:
                raise ValueError('Locked ref does not match manifest for ' + identity)
        if identity in active:
            raise ValueError('Dependency cycle in project lock: ' + ' -> '.join(active + [identity]))
        if identity in visited:
            return
        active.append(identity)
        folder = resolver.prepared[identity]['folder']
        header = identity.rsplit('/', 1)[-1] + '.hpp'
        text = git(folder, 'show', row['commit'] + ':' + header, check=False)
        if text is None:
            raise ValueError('Primary header missing from locked commit for ' + identity)
        for dep in manifest_from_text(text, header).depends:
            visit(request(dep, canonical=True), logical)
        active.pop()
        visited.add(identity)

    for root in roots:
        visit(root)
    if visited != set(records):
        raise ValueError('Project lock contains sources outside the declared dependency closure')


def write_cmake(modules_dir, records):
    lines = ['# Generated source list; build with the BSP native CMake entry.', '']
    for identity, entry in sorted(records.items()):
        validate_id(identity)
        folder = Path(modules_dir) / entry['directory']
        if (folder / 'CMakeLists.txt').exists():
            lines.append('include("${CMAKE_CURRENT_LIST_DIR}/%s/CMakeLists.txt")' % entry['directory'])
        else:
            lines.append('target_include_directories(xr PUBLIC "${CMAKE_CURRENT_LIST_DIR}/%s")' % entry['directory'])
    atomic_write(Path(modules_dir) / 'CMakeLists.txt', '\n'.join(lines) + '\n')


def sync_modules_by_config(config_path, sources_path, modules_dir, lock_path=None, update=False, frozen=False, offline=False):
    modules_dir = Path(modules_dir)
    lock_path = Path(lock_path) if lock_path else modules_dir.resolve().parent / 'xrobot.lock'
    data = load_yaml(config_path)
    if not isinstance(data.get('modules', []), list):
        raise ValueError('modules.yaml requires a list of direct package requests')
    roots = [request(value) for value in data.get('modules', [])]
    if update and (frozen or offline):
        raise ValueError('--update cannot be combined with --frozen or --offline')
    if frozen or offline or (lock_path.exists() and not update):
        if not lock_path.is_file():
            raise ValueError('No project lock exists; resolve once before using frozen/offline mode')
        lock = load_yaml(lock_path)
        if lock.get('version') != 1 or lock.get('requests') != roots:
            raise ValueError('Module requests differ from xrobot.lock; run with --update')
        resolver = Resolver(modules_dir, offline=offline)
        entries = lock.get('modules')
        if not isinstance(entries, dict):
            raise ValueError('Invalid project lock module map')
        for identity, entry in entries.items():
            validate_id(identity)
            if entry.get('directory') != identity or not re.fullmatch(r'[0-9a-f]{40}', str(entry.get('commit', ''))):
                raise ValueError('Invalid locked source path/commit for ' + identity)
            folder = resolver.prepare(identity, entry['repo'])
            if git(folder, 'cat-file', '-t', entry['commit'], check=False) != 'commit':
                if offline:
                    raise ValueError('Offline commit missing for ' + identity)
                git(folder, 'fetch', 'origin', entry['commit'])
            resolver.resolved[identity] = entry
        validate_locked_graph(resolver, roots)
        resolver.materialize()
        write_cmake(modules_dir, entries)
        return lock
    manager = SourceManager(sources_path)
    resolver = Resolver(modules_dir, manager)
    for root in roots:
        resolver.visit(root)
    resolver.materialize()
    lock = {'version': 1, 'requests': roots, 'modules': dict(sorted(resolver.resolved.items()))}
    write_cmake(modules_dir, lock['modules'])
    atomic_write(lock_path, yaml.safe_dump(lock, sort_keys=False, allow_unicode=True))
    return lock


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-c', '--config', default=str(DEFAULT_CONFIG))
    parser.add_argument('-s', '--sources', default=str(DEFAULT_SOURCES))
    parser.add_argument('-d', '--directory', default='Modules')
    parser.add_argument('--lock')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--update', action='store_true')
    group.add_argument('--frozen', action='store_true')
    parser.add_argument('--offline', action='store_true')
    args = parser.parse_args()
    try:
        lock = sync_modules_by_config(args.config, args.sources, args.directory, args.lock, args.update, args.frozen, args.offline)
        print('Resolved %d exact Module commits' % len(lock['modules']))
    except (OSError, ValueError, yaml.YAMLError, subprocess.TimeoutExpired) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
