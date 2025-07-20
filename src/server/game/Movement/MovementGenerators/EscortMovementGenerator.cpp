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

 #include "EscortMovementGenerator.h"
 #include "Creature.h"
 #include "CreatureAI.h"
 #include "MoveSplineInit.h"
 #include "Player.h"
 
 // 初始化单位的护送移动行为
 // 停止单位当前移动，设置状态，并初始化路径移动
 template<class T>
 void EscortMovementGenerator<T>::DoInitialize(T* unit)
 {
     if (!unit->IsStopped())
         unit->StopMoving();
 
     unit->AddUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
     i_recalculateSpeed = false;
     Movement::MoveSplineInit init(unit);
 
     // 如果路径点数量为2，则使用MoveTo进行简单移动
     if (m_precomputedPath.size() == 2) // xinef: simple case, just call move to
         init.MoveTo(m_precomputedPath[1].x, m_precomputedPath[1].y, m_precomputedPath[1].z, true);
     else if (m_precomputedPath.size())
         init.MovebyPath(m_precomputedPath);
 
     init.Launch();
 
     _splineId = unit->movespline->GetId();
 }
 
 // 更新单位护送移动的状态
 // 处理速度变化、路径更新、是否到达目标等逻辑
 // 返回值表示是否继续执行该移动行为（false表示已完成）
 template<class T>
 bool EscortMovementGenerator<T>::DoUpdate(T* unit, uint32  /*diff*/)
 {
     if (!unit)
         return false;
 
     // 如果单位处于不可移动状态或施法打断移动状态，则清除移动状态并继续
     if (unit->HasUnitState(UNIT_STATE_NOT_MOVE) || unit->IsMovementPreventedByCasting())
     {
         unit->ClearUnitState(UNIT_STATE_ROAMING_MOVE);
         return true;
     }
 
     unit->AddUnitState(UNIT_STATE_ROAMING_MOVE);
 
     bool arrived = unit->movespline->Finalized();
 
     // 如果需要重新计算速度并且未到达目标点
     if (i_recalculateSpeed && !arrived)
     {
         i_recalculateSpeed = false;
         Movement::MoveSplineInit init(unit);
 
         // 重新计算剩余路径并重新启动移动
         if (m_precomputedPath.size())
         {
             uint32 offset = std::min(uint32(unit->movespline->_currentSplineIdx()), uint32(m_precomputedPath.size()));
             Movement::PointsArray::iterator offsetItr = m_precomputedPath.begin();
             std::advance(offsetItr, offset);
             m_precomputedPath.erase(m_precomputedPath.begin(), offsetItr);
 
             // 恢复起始点为单位当前位置
             m_precomputedPath.insert(m_precomputedPath.begin(), G3D::Vector3(unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ()));
 
             if (m_precomputedPath.size() > 2)
                 init.MovebyPath(m_precomputedPath);
             else if (m_precomputedPath.size() == 2)
                 init.MoveTo(m_precomputedPath[1].x, m_precomputedPath[1].y, m_precomputedPath[1].z, true);
         }
 
         init.Launch();
         // 重新设置Spline ID
         _splineId = unit->movespline->GetId();
     }
 
     return !arrived;
 }
 
 // 当护送移动结束时调用，用于清理状态
 template<class T>
 void EscortMovementGenerator<T>::DoFinalize(T* unit)
 {
     unit->ClearUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
 }
 
 // 重置护送移动生成器，重新开始移动过程
 template<class T>
 void EscortMovementGenerator<T>::DoReset(T* unit)
 {
     if (!unit->IsStopped())
         unit->StopMoving();
 
     unit->AddUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
 }
 
 // 显式实例化模板函数，用于Player和Creature类型
 template void EscortMovementGenerator<Player>::DoInitialize(Player*);
 template void EscortMovementGenerator<Creature>::DoInitialize(Creature*);
 template void EscortMovementGenerator<Player>::DoFinalize(Player*);
 template void EscortMovementGenerator<Creature>::DoFinalize(Creature*);
 template void EscortMovementGenerator<Player>::DoReset(Player*);
 template void EscortMovementGenerator<Creature>::DoReset(Creature*);
 template bool EscortMovementGenerator<Player>::DoUpdate(Player* unit, uint32 diff);
 template bool EscortMovementGenerator<Creature>::DoUpdate(Creature* unit, uint32 diff);
