FROM ubuntu:18.04
SHELL ["/bin/bash", "-c"]
COPY cross/emscripten.cross cross/glew.pc cross/SDL2_image.pc /cross/
RUN apt-get update \
 && apt-get -y upgrade \
 && apt-get install -y autoconf build-essential cmake default-jre-headless git libtool ninja-build pkg-config python3 python3-distutils \
 && ln -s "$(which python3)" /usr/local/bin/python \
 && git clone 'https://github.com/hartenfels/meson' \
 && git clone 'https://github.com/recp/cglm.git' \
 && git clone 'https://github.com/emscripten-core/emsdk.git' \
 && cd /emsdk \
 && ./emsdk install latest \
 && ./emsdk activate latest \
 && . ./emsdk_env.sh \
 && cd cglm \
 && sh autogen.sh \
 && mkdir /emprefix \
 && emconfigure ./configure --prefix=/emprefix \
 && emmake make \
 && emmake make install \
 && sed -e 's/PREFIX_GOES_HERE/\/emprefix/' /cross/emscripten.cross > /emscripten.cross \
 && sed -e 's/PREFIX_GOES_HERE/\/emprefix/' /cross/glew.pc > /emprefix/lib/pkgconfig/glew.pc \
 && sed -e 's/PREFIX_GOES_HERE/\/emprefix/' /cross/SDL2_image.pc > /emprefix/lib/pkgconfig/SDL2_image.pc
