@echo off

python -m pip install --upgrade pip
python -m pip install wheel
python -m pip install bleak
python -m pip install asyncio
python -m pip install imgui[glfw]
python -m pip install imgui[sdl2]
python -m pip install pysdl2

copy "remoteroadrunner\SDL2.dll" "C:\windows\system32\SDL2.dll"

echo Done...

pause