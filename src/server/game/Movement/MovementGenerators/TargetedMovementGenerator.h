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

 #ifndef ACORE_TARGETEDMOVEMENTGENERATOR_H
 #define ACORE_TARGETEDMOVEMENTGENERATOR_H
 
 #include "FollowerReference.h"
 #include "MovementGenerator.h"
 #include "Optional.h"
 #include "PathGenerator.h"
 #include "Timer.h"
 #include "Unit.h"
 
 // TargetedMovementGeneratorBase 是一个基础类，用于管理跟随目标的引用
 class TargetedMovementGeneratorBase
 {
 public:
     // 构造函数，绑定目标单位
     TargetedMovementGeneratorBase(Unit* target) { i_target.link(target, this); }
     // 停止跟随的方法，目前为空实现
     void stopFollowing() { }
 protected:
     // 跟随目标的引用
     FollowerReference i_target;
 };
 
 // ChaseMovementGenerator 是一个模板类，用于生成追逐目标的移动逻辑
 template<class T>
 class ChaseMovementGenerator : public MovementGeneratorMedium<T, ChaseMovementGenerator<T>>, public TargetedMovementGeneratorBase
 {
 public:
     // 构造函数，初始化目标、范围、角度等参数
     ChaseMovementGenerator(Unit* target, Optional<ChaseRange> range = {}, Optional<ChaseAngle> angle = {})
         : TargetedMovementGeneratorBase(target), i_leashExtensionTimer(5000), i_path(nullptr), i_recheckDistance(0), i_recalculateTravel(true), _range(range), _angle(angle) {}
     ~ChaseMovementGenerator() { }
 
     // 获取移动生成器类型，返回 CHASE_MOTION_TYPE
     MovementGeneratorType GetMovementGeneratorType() { return CHASE_MOTION_TYPE; }
 
     // 更新追逐逻辑
     bool DoUpdate(T*, uint32);
     // 初始化追逐行为
     void DoInitialize(T*);
     // 结束追逐行为
     void DoFinalize(T*);
     // 重置追逐行为
     void DoReset(T*);
     // 移动通知处理
     void MovementInform(T*);
 
     // 检查目标位置是否符合追逐条件
     bool PositionOkay(T* owner, Unit* target, Optional<float> maxDistance, Optional<ChaseAngle> angle);
 
     // 当单位速度改变时，重置上一个目标位置缓存
     void unitSpeedChanged() { _lastTargetPosition.reset(); }
     // 获取当前追逐的目标单位
     Unit* GetTarget() const { return i_target.getTarget(); }
 
     // 是否启用步行状态（默认不启用）
     bool EnableWalking() const { return false; }
     // 是否丢失目标
     bool HasLostTarget(Unit* unit) const { return unit->GetVictim() != this->GetTarget(); }
 
 private:
     // 拖曳扩展定时器
     TimeTrackerSmall i_leashExtensionTimer;
     // 路径生成器指针
     std::unique_ptr<PathGenerator> i_path;
     // 重新检查距离的定时器
     TimeTrackerSmall i_recheckDistance;
     // 是否需要重新计算路径
     bool i_recalculateTravel;
 
     // 上一个目标位置缓存
     Optional<Position> _lastTargetPosition;
     // 追逐范围
     Optional<ChaseRange> const _range;
     // 追逐角度
     Optional<ChaseAngle> const _angle;
     // 是否正在向目标移动
     bool _movingTowards = true;
     // 是否是相互追逐
     bool _mutualChase = true;
 };
 
 // FollowMovementGenerator 是一个模板类，用于生成跟随目标的移动逻辑
 template<class T>
 class FollowMovementGenerator : public MovementGeneratorMedium<T, FollowMovementGenerator<T>>, public TargetedMovementGeneratorBase
 {
 public:
     // 构造函数，设置目标、范围、角度及是否继承行走状态和速度
     FollowMovementGenerator(Unit* target, float range, ChaseAngle angle, bool inheritWalkState, bool inheritSpeed)
         : TargetedMovementGeneratorBase(target), i_path(nullptr), i_recheckPredictedDistanceTimer(0), i_recheckPredictedDistance(false), _range(range), _angle(angle),_inheritWalkState(inheritWalkState), _inheritSpeed(inheritSpeed) {}
     ~FollowMovementGenerator() { }
 
     // 获取移动生成器类型，返回 FOLLOW_MOTION_TYPE
     MovementGeneratorType GetMovementGeneratorType() { return FOLLOW_MOTION_TYPE; }
 
     // 更新跟随逻辑
     bool DoUpdate(T*, uint32);
     // 初始化跟随行为
     void DoInitialize(T*);
     // 结束跟随行为
     void DoFinalize(T*);
     // 重置跟随行为
     void DoReset(T*);
     // 移动通知处理
     void MovementInform(T*);
 
     // 获取当前跟随的目标单位
     Unit* GetTarget() const { return i_target.getTarget(); }
 
     // 当单位速度改变时，重置上一个目标位置缓存
     void unitSpeedChanged() { _lastTargetPosition.reset(); }
 
     // 判断目标位置是否适合跟随
     bool PositionOkay(Unit* target, bool isPlayerPet, bool& targetIsMoving, uint32 diff);
 
     // 清除单位的跟随移动状态
     static void _clearUnitStateMove(T* u) { u->ClearUnitState(UNIT_STATE_FOLLOW_MOVE); }
     // 添加单位的跟随移动状态
     static void _addUnitStateMove(T* u) { u->AddUnitState(UNIT_STATE_FOLLOW_MOVE); }
 
     // 获取当前的跟随距离
     float GetFollowRange() const { return _range; }
 
 private:
     // 路径生成器指针
     std::unique_ptr<PathGenerator> i_path;
     // 重新检查预测距离的定时器
     TimeTrackerSmall i_recheckPredictedDistanceTimer;
     // 是否需要重新检查预测距离
     bool i_recheckPredictedDistance;
 
     // 上一个目标位置缓存
     Optional<Position> _lastTargetPosition;
     // 上一个预测位置缓存
     Optional<Position> _lastPredictedPosition;
     // 跟随范围
     float _range;
     // 跟随角度
     ChaseAngle _angle;
     // 是否继承目标的行走状态
     bool _inheritWalkState;
     // 是否继承目标的速度
     bool _inheritSpeed;
 };
 
 #endif
