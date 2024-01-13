
@echo build files.wxs
python files.py
@if NOT %ERRORLEVEL% == 0 goto theend

@echo create version information
for /f "tokens=*" %%a in ('python wix_version.py') do (set DUNE3D_VER=%%a)
@if NOT %ERRORLEVEL% == 0 goto theend

@echo call wix compiler ...
candle dune3d.wxs -ext WiXUtilExtension
@if NOT %ERRORLEVEL% == 0 goto theend

candle files.wxs
@if NOT %ERRORLEVEL% == 0 goto theend

@echo call wix linker ...
light -ext WixUIExtension -ext WiXUtilExtension dune3d.wixobj files.wixobj -o dune3d-%DUNE3D_VER%.msi
@if NOT %ERRORLEVEL% == 0 goto theend

@echo the installer is now created
@rem uncomment following line if you want to test the installer
goto theend

@echo install ...
msiexec /i dune3d-%DUNE3D_VER%.msi /l*v dune3d.log

pause the program is now installed. press any key to run uninstaller ...
@echo deinstall ...
msiexec /x dune3d-%DUNE3D_VER%.msi


:theend
@echo ... finished