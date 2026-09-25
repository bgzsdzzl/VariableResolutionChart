# Contributing to VariableResolutionChart

Thanks for your interest. This document covers everything you need to know
before opening an issue or a pull request.

## Before You Start

* **Bug or feature request?** Use the [issue templates](.github/ISSUE_TEMPLATE).
Blank issues are disabled on purpose.
* **Question?** Open a [Discussion](../../discussions) instead of an issue.
* **Security issue?** Do not open a public issue. Contact the maintainer
directly (see the GitHub profile).

## Development Setup

### Requirements

* Unreal Engine **5.5 – 5.8** (5.8.1 is what CI and local dev target)
* A C++ project (Blueprint-only projects cannot compile plugins)
* Visual Studio 2022 (Windows) or Xcode / clang (macOS / Linux)

### Getting the Source

```bash
# Fork on GitHub, then:
git clone https://github.com/<your-username>/VariableResolutionChart.git
```

### Building

Copy the plugin into your project's `Plugins/` folder:

```text
YourProject/
├── Plugins/
│   └── VariableResolutionChart/
└── YourProject.uproject
```

1. Right-click `YourProject.uproject` → Generate Visual Studio project files.
2. Open the generated `.sln`, build the `Development Editor` configuration.
3. Launch the editor and enable the plugin under
**Edit → Plugins → Data Structures** if it is not already on.

No engine modifications are required. Do not commit `Binaries/`,
`Intermediate/`, or `Saved/` — they are in `.gitignore` and PRs containing
them will be rejected.

## Project Layout

```text
VariableResolutionChart/
├── Source/VariableResolutionChart/
│   ├── Public/
│   │   ├── VRCTypes.h        # EVRCCellType, FVRCCell, FVRCPayload
│   │   └── VRCGrid.h         # UVRCGrid public API
│   └── Private/
│       └── VRCGrid.cpp       # implementation
├── Docs/
│   ├── API.md                # per-function reference
│   └── Design.md             # design decisions and trade-offs
└── VariableResolutionChart.uplugin
```

Before changing behavior, read `Docs/Design.md`. Most of the "why not do it
the obvious way" questions are answered there.

## Code Style

Follow the Unreal Engine coding standard.

The rules that matter most here:

* **Types:** `PascalCase`, prefix as required — `U` for `UObject` subclasses,
`F` for plain structs, `E` for enums.
* **Members:** `PascalCase`, no prefix. Private members live at the bottom of
the class body.
* **Local variables:** `PascalCase` (this project uses the newer UE style,
not the older `bIsFoo` / `MyVar` hybrid).
* **Boolean members:** prefix with `b`, e.g. `bIsValid`.
* **Constants:** `PascalCase` for `constexpr` / `static const`.
* Tabs for indentation, one tab per level. No spaces.
* Braces on their own line for functions, classes, and control structures.
* **Headers:** `#pragma once`, includes sorted (own header first, then engine
modules, then project headers).
* Keep comments sparse and meaningful. Do not restate the code; explain why
when the reason is not obvious.

## Public API Changes

The public API is `Public/VRCGrid.h` and `Public/VRCTypes.h`. Changes here
affect every consumer:

* Adding a new `UFUNCTION` or `USTRUCT` field is fine.
* Renaming or removing public symbols is a breaking change. Open an issue
first to discuss; expect to add a deprecation shim rather than a hard break.
* Changing the layout of `FVRCCell` invalidates serialized data. Call it
out in the PR description.

## Commit Messages

This project uses Conventional Commits:

```text
<type>(<scope>): <subject>

<body>

Signed-off-by: Your Name <your@email.com>
```

Allowed types:

|Type|Use for|
|-|-|
|`feat`|A new public API, cell type, or capability|
|`fix`|A bug fix|
|`docs`|Documentation only|
|`refactor`|Internal change with no behavior difference|
|`perf`|Performance improvement|
|`test`|Tests only|
|`chore`|Build scripts, tooling, CI|
|`style`|Formatting only|

