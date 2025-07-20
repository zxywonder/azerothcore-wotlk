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

#ifndef _BIH_H
#define _BIH_H

#include "G3D/AABox.h"
#include "G3D/Ray.h"
#include "G3D/Vector3.h"

#include "Define.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

#define MAX_STACK_SIZE 64

// https://stackoverflow.com/a/4328396

static inline uint32 floatToRawIntBits(float f)
{
    // 静态断言确保float和uint32大小相同，否则无法进行位转换
    static_assert(sizeof(float) == sizeof(uint32), "Size of uint32 and float must be equal for this to work");
    uint32 ret;
    memcpy(&ret, &f, sizeof(float));
    return ret;
}

static inline float intBitsToFloat(uint32 i)
{
    // 静态断言确保float和uint32大小相同，否则无法进行位转换
    static_assert(sizeof(float) == sizeof(uint32), "Size of uint32 and float must be equal for this to work");
    float ret;
    memcpy(&ret, &i, sizeof(uint32));
    return ret;
}

// 轴对齐包围盒结构体
struct AABound
{
    G3D::Vector3 lo, hi;
};

/** Bounding Interval Hierarchy 类。
    构建和光线相交函数基于 Sunflow 中的 BIH 实现，
    Sunflow 是一个使用 Java 编写的光线追踪器，使用 MIT/X11 协议发布
    http://sunflow.sourceforge.net/
    Copyright (c) 2003-2007 Christopher Kulla
*/
class BIH
{
private:
    void init_empty()
    {
        tree.clear();
        objects.clear();
        // 为第一个节点预留空间
        tree.push_back(3u << 30u); // dummy leaf
        tree.insert(tree.end(), 2, 0);
    }
public:
    BIH() { init_empty(); }

    /**
     * 构建 BIH 层次结构
     * @tparam BoundsFunc 用于获取包围盒的函数对象类型
     * @tparam PrimArray 原始图元数组类型
     * @param primitives 原始图元集合
     * @param GetBounds 获取包围盒的函数
     * @param leafSize 叶子节点最大图元数
     * @param printStats 是否打印构建统计信息
     */
    template< class BoundsFunc, class PrimArray >
    void build(const PrimArray& primitives, BoundsFunc& GetBounds, uint32 leafSize = 3, bool printStats = false)
    {
        if (primitives.size() == 0)
        {
            init_empty();
            return;
        }

        buildData dat;
        dat.maxPrims = leafSize;
        dat.numPrims = primitives.size();
        dat.indices = new uint32[dat.numPrims];
        dat.primBound = new G3D::AABox[dat.numPrims];
        GetBounds(primitives[0], bounds);
        for (uint32 i = 0; i < dat.numPrims; ++i)
        {
            dat.indices[i] = i;
            GetBounds(primitives[i], dat.primBound[i]);
            bounds.merge(dat.primBound[i]);
        }
        std::vector<uint32> tempTree;
        BuildStats stats;
        buildHierarchy(tempTree, dat, stats);
        if (printStats)
        {
            stats.printStats();
        }

        objects.resize(dat.numPrims);
        for (uint32 i = 0; i < dat.numPrims; ++i)
        {
            objects[i] = dat.indices[i];
        }
        //nObjects = dat.numPrims;
        tree = tempTree;
        delete[] dat.primBound;
        delete[] dat.indices;
    }

    // 返回图元数量
    [[nodiscard]] uint32 primCount() const { return objects.size(); }

