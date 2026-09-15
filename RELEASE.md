# Coordinated core releases

LibXR, XRobot and LibXR_CppCodeGenerator share one numeric release version.
Module releases keep their independent exact date tags. No new distribution
product or BSP package solver is required.

Freeze exact candidates on the coordinated development line. Validate in order:
LibXR automatic tests; backend compile matrix; both Python packages and generated
code; official Module compatibility; documentation; maintained release-blocking
BSPs and required hardware checks. Test a PR's actual head rather than silently
substituting a different repository's dev state. Unmaintained third-party BSPs
do not block a core release.

Only after acceptance should candidates be promoted to master, tagged and
published. An unchanged implementation may join a new release, but wheel/sdist
metadata must contain the new version: a tag on an old commit does not rewrite
a static pyproject.toml version field.

`tools/check_release.py` checks a small maintainer-produced acceptance record
against clean committed source identities and both Python package versions:

```sh
python tools/check_release.py --version 6.0.0 \
  --libxr ../libxr --xrobot . --codegen ../LibXR_CppCodeGenerator \
  --record release-acceptance.json
```

The version above is an example, not an assigned release. The record contains
`version`, a `commits` map (`libxr`, `xrobot`, `codegen`) and a `checks` map with
`automatic`, `backends`, `packages`, `modules`, `docs`, and `bsp` set to `pass`
only by the responsible acceptance workflow/maintainer. Keep the detailed logs
with those workflows. The checker validates correspondence; it does not invent
or independently certify hardware evidence and never builds, tags or publishes.

All public publication actions remain explicit. Never use a successful
single-module compilation to auto-create a release tag.
