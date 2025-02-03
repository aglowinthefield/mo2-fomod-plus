# FOMOD Plus for Mod Organizer 2
![wizard](./scanner/resources/wizard.gif)

## Overview
A total rewrite of the FOMOD installer. Supports all features of the original FOMOD installer, and adds a few new ones.

**Features**
- FOMOD window keeps the size you choose between installs.
- Filter by mods installed via FOMOD. [Pic](doc/filter.png)
- A Scanner plugin to retroactively mark mods as FOMOD-installed where possible. [Pic](doc/scanner-menu.png)
- A nice wizard hat in the content column. [Pic](doc/content.png)

## Using
1. Unzip the latest release into your MO2 plugins directory.
2. If running for the first time, use the scanner to populate your mods.
3. Install FOMODs as normal and enjoy the new features.

## Future Plans
Since this is a WIP and barely in alpha stages, expect some bugs as things progress. 
Some of my plans, roughly ordered, include:

- [ ] Better UI for image viewing, to match existing.
- [ ] Searchable installation notes (manual input and automatic)
- [ ] Selecting previously selected manual options where possible.
- [ ] Introduce new optional features to the FOMOD XML schema to allow more fun~.

## Developing

Development of this plugin is done within the context of `mob`.
You'll want to follow all setup instructions for `mob` before proceeding.

1. [Set up mob](https://github.com/ModOrganizer2/mob) and all of its requirements.
2. Clone this repository inside the generated `<rootDir>/build/modorganizer-super` directory.
3. Open in your IDE of choice. I prefer CLion and it should be fairly painless.

*Note: I don't have a good setup for building to DLLs within the repo at the moment.*

## Debugging
To debug:
1. Make sure you are debugging the `fomod_plus` target.
2. Set your target executable to the MO2 executable inside `<mobDir>/install/bin`.

*NOTE* Inside `CMakeLists.txt` you'll see a pair of lines like this:
```cmake
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_LIST_DIR}/../../../install/bin/plugins)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "D:/Modding/MO2/plugins")
```
for some reason on my main PC I can't use the `mob`-built MO2 instance to debug without a lot 
of exceptions. This is not the case on my laptop. So if you get lots of exceptions pointing to the 
`mob` dev build, try changing the hardcoded path in that second line to your _real_ MO2 instance.

## License
This project is licensed under the MIT License.

## Attribution
Much love and appreciation to the authors of the original FOMOD installer. Source can be read [here](https://github.com/ModOrganizer2/modorganizer-installer_fomod)
if curious. Early development of this plugin was heavily influenced by the original codebase, and certain classes were lifted
almost entirely from the original (with modifications and attribution in the file).

<a href="https://www.flaticon.com/free-icons/witch" title="witch icons">Witch icons created by Umeicon - Flaticon</a>
