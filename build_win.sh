# 1. Create a separate build folder for Windows
rm -rf build_win
mkdir build_win

# 2. Run CMake and force it to use the Windows cross-compiler
cmake -S . -B build_win -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc

# 3. Build the project
cmake --build build_win

#4. Copy the created file to en-croissant engines folder (WSL path)
cp build_win/talawachess.exe /mnt/c/Users/orvil/AppData/Roaming/org.encroissant.app/engines/talawachess
