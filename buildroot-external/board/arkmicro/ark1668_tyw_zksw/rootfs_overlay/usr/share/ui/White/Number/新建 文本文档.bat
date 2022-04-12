@echo off
setlocal enabledelayedexpansion
set "str=_Black"
for /f "delims=" %%i in ('dir /s/b/ *.png') do (set "var=%%i" & ren "%%i" "!var:%str%=!")