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

// 防止头文件重复包含
#ifndef _MAPTREE_H
#define _MAPTREE_H

// 包含边界区间层次结构头文件
#include "BoundingIntervalHierarchy.h"
// 包含定义文件头文件
#include "Define.h"
// 包含无序映射容器头文件
#include <unordered_map>

namespace VMAP
{
    // 前向声明模型实例类
    class ModelInstance;
    // 前向声明组模型类
    class GroupModel;
    // 前向声明VMap管理器类
    class VMapMgr2;
    // 定义模型忽略标志枚举，使用uint32作为底层类型
    enum class ModelIgnoreFlags : uint32;
    // 定义加载结果枚举，使用uint8作为底层类型
    enum class LoadResult : uint8;

    // 定义位置信息结构体
    struct LocationInfo
    {
        // 构造函数，初始化地面高度为负无穷
        LocationInfo():  ground_Z(-G3D::inf()) { }
        // 命中的模型实例指针
        const ModelInstance* hitInstance{nullptr};
        // 命中的组模型指针
        const GroupModel* hitModel{nullptr};
        // 地面高度
        float ground_Z;
        // 根ID，初始化为-1
        int32 rootId = -1;
    };

    // 定义静态地图树类
    class StaticMapTree
    {
        // 定义已加载地图块的映射类型，键为地图块ID，值为布尔值
        typedef std::unordered_map<uint32, bool> loadedTileMap;
        // 定义已加载生成点的映射类型，键为树索引，值为引用计数
        typedef std::unordered_map<uint32, uint32> loadedSpawnMap;
    private:
        // 地图ID
        uint32 iMapID;
        // 地图是否分块标志
        bool iIsTiled;
        // 边界区间层次结构对象
        BIH iTree;
        // 树的条目，模型实例指针
        ModelInstance* iTreeValues; // 树的条目
        // 树的条目数量
        uint32 iNTreeValues;

        // 存储该地图所有已加载的地图块ID
        // 有些地图不分块，我们必须确保在所有地图块都被移除之前不删除地图
        // 空地图块没有对应的文件，因此使用带布尔值的映射而不是集合（用于一致性检查）
        loadedTileMap iLoadedTiles;
        // 存储 <树索引, 引用计数> 用于使树值无效、卸载地图以及报告错误
        loadedSpawnMap iLoadedSpawns;
        // 基础路径
        std::string iBasePath;

    private:
        // 获取射线与地图的相交时间
        bool GetIntersectionTime(const G3D::Ray& pRay, float& pMaxDist, bool StopAtFirstHit, ModelIgnoreFlags ignoreFlags) const;
        //bool containsLoadedMapTile(unsigned int pTileIdent) const { return(iLoadedMapTiles.containsKey(pTileIdent)); }
    public:
        // 获取地图块文件名称
        static std::string getTileFileName(uint32 mapID, uint32 tileX, uint32 tileY);
        // 打包地图块ID，将tileX和tileY合并为一个32位无符号整数
        static uint32 packTileID(uint32 tileX, uint32 tileY) { return tileX << 16 | tileY; }
        // 解包地图块ID，将一个32位无符号整数拆分为tileX和tileY
        static void unpackTileID(uint32 ID, uint32& tileX, uint32& tileY) { tileX = ID >> 16; tileY = ID & 0xFF; }
        // 检查是否可以加载指定地图块
        static LoadResult CanLoadMap(const std::string& basePath, uint32 mapID, uint32 tileX, uint32 tileY);

        // 构造函数，初始化静态地图树
        StaticMapTree(uint32 mapID, const std::string& basePath);
        // 析构函数
        ~StaticMapTree();

        // 检查两点之间是否有视线阻挡
        [[nodiscard]] bool isInLineOfSight(const G3D::Vector3& pos1, const G3D::Vector3& pos2, ModelIgnoreFlags ignoreFlags) const;
        // 获取物体的命中位置
        bool GetObjectHitPos(const G3D::Vector3& pos1, const G3D::Vector3& pos2, G3D::Vector3& pResultHitPos, float pModifyDist) const;
        // 获取指定位置的高度
        [[nodiscard]] float getHeight(const G3D::Vector3& pPos, float maxSearchDist) const;
        // 获取指定位置的区域信息
        bool GetAreaInfo(G3D::Vector3& pos, uint32& flags, int32& adtId, int32& rootId, int32& groupId) const;
        // 获取指定位置的位置信息
        bool GetLocationInfo(const G3D::Vector3& pos, LocationInfo& info) const;

        // 初始化地图
        bool InitMap(const std::string& fname, VMapMgr2* vm);
        // 卸载地图
        void UnloadMap(VMapMgr2* vm);
        // 加载指定地图块
        bool LoadMapTile(uint32 tileX, uint32 tileY, VMapMgr2* vm);
        // 卸载指定地图块
        void UnloadMapTile(uint32 tileX, uint32 tileY, VMapMgr2* vm);
        // 判断地图是否分块
        [[nodiscard]] bool isTiled() const { return iIsTiled; }
        // 获取已加载地图块的数量
        [[nodiscard]] uint32 numLoadedTiles() const { return iLoadedTiles.size(); }
        // 获取模型实例和数量
        void GetModelInstances(ModelInstance*& models, uint32& count);
    };

    // 定义区域信息结构体
    struct AreaInfo
    {
        // 构造函数，初始化地面高度为负无穷
        AreaInfo():  ground_Z(-G3D::inf()) { }
        // 结果标志
        bool result{false};
        // 地面高度
        float ground_Z;
        // 标志位
        uint32 flags{0};
        // ADT ID
        int32 adtId{0};
        // 根ID
        int32 rootId{0};
        // 组ID
        int32 groupId{0};
    };
}                                                           // VMAP

// 结束头文件保护
#endif // _MAPTREE_H
