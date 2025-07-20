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

#ifndef _BIH_WRAP
#define _BIH_WRAP

#include "BoundingIntervalHierarchy.h"
#include "G3D/Array.h"
#include "G3D/Set.h"
#include "G3D/Table.h"

template<class T, class BoundsFunc = BoundsTrait<T>>
class BIHWrap
{
    // 定义一个模板回调结构体，用于处理射线与点的相交检测
    template<class RayCallback>
    struct MDLCallback
    {
        const T* const* objects;          // 对象指针数组
        RayCallback& _callback;           // 用户定义的回调函数引用
        uint32 objects_size;              // 对象数量

        // 构造函数，初始化成员变量
        MDLCallback(RayCallback& callback, const T* const* objects_array, uint32 objects_size )
            : objects(objects_array), _callback(callback), objects_size(objects_size) { }

        /// Intersect ray
        // 重载运算符，用于射线相交检测
        bool operator() (const G3D::Ray& ray, uint32 idx, float& maxDist, bool stopAtFirstHit)
        {
            if (idx >= objects_size)
            {
                return false;
            }
            if (const T* obj = objects[idx])
            {
                return _callback(ray, *obj, maxDist, stopAtFirstHit);
            }
            return false;
        }

        /// Intersect point
        // 重载运算符，用于点相交检测
        void operator() (const G3D::Vector3& p, uint32 idx)
        {
            if (idx >= objects_size)
            {
                return;
            }
            if (const T* obj = objects[idx])
            {
                _callback(p, *obj);
            }
        }
    };

    // 定义对象数组类型
    typedef G3D::Array<const T*> ObjArray;

    BIH m_tree;                               // BIH树结构
    ObjArray m_objects;                       // 对象数组
    G3D::Table<const T*, uint32> m_obj2Idx;   // 对象到索引的映射表
    G3D::Set<const T*> m_objects_to_push;     // 待插入的对象集合
    int unbalanced_times;                     // 未平衡操作次数计数器

public:
    // 构造函数，初始化未平衡次数为0
    BIHWrap() : unbalanced_times(0) { }

    // 插入对象到集合中，标记为待平衡
    void insert(const T& obj)
    {
        ++unbalanced_times;
        m_objects_to_push.insert(&obj);
    }

    // 从集合中移除对象，标记为待平衡
    void remove(const T& obj)
    {
        ++unbalanced_times;
        uint32 Idx = 0;
        const T* temp;
        if (m_obj2Idx.getRemove(&obj, temp, Idx))
        {
            m_objects[Idx] = nullptr;
        }
        else
        {
            m_objects_to_push.remove(&obj);
        }
    }

    // 平衡BIH树，将待插入和待移除的对象同步到树中
    void balance()
    {
        if (unbalanced_times == 0)
        {
            return;
        }

        unbalanced_times = 0;
        m_objects.fastClear();                     // 清空当前对象数组
        m_obj2Idx.getKeys(m_objects);              // 获取所有对象键值
        m_objects_to_push.getMembers(m_objects);   // 获取待插入对象并添加到数组中
        // 注意：此处应确保m_obj2Idx包含所有对象键

        // 构建BIH树，使用BoundsFunc获取对象的包围盒
        m_tree.build(m_objects, BoundsFunc::GetBounds2);
    }

    // 射线相交检测接口
    template<typename RayCallback>
    void intersectRay(const G3D::Ray& ray, RayCallback& intersectCallback, float& maxDist, bool stopAtFirstHit)
    {
        balance();  // 确保树结构是最新的
        // 创建回调适配器
        MDLCallback<RayCallback> temp_cb(intersectCallback, m_objects.getCArray(), m_objects.size());
        // 调用BIH树的射线检测方法
        m_tree.intersectRay(ray, temp_cb, maxDist, stopAtFirstHit);
    }

    // 点相交检测接口
    template<typename IsectCallback>
    void intersectPoint(const G3D::Vector3& point, IsectCallback& intersectCallback)
    {
        balance();  // 确保树结构是最新的
        // 创建回调适配器
        MDLCallback<IsectCallback> callback(intersectCallback, m_objects.getCArray(), m_objects.size());
        // 调用BIH树的点检测方法
        m_tree.intersectPoint(point, callback);
    }
};

#endif // _BIH_WRAP