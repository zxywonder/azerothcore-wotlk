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

#ifndef _IVMAPMANAGER_H
#define _IVMAPMANAGER_H

#include "Define.h"
#include "ModelIgnoreFlags.h"
#include "Optional.h"
#include <string>

//===========================================================

/**
 * 这是 VMapMamager 的最小接口。
 */

namespace VMAP
{
    // VMap 加载结果枚举
    enum VMAP_LOAD_RESULT
    {
        VMAP_LOAD_RESULT_ERROR,  // 加载出错
        VMAP_LOAD_RESULT_OK,     // 加载成功
        VMAP_LOAD_RESULT_IGNORED // 加载被忽略
    };

    // 加载结果枚举类，使用 uint8 作为底层类型
    enum class LoadResult : uint8
    {
        Success,         // 成功
        FileNotFound,    // 文件未找到
        VersionMismatch  // 版本不匹配
    };

    // 用于检查的无效高度值
    #define VMAP_INVALID_HEIGHT       -100000.0f            // for check
    // 在未知高度情况下实际赋值的无效高度值
    #define VMAP_INVALID_HEIGHT_VALUE -200000.0f            // real assigned value in unknown height case

    // 区域和液体数据结构体
    struct AreaAndLiquidData
    {
        // 区域信息结构体
        struct AreaInfo
        {
            // 构造函数
            AreaInfo(int32 _adtId, int32 _rootId, int32 _groupId, uint32 _flags)
                : adtId(_adtId), rootId(_rootId), groupId(_groupId), mogpFlags(_flags) { }
            int32 const  adtId;     // ADT ID
            int32 const  rootId;    // 根 ID
            int32 const  groupId;   // 组 ID
            uint32 const mogpFlags; // MOGP 标志
        };

        // 液体信息结构体
        struct LiquidInfo
        {
            // 构造函数
            LiquidInfo(uint32 _type, float _level)
                : type(_type), level(_level) {}
            uint32 const type;  // 液体类型
            float const  level; // 液体高度
        };

        float                floorZ = VMAP_INVALID_HEIGHT; // 地面高度，默认为无效值
        Optional<AreaInfo>   areaInfo;                    // 可选的区域信息
        Optional<LiquidInfo> liquidInfo;                  // 可选的液体信息
    };

    //===========================================================
    // VMap 管理器接口类
    class IVMapMgr
    {
    private:
        bool iEnableLineOfSightCalc{true}; // 是否启用视线计算，默认启用
        bool iEnableHeightCalc{true};      // 是否启用高度计算，默认启用

    public:
        // 构造函数
        IVMapMgr()  { }

        // 析构函数，声明为虚函数并默认实现
        virtual ~IVMapMgr() = default;

        // 加载地图
        // pBasePath: 地图文件基础路径
        // pMapId: 地图 ID
        // x: 地图 X 坐标
        // y: 地图 Y 坐标
        virtual int loadMap(const char* pBasePath, unsigned int pMapId, int x, int y) = 0;

        // 检查地图是否存在
        // pBasePath: 地图文件基础路径
        // pMapId: 地图 ID
        // x: 地图 X 坐标
        // y: 地图 Y 坐标
        virtual LoadResult existsMap(const char* pBasePath, unsigned int pMapId, int x, int y) = 0;

        // 卸载指定坐标的地图
        // pMapId: 地图 ID
        // x: 地图 X 坐标
        // y: 地图 Y 坐标
        virtual void unloadMap(unsigned int pMapId, int x, int y) = 0;

        // 卸载指定 ID 的所有地图
        // pMapId: 地图 ID
        virtual void unloadMap(unsigned int pMapId) = 0;

        // 检查两点之间是否有视线
        // pMapId: 地图 ID
        // x1, y1, z1: 起点坐标
        // x2, y2, z2: 终点坐标
        // ignoreFlags: 模型忽略标志
        virtual bool isInLineOfSight(unsigned int pMapId, float x1, float y1, float z1, float x2, float y2, float z2, ModelIgnoreFlags ignoreFlags) = 0;

        // 获取指定位置的高度
        // pMapId: 地图 ID
        // x, y, z: 位置坐标
        // maxSearchDist: 最大搜索距离
        virtual float getHeight(unsigned int pMapId, float x, float y, float z, float maxSearchDist) = 0;
        /**
         * 测试是否命中一个物体。如果命中则返回 true。
         * rx, ry, rz 将保存命中位置，如果未找到交点则保存目标位置。
         * 返回一个距离原点近 pReduceDist 的位置。
         */
        virtual bool GetObjectHitPos(unsigned int pMapId, float x1, float y1, float z1, float x2, float y2, float z2, float& rx, float& ry, float& rz, float pModifyDist) = 0;
        /**
         * 发送调试命令
         */
        virtual bool processCommand(char* pCommand) = 0;

        /**
         * 启用/禁用视线计算
         * 默认情况下是启用的。如果在游戏中途启用，地图必须手动加载
         */
        void setEnableLineOfSightCalc(bool pVal) { iEnableLineOfSightCalc = pVal; }
        /**
         * 启用/禁用模型高度计算
         * 默认情况下是启用的。如果在游戏中途启用，地图必须手动加载
         */
        void setEnableHeightCalc(bool pVal) { iEnableHeightCalc = pVal; }

        // 获取视线计算是否启用
        [[nodiscard]] bool isLineOfSightCalcEnabled() const { return (iEnableLineOfSightCalc); }
        // 获取高度计算是否启用
        [[nodiscard]] bool isHeightCalcEnabled() const { return (iEnableHeightCalc); }
        // 获取地图加载是否启用
        [[nodiscard]] bool isMapLoadingEnabled() const { return (iEnableLineOfSightCalc || iEnableHeightCalc  ); }

        // 获取指定地图坐标的目录文件名
        [[nodiscard]] virtual std::string getDirFileName(unsigned int pMapId, int x, int y) const = 0;
        /**
         * 查询世界模型区域信息
         * \param z 会被调整为该区域信息有效的地面高度
         */
        virtual bool GetAreaInfo(uint32 pMapId, float x, float y, float& z, uint32& flags, int32& adtId, int32& rootId, int32& groupId) const      = 0;
        // 获取指定位置的液体高度
        virtual bool GetLiquidLevel(uint32 pMapId, float x, float y, float z, uint8 ReqLiquidType, float& level, float& floor, uint32& type, uint32& mogpFlags) const = 0;
        // 在单次 VMap 查找中获取区域和液体数据
        virtual void GetAreaAndLiquidData(uint32 mapId, float x, float y, float z, uint8 reqLiquidType, AreaAndLiquidData& data) const = 0;
    };
}

#endif
