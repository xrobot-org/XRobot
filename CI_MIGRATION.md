# Coordinated CI transition

The reusable `module-build.yml` prepares exact PR-head sources, resolves header
package dependencies using that PR's logical branch, and then invokes native
CMake. It does not instantiate hardware objects, claim hardware validation or
create date tags on compilation success.

The candidate Module workflows pin the coordinated XRobot reusable workflow and
tooling to an exact commit, alongside an exact LibXR commit. They do not depend
on a simultaneous update of the default branches or the installed PyPI releases.
The normal ModuleCreator template targets the coordinated `dev` line; integrating
the matching tooling there is required before using that template unchanged.

Publish matching feature branches for modules changed together before testing
cross-repository PRs. Relative `same-or-dev` dependencies then select those exact
branch names. If a dependency feature branch is merged and removed, a fresh
resolve selects its `dev`; an existing project lock remains frozen until update.

Module `tests/xrobot_compile.cpp` files are explicit source compile fixtures,
not generated configurations or hardware mocks. Existing behavioral tests and
BSP/hardware acceptance remain separately required. The Linux CI profile enables
64-bit integer and double diagnostic formatting; embedded BSPs choose their own
profile. Webots Modules use the actual Webots platform, not a fake robot handle.

Pushing a candidate branch triggers its source build workflow. This does not
create tags or releases. CI uses the same package resolver and native CMake
entrypoints as a developer; there is no separate build frontend.
