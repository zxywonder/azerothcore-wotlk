/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRID_TERRAIN_DATA_H
#define GRID_TERRAIN_DATA_H

#include "Common.h"
#include <fstream>
#include <G3D/Plane.h>
#include <memory>

#define MAX_HEIGHT            100000.0f                     // can be use for find ground height at surface
#define INVALID_HEIGHT       -100000.0f                     // for check, must be equal to VMAP_INVALID_HEIGHT, real value for unknown height is VMAP_INVALID_HEIGHT_VALUE
#define MAX_FALL_DISTANCE     250000.0f                     // "unlimited fall" to find VMap ground if it is available, just larger than MAX_HEIGHT - INVALID_HEIGHT
#define MIN_HEIGHT           -500.0f

#define MAP_LIQUID_STATUS_SWIMMING (LIQUID_MAP_IN_WATER | LIQUID_MAP_UNDER_WATER)
#define MAP_LIQUID_STATUS_IN_CONTACT (MAP_LIQUID_STATUS_SWIMMING | LIQUID_MAP_WATER_WALK)

#define MAP_LIQUID_TYPE_NO_WATER    0x00
#define MAP_LIQUID_TYPE_WATER       0x01
#define MAP_LIQUID_TYPE_OCEAN       0x02
#define MAP_LIQUID_TYPE_MAGMA       0x04
#define MAP_LIQUID_TYPE_SLIME       0x08

#define MAP_ALL_LIQUIDS   (MAP_LIQUID_TYPE_WATER | MAP_LIQUID_TYPE_OCEAN | MAP_LIQUID_TYPE_MAGMA | MAP_LIQUID_TYPE_SLIME)

#define MAP_LIQUID_TYPE_DARK_WATER  0x10

// ******************************************
// Map文件格式定义
// ******************************************
// 地图魔法数字联合类型，用于存储4字节的魔法数字，可以按字符数组或无符号32位整数访问
union u_map_magic
{
    char asChar[4];  // 以字符数组形式存储魔法数字
    uint32 asUInt;   // 以无符号32位整数形式存储魔法数字
};

// 地图文件的魔法数字，值为 "MAPS"
const u_map_magic MapMagic        = { {'M', 'A', 'P', 'S'} };
// 地图版本魔法数字，值为9
const uint32 MapVersionMagic      = 9;
// 地图区域魔法数字，值为 "AREA"
const u_map_magic MapAreaMagic    = { {'A', 'R', 'E', 'A'} };
// 地图高度魔法数字，值为 "MHGT"
const u_map_magic MapHeightMagic  = { {'M', 'H', 'G', 'T'} };
// 地图液体魔法数字，值为 "MLIQ"
const u_map_magic MapLiquidMagic  = { {'M', 'L', 'I', 'Q'} };

// 地图文件头结构体，存储地图文件的基本信息
struct map_fileheader
{
    uint32 mapMagic;        // 地图魔法数字
    uint32 versionMagic;    // 版本魔法数字
    uint32 buildMagic;      // 构建魔法数字
    uint32 areaMapOffset;   // 区域地图数据偏移量
    uint32 areaMapSize;     // 区域地图数据大小
    uint32 heightMapOffset; // 高度地图数据偏移量
    uint32 heightMapSize;   // 高度地图数据大小
    uint32 liquidMapOffset; // 液体地图数据偏移量
    uint32 liquidMapSize;   // 液体地图数据大小
    uint32 holesOffset;     // 空洞数据偏移量
    uint32 holesSize;       // 空洞数据大小
};

// 表示地图区域无效的标志
#define MAP_AREA_NO_AREA      0x0001

// 地图区域头结构体，存储地图区域的基本信息
struct map_areaHeader
{
    uint32 fourcc;    // 4字符代码
    uint16 flags;     // 标志位
    uint16 gridArea;  // 网格区域ID
};

// 表示地图高度无效的标志
#define MAP_HEIGHT_NO_HEIGHT            0x0001
// 表示地图高度以16位无符号整数存储的标志
#define MAP_HEIGHT_AS_INT16             0x0002
// 表示地图高度以8位无符号整数存储的标志
#define MAP_HEIGHT_AS_INT8              0x0004
// 表示地图高度有飞行边界的标志
#define MAP_HEIGHT_HAS_FLIGHT_BOUNDS    0x0008

