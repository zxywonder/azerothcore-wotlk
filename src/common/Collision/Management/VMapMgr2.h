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

#ifndef _VMAPMANAGER2_H
#define _VMAPMANAGER2_H

#include "IVMapMgr.h"
#include <mutex>
#include <unordered_map>
#include <vector>

//===========================================================
// 定义地图文件名的扩展名
#define MAP_FILENAME_EXTENSION2 ".vmtree"

// 定义文件名缓冲区的大小
#define FILENAMEBUFFER_SIZE 500

/**
This is the main Class to manage loading and unloading of maps, line of sight, height calculation and so on.
For each map or map tile to load it reads a directory file that contains the ModelContainer files used by this map or map tile.
Each global map or instance has its own dynamic BSP-Tree.
The loaded ModelContainers are included in one of these BSP-Trees.
Additionally a table to match map ids and map names is used.
翻译：这是用于管理地图加载和卸载、视线检查、高度计算等功能的主要类。
对于每个要加载的地图或地图图块，它会读取一个目录文件，该文件包含此地图或图块使用的模型容器文件。
每个全局地图或实例都有自己的动态 BSP 树。加载的模型容器会被包含在其中一个 BSP 树中。
此外，还会使用一个表来匹配地图 ID 和地图名称。
*/

//===========================================================

namespace G3D
{
    // 声明 Vector3 类
    class Vector3;
}

namespace VMAP
{
    // 声明 StaticMapTree 类
    class StaticMapTree;
    // 声明 WorldModel 类
    class WorldModel;

    // 管理模型的类
    class ManagedModel
    {
    public:
        // 构造函数
        ManagedModel()  { }
        // 设置模型指针
        void setModel(WorldModel* model) { iModel = model; }
        // 获取模型指针
        WorldModel* getModel() { return iModel; }
        // 减少引用计数并返回新的引用计数
        int decRefCount() { return --iRefCount; }
    protected:
        // 模型指针，初始化为 nullptr
        WorldModel* iModel{nullptr};
        // 模型引用计数，初始化为 0
        int iRefCount{0};
    };

    // 定义实例树映射类型，键为 uint32 类型的 ID，值为 StaticMapTree 指针
    typedef std::unordered_map<uint32, StaticMapTree*> InstanceTreeMap;
    // 定义模型文件映射类型，键为字符串类型的文件名，值为 ManagedModel 对象
    typedef std::unordered_map<std::string, ManagedModel> ModelFileMap;

    // 定义禁用类型的枚举
    enum DisableTypes
    {
        // 禁用区域标志
        VMAP_DISABLE_AREAFLAG       = 0x1,
        // 禁用高度计算
        VMAP_DISABLE_HEIGHT         = 0x2,
        // 禁用视线检查
        VMAP_DISABLE_LOS            = 0x4,
        // 禁用液体状态检查
        VMAP_DISABLE_LIQUIDSTATUS   = 0x8
    };

    // VMap 管理器类，继承自 IVMapMgr
    class VMapMgr2 : public IVMapMgr
    {
    protected:
        // 用于检查碰撞的已加载模型文件映射
        ModelFileMap iLoadedModelFiles;
        // 实例地图树映射
        InstanceTreeMap iInstanceMapTrees;
        // 是否为线程安全环境
        bool thread_safe_environment;

        // 用于保护 iLoadedModelFiles 的互斥锁
        std::mutex LoadedModelFilesLock;

        // 加载地图的内部方法
        bool _loadMap(uint32 mapId, const std::string& basePath, uint32 tileX, uint32 tileY);
        /* 卸载地图的内部方法 */
        /* void _unloadMap(uint32 pMapId, uint32 x, uint32 y); */

        // 获取液体标志的虚拟方法
        static uint32 GetLiquidFlagsDummy(uint32) { return 0; }
        // 检查 VMAP 是否被禁用的虚拟方法
        static bool IsVMAPDisabledForDummy(uint32 /*entry*/, uint8 /*flags*/) { return false; }

        // 根据地图 ID 获取地图树的常量迭代器
        InstanceTreeMap::const_iterator GetMapTree(uint32 mapId) const;

    public:
        // 用于调试的公共方法，将位置转换为内部表示
        [[nodiscard]] G3D::Vector3 convertPositionToInternalRep(float x, float y, float z) const;
        // 根据地图 ID 获取地图文件名
        static std::string getMapFileName(unsigned int mapId);

        // 构造函数
        VMapMgr2();
        // 析构函数
        ~VMapMgr2() override;

        // 初始化非线程安全环境下的地图
        void InitializeThreadUnsafe(const std::vector<uint32>& mapIds);

        // 加载地图
        int loadMap(const char* pBasePath, unsigned int mapId, int x, int y) override;

        // 卸载指定位置的地图
        void unloadMap(unsigned int mapId, int x, int y) override;
        // 卸载指定 ID 的地图
        void unloadMap(unsigned int mapId) override;

        // 检查两点之间是否有视线
        bool isInLineOfSight(unsigned int mapId, float x1, float y1, float z1, float x2, float y2, float z2, ModelIgnoreFlags ignoreFlags) override ;
        /**
        fill the hit pos and return true, if an object was hit
        翻译：如果命中了对象，填充命中位置并返回 true
        */
        bool GetObjectHitPos(unsigned int mapId, float x1, float y1, float z1, float x2, float y2, float z2, float& rx, float& ry, float& rz, float modifyDist) override;
        // 获取指定位置的高度
        float getHeight(unsigned int mapId, float x, float y, float z, float maxSearchDist) override;

        // 处理命令，用于调试和扩展
        bool processCommand(char* /*command*/) override { return false; } 

        // 获取指定位置的区域信息
        bool GetAreaInfo(uint32 pMapId, float x, float y, float& z, uint32& flags, int32& adtId, int32& rootId, int32& groupId) const override;
        // 获取指定位置的液体高度信息
        bool GetLiquidLevel(uint32 pMapId, float x, float y, float z, uint8 reqLiquidType, float& level, float& floor, uint32& type, uint32& mogpFlags) const override;
        // 获取指定位置的区域和液体数据
        void GetAreaAndLiquidData(uint32 mapId, float x, float y, float z, uint8 reqLiquidType, AreaAndLiquidData& data) const override;

        // 获取模型实例
        WorldModel* acquireModelInstance(const std::string& basepath, const std::string& filename, uint32 flags);
        // 释放模型实例
        void releaseModelInstance(const std::string& filename);

        // 这个方法的用途是什么？ o.O
        [[nodiscard]] std::string getDirFileName(unsigned int mapId, int /*x*/, int /*y*/) const override
        {
            return getMapFileName(mapId);
        }
        // 检查地图是否存在
        LoadResult existsMap(const char* basePath, unsigned int mapId, int x, int y) override;
        // 获取实例地图树
        void GetInstanceMapTree(InstanceTreeMap& instanceMapTree);

        // 定义获取液体标志的函数指针类型
        typedef uint32(*GetLiquidFlagsFn)(uint32 liquidType);
        // 获取液体标志的函数指针
        GetLiquidFlagsFn GetLiquidFlagsPtr;

        // 定义检查 VMAP 是否被禁用的函数指针类型
        typedef bool(*IsVMAPDisabledForFn)(uint32 entry, uint8 flags);
        // 检查 VMAP 是否被禁用的函数指针
        IsVMAPDisabledForFn IsVMAPDisabledForPtr;
    };
}

#endif
