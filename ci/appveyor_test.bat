:: build
setlocal

set qtplatform=%PLATFORM%
set PATH=C:\Qt\%QT_VER%\%qtplatform%\bin;%PATH%;%CD%\build-%qtplatform%\qpmx;

mkdir build-%qtplatform%/tests
cd build-%qtplatform%/tests

C:\Qt\%QT_VER%\%qtplatform%\bin\qmake -r ../../submodules/qpmx-sample-package/qpmx-test/ || exit /B 1
nmake || exit /B 1

.\test.exe || exit /B 1