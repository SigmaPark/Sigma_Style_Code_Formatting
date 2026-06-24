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

rem === run sak on app/ ===
echo.
echo === sak app ===
build\sak.exe app

endlocal

pause