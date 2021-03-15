
project "Test"
  configurations { "Debug", "Release" }

  dependson { "FractalEngine" }

  kind "ConsoleApp"
  architecture "x64"
  language "C++"
  characterset ("MBCS")

-- Set Directories
  targetdir (binPath)
  debugdir (binPath)
  objdir (buildsPath .. "/output/%{cfg.platform}_%{cfg.buildcfg}")
  symbolspath '$(OutDir)$(TargetName).pdb'

-- Includes
  includedirs { "../sketch/" } 

-- Project Files
  files { "**.cpp", "**.h", "**.inl", "**.natvis" }

  -- Source files containing code we want to test
  files { "../sketch/List.h" }
  files { "../sketch/Str.h", "../sketch/Str.cpp" }
  files { "../sketch/Commands.h", "../sketch/Commands.cpp" }
  files { "../sketch/SerialCommands.h", "../sketch/SerialCommands.cpp" }
  files { "../sketch/Seeker.h", "../sketch/Seeker.cpp" }

