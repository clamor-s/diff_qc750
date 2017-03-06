@echo off
setlocal

rem
rem Copyright (c) 2005-2008 Trusted Logic S.A.
rem All Rights Reserved.
rem
rem This software is the confidential and proprietary information of
rem Trusted Logic S.A. ("Confidential Information"). You shall not
rem disclose such Confidential Information and shall use it only in
rem accordance with the terms of the license agreement you entered
rem into with Trusted Logic S.A.
rem
rem TRUSTED LOGIC S.A. MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
rem SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
rem BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
rem FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. TRUSTED LOGIC S.A. SHALL
rem NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
rem MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
rem

set SELF_DIR=%~dp0

rem
rem Search for Python version 2.5
rem

set PYTHONTESTDIR=%SELF_DIR%
:search_loop
set PYTHONHOME=%PYTHONTESTDIR%\python\ix86_win32
if exist %PYTHONHOME%\python25.dll goto found_python
set PYTHONHOME=%PYTHONTESTDIR%\externals\python\ix86_win32
if exist %PYTHONHOME%\python25.dll goto found_python
set PYTHONHOME=%PYTHONTESTDIR%\unsupported\python\ix86_win32
if exist %PYTHONHOME%\python25.dll goto found_python

call :go_up %PYTHONTESTDIR%\..

if defined PYTHONTESTDIR goto search_loop

echo Couldn't find a python25 interpreter
goto :EOF

:found_python

set PYTHONTESTDIR=
set PYTHONPATH=%PYTHONHOME%/../lib/python25.zip
%PYTHONHOME%\python.exe %SELF_DIR%tf_client_sign.py %*
      
goto :EOF      

:go_up
if %PYTHONTESTDIR% EQU %~f1 (
   set PYTHONTESTDIR=
) else (
   set PYTHONTESTDIR=%~f1
)
