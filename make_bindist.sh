#!/usr/bin/env bash
DISTDIR=dist/dune3d
rm -rf dist
mkdir -p $DISTDIR
cp build/dune3d.exe $DISTDIR
strip $DISTDIR/dune3d.exe
LIBS=(
	libwinpthread-1.dll\
	libstdc++-6.dll\
	libcairo-2.dll\
	libcairomm-1.16-1.dll\
	libepoxy-0.dll\
	libgio-2.0-0.dll\
	libgiomm-2.68-1.dll\
	libglib-2.0-0.dll\
	libglibmm-2.68-1.dll\
	libgobject-2.0-0.dll\
	libgtk-4-1.dll\
	libgtkmm-4.0-0.dll\
	libpango-1.0-0.dll\
	libpangomm-2.48-1.dll\
	libsigc-3.0-0.dll\
	libTKBO.dll\
	libTKBRep.dll\
	libTKCDF.dll\
	libTKernel.dll\
	libTKG3d.dll\
	libTKGeomBase.dll\
	libTKHLR.dll\
	libTKFillet.dll\
	libTKMath.dll\
	libTKLCAF.dll\
	libTKMesh.dll\
	libTKPrim.dll\
	libTKShHealing.dll\
	libTKXCAF.dll\
	libTKTopAlgo.dll\
	libTKXSBase.dll\
	libTKOffset.dll\
	libTKDEIGES.dll\
	libTKDESTEP.dll\
	libTKDESTL.dll\
	libTKDE.dll\
	libgcc_s_seh-1.dll\
	libfreetype-6.dll\
	libfontconfig-1.dll\
	libpixman-1-0.dll\
	zlib1.dll\
	libpng16-16.dll\
	libintl-8.dll\
	libpcre2-8-0.dll\
	libgmodule-2.0-0.dll\
	libffi-8.dll\
	libcairo-gobject-2.dll\
	libgdk_pixbuf-2.0-0.dll\
	libgraphene-1.0-0.dll\
	libharfbuzz-0.dll\
	libharfbuzz-subset-0.dll\
	libthai-0.dll\
	libfribidi-0.dll\
	libpangocairo-1.0-0.dll\
	libcairo-script-interpreter-2.dll\
	libjpeg-8.dll\
	libpangowin32-1.0-0.dll\
	libtiff-6.dll\
	libTKG2d.dll\
	libTKGeomAlgo.dll\
	libtbb12.dll\
	libtbbmalloc.dll\
	libTKBool.dll\
	libTKCAF.dll\
	libTKService.dll\
	libTKVCAF.dll\
	libTKV3d.dll\
	libbz2-1.dll\
	libbrotlidec.dll\
	libexpat-1.dll\
	libiconv-2.dll\
	libgraphite2.dll\
	libdatrie-1.dll\
	libpangoft2-1.0-0.dll\
	liblzo2-2.dll\
	libdeflate.dll\
	libjbig-0.dll\
	liblzma-5.dll\
	libwebp-7.dll\
	libzstd.dll\
	libLerc.dll\
	libbrotlicommon.dll\
	libsharpyuv-0.dll\
	libxml2-2.dll\
	librsvg-2-2.dll\
	vulkan-1.dll\
	gspawn-win64-helper.exe\
	gspawn-win64-helper-console.exe
)
for LIB in "${LIBS[@]}"
do
   cp /mingw64/bin/$LIB $DISTDIR
done

mkdir -p $DISTDIR/share/icons
cp -r /mingw64/share/icons/Adwaita $DISTDIR/share/icons
cp -r /mingw64/share/icons/hicolor $DISTDIR/share/icons
rm -rf $DISTDIR/share/icons/Adwaita/cursors

mkdir -p $DISTDIR/lib
cp -r /mingw64/lib/gdk-pixbuf-2.0 $DISTDIR/lib
gdk-pixbuf-query-loaders > $DISTDIR/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache
rm $DISTDIR/lib/gdk-pixbuf-*/*/loaders/*.a

mkdir -p $DISTDIR/share/glib-2.0/schemas
cp /mingw64/share/glib-2.0/schemas/gschemas.compiled $DISTDIR/share/glib-2.0/schemas

git log -10 | unix2dos > dist/log.txt
if [ "$1" != "-n" ]; then
	cd dist
	zip -r dune3d-$(date +%Y-%m-%d-%H%M).zip dune3d log.txt
fi
