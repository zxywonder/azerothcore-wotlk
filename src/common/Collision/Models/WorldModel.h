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

#ifndef _WORLDMODEL_H
#define _WORLDMODEL_H

#include "BoundingIntervalHierarchy.h"
#include "Define.h"
#include <G3D/AABox.h>
#include <G3D/Ray.h>
#include <G3D/Vector3.h>

namespace VMAP
{
    class TreeNode;
    struct AreaInfo;
    struct LocationInfo;
    enum class ModelIgnoreFlags : uint32;

    /**
     * @brief 表示网格中的一个三角形，存储三个顶点索引
     */
    class MeshTriangle
    {
    public:
        MeshTriangle() {}
        MeshTriangle(uint32 na, uint32 nb, uint32 nc) : idx0(na), idx1(nb), idx2(nc) {}

        uint32 idx0{ 0 }; //!< 第一个顶点索引
        uint32 idx1{ 0 }; //!< 第二个顶点索引
        uint32 idx2{ 0 }; //!< 第三个顶点索引
    };

    /**
     * @brief 描述WMO文件中的液态信息
     */
    class WmoLiquid
    {
    public:
        /**
         * @brief 构造函数
         * @param width 宽度（x方向上的瓦片数量）
         * @param height 高度（y方向上的瓦片数量）
         * @param corner 起始角点坐标
         * @param type 液体类型
         */
        WmoLiquid(uint32 width, uint32 height, const G3D::Vector3& corner, uint32 type);

        /**
         * @brief 拷贝构造函数
         * @param other 另一个WmoLiquid对象
         */
        WmoLiquid(const WmoLiquid& other);

        /**
         * @brief 析构函数
         */
        ~WmoLiquid();

        /**
         * @brief 赋值运算符
         * @param other 另一个WmoLiquid对象
         * @return 当前对象的引用
         */
        WmoLiquid& operator=(const WmoLiquid& other);

        /**
         * @brief 获取指定位置的液体高度
         * @param pos 位置坐标
         * @param liqHeight 输出参数，液体高度
         * @return 是否成功获取高度
         */
        bool GetLiquidHeight(const G3D::Vector3& pos, float& liqHeight) const;

        /**
         * @brief 获取液体类型
         * @return 液体类型
         */
        [[nodiscard]] uint32 GetType() const { return iType; }

        /**
         * @brief 获取高度数据存储指针
         * @return 高度数据指针
         */
        float* GetHeightStorage() { return iHeight; }

        /**
         * @brief 获取标志数据存储指针
         * @return 标志数据指针
         */
        uint8* GetFlagsStorage() { return iFlags; }

        /**
         * @brief 获取文件大小
         * @return 文件大小（字节）
         */
        uint32 GetFileSize();

        /**
         * @brief 将液体数据写入文件
         * @param wf 文件指针
         * @return 是否写入成功
         */
        bool writeToFile(FILE* wf);

        /**
         * @brief 从文件读取液体数据
         * @param rf 文件指针
         * @param liquid 输出参数，读取后的液体对象
         * @return 是否读取成功
         */
        static bool readFromFile(FILE* rf, WmoLiquid*& liquid);

        /**
         * @brief 获取位置信息
         * @param tilesX 输出参数，x方向瓦片数量
         * @param tilesY 输出参数，y方向瓦片数量
         * @param corner 输出参数，起始角点坐标
         */
        void GetPosInfo(uint32& tilesX, uint32& tilesY, G3D::Vector3& corner) const;

    private:
        WmoLiquid() {}

        uint32 iTilesX{ 0 };       //!< x方向上的瓦片数量
        uint32 iTilesY{ 0 };       //!< y方向上的瓦片数量
        G3D::Vector3 iCorner;    //!< 起始角点坐标
        uint32 iType{ 0 };         //!< 液体类型
        float* iHeight{ nullptr }; //!< 高度数组（(tilesX + 1)*(tilesY + 1) 个高度值）
        uint8* iFlags{ nullptr };  //!< 液体瓦片使用标志
    };

    /**
     * @brief 存储WMO组文件的附加信息
     */
    class GroupModel
    {
    public:
        GroupModel() {}

        /**
         * @brief 拷贝构造函数
         * @param other 另一个GroupModel对象
         */
        GroupModel(const GroupModel& other);

        /**
         * @brief 构造函数
         * @param mogpFlags 组标志
         * @param groupWMOID 组的WMO ID
         * @param bound 包围盒
         */
        GroupModel(uint32 mogpFlags, uint32 groupWMOID, const G3D::AABox& bound) :
            iBound(bound), iMogpFlags(mogpFlags), iGroupWMOID(groupWMOID), iLiquid(nullptr) {
        }

        /**
         * @brief 析构函数
         */
        ~GroupModel() { delete iLiquid; }

        /**
         * @brief 设置网格数据并创建BIH
         * @param vert 顶点数组
         * @param tri 三角形数组
         */
        void setMeshData(std::vector<G3D::Vector3>& vert, std::vector<MeshTriangle>& tri);

        /**
         * @brief 设置液体数据
         * @param liquid 液体对象指针
         */
        void setLiquidData(WmoLiquid*& liquid) { iLiquid = liquid; liquid = nullptr; }

