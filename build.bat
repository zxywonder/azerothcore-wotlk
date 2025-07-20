REM ���� CMake ���������ָ�� Boost �� MySQL ��·��
REM �����ʵ��·���޸����±���ֵ
set current_dir=%~dp0
set BOOST_ROOT=%current_dir%..\thirdparty\boost_1_88_0
set MYSQL_ROOT=%current_dir%..\thirdparty\mysql-8.4.5-winx64

REM ִ�� CMake ����
REM ��� build Ŀ¼�Ƿ���ڣ��������򴴽�
if not exist build (
    mkdir build
)

REM �л��� build Ŀ¼
cd build

cmake .. -G "Visual Studio 17 2022" -A x64 -DBOOST_ROOT="%BOOST_ROOT%" -DMYSQL_ROOT_DIR="%MYSQL_ROOT%"  -DCMAKE_INSTALL_PREFIX="%current_dir%..\AzerothCoreServer"
