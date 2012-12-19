@echo off
cd src\serial4
call build.bat
cd ..\..
del data\serial4_win.tsv
src\serial4\fisheye.exe img\100x100@24.bmp _.bmp 1>> data\serial4_win.tsv
del _.bmp
src\serial4\fisheye.exe img\300x300@24.bmp _.bmp 1>> data\serial4_win.tsv
del _.bmp
src\serial4\fisheye.exe img\1000x1000@24.bmp _.bmp 1>> data\serial4_win.tsv
del _.bmp
src\serial4\fisheye.exe img\2000x2000@24.bmp _.bmp 1>> data\serial4_win.tsv
del _.bmp
src\serial4\fisheye.exe img\4000x4000@24.bmp _.bmp 1>> data\serial4_win.tsv
del _.bmp
src\serial4\fisheye.exe img\8000x8000@24.bmp _.bmp 1>> data\serial4_win.tsv
del _.bmp

call measure.bat 0 img\100x100@24.bmp true
call measure.bat 0 img\300x300@24.bmp
call measure.bat 0 img\1000x1000@24.bmp
call measure.bat 0 img\2000x2000@24.bmp
call measure.bat 0 img\4000x4000@24.bmp
call measure.bat 0 img\8000x8000@24.bmp

call measure.bat 1 img\100x100@24.bmp true
call measure.bat 1 img\300x300@24.bmp
call measure.bat 1 img\1000x1000@24.bmp
call measure.bat 1 img\2000x2000@24.bmp
call measure.bat 1 img\4000x4000@24.bmp
call measure.bat 1 img\8000x8000@24.bmp