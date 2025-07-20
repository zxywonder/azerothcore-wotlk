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

 #ifndef ACORE_POINTMOVEMENTGENERATOR_H
 #define ACORE_POINTMOVEMENTGENERATOR_H
 
 #include "Creature.h"
 #include "MovementGenerator.h"
 
 // PointMovementGenerator 类模板，用于处理单位移动到指定点的逻辑
 template<class T>
 class PointMovementGenerator : public MovementGeneratorMedium< T, PointMovementGenerator<T> >
 {
 public:
     // 构造函数，初始化目标点坐标、速度、朝向等参数
     PointMovementGenerator(uint32 _id, float _x, float _y, float _z, float _speed = 0.0f, float orientation = 0.0f, const Movement::PointsArray* _path = nullptr,
         bool generatePath = false, bool forceDestination = false, ObjectGuid chargeTargetGUID = ObjectGuid::Empty)
         : id(_id), i_x(_x), i_y(_y), i_z(_z), speed(_speed), i_orientation(orientation), _generatePath(generatePath), _forceDestination(forceDestination),
         _chargeTargetGUID(chargeTargetGUID)
     {
         // 如果提供了路径，则复制路径数据
         if (_path)
             m_precomputedPath = *_path;
     }
 
     // 初始化移动生成器
     void DoInitialize(T*);
     // 完成时调用，清理资源
     void DoFinalize(T*);
     // 重置移动生成器
     void DoReset(T*);
     // 更新移动状态，返回是否继续
     bool DoUpdate(T*, uint32);
 
     // 移动通知，用于触发某些事件
     void MovementInform(T*);
 
     // 标记速度需要重新计算
     void unitSpeedChanged() { i_recalculateSpeed = true; }
 
     // 获取当前移动生成器的类型
     MovementGeneratorType GetMovementGeneratorType() { return POINT_MOTION_TYPE; }
 
     // 获取目标点坐标
     bool GetDestination(float& x, float& y, float& z) const { x = i_x; y = i_y; z = i_z; return true; }
 private:
     uint32 id;                          // 移动生成器的ID
     float i_x, i_y, i_z;                // 目标点坐标
     float speed;                        // 移动速度
     float i_orientation;                // 目标朝向
     bool i_recalculateSpeed;            // 是否需要重新计算速度
     Movement::PointsArray m_precomputedPath; // 预计算的路径点数组
     bool _generatePath;                 // 是否需要生成路径
     bool _forceDestination;             // 是否强制到达目标点
     ObjectGuid _chargeTargetGUID;       // 冲刺目标的GUID
 };
 
 // 助攻移动生成器，继承自 PointMovementGenerator
 class AssistanceMovementGenerator : public PointMovementGenerator<Creature>
 {
 public:
     // 构造函数，设置目标点
     AssistanceMovementGenerator(float _x, float _y, float _z) :
         PointMovementGenerator<Creature>(0, _x, _y, _z) {}
 
     // 获取移动生成器类型
     MovementGeneratorType GetMovementGeneratorType() { return ASSISTANCE_MOTION_TYPE; }
     // 完成时调用，处理单位清理逻辑
     void Finalize(Unit*);
 };
 
 // 效果移动生成器，用于防止其他移动生成器中断当前效果
 class EffectMovementGenerator : public MovementGenerator
 {
 public:
     // 构造函数，设置ID
     explicit EffectMovementGenerator(uint32 Id) : m_Id(Id) {}
     // 初始化方法，无实际操作
     void Initialize(Unit*) override {}
     // 完成方法，执行清理操作
     void Finalize(Unit*) override;
     // 重置方法，无实际操作
     void Reset(Unit*) override {}
     // 更新方法，返回是否继续
     bool Update(Unit*, uint32) override;
     // 获取移动生成器类型
     MovementGeneratorType GetMovementGeneratorType() override { return EFFECT_MOTION_TYPE; }
 private:
     uint32 m_Id; // 效果ID
 };
 
 #endif
