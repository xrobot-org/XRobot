"""Prepare source dependencies and a native CMake header/link fixture for CI.

No build, execution, tagging or hardware verification is performed here.
The checked Module owns tests/xrobot_compile.cpp when template instantiation
needs explicit parameters. This script is not a C++ interface solver.
"""
import argparse
import json
import shutil
from pathlib import Path
import yaml
from xrobot.InitModule import sync_modules_by_config
from xrobot.SourceManager import validate_id
from xrobot.GenerateMain import atomic_write


def prepare(directory, identity, repo, ref, context_ref=None, sources=None):
    validate_id(identity)
    root = Path(directory).resolve()
    root.mkdir(parents=True, exist_ok=True)
    modules = root / 'Modules'
    sources = sources or [
        'https://xrobot.work/xrobot-modules/index.yaml',
        'https://qdu-robomaster.github.io/qdu-future-modules/index.yaml',
    ]
    override = root / 'candidate-index.yaml'
    atomic_write(override, yaml.safe_dump({'packages': [{'id': identity, 'repo': repo, 'type': 'module'}]}, sort_keys=False))
    source_list = [{'url': str(override), 'priority': -1000}]
    source_list += [{'url': str(Path(s).resolve()) if not str(s).startswith(('https://', 'http://')) else s, 'priority': 0} for s in sources]
    atomic_write(modules / 'sources.yaml', yaml.safe_dump({'sources': source_list}, sort_keys=False))
    request = {'id': identity, 'ref': ref}
    if context_ref:
        request['context_ref'] = context_ref
    atomic_write(modules / 'modules.yaml', yaml.safe_dump({'modules': [request]}, sort_keys=False))
    lock = sync_modules_by_config(modules/'modules.yaml', modules/'sources.yaml', modules, root/'xrobot.lock', update=True)
    folder = modules / identity
    fixture = folder / 'tests/xrobot_compile.cpp'
    if fixture.exists():
        atomic_write(root/'module_check.cpp', fixture.read_text(encoding='utf-8-sig'))
    else:
        header = identity.rsplit('/', 1)[-1] + '.hpp'
        atomic_write(root/'module_check.cpp', '#include "%s"\nint main() { return 0; }\n' % header)
    # Native CMake consumes the exact same generated source list as a BSP.
    # Linux camera counters require the 64-bit print family; embedded BSPs
    # continue to choose their own print profile and are tested separately.
    cmake = '''cmake_minimum_required(VERSION 3.16)
project(XRobotModuleCheck LANGUAGES C CXX ASM)
if(NOT LIBXR_SOURCE_DIR)
  message(FATAL_ERROR "Set LIBXR_SOURCE_DIR to an existing LibXR checkout")
endif()
set(XROBOT_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Modules")
set(LIBXR_PRINT_INTEGER_ENABLE_64BIT ON CACHE BOOL "64-bit Linux diagnostic counters")
set(LIBXR_PRINT_FLOAT_ENABLE_DOUBLE ON CACHE BOOL "Linux diagnostic precision")
string(TOLOWER "${LIBXR_SYSTEM}" XR_CHECK_SYSTEM)
if(XR_CHECK_SYSTEM STREQUAL "webots")
  if(NOT WEBOTS_HOME AND DEFINED ENV{WEBOTS_HOME})
    set(WEBOTS_HOME "$ENV{WEBOTS_HOME}")
  endif()
  if(NOT EXISTS "${WEBOTS_HOME}/include/controller/cpp/webots/Robot.hpp")
    message(FATAL_ERROR "Set WEBOTS_HOME to the real Webots SDK")
  endif()
  set(LIBXR_DRIVER Webots)
  include_directories("${WEBOTS_HOME}/include/controller/c"
                      "${WEBOTS_HOME}/include/controller/cpp")
endif()
add_subdirectory("${LIBXR_SOURCE_DIR}" libxr)
if(XR_CHECK_SYSTEM STREQUAL "webots")
  target_link_libraries(xr PUBLIC
    "${WEBOTS_HOME}/lib/controller/libCppController.so"
    "${WEBOTS_HOME}/lib/controller/libController.so")
endif()
add_executable(module_check module_check.cpp)
target_link_libraries(module_check PRIVATE xr)
'''
    atomic_write(root/'CMakeLists.txt', cmake)
    print(json.dumps({'module': identity, 'commit': lock['modules'][identity]['commit'], 'sources': len(lock['modules']), 'directory': str(root)}, indent=2))
    return root


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module', required=True)
    parser.add_argument('--repo', required=True)
    parser.add_argument('--ref', required=True)
    parser.add_argument('--context-ref')
    parser.add_argument('--source', action='append')
    parser.add_argument('--directory', default='module-check')
    args = parser.parse_args()
    try:
        prepare(args.directory, args.module, args.repo, args.ref, args.context_ref, args.source)
    except (OSError, ValueError, yaml.YAMLError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
