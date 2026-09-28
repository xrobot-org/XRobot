"""Resolve Module sources to exact commits (xrobot.lock); native CMake owns the build.

``Modules/modules.yaml`` lists the requested Modules (and pins the XRobot tool
version with ``xrobot:``). ``xrobot.lock`` records the exact commit of every
Module in the dependency closure and is the only source of truth afterwards:

- without ``--update``, locked Modules never move; added or removed requests
  change only the affected entries (minimal change);
- ``--update <module>...`` re-resolves only the named Modules;
- ``--update`` without names re-resolves everything;
- ``--frozen`` restores exactly the lock and fails if requests changed;
- ``--release-ref`` refuses unreleased commits for the named target line.
"""
import os
import re
import subprocess
from pathlib import Path
from urllib.parse import urlparse
from urllib.request import url2pathname

import yaml

from xrobot.GenerateMain import atomic_write
from xrobot.ModuleParser import manifest_from_text
from xrobot.SourceManager import SourceManager, load_yaml, validate_id

MODULES_KEYS = ('modules', 'xrobot')
TOOL_REPOSITORIES = {
    'xrobot': 'https://github.com/xrobot-org/XRobot.git',
    'generator': 'https://github.com/xrobot-org/LibXR_CppCodeGenerator.git',
}


def git(path, *args, check=True):
    env = dict(os.environ, GIT_TERMINAL_PROMPT='0', GIT_OPTIONAL_LOCKS='0')
    command = ['git'] + (['-C', str(path)] if path else []) + list(args)
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8',
                            errors='replace', env=env, timeout=300)
    if check and result.returncode:
        raise ValueError('Git failed in %s: %s\n%s' % (path, ' '.join(args), result.stderr.strip()))
    return result.stdout.strip() if result.returncode == 0 else None


def local_locator(value):
    """Return a filesystem locator, leaving network Git URLs untouched."""
    if value.startswith('file://'):
        parsed = urlparse(value)
        if parsed.netloc not in ('', 'localhost'):
            raise ValueError('Use a network Git URL for a non-local file host')
        return url2pathname(parsed.path)
    if re.match(r'^[A-Za-z][A-Za-z0-9+.-]*://', value) or value.startswith('git@'):
        return None
    return value


def portable_locator(value, lock_directory):
    local = local_locator(value)
    if local is None:
        return value
    path = Path(local)
    if not path.is_absolute():
        path = Path(lock_directory) / path  # already relative to the lock
    try:
        return Path(os.path.relpath(path.resolve(), lock_directory)).as_posix()
    except ValueError as error:
        raise ValueError('Local source cannot be expressed relative to this lock; use a portable '
                         'repository URL') from error


def expanded_locator(value, lock_directory):
    local = local_locator(value)
    return value if local is None else str((lock_directory / local).resolve())


def repository_identity(value):
    """Normalize a repository URL for comparison (scheme, host case, .git, slashes)."""
    local = local_locator(value)
    if local is not None:
        return 'file:' + str(Path(local).resolve()).casefold()
    text = value.strip()
    match = re.match(r'^git@([^:]+):(.+)$', text)
    if match:
        host, path = match.groups()
    else:
        parsed = urlparse(text)
        host, path = parsed.hostname or '', parsed.path
    path = path.strip('/')
    if path.endswith('.git'):
        path = path[:-4]
    host = host.casefold()
    if host == 'github.com':
        path = path.casefold()
    return host + '/' + path


def same_repository(left, right):
    return repository_identity(left) == repository_identity(right)


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
    raise ValueError('A context/release ref must start with refs/heads/ or refs/tags/')


def read_modules_yaml(path):
    data = load_yaml(path) if Path(path).exists() else {}
    unknown = [key for key in data if key not in MODULES_KEYS]
    if unknown:
        raise ValueError('%s: unknown key(s) %s; allowed: %s' % (path, ', '.join(unknown), ', '.join(MODULES_KEYS)))
    if not isinstance(data.get('modules', []), list):
        raise ValueError('%s: modules must be a list of package requests' % path)
    pin = data.get('xrobot')
    if pin is not None and not _valid_pin(str(pin)):
        raise ValueError('%s: xrobot must be a release version (e.g. 1.0.0) or a 40-hex commit' % path)
    return [request(value, canonical=True) for value in data.get('modules') or []], \
        (str(pin) if pin is not None else None)


def _valid_pin(value):
    return bool(re.fullmatch(r'\d+\.\d+\.\d+(?:[.\-+][0-9A-Za-z.\-]+)?', value) or
                re.fullmatch(r'[0-9a-f]{40}', value))


