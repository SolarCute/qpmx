:: build
setlocal

set qtplatform=%PLATFORM%
set PATH=C:\projects\;C:\Qt\%QT_VER%\%qtplatform%\bin;%PATH%;%CD%\build-%qtplatform%\qpmx\release;

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 || exit /B 1

:: install plugins into qt
mkdir C:\Qt\%QT_VER%\%qtplatform%\plugins\qpmx || exit /B 1
xcopy /s build-%qtplatform%\plugins\qpmx C:\Qt\%QT_VER%\%qtplatform%\plugins\qpmx || exit /B 1

:: build tests (bin and src)
@echo on
for /L %%i IN (0, 1, 1) DO (
	if "%%i" == "1" (
		del submodules\qpmx-sample-package\qpmx-test\qpmx.json
		rename submodules\qpmx-sample-package\qpmx-test\qpmx.json.src submodules\qpmx-sample-package\qpmx-test\qpmx.json
	)

	mkdir build-%qtplatform%\tests-%%i
	cd build-%qtplatform%\tests-%%i

	C:\Qt\%QT_VER%\%qtplatform%\bin\qmake -r "CONFIG += debug_and_release" ../../submodules/qpmx-sample-package/qpmx-test/ || (
		for /D %%G in (C:\Users\appveyor\AppData\Local\Temp\1\qpmx*) do (
			type %%G\qmake.stdout.log
			type %%G\qmake.stderr.log
			type %%G\make.stdout.log
			type %%G\make.stderr.log
			type %%G\install.stdout.log
			type %%G\install.stderr.log
		)
		exit /B 1
	)

	nmake all || exit /B 1

	.\release\test.exe || exit /B 1
	.\debug\test.exe || exit /B 1
	
	cd ..\..
)

:: debug
build-%qtplatform%\qpmx\qtifw-installer\packages\de.skycoder42.qpmx\data\qpmx.exe list providers
