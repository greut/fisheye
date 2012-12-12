@echo off
"%VS80COMNTOOLS%\vsvars32.bat"

cl /EHsc /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /O0 /MT /I "..\libs" fisheye.cc