class Resolver:
    def __init__(self, modules_dir, source_manager=None, offline=False, pinned=None):
        self.directory = Path(modules_dir).resolve()
        self.sources = source_manager
        self.offline = offline
        self.pinned = pinned or {}
        self.prepared = {}
        self.resolved = {}
        self.stack = []

    def fetch_url(self, identity, repo):
        """Fetch from a mirror source when one lists the package, else from ``repo``."""
        if self.sources is not None:
            for candidate, source in self.sources.all_module_candidates.get(identity, []):
                if source.mirror_of:
                    return candidate
        return repo

    def prepare(self, identity, repo):
        validate_id(identity)
        folder = self.directory / identity
        if self.directory not in folder.resolve().parents:
            raise ValueError('Source path leaves the project module directory: ' + identity)
        if identity in self.prepared:
            if not same_repository(self.prepared[identity]['repo'], repo):
                raise ValueError('Conflicting source repositories for ' + identity)
            return folder
        url = self.fetch_url(identity, repo)
        if folder.exists():
            if not (folder / '.git').exists():
                raise ValueError('Refusing to overwrite non-Git directory: %s' % folder)
            actual = git(folder, 'remote', 'get-url', 'origin')
            if not same_repository(actual, repo) and not same_repository(actual, url):
                raise ValueError('Source mismatch for %s: %s != %s' % (identity, actual, repo))
        elif self.offline:
            raise ValueError('Offline source missing: ' + identity)
        else:
            folder.parent.mkdir(parents=True, exist_ok=True)
            git(None, 'clone', '--', url, str(folder))
        if not self.offline:
            git(folder, 'fetch', '--prune', '--tags', 'origin')
        self.prepared[identity] = {'repo': repo, 'folder': folder}
        return folder

    def resolve_ref(self, folder, ref, parent_context=None):
        if ref in ('same', 'same-or-dev'):
            if not parent_context or parent_context[0] not in ('branch', 'tag'):
                raise ValueError('%s requires a BSP branch/tag context (run on a branch, or pass '
                                 '--context-ref refs/heads/<branch>)' % ref)
            kind, name = parent_context
            full = ('refs/remotes/origin/' if kind == 'branch' else 'refs/tags/') + name
            sha = git(folder, 'rev-parse', '--verify', full + '^{commit}', check=False)
            if sha:
                return sha, kind, name
            if kind == 'tag' or ref == 'same':
                raise ValueError('Required same %s is missing: %s' % (kind, name))
            ref = 'refs/heads/dev'
            if not git(folder, 'rev-parse', '--verify', 'refs/remotes/origin/dev^{commit}', check=False):
                raise ValueError('%s has no %s branch and no dev branch; request an explicit tag, '
                                 'commit or branch' % (folder.name, name))
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
        if identity in self.stack:
            raise ValueError('Package dependency cycle: ' + ' -> '.join(self.stack + [identity]))
        pinned = self.pinned.get(identity)
        if pinned is not None:
            folder = self.prepare(identity, expanded_locator(pinned['repo'], self.directory.parent))
            explicit = req['ref'] not in (None, 'same', 'same-or-dev')
            if explicit:
                sha, _, _ = self.resolve_ref(folder, req['ref'], parent_context)
                if sha != pinned['commit']:
                    raise ValueError('%s requires %s at %s, but xrobot.lock keeps %s; run '
                                     '`xrobot setup --update %s`' % (
                                         ' -> '.join(self.stack) or 'modules.yaml', identity, req['ref'],
                                         pinned['commit'][:12], identity))
            if identity in self.resolved:
                return
            self.resolved[identity] = dict(pinned, context=list(parent_context or ('commit', pinned['commit'])))
            sha, logical = pinned['commit'], self.resolved[identity]['context']
        else:
            folder = self.prepare(identity, package.get('canonical', package['repo']))
            sha, kind, name = self.resolve_ref(folder, req['ref'], parent_context)
            logical = list(context(req['context_ref'])) if req.get('context_ref') else [kind, name]
            if identity in self.resolved:
                previous = self.resolved[identity]
                if previous['commit'] != sha:
                    raise ValueError('Dependency conflict for %s: %s vs %s (%s)' % (
                        identity, previous['commit'][:12], sha[:12], ' -> '.join(self.stack)))
                return
            self.resolved[identity] = {'repo': package.get('canonical', package['repo']),
                                       'source': package['source'], 'requested': req['ref'],
                                       'resolved_ref': name, 'ref_kind': kind, 'context': logical,
                                       'commit': sha}
        self.stack.append(identity)
        try:
            header = identity.rsplit('/', 1)[-1] + '.hpp'
            text = git(folder, 'show', sha + ':' + header, check=False)
            if text is None:
                raise ValueError('%s@%s has no primary %s' % (identity, sha[:12], header))
            for dep in manifest_from_text(text, str(folder / header)).depends:
                self.visit(request(dep, canonical=True), tuple(logical))
        finally:
            self.stack.pop()

    def materialize(self):
        """Check out every resolved commit; never discard local work."""
        before = {}
        for identity, entry in self.resolved.items():
            folder = self.prepared[identity]['folder']
            head = git(folder, 'rev-parse', '--verify', 'HEAD', check=False)
            if git(folder, 'status', '--porcelain'):
                raise ValueError('%s has uncommitted changes; they are kept, but the lock cannot '
                                 'move it. Commit and push them, or discard them, first' % identity)
            if head and head != entry['commit'] and not _published(folder, head):
                raise ValueError(
                    '%s is at local commit %s that is not on any remote branch or tag. While '
                    'developing a module keep your changes uncommitted; when they are ready, push '
                    'them to a branch of the module and run `xrobot setup --update %s`'
                    % (identity, head[:12], identity))
            gitdir = Path(git(folder, 'rev-parse', '--absolute-git-dir'))
            before[identity] = (head, git(folder, 'symbolic-ref', '--quiet', '--short', 'HEAD', check=False),
                                (gitdir / 'index').exists())
        by_name = {}
        for identity in self.resolved:
            name = identity.rsplit('/', 1)[-1]
            if name in by_name and by_name[name] != identity:
                raise ValueError('Source packages %s and %s define the same global Module; choose one '
                                 'implementation' % (by_name[name], identity))
            by_name[name] = identity
        applied = []
        try:
            for identity, entry in self.resolved.items():
                folder = self.prepared[identity]['folder']
                if before[identity][0] == entry['commit']:
                    continue
                applied.append(identity)
                if git(folder, 'cat-file', '-t', entry['commit'], check=False) != 'commit':
                    if self.offline:
                        raise ValueError('Offline commit missing for ' + identity)
                    git(folder, 'fetch', 'origin', entry['commit'])
                git(folder, 'checkout', '--detach', entry['commit'])
                if (folder / '.gitmodules').exists():
                    args = ['submodule', 'update', '--init', '--recursive']
                    if self.offline:
                        args.append('--no-fetch')
                    git(folder, *args)
        except Exception:
            for identity in reversed(applied):
                head, branch, has_index = before[identity]
                if has_index and head:
                    git(self.prepared[identity]['folder'], 'checkout', branch or head, check=False)
            raise


