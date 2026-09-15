"""Create a plain C++ Module with thin package metadata and native CMake."""
import argparse
import re
from pathlib import Path
import yaml


def create_module(class_name, description='', constructor_args=None, template_args=None, depends=None, output_dir=Path('Modules'), includes=None):
    if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', class_name):
        raise ValueError('Module name must be a C++ identifier')
    folder = Path(output_dir) / class_name
    if folder.exists():
        raise ValueError('Refusing to overwrite existing module: %s' % folder)
    constructors = list(constructor_args or [])
    templates = list(template_args or [])
    for item in constructors + templates:
        if not isinstance(item, str) or not item.strip():
            raise ValueError('Constructor/template declarations must be C++ text')
    from xrobot.InitModule import request
    for dependency in depends or []:
        request(dependency, canonical=True)
    manifest = yaml.safe_dump({'module_description': description, 'depends': depends or []}, sort_keys=False, allow_unicode=True).rstrip()
    lines = ['#pragma once', '', '// clang-format off', '/* === MODULE MANIFEST V2 ===', manifest, '=== END MANIFEST === */', '// clang-format on', '']
    for header in includes or []:
        if '\n' in header or '"' in header:
            raise ValueError('Invalid include name')
        lines.append('#include %s' % (header if header.startswith('<') else '"%s"' % header))
    if templates:
        lines += ['', 'template <%s>' % ', '.join(templates)]
    lines += ['class %s {' % class_name, ' public:', '  %s(%s) {}' % (class_name, ', '.join(constructors)), '};', '']
    folder.mkdir(parents=True)
    (folder / (class_name + '.hpp')).write_text('\n'.join(lines), encoding='utf-8')
    (folder / 'CMakeLists.txt').write_text('''target_include_directories(xr PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
file(GLOB MODULE_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/*.cc"
     "${CMAKE_CURRENT_LIST_DIR}/*.cxx"
     "${CMAKE_CURRENT_LIST_DIR}/*.c")
target_sources(xr PRIVATE ${MODULE_SOURCES})
''', encoding='utf-8')
    (folder / 'README.md').write_text('''# %s

%s

The public constructor and template declarations in `%s.hpp` are the interface.
Use `xrobot_mod_parser --path .` to display them and `xrobot_add_mod` to add an
instance. Write ordered C++ arguments in the application configuration; defaults
remain in C++.

`void OnMonitor()` is optional. Construction and monitoring follow application
configuration order. The BSP owns external object lifetimes and native builds.

## Validation

The generated CI checks that the header builds with LibXR. It does not claim
instantiation, hardware validation or compatibility of a new implementation.
Add the module-specific constructor fixture and hardware results before release.
Dependencies remain in the header manifest; CMake flags/toolchains stay in CMake.
''' % (class_name, description, class_name), encoding='utf-8')
    # A native fixture is useful from CI and from a plain local CMake invocation.
    tests = folder / 'tests'
    tests.mkdir()
    (tests / 'xrobot_compile.cpp').write_text('#include "%s.hpp"\nint main() { return 0; }\n' % class_name, encoding='utf-8')
    (tests / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.16)
project(module_header_check LANGUAGES C CXX ASM)
if(NOT LIBXR_SOURCE_DIR)
  message(FATAL_ERROR "Set LIBXR_SOURCE_DIR to an existing LibXR checkout")
endif()
add_subdirectory("${LIBXR_SOURCE_DIR}" libxr)
include("${CMAKE_CURRENT_LIST_DIR}/../CMakeLists.txt")
add_executable(module_header_check xrobot_compile.cpp)
target_link_libraries(module_header_check PRIVATE xr)
''', encoding='utf-8')
    workflow = folder / '.github/workflows'
    workflow.mkdir(parents=True)
    (workflow / 'build.yml').write_text('''name: Native Module compile check
on: [push, pull_request]
permissions:
  contents: read
jobs:
  compile:
    uses: xrobot-org/XRobot/.github/workflows/module-build.yml@dev
    with:
      libxr_ref: ${{ vars.LIBXR_CANDIDATE_REF || 'dev' }}
      tooling_ref: ${{ vars.XROBOT_CANDIDATE_REF || 'dev' }}
''', encoding='utf-8')
    return folder


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('class_name')
    parser.add_argument('--desc', default='')
    parser.add_argument('--constructor', action='append', default=[], help='A C++ parameter declaration; repeat for each parameter')
    parser.add_argument('--template', action='append', default=[], help='A C++ template parameter declaration')
    parser.add_argument('--include', action='append', default=[])
    parser.add_argument('--depends', nargs='*', default=[])
    parser.add_argument('--out', default='Modules')
    args = parser.parse_args()
    try:
        path = create_module(args.class_name, args.desc, args.constructor, args.template, args.depends, Path(args.out), args.include)
        print('Created %s' % path)
    except (OSError, ValueError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
