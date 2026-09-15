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

`xrobot_add_mod` assigns a persistent identifier such as `blinkled0`, displays
source signatures, and does not guess hardware objects, overloads or defaults.
Edit the resulting `User/xrobot.yaml`:

```yaml
modules:
  - module: xrobot-org/BlinkLED
    id: blinkled0
    args: [LED_R, 250]
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

Arguments are ordered C++ text. YAML quotation protects that text; it does not
add C++ string quotes:

```yaml
args:
  - '"sensor"'
  - 'Config{1, 2, 3}'
  - '&motor0'
  - 'flash0.GetDatabase()'
  - 'nullptr'
  - '(left, right)'
```

Defaults stay in the C++ declaration. Omit only trailing arguments; no named
argument dictionary, middle-argument hole, `@name`, `expr` wrapper, implicit
string quoting or YAML-aggregate conversion is supported. Template arguments may
be type names or ordinary C++ template argument text. Empty argument lists may
be omitted. Existing instance identifiers are not renumbered on regeneration.

Source extraction is deliberately narrow: explicit global class declarations,
template parameter text and visible public constructors. It does not expand
macros, resolve typedefs/inheritance, select overloads, or evaluate expressions.
Macro-generated or conditionally changing Module interfaces must be made explicit.
Normal conditional implementation code remains allowed. Final C++ semantics
belong to the compiler.

## External objects and monitoring

Register an existing object or named reference with explicit view types:

```cpp
auto& adc0 = adc.GetChannel(0);
XR_REGISTER(adc0, LibXR::ADC);
XR_REGISTER(spi0, LibXR::SPI);
// Further initialization is allowed here.
XROBOT_MAIN();
```

The generated bridge uses fixed `void*` slots. It converts each typed address
before erasure, including base-subobject adjustment, and restores the original
view's cv qualifications. A registered pointer variable is transported by its
storage address and restored as a reference to that pointer, not a copied
pointee address. BSP objects and pointer variables must outlive the application.
There is no runtime name table, type registry, lookup, ownership transfer or
count parameter.

A unique registered logical name is lexically bound inside configuration
expressions. Qualified/member names, strings and comments are not rewritten.
For `XR_REGISTER(device, TypeA, TypeB)`, the short `device` is ambiguous: the
parser lists the explicit fields `xr_view_device_0` and `xr_view_device_1`.
Select a field rather than asking Python to infer a constructor's type.

Optional public non-static `void OnMonitor()` methods are detected by C++ and
called in instance order. Static, overloaded, non-void or default-parameter
methods do not satisfy that convention. A local monitor callable refers to the
actual instances; no virtual manager/list or extra monitor thread is created.
`XROBOT_MAIN()` does not normally return and runs the monitor/sleep loop in its
calling task. BSP stack sizing must include its ordinary local Module objects.

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

For a detached PR SHA, preserve its logical branch explicitly in the request:

```yaml
modules:
  - id: QDU-Robomaster/Gimbal
    ref: 0123456789abcdef0123456789abcdef01234567  # Replace with the actual PR SHA.
    context_ref: refs/heads/feature/new-api
```

Normal synchronization reuses an existing project lock. `--update` deliberately
resolves moving refs again; `--frozen` rejects changed requests; `--offline`
requires already-present commits and initialized submodules and does not contact
catalogs or remotes. Resolution conflicts, cycles, dirty worktrees, source
mismatches and missing same tags fail without discarding local changes.

## Catalogs, tests and releases

Catalogs accept existing URL lists and typed `packages`/`bsps` entries. A BSP entry
may describe `id`, `repo`, `type`, `platform`, `mcu`, and native `build` method.
`community`, `verified`, and `official` are publisher/maintenance labels;
verification records include the specific `tested_ref` and `tested_libxr`, not a
permanent claim about every future version. Detailed evidence belongs to the
repository's tests, PRs and releases.

```sh
xrobot_src_man --sources Modules/sources.yaml list --type bsp
xrobot_src_man --sources Modules/sources.yaml search STM32
python -m unittest discover -s tests -v
```

C++ test cases require `CXX` (default `g++`); unavailable compilers are reported
as skipped, not passed. [CI_MIGRATION.md](CI_MIGRATION.md) describes exact-PR
source checks and coordinated publication prerequisites. Compile success does
not produce a date tag or hardware certification. Core releases coordinate
LibXR, XRobot and CodeGenerator; normal Module tags remain independent.

Documentation: <https://xrobot.work/>. Project governance: [GOVERNANCE.md](GOVERNANCE.md).