def _published(folder, commit):
    remote = git(folder, 'branch', '-r', '--contains', commit, check=False)
    if remote:
        return True
    return bool(git(folder, 'tag', '--contains', commit, check=False))


def validate_locked_graph(resolver, roots):
    """Validate lock closure from the pinned headers, without resolving moving refs."""
    records = resolver.resolved
    visited, active = set(), []

    def visit(req):
        candidates = [key for key in records if (key.casefold() == req['id'].casefold() if '/' in req['id']
                                                 else key.rsplit('/', 1)[-1] == req['id'])]
        if len(candidates) != 1:
            raise ValueError('xrobot.lock is missing or ambiguous for %s; run `xrobot setup`' % req['id'])
        identity = candidates[0]
        row = records[identity]
        kind, name = row.get('ref_kind'), row.get('resolved_ref')
        selector = req.get('ref')
        if selector and selector not in ('same', 'same-or-dev'):
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
            visit(request(dep, canonical=True))
        active.pop()
        visited.add(identity)

    for root in roots:
        visit(root)
    if visited != set(records):
        extra = sorted(set(records) - visited)
        raise ValueError('xrobot.lock contains Modules outside the declared dependency closure: %s; '
                         'run `xrobot setup`' % ', '.join(extra))


def release_line(release_ref):
    """The Module/tool line a BSP target ref requires (G3): dev, or master for master/main/tags."""
    kind, name = context(release_ref)
    if kind == 'branch' and name == 'dev':
        return 'dev'
    if (kind == 'branch' and name in ('master', 'main')) or kind == 'tag':
        return 'master'
    return None


