#! /bin/bash

# Copy binary files and search plugins to the right location
cp zip/.mozconfig .
#  artwork
cp zip/artwork/logo.gif xpfe/global/resources/content/
POSIXLY_CORRECT=1 patch -p 0 --forward < zip/artwork/unix.diff
#  throbber
cp zip/artwork/b.gif themes/modern/communicator/brand/throbber-single.gif
cp zip/artwork/throbber.gif themes/modern/communicator/brand/throbber-anim.gif
cp zip/artwork/b.gif themes/modern/communicator/brand/throbber25-single.gif
cp zip/artwork/throbber.gif themes/modern/communicator/brand/throbber25-anim.gif
cp zip/artwork/b.gif themes/classic/communicator/brand/throbber-single.gif
cp zip/artwork/throbber.gif themes/classic/communicator/brand/throbber-anim.gif

cp zip/artwork/b.gif themes/classic/communicator/brand/throbber25-single.gif
cp zip/artwork/throbber.gif themes/classic/communicator/brand/throbber25-anim.gif
cp zip/artwork/b.gif themes/classic/global/animthrob_single.gif
cp zip/artwork/throbber.gif themes/classic/global/animthrob.gif

mkdir README/user/
mkdir xpfe/bootstrap/icons/
mkdir -p extensions/spellcheck/myspell/dictionaries/
mkdir extensions/spellcheck/myspell/src/
mkdir extensions/spellcheck/idl/
mkdir extensions/spellcheck/src/
#NEWFILES=" \
#  .mozconfig \
#  xpfe/communicator/resources/locale/en-US/welcome.html \
#  ... \
#  "
#touch $NEWFILES
##cvs diff -N is a pain (cvs add, empty files and so on), so just copy (below)

# Work around dumb patch rule, which prefers a file in the current dir
mv Makefile.in Makefile.in.beonex.save
mv makefile.win makefile.win.beonex.save

# Actually apply main patch
POSIXLY_CORRECT=1 patch --forward -p 0 < zip/beonex-comm-p41.diff
#> patch-main.out

# Restore
mv Makefile.in.beonex.save Makefile.in
mv makefile.win.beonex.save makefile.win

#temp hack
(cd zip/othernew/; find . -type f| xargs -n 1 -iFILE cp FILE ../../FILE)

# Finally patch the makefiles we skipped above
#  not needed atm
patch --forward < zip/top-makefiles.diff

#New/complete files. Copy them over existing ones (in Mozilla or patch).
(cd zip/othernew/; find . -type f| xargs -n 1 -iFILE cp FILE ../../FILE)

#Inform about fails
#echo "I found at least the following patch merge fails:"
#echo "(If none are listed, all probably went OK. Full output in patch-main.out.)"
#grep FAIL patch-main.out
#echo ""
