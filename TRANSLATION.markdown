# Translating HandBrake

We use Transifex to manage the translation resources for this project.
Please see: [Transifex - HandBrake Public Page](https://www.transifex.com/HandBrakeProject/public/)

## Requirements and Rules

While we wish to welcome contributions from people, we need to have some ground rules to make sure translations run smoothly. 

- Contributors must have fluent verbal and written English. (English is the base language used for translation.)
- You must not use a translation service to aid translation. If anything is unclear, please ask!
- Our [Code of Conduct](https://github.com/HandBrake/HandBrake/blob/master/CODE_OF_CONDUCT.md) and [Community Rules](https://forum.handbrake.fr/app.php/rules) must be followed.


## How to become a translator 

1. Please submit a join request on transifex for the relevant GUI that you currently use.
2. Create a "Translation Request" GitHub issue
3. We will then discuss the request and make a decision whether to accept or reject it.


## Requesting a Language

HandBrake is managed by a very small team of contributors and as such, we do not have the resources to support translations for every language that may be requested and keep all the user interfaces in sync. As such, we welcome anyone who is willing to help by submitting pull requests to keep the user interface language files in sync with the translations available on [Transifex](https://www.transifex.com/HandBrakeProject/public/).

Languages that fall out of date or lack sufficient maintainers may be removed from the UI until such time they are in a suitable state.

You can request a language on the [Transifex - HandBrake Public Page](https://www.transifex.com/HandBrakeProject/public/)


## Viewing Updated Translations

After a pull request including the latest changes imported from Transifex is accepted and merged, you can view the updated translations in our development snapshot builds.( https://github.com/HandBrake/HandBrake-snapshots )

For macOS and Windows, these are typically published twice weekly.  
Linux builds are currently published on an ad-hoc basis but will usually be pushed after a translation update.


## HandBrake Release Policy (String Freeze)

Due to the small and volunteer nature of the team, it's difficult to plan our releases with precision. However, we will typically try to provide 2 to 4 weeks notice via a post on Transifex when we think there may be a new HandBrake release coming. 

During this time, we will try and avoid making any string changes as much as possible. We may have to do so in the case of bugs however we are generally trying to avoid refactoring / new features during this time. 




