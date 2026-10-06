# Factorio Mod Diff
Generates a json file of Factorio recipes differences across different game instances.

# Installation
## Build from source
Clone the repository and make:
```{bash}
git clone https://github.com/KompetenzAirbag/factorio-mod-diff && \
cd factorio-mod-diff && \
make
```
This will generate an executable in the `factorio-mod-diff` directory.

# Usage
```{bash}
factorio-mod-diff [FLAGS]
    -i | --instances <path>,<path>
    -o | --output <path> defaults to stdout, any other path will generate a file. ".json" does not need to be mentioned.
```
`<path>` may include `~` which will be substituted with your home directory.