        /**
         * @brief 判断射线是否与该组模型相交
         * @param ray 射线
         * @param distance 输出参数，相交距离
         * @param stopAtFirstHit 是否在第一次相交时停止
         * @return 是否相交
         */
        bool IntersectRay(const G3D::Ray& ray, float& distance, bool stopAtFirstHit) const;

        /**
         * @brief 判断点是否在对象内部
         * @param pos 位置坐标
         * @param down 向下方向
         * @param z_dist 输出参数，z轴距离
         * @return 是否在内部
         */
        bool IsInsideObject(const G3D::Vector3& pos, const G3D::Vector3& down, float& z_dist) const;

        /**
         * @brief 获取指定位置的液面高度
         * @param pos 位置坐标
         * @param liqHeight 输出参数，液面高度
         * @return 是否成功获取
         */
        bool GetLiquidLevel(const G3D::Vector3& pos, float& liqHeight) const;

        /**
         * @brief 获取液体类型
         * @return 液体类型
         */
        [[nodiscard]] uint32 GetLiquidType() const;

        /**
         * @brief 将模型数据写入文件
         * @param wf 文件指针
         * @return 是否写入成功
         */
        bool writeToFile(FILE* wf);

        /**
         * @brief 从文件读取模型数据
         * @param rf 文件指针
         * @return 是否读取成功
         */
        bool readFromFile(FILE* rf);

        /**
         * @brief 获取包围盒
         * @return 包围盒
         */
        [[nodiscard]] const G3D::AABox& GetBound() const { return iBound; }

        /**
         * @brief 获取MOGP标志
         * @return MOGP标志
         */
        [[nodiscard]] uint32 GetMogpFlags() const { return iMogpFlags; }

        /**
         * @brief 获取WMO ID
         * @return WMO ID
         */
        [[nodiscard]] uint32 GetWmoID() const { return iGroupWMOID; }

        /**
         * @brief 获取网格数据
         * @param outVertices 输出顶点数组
         * @param outTriangles 输出三角形数组
         * @param liquid 输出液体对象
         */
        void GetMeshData(std::vector<G3D::Vector3>& outVertices, std::vector<MeshTriangle>& outTriangles, WmoLiquid*& liquid);

    protected:
        G3D::AABox iBound;                //!< 包围盒
        uint32 iMogpFlags{ 0 };             //!< 标志位（0x8 户外；0x2000 室内）
        uint32 iGroupWMOID{ 0 };            //!< 组的WMO ID
        std::vector<G3D::Vector3> vertices; //!< 顶点列表
        std::vector<MeshTriangle> triangles; //!< 三角形列表
        BIH meshTree;                       //!< 网格树结构（Bounding Interval Hierarchy）
        WmoLiquid* iLiquid{ nullptr };        //!< 液体对象指针
    };

    /**
     * @brief 表示模型（转换后的M2或WMO）在其原始坐标空间中的状态
     */
    class WorldModel
    {
    public:
        WorldModel() {}

        /**
         * @brief 设置组模型并创建BIH
         * @param models 组模型列表
         */
        void setGroupModels(std::vector<GroupModel>& models);

        /**
         * @brief 设置根WMO ID
         * @param id 根WMO ID
         */
        void setRootWmoID(uint32 id) { RootWMOID = id; }

        /**
         * @brief 判断射线是否与世界模型相交
         * @param ray 射线
         * @param distance 输出参数，相交距离
         * @param stopAtFirstHit 是否在第一次相交时停止
         * @param ignoreFlags 忽略标志
         * @return 是否相交
         */
        bool IntersectRay(const G3D::Ray& ray, float& distance, bool stopAtFirstHit, ModelIgnoreFlags ignoreFlags) const;

        /**
         * @brief 判断点是否与模型相交并获取区域信息
         * @param p 位置坐标
         * @param down 向下方向
         * @param dist 输出参数，距离
         * @param info 输出参数，区域信息
         * @return 是否相交
         */
        bool IntersectPoint(const G3D::Vector3& p, const G3D::Vector3& down, float& dist, AreaInfo& info) const;

        /**
         * @brief 获取指定位置的信息
         * @param p 位置坐标
         * @param down 向下方向
         * @param dist 输出参数，距离
         * @param info 输出参数，位置信息
         * @return 是否成功获取
         */
        bool GetLocationInfo(const G3D::Vector3& p, const G3D::Vector3& down, float& dist, LocationInfo& info) const;

        /**
         * @brief 将模型写入文件
         * @param filename 文件名
         * @return 是否写入成功
         */
        bool writeFile(const std::string& filename);

        /**
         * @brief 从文件读取模型
         * @param filename 文件名
         * @return 是否读取成功
         */
        bool readFile(const std::string& filename);

        /**
         * @brief 获取组模型列表
         * @param outGroupModels 输出参数，组模型列表
         */
        void GetGroupModels(std::vector<GroupModel>& outGroupModels);

        uint32 Flags; //!< 模型标志
    protected:
        uint32 RootWMOID{ 0 };           //!< 根WMO ID
        std::vector<GroupModel> groupModels; //!< 组模型列表
        BIH groupTree;                 //!< 组树结构（Bounding Interval Hierarchy）
    };
} // namespace VMAP

#endif // _WORLDMODEL_H
