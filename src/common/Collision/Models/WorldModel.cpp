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

#include "WorldModel.h"
#include "MapTree.h"
#include "ModelIgnoreFlags.h"
#include "ModelInstance.h"
#include "VMapDefinitions.h"

using G3D::Vector3;
using G3D::Ray;

// 模板特化，用于获取 VMAP::GroupModel 的边界框
template<> struct BoundsTrait<VMAP::GroupModel>
{
    // 获取指定 GroupModel 对象的边界框
    static void GetBounds(const VMAP::GroupModel& obj, G3D::AABox& out) { out = obj.GetBound(); }
};

namespace VMAP
{
    // 判断射线是否与三角形相交
    bool IntersectTriangle(const MeshTriangle& tri, std::vector<Vector3>::const_iterator points, const G3D::Ray& ray, float& distance)
    {
        static const float EPS = 1e-5f;

        // 算法参考 RTR2 第 13.7 章
        const Vector3 e1 = points[tri.idx1] - points[tri.idx0];
        const Vector3 e2 = points[tri.idx2] - points[tri.idx0];
        const Vector3 p(ray.direction().cross(e2));
        const float a = e1.dot(p);

        if (std::fabs(a) < EPS)
        {
            // 行列式条件数不佳，提前终止
            return false;
        }

        const float f = 1.0f / a;
        const Vector3 s(ray.origin() - points[tri.idx0]);
        const float u = f * s.dot(p);

        if ((u < 0.0f) || (u > 1.0f))
        {
            // 射线命中了几何体所在的平面，但在几何体外部
            return false;
        }

        const Vector3 q(s.cross(e1));
        const float v = f * ray.direction().dot(q);

        if ((v < 0.0f) || ((u + v) > 1.0f))
        {
            // 射线命中了三角形所在的平面，但在三角形外部
            return false;
        }

        const float t = f * e2.dot(q);

        if ((t > 0.0f) && (t < distance))
        {
            // 这是一个新的命中点，比之前的命中点更近
            distance = t;

            /* baryCoord[0] = 1.0 - u - v;
            baryCoord[1] = u;
            baryCoord[2] = v; */

            return true;
        }
        // 这个命中点在之前命中点之后，忽略它
        return false;
    }

    // 三角形边界框计算函数类
    class TriBoundFunc
    {
    public:
        // 构造函数，初始化顶点迭代器
        TriBoundFunc(std::vector<Vector3>& vert): vertices(vert.begin()) { }
        // 计算指定三角形的边界框
        void operator()(const MeshTriangle& tri, G3D::AABox& out) const
        {
            G3D::Vector3 lo = vertices[tri.idx0];
            G3D::Vector3 hi = lo;

            lo = (lo.min(vertices[tri.idx1])).min(vertices[tri.idx2]);
            hi = (hi.max(vertices[tri.idx1])).max(vertices[tri.idx2]);

            out = G3D::AABox(lo, hi);
        }
    protected:
        // 顶点的常量迭代器
        const std::vector<Vector3>::const_iterator vertices;
    };

    // ===================== WmoLiquid ==================================
    // WmoLiquid 构造函数，初始化液体区域的尺寸、角落位置和类型
    WmoLiquid::WmoLiquid(uint32 width, uint32 height, const Vector3& corner, uint32 type):
        iTilesX(width), iTilesY(height), iCorner(corner), iType(type)
    {
        if (width && height)
        {
            iHeight = new float[(width + 1) * (height + 1)];
            iFlags = new uint8[width * height];
        }
        else
        {
            iHeight = new float[1];
            iFlags = nullptr;
        }
    }

    // WmoLiquid 拷贝构造函数
    WmoLiquid::WmoLiquid(const WmoLiquid& other): iHeight(0), iFlags(0)
    {
        *this = other; // 使用赋值运算符
    }

    // WmoLiquid 析构函数，释放动态分配的内存
    WmoLiquid::~WmoLiquid()
    {
        delete[] iHeight;
        delete[] iFlags;
    }

    // WmoLiquid 赋值运算符重载
    WmoLiquid& WmoLiquid::operator=(const WmoLiquid& other)
    {
        if (this == &other)
        {
            return *this;
        }
        iTilesX = other.iTilesX;
        iTilesY = other.iTilesY;
        iCorner = other.iCorner;
        iType = other.iType;
        delete[] iHeight;
        delete[] iFlags;
        if (other.iHeight)
        {
            iHeight = new float[(iTilesX + 1) * (iTilesY + 1)];
            memcpy(iHeight, other.iHeight, (iTilesX + 1) * (iTilesY + 1)*sizeof(float));
        }
        else
        {
            iHeight = 0;
        }
        if (other.iFlags)
        {
            iFlags = new uint8[iTilesX * iTilesY];
            memcpy(iFlags, other.iFlags, iTilesX * iTilesY);
        }
        else
        {
            iFlags = 0;
        }
        return *this;
    }

