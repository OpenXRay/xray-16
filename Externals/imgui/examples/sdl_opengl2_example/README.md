
# How to Build

- On Windows with Visual Studio's CLI

```
set SDL2DIR=path_to_your_sdl2_folder
cl /Zi /MD /I %SDL2DIR%\include /I ..\.. main.cpp imgui_impl_sdl_gl2.cpp ..\..\imgui*.cpp /link /LIBPATH:%SDL2DIR%\lib SDL2.lib SDL2main.lib opengl32.lib /subsystem:console
```

- On Linux and similar Unixes

```
c++ `sdl2-config --cflags` -I ../.. main.cpp imgui_impl_sdl_gl2.cpp ../../imgui*.cpp `sdl2-config --libs` -lGL -o sdl2example
```

- On Mac OS X

```
brew install sdl2
c++ `sdl2-config --cflags` -I ../.. main.cpp imgui_impl_sdl_gl2.cpp ../../imgui*.cpp `sdl2-config --libs` -framework OpenGl -o sdl2example
```
