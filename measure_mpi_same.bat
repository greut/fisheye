SET MPI=%1
SET N=%2
SET IMG=%3
SET TEMP=_.bmp
SET FISHEYE=src\mpi%MPI%
SET TSV=data\mpi%MPI%-same-%N%.tsv
cd %FISHEYE%
call build.bat
cd ..\..
IF "%4"=="true" del %TSV%
mpiexec -mapall -n %N% %FISHEYE%\fisheye.exe %IMG% %TEMP% 2>> %TSV%
del %TEMP%