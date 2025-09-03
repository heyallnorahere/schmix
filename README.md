# schmix

Shitass DAW.

## Building

Only tested on my Arch development machine. No guarantee it'll work in your environment.

Required packages:
- SDL3 libraries and headers
- spdlog libraries and headers
- .NET 8 runtime

```bash
# build
dotnet build -f net8.0
cmake . -B build
cmake --build build -j 8

# run
build/src/schmix
```
