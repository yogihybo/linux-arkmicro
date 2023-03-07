cd ../obj/arm/
mkdir -p libusbmuxd-1.0.10
cd libusbmuxd-1.0.10
#../../../libusbmuxd-1.0.10/configure --host=arm --disable-shared --enable-static --prefix=/media/wenyi/disk2/install/arm/ CC=/media/work/project/linux-arkmicro/output/board/ark1668e_devb/buildroot/host/bin/arm-linux-gnueabihf-gcc CXX=/media/work/project/linux-arkmicro/output/board/ark1668e_devb/buildroot/host/bin/arm-linux-gnueabihf-g++ LDFLAGS='-lplist -L/media/work/project/linux-arkmicro/output/board/ark1668e_devb/buildroot/staging/usr/lib/' CFLAGS=-I/media/work/project/linux-arkmicro/output/board/ark1668e_devb/buildroot/staging/usr/include/
../../../libusbmuxd-1.0.10/configure --host=arm --disable-shared --enable-static --prefix=/media/wenyi/disk2/install/arm/ CC=/media/work/project/linux-arkmicro/output/board/ark1668e_devb/buildroot/host/bin/arm-linux-gnueabihf-gcc CXX=/media/work/project/linux-arkmicro/output/board/ark1668e_devb/buildroot/host/bin/arm-linux-gnueabihf-g++
make -j8
make install
