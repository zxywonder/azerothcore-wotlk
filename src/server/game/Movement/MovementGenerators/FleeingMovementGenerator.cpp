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

 #include "FleeingMovementGenerator.h"
 #include "Creature.h"
 #include "CreatureAI.h"
 #include "MapMgr.h"
 #include "MoveSplineInit.h"
 #include "ObjectAccessor.h"
 #include "Player.h"
 
 // 定义玩家与逃离目标之间的最小和最大安静距离，以及最小路径长度
 #define MIN_QUIET_DISTANCE 28.0f
 #define MAX_QUIET_DISTANCE 43.0f
 #define MIN_PATH_LENGTH 2.0f
 
 /// 初始化逃离行为，停止移动，设置逃跑标志和状态，并设置目标位置
 template<class T>
 void FleeingMovementGenerator<T>::DoInitialize(T* owner)
 {
     if (!owner)
     {
         return;
     }
 
     owner->StopMoving();                              // 停止当前移动
     _path = nullptr;                                  // 重置路径生成器
     owner->SetUnitFlag(UNIT_FLAG_FLEEING);            // 设置逃跑标志
     owner->AddUnitState(UNIT_STATE_FLEEING);          // 添加逃跑状态
     SetTargetLocation(owner);                         // 设置目标位置
 }
 
 /// DoFinalize 的通用实现，目前为空
 template<class T>
 void FleeingMovementGenerator<T>::DoFinalize(T*)
 {
 }
 
 /// 为 Player 类型特化 DoFinalize 方法，移除逃跑标志和状态，并停止移动
 template<>
 void FleeingMovementGenerator<Player>::DoFinalize(Player* owner)
 {
     owner->RemoveUnitFlag(UNIT_FLAG_FLEEING);         // 移除逃跑标志
     owner->ClearUnitState(UNIT_STATE_FLEEING);        // 清除逃跑状态
     owner->StopMoving();                              // 停止移动
 }
 
 /// 为 Creature 类型特化 DoFinalize 方法，移除逃跑标志和状态，恢复目标
 template<>
 void FleeingMovementGenerator<Creature>::DoFinalize(Creature* owner)
 {
     owner->RemoveUnitFlag(UNIT_FLAG_FLEEING);                         // 移除逃跑标志
     owner->ClearUnitState(UNIT_STATE_FLEEING | UNIT_STATE_FLEEING_MOVE); // 清除逃跑状态
 
     if (Unit* victim = owner->GetVictim())                            // 如果有当前目标
     {
         owner->SetTarget(victim->GetGUID());                          // 恢复目标
     }
 }
 
 /// 重置逃跑行为，调用 DoInitialize
 template<class T>
 void FleeingMovementGenerator<T>::DoReset(T* owner)
 {
     DoInitialize(owner);
 }
 
 /// 每帧更新逃跑状态，检查是否中断或需要重新计算路径
 template<class T>
 bool FleeingMovementGenerator<T>::DoUpdate(T* owner, uint32 diff)
 {
     if (!owner || !owner->IsAlive())
     {
         return false;
     }
 
     // 如果不能移动或正在施法，停止移动并中断
     if (owner->HasUnitState(UNIT_STATE_NOT_MOVE) || owner->IsMovementPreventedByCasting())
     {
         _path = nullptr;
         _interrupt = true;
         owner->StopMoving();
         return true;
     }
     else
         _interrupt = false;
 
     _timer.Update(diff);  // 更新计时器
     if (!_interrupt && _timer.Passed() && owner->movespline->Finalized())  // 时间到且当前移动已完成
     {
         SetTargetLocation(owner);  // 设置新的目标位置
     }
 
     return true;
 }
 
 /// 设置目标位置，生成逃跑路径
 template<class T>
 void FleeingMovementGenerator<T>::SetTargetLocation(T* owner)
 {
     if (!owner)
     {
         return;
     }
 
     // 如果不能移动或正在施法，停止移动并中断
     if (owner->HasUnitState(UNIT_STATE_NOT_MOVE) || owner->IsMovementPreventedByCasting())
     {
         _path = nullptr;
         _interrupt = true;
         owner->StopMoving();
         return;
     }
 
     owner->AddUnitState(UNIT_STATE_FLEEING_MOVE);  // 添加逃跑移动状态
 
     Position destination = owner->GetPosition();   // 获取当前位置
     GetPoint(owner, destination);                  // 获取目标点
 
     // 检查目标点是否在视野内
     if (!owner->IsWithinLOS(destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ()))
     {
         _timer.Reset(200);  // 重置计时器
         return;
     }
 
     // 初始化路径生成器或重置路径
     if (!_path)
     {
         _path = std::make_unique<PathGenerator>(owner);
     }
     else
     {
         _path->Clear();
     }
 
     if (owner->IsPlayer())
         _path->SetSlopeCheck(true);  // 玩家启用坡度检查
 
     _path->SetPathLengthLimit(30.0f);  // 设置路径长度限制
     bool result = _path->CalculatePath(destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ());
     // 如果路径无效，重置计时器并增加无效路径计数
     if (!result || (_path->GetPathType() & PathType(PATHFIND_NOPATH | PATHFIND_SHORTCUT | PATHFIND_FARFROMPOLY | PATHFIND_NOT_USING_PATH)))
     {
         if (_fleeTargetGUID)
             ++_invalidPathsCount;
 
         _timer.Reset(100);
         return;
     }
 
     // 如果路径太短，视为无效
     if (_path->getPathLength() < MIN_PATH_LENGTH)
     {
         if (_fleeTargetGUID)
             ++_invalidPathsCount;
 
         _timer.Reset(100);
         return;
     }
 
     _invalidPathsCount = 0;  // 重置无效路径计数
 
     Movement::MoveSplineInit init(owner);            // 初始化移动路径
     init.MovebyPath(_path->GetPath());               // 设置路径
     init.SetWalk(false);                             // 设置为奔跑
     int32 traveltime = init.Launch();                // 启动移动
     _timer.Reset(traveltime + urand(800, 1500));     // 重置计时器
 }
 
 /// 获取逃跑目标点的位置
 template<class T>
 void FleeingMovementGenerator<T>::GetPoint(T* owner, Position& position)
 {
     float casterDistance = 0.f;
     float casterAngle = 0.f;
     Unit* fleeTarget = nullptr;
     if (_invalidPathsCount < 5)
         fleeTarget = ObjectAccessor::GetUnit(*owner, _fleeTargetGUID);  // 获取逃跑目标
 
     if (fleeTarget)
     {
         casterDistance = fleeTarget->GetDistance(owner);  // 获取距离
         if (casterDistance > 0.2f)
         {
             casterAngle = fleeTarget->GetAngle(owner);  // 获取角度
         }
         else
         {
             casterAngle = frand(0.0f, 2.0f * float(M_PI));  // 随机角度
         }
     }
     else
     {
         casterDistance = 0.0f;
         casterAngle = frand(0.0f, 2.0f * float(M_PI));  // 随机角度
     }
 
     float distance = 0.f;
     float angle = 0.f;
     if (casterDistance < MIN_QUIET_DISTANCE)
     {
         // 目标太近，远离一点
         distance = frand(0.4f, 1.3f) * (MIN_QUIET_DISTANCE - casterDistance);
         angle = casterAngle + frand(-float(M_PI) / 8.0f, float(M_PI) / 8.0f);
     }
     else if (casterDistance > MAX_QUIET_DISTANCE)
     {
         // 目标太远，随机方向移动
         distance = frand(0.4f, 1.0f) * (MAX_QUIET_DISTANCE - MIN_QUIET_DISTANCE);
         angle = -casterAngle + frand(-float(M_PI) / 4.0f, float(M_PI) / 4.0f);
     }
     else
     {
         // 目标在安全距离内，随机方向移动
         distance = frand(0.6f, 1.2f) * (MAX_QUIET_DISTANCE - MIN_QUIET_DISTANCE);
         angle = frand(0.0f, 2.0f * float(M_PI));
     }
 
     // 调整角度以考虑单位当前方向
     angle -= owner->GetOrientation();
 
     owner->MovePositionToFirstCollision(position, distance, angle);  // 获取第一个碰撞点作为目标点
 }
 
 // 显式模板实例化
 template void FleeingMovementGenerator<Player>::DoInitialize(Player*);
 template void FleeingMovementGenerator<Creature>::DoInitialize(Creature*);
 template void FleeingMovementGenerator<Player>::DoReset(Player*);
 template void FleeingMovementGenerator<Creature>::DoReset(Creature*);
 template bool FleeingMovementGenerator<Player>::DoUpdate(Player*, uint32);
 template bool FleeingMovementGenerator<Creature>::DoUpdate(Creature*, uint32);
 template void FleeingMovementGenerator<Player>::SetTargetLocation(Player*);
 template void FleeingMovementGenerator<Creature>::SetTargetLocation(Creature*);
 template void FleeingMovementGenerator<Player>::GetPoint(Player*, Position&);
 template void FleeingMovementGenerator<Creature>::GetPoint(Creature*, Position&);
 
 /// 结束定时逃跑行为，清理状态，恢复目标，并通知AI
 void TimedFleeingMovementGenerator::Finalize(Unit* owner)
 {
     owner->RemoveUnitFlag(UNIT_FLAG_FLEEING);                         // 移除逃跑标志
     owner->ClearUnitState(UNIT_STATE_FLEEING | UNIT_STATE_FLEEING_MOVE); // 清除逃跑状态
 
     if (Unit* victim = owner->GetVictim())                            // 如果有当前目标
     {
         owner->SetTarget(victim->GetGUID());                          // 恢复目标
     }
 
     if (Creature* ownerCreature = owner->ToCreature())                // 如果是生物
     {
         if (CreatureAI* AI = ownerCreature->AI())                     // 获取AI
         {
             AI->MovementInform(TIMED_FLEEING_MOTION_TYPE, 0);         // 通知AI移动结束
         }
     }
 }
 
 /// 每帧更新定时逃跑状态，检查是否超时
 bool TimedFleeingMovementGenerator::Update(Unit* owner, uint32 time_diff)
 {
     if (!owner->IsAlive())
         return false;
 
     // 如果不能移动或正在施法，停止移动
     if (owner->HasUnitState(UNIT_STATE_NOT_MOVE) || owner->IsMovementPreventedByCasting())
     {
         owner->StopMoving();
         return true;
     }
 
     i_totalFleeTime.Update(time_diff);  // 更新总逃跑时间
     if (i_totalFleeTime.Passed())       // 时间已到
         return false;
 
     // 调用父类的 Update 方法
     return MovementGeneratorMedium< Creature, FleeingMovementGenerator<Creature> >::Update(owner, time_diff);
 }
