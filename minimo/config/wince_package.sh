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

cp -a bin/chrome/minimo-skin.jar                         minimo/chrome
cp -a bin/chrome/minimo-skin.manifest                    minimo/chrome

cp -a bin/chrome/minimo-skin-vga.jar                     minimo/chrome
cp -a bin/chrome/minimo-skin-vga.manifest                minimo/chrome

cp -a bin/chrome/toolkit.jar                             minimo/chrome
cp -a bin/chrome/toolkit.manifest                        minimo/chrome

mkdir -p minimo/components

cp -a bin/extensions/spatial-navigation@extensions.mozilla.org/components/* minimo/components

mkdir -p minimo/greprefs
cp -a bin/greprefs/*                                     minimo/greprefs

mkdir -p minimo/res
cp -a bin/res/*                                          minimo/res
rm -rf minimo/res/samples
rm -rf minimo/res/throbber

mkdir -p minimo/plugins

echo Linking XPT files.

host/bin/host_xpt_link minimo/components/all.xpt          bin/components/*.xpt



echo Chewing on chrome

cd minimo/chrome

unzip toolkit.jar
rm -rf toolkit.jar
#perl $SRCDIR/../config/liceater.pl `find .`
zip -r -9 toolkit.jar content
rm -rf content

unzip en-US.jar
rm -rf en-US.jar
#perl $SRCDIR/../config/liceater.pl `find .`
zip -r -9 en-US.jar content
rm -rf content

unzip classic.jar
rm -rf classic.jar
#perl $SRCDIR/../config/liceater.pl `find .`
zip -r -9 classic.jar skin
rm -rf skin

unzip en-US.jar
rm -rf en-US.jar
#perl $SRCDIR/../config/liceater.pl `find .`
zip -r -9 en-US.jar locale
rm -rf locale

unzip minimo.jar
rm -rf minimo.jar
#perl $SRCDIR/../config/liceater.pl `find .`
zip -r -9 minimo.jar skin locale branding content
rm -rf skin locale branding content

unzip minimo-skin.jar
rm -rf minimo-skin.jar
#perl $SRCDIR/../config/liceater.pl `find .`
zip -r -9 minimo-skin.jar skin locale branding content
rm -rf skin locale branding content

unzip minimo-skin-vga.jar
rm -rf minimo-skin-vga.jar
#perl $SRCDIR/../config/liceater.pl `find .`
zip -r -9 minimo-skin-vga.jar skin locale branding content
rm -rf skin locale branding content

popd

pushd $SRCDIR

echo Copying over customized files

cp -a ../customization/all.js                             $OBJDIR/dist/minimo/greprefs
cp -a ../customization/HelperAppDlg.js                    $OBJDIR/dist/minimo/components

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
