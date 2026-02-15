@echo off
setlocal

if "%~1"=="" (
  echo Usage: sign.bat "path\to\installer.exe"
  exit /b 1
)

if "%AZURE_SIGNING_DLIB%"=="" (
  echo AZURE_SIGNING_DLIB not set - skipping signing.
  exit /b 0
)
if "%AZURE_METADATA_JSON%"=="" (
  echo AZURE_METADATA_JSON not set - skipping signing.
  exit /b 0
)

signtool.exe sign /v /fd SHA256 /tr http://timestamp.acs.microsoft.com /td SHA256 ^
    /dlib "%AZURE_SIGNING_DLIB%" ^
    /dmdf "%AZURE_METADATA_JSON%" ^
    "%~1"

if errorlevel 1 exit /b %errorlevel%
endlocal
