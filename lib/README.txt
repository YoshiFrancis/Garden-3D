Bundled GLFW library
====================
glfw3.lib is the Windows x64 static GLFW library used by base_glfw.vcxproj.
Both Debug and Release configurations target the MSVC v145 toolset.

Keep the provided library and include/GLFW/ headers together. Do not define
GLFW_DLL or replace this file with glfw3dll.lib. No glfw3.dll is required.
Normal Microsoft C/C++ runtime dependencies still apply.

License notice: include/GLFW/LICENSE.txt.
Linux/macOS: the Makefile links a native GLFW installation through pkg-config;
the Windows MSVC library is not used on those platforms.
