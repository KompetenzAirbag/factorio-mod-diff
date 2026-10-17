# Factorio Mod Diff
Generates a json file of Factorio recipe differences across different game instances.

# Installation
## Build from source
Clone the repository and make:
```{bash}
git clone https://github.com/KompetenzAirbag/factorio-mod-diff && \
cd factorio-mod-diff && \
make
```
This will generate an executable in the current directory.

# Usage
```{bash}
factorio-mod-diff [FLAGS]
    -i | --instances <path 1> <path 2> path should point to the base directory of Factorio (for Steam installs: <steam>/steamapps/common/Factorio). <path 1> expects a 1.x or 2.x install while <path 2> only expects a 2.x install
    -o | --output <path> defaults to stdout, any other path will generate a file. ".json" does not need to be mentioned.
```
`<path>` may include `~` which will be substituted with your home directory. Any whitespaces must be escaped using "\\".

# TODO
- [x] Add support for 'space' in \<path\>
- [x] Generate full migration map from all mods per instance
- [ ] Add UI with clay
- [ ] Remove trailing "/" from -i path flags
- [ ] There can be multiple zips of the same mod (i. e. different versions)
- [x] Run factorio instances in child process so they run in parallel
- [x] Add flag for switching to "expensive" 1.1 recipes
- [ ] Refactor code so that \<path\> can be any Factorio version in any order
- [ ] Add warning for differently enabled mods across instances
- [ ] Sometimes an ingredient is marked as "added" although nothing changed (`wooden-chest`)
