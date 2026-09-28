# Core releases

LibXR, XRobot (`xrobot` on PyPI) and LibXR_CppCodeGenerator (`libxr` on PyPI)
are versioned independently; for example xrobot 1.0.0 is released together with
CodeGenerator 6.0.0. Both Python packages pin the same xr-syntax version exactly
(`xr-syntax==X`), so xr-syntax is released first. Modules keep their own tags.

Freeze exact candidates on the dev lines and validate them in order: LibXR
automatic tests; the backend compile matrix; both Python packages and the code
they generate; the official Module compile matrix; documentation; the
release-blocking BSPs and their hardware checks. Test a pull request's actual
head, not another repository's dev state. Unmaintained third-party BSPs do not
block a release.

Only accepted candidates are promoted to master, tagged and published. A tag does
not change a package: `pyproject.toml` must already carry the released version.

`tools/check_release.py` compares a maintainer-written acceptance record with the
candidate checkouts:

```sh
python tools/check_release.py --libxr ../libxr --xrobot . \
  --codegen ../LibXR_CppCodeGenerator --record release-acceptance.json
```

```json
{
  "xr-syntax": "0.2.0",
  "repositories": {
    "libxr":   {"commit": "<40-hex>"},
    "xrobot":  {"commit": "<40-hex>", "version": "1.0.0"},
    "codegen": {"commit": "<40-hex>", "version": "6.0.0"}
  },
  "checks": {"automatic": "pass", "backends": "pass", "packages": "pass",
             "modules": "pass", "docs": "pass", "bsp": "pass"}
}
```

It fails unless every checkout is clean and at the recorded commit, each package's
`pyproject.toml` version equals its recorded version, both packages pin
`xr-syntax==` the recorded version, and every check is `pass`. Only the responsible
workflow or maintainer sets a check to `pass`; logs stay with those workflows. The
checker validates correspondence only: it never builds, tags or publishes.
