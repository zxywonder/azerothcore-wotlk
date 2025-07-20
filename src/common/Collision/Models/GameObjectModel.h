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

// 防止头文件重复包含
#ifndef _GAMEOBJECT_MODEL_H
#define _GAMEOBJECT_MODEL_H

// 包含自定义定义头文件
#include "Define.h"
// 包含G3D库的轴对齐边界框类头文件
#include <G3D/AABox.h>
// 包含G3D库的3x3矩阵类头文件
#include <G3D/Matrix3.h>
// 包含G3D库的射线类头文件
#include <G3D/Ray.h>
// 包含G3D库的三维向量类头文件
#include <G3D/Vector3.h>

// 声明VMAP命名空间
namespace VMAP
{
    // 前向声明WorldModel类
    class WorldModel;
    // 前向声明AreaInfo结构体
    struct AreaInfo;
    // 前向声明LocationInfo结构体
    struct LocationInfo;
    // 前向声明模型忽略标志枚举类
    enum class ModelIgnoreFlags : uint32;
}

// 前向声明GameObject类
class GameObject;
// 前向声明GameObjectDisplayInfoEntry结构体
struct GameObjectDisplayInfoEntry;

// 游戏对象模型所有者基类
class GameObjectModelOwnerBase
{
public:
    // 虚析构函数，确保正确释放派生类对象
    virtual ~GameObjectModelOwnerBase() = default;

    // 获取是否已生成的状态，返回布尔值
    [[nodiscard]] virtual bool IsSpawned() const = 0;
    // 获取显示ID，返回无符号32位整数
    [[nodiscard]] virtual uint32 GetDisplayId() const = 0;
    // 获取相位掩码，返回无符号32位整数
    [[nodiscard]] virtual uint32 GetPhaseMask() const = 0;
    // 获取位置，返回三维向量
    [[nodiscard]] virtual G3D::Vector3 GetPosition() const = 0;
    // 获取朝向，返回浮点数
    [[nodiscard]] virtual float GetOrientation() const = 0;
    // 获取缩放比例，返回浮点数
    [[nodiscard]] virtual float GetScale() const = 0;
    // 调试可视化角落，参数为三维向量
    virtual void DebugVisualizeCorner(G3D::Vector3 const& /*corner*/) const = 0;
};

// 游戏对象模型类
class GameObjectModel
{
    // 默认构造函数
    GameObjectModel()  = default;

public:
    // 模型名称
    std::string name;

    // 获取边界框，返回轴对齐边界框的常量引用
    [[nodiscard]] const G3D::AABox& GetBounds() const { return iBound; }

    // 析构函数
    ~GameObjectModel();

    // 获取位置，返回三维向量的常量引用
    [[nodiscard]] const G3D::Vector3& GetPosition() const { return iPos; }

    /** Enables\disables collision. */
    // 禁用碰撞
    void disable() { phasemask = 0; }
    // 启用碰撞，参数为相位掩码
    void enable(uint32 ph_mask) { phasemask = ph_mask; }

    // 判断是否已启用，返回布尔值
    [[nodiscard]] bool isEnabled() const { return phasemask != 0; }
    // 判断是否为地图对象，返回布尔值
    [[nodiscard]] bool IsMapObject() const { return isWmo; }

    // 射线相交检测，参数为射线、最大距离、是否首次命中停止、相位掩码和模型忽略标志，返回布尔值
    bool intersectRay(const G3D::Ray& Ray, float& MaxDist, bool StopAtFirstHit, uint32 ph_mask, VMAP::ModelIgnoreFlags ignoreFlags) const;
    // 点相交检测，参数为点、区域信息和相位掩码
    void IntersectPoint(G3D::Vector3 const& point, VMAP::AreaInfo& info, uint32 ph_mask) const;
    // 获取位置信息，参数为点、位置信息和相位掩码，返回布尔值
    bool GetLocationInfo(G3D::Vector3 const& point, VMAP::LocationInfo& info, uint32 ph_mask) const;
    // 获取液体高度，参数为点、位置信息和液体高度，返回布尔值
    bool GetLiquidLevel(G3D::Vector3 const& point, VMAP::LocationInfo& info, float& liqHeight) const;

    // 创建游戏对象模型，参数为模型所有者的唯一指针和数据路径，返回游戏对象模型指针
    static GameObjectModel* Create(std::unique_ptr<GameObjectModelOwnerBase> modelOwner, std::string const& dataPath);

    // 更新位置，返回布尔值表示是否成功
    bool UpdatePosition();

private:
    // 初始化游戏对象模型，参数为模型所有者的唯一指针和数据路径，返回布尔值表示是否成功
    bool initialize(std::unique_ptr<GameObjectModelOwnerBase> modelOwner, std::string const& dataPath);

    // 相位掩码
    uint32 phasemask{0};
    // 轴对齐边界框
    G3D::AABox iBound;
    // 逆旋转矩阵
    G3D::Matrix3 iInvRot;
    // 位置
    G3D::Vector3 iPos;
    // 逆缩放比例
    float iInvScale{0};
    // 缩放比例
    float iScale{0};
    // 世界模型指针
    VMAP::WorldModel* iModel{nullptr};
    // 模型所有者的唯一指针
    std::unique_ptr<GameObjectModelOwnerBase> owner;
    // 是否为WMO（World Model Object）对象
    bool isWmo{false};
};

// 加载游戏对象模型列表，参数为数据路径
void LoadGameObjectModelList(std::string const& dataPath);

#endif // _GAMEOBJECT_MODEL_H
