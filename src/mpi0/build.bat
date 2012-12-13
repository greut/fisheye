@echo off
%comspec% /c ""%VS80COMNTOOLS%\vsvars32.bat" && cl.exe /EHsc /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /MT /I "C:\Program Files\MPICH2\include" /I "..\libs" /c fisheye.cc ..\libs\lspbmp.c ..\libs\magnify.c"
%comspec% /c ""%VS80COMNTOOLS%\vsvars32.bat" && link.exe mpi.lib /nologo /subsystem:console /libpath:"C:\Program Files\MPICH2\lib" /out:fisheye.exe fisheye.obj magnify.obj lspbmp.obj"