    /**
     * 光线相交检测
     * @tparam RayCallback 回调函数类型
     * @param r 光线对象
     * @param intersectCallback 相交回调函数
     * @param maxDist 最大相交距离
     * @param stopAtFirstHit 是否在第一次命中时停止
     */
    template<typename RayCallback>
    void intersectRay(const G3D::Ray& r, RayCallback& intersectCallback, float& maxDist, bool stopAtFirstHit) const
    {
        float intervalMin = -1.f;
        float intervalMax = -1.f;
        G3D::Vector3 org = r.origin();
        G3D::Vector3 dir = r.direction();
        G3D::Vector3 invDir;
        for (int i = 0; i < 3; ++i)
        {
            invDir[i] = 1.f / dir[i];
            if (G3D::fuzzyNe(dir[i], 0.0f))
            {
                float t1 = (bounds.low()[i]  - org[i]) * invDir[i];
                float t2 = (bounds.high()[i] - org[i]) * invDir[i];
                if (t1 > t2)
                {
                    std::swap(t1, t2);
                }
                if (t1 > intervalMin)
                {
                    intervalMin = t1;
                }
                if (t2 < intervalMax || intervalMax < 0.f)
                {
                    intervalMax = t2;
                }
                // intervalMax 可能因其他轴而变得更小，
                // intervalMin 可能因其他轴而变大，所以可以提前终止
                if (intervalMax <= 0 || intervalMin >= maxDist)
                {
                    return;
                }
            }
        }

        if (intervalMin > intervalMax)
        {
            return;
        }
        intervalMin = std::max(intervalMin, 0.f);
        intervalMax = std::min(intervalMax, maxDist);

        uint32 offsetFront[3];
        uint32 offsetBack[3];
        uint32 offsetFront3[3];
        uint32 offsetBack3[3];
        // 根据方向符号位计算偏移量

        for (int i = 0; i < 3; ++i)
        {
            offsetFront[i] = floatToRawIntBits(dir[i]) >> 31;
            offsetBack[i] = offsetFront[i] ^ 1;
            offsetFront3[i] = offsetFront[i] * 3;
            offsetBack3[i] = offsetBack[i] * 3;

            // 避免在内循环中总是加1
            ++offsetFront[i];
            ++offsetBack[i];
        }

        StackNode stack[MAX_STACK_SIZE];
        int stackPos = 0;
        int node = 0;

        while (true)
        {
            while (true)
            {
                uint32 tn = tree[node];
                uint32 axis = (tn & (3 << 30)) >> 30; // cppcheck-suppress integerOverflow
                bool BVH2 = tn & (1 << 29); // cppcheck-suppress integerOverflow
                int offset = tn & ~(7 << 29); // cppcheck-suppress integerOverflow
                if (!BVH2)
                {
                    if (axis < 3)
                    {
                        // "正常" 内部节点
                        float tf = (intBitsToFloat(tree[node + offsetFront[axis]]) - org[axis]) * invDir[axis];
                        float tb = (intBitsToFloat(tree[node + offsetBack[axis]]) - org[axis]) * invDir[axis];
                        // 光线穿过两个剪裁区域之间
                        if (tf < intervalMin && tb > intervalMax)
                        {
                            break;
                        }
                        int back = offset + offsetBack3[axis];
                        node = back;
                        // 光线只穿过远节点
                        if (tf < intervalMin)
                        {
                            intervalMin = (tb >= intervalMin) ? tb : intervalMin;
                            continue;
                        }
                        node = offset + offsetFront3[axis]; // 前节点
                        // 光线只穿过近节点
                        if (tb > intervalMax)
                        {
                            intervalMax = (tf <= intervalMax) ? tf : intervalMax;
                            continue;
                        }
                        // 光线穿过两个节点
                        // 压栈远节点
                        stack[stackPos].node = back;
                        stack[stackPos].tnear = (tb >= intervalMin) ? tb : intervalMin;
                        stack[stackPos].tfar = intervalMax;
                        stackPos++;
                        // 更新前节点的光线区间
                        intervalMax = (tf <= intervalMax) ? tf : intervalMax;
                        continue;
                    }
                    else
                    {
                        // 叶子节点 - 测试一些对象
                        int n = tree[node + 1];
                        while (n > 0)
                        {
                            bool hit = intersectCallback(r, objects[offset], maxDist, stopAtFirstHit);
                            if (stopAtFirstHit && hit) { return; }
                            --n;
                            ++offset;
                        }
                        break;
                    }
                }
                else
                {
                    if (axis > 2)
                    {
                        return;    // 不应该发生
                    }
                    float tf = (intBitsToFloat(tree[node + offsetFront[axis]]) - org[axis]) * invDir[axis];
                    float tb = (intBitsToFloat(tree[node + offsetBack[axis]]) - org[axis]) * invDir[axis];
                    node = offset;
                    intervalMin = (tf >= intervalMin) ? tf : intervalMin;
                    intervalMax = (tb <= intervalMax) ? tb : intervalMax;
                    if (intervalMin > intervalMax)
                    {
                        break;
                    }
                    continue;
                }
            } // 遍历循环
            do
            {
                // 栈是否为空？
                if (stackPos == 0)
                {
                    return;
                }
                // 返回栈上层
                stackPos--;
                intervalMin = stack[stackPos].tnear;
                if (maxDist < intervalMin)
                {
                    continue;
                }
                node = stack[stackPos].node;
                intervalMax = stack[stackPos].tfar;
                break;
            } while (true);
        }
    }

