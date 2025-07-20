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

 #include "ConfusedMovementGenerator.h"
 #include "Creature.h"
 #include "MapMgr.h"
 #include "MoveSplineInit.h"
 #include "Player.h"
 
 template<class T>
 void ConfusedMovementGenerator<T>::DoInitialize(T* unit)
 {
     unit->StopMoving(); // 停止当前移动
     float const wander_distance = 4; // 混乱移动的范围半径
     float x = unit->GetPositionX(); // 获取当前位置 x 坐标
     float y = unit->GetPositionY(); // 获取当前位置 y 坐标
     float z = unit->GetPositionZ(); // 获取当前位置 z 坐标
 
     Map const* map = unit->GetMap(); // 获取当前地图
 
     bool is_water_ok, is_land_ok;
     _InitSpecific(unit, is_water_ok, is_land_ok); // 初始化特定于类型的移动属性（是否可进入水或陆地）
 
     for (uint8 idx = 0; idx < MAX_CONF_WAYPOINTS + 1; ++idx)
     {
         // 随机生成新的 x 和 y 坐标，范围在 wander_distance 内
         float wanderX = x + (wander_distance * (float)rand_norm() - wander_distance / 2);
         float wanderY = y + (wander_distance * (float)rand_norm() - wander_distance / 2);
 
         // 防止生成无效坐标
         Acore::NormalizeMapCoord(wanderX);
         Acore::NormalizeMapCoord(wanderY);
 
         float new_z = unit->GetMapHeight(wanderX, wanderY, z); // 获取目标点的高度
         if (new_z <= INVALID_HEIGHT || std::fabs(z - new_z) > 3.0f) // 如果高度无效或差异过大
         {
             // 使用上一个有效坐标或当前位置
             i_waypoints[idx][0] = idx > 0 ? i_waypoints[idx - 1][0] : x;
             i_waypoints[idx][1] = idx > 0 ? i_waypoints[idx - 1][1] : y;
             i_waypoints[idx][2] = idx > 0 ? i_waypoints[idx - 1][2] : z;
             continue;
         }
         else if (unit->IsWithinLOS(wanderX, wanderY, z)) // 检查是否在视野范围内
         {
             // 判断是否为水域
             bool is_water = map->IsInWater(unit->GetPhaseMask(), wanderX, wanderY, z, unit->GetCollisionHeight());
 
             // 如果不能进入水域或陆地，则使用上一个有效坐标
             if ((is_water && !is_water_ok) || (!is_water && !is_land_ok))
             {
                 i_waypoints[idx][0] = idx > 0 ? i_waypoints[idx - 1][0] : x;
                 i_waypoints[idx][1] = idx > 0 ? i_waypoints[idx - 1][1] : y;
                 i_waypoints[idx][2] = idx > 0 ? i_waypoints[idx - 1][2] : z;
                 continue;
             }
         }
         else
         {
             // 目标点不在视线范围内，使用上一个有效坐标
             i_waypoints[idx][0] = idx > 0 ? i_waypoints[idx - 1][0] : x;
             i_waypoints[idx][1] = idx > 0 ? i_waypoints[idx - 1][1] : y;
             i_waypoints[idx][2] = idx > 0 ? i_waypoints[idx - 1][2] : z;
             continue;
         }
 
         // 位置有效，保存到路径点数组
         i_waypoints[idx][0] = wanderX;
         i_waypoints[idx][1] = wanderY;
         i_waypoints[idx][2] = new_z;
     }
 
     // 初始化下一次移动的索引
     i_nextMove = urand(1, MAX_CONF_WAYPOINTS);
     DoUpdate(unit, 1); // 立即更新一次移动
 
     // 设置单位为混乱状态
     unit->SetUnitFlag(UNIT_FLAG_CONFUSED);
     unit->AddUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_CONFUSED_MOVE);
 }
 
 template<>
 void ConfusedMovementGenerator<Creature>::_InitSpecific(Creature* creature, bool& is_water_ok, bool& is_land_ok)
 {
     // 为 Creature 设置是否允许进入水域或陆地
     is_water_ok = creature->CanEnterWater();
     is_land_ok  = creature->CanWalk();
 }
 
 template<>
 void ConfusedMovementGenerator<Player>::_InitSpecific(Player*, bool& is_water_ok, bool& is_land_ok)
 {
     // 玩家始终允许进入水域和陆地
     is_water_ok = true;
     is_land_ok  = true;
 }
 
 template<class T>
 void ConfusedMovementGenerator<T>::DoReset(T* unit)
 {
     // 重置混乱移动，重新初始化
     DoInitialize(unit);
 }
 
 template<class T>
 bool ConfusedMovementGenerator<T>::DoUpdate(T* unit, uint32 diff)
 {
     // 如果单位不能移动或在施法中阻止移动
     if (unit->HasUnitState(UNIT_STATE_NOT_MOVE) || unit->IsMovementPreventedByCasting())
     {
         unit->StopMoving(); // 停止移动
         return true;
     }
 
     if (i_nextMoveTime.Passed())
     {
         // 当前正在移动，更新位置
         unit->AddUnitState(UNIT_STATE_CONFUSED_MOVE);
 
         if (unit->movespline->Finalized()) // 如果当前移动已完成
         {
             i_nextMove = urand(1, MAX_CONF_WAYPOINTS); // 随机选择下一个路径点
             i_nextMoveTime.Reset(urand(600, 1200)); // 设置下一次移动的时间
         }
     }
     else
     {
         // 等待下一次移动开始
         i_nextMoveTime.Update(diff);
         if (i_nextMoveTime.Passed())
         {
             // 开始移动到下一个路径点
             unit->AddUnitState(UNIT_STATE_CONFUSED_MOVE);
 
             ASSERT(i_nextMove <= MAX_CONF_WAYPOINTS);
             float x = i_waypoints[i_nextMove][0];
             float y = i_waypoints[i_nextMove][1];
             float z = i_waypoints[i_nextMove][2];
 
             // 初始化移动路径
             Movement::MoveSplineInit init(unit);
             init.MoveTo(x, y, z, true);
             init.Launch(); // 启动移动
         }
     }
 
     return true;
 }
 
 template<>
 void ConfusedMovementGenerator<Player>::DoFinalize(Player* unit)
 {
     // 玩家结束混乱移动，清除状态和标志
     unit->RemoveUnitFlag(UNIT_FLAG_CONFUSED);
     unit->ClearUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_CONFUSED_MOVE);
     unit->StopMoving(); // 停止移动
 }
 
 template<>
 void ConfusedMovementGenerator<Creature>::DoFinalize(Creature* unit)
 {
     // 怪物结束混乱移动，清除状态和标志
     unit->RemoveUnitFlag(UNIT_FLAG_CONFUSED);
     unit->ClearUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_CONFUSED_MOVE);
     if (unit->GetVictim()) // 如果有目标，重新设置目标
         unit->SetTarget(unit->GetVictim()->GetGUID());
 }
 
 // 显式实例化模板函数
 template void ConfusedMovementGenerator<Player>::DoInitialize(Player*);
 template void ConfusedMovementGenerator<Creature>::DoInitialize(Creature*);
 template void ConfusedMovementGenerator<Player>::DoReset(Player*);
 template void ConfusedMovementGenerator<Creature>::DoReset(Creature*);
 template bool ConfusedMovementGenerator<Player>::DoUpdate(Player*, uint32 diff);
 template bool ConfusedMovementGenerator<Creature>::DoUpdate(Creature*, uint32 diff);
