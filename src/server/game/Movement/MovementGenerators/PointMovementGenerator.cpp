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

 #include "PointMovementGenerator.h"
 #include "Creature.h"
 #include "CreatureAI.h"
 #include "MoveSpline.h"
 #include "MoveSplineInit.h"
 #include "ObjectAccessor.h"
 #include "Player.h"
 #include "World.h"
 
 //----- Point Movement Generator
 // 初始化单位移动，设置移动路径或目标点
 template<class T>
 void PointMovementGenerator<T>::DoInitialize(T* unit)
 {
     // 如果单位处于不能移动状态或被施法阻止移动，则标记需要重新计算速度并返回
     if (unit->HasUnitState(UNIT_STATE_NOT_MOVE) || unit->IsMovementPreventedByCasting())
     {
         i_recalculateSpeed = true;
         return;
     }
 
     // 如果单位未停止，则停止移动
     if (!unit->IsStopped())
         unit->StopMoving();
 
     // 添加单位状态：漫游和漫游移动
     unit->AddUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
 
     // 如果是冲锋事件，添加冲锋状态
     if (id == EVENT_CHARGE || id == EVENT_CHARGE_PREPATH)
     {
         unit->AddUnitState(UNIT_STATE_CHARGING);
     }
 
     i_recalculateSpeed = false;
 
     Movement::MoveSplineInit init(unit);
 
     // 如果预计算路径存在且长度大于2，使用该路径移动
     if (m_precomputedPath.size() > 2)
         init.MovebyPath(m_precomputedPath);
     // 否则如果需要生成路径，尝试计算路径
     else if (_generatePath)
     {
         PathGenerator path(unit);
         bool result = path.CalculatePath(i_x, i_y, i_z, _forceDestination);
         if (result && !(path.GetPathType() & PATHFIND_NOPATH) && path.GetPath().size() > 2)
         {
             m_precomputedPath = path.GetPath();
             init.MovebyPath(m_precomputedPath);
         }
         else
         {
             // 修复客户端视觉bug：仅在Z轴移动时客户端方向反转问题
             if (G3D::fuzzyEq(unit->GetPositionX(), i_x) && G3D::fuzzyEq(unit->GetPositionY(), i_y))
             {
                 i_x += 0.2f * cos(unit->GetOrientation());
                 i_y += 0.2f * std::sin(unit->GetOrientation());
             }
 
             init.MoveTo(i_x, i_y, i_z, true);
         }
     }
     else
     {
         // 同上修复客户端视觉bug
         if (G3D::fuzzyEq(unit->GetPositionX(), i_x) && G3D::fuzzyEq(unit->GetPositionY(), i_y))
         {
             i_x += 0.2f * cos(unit->GetOrientation());
             i_y += 0.2f * std::sin(unit->GetOrientation());
         }
 
         init.MoveTo(i_x, i_y, i_z, true);
     }
 
     // 设置速度
     if (speed > 0.0f)
         init.SetVelocity(speed);
 
     // 设置朝向
     if (i_orientation > 0.0f)
     {
         init.SetFacing(i_orientation);
     }
 
     // 启动移动
     init.Launch();
 }
 
 // 更新单位移动状态，返回是否继续移动
 template<class T>
 bool PointMovementGenerator<T>::DoUpdate(T* unit, uint32 /*diff*/)
 {
     if (!unit)
         return false;
 
     // 如果单位被施法阻止移动，停止移动并返回true
     if (unit->IsMovementPreventedByCasting())
     {
         unit->StopMoving();
         return true;
     }
 
     // 如果单位处于不能移动状态，停止移动（除非正在冲锋）
     if (unit->HasUnitState(UNIT_STATE_NOT_MOVE))
     {
         if (!unit->HasUnitState(UNIT_STATE_CHARGING))
         {
             unit->StopMoving();
         }
 
         return true;
     }
 
     // 添加漫游移动状态
     unit->AddUnitState(UNIT_STATE_ROAMING_MOVE);
 
     // 如果需要重新计算速度且当前路径未完成
     if (id != EVENT_CHARGE_PREPATH && i_recalculateSpeed && !unit->movespline->Finalized())
     {
         i_recalculateSpeed = false;
         Movement::MoveSplineInit init(unit);
 
         // 如果存在预计算路径，重新计算剩余路径
         if (m_precomputedPath.size())
         {
             uint32 offset = std::min(uint32(unit->movespline->_currentSplineIdx()), uint32(m_precomputedPath.size()));
             Movement::PointsArray::iterator offsetItr = m_precomputedPath.begin();
             std::advance(offsetItr, offset);
             m_precomputedPath.erase(m_precomputedPath.begin(), offsetItr);
 
             // 恢复当前位置作为第一个路径点
             m_precomputedPath.insert(m_precomputedPath.begin(), G3D::Vector3(unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ()));
 
             // 根据路径长度设置移动方式
             if (m_precomputedPath.size() > 2)
                 init.MovebyPath(m_precomputedPath);
             else if (m_precomputedPath.size() == 2)
                 init.MoveTo(m_precomputedPath[1].x, m_precomputedPath[1].y, m_precomputedPath[1].z, true);
         }
         else
             init.MoveTo(i_x, i_y, i_z, true);
 
         // 设置速度
         if (speed > 0.0f)
             init.SetVelocity(speed);
 
         // 设置朝向
         if (i_orientation > 0.0f)
         {
             init.SetFacing(i_orientation);
         }
 
         // 启动移动
         init.Launch();
     }
 
     // 返回是否仍在移动
     return !unit->movespline->Finalized();
 }
 
 // 移动生成完成时调用，清理状态并通知AI
 template<class T>
 void PointMovementGenerator<T>::DoFinalize(T* unit)
 {
     // 清除漫游和漫游移动状态
     unit->ClearUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
 
     // 如果是冲锋事件，清除冲锋状态
     if (id == EVENT_CHARGE || id == EVENT_CHARGE_PREPATH)
     {
         unit->ClearUnitState(UNIT_STATE_CHARGING);
 
         // 如果有冲锋目标且目标是当前目标，攻击目标
         if (_chargeTargetGUID && _chargeTargetGUID == unit->GetTarget())
         {
             if (Unit* target = ObjectAccessor::GetUnit(*unit, _chargeTargetGUID))
             {
                 unit->Attack(target, true);
             }
         }
     }
 
     // 如果路径已完成，触发移动通知
     if (unit->movespline->Finalized())
         MovementInform(unit);
 }
 
 // 重置移动状态
 template<class T>
 void PointMovementGenerator<T>::DoReset(T* unit)
 {
     // 如果未停止，停止移动
     if (!unit->IsStopped())
         unit->StopMoving();
 
     // 添加漫游和漫游移动状态
     unit->AddUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
 
     // 如果是冲锋事件，添加冲锋状态
     if (id == EVENT_CHARGE || id == EVENT_CHARGE_PREPATH)
     {
         unit->AddUnitState(UNIT_STATE_CHARGING);
     }
 }
 
 // 移动通知，空实现
 template<class T>
 void PointMovementGenerator<T>::MovementInform(T* /*unit*/)
 {
 }
 
 // 针对Creature的移动通知实现
 template <> void PointMovementGenerator<Creature>::MovementInform(Creature* unit)
 {
     // 调用AI的MovementInform
     if (unit->AI())
         unit->AI()->MovementInform(POINT_MOTION_TYPE, id);
 
     // 如果有召唤者，调用召唤者的AI的SummonMovementInform
     if (Unit* summoner = unit->GetCharmerOrOwner())
     {
         if (UnitAI* AI = summoner->GetAI())
         {
             AI->SummonMovementInform(unit, POINT_MOTION_TYPE, id);
         }
     }
 }
 
 // 显式实例化模板函数
 template void PointMovementGenerator<Player>::DoInitialize(Player*);
 template void PointMovementGenerator<Creature>::DoInitialize(Creature*);
 template void PointMovementGenerator<Player>::DoFinalize(Player*);
 template void PointMovementGenerator<Creature>::DoFinalize(Creature*);
 template void PointMovementGenerator<Player>::DoReset(Player*);
 template void PointMovementGenerator<Creature>::DoReset(Creature*);
 template bool PointMovementGenerator<Player>::DoUpdate(Player*, uint32);
 template bool PointMovementGenerator<Creature>::DoUpdate(Creature*, uint32);
 
 // 助攻移动生成器完成时调用
 void AssistanceMovementGenerator::Finalize(Unit* unit)
 {
     // 设置不调用援助并触发援助调用
     unit->ToCreature()->SetNoCallAssistance(false);
     unit->ToCreature()->CallAssistance();
 
     // 如果单位存活，移动以分散注意力
     if (unit->IsAlive())
         unit->GetMotionMaster()->MoveSeekAssistanceDistract(sWorld->getIntConfig(CONFIG_CREATURE_FAMILY_ASSISTANCE_DELAY));
 }
 
 // 效果移动生成器更新函数，返回是否仍在移动
 bool EffectMovementGenerator::Update(Unit* unit, uint32)
 {
     return !unit->movespline->Finalized();
 }
 
 // 效果移动生成器完成时调用
 void EffectMovementGenerator::Finalize(Unit* unit)
 {
     // 如果不是生物，返回
     if (!unit->IsCreature())
         return;
 
     // 如果正在下落，移除下落标志
     if (unit->IsCreature() && unit->HasUnitMovementFlag(MOVEMENTFLAG_FALLING) && unit->movespline->isFalling())
         unit->RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING);
 
     // 调用AI的MovementInform
     if (unit->ToCreature()->AI())
         unit->ToCreature()->AI()->MovementInform(EFFECT_MOTION_TYPE, m_Id);
 }