    /**
     * 点相交检测
     * @tparam IsectCallback 回调函数类型
     * @param p 点坐标
     * @param intersectCallback 相交回调函数
     */
    template<typename IsectCallback>
    void intersectPoint(const G3D::Vector3& p, IsectCallback& intersectCallback) const
    {
        if (!bounds.contains(p))
        {
            return;
        }

        StackNode stack[MAX_STACK_SIZE];
        int stackPos = 0;
        int node = 0;

        while (true)
        {
            while (true)
            {
                uint32 tn = tree[node];
                uint32 axis = (tn & (3 << 30)) >> 30; // cppcheck-suppress integerOverflow
                bool BVH2 = tn & (1 << 29); // cppcheck-suppress integerOverflow
                int offset = tn & ~(7 << 29); // cppcheck-suppress integerOverflow
                if (!BVH2)
                {
                    if (axis < 3)
                    {
                        // "正常" 内部节点
                        float tl = intBitsToFloat(tree[node + 1]);
                        float tr = intBitsToFloat(tree[node + 2]);
                        // 点位于两个剪裁区域之间
                        if (tl < p[axis] && tr > p[axis])
                        {
                            break;
                        }
                        int right = offset + 3;
                        node = right;
                        // 点只在右节点中
                        if (tl < p[axis])
                        {
                            continue;
                        }
                        node = offset; // 左节点
                        // 点只在左节点中
                        if (tr > p[axis])
                        {
                            continue;
                        }
                        // 点在两个节点中
                        // 压栈右节点
                        stack[stackPos].node = right;
                        stackPos++;
                        continue;
                    }
                    else
                    {
                        // 叶子节点 - 测试一些对象
                        int n = tree[node + 1];
                        while (n > 0)
                        {
                            intersectCallback(p, objects[offset]); // !!!
                            --n;
                            ++offset;
                        }
                        break;
                    }
                }
                else // BVH2 节点 (空空间剪裁)
                {
                    if (axis > 2)
                    {
                        return;    // 不应该发生
                    }
                    float tl = intBitsToFloat(tree[node + 1]);
                    float tr = intBitsToFloat(tree[node + 2]);
                    node = offset;
                    if (tl > p[axis] || tr < p[axis])
                    {
                        break;
                    }
                    continue;
                }
            } // 遍历循环

            // 栈是否为空？
            if (stackPos == 0)
            {
                return;
            }
            // 返回栈上层
            stackPos--;
            node = stack[stackPos].node;
        }
    }

    bool writeToFile(FILE* wf) const;
    bool readFromFile(FILE* rf);

protected:
    std::vector<uint32> tree;
    std::vector<uint32> objects;
    G3D::AABox bounds;

    // 构建数据结构
    struct buildData
    {
        uint32* indices;
        G3D::AABox* primBound;
        uint32 numPrims;
        int maxPrims;
    };

    // 栈节点结构
    struct StackNode
    {
        uint32 node;
        float tnear;
        float tfar;
    };

    // 构建统计信息类
    class BuildStats
    {
    private:
        int numNodes{0};
        int numLeaves{0};
        int sumObjects{0};
        int minObjects{0x0FFFFFFF};
        int maxObjects{-1}; // 0xFFFFFFFF
        int sumDepth{0};
        int minDepth{0x0FFFFFFF};
        int maxDepth{-1}; // 0xFFFFFFFF
        int numLeavesN[6];
        int numBVH2{0};

    public:
        BuildStats()
        {
            for (int& i : numLeavesN) { i = 0; }
        }

        void updateInner() { numNodes++; }
        void updateBVH2() { numBVH2++; }
        void updateLeaf(int depth, int n);
        void printStats();
    };

    void buildHierarchy(std::vector<uint32>& tempTree, buildData& dat, BuildStats& stats);

    void createNode(std::vector<uint32>& tempTree, int nodeIndex, uint32 left, uint32 right) const
    {
        // 写入叶子节点
        tempTree[nodeIndex + 0] = (3 << 30) | left; // cppcheck-suppress integerOverflow
        tempTree[nodeIndex + 1] = right - left + 1;
    }

    void subdivide(int left, int right, std::vector<uint32>& tempTree, buildData& dat, AABound& gridBox, AABound& nodeBox, int nodeIndex, int depth, BuildStats& stats);
};

#endif // _BIH_H