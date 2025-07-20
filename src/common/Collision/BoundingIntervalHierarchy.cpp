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

#include "BoundingIntervalHierarchy.h"

// 定义 isnan 宏，兼容 MSVC 和其他编译器
#ifdef _MSC_VER
#define isnan _isnan
#else
#define isnan std::isnan
#endif

// 构建 BIH 层次结构
void BIH::buildHierarchy(std::vector<uint32>& tempTree, buildData& dat, BuildStats& stats)
{
    // 为第一个节点预留空间
    // cppcheck-suppress integerOverflow
    tempTree.push_back(uint32(3 << 30)); // dummy leaf
    tempTree.insert(tempTree.end(), 2, 0);
    //tempTree.add(0);

    // 初始化包围盒
    AABound gridBox = { bounds.low(), bounds.high() };
    AABound nodeBox = gridBox;
    // 递归分割函数
    subdivide(0, dat.numPrims - 1, tempTree, dat, gridBox, nodeBox, 0, 1, stats);
}

// 递归分割函数，构建 BIH 树
void BIH::subdivide(int left, int right, std::vector<uint32>& tempTree, buildData& dat, AABound& gridBox, AABound& nodeBox, int nodeIndex, int depth, BuildStats& stats)
{
    // 如果当前节点包含的图元数量小于等于最大叶子节点图元数 或者 达到最大递归深度，则创建叶子节点
    if ((right - left + 1) <= dat.maxPrims || depth >= MAX_STACK_SIZE)
    {
        // 创建叶子节点
        stats.updateLeaf(depth, right - left + 1);
        createNode(tempTree, nodeIndex, left, right);
        return;
    }

    // 初始化分割参数
    int axis = -1, prevAxis, rightOrig;
    float clipL = G3D::fnan(), clipR = G3D::fnan(), prevClip = G3D::fnan();
    float split = G3D::fnan(), prevSplit;
    bool wasLeft = true;

    // 开始分割循环
    while (true)
    {
        prevAxis = axis;
        prevSplit = split;

        // 执行一致性检查
        G3D::Vector3 d( gridBox.hi - gridBox.lo );
        if (d.x < 0 || d.y < 0 || d.z < 0)
        {
            throw std::logic_error("negative node extents");
        }
        for (int i = 0; i < 3; i++)
        {
            if (nodeBox.hi[i] < gridBox.lo[i] || nodeBox.lo[i] > gridBox.hi[i])
            {
                throw std::logic_error("invalid node overlap");
            }
        }

        // 找到最长轴进行分割
        axis = d.primaryAxis();
        split = 0.5f * (gridBox.lo[axis] + gridBox.hi[axis]);

        // 分割左右子集
        clipL = -G3D::inf();
        clipR = G3D::inf();
        rightOrig = right; // 保存原始右边界
        float nodeL = G3D::inf();
        float nodeR = -G3D::inf();

        // 根据分割轴对图元进行左右分区
        for (int i = left; i <= right;)
        {
            int obj = dat.indices[i];
            float minb = dat.primBound[obj].low()[axis];
            float maxb = dat.primBound[obj].high()[axis];
            float center = (minb + maxb) * 0.5f;
            if (center <= split)
            {
                i++;
                if (clipL < maxb)
                {
                    clipL = maxb;
                }
            }
            else
            {
                int t = dat.indices[i];
                dat.indices[i] = dat.indices[right];
                dat.indices[right] = t;
                right--;
                if (clipR > minb)
                {
                    clipR = minb;
                }
            }
            nodeL = std::min(nodeL, minb);
            nodeR = std::max(nodeR, maxb);
        }

        // 检查空空间
        if (nodeL > nodeBox.lo[axis] && nodeR < nodeBox.hi[axis])
        {
            float nodeBoxW = nodeBox.hi[axis] - nodeBox.lo[axis];
            float nodeNewW = nodeR - nodeL;
            // 如果节点包围盒过大，创建 BVH2 节点
            if (1.3f * nodeNewW < nodeBoxW)
            {
                stats.updateBVH2();
                int nextIndex = tempTree.size();
                // 分配子节点空间
                tempTree.push_back(0);
                tempTree.push_back(0);
                tempTree.push_back(0);
                // 写入 BVH2 节点
                stats.updateInner();
                tempTree[nodeIndex + 0] = (axis << 30) | (1 << 29) | nextIndex;
                tempTree[nodeIndex + 1] = floatToRawIntBits(nodeL);
                tempTree[nodeIndex + 2] = floatToRawIntBits(nodeR);
                // 更新节点包围盒并递归
                nodeBox.lo[axis] = nodeL;
                nodeBox.hi[axis] = nodeR;
                subdivide(left, rightOrig, tempTree, dat, gridBox, nodeBox, nextIndex, depth + 1, stats);
                return;
            }
        }

        // 确保分割有进展
        if (right == rightOrig)
        {
            // 全部在左边
            if (prevAxis == axis && G3D::fuzzyEq(prevSplit, split))
            {
                stats.updateLeaf(depth, right - left + 1);
                createNode(tempTree, nodeIndex, left, right);
                return;
            }
            if (clipL <= split)
            {
                gridBox.hi[axis] = split;
                prevClip = clipL;
                wasLeft = true;
                continue;
            }
            gridBox.hi[axis] = split;
            prevClip = G3D::fnan();
        }
        else if (left > right)
        {
            // 全部在右边
            right = rightOrig;
            if (prevAxis == axis && G3D::fuzzyEq(prevSplit, split))
            {
                stats.updateLeaf(depth, right - left + 1);
                createNode(tempTree, nodeIndex, left, right);
                return;
            }
            if (clipR >= split)
            {
                gridBox.lo[axis] = split;
                prevClip = clipR;
                wasLeft = false;
                continue;
            }
            gridBox.lo[axis] = split;
            prevClip = G3D::fnan();
        }
        else
        {
            // 实际分割了数据
            if (prevAxis != -1 && !isnan(prevClip))
            {
                int nextIndex = tempTree.size();
                tempTree.push_back(0);
                tempTree.push_back(0);
                tempTree.push_back(0);
                if (wasLeft)
                {
                    stats.updateInner();
                    tempTree[nodeIndex + 0] = (prevAxis << 30) | nextIndex;
                    tempTree[nodeIndex + 1] = floatToRawIntBits(prevClip);
                    tempTree[nodeIndex + 2] = floatToRawIntBits(G3D::inf());
                }
                else
                {
                    stats.updateInner();
                    tempTree[nodeIndex + 0] = (prevAxis << 30) | (nextIndex - 3);
                    tempTree[nodeIndex + 1] = floatToRawIntBits(-G3D::inf());
                    tempTree[nodeIndex + 2] = floatToRawIntBits(prevClip);
                }
                depth++;
                stats.updateLeaf(depth, 0);
                nodeIndex = nextIndex;
            }
            break;
        }
    }

    // 计算子节点索引
    int nextIndex = tempTree.size();
    int nl = right - left + 1;
    int nr = rightOrig - (right + 1) + 1;

    // 分配左子节点空间
    if (nl > 0)
    {
        tempTree.push_back(0);
        tempTree.push_back(0);
        tempTree.push_back(0);
    }
    else
    {
        nextIndex -= 3;
    }

    // 分配右子节点空间
    if (nr > 0)
    {
        tempTree.push_back(0);
        tempTree.push_back(0);
        tempTree.push_back(0);
    }

    // 写入内部节点
    stats.updateInner();
    tempTree[nodeIndex + 0] = (axis << 30) | nextIndex;
    tempTree[nodeIndex + 1] = floatToRawIntBits(clipL);
    tempTree[nodeIndex + 2] = floatToRawIntBits(clipR);

    // 准备左右子节点包围盒
    AABound gridBoxL(gridBox), gridBoxR(gridBox);
    AABound nodeBoxL(nodeBox), nodeBoxR(nodeBox);
    gridBoxL.hi[axis] = gridBoxR.lo[axis] = split;
    nodeBoxL.hi[axis] = clipL;
    nodeBoxR.lo[axis] = clipR;

    // 递归处理左右子节点
    if (nl > 0)
    {
        subdivide(left, right, tempTree, dat, gridBoxL, nodeBoxL, nextIndex, depth + 1, stats);
    }
    else
    {
        stats.updateLeaf(depth + 1, 0);
    }
    if (nr > 0)
    {
        subdivide(right + 1, rightOrig, tempTree, dat, gridBoxR, nodeBoxR, nextIndex + 3, depth + 1, stats);
    }
    else
    {
        stats.updateLeaf(depth + 1, 0);
    }
}

