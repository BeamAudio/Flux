# Beam Audio Flux - Windows Packaging Script
$distDir = "dist"
$appName = "BeamAudioFlux"

# 1. Compile with CMake
Write-Host "Configuring and Building with CMake..."
if (!(Test-Path "build")) { New-Item -ItemType Directory -Path "build" }
cmake -S . -B build
cmake --build build --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed! Exiting."
    exit $LASTEXITCODE
}

# 2. Clean and Create Dist folder
if (Test-Path $distDir) { Remove-Item -Recurse -Force $distDir }
New-Item -ItemType Directory -Path $distDir
New-Item -ItemType Directory -Path "$distDir/assets"
New-Item -ItemType Directory -Path "$distDir/tools"

# 2. Copy Executable
Write-Host "Copying executable..."
if (Test-Path "build/Release/BeamAudioFlux.exe") {
    Copy-Item "build/Release/BeamAudioFlux.exe" "$distDir/$appName.exe"
} else {
    Write-Error "BeamAudioFlux.exe not found! Compile in Release mode first."
    exit 1
}

# Copy SDL3.dll if it exists in root (some builds might need it)
if (Test-Path "SDL3.dll") {
    Write-Host "Copying SDL3.dll..."
    Copy-Item "SDL3.dll" "$distDir/"
}

# 3. Copy Assets and SDK (Headers)
Write-Host "Copying assets..."
Copy-Item -Recurse "assets/*" "$distDir/assets"

Write-Host "Copying SDK headers..."
New-Item -ItemType Directory -Path "$distDir/src"
# Copy only .hpp and .h files to keep distribution small
Get-ChildItem -Path "src" -Recurse -Filter "*.h*" | ForEach-Object {
    $dest = Join-Path "$distDir/src" ($_.FullName.Substring((Get-Item "src").FullName.Length + 1))
    $destDir = Split-Path $dest
    if (!(Test-Path $destDir)) { New-Item -ItemType Directory -Path $destDir }
    Copy-Item $_.FullName $dest
}

Write-Host "Copying third_party headers..."
New-Item -ItemType Directory -Path "$distDir/third_party"
Copy-Item -Filter "*.h*" "third_party/*" "$distDir/third_party"

# 4. Create Plugins and Copy Compiler
Write-Host "Creating plugins folder..."
New-Item -ItemType Directory -Path "$distDir/plugins"

Write-Host "Packaging compiler..."
if (Test-Path "tools/compiler/bin") {
    # Ensure destination parent exists
    if (!(Test-Path "$distDir/tools/compiler")) { New-Item -ItemType Directory -Path "$distDir/tools/compiler" }
    # Copy contents with Force to overwrite
    Copy-Item -Recurse -Force "tools/compiler/*" "$distDir/tools/compiler/"
    Write-Host "Compiler bundled successfully."
} else {
    Write-Warning "Compiler not found in tools/compiler/bin. Distribution will require manual compiler setup."
    if (!(Test-Path "$distDir/tools/compiler")) { New-Item -ItemType Directory -Path "$distDir/tools/compiler" }
}

# 5. Instructions
$readme = @"
# Beam Audio Flux - Windows Distribution

To enable FluxScript AOT Compilation:
1. Download a portable MinGW-w64 (GCC) distribution.
2. Copy the contents of 'bin', 'include', and 'lib' to the 'tools/compiler/' folder.
3. Ensure 'g++.exe' exists at 'tools/compiler/bin/g++.exe'.

Run '$appName.exe' to start the DAW.
"@
$readme | Out-File "$distDir/README_DIST.txt"

Write-Host "Packaging Complete! See the '$distDir' folder."
