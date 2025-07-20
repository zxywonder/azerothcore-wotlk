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

 #ifndef _MAPDEFINES_H
 #define _MAPDEFINES_H
 
 #include "Define.h"
 #include "DetourNavMesh.h"
 
 // 定义地图中网格的最大数量
 #define MAX_NUMBER_OF_GRIDS      64
 // 定义每个网格中单元格的最大数量
 #define MAX_NUMBER_OF_CELLS      8
 // 每个网格的大小（单位：游戏坐标单位）
 #define SIZE_OF_GRIDS            533.3333f
 
 // MMAP文件格式的魔数标识符，对应ASCII字符 'MMAP'
 #define MMAP_MAGIC 0x4d4d4150   // 'MMAP'
 // MMAP文件版本号
 #define MMAP_VERSION 16
 
 // MMAP瓦片文件的头部结构体
 struct MmapTileHeader
 {
     uint32 mmapMagic{MMAP_MAGIC};       // 魔数标识符
     uint32 dtVersion;                   // Detour库的版本号
     uint32 mmapVersion{MMAP_VERSION};   // MMAP文件版本号
     uint32 size{0};                     // 瓦片数据的大小
     char usesLiquids{true};             // 是否包含液体信息
     char padding[3] {};                 // 填充字段，用于对齐
 
     // 构造函数，初始化Detour版本号
     MmapTileHeader() : dtVersion(DT_NAVMESH_VERSION) { }
 };
 
 // 静态断言，确保MmapTileHeader结构体大小为20字节，否则调整填充字段
 static_assert(sizeof(MmapTileHeader) == 20, "MmapTileHeader size is not correct, adjust the padding field size");
 // 静态断言，确保MmapTileHeader结构体中所有字段（包括填充字段）都被正确初始化
 static_assert(sizeof(MmapTileHeader) == (sizeof(MmapTileHeader::mmapMagic) +
               sizeof(MmapTileHeader::dtVersion) +
               sizeof(MmapTileHeader::mmapVersion) +
               sizeof(MmapTileHeader::size) +
               sizeof(MmapTileHeader::usesLiquids) +
               sizeof(MmapTileHeader::padding)), "MmapTileHeader has uninitialized padding fields");
 
 // 导航地形类型枚举，用于表示不同类型的地形属性
 enum NavTerrain
 {
     NAV_EMPTY   = 0x00,  // 空地形
     NAV_GROUND  = 0x01,  // 地面
     NAV_MAGMA   = 0x02,  // 岩浆
     NAV_SLIME   = 0x04,  // 毒液
     NAV_WATER   = 0x08,  // 水
     NAV_UNUSED1 = 0x10,  // 未使用
     NAV_UNUSED2 = 0x20,  // 未使用
     NAV_UNUSED3 = 0x40,  // 未使用
     NAV_UNUSED4 = 0x80   // 未使用，总共8位
 };
 
 #endif /* _MAPDEFINES_H */
