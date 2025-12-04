# How to upgrade the port to latest INAV version

When a new version of INAV gets released a new branch named after the corresponding tag name can be created. This branch should have the commit with the wanted tag as the base. The branch should be portected using the rule set "release protection". Afterwards the needed commits can be merged into the branch via a PR. After the firmware has been tested a new tag can be created with its name containing the original tag with the suffix "-aurix".
```
git tag 2.1.0-aurix //create a tag
git push origin 2.1.0-aurix //push the tag
```
Now the firmware can be build and released.

## Summary
- Create a new branch with the latest commit pointing to the wanted release tag. eg https://github.com/jakobgif/inav_tc375/tree/9.0.0
- merge branch https://github.com/jakobgif/inav_tc375/tree/devel (which is used for development) into the newly created https://github.com/jakobgif/inav_tc375/tree/9.0.0