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

 #ifndef ACORE_FLEEINGMOVEMENTGENERATOR_H
 #define ACORE_FLEEINGMOVEMENTGENERATOR_H
 
 #include "Creature.h"
 #include "MovementGenerator.h"
 #include "Timer.h"
 
 /// 模板类 FleeingMovementGenerator，用于实现生物逃离行为的移动生成器
 template<class T>
 class FleeingMovementGenerator : public MovementGeneratorMedium< T, FleeingMovementGenerator<T> >
 {
     public:
         /// 构造函数，初始化逃离目标GUID和相关变量
         explicit FleeingMovementGenerator(ObjectGuid fleeTargetGUID) : _path(nullptr), _fleeTargetGUID(fleeTargetGUID), _timer(0), _interrupt(false), _invalidPathsCount(0) { }
 
         /// 获取移动生成器类型，返回 FLEEING_MOTION_TYPE
         MovementGeneratorType GetMovementGeneratorType() override { return FLEEING_MOTION_TYPE; }
 
         /// 初始化逃离行为，设置初始状态
         void DoInitialize(T*);
         /// 结束逃离行为时调用，清理资源
         void DoFinalize(T*);
         /// 重置逃离行为
         void DoReset(T*);
         /// 更新逃离状态，每帧调用，返回是否继续更新
         bool DoUpdate(T*, uint32);
 
     private:
         /// 设置目标位置，生成逃跑路径
         void SetTargetLocation(T*);
         /// 获取一个逃跑点的位置
         void GetPoint(T*, Position& position);
 
         std::unique_ptr<PathGenerator> _path;         // 路径生成器，用于生成逃跑路径
         ObjectGuid _fleeTargetGUID;                   // 逃离的目标GUID
         TimeTracker _timer;                           // 计时器，用于控制逃跑行为的时间间隔
         bool _interrupt;                              // 是否中断逃跑
         uint8 _invalidPathsCount;                     // 失败路径的数量统计
 };
 
 /// 定时逃离移动生成器类，基于 FleeingMovementGenerator，增加逃跑时间限制
 class TimedFleeingMovementGenerator : public FleeingMovementGenerator<Creature>
 {
 public:
     /// 构造函数，设置逃离目标和总逃跑时间
     TimedFleeingMovementGenerator(ObjectGuid fright, uint32 time) :
         FleeingMovementGenerator<Creature>(fright),
         i_totalFleeTime(time) {}
 
     /// 获取移动生成器类型，返回 TIMED_FLEEING_MOTION_TYPE
     MovementGeneratorType GetMovementGeneratorType() { return TIMED_FLEEING_MOTION_TYPE; }
 
     /// 每帧更新定时逃跑状态，检查是否超时
     bool Update(Unit*, uint32);
 
     /// 结束定时逃跑行为，调用基类Finalize并执行额外清理
     void Finalize(Unit*);
 
 private:
     TimeTracker i_totalFleeTime;  // 总逃跑时间计时器
 };
 
 #endif
