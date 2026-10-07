# ARM64 dependencies for the Android build kit

The `arm64-v8a` prefix in build kit 0.9.104 was built for API 26 with NDK r30,
`c++_shared` and the same SDL source tree as the ARMv7 prefix. Libraries in the
prefix are static except for OpenAL Soft. The source revisions are:

| Library | Source |
|---|---|
| SDL2 | `toolchain/SDL` from build kit 0.8.0 (SDL 2.30.2, base `f461d91cd265d7b9a44b4d472b1df0c0ad2855a0` with the kit's Android edits) |
| OpenAL Soft 1.24.3 | `kcat/openal-soft@dc7d7054a5b4f3bec1dc23a42fd616a0847af948` |
| libjpeg-turbo 3.1.2 | `libjpeg-turbo/libjpeg-turbo@4e151a4ad91001b3aa8c2ece2205c15f487ce320` |
| Ogg 1.3.5 | `xiph/ogg@e1774cd77f471443541596e09078e78fdc342e4f` |
| Vorbis 1.3.7 | `xiph/vorbis@0657aee69dec8508a0011f47f3b69d7538e9d262` |
| Theora 1.1.1 | `xiph/theora@7ffd8b2ecfc2d93ae5e16028e7528e609266bfbf` |
| LZO 2.10 | `https://www.oberhumer.com/opensource/lzo/download/lzo-2.10.tar.gz`, SHA-256 `c0f892943208266f9b6543b3ae308fab6284c5c90e627931446fb49b4221a072` |

SDL2, OpenAL Soft, libjpeg-turbo, Ogg, Vorbis and LZO were configured with
CMake against the NDK toolchain. Vorbis was pointed at the installed Ogg
headers and static library. Theora was configured through Autotools with
`--host=aarch64-linux-android --disable-shared --enable-static --disable-examples`
and the cross compiler `aarch64-linux-android26-clang`. OpenAL Soft 1.24.3's
bundled fmt headers need `-include cstdlib` with NDK r30. Host tools for the
APK build are JDK 17 and `glslc` in addition to those archived in the kit.
