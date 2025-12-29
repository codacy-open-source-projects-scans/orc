# Building Orc and Applications That Use Orc

## Building Orc

Orc uses the [Meson build system](https://mesonbuild.com). When using the Meson ninja backend (which is the default), the normal sequence for compiling and installing the Orc library is thus:

```
meson builddir
ninja -C builddir
ninja -C builddir install
```

To see build options, check the meson_options.txt file and `meson setup --help`, or run `meson configure builddir` (after the first meson configure run). You can pass build options directly when running meson for the first time, or you can change options with `meson configure builddir -Doption=xyz` after the initial meson run.

By default, code generators for all targets are built into the library. Embedded system developers may wish to use the `orc-backend` option, which will disable all other code generators, saving about 200 kB in binary size. Outside of embedded systems, using the `orc-backend` option is not recommended.

## Building Orc Applications

Applications and libraries can use `pkg-config` to get all the needed compiler and linker flags to build against Orc. The following commands will provide the necessary compiler and linker flags:

```
pkg-config --cflags orc-0.4
pkg-config --libs orc-0.4
```

When compiling from source, the default installation directory is not in the default path for the `pkg-config`, so you may need to set the PKG_CONFIG_DIR environment variable.
