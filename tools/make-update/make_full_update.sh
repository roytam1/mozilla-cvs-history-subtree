#!/bin/bash
#
# This tool generates full update packages for the update system.
# Author: Darin Fisher
#

# -----------------------------------------------------------------------------
# By default just assume that these tools exist on our path
MAR=${MAR:-mar}
BZIP2=${BZIP2:-bzip2}

# -----------------------------------------------------------------------------

print_usage() {
  echo "Usage: $(basename $0) [OPTIONS] ARCHIVE DIRECTORY"
}

if [ $# = 0 ]; then
  print_usage
  exit 1
fi

if [ $1 = -h ]; then
  print_usage
  echo ""
  echo "The contents of DIRECTORY will be stored in ARCHIVE."
  echo ""
  echo "Options:"
  echo "  -h  show this help text"
  echo ""
  exit 1
fi

# -----------------------------------------------------------------------------

archive="$1"
targetdir="$2"
workdir="$targetdir.work"
manifest="$workdir/update.manifest"

# Generate a list of all files in the target directory.
targetfiles=$(cd "$targetdir" && find . -type f | cut -d'/' -f2-)

mkdir -p "$workdir"
> $manifest

for f in $targetfiles; do
  echo "  processing $f"

  echo "add $f" >> $manifest

  dir=$(dirname $f)
  mkdir -p "$workdir/$dir"
  $BZIP2 -cz9 "$targetdir/$f" > "$workdir/$f"
  chmod --reference="$targetdir/$f" "$workdir/$f"
done

$BZIP2 -z9 "$manifest" && mv -f "$manifest.bz2" "$manifest"

(cd "$workdir" && $MAR -c output.mar update.manifest $targetfiles)
mv -f "$workdir/output.mar" "$archive"

# cleanup
rm -fr "$workdir"
