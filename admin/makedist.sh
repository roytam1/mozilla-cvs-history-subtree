#! /bin/bash
# One command that does everything to create a distributable file.
# Does not work flawlessly, better do the steps by hand.

# Make Beonex Communicator out of Mozilla
zip/patch.sh

# Build
./configure
#  hack to avoid dynamical linking to libstdc++ (start)
mkdir -p dist/bin/components
mkdir -p dist/bin/plugins
ln -s `gcc -print-file-name=libstdc++.a` dist/bin/
ln -s `gcc -print-file-name=libstdc++.a` dist/bin/components
ln -s `gcc -print-file-name=libstdc++.a` dist/bin/plugins
#  (end)
zip/build.sh

# Create necessary runtime files
cd dist/bin/
../../zip/install.sh

# Create package file
cd ../../xpinstall/packager
MOZ_PKG_APPNAME=beonex-comm
MOZ_PKG_FORMAT=BZ2
MOZ_PKG_DEST=/tmp/beonex-comm
make dist
ls /tmp/beonex-comm
cd ../../

echo "Done."
