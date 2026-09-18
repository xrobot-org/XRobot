# XRobot

XRobot manages reusable source modules and generates ordinary C++ applications on
[LibXR](https://github.com/xrobot-org/libxr). It is not a build frontend: after
source preparation, use the BSP's CMake, Docker or vendor build commands.

This source line uses **static assembly**. Existing dynamic-manager configurations
must be migrated together with their Module sources; see [MIGRATION.md](MIGRATION.md).
LibXR remains usable without XRobot.

## Files and responsibilities

| File | Meaning |
| --- | --- |
| `Modules/sources.yaml` | Federated Module/BSP catalogs |
| `Modules/modules.yaml` | Direct source packages and requested Git refs |
| `xrobot.lock` | Resolved repository URLs, logical refs and exact commits |
| `User/xrobot.yaml` | Ordered C++ instances and their explicit arguments |
| Module `.hpp` | C++ constructor/template declarations and thin package metadata |
| `User/xrobot_main.hpp` | Generated external-object bridge, instances and monitor loop |
| `CMakeLists.txt` | Actual build, linking and toolchain configuration |

A reusable Module does not commit an application lockfile. BSPs are cataloged for
discovery; they are not normal Module dependencies or a universal project template.

## Prepare an application

Install this Python package in the project's environment, then create or edit
`Modules/sources.yaml`:

```yaml
sources:
  - url: https://xrobot.work/xrobot-modules/index.yaml
    priority: 0
```

Select Module source versions that implement the same static-assembly interface:

```yaml
modules:
  - xrobot-org/BlinkLED@refs/heads/dev
```

Fetch sources and display the constructor before choosing arguments:

```sh
xrobot_init_mod --update
xrobot_mod_parser --path Modules/xrobot-org/BlinkLED
xrobot_add_mod xrobot-org/BlinkLED --instance
```

`xrobot_add_mod` assigns a persistent identifier such as `blinkled_0` (lowercase class name, an underscore, then a sequence number), displays
source signatures, and seeds the first supported constructor with its explicit
defaults. Required dependency bindings remain unfilled; no hardware is guessed.
Edit the resulting `User/xrobot.yaml`:

```yaml
modules:
  - module: xrobot-org/BlinkLED
    id: blinkled_0
    args:
      - led: LED_R
      - blink_cycle: 250
settings:
  monitor_sleep_ms: 1000
```

In the BSP scope that owns `LED_R`, include the generated header, finish normal
peripheral initialization, and enter the application:

```cpp
#include "xrobot_main.hpp"

// Inside the existing application entry, after LED_R has been constructed:
XR_REGISTER(LED_R, LibXR::GPIO);
XROBOT_MAIN();
```

Generate and build through native tools:

```sh
xrobot_gen_main --config User/xrobot.yaml --register-source User/app_main.cpp
cmake -S . -B build
cmake --build build
```

Use the BSP's normal toolchain/presets where required. `xrobot_setup` combines
source synchronization and generation only; it never invokes CMake or flashes a
board. Its `--config` selects an alternative instance configuration.

The snippet assumes a real BSP object named `LED_R`; XRobot does not create it or
infer an electrical pin. A first generation scans the original registration
source, so the generated header need not already exist.

## C++ arguments, instances and defaults

A Module is a plain global C++ class matching its primary header name. It may have
multiple instances, public constructor overloads, and template parameters. No
`Application` inheritance or hidden `(HardwareContainer&, ApplicationManager&)`
arguments are required.

Instances are constructed and monitored in YAML order. Referring to a later
instance is a normal C++ compilation error, not a request for Python topological
sorting. Constructors perform initialization; there is no extra framework
`Init()`/`Start()` phase, start barrier or automatic teardown protocol.

`args` is an ordered list of named constructor parameters. A new instance is
seeded from the first supported public constructor: dependencies without defaults
are written as `null` placeholders, while source defaults are written explicitly.
Structured brace/list defaults may be edited as YAML maps/lists; expressions that
cannot be represented safely stay as C++ text:

```yaml
args:
  - uart: null
  - optional_uart: nullptr
  - param:
      kp: 1.0f
      ki: 0.0f
  - period_ms: 10
```

`null` means “not filled in yet”; it is not C++ `nullptr`. Existing user values are
not overwritten when source defaults later change. Users may edit the named
parameter list to match another supported constructor. The generator matches the
agreed declaration subset by parameter names/order and available explicit type
information; arbitrary C++ conversions and final overload viability remain the
compiler's job. `@name`, `expr` wrappers and implicit string quoting are not part
of the new configuration format. Existing instance identifiers are not
renumbered on regeneration.

Source extraction is deliberately narrow: explicit global class declarations,
template parameter text, visible public constructors, parameter names/types and
default expressions. It does not expand macros, reflect arbitrary structures,
resolve inheritance, or evaluate general C++ expressions. Macro-generated or
conditionally changing Module interfaces must be made explicit. Normal
conditional implementation code remains allowed.

## External objects and monitoring

Register an existing object or named reference with explicit view types:

```cpp
static auto& adc0 = adc.GetChannel(0);
XR_REGISTER(adc0, LibXR::ADC);
XR_REGISTER(spi0, LibXR::SPI);
// Further initialization is allowed here.
XROBOT_MAIN();
```

The generated global `XRobotMain` takes explicit references to the selected
registered views. `XROBOT_MAIN()` only supplies the BSP object names; constructors,
configuration and the monitor loop remain ordinary C++ in `User/xrobot_main.hpp`.
There is no runtime `void*` table. C++ reference binding performs base-subobject
adjustment and preserves cv qualifications. A registered pointer variable is
passed by reference to its original storage (`T*&`), not as a copied pointee
address. BSP objects and pointer variables must outlive the application.
There is no runtime name table, type registry, lookup, ownership transfer or
count parameter.

Caller-local registered types and aliases retain their existing support through
only the necessary view template parameters. The explicit template arguments are
the registered views, not the unrestricted concrete provider types. Normal,
namespace-visible interfaces produce a non-template `XRobotMain`.

A unique registered logical name is lexically bound inside configuration
expressions. Qualified/member names, strings and comments are not rewritten.
For `XR_REGISTER(device, TypeA, TypeB)`, a direct argument named `device` selects
its view using the matched constructor parameter type. A `TypeB&` parameter uses
the `TypeB` view; a `TypeB*` parameter can take the address of that same registered
object. Explicit `nullptr` remains a null dependency. Complex expressions with
multiple possible views must provide enough explicit type information; the
generator never chooses an arbitrary first view.

Optional public non-static `void OnMonitor()` methods are detected by C++ and
called in instance order. Static, overloaded, non-void or default-parameter
methods do not satisfy that convention. The loop calls the monitor helper directly on the
actual instances; no virtual manager/list or extra monitor thread is created.
`XROBOT_MAIN()` does not normally return and runs the monitor/sleep loop in its
calling task. Module instances, their long-lived configuration storage and resident
BSP objects use function-local `static` storage, initialized in startup order rather
than before `main`. Stack sizing still needs to account for constructor temporaries
and nested calls; static objects do not prove a minimum safe task stack.

## Runtime cost and storage

The generator transmits only the registered views referenced by this application;
all BSP registration declarations still receive compile-time type checks. Existing
named dependencies are passed directly instead of being cached in another static
reference. Configuration temporaries and initializer-list backing storage retain
the lifetime extension required by the constructor contract.

This removes runtime assembly lookup and manager dispatch, not every driver virtual
call. Function-local static objects still occupy RAM and can require initialization
guards/destructor registration. A smaller stack frame does not by itself reduce a
fixed task-stack reservation or the firmware's total RAM budget. Measure the linked
firmware and actual runtime separately before claiming zero overhead or changing
stack sizes.

## Generated code and IDE support

The public entry remains global `XRobotMain`; no new namespace or file suffix is
required. Open `User/xrobot_main.hpp` to inspect its typed parameters, constructors
and local instances. The entry macro adds one definition-navigation step; a direct
`XRobotMain(...)` call exposes its parameter signature. Refactor YAML/registrations
and regenerate instead of relying on IDE rename to edit generated macro bodies.

The existing LibXR CMake integration exports `XROBOT_OPTIMIZED_BUILD` from the
actual Release/RelWithDebInfo/MinSizeRel configuration. Optimized Clang builds
request inlining only for this entry; GCC and Debug retain normal `inline`.
Without this CMake definition, the header uses `NDEBUG` as a conservative fallback.
An explicit Debug profile (`LIBXR_DEBUG_BUILD`) always disables forced inlining.
The attribute does not change runtime semantics or force driver devirtualization;
no global LTO, strict-vtable, visibility or WPD flags are added by the generator.
Use the actual BSP build's `compile_commands.json` for language-server settings.
Generation keeps the same file path and does not rewrite identical content.

## Package identity, refs and reproducibility

A package identity is `owner/repo`; independent forks may coexist in catalogs.
A short name is accepted only when unique. Ambiguity requires a full identity;
no arbitrary source ordering selects an implementation. Two selected packages
that define the same global Module class are rejected rather than automatically
renamed or wrapped in a namespace.

Thin metadata stays in the primary header:

```cpp
/* === MODULE MANIFEST V2 ===
module_description: Motor controller
depends:
  - id: QDU-Robomaster/Motor
    ref: same-or-dev
=== END MANIFEST === */
```

`same-or-dev` uses the dependency's same-named branch, otherwise `dev`. From a
tag, it requires the identical tag string and never falls back to a moving
branch. `same` requires a match in both contexts. Independent modules may specify
an exact date tag. Tag names are compared literally, not by extracting dates
from different names.

Root `same` / `same-or-dev` requests use the ordinary BSP checkout's current Git
branch. In detached CI, pass `--context-ref refs/heads/feature/new-api` to
`xrobot_init_mod` or `xrobot_setup`; tag context must use `refs/tags/...`. The lock
records this context and rejects reuse after it changes until an explicit update.
No extra BSP identity file is needed. A detached Module PR SHA can preserve its
logical dependency branch in the request:

```yaml
modules:
  - id: QDU-Robomaster/Gimbal
    ref: 0123456789abcdef0123456789abcdef01234567  # Replace with the actual PR SHA.
    context_ref: refs/heads/feature/new-api
```

Local repository/index paths are stored relative to the lock file, with forward
slashes instead of machine-specific drive or root paths. Network repository URLs
remain unchanged. Normal synchronization reuses an existing project lock. `--update` deliberately
resolves moving refs again; `--frozen` rejects changed requests; `--offline`
requires already-present commits and initialized submodules and does not contact
catalogs or remotes. Resolution conflicts, cycles, dirty worktrees, source
mismatches and missing same tags fail without discarding local changes.

## Catalogs, tests and releases

A BSP catalog is discovery for ordinary Git repositories: list the repository URL,
without a second platform/MCU/build/validation identity schema. Module catalogs
retain canonical IDs, source priorities and package metadata. A smaller numeric
priority wins; conflicting repositories at equal priority are rejected. Module
`community`, `verified`, and `official` labels describe maintenance or version-bound
validation, not a guarantee about every later version. Detailed evidence belongs
to the repository's tests, PRs and releases.

```sh
xrobot_src_man --sources Modules/sources.yaml list --type bsp
xrobot_src_man --sources Modules/sources.yaml search STM32
python -m unittest discover -s tests -v
```

C++ test cases require `CXX` (default `g++`); unavailable compilers are reported
as skipped, not passed. Module workflows compile a source-generated constructor
probe as a CMake OBJECT target and compile the actual Module sources. `void*`
dependency placeholders exist only in this non-executed probe. Existing behavior
tests remain separate. [MIGRATION.md](MIGRATION.md#source-packages-and-ci) describes
source preparation, and [RELEASE.md](RELEASE.md) records publication prerequisites. Compile success does
not produce a date tag or hardware certification. Core releases coordinate
LibXR, XRobot and CodeGenerator; normal Module tags remain independent.

Documentation: <https://xrobot.work/>. Project governance: [GOVERNANCE.md](GOVERNANCE.md).