    // 获取指定位置的液体高度
    bool WmoLiquid::GetLiquidHeight(const Vector3& pos, float& liqHeight) const
    {
        // 简单情况
        if (!iFlags)
        {
            liqHeight = iHeight[0];
            return true;
        }

        float tx_f = (pos.x - iCorner.x) / LIQUID_TILE_SIZE;
        uint32 tx = uint32(tx_f);
        if (tx_f < 0.0f || tx >= iTilesX)
        {
            return false;
        }
        float ty_f = (pos.y - iCorner.y) / LIQUID_TILE_SIZE;
        uint32 ty = uint32(ty_f);
        if (ty_f < 0.0f || ty >= iTilesY)
        {
            return false;
        }

        // 检查该瓦片是否用于计算液体高度
        // 检查 0x08 可能就足够了，但禁用的瓦片总是 0x?F:
        if (iFlags && (iFlags[tx + ty * iTilesX] & 0x0F) == 0x0F)
        {
            return false;
        }

        // 瓦片内的 (dx, dy) 坐标，范围在 [0, 1]^2
        float dx = tx_f - (float)tx;
        float dy = ty_f - (float)ty;

        /* 将瓦片分割为两个三角形（不确定客户端是否完全这样处理）

            ^ dy
            |
          1 x---------x (1, 1)
            | (b)   / |
            |     /   |
            |   /     |
            | /   (a) |
            x---------x---> dx
          0           1
        */

        if (!iHeight)
        {
            return false;
        }

        const uint32 rowOffset = iTilesX + 1;
        if (dx > dy) // 情况 (a)
        {
            float sx = iHeight[tx + 1 +  ty    * rowOffset] - iHeight[tx   + ty * rowOffset];
            float sy = iHeight[tx + 1 + (ty + 1) * rowOffset] - iHeight[tx + 1 + ty * rowOffset];
            liqHeight = iHeight[tx + ty * rowOffset] + dx * sx + dy * sy;
        }
        else // 情况 (b)
        {
            float sx = iHeight[tx + 1 + (ty + 1) * rowOffset] - iHeight[tx + (ty + 1) * rowOffset];
            float sy = iHeight[tx   + (ty + 1) * rowOffset] - iHeight[tx +  ty    * rowOffset];
            liqHeight = iHeight[tx + ty * rowOffset] + dx * sx + dy * sy;
        }
        return true;
    }

    // 获取 WmoLiquid 对象在文件中占用的大小
    uint32 WmoLiquid::GetFileSize()
    {
        return 2 * sizeof(uint32) +
               sizeof(Vector3) +
               sizeof(uint32) +
               (iFlags ? ((iTilesX + 1) * (iTilesY + 1) * sizeof(float) + iTilesX * iTilesY) : sizeof(float));
    }

    // 将 WmoLiquid 对象写入文件
    bool WmoLiquid::writeToFile(FILE* wf)
    {
        bool result = false;
        if (fwrite(&iTilesX, sizeof(uint32), 1, wf) == 1 &&
                fwrite(&iTilesY, sizeof(uint32), 1, wf) == 1 &&
                fwrite(&iCorner, sizeof(Vector3), 1, wf) == 1 &&
                fwrite(&iType, sizeof(uint32), 1, wf) == 1)
        {
            if (iTilesX && iTilesY)
            {
                uint32 size = (iTilesX + 1) * (iTilesY + 1);
                if (fwrite(iHeight, sizeof(float), size, wf) == size)
                {
                    size = iTilesX * iTilesY;
                    result = fwrite(iFlags, sizeof(uint8), size, wf) == size;
                }
            }
            else
                result = fwrite(iHeight, sizeof(float), 1, wf) == 1;
        }

        return result;
    }

