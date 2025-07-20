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

#include "DynamicTree.h"
#include "BoundingIntervalHierarchyWrapper.h"
#include "GameObjectModel.h"
#include "MapTree.h"
#include "ModelIgnoreFlags.h"
#include "ModelInstance.h"
#include "RegularGrid.h"
#include "Timer.h"
#include "VMapFactory.h"
#include "VMapMgr2.h"
#include "WorldModel.h"

#include <G3D/AABox.h>
#include <G3D/Ray.h>
#include <G3D/Vector3.h>

using VMAP::ModelInstance;

namespace
{
    // 检查树结构是否需要平衡的时间间隔（单位：毫秒）
    int CHECK_TREE_PERIOD = 200;
}

// 为GameObjectModel定义哈希函数的特化模板
template<> struct HashTrait< GameObjectModel>
{
    static std::size_t hashCode(const GameObjectModel& g) { return (size_t)(void*)&g; }
};

// 为GameObjectModel定义位置获取函数的特化模板
template<> struct PositionTrait< GameObjectModel>
{
    static void GetPosition(const GameObjectModel& g, G3D::Vector3& p) { p = g.GetPosition(); }
};

// 为GameObjectModel定义包围盒获取函数的特化模板
template<> struct BoundsTrait< GameObjectModel>
{
    static void GetBounds(const GameObjectModel& g, G3D::AABox& out) { out = g.GetBounds();}
    static void GetBounds2(const GameObjectModel* g, G3D::AABox& out) { out = g->GetBounds();}
};

// 定义基于RegularGrid和BIH的父树类型
typedef RegularGrid2D<GameObjectModel, BIHWrap<GameObjectModel>> ParentTree;

// 动态树实现类，继承自ParentTree
struct DynTreeImpl : public ParentTree
{
    typedef GameObjectModel Model;
    typedef ParentTree base;

    // 构造函数，初始化定时器和不平衡计数器
    DynTreeImpl() :
        rebalance_timer(CHECK_TREE_PERIOD),
        unbalanced_times(0)
    {
    }

    // 插入游戏对象模型到树中
    void insert(const Model& mdl)
    {
        base::insert(mdl);
        ++unbalanced_times;
    }

    // 从树中移除游戏对象模型
    void remove(const Model& mdl)
    {
        base::remove(mdl);
        ++unbalanced_times;
    }

    // 平衡树结构，优化查询性能
    void balance()
    {
        base::balance();
        unbalanced_times = 0;
    }

    // 定时更新函数，检查是否需要平衡树结构
    void update(uint32 difftime)
    {
        if (!size())
        {
            return;
        }

        rebalance_timer.Update(difftime);
        if (rebalance_timer.Passed())
        {
            rebalance_timer.Reset(CHECK_TREE_PERIOD);
            if (unbalanced_times > 0)
            {
                balance();
            }
        }
    }

    // 树结构平衡定时器
    TimeTrackerSmall rebalance_timer;
    // 树不平衡次数计数器
    int unbalanced_times;
};

// 构造函数，初始化动态地图树实现
DynamicMapTree::DynamicMapTree() : impl(new DynTreeImpl()) { }

// 析构函数，释放动态地图树实现
DynamicMapTree::~DynamicMapTree()
{
    delete impl;
}

// 插入游戏对象模型到动态地图树中
void DynamicMapTree::insert(const GameObjectModel& mdl)
{
    impl->insert(mdl);
}

// 从动态地图树中移除游戏对象模型
void DynamicMapTree::remove(const GameObjectModel& mdl)
{
    impl->remove(mdl);
}

// 检查动态地图树是否包含指定的游戏对象模型
bool DynamicMapTree::contains(const GameObjectModel& mdl) const
{
    return impl->contains(mdl);
}

// 平衡动态地图树结构
void DynamicMapTree::balance()
{
    impl->balance();
}

// 获取动态地图树中游戏对象模型的数量
int DynamicMapTree::size() const
{
    return impl->size();
}

// 更新动态地图树状态
void DynamicMapTree::update(uint32 t_diff)
{
    impl->update(t_diff);
}

