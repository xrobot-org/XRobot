#!/usr/bin/env python3
# coding: utf-8
"""
xrobot_source_manager.py - XRobot multi-source module repository management/aggregation/lookup/maintenance tool

Supports both command-line and Python package usage.
"""

import argparse
import sys
import logging
from pathlib import Path
from typing import Optional, Union
import yaml
import requests

DEFAULT_SOURCES = Path("Modules/sources.yaml")
DEFAULT_INDEX = Path("Modules/index.yaml")

def extract_name_from_url(url: str) -> str:
    """
    Extract the module name from a repository URL (local or remote).
    """
    name = url.rstrip('/').split('/')[-1]
    if name.endswith('.git'):
        name = name[:-4]
    return name

def load_yaml(source: Union[str, Path]) -> dict:
    """
    Load YAML from a local file or http(s) URL.
    Returns a dict (empty if content is empty or not a dict).
    """
    src = str(source)
    try:
        if src.startswith("http://") or src.startswith("https://"):
            resp = requests.get(src, timeout=10)
            resp.raise_for_status()
            data = yaml.safe_load(resp.text)
        else:
            path = Path(src)
            data = yaml.safe_load(path.read_text(encoding="utf-8")) if path.exists() else {}
        if not isinstance(data, dict):
            return {}
        return data
    except Exception as e:
        logging.warning(f"[WARN] Failed to load yaml: {source} ({e})")
        return {}

def save_yaml(path: Union[str, Path], data: dict):
    """
    Save YAML data to a local file.
    """
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(yaml.dump(data, sort_keys=False, allow_unicode=True), encoding="utf-8")

class ModuleSource:
    """
    Single module source object.
    Handles one index.yaml (local or remote).
    """
    def __init__(self, url, public_key=None, priority=0):
        self.url = url
        self.public_key = public_key
        self.priority = priority
        self.namespace = None      # Required
        self.mirror_of = None      # Optional
        self.module_urls = []
        self.module_name_to_url = {}

    def load_index(self):
        """
        Load index.yaml (supports local and remote).
        """
        try:
            index_data = load_yaml(self.url)
        except Exception as e:
            logging.warning(f"[WARN] Failed to load index.yaml: {self.url} ({e})")
            return False

        ns = index_data.get("namespace")
        if not ns:
            logging.error(f"[ERROR] index.yaml is missing the namespace field: {self.url}")
            return False
        self.namespace = ns
        self.mirror_of = index_data.get('mirror_of')
        self.module_urls = index_data.get('modules', [])
        for url in self.module_urls:
            name = extract_name_from_url(url)
            self.module_name_to_url[name] = url
        return True

    def add_module_url(self, repo_url: str):
        """
        Add a repository URL to index.yaml, avoiding duplicates.
        """
        if repo_url not in self.module_urls:
            self.module_urls.append(repo_url)
            self.module_name_to_url[extract_name_from_url(repo_url)] = repo_url

    def save_index_yaml(self, path=None):
        """
        Save index.yaml to disk.
        """
        index_data = {"namespace": self.namespace}
        if self.mirror_of:
            index_data['mirror_of'] = self.mirror_of
        index_data['modules'] = list(self.module_urls)
        save_yaml(path or self.url, index_data)

    @staticmethod
    def create_index_yaml(path, namespace="your-namespace", mirror_of=None):
        """
        Create a template index.yaml file.
        """
        data = {"namespace": namespace, "modules": ["https://github.com/xrobot-org/BlinkLED.git"]}
        if mirror_of:
            data["mirror_of"] = mirror_of
        save_yaml(path, data)

def get_primary_namespace(src: 'ModuleSource') -> str:
    """
    Use the mirror_of field as the primary namespace if available, otherwise use the namespace field.
    """
    return src.mirror_of or src.namespace

