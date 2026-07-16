# arm-static-libs

Pre-built static cross-libraries for `arm-linux-gnueabihf`, kept here so
future on-device utility builds (in the main repo's `tools/`) don't need
to rebuild these from source every time.

## ncurses-install

ncurses 6.1, statically built with terminal descriptions for
`linux`/`vt100`/`vt102`/`xterm`/`xterm-color`/`ansi` compiled directly
into `libncurses.a` (`--with-fallbacks=...`) — this means anything
linked against it works with **no terminfo database needed on the
target rootfs at all**. This project's serial console sets
`TERM=vt100` (`/etc/inittab`: `getty -L ttyS0 115200 vt100`), which is
one of the compiled-in fallbacks.

Used by: `tools/nano`, `tools/less`, `tools/htop`, `tools/tmux` in the
main repo.

Rebuild (source already cached in `../../buildroot-2021.02.2/dl/ncurses/`):

```sh
tar xzf ncurses-6.1.tar.gz && cd ncurses-6.1
./configure --host=arm-linux-gnueabihf --build=x86_64-linux-gnu \
  --prefix=/path/to/arm-static-libs/ncurses-install \
  --without-shared --with-normal --without-debug --without-cxx --without-cxx-binding \
  --without-ada --without-tests --without-manpages --without-progs \
  --disable-db-install --enable-termcap \
  --with-fallbacks=linux,vt100,vt102,xterm,xterm-color,ansi \
  --with-terminfo-dirs=/usr/share/terminfo
make -j$(nproc) && make install
```

## libevent-install

libevent 2.1.12-stable, statically built (`--disable-shared
--disable-openssl` — no TLS needed for local terminal-multiplexer use).

Used by: `tools/tmux`.

```sh
tar xzf libevent-2.1.12-stable.tar.gz && cd libevent-2.1.12-stable
./configure --host=arm-linux-gnueabihf --build=x86_64-linux-gnu \
  --prefix=/path/to/arm-static-libs/libevent-install \
  --disable-shared --enable-static --disable-openssl --disable-samples \
  --disable-libevent-regress
make -j$(nproc) && make install
```

## Requires `arm-linux-gnueabihf-g++`

Building `tmux`/`gdbserver` (C++) needs the cross C++ compiler, which
wasn't installed by default alongside the C-only
`arm-linux-gnueabihf-gcc` already on this system:

```sh
sudo apt-get install -y g++-arm-linux-gnueabihf
```