// 动态树交集回调类，用于射线检测
struct DynamicTreeIntersectionCallback
{
    // 构造函数，初始化相位掩码和忽略标志
    DynamicTreeIntersectionCallback(uint32 phasemask, VMAP::ModelIgnoreFlags ignoreFlags) :
        _didHit(false), _phaseMask(phasemask), _ignoreFlags(ignoreFlags) { }

    // 射线检测回调函数，检测对象是否与射线相交
    bool operator()(const G3D::Ray& r, const GameObjectModel& obj, float& distance, bool stopAtFirstHit)
    {
        bool result = obj.intersectRay(r, distance, stopAtFirstHit, _phaseMask, _ignoreFlags);
        if (result)
        {
            _didHit = result;
        }
        return result;
    }

    // 返回是否检测到碰撞
    [[nodiscard]] bool didHit() const
    {
        return _didHit;
    }

private:
    bool _didHit;
    uint32 _phaseMask;
    VMAP::ModelIgnoreFlags _ignoreFlags;
};

// 动态树区域信息回调类，用于获取区域信息
struct DynamicTreeAreaInfoCallback
{
    // 构造函数，初始化相位掩码
    DynamicTreeAreaInfoCallback(uint32 phaseMask) : _phaseMask(phaseMask) { }

    // 点检测回调函数，获取对象的区域信息
    void operator()(G3D::Vector3 const& p, GameObjectModel const& obj)
    {
        obj.IntersectPoint(p, _areaInfo, _phaseMask);
    }

    // 获取区域信息
    VMAP::AreaInfo const& GetAreaInfo() const
    {
        return _areaInfo;
    }

private:
    uint32 _phaseMask;
    VMAP::AreaInfo _areaInfo;
};

// 动态树位置信息回调类，用于获取位置信息
struct DynamicTreeLocationInfoCallback
{
    // 构造函数，初始化相位掩码
    DynamicTreeLocationInfoCallback(uint32 phaseMask)
        : _phaseMask(phaseMask), _hitModel(nullptr) {}

    // 点检测回调函数，获取对象的位置信息
    void operator()(G3D::Vector3 const& p, GameObjectModel const& obj)
    {
        if (obj.GetLocationInfo(p, _locationInfo, _phaseMask))
            _hitModel = &obj;
    }

    // 获取位置信息
    VMAP::LocationInfo& GetLocationInfo()
    {
        return _locationInfo;
    }

    // 获取命中模型
    GameObjectModel const* GetHitModel() const
    {
        return _hitModel;
    }

private:
    uint32                 _phaseMask;
    VMAP::LocationInfo     _locationInfo;
    GameObjectModel const* _hitModel;
};

// 获取射线与物体的交点时间（用于碰撞检测）
bool DynamicMapTree::GetIntersectionTime(const uint32 phasemask, const G3D::Ray& ray, const G3D::Vector3& endPos, float& maxDist) const
{
    float distance = maxDist;
    DynamicTreeIntersectionCallback callback(phasemask, VMAP::ModelIgnoreFlags::Nothing);
    impl->intersectRay(ray, callback, distance, endPos, false);
    if (callback.didHit())
    {
        maxDist = distance;
    }
    return callback.didHit();
}

// 获取两个点之间的碰撞位置
bool DynamicMapTree::GetObjectHitPos(const uint32 phasemask, const G3D::Vector3& startPos,
                                     const G3D::Vector3& endPos, G3D::Vector3& resultHit,
                                     float modifyDist) const
{
    bool result = false;
    float maxDist = (endPos - startPos).magnitude();
    // 有效地图坐标应永远不会导致浮点溢出，但此检查可防止出现NaN值
    ASSERT(maxDist < std::numeric_limits<float>::max());
    // 防止NaN值导致BIH交集进入无限循环
    if (maxDist < 1e-10f)
    {
        resultHit = endPos;
        return false;
    }
    G3D::Vector3 dir = (endPos - startPos) / maxDist;            // 单位长度的方向向量
    G3D::Ray ray(startPos, dir);
    float dist = maxDist;
    if (GetIntersectionTime(phasemask, ray, endPos, dist))
    {
        resultHit = startPos + dir * dist;
        if (modifyDist < 0)
        {
            if ((resultHit - startPos).magnitude() > -modifyDist)
            {
                resultHit = resultHit + dir * modifyDist;
            }
            else
            {
                resultHit = startPos;
            }
        }
        else
        {
            resultHit = resultHit + dir * modifyDist;
        }

        result = true;
    }
    else
    {
        resultHit = endPos;
        result = false;
    }
    return result;
}

