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
    -i | --instances <path> <path> path should point to the base directory of Factorio (for Steam installs: <steam>/steamapps/common/Factorio)
    -o | --output <path> defaults to stdout, any other path will generate a file. ".json" does not need to be mentioned.
```
`<path>` may include `~` which will be substituted with your home directory.

# TODO
- [ ] Add support for 'space' in \<path\>
- [x] Generate full migration map from all mods per instance
- [ ] Add UI with clay
- [ ] Remove trailing "/" from -i path flags
- [ ] There can be multiple zips of the same mod (i. e. different versions)
- [ ] Run factorio instances in child process so they run in parallel