// 地图高度头结构体，存储地图高度的基本信息
struct map_heightHeader
{
    uint32 fourcc;       // 4字符代码
    uint32 flags;        // 标志位
    float  gridHeight;   // 网格高度
    float  gridMaxHeight;// 网格最大高度
};

// 表示地图液体类型无效的标志
#define MAP_LIQUID_NO_TYPE    0x0001
// 表示地图液体高度无效的标志
#define MAP_LIQUID_NO_HEIGHT  0x0002

// 地图液体头结构体，存储地图液体的基本信息
struct map_liquidHeader
{
    uint32 fourcc;        // 4字符代码
    uint8 flags;          // 标志位
    uint8 liquidFlags;    // 液体标志位
    uint16 liquidType;    // 液体类型
    uint8  offsetX;       // X轴偏移量
    uint8  offsetY;       // Y轴偏移量
    uint8  width;         // 宽度
    uint8  height;        // 高度
    float  liquidLevel;   // 液体高度
};

// ******************************************
// 已加载的地图数据结构
// ******************************************

// 已加载的区域数据结构体
struct LoadedAreaData
{
    // 区域地图数据类型，使用16x16的无符号16位整数数组
    typedef std::array<uint16, 16 * 16> AreaMapType;

    uint16 gridArea;                 // 网格区域ID
    std::unique_ptr<AreaMapType> areaMap; // 指向区域地图数据的智能指针
};

// 已加载的高度数据结构体
struct LoadedHeightData
{
    // 高度平面数据类型，使用包含8个G3D::Plane的数组
    typedef std::array<G3D::Plane, 8> HeightPlanesType;

    // 16位无符号整数高度数据结构体
    struct Uint16HeightData
    {
        // 版本9的高度数据类型，使用129x129的无符号16位整数数组
        typedef std::array<uint16, 129 * 129> V9Type;
        // 版本8的高度数据类型，使用128x128的无符号16位整数数组
        typedef std::array<uint16, 128 * 128> V8Type;

        V9Type v9;                     // 版本9的高度数据
        V8Type v8;                     // 版本8的高度数据
        float gridIntHeightMultiplier; // 网格整数高度乘数
    };

    // 8位无符号整数高度数据结构体
    struct Uint8HeightData
    {
        // 版本9的高度数据类型，使用129x129的无符号8位整数数组
        typedef std::array<uint8, 129 * 129> V9Type;
        // 版本8的高度数据类型，使用128x128的无符号8位整数数组
        typedef std::array<uint8, 128 * 128> V8Type;

        V9Type v9;                     // 版本9的高度数据
        V8Type v8;                     // 版本8的高度数据
        float gridIntHeightMultiplier; // 网格整数高度乘数
    };

    // 浮点数高度数据结构体
    struct FloatHeightData
    {
        // 版本9的高度数据类型，使用129x129的浮点数数组
        typedef std::array<float, 129 * 129> V9Type;
        // 版本8的高度数据类型，使用128x128的浮点数数组
        typedef std::array<float, 128 * 128> V8Type;

        V9Type v9; // 版本9的高度数据
        V8Type v8; // 版本8的高度数据
    };

    float gridHeight;                     // 网格高度
    std::unique_ptr<Uint16HeightData> uint16HeightData; // 指向16位无符号整数高度数据的智能指针
    std::unique_ptr<Uint8HeightData> uint8HeightData;   // 指向8位无符号整数高度数据的智能指针
    std::unique_ptr<FloatHeightData> floatHeightData;   // 指向浮点数高度数据的智能指针
    std::unique_ptr<HeightPlanesType> minHeightPlanes;  // 指向最小高度平面数据的智能指针
};

// 已加载的液体数据结构体
struct LoadedLiquidData
{
    // 液体条目数据类型，使用16x16的无符号16位整数数组
    typedef std::array<uint16, 16 * 16> LiquidEntryType;
    // 液体标志数据类型，使用16x16的无符号8位整数数组
    typedef std::array<uint8, 16 * 16> LiquidFlagsType;
    // 液体地图数据类型，使用浮点数向量
    typedef std::vector<float> LiquidMapType;

