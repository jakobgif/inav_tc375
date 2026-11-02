# How to develop features for the Aurix TC375 INAV port

This document describes the Git workflow for implementing features, fixing bugs, and releasing firmware.

## Branch overview

| Branch | Purpose |
|---|---|
| `upstream` | Mirror of the original INAV repo. Read-only, never commit here. |
| `main` | Tracks all Aurix-specific changes across INAV versions. No direct pushes — all changes must come in via pull request. |
| `devel` | Active development. Feature branches are merged here before promoting to `main`. |
| `<version>` | Release branch for a specific INAV version (e.g. `8.0.1`, `9.0.0`). Protected, changes come in via pull request. |
| `<feature>` | Short-lived feature or fix branch created from `main` or `devel`. |
| `merge_<version>` | Temporary branch used to port `main` changes into a new release branch. Deleted after the PR is merged. |

## Feature development flow

### 1. Create a feature branch

Branch off from `main` (or `devel` if the work in progress there is needed):

```bash
git checkout main
git pull origin main
git checkout -b my-feature
```

### 2. Commit changes

Make focused commits with descriptive messages. Each commit should represent a single logical change.

### 3. Open a pull request to `devel`

Push the feature branch and open a PR targeting `devel`:

```bash
git push origin my-feature
```

On GitHub, open a pull request from `my-feature` -> `devel`.

### 4. Promote `devel` to `main`

Once the feature is stable and tested on `devel`, open a pull request from `devel` -> `main`.

### 5. Apply to the active release branch

New features that should be included in the current release are brought in by opening a pull request from `main` -> `<release-branch>` (e.g. `main` -> `8.0.1`). If the diff is not clean, create a dedicated `merge/<version>` branch to resolve conflicts first, then PR that into the release branch.

## Releasing

After the firmware on a release branch is flight-proven, create a version tag (see [how_to_upgrade.md](how_to_upgrade.md) for the exact commands). Once the tag is pushed, create the GitHub release manually: go to **Releases -> Draft a new release**, select the tag, and publish it with the compiled firmware artifacts attached.

### Tag naming convention

| Scenario | Tag format | Example |
|---|---|---|
| Standard single-core build | `<inav-version>-aurix` | `8.0.1-aurix` |
| Multicore build | `<inav-version>-aurix-multicore` | `8.0.1-aurix-multicore` |
