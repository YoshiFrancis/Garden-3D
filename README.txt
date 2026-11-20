CS334 / ECE30834 - Assignment 4: Procedural-it!
L-Systems and Basic Procedural Modeling
=============================================
Student starter | GLFW viewer | Fall 2026
Release: October 21, 2026
Deadline: follow the current course handout and Brightspace announcement.

This project supplies the viewer and rendering framework. Implement the TODOs
in src/lsystem.cpp and, for the optional bonus, src/lsystem.hpp. The parser,
rewriting algorithm, drawing interpreter, jitter, and weighted-rule selection
are deliberately incomplete. Building the project does not complete the tasks.

1. WINDOWS: BUILD AND RUN
------------------------
1) Extract the entire ZIP to a writable folder. Do not run inside the ZIP.
2) Open base_glfw.sln in Microsoft Visual Studio 2026, NOT Visual Studio Code.
3) In Visual Studio Installer, install the Desktop development with C++ workload,
   including the MSVC v145 toolset and Windows SDK used by this project.
4) Select Debug | x64 (or Release | x64), then Build > Rebuild Solution.
5) Run with Ctrl+F5, or F5 to use the debugger.

This is a native C++ project. Do not use "dotnet build" for this solution.
From a Visual Studio Developer Command Prompt, an alternative is:
  msbuild base_glfw.sln /p:Configuration=Debug /p:Platform=x64

The provided project targets C++17 and OpenGL 3.3 core. It includes the matching
GLFW headers in include/GLFW/ and a Windows x64 static library in lib/glfw3.lib.
No separate GLFW installation or glfw3.dll is needed for the Windows project.
The C/C++ runtime supplied with the Visual Studio environment is still used.
Do not define GLFW_DLL or substitute glfw3dll.lib. There is no x86/Win32 build.

Output locations:
  x64/Debug/base_glfw.exe
  x64/Release/base_glfw.exe

Visual Studio uses the project root as the working directory. The build also
copies models/ and shaders/ next to the executable. Keep those resource folders
with the executable when moving it. When running from Visual Studio, edit the
project-root models/ input files, not a stale copy under x64/Debug/.

2. WHAT YOU SHOULD SEE BEFORE IMPLEMENTING THE TODOs
---------------------------------------------------
A dark-gray GLFW window with no tree is EXPECTED. The console prints:
  TODO1 not implemented: complete LSystem::parse() in src/lsystem.cpp.

This diagnostic is inside TODO1; remove it after implementing the parser.
The placeholder parser creates an empty axiom, applyRules() returns an empty
string, and createGeometry() returns no vertices. The provided framework safely
handles empty geometry. No expected tree is hard-coded into the starter.

Use the console to inspect parsed fields and short expanded strings while
working on TODO1/TODO2. You will not obtain a completed tree until parsing,
rewriting, and drawing are implemented. After editing C++ files, rebuild and
restart. Space/R reloads a TEXT INPUT FILE; it does not compile C++.

3. CONTROLS
-----------
Up / Down       Select previous / next input file (alphabetical order)
Left / Right    Previous / next iteration; Right can generate a new iteration
Space or R      Reload the selected input file
Escape          Exit
H or F1         Print viewer help to the console
Drop a .txt     Load the dropped input file
Right click     Windows: model / iteration / reload / exit popup menu
                Linux/macOS: print the input-file list; use Up/Down to select

The default selection is models/tree1.txt. The title shows the file and current
iteration. Keyboard controls are available on all platforms. The Windows popup
menu, event loop, high-DPI viewport handling, and resource lookup are provided.
There are no assignment TODOs in main.cpp, the GL loader, or the shaders.

4. TODO LOCATIONS AND WEIGHTS
----------------------------
Search for the exact labels TODO1, TODO2, TODO3, TODO4, and TODO5 in the source.

TODO1 | 30% | src/lsystem.cpp : LSystem::parse()
  Read the angle, iteration setting, axiom, and deterministic production rules.
  Fill inAngle, inIters, inAxiom, and inRules. Keep the provided state update and
  iteration loop below the TODO blocks. Do not add another iteration loop.

TODO2 | 30% | src/lsystem.cpp : LSystem::applyRules()
  Return one simultaneous rewriting step. A character with no rule survives.
  Replacement characters are rewritten only on the next iteration.

TODO3 | 30% | src/lsystem.cpp : LSystem::createGeometry()
  Complete initialization and the individually marked drawing-command cases.
  Each group below is 5% of the assignment:
    f/F/g/G : draw a segment and advance using the current step length
    s/S     : advance without emitting vertices
    +       : counterclockwise turn
    -       : clockwise turn
    [       : save the whole state, then scale the new branch step by 0.75
    ]       : restore position, heading, and step length; do not draw on return
  The state starts at the origin pointing along +Y. Choose a positive initial
  step (the handout examples use 1). Each pair of output vertices is one line.
  A numeric branchScale = 0.75 is provided, but applying and restoring that
  scale is part of YOUR TODO3 implementation, not an implemented feature.

TODO4 | 10% | src/lsystem.cpp : parse() and BOTH turn cases in createGeometry()
  Extend the angle-line parser to read optional J in A<J>; missing J means 0.
  Sample a fresh uniform offset in [-J,+J] for each turn, using the rng member.
  Apply A plus that offset with the command's sign. Do not change step lengths.
  The parser note belongs to the first-line reader; integrate it there rather
  than attempting to reread the first line after the rule parser has finished.

