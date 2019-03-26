
= Branches in Openconsole

In Openconsole, `dev/main` is the master branch for the repo.

Any branch that begins with `dev/` is recognized by our CI system and will automatically run x86 and amd64 builds and run our unit and feature tests. For feature branchs the pattern we use is `dev/<alias>/<whatever you want here>`. ex. `dev/austdi/SomeCoolUnicodeFeature`. The important parts are the dev prefix and your alias.

`inbox` is a special branch that coordinates Openconsole code to the main OS repo.

= Code Submission Process

Because we build outside of the OS repo, we need a way to get code back into it once it's been merged into `dev/main`. This is done by cherry-picking the PR to the `inbox` branch once it has been merged (and preferably squashed) into `dev/main`. We have a tool called Git2Git that listens for new merges into `inbox` and replicates the commits over to the OS repo. Feel free to approve and complete the `inbox` PR yourself. About a minute after the `inbox` PR is submitted, Git2Git will create a PR in the OS repo under the alias `miniksa`. It will automatically target the OS branch we're using at the time, it just needs you to go approve and complete it. Once that merge is completed it is a good idea to build the OS branch with the new code in it to make sure that the PR won't be the cause of a build break that evening.
