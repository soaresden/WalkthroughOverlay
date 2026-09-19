@echo off
rem Double-click: opens the guide2switch window. You can also drag & drop files onto this .bat.
rem Needs Python 3 (pymupdf / pillow are installed automatically the first time).
cd /d "%~dp0"
where py >nul 2>nul && (py -3 guide2switch_gui.py %* ) || (python guide2switch_gui.py %* )
if errorlevel 1 pause