// 判断两点之间是否通视（视线是否被阻挡）
bool DynamicMapTree::isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2, uint32 phasemask, VMAP::ModelIgnoreFlags ignoreFlags) const
{
    G3D::Vector3 v1(x1, y1, z1), v2(x2, y2, z2);

    float maxDist = (v2 - v1).magnitude();

    if (!G3D::fuzzyGt(maxDist, 0))
    {
        return true;
    }

    G3D::Ray r(v1, (v2 - v1) / maxDist);
    DynamicTreeIntersectionCallback callback(phasemask, ignoreFlags);
    impl->intersectRay(r, callback, maxDist, v2, true);

    return !callback.didHit();
}

// 获取指定位置上方/下方的地表高度
float DynamicMapTree::getHeight(float x, float y, float z, float maxSearchDist, uint32 phasemask) const
{
    G3D::Vector3 v(x, y, z);
    G3D::Ray r(v, G3D::Vector3(0, 0, -1));
    DynamicTreeIntersectionCallback callback(phasemask, VMAP::ModelIgnoreFlags::Nothing);
    impl->intersectZAllignedRay(r, callback, maxSearchDist);

    if (callback.didHit())
    {
        return v.z - maxSearchDist;
    }
    else
    {
        return -G3D::finf();
    }
}

// 获取指定位置的区域信息
bool DynamicMapTree::GetAreaInfo(float x, float y, float& z, uint32 phasemask, uint32& flags, int32& adtId, int32& rootId, int32& groupId) const
{
    G3D::Vector3 v(x, y, z + 0.5f);
    DynamicTreeAreaInfoCallback intersectionCallBack(phasemask);
    impl->intersectPoint(v, intersectionCallBack);
    if (intersectionCallBack.GetAreaInfo().result)
    {
        flags = intersectionCallBack.GetAreaInfo().flags;
        adtId = intersectionCallBack.GetAreaInfo().adtId;
        rootId = intersectionCallBack.GetAreaInfo().rootId;
        groupId = intersectionCallBack.GetAreaInfo().groupId;
        z = intersectionCallBack.GetAreaInfo().ground_Z;
        return true;
    }
    return false;
}

// 获取指定位置的区域和液体数据
void DynamicMapTree::GetAreaAndLiquidData(float x, float y, float z, uint32 phasemask, uint8 reqLiquidType, VMAP::AreaAndLiquidData& data) const
{
    G3D::Vector3 v(x, y, z + 0.5f);
    DynamicTreeLocationInfoCallback intersectionCallBack(phasemask);
    impl->intersectPoint(v, intersectionCallBack);
    if (intersectionCallBack.GetLocationInfo().hitModel)
    {
        data.floorZ = intersectionCallBack.GetLocationInfo().ground_Z;
        uint32 liquidType = intersectionCallBack.GetLocationInfo().hitModel->GetLiquidType();
        float liquidLevel;
        if (!reqLiquidType || (dynamic_cast<VMAP::VMapMgr2*>(VMAP::VMapFactory::createOrGetVMapMgr())->GetLiquidFlagsPtr(liquidType) & reqLiquidType))
            if (intersectionCallBack.GetHitModel()->GetLiquidLevel(v, intersectionCallBack.GetLocationInfo(), liquidLevel))
                data.liquidInfo.emplace(liquidType, liquidLevel);

        data.areaInfo.emplace(0,
            intersectionCallBack.GetLocationInfo().rootId,
            intersectionCallBack.GetLocationInfo().hitModel->GetWmoID(),
            intersectionCallBack.GetLocationInfo().hitModel->GetMogpFlags());
    }
}