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

#ifndef _TILEASSEMBLER_H_
#define _TILEASSEMBLER_H_

#include <G3D/Matrix3.h>
#include <G3D/Vector3.h>
#include <map>
#include <set>

#include "ModelInstance.h"
#include "WorldModel.h"

namespace VMAP
{
    /**
    This Class is used to convert raw vector data into balanced BSP-Trees.
    To start the conversion call convertWorld().
    */
    //===============================================

    // 模型位置类，用于处理模型的位置、旋转和缩放信息
    class ModelPosition
    {
    private:
        // 旋转矩阵
        G3D::Matrix3 iRotation;
    public:
        ModelPosition() { }
        // 模型位置
        G3D::Vector3 iPos;
        // 模型方向
        G3D::Vector3 iDir;
        // 模型缩放比例
        float iScale{0.0f};
        // 初始化旋转矩阵
        void init()
        {
            iRotation = G3D::Matrix3::fromEulerAnglesZYX(G3D::pif() * iDir.y / 180.f, G3D::pif() * iDir.x / 180.f, G3D::pif() * iDir.z / 180.f);
        }
        // 转换向量坐标
        [[nodiscard]] G3D::Vector3 transform(const G3D::Vector3& pIn) const;
        // 将模型位置移动到基准位置
        void moveToBasePos(const G3D::Vector3& pBasePos) { iPos -= pBasePos; }
    };

    // 唯一条目映射类型，键为 uint32，值为 ModelSpawn
    typedef std::map<uint32, ModelSpawn> UniqueEntryMap;
    // 瓦片映射类型，键为 uint32，值为 uint32，允许一个键对应多个值
    typedef std::multimap<uint32, uint32> TileMap;

    // 地图生成信息结构体
    struct MapSpawns
    {
        // 唯一条目映射
        UniqueEntryMap UniqueEntries;
        // 瓦片映射
        TileMap TileEntries;
    };

    // 地图数据类型，键为 uint32，值为 MapSpawns 指针
    typedef std::map<uint32, MapSpawns*> MapData;
    //===============================================

    // 原始组模型结构体
    struct GroupModel_Raw
    {
        // MOGP 标志
        uint32 mogpflags{0};
        // 组 WMO ID
        uint32 GroupWMOID{0};

        // 轴对齐边界框
        G3D::AABox bounds;
        // 液体标志
        uint32 liquidflags{0};
        // 三角形网格数组
        std::vector<MeshTriangle> triangles;
        // 顶点数组
        std::vector<G3D::Vector3> vertexArray;
        // WMO 液体对象指针
        class WmoLiquid* liquid;

        // 构造函数，初始化液体指针为 nullptr
        GroupModel_Raw() : liquid(nullptr) { }

        // 析构函数
        ~GroupModel_Raw();

        // 从文件读取数据
        bool Read(FILE* f);
    };

    // 原始世界模型结构体
    struct WorldModel_Raw
    {
        // 根 WMO ID
        uint32 RootWMOID;
        // 组模型数组
        std::vector<GroupModel_Raw> groupsArray;

        // 从指定路径读取文件
        bool Read(const char* path);
    };

    // 瓦片组装器类
    class TileAssembler
    {
    private:
        // 目标目录
        std::string iDestDir;
        // 源目录
        std::string iSrcDir;
        // 唯一名称 ID 表
        G3D::Table<std::string, unsigned int > iUniqueNameIds;
        // 地图数据
        MapData mapData;
        // 已生成的模型文件集合
        std::set<std::string> spawnedModelFiles;

    public:
        // 构造函数，初始化源目录和目标目录
        TileAssembler(const std::string& pSrcDirName, const std::string& pDestDirName);
        // 析构函数
        virtual ~TileAssembler();

        // 转换世界数据
        bool convertWorld2();
        // 读取地图生成信息
        bool readMapSpawns();
        // 计算转换后的边界
        bool calculateTransformedBound(ModelSpawn& spawn);
        // 导出游戏对象模型
        void exportGameobjectModels();

        // 转换原始文件
        bool convertRawFile(const std::string& pModelFilename);
    };

}                                                           // VMAP
#endif                                                      /*_TILEASSEMBLER_H_*/
