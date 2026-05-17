# Focus Video Windows package

This ZIP contains the `focus-video.exe` command-line prototype and project docs.
It is a real Windows executable package, but the current product slice is still
limited to CLI inspection/decode smoke paths, local playback through an installed
FFmpeg `ffplay.exe` backend, and PPM reference upscaling. Cross-compiled packages
do not bundle Windows FFmpeg development DLLs yet, so the in-process FFmpeg path
should be validated on a native Windows build when those libraries are supplied.

## Requirements

- Windows 10/11 x64.
- FFmpeg for Windows with `ffplay.exe` available on `PATH` for local video playback.
- For future TensorRT SR validation: NVIDIA RTX GPU, matching NVIDIA driver,
  TensorRT with `trtexec.exe` on `PATH`, and a compatible `.engine` model.

## Quick smoke test

Open PowerShell in the extracted package directory:

```powershell
.\focus-video.exe --check-runtime --play C:\path\to\video.mp4
.\focus-video.exe --play C:\path\to\video.mp4
```

If `ffplay.exe` is not on `PATH`, install an FFmpeg Windows build and add its
`bin` directory to `PATH`, then open a new terminal.

TensorRT super-resolution requests intentionally fail until the FFmpeg/NVDEC ->
TensorRT -> renderer frame bridge is implemented:

```powershell
.\focus-video.exe --check-runtime --play C:\path\to\video.mp4 --with-tensorrt-sr --trt-engine C:\path\to\model.engine
```