class SourceManager:
    """
    Multi-source aggregator and manager. Can be imported as a package.
    """
    def __init__(self, sources_yaml=DEFAULT_SOURCES):
        self.sources = []
        self.module_map = {}            # {modid: repo URL}
        self.module_source_map = {}     # {modid: ModuleSource object}
        self.all_module_candidates = {} # {modid: [ (url, ModuleSource) ]}
        if Path(sources_yaml).exists():
            self.load_sources(sources_yaml)

    def load_sources(self, yaml_path: Union[Path, str]):
        """
        Load all sources from sources.yaml and merge their modules.
        """
        data = load_yaml(yaml_path)
        if not isinstance(data, dict):
            data = {}
        sources_list = data.get("sources", [])
        if not isinstance(sources_list, list):
            sources_list = []
        for src in sources_list:
            url = src["url"]
            public_key = src.get("public_key")
            priority = int(src.get("priority", 0))
            ms = ModuleSource(url, public_key, priority)
            ok = ms.load_index()
            if ok:
                self.sources.append(ms)
        self.sources.sort(key=lambda s: s.priority)
        seen = set()
        for src in self.sources:
            pns = get_primary_namespace(src)
            for name, repo_url in src.module_name_to_url.items():
                modid = f"{pns}/{name}"
                if modid not in self.all_module_candidates:
                    self.all_module_candidates[modid] = []
                self.all_module_candidates[modid].append((repo_url, src))
                if modid not in seen:
                    self.module_map[modid] = repo_url
                    self.module_source_map[modid] = src
                    seen.add(modid)

    def list_modules(self):
        """
        Return all unique module IDs (namespace/ModuleName).
        """
        return sorted(self.module_map.keys())

    def get_repo_url(self, modid: str) -> Optional[str]:
        """
        Return the repository URL for the given module.
        """
        return self.module_map.get(modid)

    def find_module(self, modid: str) -> list:
        """
        Return all candidates (for multi-source/mirror environments).
        """
        return self.all_module_candidates.get(modid, [])

    def add_source(self, url, public_key=None, priority=0, sources_yaml=DEFAULT_SOURCES):
        """
        Add a new source to sources.yaml, avoiding duplicates.
        """
        path = Path(sources_yaml)
        if path.exists():
            data = load_yaml(path)
        else:
            data = {"sources": []}
        if not isinstance(data, dict):
            data = {"sources": []}
        for s in data["sources"]:
            if s["url"] == url:
                logging.warning(f"[WARN] Source already exists: {url}")
                return
        entry = {"url": url}
        if public_key:
            entry["public_key"] = public_key
        if priority is not None:
            entry["priority"] = int(priority)
        data["sources"].append(entry)
        save_yaml(path, data)

    def create_sources_yaml(self, path=DEFAULT_SOURCES):
        """
        Create a template sources.yaml.
        """
        save_yaml(path, {'sources': [
            {
                "url": "https://xrobot-org.github.io/xrobot-modules/index.yaml",
                "priority": 0
            }
        ]})

    def save_sources_yaml(self, path=DEFAULT_SOURCES):
        """
        Save the current sources list.
        """
        srcs = []
        for s in self.sources:
            entry = {"url": s.url, "priority": s.priority}
            if s.public_key:
                entry["public_key"] = s.public_key
            srcs.append(entry)
        save_yaml(path, {'sources': srcs})

    def add_index_entry(self, index_yaml, repo_url):
        """
        Add a repository to the specified index.yaml, avoiding duplicates.
        """
        path = Path(index_yaml)
        if path.exists():
            data = load_yaml(path)
        else:
            data = {"namespace": "local", "modules": []}
        if not isinstance(data, dict):
            data = {"namespace": "local", "modules": []}
        if "namespace" not in data:
            data["namespace"] = "local"
        if "modules" not in data:
            data["modules"] = []
        if repo_url in data["modules"]:
            logging.warning(f"[WARN] Repository already exists: {repo_url}")
            return
        data["modules"].append(repo_url)
        save_yaml(path, data)

    def create_index_yaml(self, path=DEFAULT_INDEX, namespace="local", mirror_of="xrobot-org"):
        """
        Create a template index.yaml (defaults to 'local' mirroring 'xrobot-org').
        """
        ModuleSource.create_index_yaml(path, namespace=namespace, mirror_of=mirror_of)

    def save_index_yaml(self, module_source: ModuleSource, path=None):
        """
        Save the specified source's index.yaml.
        """
        module_source.save_index_yaml(path)

# ================= CLI ===================

