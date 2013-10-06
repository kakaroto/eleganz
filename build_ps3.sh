#!/bin/bash
#

generate_ps3sfo() {
    title=$1
    appid=$2

    sed -e "s/Test - PSL1GHT/$title/" -e "s/TEST00000/$appid/" $PS3DEV/bin/sfo.xml > ps3sfo.xml || return 1
}

generate_ps3sfo_iso() {
    title=$1
    appid=$2

    sed -e "s/Test - PSL1GHT/$title/" -e "s/TEST00000/$appid/" -e "s/HG/DG/" $PS3DEV/bin/sfo.xml > ps3sfo.xml || return 1
}

make_pkg() {
    appid=$1
    name="Eleganz"
    logo=data/icon.png
    title="Eleganz"
    datadir="/dev_hdd0/game/$appid/USRDIR/"
    contentid="UP0001-$appid-0000000000000000"

    generate_ps3sfo "$title" "$appid" || return 1

    rm -rf pkg
    cp src/$name $name.elf && sprxlinker $name.elf && sprxlinker $name.elf && \
        cp $name.elf $name.dbg.elf && ppu-strip $name.elf && \
        make_fself $name.elf $name.self && \
        mkdir -p pkg/USRDIR && cp $logo pkg/ICON0.PNG && \
        scetool --sce-type=SELF --key-revision=01 --self-auth-id=1010000001000003 --self-vendor-id=01000002 --self-type=NPDRM --self-fw-version=0003005500000000 --self-app-version=0001000000000000 --self-ctrl-flags=4000000000000000000000000000000000000000000000000000000000000002 --self-cap-flags=00000000000000000000000000000000000000000000003B0000000100002000 --np-license-type=FREE --np-app-type=EXEC --np-content-id=$contentid --np-real-fname=EBOOT.BIN --compress-data=TRUE --encrypt $name.elf pkg/USRDIR/EBOOT.BIN && \
        sfo.py --title "$title" --appid "$appid" -f ps3sfo.xml pkg/PARAM.SFO  && \
        make install DESTDIR=`pwd`/temp_install && \
        cp -rf temp_install/$datadir/* pkg/USRDIR/  && \
        rm -rf temp_install && \
        pkg.py --contentid $contentid pkg/ $name.pkg && \
        cp $name.pkg $name.retail.pkg && package_finalize $name.retail.pkg
}
        
make_iso() {
    appid=$1
    name="Eleganz"
    logo=data/icon.png
    title="Eleganz"
    datadir="/dev_hdd0/game/$appid/USRDIR/"

    generate_ps3sfo_iso "$title" "$appid" || return 1

    cp src/$name $name.elf && sprxlinker $name.elf && sprxlinker $name.elf && \
        cp $name.elf $name.dbg.elf && ppu-strip $name.elf && \
        make_fself $name.elf $name.self && \
        mkdir -p iso/PS3_GAME/USRDIR && cp $logo iso/PS3_GAME/ICON0.PNG && \
        scetool --sce-type SELF --compress-data FALSE --self-type APP --key-revision 0004 --self-fw-version 0003004100000000 --self-app-version 0001000000000000 --self-auth-id 1010000001000003 --self-vendor-id  01000002 --self-cap-flags 00000000000000000000000000000000000000000000003b0000000100040000 -e $name.elf iso/PS3_GAME/USRDIR/EBOOT.BIN  && \
        sfo.py --title "$title" --appid "$appid" -f ps3sfo.xml iso/PS3_GAME/PARAM.SFO  && \
        make install DESTDIR=`pwd`/temp_install && \
        cp -rf temp_install/$datadir/* iso/PS3_GAME/USRDIR/  && \
        rm -rf temp_install
}

run_configure() {
    appid=$1
    extra_options=$2
    if test -z $NOCONFIGURE ; then
        AR="powerpc64-ps3-elf-ar" CC="powerpc64-ps3-elf-gcc" RANLIB="powerpc64-ps3-elf-ranlib" CFLAGS="$DEBUG_CFLAGS -Wall -I$PSL1GHT/ppu/include -I$PS3DEV/portlibs/ppu/include $MINIMAL_TOC $MYCFLAGS" CPPFLAGS="-I$PSL1GHT/ppu/include -I$PS3DEV/portlibs/ppu/include" CXXFLAGS="-I$PSL1GHT/ppu/include -I$PS3DEV/portlibs/ppu/include"  LDFLAGS="-L$PSL1GHT/ppu/lib -L$PS3DEV/portlibs/ppu/lib" PKG_CONFIG_LIBDIR="$PSL1GHT/ppu/lib/pkgconfig" PKG_CONFIG_PATH="$PS3DEV/portlibs/ppu/lib/pkgconfig"  PKG_CONFIG="pkg-config --static" ./configure   --prefix="$PS3DEV/portlibs/ppu"   --host=powerpc64-ps3-elf    --includedir="$PS3DEV/portlibs/ppu/include"   --libdir="$PS3DEV/portlibs/ppu/lib" --datadir="/dev_hdd0/game/$appid/USRDIR/data" $extra_options
    fi
}

type=$1
if [ -z "$type" ]; then
    run_configure "ELEGANZ00" && make clean all EDJE_CC=$(which edje_cc) && make_pkg "ELEGANZ00"
    LIBS="-lsysfs" run_configure "BCES00908" --enable-cobra-ode && make clean all EDJE_CC=$(which edje_cc) && make_iso "BCES00908"
elif [ "$type" = "pkg" ]; then
    run_configure "ELEGANZ00" && make clean all EDJE_CC=$(which edje_cc) && make_pkg "ELEGANZ00"
elif [ "$type" = "iso" ]; then
    LIBS="-lsysfs" run_configure "BCES00908" --enable-cobra-ode && make clean all EDJE_CC=$(which edje_cc) && make_iso "BCES00908"
fi