    // 从文件中读取 WmoLiquid 对象
    bool WmoLiquid::readFromFile(FILE* rf, WmoLiquid*& out)
    {
        bool result = false;
        WmoLiquid* liquid = new WmoLiquid();

        if (fread(&liquid->iTilesX, sizeof(uint32), 1, rf) == 1 &&
                fread(&liquid->iTilesY, sizeof(uint32), 1, rf) == 1 &&
                fread(&liquid->iCorner, sizeof(Vector3), 1, rf) == 1 &&
                fread(&liquid->iType, sizeof(uint32), 1, rf) == 1)
        {
            if (liquid->iTilesX && liquid->iTilesY)
            {
                uint32 size = (liquid->iTilesX + 1) * (liquid->iTilesY + 1);
                liquid->iHeight = new float[size];
                if (fread(liquid->iHeight, sizeof(float), size, rf) == size)
                {
                    size = liquid->iTilesX * liquid->iTilesY;
                    liquid->iFlags = new uint8[size];
                    result = fread(liquid->iFlags, sizeof(uint8), size, rf) == size;
                }
            }
            else
            {
                liquid->iHeight = new float[1];
                result = fread(liquid->iHeight, sizeof(float), 1, rf) == 1;
            }
        }

        if (!result)
        {
            delete liquid;
        }
        else
        {
            out = liquid;
        }

        return result;
    }

    // 获取 WmoLiquid 对象的位置信息
    void WmoLiquid::GetPosInfo(uint32& tilesX, uint32& tilesY, G3D::Vector3& corner) const
    {
        tilesX = iTilesX;
        tilesY = iTilesY;
        corner = iCorner;
    }

    // ===================== GroupModel ==================================
    // GroupModel 拷贝构造函数
    GroupModel::GroupModel(const GroupModel& other):
        iBound(other.iBound), iMogpFlags(other.iMogpFlags), iGroupWMOID(other.iGroupWMOID),
        vertices(other.vertices), triangles(other.triangles), meshTree(other.meshTree), iLiquid(0)
    {
        if (other.iLiquid)
        {
            iLiquid = new WmoLiquid(*other.iLiquid);
        }
    }

    // 设置 GroupModel 的网格数据
    void GroupModel::setMeshData(std::vector<Vector3>& vert, std::vector<MeshTriangle>& tri)
    {
        vertices.swap(vert);
        triangles.swap(tri);
        TriBoundFunc bFunc(vertices);
        meshTree.build(triangles, bFunc);
    }

    // 将 GroupModel 对象写入文件
    bool GroupModel::writeToFile(FILE* wf)
    {
        bool result = true;
        uint32 chunkSize, count;

        if (fwrite(&iBound, sizeof(G3D::AABox), 1, wf) != 1) { result = false; }
        if (result && fwrite(&iMogpFlags, sizeof(uint32), 1, wf) != 1) { result = false; }
        if (result && fwrite(&iGroupWMOID, sizeof(uint32), 1, wf) != 1) { result = false; }

        // 写入顶点数据
        if (result && fwrite("VERT", 1, 4, wf) != 4) { result = false; }
        count = vertices.size();
        chunkSize = sizeof(uint32) + sizeof(Vector3) * count;
        if (result && fwrite(&chunkSize, sizeof(uint32), 1, wf) != 1) { result = false; }
        if (result && fwrite(&count, sizeof(uint32), 1, wf) != 1) { result = false; }
        if (!count) // 没有（碰撞）几何体的模型到此结束，不确定是否有用
        {
            return result;
        }
        if (result && fwrite(&vertices[0], sizeof(Vector3), count, wf) != count) { result = false; }

        // 写入三角形网格数据
        if (result && fwrite("TRIM", 1, 4, wf) != 4) { result = false; }
        count = triangles.size();
        chunkSize = sizeof(uint32) + sizeof(MeshTriangle) * count;
        if (result && fwrite(&chunkSize, sizeof(uint32), 1, wf) != 1) { result = false; }
        if (result && fwrite(&count, sizeof(uint32), 1, wf) != 1) { result = false; }
        if (result && fwrite(&triangles[0], sizeof(MeshTriangle), count, wf) != count) { result = false; }

        // 写入网格 BIH 数据
        if (result && fwrite("MBIH", 1, 4, wf) != 4) { result = false; }
        if (result) { result = meshTree.writeToFile(wf); }

        // 写入液体数据
        if (result && fwrite("LIQU", 1, 4, wf) != 4) { result = false; }
        if (!iLiquid)
        {
            chunkSize = 0;
            if (result && fwrite(&chunkSize, sizeof(uint32), 1, wf) != 1) { result = false; }
            return result;
        }
        chunkSize = iLiquid->GetFileSize();
        if (result && fwrite(&chunkSize, sizeof(uint32), 1, wf) != 1) { result = false; }
        if (result) { result = iLiquid->writeToFile(wf); }

        return result;
    }

