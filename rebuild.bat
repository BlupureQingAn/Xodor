@echo off
chcp 65001 >nul
echo 清理并重新构建项目...

REM 删除旧的构建目录
if exist build rmdir /s /q build

REM 创建新的构建目录
mkdir build
cd build

REM 配置 CMake
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=F:/Qt/qt/6.10.0/mingw_64 ..

REM 构建
cmake --build .

echo.
echo 构建完成！
