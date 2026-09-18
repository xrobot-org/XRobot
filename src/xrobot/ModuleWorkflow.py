"""Generate the native Module compile-check workflow, without running a build."""
import json
import shlex
import yaml


_FETCH_TOOLS = r"""set -euo pipefail
checkout_tool() {
  local repo="$1" directory="$2" selected="$3" available
  if [[ -n "$selected" ]]; then
    if [[ "$selected" != refs/heads/* && "$selected" != refs/tags/* &&
          ! "$selected" =~ ^[0-9a-fA-F]{40}$ ]]; then
      echo "Use a full refs/heads/, refs/tags/ or commit SHA for $directory" >&2
      return 1
    fi
  elif [[ "$XR_REF_KIND" == tag ]]; then
    selected="refs/tags/$XR_REF_NAME"
  else
    available=$(git ls-remote --heads "$repo" "refs/heads/$XR_REF_NAME")
    if [[ -n "$available" ]]; then
      selected="refs/heads/$XR_REF_NAME"
    else
      selected=refs/heads/dev
    fi
  fi
  git init "$directory"
  git -C "$directory" remote add origin "$repo"
  git -C "$directory" fetch --depth=1 origin "$selected"
  git -C "$directory" checkout --detach FETCH_HEAD
  git -C "$directory" submodule update --init --recursive --depth=1
}
checkout_tool https://github.com/xrobot-org/XRobot.git XRobot "$XROBOT_REF"
checkout_tool https://github.com/xrobot-org/libxr.git libxr "$LIBXR_REF"
"""


_PREPARE_SOURCES = r"""python3 - <<'PYCODE'
import json
import os
from pathlib import Path
import subprocess
import yaml
from xrobot.InitModule import sync_modules_by_config
from xrobot.GenerateMain import generate_compile_check
from xrobot.ModuleParser import load_single_module
from xrobot.SourceManager import validate_id

identity = validate_id(os.environ['XR_MODULE_ID'])
folder = Path('Modules') / identity

def git(*args):
    return subprocess.check_output(['git', '-C', str(folder), *args], text=True).strip()

def write_yaml(path, data):
    Path(path).write_text(yaml.safe_dump(data, sort_keys=False), encoding='utf-8')

# Pin the checked-out PR/branch commit. Only its logical dependency context moves.
context = ('refs/tags/' if os.environ['XR_REF_KIND'] == 'tag' else 'refs/heads/')
context += os.environ['XR_REF_NAME']
write_yaml('ci-index.yaml', {'modules': [{'id': identity, 'repo': git('remote', 'get-url', 'origin')}]})
write_yaml('Modules/sources.yaml', {'sources': [
    {'url': 'https://xrobot.work/xrobot-modules/index.yaml', 'priority': 0},
    {'url': 'https://qdu-robomaster.github.io/qdu-future-modules/index.yaml', 'priority': 0},
    {'url': '../ci-index.yaml', 'priority': -100},
]})
write_yaml('Modules/modules.yaml', {'modules': [{
    'id': identity, 'ref': git('rev-parse', 'HEAD'), 'context_ref': context,
}]})
sync_modules_by_config('Modules/modules.yaml', 'Modules/sources.yaml', 'Modules')
manifest = load_single_module(folder)
if manifest.standalone:
    generate_compile_check(identity, 'Modules', 'module_check.cpp',
                           json.loads(os.environ['XR_TEMPLATE_ARGS']))
else:
    # A library has no Module constructor. Its actual .cpp files still build in xr.
    Path('module_check.cpp').write_text('#include "' + folder.name + '.hpp"\n', encoding='utf-8')
PYCODE
"""


_NATIVE_CMAKE = r"""cat > CMakeLists.txt <<'CMAKE'
cmake_minimum_required(VERSION 3.16)
project(xrobot_module_compile LANGUAGES C CXX ASM)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(LIBXR_SYSTEM linux CACHE STRING "LibXR execution backend")
set(LIBXR_DRIVER linux CACHE STRING "LibXR driver backend")
set(LIBXR_PRINT_INTEGER_ENABLE_64BIT ON CACHE BOOL "")
set(LIBXR_PRINT_FLOAT_ENABLE_DOUBLE ON CACHE BOOL "")
set(LIBXR_PRINT_ENABLE_POINTER ON CACHE BOOL "")
set(XROBOT_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Modules")
include(CTest)
add_subdirectory(libxr)
set_property(TARGET xr PROPERTY CXX_STANDARD 20)
set_property(TARGET xr PROPERTY CXX_STANDARD_REQUIRED ON)
add_library(module_check OBJECT module_check.cpp)
target_link_libraries(module_check PRIVATE xr)
CMAKE
"""


class _WorkflowDumper(yaml.SafeDumper):
    pass


def _string(dumper, value):
    return dumper.represent_scalar('tag:yaml.org,2002:str', value,
                                  style='|' if '\n' in value else None)


_WorkflowDumper.add_representer(str, _string)


def module_workflow(template_args=None, image='ghcr.io/xrobot-org/docker-image-linux:main',
                    cmake_options=None, packages=None, test_steps=None):
    """Keep existing Module-owned tests separate from the non-executed probe."""
    tests = list(test_steps or [])
    packages = sorted(set(['libgpiod-dev'] + list(packages or [])))
    steps = [
        {'name': 'Checkout the exact Module revision', 'uses': 'actions/checkout@v4',
         'with': {'path': 'Modules/${{ github.repository }}',
                  'ref': '${{ github.event.pull_request.head.sha || github.sha }}',
                  'submodules': 'recursive'}},
        {'name': 'Checkout matching core tools', 'shell': 'bash', 'run': _FETCH_TOOLS},
        {'name': 'Install build dependencies', 'run':
         'apt-get update\napt-get install -y --no-install-recommends ' +
         ' '.join(shlex.quote(p) for p in packages) +
         '\npython3 -m pip install ./XRobot\n'},
        {'name': 'Resolve sources and generate the compile-only constructor',
         'run': _PREPARE_SOURCES},
        {'name': 'Create native CMake project', 'run': _NATIVE_CMAKE},
        {'name': 'Compile Module sources and constructor', 'run':
         'cmake -S . -B build -DBUILD_TESTING=' + ('ON' if tests else 'OFF') +
         ''.join(' ' + shlex.quote(v) for v in (cmake_options or [])) +
         '\ncmake --build build --parallel 2\n'},
    ] + tests
    workflow = {
        'name': 'XRobot Module Build Test',
        'on': {'push': None, 'pull_request': None,
               'schedule': [{'cron': '0 3 1 * *'}]},
        'permissions': {'contents': 'read'},
        'jobs': {'build': {
            'runs-on': 'ubuntu-latest',
            'container': {'image': image, 'options': '--user 0'},
            'env': {
                'XR_MODULE_ID': '${{ github.repository }}',
                'XR_MODULE_NAME': '${{ github.event.repository.name }}',
                'XR_REF_NAME': '${{ github.head_ref || github.ref_name }}',
                'XR_REF_KIND': '${{ github.ref_type }}',
                'XR_TEMPLATE_ARGS': json.dumps(list(template_args or [])),
                'XROBOT_REF': "${{ vars.XROBOT_REF || '' }}",
                'LIBXR_REF': "${{ vars.LIBXR_REF || '' }}",
                'PYTHONPATH': '${{ github.workspace }}/XRobot/src',
            },
            'steps': steps,
        }},
    }
    return yaml.dump(workflow, Dumper=_WorkflowDumper, sort_keys=False,
                     allow_unicode=True, width=110)