    // 从文件中读取 GroupModel 对象
    bool GroupModel::readFromFile(FILE* rf)
    {
        char chunk[8];
        bool result = true;
        uint32 chunkSize = 0;
        uint32 count = 0;
        triangles.clear();
        vertices.clear();
        delete iLiquid;
        iLiquid = nullptr;

        if (fread(&iBound, sizeof(G3D::AABox), 1, rf) != 1) { result = false; }
        if (result && fread(&iMogpFlags, sizeof(uint32), 1, rf) != 1) { result = false; }
        if (result && fread(&iGroupWMOID, sizeof(uint32), 1, rf) != 1) { result = false; }

        // 读取顶点数据
        if (result && !readChunk(rf, chunk, "VERT", 4)) { result = false; }
        if (result && fread(&chunkSize, sizeof(uint32), 1, rf) != 1) { result = false; }
        if (result && fread(&count, sizeof(uint32), 1, rf) != 1) { result = false; }
        if (!count) // 没有（碰撞）几何体的模型到此结束，不确定是否有用
        {
            return result;
        }
        if (result) { vertices.resize(count); }
        if (result && fread(&vertices[0], sizeof(Vector3), count, rf) != count) { result = false; }

        // 读取三角形网格数据
        if (result && !readChunk(rf, chunk, "TRIM", 4)) { result = false; }
        if (result && fread(&chunkSize, sizeof(uint32), 1, rf) != 1) { result = false; }
        if (result && fread(&count, sizeof(uint32), 1, rf) != 1) { result = false; }
        if (result) { triangles.resize(count); }
        if (result && fread(&triangles[0], sizeof(MeshTriangle), count, rf) != count) { result = false; }

        // 读取网格 BIH 数据
        if (result && !readChunk(rf, chunk, "MBIH", 4)) { result = false; }
        if (result) { result = meshTree.readFromFile(rf); }

        // 读取液体数据
        if (result && !readChunk(rf, chunk, "LIQU", 4)) { result = false; }
        if (result && fread(&chunkSize, sizeof(uint32), 1, rf) != 1) { result = false; }
        if (result && chunkSize > 0)
        {
            result = WmoLiquid::readFromFile(rf, iLiquid);
        }
        return result;
    }

    // GroupModel 射线检测回调类
    struct GModelRayCallback
    {
        // 构造函数，初始化顶点和三角形迭代器
        GModelRayCallback(const std::vector<MeshTriangle>& tris, const std::vector<Vector3>& vert):
            vertices(vert.begin()), triangles(tris.begin()), hit(false) { }
        // 射线检测回调函数
        bool operator()(const G3D::Ray& ray, uint32 entry, float& distance, bool /*StopAtFirstHit*/)
        {
            bool result = IntersectTriangle(triangles[entry], vertices, ray, distance);
            if (result) { hit = true; }
            return hit;
        }
        // 顶点的常量迭代器
        std::vector<Vector3>::const_iterator vertices;
        // 三角形的常量迭代器
        std::vector<MeshTriangle>::const_iterator triangles;
        // 是否命中标志
        bool hit;
    };

    // 检测射线是否与 GroupModel 相交
    bool GroupModel::IntersectRay(const G3D::Ray& ray, float& distance, bool stopAtFirstHit) const
    {
        if (triangles.empty())
        {
            return false;
        }

        GModelRayCallback callback(triangles, vertices);
        meshTree.intersectRay(ray, callback, distance, stopAtFirstHit);
        return callback.hit;
    }

    // 判断点是否在 GroupModel 对象内部
    bool GroupModel::IsInsideObject(const Vector3& pos, const Vector3& down, float& z_dist) const
    {
        if (triangles.empty() || !iBound.contains(pos))
        {
            return false;
        }
        Vector3 rPos = pos - 0.1f * down;
        float dist = G3D::inf();
        G3D::Ray ray(rPos, down);
        bool hit = IntersectRay(ray, dist, false);
        if (hit)
        {
            z_dist = dist - 0.1f;
        }
        return hit;
    }

    // 获取指定位置的液体高度
    bool GroupModel::GetLiquidLevel(const Vector3& pos, float& liqHeight) const
    {
        if (iLiquid)
        {
            return iLiquid->GetLiquidHeight(pos, liqHeight);
        }
        return false;
    }

