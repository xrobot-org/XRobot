# 发版 / Releases

LibXR、XRobot（PyPI 上的 `xrobot`）和 LibXR_CppCodeGenerator（PyPI 上的 `libxr`）各自有版本号，
例如 xrobot 1.0.0 与 CodeGenerator 6.0.0 一同发布。两个 Python 包都精确锁定同一个 xr-syntax
版本（`xr-syntax==X`），所以 xr-syntax 先发布。模块使用各自的 tag。

LibXR, XRobot (`xrobot` on PyPI) and LibXR_CppCodeGenerator (`libxr` on PyPI) are versioned
independently; for example xrobot 1.0.0 is released together with CodeGenerator 6.0.0. Both
Python packages pin the same xr-syntax version exactly (`xr-syntax==X`), so xr-syntax is
released first. Modules keep their own tags.

## 验收 / Acceptance

发版前先在各仓库的 dev 上选定候选提交，再按下表的顺序逐项检查。每一项都使用这些候选提交。

Candidate commits are chosen on the dev line of each repository and checked in the order of the
table below. Every check uses these candidates.

| 检查项 Check | 内容 | Content |
| --- | --- | --- |
| `automatic` | LibXR 的自动测试 | LibXR's automatic tests |
| `backends` | 各平台后端的编译矩阵 | The compile matrix of the platform backends |
| `packages` | 两个 Python 包的测试，以及它们生成的代码 | The tests of both Python packages and the code they generate |
| `modules` | 官方模块的编译矩阵 | The compile matrix of the official Modules |
| `docs` | 文档网站的构建 | The build of the documentation website |
| `bsp` | 官方 BSP 用候选版本重新生成并构建 | The official BSPs regenerated and built with the candidates |

BSP 的硬件测试由各 BSP 的维护者自行安排。

Hardware testing of a BSP is arranged by its maintainers.

## 发布 / Publishing

全部检查通过后按以下步骤发布：

1. 每个仓库从 dev 向 master 提交 PR 并合并。
2. 用 `tools/check_release.py` 核对验收记录（见下一节）。
3. xr-syntax 版本有变化时先发布 xr-syntax，再发布 CodeGenerator 和 XRobot：在 master 上创建 GitHub
   Release，tag 为 `v` 加版本号（如 `v1.0.0`），发布工作流随即构建并上传到 PyPI。tag 只标记
   提交，`pyproject.toml` 中必须已经是要发布的版本号。
4. XRobot 把 `v1` 移到同一个提交。模块 CI 通过 `module-ci.yml@v1` 调用本仓库的共享工作流；
   共享工作流有不兼容的修改时改用 `v2`。

The release follows these steps once every check has passed:

1. Each repository merges a pull request from dev into master.
2. `tools/check_release.py` checks the acceptance record (next section).
3. xr-syntax is published first when its version changed, then CodeGenerator and XRobot: a
   GitHub Release on master with the tag `v` plus the version (such as `v1.0.0`) makes the
   publish workflow build the package and upload it to PyPI. The tag only marks the commit,
   so `pyproject.toml` must already carry the released version.
4. XRobot moves `v1` to the same commit. Module CI calls this repository's shared workflow as
   `module-ci.yml@v1`; an incompatible change to the shared workflow moves to `v2`.

```sh
git tag -f v1 'v1.0.0^{commit}'
git push -f origin v1
```

## 验收记录 / Acceptance record

`tools/check_release.py` 比较维护者填写的验收记录与各候选仓库：

`tools/check_release.py` compares the acceptance record written by a maintainer with the
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

只有以下条件全部满足时核对才通过：每个仓库没有未提交的修改，且位于记录的提交；每个包
`pyproject.toml` 中的版本等于记录的版本；两个包都锁定记录中的 xr-syntax 版本；每个检查项都是
`pass`。记录中的检查结果由维护者根据各项检查填写。这个工具只做核对，构建、打 tag 和发布按上一节
的步骤进行。

The check passes only when every checkout is clean and at the recorded commit, each package's
`pyproject.toml` version equals the recorded version, both packages pin the recorded xr-syntax
version, and every check is `pass`. A maintainer fills in the checks from their results. The
tool only compares; building, tagging and publishing follow the steps of the previous section.
