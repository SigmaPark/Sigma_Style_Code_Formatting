@echo off
setlocal

rem === configure / build ===
if not exist build mkdir build
cd build
cmake ..
if errorlevel 1 ( cd .. & exit /b 1 )
cmake --build .
if errorlevel 1 ( cd .. & exit /b 1 )
cd ..

rem === self-check: run sak on app/ recursively ===
echo.
echo === sak -r app ===
build\sak.exe -r app

endlocal
pause