TODO5 | BONUS 15% | src/lsystem.hpp : rules member
                      src/lsystem.cpp : parse() and applyRules()
  Support multiple weighted alternatives for one predecessor. Update BOTH the
  member rule container and parse()'s inRules type, and the replacement lookup.
  Select each alternative with probability weight / total weight for that
  predecessor. Missing weights mean 1. Preserve deterministic input support.
  The starter provides std::map<char, std::string> for required TODO1/TODO2;
  the bonus data structure and sampling logic are NOT implemented.

The required tasks total 100%; TODO5 is optional. The 0.75 branch rule belongs
inside TODO3, not an extra task or bonus. See the handout for full requirements.

5. INPUT FILES AND ITERATION NUMBERING
------------------------------------
A deterministic input has:
  first line     rotation angle A, optionally followed by <J>
  second line    iteration setting N
  third line     axiom (starting string)
  remaining      one predecessor : successor rule on each line

For example, models/tree1.txt contains:
  25.7
  6
  f
  f : f[+f]f[-f]f

Preprocessing removes comments introduced by #, empty lines, and leading /
trailing whitespace. Your parser should also ignore whitespace as required by
the handout. Each predecessor is one character. Preserve the supplied syntax
and model files. Unknown drawing symbols do not generate geometry; that is
separate from preserving characters with no production rule during rewriting.

The axiom is iteration 0. For positive N, the supplied framework initially caches
N states INCLUDING the axiom. N=6 therefore means iterations 0 through 5.
The viewer starts at the last initially cached state. Right requests another
state when necessary. Compare the same displayed iteration as the handout.

Branch steps depend on bracket nesting, not rewriting iteration number.
Save the parent state BEFORE shortening the step on '['. A child step is 0.75
of its parent's. Both drawn and non-drawn moves use this step. On ']', restore
the parent's step as well as its position and heading. The branch attachment
point does not move just because its step length changes.

Suggested supplied inputs:
  tree1.txt / tree2.txt   Deterministic branches, saved state, and step scaling
  dragon.txt             Deterministic turn sequence without bracket branches
  sierpinski.txt          Multiple rules without bracket branches
  tree1_rand_angle.txt    Required jitter extension (TODO4)
  tree_rand.txt           Optional weighted-rule extension (TODO5)

A parser without TODO4 is not expected to handle jitter syntax yet. The weighted
input tree_rand.txt is a bonus test, not a required deterministic-parser test.
Random runs can differ; compare the required behavior, not exact random pixels.

6. LINUX BUILD
--------------
Install C/C++ build tools, pkg-config, and the native GLFW development package.
On Ubuntu/Debian, for example:
  sudo apt install build-essential pkg-config libglfw3-dev
From the extracted project root:
  make
  ./base_glfw
Or choose an input on launch:
  ./base_glfw models/tree2.txt

The Makefile uses C++17 and pkg-config glfw3. GLM headers and the OpenGL function
loader are bundled. The Windows .lib is NOT used by the Linux build.
A desktop/display server and an OpenGL 3.3-capable context are needed to run it.

7. MACOS BUILD (OPTIONAL ENVIRONMENT)
------------------------------------
Install Xcode Command Line Tools and a native GLFW development package.
With an existing Homebrew installation:
  brew install glfw pkg-config
  make CXX=clang++ CC=clang
  ./base_glfw

The macOS context is forward-compatible OpenGL 3.3 core. OS/hardware support
is required; the Windows .lib is not usable on macOS. The Windows right-click
menu is not part of this build; keyboard and drop-file controls are available.
This package has not been runtime-tested on macOS. See the course's platform
support policy in the handout.

8. TROUBLESHOOTING
------------------
Dark-gray window and the TODO1 diagnostic:
  This is normal before the assignment code is implemented; see Section 2.

Visual Studio cannot find a C++ toolset or does not load the project:
  Check the C++ desktop workload, the v145 toolset, and an x64 configuration.
  Use Microsoft Visual Studio rather than VS Code or dotnet build.

Cannot find models/ or shaders/:
  Extract the full archive and run from the project root, or retain both folders
  beside the executable. Do not copy only the executable to a different folder.

Cannot see a change after editing a .cpp or .hpp:
  Rebuild and restart. Reloading a .txt does not recompile source code.

GLFW reports it cannot create an OpenGL 3.3 context:
  Check graphics support and the display environment; share the complete console
  output on the course discussion board when asking for help.

A high iteration becomes slow or exceeds a buffer limit:
  Rewriting can grow quickly. Start with the supplied iteration settings and
  test shorter expansions first. Keep the viewer's existing size checks.

9. FILE LAYOUT AND SUBMISSION
-----------------------------
base_glfw.sln / .vcxproj / .filters  Visual Studio x64 project
Makefile                             Linux/macOS build
src/lsystem.cpp                     TODO1-5; provided cache/upload helpers
src/lsystem.hpp                     LSystem declaration; TODO5 rule storage
src/main.cpp                        Provided GLFW viewer and UI
src/gl_core_3_3.c/.h                 Provided OpenGL entry-point loader
src/util.cpp/.hpp                   Provided shader loading helpers
include/GLFW/                       GLFW headers and license notice
include/glm/                        GLM headers
lib/glfw3.lib                       Bundled Windows x64 static GLFW library
models/                             Supplied input files
shaders/                            Provided vertex and fragment shaders

Submit through Brightspace as specified in the current handout. Include source,
project/build files, the shaders, the input files, and an executable for your
platform, with the runtime resources it needs. Omit .vs/, object files, compiler
caches, and other temporary build products; retain the requested executable.
Keep bundled third-party headers, library files, and license notices required
for rebuilding. The deadline is set by the course, not by this README.
