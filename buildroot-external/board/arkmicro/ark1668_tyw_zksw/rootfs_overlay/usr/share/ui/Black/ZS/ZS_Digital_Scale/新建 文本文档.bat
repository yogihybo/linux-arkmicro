@echo off
setlocal enabledelayedexpansion
set "str=_x690_y106"
for /f "delims=" %%i in ('dir /b *.png') do (set "var=%%i" & ren "%%i" "!var:%str%=!")