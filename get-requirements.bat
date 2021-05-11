@echo off

python -m pip install -r remoteroadrunner\requirements.txt

copy "remoteroadrunner\SDL2.dll" "C:\windows\system32\SDL2.dll"

echo Done...

pause