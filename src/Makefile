all:
	clear
	arm-linux-gnueabi-gcc -static -march=armv6k -mfpu=vfp -mfpu=neon gameboy.c platform.c render.c fps.c input.c -Wno-format-truncation -Ofast -flto -fomit-frame-pointer -funroll-loops -o ../bin/gb3ds

old:
	clear
	arm-linux-gnueabi-gcc -static -march=armv6k -mfpu=vfp gameboy.c platform.c render.c fps.c input.c -Wno-format-truncation -Wno-unused-result -Ofast -flto -fomit-frame-pointer -funroll-loops -o ../bin/gbold