def _line_ref(folder, line):
    """``origin/master`` (or ``origin/main`` for a main-only repository) / ``origin/dev``."""
    names = [line] if line == 'dev' else ['master', 'main']
    for name in names:
        ref = 'refs/remotes/origin/' + name
        if git(folder, 'rev-parse', '--verify', ref + '^{commit}', check=False):
            return ref
    return None


def check_released(resolver, release_ref, offline=False):
    """Every locked commit must be on the Module line of the BSP target (G3)."""
    line = release_line(release_ref)
    if line is None:
        return
    problems = []
    for identity, row in sorted(resolver.resolved.items()):
        folder = resolver.prepared[identity]['folder']
        requested = row.get('requested')
        if row.get('ref_kind') == 'tag' and requested not in (None, 'same', 'same-or-dev'):
            continue  # an explicitly requested tag is released by definition
        if not offline:
            branches = ['dev'] if line == 'dev' else ['master', 'main']
            for branch in branches:
                git(folder, 'fetch', 'origin', '+refs/heads/%s:refs/remotes/origin/%s' % (branch, branch), check=False)
        target = _line_ref(folder, line)
        if target is None:
            if row.get('ref_kind') == 'commit':
                continue  # third-party Module without the line: explicit commit pins only
            problems.append('%s has no %s branch; request an explicit tag or commit' % (identity, line))
            continue
        contained = subprocess.run(['git', '-C', str(folder), 'merge-base', '--is-ancestor', row['commit'], target],
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
        if not contained:
            problems.append('%s: locked commit %s is not on %s (feature branch not merged, or merged by '
                            'squash/rebase); after merging the Module, run `xrobot setup --update %s '
                            '--context-ref refs/heads/%s`' % (identity, row['commit'][:12],
                                                             target.split('/')[-1], identity,
                                                             'dev' if line == 'dev' else 'master'))
    if problems:
        raise ValueError('\n'.join(problems))


def check_tool_pins(project, release_ref, offline=False, cache=None):
    """Tool pins follow the same released-line rule as Modules (G3)."""
    line = release_line(release_ref)
    if line is None:
        return
    pins = {}
    _, pins['xrobot'] = read_modules_yaml(project.modules_yaml)
    if project.libxr_config.is_file():
        data = yaml.safe_load(project.libxr_config.read_text(encoding='utf-8')) or {}
        if isinstance(data, dict) and data.get('generator') is not None:
            pins['generator'] = str(data['generator'])
    problems = []
    cache = Path(cache or os.environ.get('XROBOT_CACHE') or Path.home() / '.cache' / 'xrobot')
    for tool, pin in pins.items():
        if pin is None:
            problems.append('%s is not pinned; add `%s: <version>`' % (tool, tool))
            continue
        if not re.fullmatch(r'[0-9a-f]{40}', pin):
            continue  # a release version is released by definition
        if offline:
            problems.append('%s pin %s cannot be checked offline' % (tool, pin[:12]))
            continue
        repo = cache / (tool + '.git')
        if not repo.exists():
            git(None, 'clone', '--bare', '--filter=blob:none', TOOL_REPOSITORIES[tool], str(repo))
        git(repo, 'fetch', 'origin', '+refs/heads/*:refs/heads/*', check=False)
        branches = ['dev'] if line == 'dev' else ['master', 'main']
        ok = any(subprocess.run(['git', '-C', str(repo), 'merge-base', '--is-ancestor', pin, 'refs/heads/' + b],
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
                 for b in branches)
        if not ok:
            problems.append('%s pin %s is not on the tool\'s %s line; pin a release version or a '
                            'merged commit' % (tool, pin[:12], branches[0]))
    if problems:
        raise ValueError('\n'.join(problems))


def write_cmake(modules_dir, records):
    lines = ['# Generated by `xrobot setup` from xrobot.lock; do not edit or commit.', '']
    for identity, entry in sorted(records.items()):
        validate_id(identity)
        folder = Path(modules_dir) / identity
        if (folder / 'CMakeLists.txt').exists():
            lines.append('include("${CMAKE_CURRENT_LIST_DIR}/%s/CMakeLists.txt")' % identity)
        else:
            lines.append('target_include_directories(xr PUBLIC "${CMAKE_CURRENT_LIST_DIR}/%s")' % identity)
    atomic_write(Path(modules_dir) / 'CMakeLists.txt', '\n'.join(lines) + '\n')


def _load_lock(lock_path):
    lock = load_yaml(lock_path)
    entries = lock.get('modules')
    if lock.get('version') != 1 or not isinstance(entries, dict):
        raise ValueError('%s: unsupported lock format; regenerate it with `xrobot setup --update`' % lock_path)
    for identity, entry in entries.items():
        validate_id(identity)
        if not isinstance(entry, dict) or 'directory' in entry:
            raise ValueError('Legacy lock entry for %s; regenerate it with `xrobot setup --update`' % identity)
        if not re.fullmatch(r'[0-9a-f]{40}', str(entry.get('commit', ''))):
            raise ValueError('Invalid locked commit for ' + identity)
    return lock


def _root_context(project, roots, context_ref):
    needs_parent = any(req['ref'] in ('same', 'same-or-dev') and not req.get('context_ref') for req in roots)
    selected = context_ref
    if selected is None and needs_parent:
        selected = git(project.root, 'symbolic-ref', '--quiet', 'HEAD', check=False)
    return context(selected) if selected else None


def sync_modules(project, update=None, frozen=False, offline=False, context_ref=None, release_ref=None):
    """Resolve and check out Modules; return the lock mapping.

    ``update`` is None (no update), [] (update everything) or a list of Module ids.
    """
    if update is not None and (frozen or offline):
        raise ValueError('--update cannot be combined with --frozen or --offline')
    roots, _ = read_modules_yaml(project.modules_yaml)
    lock_dir = project.lock.resolve().parent
    lock = _load_lock(project.lock) if project.lock.is_file() else None
    if frozen or offline:
        if lock is None:
            raise ValueError('xrobot.lock does not exist; run `xrobot setup` once without --frozen')
        if lock.get('requests') != roots:
            changed = _changed_requests(lock.get('requests') or [], roots)
            raise ValueError('Modules/modules.yaml differs from xrobot.lock (%s); run `xrobot setup` to '
                             'update the lock' % ', '.join(changed))
    if lock is not None and update is None and lock.get('requests') == roots:
        manager = SourceManager(project.sources_yaml) if not offline and project.sources_yaml.is_file() else None
        resolver = Resolver(project.modules_dir, manager, offline=offline)
        for identity, entry in lock['modules'].items():
            resolver.prepare(identity, expanded_locator(entry['repo'], lock_dir))
            resolver.resolved[identity] = entry
        validate_locked_graph(resolver, roots)
        records = lock['modules']
    else:
        manager = SourceManager(project.sources_yaml)
        pinned = {}
        if lock is not None and update != []:
            named = set()
            for name in update or []:
                matches = [k for k in lock['modules'] if k.casefold() == name.casefold() or
                           k.rsplit('/', 1)[-1] == name]
                if len(matches) != 1:
                    raise ValueError('%s is not in xrobot.lock; `--update` takes Module ids from the lock' % name)
                named.add(matches[0])
            previous = {r['id']: r.get('ref') for r in lock.get('requests') or []}
            changed = {r['id'] for r in roots if r['id'] in previous and previous[r['id']] != r.get('ref')}
            pinned = {k: v for k, v in lock['modules'].items() if k not in named and k not in changed}
        resolver = Resolver(project.modules_dir, manager, offline, pinned)
        root_context = _root_context(project, roots, context_ref)
        for root in roots:
            parent = context(root['context_ref']) if root.get('context_ref') else root_context
            if root['ref'] in ('same', 'same-or-dev') and parent is None and root['id'] not in pinned:
                raise ValueError('same/same-or-dev requests need a BSP branch; pass --context-ref '
                                 'refs/heads/<branch> in a detached checkout')
            resolver.visit(root, parent)
        records = {}
        for identity, entry in sorted(resolver.resolved.items()):
            row = {k: v for k, v in entry.items() if k != 'context'}
            for field in ('repo', 'source'):
                row[field] = portable_locator(str(row[field]), lock_dir)
            records[identity] = row
    if release_ref:
        check_released(resolver, release_ref, offline)
    resolver.materialize()
    write_cmake(project.modules_dir, records)
    result = {'version': 1, 'requests': roots, 'modules': records}
    if lock is None or result != lock:
        if frozen:
            raise ValueError('internal: --frozen would change xrobot.lock')
        atomic_write(project.lock, yaml.safe_dump(result, sort_keys=False, allow_unicode=True))
    return result


def _changed_requests(before, after):
    old = {r['id']: r for r in before}
    new = {r['id']: r for r in after}
    changed = ['+' + i for i in new if i not in old] + ['-' + i for i in old if i not in new]
    changed += ['~' + i for i in new if i in old and old[i] != new[i]]
    return changed or ['order']