def main():
    parser = argparse.ArgumentParser(
        description="XRobot multi-source module repository aggregator/lookup/maintenance tool (CLI and Python package supported)."
    )
    subparsers = parser.add_subparsers(dest="cmd", help="Sub-command help")

    subparsers.add_parser("list", help="List all unique modules and their source")
    get_parser = subparsers.add_parser("get", help="Get the repository URL and source of a module")
    get_parser.add_argument("modid", help="Module ID (namespace/ModuleName)")

    find_parser = subparsers.add_parser("find", help="Find all sources for the same module")
    find_parser.add_argument("modid", help="Module ID (namespace/ModuleName)")

    cs_parser = subparsers.add_parser("create-sources", help="Create a sources.yaml template")
    cs_parser.add_argument("--output", "-o", help="Output file", default=DEFAULT_SOURCES)

    as_parser = subparsers.add_parser("add-source", help="Add a module source to sources.yaml")
    as_parser.add_argument("url", help="index.yaml path or url")
    as_parser.add_argument("--public-key", help="Public key (optional)")
    as_parser.add_argument("--priority", type=int, help="Priority", default=0)
    as_parser.add_argument("--sources", help="sources.yaml path", default=DEFAULT_SOURCES)

    ci_parser = subparsers.add_parser("create-index", help="Create an index.yaml template")
    ci_parser.add_argument("--output", "-o", help="Output file", default=DEFAULT_INDEX)
    ci_parser.add_argument("--namespace", help="Namespace", default="local")
    ci_parser.add_argument("--mirror-of", help="Mirror of which source", default="xrobot-org")

    ai_parser = subparsers.add_parser("add-index", help="Add a repository to index.yaml")
    ai_parser.add_argument("repo_url", help="Git repository URL or local path")
    ai_parser.add_argument("--index", help="index.yaml path", required=True)

    subparsers.add_parser("verify", help="Signature verification (TODO)")

    args = parser.parse_args()

    if args.cmd == "list" or args.cmd is None:
        sm = SourceManager()
        mods = sm.list_modules()
        print("Available modules:")
        for modid in mods:
            src = sm.module_source_map[modid]
            mirror_str = (f"(mirror of: {src.mirror_of})" if src.mirror_of else "")
            nsinfo = f"(actual namespace: {src.namespace})"
            print(f"  {modid:<32} source: {src.url} {mirror_str} {nsinfo}")

    elif args.cmd == "get":
        sm = SourceManager()
        url = sm.get_repo_url(args.modid)
        if url:
            src = sm.module_source_map[args.modid]
            mirror_str = f"(mirror of: {src.mirror_of})" if src.mirror_of else ""
            nsinfo = f"(actual namespace: {src.namespace})"
            print(f"{args.modid}\n{url}\nSource: {src.url} {mirror_str} {nsinfo}")
        else:
            print(f"[ERROR] Module not found: {args.modid}")
            sys.exit(1)

    elif args.cmd == "find":
        sm = SourceManager()
        candidates = sm.find_module(args.modid)
        if not candidates:
            print(f"[ERROR] Module not found: {args.modid}")
            sys.exit(1)
        print(f"All sources for module {args.modid}:")
        for url, src in candidates:
            mirror_str = f"(mirror of: {src.mirror_of})" if src.mirror_of else ""
            nsinfo = f"(actual namespace: {src.namespace})"
            print(f"  {url} source: {src.url} {mirror_str} {nsinfo}")

    elif args.cmd == "create-sources":
        sm = SourceManager()
        sm.create_sources_yaml(args.output)
        print(f"[SUCCESS] Created template: {args.output}")

    elif args.cmd == "add-source":
        sm = SourceManager(args.sources)
        sm.add_source(args.url, args.public_key, args.priority, args.sources)
        print(f"[SUCCESS] Added source: {args.url} to {args.sources}")

    elif args.cmd == "create-index":
        ModuleSource.create_index_yaml(args.output, args.namespace, args.mirror_of)
        print(f"[SUCCESS] Created template: {args.output}, namespace: {args.namespace}")

    elif args.cmd == "add-index":
        sm = SourceManager()
        sm.add_index_entry(args.index, args.repo_url)
        print(f"[SUCCESS] Added repository: {args.repo_url} to {args.index}")

    elif args.cmd == "verify":
        print("Signature verification feature TODO: not implemented yet.")

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    main()
