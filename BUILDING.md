# GLFW build and migration notes

## Supported environment

Windows 10/11, Visual Studio 2022 with Desktop development with C++, MSVC v143,
and Windows SDK 10.0. Both Win32 and x64 configurations are provided.
The GPU and installed driver must support desktop OpenGL 3.3 compatibility contexts.

This change makes the Windows project self-contained across development PCs.
It does not provide Linux/macOS support: image decoding, text rasterization,
screenshots, and some file operations still use Windows APIs/GDI+.
CMake rejects non-Windows targets rather than generating an unusable project.

## Build using the existing solution

Open `SimpleGame.sln` in Visual Studio 2022. Select `Release` and `x64` (or `x86`),
then build. No package manager, dependency download, Python, or CMake is required.
GLFW and GLEW C sources compile directly into the executable.

Output: `build/msbuild/x64/Release/SimpleGame.exe`.
The x86 solution configuration uses `build/msbuild/Win32/Release/`.
Debug output uses the corresponding `Debug` directory.

## Build using CMake

Install CMake 3.20 or newer, then run from the repository root:

```powershell
cmake -S . -B build/cmake-x64 -G "Visual Studio 17 2022" -A x64
cmake --build build/cmake-x64 --config Release
```

Output: `build/cmake-x64/bin/Release/SimpleGame.exe`.
For x86, use a separate build directory and `-A Win32`.
The included upstream GLFW CMake files build a static library; GLEW is also static.
Both build paths use C++17, UTF-8 source encoding, and the static MSVC runtime.

## Running and distributing

The runtime copy step uses the Windows-supplied `powershell.exe` and
`tools/CopyRuntime.ps1` to copy `Data`, `Shaders`, `Fonts`, runtime PNG/JSON assets,
and dependency licenses beside the executable. It does not run the game or tests.
Blender source files and asset previews are not copied into the runtime package.
Visual Studio runs this step after linking. CMake uses one shared dependency target
so parallel check builds do not write the same runtime files concurrently.

Distribute the entire output directory, including `Fonts/OFL.txt` and `Licenses`.
The executable selects its own directory for game resources, so shortcuts and
launching from a different working directory do not change resource lookup.
Relative `--capture` output paths remain relative to the original launch directory.
The optional capture mode writes a frame only when explicitly invoked.

No `freeglut.dll`, `glfw3.dll`, or `glew32.dll` is required by the new executable.
The static runtime removes the application's MSVC runtime DLL dependency.
Windows still supplies GDI+, OpenGL system libraries, and GPU drivers.
Do not copy an old local `opengl32.dll` into the output directory.

Old freeglut/GLEW headers, libraries, and old executable directories remain in place
to preserve existing files. The updated application and checks do not reference them.
Use the new output directories; an old executable is not evidence of this migration.

## Behavior preservation

| Area | Implementation |
| --- | --- |
| Context | OpenGL 3.3 compatibility, double buffering, no requested MSAA/depth/stencil/alpha bits; retains the renderer's default VAO usage |
| Loop | Poll events, update gameplay, render world, postprocess, draw UI, swap buffers |
| Time | Milliseconds from `glfwGetTime`; retains the 0.05-second maximum gameplay step |
| Frame pacing | No explicit swap-interval override or new FPS limiter; driver policy remains in control |
| Keyboard | GLFW key press/repeat/release maps to the existing ASCII `GameInput`; Escape maps to 27 |
| Shift | Poll both Shift keys while focused; does not require another movement-key event |
| Focus | Clear held keys when focus is lost to prevent stuck movement |
| Mouse | Convert window cursor coordinates into the renderer's pixel coordinate space before UI hit testing |
| Resize | Use framebuffer dimensions with the existing 640-by-360 rendering minimum; skip zero-sized framebuffers |
| DPI | On Windows 10 1703 and newer, pin the existing DPI awareness before GLFW can change it; respect an already fixed manifest policy |
| Shutdown | Destroy world/renderer resources before the current GLFW context, including initialization failures |
| Text | Retain GDI+ 18px Malgun Gothic, antialiasing, measurements, and cached label textures |
| Font fallback | If Malgun Gothic is unavailable, load bundled Nanum Gothic privately; fail initialization if neither is usable |
| Window title | Pass UTF-8 Korean titles directly to GLFW |
| Rendering | Gameplay, shaders, atlas sampling, world draw ordering, terrain cache, postprocessing, and UI layout are unchanged by this migration |

Nanum Gothic has different glyph metrics from Malgun Gothic. Identical text pixels
are expected only when the same system font and rendering environment are used.
Older Windows 10 versions without the DPI-awareness-context API retain GLFW's
platform defaults. Mixed-DPI monitors and Windows scaling require a manual comparison.
No claim of identical frame timing, FPS, or GPU output is made without runtime checks.

## Optional checks

Check executables are disabled by default. To build them explicitly:

```powershell
cmake -S . -B build/cmake-checks -G "Visual Studio 17 2022" -A x64 -DGSE_BUILD_TESTS=ON
cmake --build build/cmake-checks --config Debug
```

Run checks from `build/cmake-checks/bin/Debug/` so their relative resource paths resolve.
`GlfwWindowCheck.exe` creates a hidden window and checks the compatibility context,
double buffering, UTF-8 title, Korean label drawing, existing input normalization,
repeat suppression, and close flag. It does not capture images.
The original three renderer smoke checks now use GLFW and release their contexts
after GPU objects. Some of those checks generate captures when run; execute them
only when capture verification is requested.
Existing gameplay/content checks are also available as separate executables.
No tests run automatically after a build.

Manual comparison should cover held WASD, Shift changes while moving, repeated
E/Q/Space, inventory/crafting repeat suppression, focus loss, mouse HUD clicks,
resize/minimize/restore, mixed DPI, Korean labels/titles, Escape/window close,
and matching Release FPS/GPU measurements under the same driver settings.

## Included dependencies and provenance

Sources and licenses are included locally. Builds do not fetch from the network.
Upstream files are unmodified.

| Dependency | Source | SHA-256 of downloaded archive/file |
| --- | --- | --- |
| GLFW 3.4 | https://codeload.github.com/glfw/glfw/zip/refs/tags/3.4 | `a133ddc3d3c66143eba9035621db8e0bcf34dba1ee9514a9e23e96afd39fd57a` |
| GLEW 2.2.0 | https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.zip | `a9046a913774395a095edcc0b0ac2d81c3aacca61787b39839b941e9be14e0d4` |
| Nanum Gothic Regular | https://raw.githubusercontent.com/google/fonts/main/ofl/nanumgothic/NanumGothic-Regular.ttf | `76f45ef4a6bcff344c837c95a7dcc26e017e38b5846d5ae0cdcb5b86be2e2d31` |

GLFW license: `SimpleGame/Dependencies/glfw/LICENSE.md`.
GLEW and embedded component licenses: `SimpleGame/Dependencies/glew/LICENSE.txt`.
Font license: `SimpleGame/Fonts/OFL.txt` (SIL Open Font License 1.1).
The Windows Malgun Gothic font is not redistributed.

## Verification status for this migration

Source/configuration inspection only. Builds, executables, GPU rendering, screenshots,
font fallback, and performance comparisons have not been run, per project instructions.
Prior freeglut test results do not validate this GLFW version.

To undo the migration, reverse only the GLFW-related patches and newly added files.
Do not reset whole files: this checkout already contained unrelated edits before
the migration began.
