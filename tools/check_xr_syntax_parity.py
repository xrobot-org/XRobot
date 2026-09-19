"""Compare legacy XRobot interface extraction with the xr-syntax migration path."""

import argparse
from pathlib import Path

from xrobot.CppSource import extract_interface as legacy_extract_interface
from xrobot.SourceSyntax import extract_interface as syntax_extract_interface


def _result(source, name, extractor):
    try:
        return ("accepted", extractor(source, name))
    except ValueError as error:
        return ("rejected", str(error))


def compare_header(path):
    source = path.read_text(encoding="utf-8-sig")
    name = path.stem
    legacy = _result(source, name, legacy_extract_interface)
    migrated = _result(source, name, syntax_extract_interface)
    if legacy[0] != migrated[0]:
        return False, "%s: legacy=%s xr-syntax=%s" % (path, legacy[0], migrated[0])
    if legacy[0] == "accepted" and legacy[1] != migrated[1]:
        return False, "%s: interface mismatch\nlegacy=%r\nxr-syntax=%r" % (
            path,
            legacy[1],
            migrated[1],
        )
    return True, "%s: %s" % (path, legacy[0])


def headers(roots):
    for root in roots:
        root = Path(root)
        if root.is_file() and root.suffix in (".hpp", ".h"):
            yield root
            continue
        for path in sorted(root.rglob("*.hpp")):
            if path.stem == path.parent.name or path.parent == root:
                yield path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("roots", nargs="+")
    args = parser.parse_args()

    total = accepted = rejected = 0
    failures = []
    for path in headers(args.roots):
        total += 1
        ok, message = compare_header(path)
        print(message)
        if not ok:
            failures.append(message)
            continue
        source = path.read_text(encoding="utf-8-sig")
        if _result(source, path.stem, legacy_extract_interface)[0] == "accepted":
            accepted += 1
        else:
            rejected += 1
    print(
        "parity_total=%d accepted=%d rejected=%d failures=%d"
        % (total, accepted, rejected, len(failures))
    )
    if failures:
        raise SystemExit(1)
    if total == 0:
        raise SystemExit("no module headers found")


if __name__ == "__main__":
    main()