// 将 BIH 数据写入文件
bool BIH::writeToFile(FILE* wf) const
{
    uint32 treeSize = tree.size();
    uint32 check = 0, count;
    check += fwrite(&bounds.low(), sizeof(float), 3, wf);
    check += fwrite(&bounds.high(), sizeof(float), 3, wf);
    check += fwrite(&treeSize, sizeof(uint32), 1, wf);
    check += fwrite(&tree[0], sizeof(uint32), treeSize, wf);
    count = objects.size();
    check += fwrite(&count, sizeof(uint32), 1, wf);
    check += fwrite(&objects[0], sizeof(uint32), count, wf);
    return check == (3 + 3 + 2 + treeSize + count);
}

// 从文件读取 BIH 数据
bool BIH::readFromFile(FILE* rf)
{
    uint32 treeSize;
    G3D::Vector3 lo, hi;
    uint32 check = 0, count = 0;
    check += fread(&lo, sizeof(float), 3, rf);
    check += fread(&hi, sizeof(float), 3, rf);
    bounds = G3D::AABox(lo, hi);
    check += fread(&treeSize, sizeof(uint32), 1, rf);
    tree.resize(treeSize);
    check += fread(&tree[0], sizeof(uint32), treeSize, rf);
    check += fread(&count, sizeof(uint32), 1, rf);
    objects.resize(count);
    check += fread(&objects[0], sizeof(uint32), count, rf);
    return uint64(check) == uint64(3 + 3 + 1 + 1 + uint64(treeSize) + uint64(count));
}

