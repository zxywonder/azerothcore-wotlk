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

#ifndef _VMAPTOOLS_H
#define _VMAPTOOLS_H

#include <G3D/AABox.h>
#include <G3D/CollisionDetection.h>

/**
The Class is mainly taken from G3D/AABSPTree.h but modified to be able to use our internal data structure.
This is an iterator that helps us analysing the BSP-Trees.
The collision detection is modified to return true, if we are inside an object.
*/

namespace VMAP
{
    // IntersectionCallBack 是一个模板类，用于处理射线与实体的交集检测回调
    // TValue 是实体类型
    template<class TValue>
    class IntersectionCallBack
    {
    public:
        TValue*      closestEntity;  // 最近的碰撞实体
        G3D::Vector3 hitLocation;    // 碰撞发生的位置
        G3D::Vector3 hitNormal;      // 碰撞点的法线方向

        // 重载运算符，用于执行交集检测
        // ray: 输入射线，entity: 被检测的实体，StopAtFirstHit: 是否在第一次碰撞时停止，distance: 射线距离
        void operator()(const G3D::Ray& ray, const TValue* entity, bool StopAtFirstHit, float& distance)
        {
            entity->intersect(ray, distance, StopAtFirstHit, hitLocation, hitNormal);
        }
    };

    //==============================================================
    //==============================================================
    //==============================================================

    // MyCollisionDetection 类提供自定义的碰撞检测功能
    class MyCollisionDetection
    {
    public:
        // 检测从某一点沿固定方向移动时是否与轴对齐包围盒（AABox）发生碰撞
        // origin: 起始点，dir: 移动方向，box: 被检测的包围盒
        // location: 输出的碰撞位置，Inside: 是否起点就在包围盒内部
        static bool collisionLocationForMovingPointFixedAABox(
            const G3D::Vector3&     origin,
            const G3D::Vector3&     dir,
            const G3D::AABox&       box,
            G3D::Vector3&           location,
            bool&                   Inside)
        {
            // Integer representation of a floating-point value.
#define IR(x)   (reinterpret_cast<G3D::uint32 const&>(x))

            Inside = true;
            const G3D::Vector3& MinB = box.low();   // 包围盒最小坐标点
            const G3D::Vector3& MaxB = box.high();  // 包围盒最大坐标点
            G3D::Vector3 MaxT(-1.0f, -1.0f, -1.0f); // 用于存储各轴方向的碰撞时间

            // 查找候选的碰撞平面
            for (int i = 0; i < 3; ++i)
            {
                if (origin[i] < MinB[i])
                {
                    location[i] = MinB[i];  // 设置为包围盒边界
                    Inside      = false;    // 起点不在包围盒内部

                    // 计算到候选平面的距离
                    if (IR(dir[i]))
                    {
                        MaxT[i] = (MinB[i] - origin[i]) / dir[i];
                    }
                }
                else if (origin[i] > MaxB[i])
                {
                    location[i] = MaxB[i];  // 设置为包围盒边界
                    Inside      = false;    // 起点不在包围盒内部

                    // 计算到候选平面的距离
                    if (IR(dir[i]))
                    {
                        MaxT[i] = (MaxB[i] - origin[i]) / dir[i];
                    }
                }
            }

            // 如果起点在包围盒内部，则一定发生碰撞
            if (Inside)
            {
                location = origin;
                return true;
            }

            // 找出最大的 MaxT 值，确定最终的碰撞平面
            int WhichPlane = 0;
            if (MaxT[1] > MaxT[WhichPlane])
            {
                WhichPlane = 1;
            }

            if (MaxT[2] > MaxT[WhichPlane])
            {
                WhichPlane = 2;
            }

            // 检查最终的候选平面是否有效
            if (IR(MaxT[WhichPlane]) & 0x80000000)
            {
                // 碰撞时间无效，未命中包围盒
                return false;
            }

            // 检查碰撞点是否在包围盒范围内
            for (int i = 0; i < 3; ++i)
            {
                if (i != WhichPlane)
                {
                    location[i] = origin[i] + MaxT[WhichPlane] * dir[i];
                    if ((location[i] < MinB[i]) ||
                            (location[i] > MaxB[i]))
                    {
                        // 碰撞点在此平面上超出包围盒范围，未命中
                        return false;
                    }
                }
            }
            /*
            // Choose the normal to be the plane normal facing into the ray
            normal = G3D::Vector3::zero();
            normal[WhichPlane] = (dir[WhichPlane] > 0) ? -1.0 : 1.0;
            */
            return true;

#undef IR
        }
    };
}
#endif