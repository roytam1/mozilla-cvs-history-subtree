# vim:set ts=8 sw=8 sts=8 noet cin:

mozicoh.dll: mozicoh.obj mozicoh.def Makefile
	link /nologo /dll /out:$@ /pdb:mozicoh.pdb /subsystem:windows /def:mozicoh.def $< advapi32.lib

mozicoh.obj: mozicoh.cpp Makefile
	cl /nologo /TP /W3 /Gy /Fdmozicoh.pdb /DDEBUG /D_DEBUG /Zi /c /MDd $<
