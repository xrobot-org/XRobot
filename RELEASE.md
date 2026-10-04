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

全部检查通过、并用 `tools/check_release.py` 核对验收记录（见下一节）后，按以下顺序发布。每一步
依赖的东西都在它之前就位：

1. LibXR：先在 master 上手动运行一次 “Build Docs Image” 工作流，并把 GHCR 上的 `libxr-docs`
   包设为公开（API 文档的部署任务拉取这个镜像）；然后从 dev 向 master 提交 PR 并合并。
2. xr-syntax 版本有变化时发布 xr-syntax。
3. CodeGenerator 和 XRobot 从 dev 合入 master，在 master 上创建 GitHub Release，tag 为 `v` 加版本号
   （如 `v1.0.0`），发布工作流随即构建并上传到 PyPI。tag 只标记提交，`pyproject.toml` 中必须已经
   是要发布的版本号。CodeGenerator 的发布工作流把 LibXR master 当时的提交写成默认检出的提交，
   所以这一步在 LibXR 合并之后。
4. XRobot 把 `v1` 移到同一个提交。模块 CI 通过 `module-ci.yml@v1`、STM32 BSP 的 CI 通过
   `bsp-stm32-ci.yml@v1` 调用本仓库的共享工作流；共享工作流有不兼容的修改时改用 `v2`。
5. 各模块在 dev 上把 CI 改为调用 `module-ci.yml@v1`（删去指向 dev 的 `xrobot-ref`、`libxr-ref`、
   `dependency-ref`），再从 dev 合入 master；模块源（catalog）随后合入 master 并重新部署。
6. 各 BSP 在 dev 上把 LibXR 子模块更新到第 1 步合入的提交，CI 改为调用 `bsp-stm32-ci.yml@v1`，
   按模块的 master 重新解析 `xrobot.lock`，再从 dev 合入 master。模板仓库同样更新 LibXR 子模块。
7. 文档网站从 dev 合入 master，`master` 的推送触发部署（Pages 的 `github-pages` 环境需允许 `master`）；发布 VS Code 扩展。

The release follows these steps in this order once every check has passed and
`tools/check_release.py` has checked the acceptance record (next section); each step finds
what it depends on already in place:

1. LibXR: run the "Build Docs Image" workflow once on master and make the `libxr-docs` package on
   GHCR public (the API documentation deployment pulls this image); then merge a pull request
   from dev into master.
2. xr-syntax is published when its version changed.
3. CodeGenerator and XRobot merge dev into master; a GitHub Release on master with the tag `v`
   plus the version (such as `v1.0.0`) makes the publish workflow build the package and upload it
   to PyPI. The tag only marks the commit, so `pyproject.toml` must already carry the released
   version. The CodeGenerator publish workflow writes the commit LibXR master has at that moment as
   the default checkout, so this step comes after the LibXR merge.
4. XRobot moves `v1` to the same commit. Module CI calls this repository's shared workflows as
   `module-ci.yml@v1` and the CI of STM32 BSPs as `bsp-stm32-ci.yml@v1`; an incompatible change
   to a shared workflow moves to `v2`.
5. Each Module switches its CI on dev to `module-ci.yml@v1` (dropping the `xrobot-ref`,
   `libxr-ref` and `dependency-ref` that point at dev) and then merges dev into master; the Module
   Sources (catalogs) then merge into master and are deployed again.
6. Each BSP updates its LibXR submodule on dev to the commit merged in step 1, switches its CI to
   `bsp-stm32-ci.yml@v1`, resolves `xrobot.lock` again against the master branches of its
   Modules, and then merges dev into master. The template repositories update their LibXR
   submodule as well.
7. The documentation website merges dev into master, and the push to `master` deploys it (the `github-pages` environment of Pages has to allow `master`); the VS Code extension is published.

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
