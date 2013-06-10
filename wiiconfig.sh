export SDL_CONFIG=`pwd`/sdl-config
./configure --enable-release --host=powerpc-eabi --program-suffix=".elf" --disable-shared --enable-static --disable-network --prefix=/apps/abuse --with-assetdir=/apps/abuse/data --bindir=/apps/abuse -sbindir=/apps/abuse -libexecdir=/apps/abuse
unset SDL_CONFIG
find . -name Makefile | xargs sed -i -e 's/  -lSDL_mixer$//' -e 's/ -lm -mrvl/-mrvl/'
