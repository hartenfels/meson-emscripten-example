# Meson Emscripten Example

This assumes you're using this branch in my forked Meson repo: <https://github.com/hartenfels/meson/tree/wasm>.

See also this issue: <https://github.com/mesonbuild/meson/pull/5512>

This is a non-trivial example of compiling a project to Web Assembly (and asm.js if you want, it doesn't really change anything) with Emscripten. It draws a rotating cube with WebGL and uses SDL2, SDL2\_image, cglm and GLEW as dependencies. There's also a test that's run using node.js. It also runs on Linux (and probably other platforms), where it's just a regular OpenGL ES 2.0 application.

Meson needs some patches, the [meson.build](meson.build) needs some special-casing and the build environment needs *a lot* of hacks. I don't know how much of this stuff can actually be solved by changes to Meson or Emscripten and how much will have to stay as project-specific hacks.

There's a [Dockerfile](Dockerfile) provided to set up the build environment. You can use the [build-in-docker](build-in-docker) script to set up the docker image and run the build.

## Build Setup Walkthrough

I'll walk through the Dockerfile and describe all the stuff it needs to do to get from a pristine Ubuntu image to actually building this thing. "Interesting" parts are marked with **hack**.

```
FROM ubuntu:18.04
SHELL ["/bin/bash", "-c"]
COPY cross/emscripten.cross cross/glew.pc cross/SDL2_image.pc /cross/
RUN apt-get update \
 && apt-get -y upgrade \
```

Start off with some housekeeping. We're basing on Ubuntu 18.04 and using `bash` so that some Emscripten scripts work. We copy some things around and do package upgrades.

```
 && apt-get install -y autoconf build-essential cmake default-jre-headless git libtool ninja-build pkg-config python3 python3-distutils \
```

Install various dependencies. Build essentials and git are obvious. Autotools and libtool is needed for building cglm. The JRE and distutils are needed for Emscripten. The rest is for Meson and Ninja.

```
 && ln -s "$(which python3)" /usr/local/bin/python \
```

I'm sure there's a way to do this properly, but I just want `python` in my `PATH`.

```
 && git clone 'https://github.com/hartenfels/meson' \
 && git clone 'https://github.com/recp/cglm.git' \
 && git clone 'https://github.com/emscripten-core/emsdk.git' \
```

Go fetch [the appropraite Meson fork](https://github.com/hartenfels/meson), [cglm sources](https://github.com/recp/cglm.git) (a math library) and [the Emscripten emsdk](https://github.com/emscripten-core/emsdk.git).

```
 && cd /emsdk \
 && ./emsdk install latest \
 && ./emsdk activate latest \
 && . ./emsdk_env.sh \
```

Fetch, enable and arm Emscripten tools.

```
 && cd cglm \
 && sh autogen.sh \
 && mkdir /emprefix \
 && emconfigure ./configure --prefix=/emprefix \
 && emmake make \
 && emmake make install \
 ```

Build cglm (a math library) and stick the results into /emprefix. That may be a **hack**, but maybe I just can't figure out where you're supposed to put these files normally so that Emscripten finds them on its own.

```
 && sed -e 's/PREFIX_GOES_HERE/\/emprefix/' /cross/emscripten.cross > /emscripten.cross \
```

Ah yes, the cross file. It has its explanation in the comments within it and more than one **hack**, so [look in there](cross/emscripten.cross).

```
 && sed -e 's/PREFIX_GOES_HERE/\/emprefix/' /cross/glew.pc > /emprefix/lib/pkgconfig/glew.pc \
```

**Hack:** a custom pkg-config file for GLEW. As mentioned before, GLEW seems to just *exist* in Emscripten. Maybe Meson could just know that it does.

```
 && sed -e 's/PREFIX_GOES_HERE/\/emprefix/' /cross/SDL2_image.pc > /emprefix/lib/pkgconfig/SDL2_image.pc
```

**Hack:** ahh, SDL2_image is a tricky one. You have to specify *which image formats you need*. So that means it's not enough for Meson to know that you need SDL2_image, it *also* needs to know that you want to load like PNG and TGA images or whatever. Oh, and JPEG is unsupported altogether.

Either way, this pkg-config file just has PNG as its only format welded into it, but a proper solution would somehow require telling Meson what image formats you need. Or the user has to specify these manually as custom cflags.

Also yes, these are cflags *and* ldflags. The former is necessary for the headers to be found and the latter for it actually pulling in the library correctly.

And finally, have a look at the [meson.build](meson.build), it needs to do some special-casing for e.g. preloading files. That's basically unavoidable though... unless you want to implement something like `meson.get_compiler('c').maybe_preload_or_embed_files(['assets'])` that does nothing on normal compilers, but preloads or embeds files depending on if you got a gui_app or not.
