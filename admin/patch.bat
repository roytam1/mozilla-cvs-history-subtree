:: copy binary files
::  artwork
copy zip\artwork\logo.gif xpfe\global\resources\content\
copy zip\artwork\splash.bmp xpfe\bootstrap\
copy zip\artwork\beonex.ico xpfe\bootstrap\
copy zip\artwork\beonex.ico xpinstall\wizard\windows\nsinstall\
copy zip\artwork\beonex.ico xpinstall\wizard\windows\uninstall\
copy zip\artwork\beonex.ico xpinstall\wizard\windows\setuprsc\
copy zip\artwork\dialogsLogo.bmp xpinstall\wizard\windows\setuprsc\
copy zip\artwork\downloadLogo.bmp xpinstall\wizard\windows\setuprsc\
::  throbber
copy zip\artwork\b.gif themes\modern\communicator\brand\throbber-single.gif
copy zip\artwork\throbber.gif themes\modern\communicator\brand\throbber-anim.gif
copy zip\artwork\b.gif themes\modern\communicator\brand\throbber25-single.gif
copy zip\artwork\throbber.gif themes\modern\communicator\brand\throbber25-anim.gif
copy zip\artwork\b.gif themes\classic\global\animthrob_single.gif
copy zip\artwork\throbber.gif themes\classic\global\animthrob.gif

:: Work around dumb patch rule, which prefers a file in the current dir
move Makefile.in Makefile.in.beonex.save
move makefile.win makefile.win.beonex.save

:: Actually apply patch
set POSIXLY_CORRECT=1
patch -p 0 --forward < zip\beonex-comm-*.diff

:: Restore
move Makefile.in.beonex.save Makefile.in
move makefile.win.beonex.save makefile.win

:: Finally patch the makefiles we skipped above
patch --forward < zip\top-makefiles.diff


:: New/complete files. Copy them over existing ones (in Mozilla or patch).
echo "Important: You have to copy the content of othernew\ to the source tree."
echo "I don't know the appropriate DOS command for that (probably copy with
echo "some flag)."
