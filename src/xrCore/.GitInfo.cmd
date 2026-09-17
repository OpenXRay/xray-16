@FOR /f "delims=" %%i in ('git rev-parse --verify HEAD') DO set COMMIT=%%i
@FOR /f "delims=" %%i in ('git rev-parse --abbrev-ref HEAD') DO set BRANCH=%%i

echo #pragma once > .GitInfo.hpp

echo #ifndef GIT_INFO_CURRENT_COMMIT >> .GitInfo.hpp
echo #define GIT_INFO_CURRENT_COMMIT "%COMMIT%" >> .GitInfo.hpp
echo #endif >> .GitInfo.hpp

echo #ifndef GIT_INFO_CURRENT_BRANCH >> .GitInfo.hpp
echo #define GIT_INFO_CURRENT_BRANCH "%BRANCH%" >> .GitInfo.hpp
echo #endif >> .GitInfo.hpp
