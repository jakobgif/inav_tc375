# How to upgrade the port to latest INAV version

When a new version of INAV gets released the following prcedure shall be applied:

- Sync the "upstream" branch. This can be done via the GitHub website by pressing the "Sync" button while beeing in the "upstream" branch.

- Sync tags from the upstream repo. This can be done by running the commands 
```bash
git fetch --tags upstream
git push --tags
```
while being in the "upstream" branch. (https://stackoverflow.com/questions/70678073/how-do-i-sync-tags-to-a-forked-github-repo)

- Create a new release branch. Create a new branch from the latest tag. This can be done with the command
```bash
git checkout -b new-branch-name tag-name
```
example:
```bash
git checkout -b 8.0.1 8.0.1
```
(https://graphite.com/guides/git-create-branch-from-tag)

- The new branch shall be protected with the rule set "release protection".

- Now the needed commits can be merged into the release branch via pull requests.

- After the firmware has been tested a new tag can be created with its name containing the original tag with the suffix "-aurix". 
example:
```bash
git tag 8.0.1-aurix
git push origin 8.0.1-aurix
```
Now the firmware can be build and released.