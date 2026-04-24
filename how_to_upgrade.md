# How to upgrade the port to a new INAV version

This document describes the steps to port the Aurix TC375 firmware to a new INAV release.

## 1. Sync the upstream branch

On GitHub, switch to the `upstream` branch and press the **Sync fork** button to pull in the latest commits from the original INAV repository. This branch is read-only — do not push custom commits to it.

## 2. Fetch the new release tag

After syncing, pull the new tags into the local clone and push them to the fork:

```bash
git fetch --tags upstream
git push --tags
```

Verify that the new tag (e.g. `9.0.1`) is now visible:

```bash
git tag | sort -V | tail -10
```

## 3. Create the release branch

Create a new local branch from the upstream release tag, then push it to the fork:

```bash
git checkout -b 9.0.1 9.0.1
git push origin 9.0.1
```

The branch name matches the INAV version exactly.

## 4. Protect the release branch

On GitHub, go to **Settings -> Rules -> Rulesets** and apply the existing **release protection** ruleset to the new branch. This prevents direct pushes and requires pull requests.

## 5. Bring in the Aurix-specific changes

Create a merge branch from the new release branch. This branch is used to bring all Aurix-specific changes from the previous release (tracked on `main`) into the new INAV version:

```bash
git checkout -b merge_9.0.1 9.0.1
git merge origin/main
```

Resolve any conflicts caused by INAV API changes, then commit and push:

```bash
git push origin merge_9.0.1
```

Open a pull request from `merge_9.0.1` into `9.0.1` on GitHub.

## 6. Build and test

After merging, build the firmware from the new release branch and run the standard flight tests. The CI pipeline builds automatically on every push.

## 7. Tag the release

Once the firmware is flight-proven, create and push the Aurix release tag:

```bash
git checkout 9.0.1
git pull origin 9.0.1
git tag 9.0.1-aurix
git push origin 9.0.1-aurix
```

For a multicore-enabled build, use a separate tag:

```bash
git tag 9.0.1-aurix-multicore
git push origin 9.0.1-aurix-multicore
```

Once the tag is pushed, create the GitHub release manually: go to **Releases -> Draft a new release**, select the tag, and publish it with the compiled firmware artifacts attached.
