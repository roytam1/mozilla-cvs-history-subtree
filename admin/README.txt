Unix

1. Extract the zipfile in the top-level source dir, i.e. mozilla/, i.e. the dir that contains xpfe/, mailnews/ etc..
2. Make zip/patch.sh executable, e.g. with |chmod 700 zip/patch.sh|
3. Run patch.sh from the top-level source dir, e.g. with |zip/patch.sh|
4. Double-check that
   .mozconfig,
   xpfe/communicator/resources/locale/en-US/welcome.html
   and the image files are fine.

makedist.sh shows, which steps load to the build. In an ideal situation,
you can run it, but you should better execute the steps manually.
Also look 

Win32

Similar to Unix, but s/.sh/.bat/
The build environment options are in file build.win32.env. You will have to adjust system-specifics like |Path|.
