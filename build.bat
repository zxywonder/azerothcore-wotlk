REM 创建 CMake 构建命令，并指定 Boost 和 MySQL 的路径
REM 请根据实际路径修改以下变量值
set current_dir=%~dp0
set BOOST_ROOT=%current_dir%..\thirdparty\boost_1_88_0
set MYSQL_ROOT=%current_dir%..\thirdparty\mysql-8.4.5-winx64

REM 执行 CMake 命令
REM 检查 build 目录是否存在，不存在则创建
if not exist build (
    mkdir build
)

REM 切换到 build 目录
cd build

cmake .. -G "Visual Studio 17 2022" -A x64 -DBOOST_ROOT="%BOOST_ROOT%" -DMYSQL_ROOT_DIR="%MYSQL_ROOT%"  -DCMAKE_INSTALL_PREFIX="%current_dir%..\AzerothCoreServer"
