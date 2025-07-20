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

 #include "TargetedMovementGenerator.h"
 #include "Creature.h"
 #include "CreatureAI.h"
 #include "MoveSplineInit.h"
 #include "Pet.h"
 #include "Player.h"
 #include "Spell.h"
 #include "Transport.h"
 
 // 判断两个单位是否在互相追逐
 static bool IsMutualChase(Unit* owner, Unit* target)
 {
     if (target->GetMotionMaster()->GetCurrentMovementGeneratorType() != CHASE_MOTION_TYPE)
         return false;
 
     return target->GetVictim() == owner;
 }
 
 // 计算追逐范围（考虑单位的战斗半径）
 inline float GetChaseRange(Unit const* owner, Unit const* target)
 {
     float hitboxSum = owner->GetCombatReach() + target->GetCombatReach();
 
     float hoverDelta = owner->GetHoverHeight() - target->GetHoverHeight();
     if (hoverDelta != 0.0f)
         return std::sqrt(std::max(hitboxSum * hitboxSum - hoverDelta * hoverDelta, 0.0f));
 
     return hitboxSum;
 }
 
 // 检查目标位置是否在追逐范围内、角度是否合适、是否在视线范围内
 template<class T>
 bool ChaseMovementGenerator<T>::PositionOkay(T* owner, Unit* target, Optional<float> maxDistance, Optional<ChaseAngle> angle)
 {
     float const distSq = owner->GetExactDistSq(target);
 
     // 如果距离超出最大允许距离
     if (maxDistance && distSq > G3D::square(*maxDistance))
         return false;
 
     // 如果角度不符合要求
     if (angle && !angle->IsAngleOkay(target->GetRelativeAngle(owner)))
         return false;
 
     // 如果不在视线范围内
     if (!owner->IsWithinLOSInMap(target))
         return false;
     return true;
 }
 
 // 更新追逐逻辑
 template<class T>
 bool ChaseMovementGenerator<T>::DoUpdate(T* owner, uint32 time_diff)
 {
     if (!i_target.isValid() || !i_target->IsInWorld() || !owner->IsInMap(i_target.getTarget()))
         return false;
 
     if (!owner || !owner->IsAlive())
         return false;
 
     Creature* cOwner = owner->ToCreature();
 
     // 如果单位无法移动（被定身或施法），或目标丢失，停止移动
     if (owner->HasUnitState(UNIT_STATE_NOT_MOVE) || HasLostTarget(owner) || (cOwner && cOwner->IsMovementPreventedByCasting()))
     {
         owner->StopMoving();
         _lastTargetPosition.reset();
         if (cOwner)
         {
             cOwner->UpdateLeashExtensionTime();
             cOwner->SetCannotReachTarget();
         }
         return true;
     }
 
     // 强制目的地标志
     bool forceDest =
         (i_target->IsPlayer() && i_target->ToPlayer()->IsGameMaster()) || // 用于 .npc follow
         (owner->CanFly())
         ;
 
     Unit* target = i_target.getTarget();
 
     bool mutualChase = IsMutualChase(owner, target);
     bool const mutualTarget = target->GetVictim() == owner;
     float const chaseRange = GetChaseRange(owner, target);
     float const meleeRange = owner->GetMeleeRange(target);
     float const minTarget = (_range ? _range->MinTolerance : 0.0f) + chaseRange;
     float const maxRange = _range ? _range->MaxRange + chaseRange : meleeRange; // melee range already includes hitboxes
     float const maxTarget = _range ? _range->MaxTolerance + chaseRange : CONTACT_DISTANCE + chaseRange;
 
     Optional<ChaseAngle> angle = mutualChase ? Optional<ChaseAngle>() : _angle;
 
     // 防止互相对视时无限旋转
     if (angle && !mutualChase && _mutualChase && mutualTarget && chaseRange < meleeRange)
     {
         angle = Optional<ChaseAngle>();
         mutualChase = true;
     }
 
     // 防止宠物互相对视时无限旋转
     if (angle && !mutualChase && !_mutualChase && mutualTarget && chaseRange < meleeRange && cOwner && cOwner->IsPet())
     {
         angle = Optional<ChaseAngle>();
         mutualChase = true;
     }
 
     // 定期检查是否已到达目标范围
     i_recheckDistance.Update(time_diff);
     if (i_recheckDistance.Passed())
     {
         i_recheckDistance.Reset(400); // Sniffed value
 
         if (i_recalculateTravel && PositionOkay(owner, target, _movingTowards ? maxTarget : Optional<float>(), angle))
         {
             if ((owner->HasUnitState(UNIT_STATE_CHASE_MOVE) && !target->isMoving() && !mutualChase) || _range)
             {
                 i_recalculateTravel = false;
                 i_path = nullptr;
                 if (cOwner)
                     cOwner->SetCannotReachTarget();
                 owner->StopMoving();
                 owner->SetInFront(target);
                 MovementInform(owner);
                 return true;
             }
         }
     }
 
     // 如果移动完成，清理状态
     if (owner->HasUnitState(UNIT_STATE_CHASE_MOVE) && owner->movespline->Finalized())
     {
         i_recalculateTravel = false;
         i_path = nullptr;
         if (cOwner)
             cOwner->SetCannotReachTarget();
         owner->ClearUnitState(UNIT_STATE_CHASE_MOVE);
         owner->SetInFront(target);
         MovementInform(owner);
     }
 
     // 如果在近战范围内，重置拖曳定时器
     if (cOwner)
     {
         if (owner->movespline->Finalized() && cOwner->IsWithinMeleeRange(target))
         {
             i_leashExtensionTimer.Update(time_diff);
             if (i_leashExtensionTimer.Passed())
             {
                 i_leashExtensionTimer.Reset(cOwner->GetAttackTime(BASE_ATTACK));
                 cOwner->UpdateLeashExtensionTime();
             }
         }
         else if (i_recalculateTravel)
             i_leashExtensionTimer.Reset(cOwner->GetAttackTime(BASE_ATTACK));
     }
 
     // 如果目标位置变化，重新计算路径
     if (!_lastTargetPosition || target->GetPosition() != _lastTargetPosition.value() || mutualChase != _mutualChase || !owner->IsWithinLOSInMap(target))
     {
         _lastTargetPosition = target->GetPosition();
         _mutualChase = mutualChase;
         if (owner->HasUnitState(UNIT_STATE_CHASE_MOVE) || !PositionOkay(owner, target, maxTarget, angle))
         {
             // 检查目标是否可到达
             if (cOwner && !target->isInAccessiblePlaceFor(cOwner))
             {
                 cOwner->SetCannotReachTarget(target->GetGUID());
                 cOwner->StopMoving();
                 i_path = nullptr;
                 return true;
             }
 
             // 计算目标位置
             float x, y, z;
             target->GetPosition(x, y, z);
             bool withinRange = owner->IsInDist(target, maxRange);
             bool withinLOS = owner->IsWithinLOS(x, y, z);
             bool moveToward = !(withinRange && withinLOS);
 
             // 创建路径生成器
             if (!i_path || moveToward != _movingTowards)
                 i_path = std::make_unique<PathGenerator>(owner);
             else
                 i_path->Clear();
 
             // 预测目标位置
             float additionalRange = 0;
             bool predictDestination = !mutualChase && target->isMoving();
             if (predictDestination)
             {
                 UnitMoveType moveType = MOVE_RUN;
                 if (target->CanFly())
                     moveType = target->HasUnitMovementFlag(MOVEMENTFLAG_BACKWARD) ? MOVE_FLIGHT_BACK : MOVE_FLIGHT;
                 else
                 {
                     if (target->IsWalking())
                         moveType = MOVE_WALK;
                     else
                         moveType = target->HasUnitMovementFlag(MOVEMENTFLAG_BACKWARD) ? MOVE_RUN_BACK : MOVE_RUN;
                 }
                 float speed = target->GetSpeed(moveType) * 0.5f;
                 additionalRange = owner->GetExactDistSq(target) < G3D::square(speed) ? 0 : speed;
             }
 
             bool shortenPath;
 
             // 如果向目标移动且无角度限制，则缩短路径
             if (moveToward && !angle)
             {
                 shortenPath = true;
             }
             else
             {
                 // 否则计算附近点
                 target->GetNearPoint(owner, x, y, z, (moveToward ? maxTarget : minTarget) - chaseRange - additionalRange, 0, angle ? target->ToAbsoluteAngle(angle->RelativeAngle) : target->GetAngle(owner));
                 shortenPath = false;
             }
 
             if (owner->IsHovering())
                 owner->UpdateAllowedPositionZ(x, y, z);
 
             // 计算路径
             bool success = i_path->CalculatePath(x, y, z, forceDest);
             if (!success || i_path->GetPathType() & PATHFIND_NOPATH)
             {
                 if (cOwner)
                 {
                     cOwner->SetCannotReachTarget(target->GetGUID());
                 }
 
                 owner->StopMoving();
                 return true;
             }
 
             if (shortenPath)
                 i_path->ShortenPathUntilDist(G3D::Vector3(x, y, z), maxTarget);
 
             if (cOwner)
             {
                 cOwner->SetCannotReachTarget();
             }
 
             // 设置移动状态
             bool walk = false;
             if (cOwner && !cOwner->IsPet())
             {
                 switch (cOwner->GetMovementTemplate().GetChase())
                 {
                 case CreatureChaseMovementType::CanWalk:
                     walk = owner->IsWalking();
                     break;
                 case CreatureChaseMovementType::AlwaysWalk:
                     walk = true;
                     break;
                 default:
                     break;
                 }
             }
 
             owner->AddUnitState(UNIT_STATE_CHASE_MOVE);
             i_recalculateTravel = true;
 
             // 初始化移动路径
             Movement::MoveSplineInit init(owner);
             init.MovebyPath(i_path->GetPath());
             init.SetFacing(target);
             init.SetWalk(walk);
             init.Launch();
         }
     }
 
     return true;
 }
 
 //-----------------------------------------------//
 // 玩家初始化追逐行为
 template<>
 void ChaseMovementGenerator<Player>::DoInitialize(Player* owner)
 {
     i_path = nullptr;
     _lastTargetPosition.reset();
     owner->StopMoving();
     owner->AddUnitState(UNIT_STATE_CHASE);
 }
 
 // 生物初始化追逐行为
 template<>
 void ChaseMovementGenerator<Creature>::DoInitialize(Creature* owner)
 {
     i_path = nullptr;
     _lastTargetPosition.reset();
     i_recheckDistance.Reset(0);
     i_leashExtensionTimer.Reset(owner->GetAttackTime(BASE_ATTACK));
     owner->SetWalk(false);
     owner->AddUnitState(UNIT_STATE_CHASE);
 }
 
 // 结束追逐行为
 template<class T>
 void ChaseMovementGenerator<T>::DoFinalize(T* owner)
 {
     owner->ClearUnitState(UNIT_STATE_CHASE | UNIT_STATE_CHASE_MOVE);
     if (Creature* cOwner = owner->ToCreature())
     {
         cOwner->SetCannotReachTarget();
     }
 }
 
 // 重置追逐行为
 template<class T>
 void ChaseMovementGenerator<T>::DoReset(T* owner)
 {
     DoInitialize(owner);
 }
 
 // 移动通知处理
 template<class T>
 void ChaseMovementGenerator<T>::MovementInform(T* owner)
 {
     if (!owner->IsCreature())
         return;
 
     // 调用AI的MovementInform
     if (CreatureAI* AI = owner->ToCreature()->AI())
         AI->MovementInform(CHASE_MOTION_TYPE, i_target.getTarget()->GetGUID().GetCounter());
 }
 
 //-----------------------------------------------//
 
 // 计算跟随速度
 static Optional<float> GetVelocity(Unit* owner, Unit* target, G3D::Vector3 const& dest, bool playerPet)
 {
     Optional<float> speed = {};
     if (!owner->IsInCombat() && !owner->IsVehicle() && !owner->HasUnitFlag(UNIT_FLAG_POSSESSED) &&
         (owner->IsPet() || owner->IsGuardian() || owner->GetGUID() == target->GetCritterGUID() || owner->GetCharmerOrOwnerGUID() == target->GetGUID()))
     {
         uint32 moveFlags = target->GetUnitMovementFlags();
         if (target->movespline->isWalking())
         {
             moveFlags |= MOVEMENTFLAG_WALKING;
         }
 
         UnitMoveType moveType = Movement::SelectSpeedType(moveFlags);
         speed = target->GetSpeed(moveType);
         if (playerPet)
         {
             float distance = owner->GetDistance2d(dest.x, dest.y) - target->GetObjectSize() - (*speed / 2.f);
             if (distance > 0.f)
             {
                 float multiplier = 1.f + (distance / 10.f);
                 *speed *= multiplier;
             }
         }
     }
 
     return speed;
 }
 
 // 预测目标位置
 static Position const PredictPosition(Unit* target)
 {
     Position pos = target->GetPosition();
 
     float speed = target->GetSpeed(Movement::SelectSpeedType(target->GetUnitMovementFlags())) * 0.5f;
     float orientation = target->GetOrientation();
 
     if (target->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FORWARD))
     {
         pos.m_positionX += cos(orientation) * speed;
         pos.m_positionY += std::sin(orientation) * speed;
     }
     else if (target->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_BACKWARD))
     {
         pos.m_positionX -= cos(orientation) * speed;
         pos.m_positionY -= std::sin(orientation) * speed;
     }
 
     if (target->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_STRAFE_LEFT))
     {
         pos.m_positionX += cos(orientation + M_PI / 2.f) * speed;
         pos.m_positionY += std::sin(orientation + M_PI / 2.f) * speed;
     }
     else if (target->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_STRAFE_RIGHT))
     {
         pos.m_positionX += cos(orientation - M_PI / 2.f) * speed;
         pos.m_positionY += std::sin(orientation - M_PI / 2.f) * speed;
     }
 
     return pos;
 }
 
 // 检查目标位置是否合适
 template<class T>
 bool FollowMovementGenerator<T>::PositionOkay(Unit* target, bool isPlayerPet, bool& targetIsMoving, uint32 diff)
 {
     if (!_lastTargetPosition)
         return false;
 
     float exactDistSq = target->GetExactDistSq(_lastTargetPosition->GetPositionX(), _lastTargetPosition->GetPositionY(), _lastTargetPosition->GetPositionZ());
     float distanceTolerance = 0.25f;
     if (target->IsCreature())
     {
         distanceTolerance += _range + _range;
     }
 
     if (isPlayerPet)
     {
         targetIsMoving = target->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT);
     }
 
     if (exactDistSq > distanceTolerance)
         return false;
 
     if (isPlayerPet)
     {
         if (!targetIsMoving)
         {
             if (i_recheckPredictedDistanceTimer.GetExpiry())
             {
                 i_recheckPredictedDistanceTimer.Update(diff);
                 if (i_recheckPredictedDistanceTimer.Passed())
                 {
                     i_recheckPredictedDistanceTimer = 0;
                     return false;
                 }
             }
 
             return true;
         }
 
         return false;
     }
 
     return true;
 }
 
 // 更新跟随逻辑
 template<class T>
 bool FollowMovementGenerator<T>::DoUpdate(T* owner, uint32 time_diff)
 {
     if (!i_target.isValid() || !i_target->IsInWorld() || !owner->IsInMap(i_target.getTarget()))
         return false;
 
     if (!owner || !owner->IsAlive())
         return false;
 
     Creature* cOwner = owner->ToCreature();
     Unit* target = i_target.getTarget();
 
     // 如果无法移动或施法中，停止跟随
     if (owner->HasUnitState(UNIT_STATE_NOT_MOVE) || (cOwner && owner->ToCreature()->IsMovementPreventedByCasting()))
     {
         i_path = nullptr;
         owner->StopMoving();
         _lastTargetPosition.reset();
         return true;
     }
 
     // 判断是否是跟随主人的宠物
     bool followingMaster = false;
     Pet* oPet = owner->ToPet();
     if (oPet)
     {
         if (target->GetGUID() == oPet->GetOwnerGUID())
             followingMaster = true;
     }
 
     bool forceDest =
         (followingMaster) || // 宠物可以作弊生成路径
         (i_target->IsPlayer() && i_target->ToPlayer()->IsGameMaster()) // .npc follow
         ;
 
     bool targetIsMoving = false;
     if (PositionOkay(target, owner->IsGuardian() && target->IsPlayer(), targetIsMoving, time_diff))
     {
         if (owner->HasUnitState(UNIT_STATE_FOLLOW_MOVE) && owner->movespline->Finalized())
         {
             owner->ClearUnitState(UNIT_STATE_FOLLOW_MOVE);
             i_path = nullptr;
             MovementInform(owner);
 
             if (i_recheckPredictedDistance)
             {
                 i_recheckPredictedDistanceTimer.Reset(1000);
             }
 
             owner->SetFacingTo(target->GetOrientation());
         }
     }
     else
     {
         Position targetPosition = target->GetPosition();
         _lastTargetPosition = targetPosition;
 
         // 如果目标移动，预测位置
         if (targetIsMoving)
         {
             Position predictedPosition = PredictPosition(target);
             if (_lastPredictedPosition && _lastPredictedPosition->GetExactDistSq(&predictedPosition) < 0.25f)
                 return true;
 
             _lastPredictedPosition = predictedPosition;
             targetPosition = predictedPosition;
             i_recheckPredictedDistance = true;
         }
         else
         {
             i_recheckPredictedDistance = false;
             i_recheckPredictedDistanceTimer.Reset(0);
         }
 
         // 创建路径生成器
         if (!i_path)
             i_path = std::make_unique<PathGenerator>(owner);
         else
             i_path->Clear();
 
         // 计算目标点
         target->MovePositionToFirstCollision(targetPosition, owner->GetCombatReach() + _range, target->ToAbsoluteAngle(_angle.RelativeAngle) - target->GetOrientation());
 
         float x, y, z;
         targetPosition.GetPosition(x, y, z);
 
         if (owner->IsHovering())
             owner->UpdateAllowedPositionZ(x, y, z);
 
         // 计算路径
         bool success = i_path->CalculatePath(x, y, z, forceDest);
         if (!success || (i_path->GetPathType() & PATHFIND_NOPATH && !followingMaster))
         {
             if (!owner->IsStopped())
                 owner->StopMoving();
 
             return true;
         }
 
         owner->AddUnitState(UNIT_STATE_FOLLOW_MOVE);
 
         // 初始化移动路径
         Movement::MoveSplineInit init(owner);
         init.MovebyPath(i_path->GetPath());
         if (_inheritWalkState)
             init.SetWalk(target->IsWalking() || target->movespline->isWalking());
 
         if (_inheritSpeed)
             if (Optional<float> velocity = GetVelocity(owner, target, i_path->GetActualEndPosition(), owner->IsGuardian()))
                 init.SetVelocity(*velocity);
         init.Launch();
     }
 
     return true;
 }
 
 // 初始化跟随行为
 template<class T>
 void FollowMovementGenerator<T>::DoInitialize(T* owner)
 {
     i_path = nullptr;
     _lastTargetPosition.reset();
     owner->AddUnitState(UNIT_STATE_FOLLOW);
 }
 
 // 结束跟随行为
 template<class T>
 void FollowMovementGenerator<T>::DoFinalize(T* owner)
 {
     owner->ClearUnitState(UNIT_STATE_FOLLOW | UNIT_STATE_FOLLOW_MOVE);
 }
 
 // 重置跟随行为
 template<class T>
 void FollowMovementGenerator<T>::DoReset(T* owner)
 {
     DoInitialize(owner);
 }
 
 // 移动通知处理
 template<class T>
 void FollowMovementGenerator<T>::MovementInform(T* owner)
 {
     if (!owner->IsCreature())
         return;
 
     // 调用AI的MovementInform
     if (CreatureAI* AI = owner->ToCreature()->AI())
         AI->MovementInform(FOLLOW_MOTION_TYPE, i_target.getTarget()->GetGUID().GetCounter());
 }
 
 //-----------------------------------------------//
 
 // 显式模板实例化
 template void ChaseMovementGenerator<Player>::DoFinalize(Player*);
 template void ChaseMovementGenerator<Creature>::DoFinalize(Creature*);
 template void ChaseMovementGenerator<Player>::DoReset(Player*);
 template void ChaseMovementGenerator<Creature>::DoReset(Creature*);
 template bool ChaseMovementGenerator<Player>::DoUpdate(Player*, uint32);
 template bool ChaseMovementGenerator<Creature>::DoUpdate(Creature*, uint32);
 template void ChaseMovementGenerator<Unit>::MovementInform(Unit*);
 
 template void FollowMovementGenerator<Player>::DoInitialize(Player*);
 template void FollowMovementGenerator<Creature>::DoInitialize(Creature*);
 template void FollowMovementGenerator<Player>::DoFinalize(Player*);
 template void FollowMovementGenerator<Creature>::DoFinalize(Creature*);
 template void FollowMovementGenerator<Player>::DoReset(Player*);
 template void FollowMovementGenerator<Creature>::DoReset(Creature*);
 template bool FollowMovementGenerator<Player>::DoUpdate(Player*, uint32);
 template bool FollowMovementGenerator<Creature>::DoUpdate(Creature*, uint32);
