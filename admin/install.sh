#! /bin/bash
# Run in dist/bin/

LD_LIBRARY_PATH=.
MOZILLA_FIVE_HOME=.
echo "skin,install,select,modern/1.0" >> chrome/installed-chrome.txt
./regxpcom
./regchrome
# user-locales must be empty (but exist),
# or the ProfileSelector will break. See bug 92898.
rm -f chrome/user-locales.rdf
touch chrome/user-locales.rdf