    // 获取液体类型
    uint32 GroupModel::GetLiquidType() const
    {
        if (iLiquid)
        {
            return iLiquid->GetType();
        }
        return 0;
    }

    // 获取 GroupModel 的网格数据
    void GroupModel::GetMeshData(std::vector<G3D::Vector3>& outVertices, std::vector<MeshTriangle>& outTriangles, WmoLiquid*& liquid)
    {
        outVertices = vertices;
        outTriangles = triangles;
        liquid = iLiquid;
    }

    // ===================== WorldModel ==================================
    // 设置 WorldModel 的组模型
    void WorldModel::setGroupModels(std::vector<GroupModel>& models)
    {
        groupModels.swap(models);
        groupTree.build(groupModels, BoundsTrait<GroupModel>::GetBounds, 1);
    }

    // WorldModel 射线检测回调类
    struct WModelRayCallBack
    {
        // 构造函数，初始化组模型迭代器
        WModelRayCallBack(const std::vector<GroupModel>& mod): models(mod.begin()), hit(false) { }
        // 射线检测回调函数
        bool operator()(const G3D::Ray& ray, uint32 entry, float& distance, bool StopAtFirstHit)
        {
            bool result = models[entry].IntersectRay(ray, distance, StopAtFirstHit);
            if (result) { hit = true; }
            return hit;
        }
        // 组模型的常量迭代器
        std::vector<GroupModel>::const_iterator models;
        // 是否命中标志
        bool hit;
    };

    // 检测射线是否与 WorldModel 相交
    bool WorldModel::IntersectRay(const G3D::Ray& ray, float& distance, bool stopAtFirstHit, ModelIgnoreFlags ignoreFlags) const
    {
        // 如果调用者要求忽略某些对象，我们应该检查标志
        if ((ignoreFlags & ModelIgnoreFlags::M2) != ModelIgnoreFlags::Nothing)
        {
            // 如果调用者要求忽略 M2 模型，则在视线计算中不考虑它们
            if (Flags & MOD_M2)
            {
                return false;
            }
        }

        // 小 M2 模型的变通方法，也许最好创建一个带有虚相交函数的单独类
        // 无论如何，如果我们只有一个子模型，就没有必要使用边界树
        if (groupModels.size() == 1)
        {
            return groupModels[0].IntersectRay(ray, distance, stopAtFirstHit);
        }

        WModelRayCallBack isc(groupModels);
        groupTree.intersectRay(ray, isc, distance, stopAtFirstHit);
        return isc.hit;
    }

    // WorldModel 区域检测回调类
    class WModelAreaCallback
    {
    public:
        // 构造函数，初始化组模型迭代器、最小体积、Z 轴距离和 Z 轴向量
        WModelAreaCallback(const std::vector<GroupModel>& vals, const Vector3& down):
            prims(vals.begin()), hit(vals.end()), minVol(G3D::inf()), zDist(G3D::inf()), zVec(down) { }
        // 组模型的常量迭代器
        std::vector<GroupModel>::const_iterator prims;
        // 命中的组模型迭代器
        std::vector<GroupModel>::const_iterator hit;
        // 最小体积
        float minVol;
        // Z 轴距离
        float zDist;
        // Z 轴向量
        Vector3 zVec;
        // 区域检测回调函数
        void operator()(const Vector3& point, uint32 entry)
        {
            float group_Z;
            //float pVol = prims[entry].GetBound().volume();
            //if (pVol < minVol)
            //{
            /* if (prims[entry].iBound.contains(point)) */
            if (prims[entry].IsInsideObject(point, zVec, group_Z))
            {
                //minVol = pVol;
                //hit = prims + entry;
                if (group_Z < zDist)
                {
                    zDist = group_Z;
                    hit = prims + entry;
                }
#ifdef VMAP_DEBUG
                const GroupModel& gm = prims[entry];
                printf("%10u %8X %7.3f, %7.3f, %7.3f | %7.3f, %7.3f, %7.3f | z=%f, p_z=%f\n", gm.GetWmoID(), gm.GetMogpFlags(),
                       gm.GetBound().low().x, gm.GetBound().low().y, gm.GetBound().low().z,
                       gm.GetBound().high().x, gm.GetBound().high().y, gm.GetBound().high().z, group_Z, point.z);
#endif
            }
            //}
            //std::cout << "trying to intersect '" << prims[entry].name << "'\n";
        }
    };

