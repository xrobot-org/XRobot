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
and an ordered list of single-key named `args`; use `template_args` as an ordered
list. Put explicit dependency parameters first and defaulted value configuration
last. Keep instance order. New instance configuration writes source defaults
explicitly; existing user values are not overwritten. Brace initializers can be
represented as ordered YAML maps/lists without reflecting unspecified fields.
Keep complex expressions as C++ text, including string quotes. YAML `null` is an
unfilled binding; C++ `nullptr` is an explicit optional null. Remove old `@`
prefixes. Audit old manifest defaults before moving them into source declarations.

The historical generator emitted argument dictionaries by insertion order, not
by their key names. Audit real generated calls when migrating old configurations;
renaming dictionary keys does not establish the old constructor's meaning.

## BSP entry

Name the actual peripheral objects/references and register explicit view types
with `XR_REGISTER`. Call `XROBOT_MAIN()` only after normal BSP initialization.
Use a named `static auto&` for a resident borrowed ADC channel. A direct logical
object binding selects its registered view from the constructor's target type;
required reference and optional pointer consumers do not require duplicate
registrations. The entry does not normally return.
The generated entry is global `XRobotMain` with selected-view reference parameters.
`XROBOT_MAIN()` only binds the original BSP names; it no longer creates a `void*`
table. Regenerate existing application headers; required/optional dependencies,
registered pointer-variable storage and the compile-only CI probe keep their
previous semantics. Do not replace resident `static` objects with stack or heap
objects to imitate an optimizer benchmark.

Regenerate the original source/header pair together. CodeGenerator preserves
user code blocks: existing `XRobotMain(peripherals)` or custom container calls
inside them require an explicit one-time migration. It does not silently rewrite
arbitrary user statements. Pure LibXR applications remain independent of XRobot.
The `--hw-cntr` option and old container-generation path are removed, not kept as
an alternative assembly mode. Generic USB generation emits one CDC; an application
requiring composite USB owns that code in preserved BSP user blocks.

Resident hardware, Modules and long-lived configuration/backing storage use
function-local `static` objects. Check constructor temporaries, nested calls and
the actual RTOS allocation before changing task stacks. A compiler frame report
or linked firmware is not a measured high-water mark or timing acceptance result.

## Source packages and CI

Package selection and instance selection are separate. Use canonical owner/repo
identities in source requests; a short class/package name is only a convenience
when unambiguous. Do not include two independent global classes with the same
name in one build.

Use `same-or-dev` for cooperating feature branches and exact tags for independent
releases. A same-tag dependency never falls back to dev. A project lock records
exact commits. Root relative requests use the BSP Git branch; detached jobs pass
an explicit `--context-ref refs/heads/...` or `refs/tags/...`. Changing the root
context requires an explicit lock update. Do not claim an uncommitted local source
overlay is represented by an older Git SHA. Build candidate overlays explicitly and publish/pin the actual
commits before relying on a fresh remote checkout.

The generated `Modules/CMakeLists.txt` lists selected nested source repositories.
The matching LibXR CMake integration includes that file. No build/test command
moves behind the XRobot CLI. Existing Module CI remains Module-owned; meaningful
behavior and hardware validation remain separate from the compile-only constructor
probe. The probe uses typed expressions from `void*` only for leading dependencies;
value configuration still uses real source defaults. It is compiled as an OBJECT
target, never executed, linked as a runnable fake application or flashed. Existing
Module-owned tests continue to run through native CMake/CTest. New workflows do
not create release tags.
