#!/bin/bash

if [ $# -ne 2 ]
then
  echo "Usage: `basename $0` OBJDIR SRCDIR"
  exit $E_BADARGS 
fi

OBJDIR=$1
SRCDIR=$2

echo ---------------------------------------------------
echo OBJDIR = $OBJDIR
echo SRCDIR = $SRCDIR
echo ---------------------------------------------------

pushd $OBJDIR/dist
rm -rf minimo
rm -f minimo.zip

echo Copying over files from OBJDIR

mkdir minimo
cp -a bin/minimo.exe                                     minimo
cp -a bin/minimo_runner.exe                              minimo

mkdir -p minimo/chrome

cp -a bin/chrome/classic.jar                             minimo/chrome
cp -a bin/chrome/classic.manifest                        minimo/chrome

cp -a bin/chrome/en-US.jar                               minimo/chrome
cp -a bin/chrome/en-US.manifest                          minimo/chrome

cp -a bin/chrome/minimo.jar                              minimo/chrome
cp -a bin/chrome/minimo.manifest                         minimo/chrome

cp -a bin/chrome/toolkit.jar                             minimo/chrome
cp -a bin/chrome/toolkit.manifest                        minimo/chrome

#echo Copying over some minimo extensions
#
#cp -a bin/chrome/history.jar                            minimo/chrome
#cp -a bin/chrome/history.manifest                       minimo/chrome
#
#cp -a bin/chrome/map.jar                                minimo/chrome
#cp -a bin/chrome/map.manifest                           minimo/chrome
#
#cp -a bin/chrome/mobile.jar                             minimo/chrome
#cp -a bin/chrome/mobile.manifest                        minimo/chrome
#
#cp -a bin/chrome/shopping.jar                           minimo/chrome
#cp -a bin/chrome/shopping.manifest                      minimo/chrome
#
#cp -a bin/chrome/technorati.jar                         minimo/chrome
#cp -a bin/chrome/technorati.manifest                    minimo/chrome
#
#cp -a bin/chrome/update.jar                             minimo/chrome
#cp -a bin/chrome/update.manifest                        minimo/chrome
#
#cp -a bin/chrome/wunderground.jar                       minimo/chrome
#cp -a bin/chrome/wunderground.manifest                  minimo/chrome
#
#cp -a bin/chrome/yokel.jar                              minimo/chrome
#cp -a bin/chrome/yokel.manifest                         minimo/chrome


mkdir -p minimo/components

cp -a bin/extensions/spatial-navigation@extensions.mozilla.org/components/* minimo/components

mkdir -p minimo/greprefs
cp -a bin/greprefs/*                                     minimo/greprefs

mkdir -p minimo/res
cp -a bin/res/*                                          minimo/res
rm -rf minimo/res/samples
rm -rf minimo/res/throbber
rm -rf minimo/res/viewer.properties
rm -rf minimo/res/viewsource.css
rm -rf minimo/res/hiddenWindow.html
rm -rf minimo/res/bloatcycle.html


mkdir -p minimo/plugins

echo Linking XPT files.

host/bin/host_xpt_link minimo/components/all.xpt          bin/components/*.xpt



echo Chewing on chrome

cd minimo/chrome

cat *.manifest > temp
rm *.manifest
sed  -e 's/jar:.*.jar/jar:minimo.jar/' temp > minimo.manifest
rm temp

ls -a *.jar > files
sed  -e 's/^/unzip /' files > run
rm files
chmod +x run 
./run
rm run
rm -rf *.jar
zip -r -0 minimo.jar skin locale branding content
rm -rf skin locale branding content

cd ..
cd ..

cp -a bin/chrome/minimo-skin.jar                         minimo/chrome
cp -a bin/chrome/minimo-skin.manifest                    minimo/chrome

cp -a bin/chrome/minimo-skin-vga.jar                     minimo/chrome
cp -a bin/chrome/minimo-skin-vga.manifest                minimo/chrome


popd

pushd $SRCDIR

echo Copying over customized files

cp -a ../customization/all.js                             $OBJDIR/dist/minimo/greprefs
cp -a ../customization/HelperAppDlg.js                    $OBJDIR/dist/minimo/components
cp -a ../customization/start.html                         $OBJDIR/dist/minimo/res

cat ../customization/ua.css.additions >> $OBJDIR/dist/minimo/res/ua.css

popd




#pushd $OBJDIR/dist/minimo
#
# echo Zipping
#
# zip -r $OBJDIR/dist/minimo.zip $OBJDIR/dist/minimo
#
#popd

echo Done.