    // 检测点是否与 WorldModel 相交
    bool WorldModel::IntersectPoint(const G3D::Vector3& p, const G3D::Vector3& down, float& dist, AreaInfo& info) const
    {
        if (groupModels.empty())
        {
            return false;
        }

        WModelAreaCallback callback(groupModels, down);
        groupTree.intersectPoint(p, callback);
        if (callback.hit != groupModels.end())
        {
            info.rootId = RootWMOID;
            info.groupId = callback.hit->GetWmoID();
            info.flags = callback.hit->GetMogpFlags();
            info.result = true;
            dist = callback.zDist;
            return true;
        }
        return false;
    }

    // 获取指定位置的位置信息
    bool WorldModel::GetLocationInfo(const G3D::Vector3& p, const G3D::Vector3& down, float& dist, LocationInfo& info) const
    {
        if (groupModels.empty())
        {
            return false;
        }

        WModelAreaCallback callback(groupModels, down);
        groupTree.intersectPoint(p, callback);
        if (callback.hit != groupModels.end())
        {
            info.rootId   = RootWMOID;
            info.hitModel = &(*callback.hit);
            dist = callback.zDist;
            return true;
        }
        return false;
    }

    // 将 WorldModel 对象写入文件
    bool WorldModel::writeFile(const std::string& filename)
    {
        FILE* wf = fopen(filename.c_str(), "wb");
        if (!wf)
        {
            return false;
        }

        uint32 chunkSize, count;
        bool result = fwrite(VMAP_MAGIC, 1, 8, wf) == 8;
        if (result && fwrite("WMOD", 1, 4, wf) != 4) { result = false; }
        chunkSize = sizeof(uint32) + sizeof(uint32);
        if (result && fwrite(&chunkSize, sizeof(uint32), 1, wf) != 1) { result = false; }
        if (result && fwrite(&RootWMOID, sizeof(uint32), 1, wf) != 1) { result = false; }

        // 写入组模型数据
        count = groupModels.size();
        if (count)
        {
            if (result && fwrite("GMOD", 1, 4, wf) != 4) { result = false; }
            //chunkSize = sizeof(uint32)+ sizeof(GroupModel)*count;
            //if (result && fwrite(&chunkSize, sizeof(uint32), 1, wf) != 1) result = false;
            if (result && fwrite(&count, sizeof(uint32), 1, wf) != 1) { result = false; }
            for (uint32 i = 0; i < groupModels.size() && result; ++i)
            {
                result = groupModels[i].writeToFile(wf);
            }

            // 写入组 BIH 数据
            if (result && fwrite("GBIH", 1, 4, wf) != 4) { result = false; }
            if (result) { result = groupTree.writeToFile(wf); }
        }

        fclose(wf);
        return result;
    }

    // 从文件中读取 WorldModel 对象
    bool WorldModel::readFile(const std::string& filename)
    {
        FILE* rf = fopen(filename.c_str(), "rb");
        if (!rf)
        {
            return false;
        }

        bool result = true;
        uint32 chunkSize = 0;
        uint32 count = 0;
        char chunk[8];                          // 忽略添加的魔法头
        if (!readChunk(rf, chunk, VMAP_MAGIC, 8)) { result = false; }

        if (result && !readChunk(rf, chunk, "WMOD", 4)) { result = false; }
        if (result && fread(&chunkSize, sizeof(uint32), 1, rf) != 1) { result = false; }
        if (result && fread(&RootWMOID, sizeof(uint32), 1, rf) != 1) { result = false; }

        // 读取组模型数据
        if (result && readChunk(rf, chunk, "GMOD", 4))
        {
            //if (fread(&chunkSize, sizeof(uint32), 1, rf) != 1) result = false;

            if (fread(&count, sizeof(uint32), 1, rf) != 1) { result = false; }
            if (result) { groupModels.resize(count); }
            //if (result && fread(&groupModels[0], sizeof(GroupModel), count, rf) != count) result = false;
            for (uint32 i = 0; i < count && result; ++i)
            {
                result = groupModels[i].readFromFile(rf);
            }

            // 读取组 BIH 数据
            if (result && !readChunk(rf, chunk, "GBIH", 4)) { result = false; }
            if (result) { result = groupTree.readFromFile(rf); }
        }

        fclose(rf);
        return result;
    }

    // 获取 WorldModel 的组模型
    void WorldModel::GetGroupModels(std::vector<GroupModel>& outGroupModels)
    {
        outGroupModels = groupModels;
    }
}