    uint16 liquidGlobalEntry;   // 全局液体条目
    uint8 liquidGlobalFlags;    // 全局液体标志
    uint8 liquidOffX;           // X轴偏移量
    uint8 liquidOffY;           // Y轴偏移量
    uint8 liquidWidth;          // 宽度
    uint8 liquidHeight;         // 高度
    float liquidLevel;          // 液体高度
    std::unique_ptr<LiquidEntryType> liquidEntry; // 指向液体条目数据的智能指针
    std::unique_ptr<LiquidFlagsType> liquidFlags; // 指向液体标志数据的智能指针
    std::unique_ptr<LiquidMapType> liquidMap;     // 指向液体地图数据的智能指针
};

// 已加载的空洞数据结构体
struct LoadedHoleData
{
    // 空洞数据类型，使用16x16的无符号16位整数数组
    typedef std::array<uint16, 16 * 16> HolesType;

    HolesType holes; // 空洞数据
};

// 液体状态枚举
enum LiquidStatus
{
    LIQUID_MAP_NO_WATER     = 0x00000000, // 无液体
    LIQUID_MAP_ABOVE_WATER  = 0x00000001, // 在液体上方
    LIQUID_MAP_WATER_WALK   = 0x00000002, // 可以在液体上行走
    LIQUID_MAP_IN_WATER     = 0x00000004, // 在液体中
    LIQUID_MAP_UNDER_WATER  = 0x00000008  // 在液体下方
};

// 液体数据结构体
struct LiquidData
{
    LiquidData() = default; // 默认构造函数

    uint32 Entry{ 0 };          // 液体条目
    uint32 Flags{ 0 };          // 液体标志
    float  Level{ INVALID_HEIGHT }; // 液体高度
    float  DepthLevel{ INVALID_HEIGHT }; // 液体深度
    LiquidStatus Status{ LIQUID_MAP_NO_WATER }; // 液体状态
};

// 地形地图数据读取结果枚举类
enum class TerrainMapDataReadResult
{
    Success,            // 读取成功
    NotFound,           // 未找到文件
    ReadError,          // 读取错误
    InvalidMagic,       // 魔法数字无效
    InvalidAreaData,    // 区域数据无效
    InvalidHeightData,  // 高度数据无效
    InvalidLiquidData,  // 液体数据无效
    InvalidHoleData     // 空洞数据无效
};

// 网格地形数据类
class GridTerrainData
{
    // 加载区域数据
    bool LoadAreaData(std::ifstream& fileStream, uint32 const offset);
    // 加载高度数据
    bool LoadHeightData(std::ifstream& fileStream, uint32 const offset);
    // 加载液体数据
    bool LoadLiquidData(std::ifstream& fileStream, uint32 const offset);
    // 加载空洞数据
    bool LoadHolesData(std::ifstream& fileStream, uint32 const offset);

    std::unique_ptr<LoadedAreaData> _loadedAreaData;     // 指向已加载区域数据的智能指针
    std::unique_ptr<LoadedHeightData> _loadedHeightData; // 指向已加载高度数据的智能指针
    std::unique_ptr<LoadedLiquidData> _loadedLiquidData; // 指向已加载液体数据的智能指针
    std::unique_ptr<LoadedHoleData> _loadedHoleData;     // 指向已加载空洞数据的智能指针

    // 判断指定位置是否为空洞
    bool isHole(int row, int col) const;

    // 获取高度函数指针类型
    typedef float (GridTerrainData::* GetHeightPtr) (float x, float y) const;
    GetHeightPtr _gridGetHeight; // 高度获取函数指针

    // 从浮点数数据获取高度
    float getHeightFromFloat(float x, float y) const;
    // 从16位无符号整数数据获取高度
    float getHeightFromUint16(float x, float y) const;
    // 从8位无符号整数数据获取高度
    float getHeightFromUint8(float x, float y) const;
    // 从平面数据获取高度
    float getHeightFromFlat(float x, float y) const;

public:
    GridTerrainData(); // 构造函数
    ~GridTerrainData() { }; // 析构函数
    // 加载地图文件
    TerrainMapDataReadResult Load(std::string const& mapFileName);

    // 获取指定位置的区域ID
    uint16 getArea(float x, float y) const;
    // 获取指定位置的高度
    inline float getHeight(float x, float y) const { return (this->*_gridGetHeight)(x, y); }
    // 获取指定位置的最小高度
    float getMinHeight(float x, float y) const;
    // 获取指定位置的液体高度
    float getLiquidLevel(float x, float y) const;
    // 获取指定位置的液体数据
    LiquidData const GetLiquidData(float x, float y, float z, float collisionHeight, uint8 ReqLiquidType) const;
};

#endif
