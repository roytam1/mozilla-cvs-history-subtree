#! /bin/bash
# execute from source dir
# One command that does everything to create a distributable file.
# Might not work flawlessly, better do the steps by hand.

#make obj dir
cd ..
mkdir bin
cd bin

# Build
../mozilla/configure
#  hack to avoid dynamical linking to libstdc++ (start)
mkdir -p dist/bin/components
mkdir -p dist/bin/plugins
ln -s `gcc -print-file-name=libstdc++.a` dist/bin/
ln -s `gcc -print-file-name=libstdc++.a` dist/bin/components
ln -s `gcc -print-file-name=libstdc++.a` dist/bin/plugins
#  (end)
make -s

# Create necessary runtime files
cd dist/bin/
echo "skin,install,select,modern/1.0" >> chrome/installed-chrome.txt

# Create package file
cd ../../xpinstall/packager
MOZ_PKG_APPNAME=beonex-comm
MOZ_PKG_FORMAT=BZ2
MOZ_PKG_DEST=/tmp/beonex-comm
make dist
ls /tmp/beonex-comm
cd ../../

echo "Done."
