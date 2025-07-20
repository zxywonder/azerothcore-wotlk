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
 * You should have received the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MMAP_MANAGER_H
#define _MMAP_MANAGER_H

#include "Common.h"
#include "DetourAlloc.h"
#include "DetourExtended.h"
#include "DetourNavMesh.h"
#include <unordered_map>
#include <vector>

// 内存管理
inline void* dtCustomAlloc(std::size_t size, dtAllocHint /*hint*/)
{
    return (void*)new unsigned char[size];
}

inline void dtCustomFree(void* ptr)
{
    delete [] (unsigned char*)ptr;
}

// 移动地图相关类
namespace MMAP
{
    // 定义地图瓦片集合类型，键为 uint32 类型，值为 dtTileRef 类型
    typedef std::unordered_map<uint32, dtTileRef> MMapTileSet;
    // 定义导航网格查询集合类型，键为 uint32 类型，值为 dtNavMeshQuery* 类型
    typedef std::unordered_map<uint32, dtNavMeshQuery*> NavMeshQuerySet;

    // 用于保存地图的移动地图数据的结构体
    struct MMapData
    {
        // 构造函数，初始化导航网格指针
        MMapData(dtNavMesh* mesh) : navMesh(mesh) { }

        // 析构函数，释放导航网格查询和导航网格占用的内存
        ~MMapData()
        {
            for (auto& navMeshQuerie : navMeshQueries)
            {
                dtFreeNavMeshQuery(navMeshQuerie.second);
            }

            if (navMesh)
            {
                dtFreeNavMesh(navMesh);
            }
        }

        // 由于 dtNavMeshQuery 不是线程安全的，每个实例需要单独使用一个
        NavMeshQuerySet navMeshQueries; // 实例 ID 到查询对象的映射
        dtNavMesh* navMesh;             // 导航网格指针
        MMapTileSet loadedTileRefs;     // 地图网格坐标到 dtTile 的映射
    };

    // 定义移动地图数据集合类型，键为 uint32 类型，值为 MMapData* 类型
    typedef std::unordered_map<uint32, MMapData*> MMapDataSet;

    // 单例类
    // 负责所有移动地图的加载、卸载和网格访问操作
    class MMapMgr
    {
    public:
        // 默认构造函数
        MMapMgr()  = default;
        // 析构函数
        ~MMapMgr();

        // 非线程安全的初始化函数，初始化指定地图 ID 的移动地图
        void InitializeThreadUnsafe(const std::vector<uint32>& mapIds);
        // 加载指定地图和坐标的移动地图
        bool loadMap(uint32 mapId, int32 x, int32 y);
        // 卸载指定地图和坐标的移动地图
        bool unloadMap(uint32 mapId, int32 x, int32 y);
        // 卸载指定地图的所有移动地图
        bool unloadMap(uint32 mapId);
        // 卸载指定地图和实例 ID 的移动地图
        bool unloadMapInstance(uint32 mapId, uint32 instanceId);

        // 获取指定地图和实例 ID 的导航网格查询对象，返回的对象不是线程安全的
        dtNavMeshQuery const* GetNavMeshQuery(uint32 mapId, uint32 instanceId);
        // 获取指定地图的导航网格
        dtNavMesh const* GetNavMesh(uint32 mapId);

        // 获取已加载的瓦片数量
        [[nodiscard]] uint32 getLoadedTilesCount() const { return loadedTiles; }
        // 获取已加载的地图数量
        [[nodiscard]] uint32 getLoadedMapsCount() const { return loadedMMaps.size(); }

    private:
        // 加载指定地图的移动地图数据
        bool loadMapData(uint32 mapId);
        // 打包瓦片 ID
        uint32 packTileID(int32 x, int32 y);
        // 获取指定地图的移动地图数据迭代器
        [[nodiscard]] MMapDataSet::const_iterator GetMMapData(uint32 mapId) const;

        // 已加载的移动地图数据集合
        MMapDataSet loadedMMaps;
        // 已加载的瓦片数量
        uint32 loadedTiles{0};
        // 是否处于线程安全的环境
        bool thread_safe_environment{true};
    };
}

#endif
