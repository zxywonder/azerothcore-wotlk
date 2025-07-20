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

#ifndef _DYNTREE_H
#define _DYNTREE_H

#include "Define.h"

namespace G3D
{
    class Ray;
    class Vector3;
}

namespace VMAP
{
    // 存储区域和液体数据的结构体
    struct AreaAndLiquidData;
    // 定义模型忽略标志的枚举类，用于控制射线检测时忽略某些模型部分
    enum class ModelIgnoreFlags : uint32;
}

// 表示游戏对象的模型类
class GameObjectModel;
// 动态树实现的前向声明
struct DynTreeImpl;

// 动态地图树类，用于管理动态对象的碰撞检测和查询
class DynamicMapTree
{
    // 指向实际实现的指针
    DynTreeImpl* impl;

public:
    // 构造函数，初始化动态地图树
    DynamicMapTree();
    // 析构函数，释放动态地图树资源
    ~DynamicMapTree();

    // 判断两点之间是否通视（视线是否被阻挡）
    [[nodiscard]] bool isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2, uint32 phasemask, VMAP::ModelIgnoreFlags ignoreFlags) const;

    // 获取射线与物体的交点时间（用于碰撞检测）
    bool GetIntersectionTime(uint32 phasemask, const G3D::Ray& ray, const G3D::Vector3& endPos, float& maxDist) const;

    // 获取指定位置的区域信息
    bool GetAreaInfo(float x, float y, float& z, uint32 phasemask, uint32& flags, int32& adtId, int32& rootId, int32& groupId) const;
    // 获取指定位置的区域和液体数据
    void GetAreaAndLiquidData(float x, float y, float z, uint32 phasemask, uint8 reqLiquidType, VMAP::AreaAndLiquidData& data) const;

    // 获取两个点之间的碰撞位置
    bool GetObjectHitPos(uint32 phasemask, const G3D::Vector3& pPos1,
                         const G3D::Vector3& pPos2, G3D::Vector3& pResultHitPos,
                         float pModifyDist) const;

    // 获取指定位置上方/下方的地表高度
    [[nodiscard]] float getHeight(float x, float y, float z, float maxSearchDist, uint32 phasemask) const;

    // 插入一个游戏对象模型到动态树中
    void insert(const GameObjectModel&);
    // 从动态树中移除一个游戏对象模型
    void remove(const GameObjectModel&);
    // 检查动态树是否包含指定的游戏对象模型
    [[nodiscard]] bool contains(const GameObjectModel&) const;
    // 返回动态树中包含的游戏对象模型数量
    [[nodiscard]] int size() const;

    // 平衡动态树结构（优化碰撞检测性能）
    void balance();
    // 更新动态树状态，处理模型更新等逻辑
    void update(uint32 diff);
};

#endif // _DYNTREE_H