@echo off
setlocal enabledelayedexpansion
set "str=_White"
for /f "delims=" %%i in ('dir /b *.png') do (set "var=%%i" & ren "%%i" "!var:%str%=!")