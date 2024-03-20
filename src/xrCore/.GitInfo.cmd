@FOR /f "delims=" %%i in ('git rev-parse --verify HEAD') DO set COMMIT=%%i
echo #define GIT_INFO_CURRENT_COMMIT "%COMMIT%" > .GitInfo.hpp

@FOR /f "delims=" %%i in ('git rev-parse --abbrev-ref HEAD') DO set BRANCH=%%i
echo #define GIT_INFO_CURRENT_BRANCH "%BRANCH%" >> .GitInfo.hpp
