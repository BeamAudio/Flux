# How to Create a GitHub Release for Beam Audio Flux

1.  **Commit and Push**: Ensure all your changes are committed and pushed to the `master` branch on GitHub.
2.  **Go to GitHub**: Open the repository page in your browser.
3.  **Releases**: Click on "Releases" in the sidebar (or "Create a new release").
4.  **Draft a New Release**:
    *   **Tag version**: e.g., `v0.1.0`.
    *   **Release title**: e.g., "Beam Audio Flux v0.1.0 - Initial Prototype".
    *   **Description**: Copy the "User Guide" section from `README.md` or describe new features.
5.  **Upload Binaries**:
    *   Locate your compiled executable (`BeamAudioFlux.exe` in `build/Release/`).
    *   (Optional) Zip the executable along with the `assets` folder into `BeamAudioFlux_Win64.zip`.
    *   Drag and drop the zip file into the "Attach binaries" area.
6.  **Publish**: Click "Publish release".

## Compiled Targets
Currently, the build system is configured for the **Host Platform** (Windows x64).
To compile for other targets (e.g., Linux, macOS), you must run the CMake build process on those machines or set up a cross-compilation toolchain (which is complex and currently not configured).
The uploaded executable will be **Windows x64**.
