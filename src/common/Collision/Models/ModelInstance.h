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
 * You should have received the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MODELINSTANCE_H_
#define _MODELINSTANCE_H_

#include "Define.h"
#include <G3D/AABox.h>
#include <G3D/Matrix3.h>
#include <G3D/Ray.h>
#include <G3D/Vector3.h>

namespace VMAP
{
    // 前向声明 WorldModel 类
    class WorldModel;
    // 前向声明 AreaInfo 结构体
    struct AreaInfo;
    // 前向声明 LocationInfo 结构体
    struct LocationInfo;
    // 前向声明模型忽略标志枚举类，使用 uint32 作为底层类型
    enum class ModelIgnoreFlags : uint32;

    // 模型标志枚举
    enum ModelFlags
    {
        MOD_M2 = 1,                 // M2 模型标志
        MOD_WORLDSPAWN = 1 << 1,    // 世界生成模型标志
        MOD_HAS_BOUND = 1 << 2      // 具有边界的模型标志
    };

    // 模型生成信息类
    class ModelSpawn
    {
    public:
        //mapID, tileX, tileY, Flags, ID, Pos, Rot, Scale, Bound_lo, Bound_hi, name
        uint32 flags;      // 模型标志
        uint16 adtId;      // ADT 编号
        uint32 ID;         // 模型 ID
        G3D::Vector3 iPos; // 模型位置
        G3D::Vector3 iRot; // 模型旋转
        float iScale;      // 模型缩放
        G3D::AABox iBound; // 模型边界框
        std::string name;  // 模型名称

        // 判断两个模型生成信息是否相等，通过比较模型 ID
        bool operator==(const ModelSpawn& other) const { return ID == other.ID; }
        //uint32 hashCode() const { return ID; }
        // temp?

        // 获取模型的边界框
        [[nodiscard]] const G3D::AABox& GetBounds() const { return iBound; }

        // 从文件中读取模型生成信息
        static bool readFromFile(FILE* rf, ModelSpawn& spawn);
        // 将模型生成信息写入文件
        static bool writeToFile(FILE* rw, const ModelSpawn& spawn);
    };

    // 模型实例类，继承自 ModelSpawn
    class ModelInstance: public ModelSpawn
    {
    public:
        // 默认构造函数
        ModelInstance() { }
        // 构造函数，使用模型生成信息和 WorldModel 指针初始化
        ModelInstance(const ModelSpawn& spawn, WorldModel* model);
        // 将模型设置为未加载状态
        void setUnloaded() { iModel = nullptr; }
        // 检测射线与模型是否相交
        bool intersectRay(const G3D::Ray& pRay, float& pMaxDist, bool StopAtFirstHit, ModelIgnoreFlags ignoreFlags) const;
        // 检测点与模型的相交信息
        void intersectPoint(const G3D::Vector3& p, AreaInfo& info) const;
        // 获取指定点的位置信息
        bool GetLocationInfo(const G3D::Vector3& p, LocationInfo& info) const;
        // 获取指定点的液体高度信息
        bool GetLiquidLevel(const G3D::Vector3& p, LocationInfo& info, float& liqHeight) const;
        // 获取世界模型指针
        WorldModel* getWorldModel() { return iModel; }
    protected:
        G3D::Matrix3 iInvRot;  // 逆旋转矩阵
        float iInvScale{0.0f}; // 逆缩放因子
        WorldModel* iModel{nullptr}; // 世界模型指针
    };
} // namespace VMAP

#endif // _MODELINSTANCE_H_