Scopes follow the same list used for issue titles: `Grid`, `Accessors`,
`TypeSystem`, `Payload`, `Build`, `Docs`, `Other`. Pick the most specific one.

Examples:

```text
feat(TypeSystem): add Vector4 cell type
fix(Grid): preserve payload map on shrink
docs(API): clarify Resize overlap semantics
```

Keep the subject line under 72 characters, imperative mood, no trailing
period.

## Sign-Off (DCO)

Every commit must carry a `Signed-off-by` line. This is enforced by the
DCO2 app on every pull request.

To sign off, use `-s` when committing:

```bash
git commit -s -m "feat(TypeSystem): add Vector4 cell type"
```

This appends:

```text
Signed-off-by: Your Name <your@email.com>
```

The name and email must match the ones in your Git config
(`git config user.name` / `git config user.email`). If you forgot, amend:

```bash
git commit --amend -s --no-edit
git push --force-with-lease
```

By signing off, you certify the Developer Certificate of Origin — in short:
you wrote the code, or you have the right to submit it, and you are granting
the project the right to distribute it under its license.

If you use a web-based editor, GitHub's UI adds the sign-off automatically
when required by branch protection.

## Pull Requests

Open an issue first if the change touches the public API or is non-trivial.
Agreeing on the approach before writing code saves everyone time.

Create a branch from `main`:

```text
feat/add-vector4
fix/resize-payload-rekey
docs/api-clarify
```

Keep the PR focused. One logical change per PR. Do not bundle a refactor
with a feature.

Update documentation if the public API changes. `Docs/API.md` and
`Docs/Design.md` are part of the deliverable, not an afterthought.

Fill the PR description with:

* What problem the PR solves
* What approach you took and why
* Anything reviewers should pay attention to

## Merge Requirements

Every PR must satisfy:

* \[ ] DCO check passes (all commits signed off)
* \[ ] Builds cleanly on UE 5.8.1, Win64 Development Editor
* \[ ] No `Binaries/`, `Intermediate/`, or `Saved/` files committed
* \[ ] Public API changes reflected in `Docs/API.md`
* \[ ] Title and commit messages follow the formats above
* \[ ] All `VariableResolutionChart.\*` automation tests pass.
* \[ ] Logic changes in `Private/` are covered by tests or a documented manual verification.

The maintainer reviews and merges. Squash merges are used for multi-commit
PRs; the final commit message is taken from the PR title.

## Reporting Bugs

Use the Bug Report template. Titles must match:

```text
\\\[Bug] <Area> - <Short description>
```

Example: `\\\[Bug] Grid - payload map not rekeyed on Resize`.

Fill in the overview, environment, reproduction steps, and effect. An issue
without reproduction steps will be labelled `need-info` and closed if there
is no follow-up.

## Requesting Features

Use the Feature Request template. Titles must match:

```text
\\\[Feature] <Area> - <Short description>
```

Example: `\\\[Feature] TypeSystem - add Vector4 cell type`.

Describe the problem, not just the API you want. "I want SetVector4"
is less useful than "I am storing 4-component bone weights and currently
have to split them across two cells."

## Scope

This plugin is intentionally small and CPU-only. PRs that add the following
are out of scope and will be declined:

* Rendering dependencies (RHI, RenderCore, any texture class)
* Virtual Texture or Texture2DArray integration

If you need any of the above, build it on top of `UVRCGrid` in your own
module. The public API is designed to be the substrate, not the whole stack.

## Code of Conduct

Be civil. Disagree with the code, not the person. Off-topic or hostile
comments will be deleted, and repeat offenders will be blocked.

## License

By contributing, you agree that your contributions are licensed under the
Apache License 2.0. The `Signed-off-by` line in each commit is
your certification that you have the right to submit the code under that
license.

