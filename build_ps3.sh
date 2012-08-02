#!/bin/bash
#

generate_ps3sfo() {
    title=$1
    appid=$2

    sed -e "s/Test - PSL1GHT/$title/" -e "s/TEST00000/$appid/" $PS3DEV/bin/sfo.xml > ps3sfo.xml || return 1
}

make_pkg() {
    name="Eleganz"
    logo=data/themes/dark/images/homebrew.png 
    title="Eleganz"
    appid="ELEGANZ00"
    datadir="/dev_hdd0/game/$appid/USRDIR/"
    contentid="UP0001-$appid-0000000000000000"

    generate_ps3sfo "$title" "$appid" || return 1

    cp src/$name $name.elf && sprxlinker $name.elf && sprxlinker $name.elf && \
        cp $name.elf $name.dbg.elf && ppu-strip $name.elf && \
        make_fself $name.elf $name.self && \
        mkdir -p pkg/USRDIR && cp $logo pkg/ICON0.PNG && \
        make_self_npdrm $name.elf pkg/USRDIR/EBOOT.BIN $contentid && \
        sfo.py --title "$title" --appid "$appid" -f ps3sfo.xml pkg/PARAM.SFO  && \
        make install DESTDIR=`pwd`/temp_install && \
        cp -rf temp_install/$datadir/* pkg/USRDIR/  && \
        rm -rf temp_install && \
        pkg.py --contentid $contentid pkg/ $name.pkg && \
        cp $name.pkg $name.retail.pkg && package_finalize $name.retail.pkg && \
        rm -rf pkg
}
        
if test -z $NOCONFIGURE ; then
  AR="powerpc64-ps3-elf-ar" CC="powerpc64-ps3-elf-gcc" RANLIB="powerpc64-ps3-elf-ranlib" CFLAGS="$DEBUG_CFLAGS -Wall -I$PSL1GHT/ppu/include -I$PS3DEV/portlibs/ppu/include $MINIMAL_TOC $MYCFLAGS" CPPFLAGS="-I$PSL1GHT/ppu/include -I$PS3DEV/portlibs/ppu/include" CXXFLAGS="-I$PSL1GHT/ppu/include -I$PS3DEV/portlibs/ppu/include"  LDFLAGS="-L$PSL1GHT/ppu/lib -L$PS3DEV/portlibs/ppu/lib" PKG_CONFIG_LIBDIR="$PSL1GHT/ppu/lib/pkgconfig" PKG_CONFIG_PATH="$PS3DEV/portlibs/ppu/lib/pkgconfig"  PKG_CONFIG="pkg-config --static" ./configure   --prefix="$PS3DEV/portlibs/ppu"   --host=powerpc64-ps3-elf    --includedir="$PS3DEV/portlibs/ppu/include"   --libdir="$PS3DEV/portlibs/ppu/lib" --datadir="/dev_hdd0/game/ELEGANZ00/USRDIR/data"
fi 

make clean all EDJE_CC=$(which edje_cc) && make_pkg
