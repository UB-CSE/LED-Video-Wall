# Microcontroller Client

# Installation
## Required
* [`idf.py`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html#installation) (Install `v5.3.2`, due to dependency lock of the Arduino component)

## Optional
* [`idf-eclipse-plugin`](https://github.com/espressif/idf-eclipse-plugin/blob/master/README.md) (Eclipse)
* [`vscode-esp-idf-extension`](https://github.com/espressif/vscode-esp-idf-extension/blob/master/README.md) (VS Code)
* [`clangd`](https://clangd.llvm.org/installation) (LSP)
* [`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) (Formatter)
* [`espflash`](https://github.com/esp-rs/espflash) (Alternative to `esptool.py`)

# Usage
Before running any commands, be sure to load the environment via:
```bash
$ . $HOME/esp/esp-idf/export.sh
```
or for fish:
```bash
$ . $HOME/esp/esp-idf/export.fish
```

## Configuring
To configure the project and components, run:
```bash
$ idf.py menuconfig
```

To update settings and install components, run:
```bash
$ idf.py reconfigure
```

## Building
To build the project, run:
```bash
$ idf.py build
```

## Flashing & Monitoring
To flash the build to the microcontroller, run:
```bash
$ idf.py flash
```

Finally, to monitor the microcontroller, run:
```bash
$ idf.py monitor
```

These commands can all be combined via:
```bash
$ idf.py build flash monitor
```

### `espflash`
`espflash` is an alternative to `esptool.py` (which `idf.py` uses internally) for flashing and monitoring. It can flash via the command:
```bash
$ espflash flash build/client.bin
```

And monitored via:
```bash
$ espflash monitor
```

## `clangd` Support
For LSP support with `clangd`, the project needs to be reconfigured with the `clang` compiler. There are a few prerequisites before getting started:
* [`clangd`](https://clangd.llvm.org/installation) (LSP)
* `esp-clang` (Compiler, see below)

To install `esp-clang`, run:
```bash
idf_tools.py install esp-clang
````

To setup `clangd` support, run:
```bash
$ idf.py -B build-clang -D IDF_TOOLCHAIN=clang reconfigure
```
Unfortunately, `clang` support is still experimental and will fail to build. Thus, after executing this command, ensure the `gcc` compiler is specified while building:
```bash
$ idf.py -D IDF_TOOLCHAIN=gcc build
```
Alternatively, reconfigure to use `gcc`:
```bash
idf.py -D IDF_TOOLCHAIN=gcc reconfigure
```

## FAQ
### `idf.py` or `idf_tools.py` not found?
Make sure the [environment variables are loaded](#Usage).

### Issues building or setting things up?
See if running the command below helps:
```bash
$ idf.py fullclean
```

If there are issues with `clang` support, try removing the `build-clang/` directory:
```bash
rm -rf build-clang
```

