# Meson Emscripten Example

This assumes you're using this branch in my forked Meson repo: <https://github.com/hartenfels/meson/tree/wasm>.

See also this issue: <https://github.com/mesonbuild/meson/pull/5512>

This is a non-trivial example of compiling a project to Web Assembly (and asm.js if you want, it doesn't really change anything) with Emscripten. It draws a rotating cube with WebGL and uses SDL2, SDL2\_image, cglm and GLEW as dependencies. There's also a test that's run using node.js. It also runs on Linux (and probably other platforms), where it's just a regular OpenGL ES 2.0 application.

Meson needs some patches, the [meson.build](meson.build) needs some special-casing and the build environment needs *a lot* of hacks. I don't know how much of this stuff can actually be solved by changes to Meson or Emscripten and how much will have to stay as project-specific hacks.

There's a [Dockerfile](Dockerfile) provided to set up the build environment. You can use the [build-in-docker](build-in-docker) script to set up the docker image and run the build.

TODO: put an explanation of all the hacks here. Walk through the Dockerfile and explain it.
