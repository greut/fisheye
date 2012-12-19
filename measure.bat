SET MPI=%1
SET IMG=%2
IF "%N%"=="" SET N=5
SET TEMP=_.bmp
SET FISHEYE=src\mpi%MPI%
SET TSV=data\mpi%MPI%.tsv
cd %FISHEYE%
call build.bat
cd ..\..
IF "%3"=="true" del %TSV%
mpiexec -mapall -machinefile machines.txt -n %N% %FISHEYE%\fisheye.exe %IMG% %TEMP% 2>> %TSV%
del %TEMP%