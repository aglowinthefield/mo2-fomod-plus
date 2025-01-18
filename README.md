### Developing

Development of this plugin is done within the context of `mob`.
You'll want to follow all setup instructions for `mob` before proceeding.

1. [Set up mob](https://github.com/ModOrganizer2/mob) and all of its requirements.
2. Clone this repository inside the generated `<rootDir>/build/modorganizer-super` directory.
3. Open in your IDE of choice. I prefer CLion and it should be fairly painless.

#### Debugging

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

---
#### Attribution
<a href="https://www.flaticon.com/free-icons/witch" title="witch icons">Witch icons created by Umeicon - Flaticon</a>