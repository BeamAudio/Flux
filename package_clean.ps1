# PowerShell Build Script for Beam Audio Flux
$BUILD_DIR = "build_release"
$DIST_DIR = "dist"

Write-Host "Cleaning previous build..."
if (Test-Path $DIST_DIR) { Remove-Item -Recurse -Force $DIST_DIR }
# We don't remove BUILD_DIR here to save time if run repeated, but for this first run it will be new.

Write-Host "Configuring and Building with CMake..."
cmake -B $BUILD_DIR -S . -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { Write-Error "Configuration failed!"; exit 1 }

cmake --build $BUILD_DIR --config Release -j 16
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed!"; exit 1 }

# --- Packaging ---
Write-Host "Packaging..."
New-Item -ItemType Directory -Force $DIST_DIR | Out-Null

# Copy Executable
$EXE_PATH = "$BUILD_DIR\Release\BeamAudioFlux.exe" 
if (!(Test-Path $EXE_PATH)) { $EXE_PATH = "$BUILD_DIR\BeamAudioFlux.exe" } # Fallback
Copy-Item $EXE_PATH "$DIST_DIR\"

# Copy DLLs (SDL3)
$SDL_DLL = "$BUILD_DIR\_deps\sdl3-build\Release\SDL3.dll"
if (Test-Path $SDL_DLL) { Copy-Item $SDL_DLL "$DIST_DIR\" }

# Copy Assets
Write-Host "Copying assets..."
if (Test-Path "assets") { 
    New-Item -ItemType Directory -Force "$DIST_DIR/assets" | Out-Null
    Copy-Item -Recurse "assets/*" "$DIST_DIR/assets" 
}

# Copy Plugins
Write-Host "Creating plugins folder..."
New-Item -ItemType Directory -Force "$DIST_DIR/plugins" | Out-Null

# Copy SDK headers
Write-Host "Copying SDK headers..."
New-Item -ItemType Directory -Force "$DIST_DIR/src" | Out-Null
Get-ChildItem -Path "src" -Recurse -Filter "*.h*" | ForEach-Object {
    $dest = Join-Path "$DIST_DIR/src" ($_.FullName.Substring((Get-Item "src").FullName.Length + 1))
    $destDir = Split-Path $dest
    if (!(Test-Path $destDir)) { New-Item -ItemType Directory -Force $destDir | Out-Null }
    Copy-Item $_.FullName $dest
}

# Copy Compiler
Write-Host "Packaging compiler..."
if (Test-Path "tools/compiler") {
    New-Item -ItemType Directory -Force "$DIST_DIR/tools/compiler" | Out-Null
    Copy-Item -Recurse -Force "tools/compiler/*" "$DIST_DIR/tools/compiler/"
}

Write-Host "Packaging Complete! See the '$DIST_DIR' folder."
