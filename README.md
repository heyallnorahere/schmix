# schmix

Shitass DAW.

## Building

Only tested on my Arch development machine. No guarantee it'll work in your environment.

Required packages:
- SDL3 libraries and headers
- .NET 8 runtime

```bash
# build
dotnet build
cmake . -B build
cmake --build build -j 8

# run
build/src/schmix
```
