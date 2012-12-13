@echo off
%comspec% /c ""%VS80COMNTOOLS%\vsvars32.bat" && cl.exe /EHsc /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /MT /I "..\libs" fisheye.cc ..\libs\lspbmp.c ..\libs\magnify.c"