// 更新叶子节点统计信息
void BIH::BuildStats::updateLeaf(int depth, int n)
{
    numLeaves++;
    minDepth = std::min(depth, minDepth);
    maxDepth = std::max(depth, maxDepth);
    sumDepth += depth;
    minObjects = std::min(n, minObjects);
    maxObjects = std::max(n, maxObjects);
    sumObjects += n;
    int nl = std::min(n, 5);
    ++numLeavesN[nl];
}

// 打印构建统计信息
void BIH::BuildStats::printStats()
{
    printf("Tree stats:\n");
    printf("  * Nodes:          %d\n", numNodes);
    printf("  * Leaves:         %d\n", numLeaves);
    printf("  * Objects: min    %d\n", minObjects);
    printf("             avg    %.2f\n", (float) sumObjects / numLeaves);
    printf("           avg(n>0) %.2f\n", (float) sumObjects / (numLeaves - numLeavesN[0]));
    printf("             max    %d\n", maxObjects);
    printf("  * Depth:   min    %d\n", minDepth);
    printf("             avg    %.2f\n", (float) sumDepth / numLeaves);
    printf("             max    %d\n", maxDepth);
    printf("  * Leaves w/: N=0  %3d%%\n", 100 * numLeavesN[0] / numLeaves);
    printf("               N=1  %3d%%\n", 100 * numLeavesN[1] / numLeaves);
    printf("               N=2  %3d%%\n", 100 * numLeavesN[2] / numLeaves);
    printf("               N=3  %3d%%\n", 100 * numLeavesN[3] / numLeaves);
    printf("               N=4  %3d%%\n", 100 * numLeavesN[4] / numLeaves);
    printf("               N>4  %3d%%\n", 100 * numLeavesN[5] / numLeaves);
    printf("  * BVH2 nodes:     %d (%3d%%)\n", numBVH2, 100 * numBVH2 / (numNodes + numLeaves - 2 * numBVH2));
}
