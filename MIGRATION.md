# Migrating to static assembly

This is a source-breaking transition. Migrate the tools, Module sources,
application configuration and BSP entry together. Historical release tags remain
usable with their original tool versions; the new generator has no dynamic-
manager compatibility mode.

## Module source

Remove the mandatory `Application` base, hidden `HardwareContainer` and
`ApplicationManager` parameters, container lookups and manager registration.
Required external objects become explicit reference parameters; optional objects
remain explicit nullable pointers with the module's existing checks. Module
outputs remain normal members/accessors, not runtime registrations.

Keep only package metadata such as description, `depends` and `standalone` in
the header manifest. Constructor/template declarations and defaults are read
from C++ source, not copied into a second schema. `OnMonitor()` is optional;
remove `override` when its old base is removed. Internal threads, timers,
callbacks and device behavior still belong to the Module.

## Application configuration

Replace `name`/`constructor_args` dictionaries with `module`, persistent `id`
and ordered `args`; use `template_args` as an ordered list. Keep the explicit
instance order. Remove old `@` prefixes and write C++ string quotes and aggregate
initializers explicitly. Do not copy default arguments from old manifests into
new source automatically.

The historical generator emitted argument dictionaries by insertion order, not
by their key names. Audit real generated calls when migrating old configurations;
renaming dictionary keys does not establish the old constructor's meaning.

## BSP entry

Name the actual peripheral objects/references and register explicit view types
with `XR_REGISTER`. Call `XROBOT_MAIN()` only after normal BSP initialization.
Use `auto&` for borrowed ADC channels. The entry does not normally return.

Regenerate the original source/header pair together. CodeGenerator preserves
user code blocks: existing `XRobotMain(peripherals)` or custom container calls
inside them require an explicit one-time migration. It does not silently rewrite
arbitrary user statements. Pure LibXR applications remain independent of XRobot;
their explicitly requested legacy container option is not a second XRobot mode.

Local Module objects now occupy the calling task's stack. Check generated stack
usage and the actual RTOS task allocation, then confirm high-water margin on the
board. A linked firmware alone is not a stack or timing acceptance result.

## Source packages and CI

Package selection and instance selection are separate. Use canonical owner/repo
identities in source requests; a short class/package name is only a convenience
when unambiguous. Do not include two independent global classes with the same
name in one build.

Use `same-or-dev` for cooperating feature branches and exact tags for independent
releases. A same-tag dependency never falls back to dev. A project lock records
exact commits; do not claim an uncommitted local source overlay is represented by
an older Git SHA. Build candidate overlays explicitly and publish/pin the actual
commits before relying on a fresh remote checkout.

The generated `Modules/CMakeLists.txt` lists selected nested source repositories.
The matching LibXR CMake integration includes that file. No build/test command
moves behind the XRobot CLI. See CI_MIGRATION.md for exact-PR source preparation
and the order in which new tool and Module branches become available.
