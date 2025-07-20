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

 #include "MoveSplineInit.h"
 #include "MoveSpline.h"
 #include "MovementPacketBuilder.h"
 #include "Opcodes.h"
 #include "Transport.h"
 #include "Unit.h"
 #include "Vehicle.h"
 #include "WorldPacket.h"
 
 namespace Movement
 {
     /**
      * 根据移动标志选择移动类型（速度类型）
      * @param moveFlags 移动标志
      * @return 返回对应的 UnitMoveType
      */
     UnitMoveType SelectSpeedType(uint32 moveFlags)
     {
         if (moveFlags & MOVEMENTFLAG_FLYING)
         {
             if (moveFlags & MOVEMENTFLAG_BACKWARD /*&& speed_obj.flight >= speed_obj.flight_back*/)
                 return MOVE_FLIGHT_BACK;
             else
                 return MOVE_FLIGHT;
         }
         else if (moveFlags & MOVEMENTFLAG_SWIMMING)
         {
             if (moveFlags & MOVEMENTFLAG_BACKWARD /*&& speed_obj.swim >= speed_obj.swim_back*/)
                 return MOVE_SWIM_BACK;
             else
                 return MOVE_SWIM;
         }
         else if (moveFlags & MOVEMENTFLAG_WALKING)
         {
             //if (speed_obj.run > speed_obj.walk)
             return MOVE_WALK;
         }
         else if (moveFlags & MOVEMENTFLAG_BACKWARD /*&& speed_obj.run >= speed_obj.run_back*/)
             return MOVE_RUN_BACK;
 
         // 飞行生物使用 MOVEMENTFLAG_CAN_FLY 或 MOVEMENTFLAG_DISABLE_GRAVITY
         // 跑步速度是它们的默认飞行速度。
         return MOVE_RUN;
     }
 
     /**
      * 启动移动样条动画
      * @return 返回移动的持续时间
      */
     int32 MoveSplineInit::Launch()
     {
         MoveSpline& move_spline = *unit->movespline;
 
         bool transport = unit->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT) && unit->GetTransGUID();
         Location real_position;
         // 如果当前状态未最终确定，有很大可能当前位置未知，需要计算它
         // 这也允许在更长的时间间隔内计算路径和更新地图位置
         if (!move_spline.Finalized() && move_spline.onTransport == transport)
             real_position = move_spline.ComputePosition();
         else
         {
             Position const* pos;
             if (!transport)
                 pos = unit;
             else
                 pos = &unit->m_movementInfo.transport.pos;
 
             real_position.x = pos->GetPositionX();
             real_position.y = pos->GetPositionY();
             real_position.z = pos->GetPositionZ();
             real_position.orientation = unit->GetOrientation();
         }
 
         // 如果没有路径点，则直接返回
         if (args.path.empty())
             return 0;
 
         // 校正第一个路径点
         args.path[0] = real_position;
         args.initialOrientation = real_position.orientation;
         move_spline.onTransport = transport;
 
         uint32 moveFlags = unit->m_movementInfo.GetMovementFlags();
         moveFlags |= MOVEMENTFLAG_SPLINE_ENABLED;
 
         // 设置移动方向标志
         if (!args.flags.orientationInversed)
         {
             moveFlags = (moveFlags & ~(MOVEMENTFLAG_BACKWARD)) | MOVEMENTFLAG_FORWARD;
         }
         else
         {
             moveFlags = (moveFlags & ~(MOVEMENTFLAG_FORWARD)) | MOVEMENTFLAG_BACKWARD;
         }
 
         // 检查是否仅改变朝向
         bool isOrientationOnly = args.path.size() == 2 && args.path[0] == args.path[1];
 
         // 如果被定身或仅改变朝向，则清除移动标志
         if ((moveFlags & MOVEMENTFLAG_ROOT) || isOrientationOnly)
             moveFlags &= ~MOVEMENTFLAG_MASK_MOVING;
 
         // 如果没有设置速度，则根据移动类型计算速度
         if (!args.HasVelocity)
         {
             // 如果 spline 是通过 SetWalk 初始化的，那只是意味着我们需要为它选择步行速度
             // 但不需要添加步行标志给单位
             uint32 moveFlagsForSpeed = moveFlags;
             if (args.flags.walkmode)
                 moveFlagsForSpeed |= MOVEMENTFLAG_WALKING;
             else
                 moveFlagsForSpeed &= ~MOVEMENTFLAG_WALKING;
 
             args.velocity = unit->GetSpeed(SelectSpeedType(moveFlagsForSpeed));
         }
 
         // 限制速度，模拟客户端行为
         args.velocity = std::min(args.velocity, args.flags.catmullrom || args.flags.flying ? 50.0f : std::max(28.0f, unit->GetSpeed(MOVE_RUN) * 4.0f));
 
         // 验证参数
         if (!args.Validate(unit))
             return 0;
 
         // 设置移动标志
         unit->m_movementInfo.SetMovementFlags(moveFlags);
         move_spline.Initialize(args);
 
         // 构造移动数据包
         WorldPacket data(SMSG_MONSTER_MOVE, 64);
         data << unit->GetPackGUID();
         if (transport)
         {
             data.SetOpcode(SMSG_MONSTER_MOVE_TRANSPORT);
             data << unit->GetTransGUID().WriteAsPacked();
             data << int8(unit->GetTransSeat());
         }
 
         // 写入怪物移动数据
         PacketBuilder::WriteMonsterMove(move_spline, data);
         unit->SendMessageToSet(&data, true);
 
         return move_spline.Duration();
     }
 
     /**
      * 停止移动样条动画
      */
     void MoveSplineInit::Stop()
     {
         MoveSpline& move_spline = *unit->movespline;
 
         // 如果已经停止，则无需处理
         if (move_spline.Finalized())
             return;
 
         bool transport = unit->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT) && unit->GetTransGUID();
         Location loc;
         if (move_spline.onTransport == transport)
             loc = move_spline.ComputePosition();
         else
         {
             Position const* pos;
             if (!transport)
                 pos = unit;
             else
                 pos = &unit->m_movementInfo.transport.pos;
 
             loc.x = pos->GetPositionX();
             loc.y = pos->GetPositionY();
             loc.z = pos->GetPositionZ();
             loc.orientation = unit->GetOrientation();
         }
 
         // 设置停止标志
         args.flags = MoveSplineFlag::Done;
         unit->m_movementInfo.RemoveMovementFlag(MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_SPLINE_ENABLED);
         move_spline.onTransport = transport;
         move_spline.Initialize(args);
 
         // 构造停止移动数据包
         WorldPacket data(SMSG_MONSTER_MOVE, 64);
         data << unit->GetPackGUID();
         if (transport)
         {
             data.SetOpcode(SMSG_MONSTER_MOVE_TRANSPORT);
             data << unit->GetTransGUID().WriteAsPacked();
             data << int8(unit->GetTransSeat());
         }
 
         // 写入停止移动数据
         PacketBuilder::WriteStopMovement(loc, args.splineId, data);
         unit->SendMessageToSet(&data, true);
     }
 
     /**
      * MoveSplineInit 构造函数
      * @param m 指向 Unit 的指针
      */
     MoveSplineInit::MoveSplineInit(Unit* m) : unit(m)
     {
         args.splineId = splineIdGen.NewId();
         args.TransformForTransport = unit->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT) && unit->GetTransGUID();
         // 混合现有状态到新初始化的参数中
         args.flags.walkmode = unit->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_WALKING);
         args.flags.flying = unit->m_movementInfo.HasMovementFlag((MovementFlags)(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_DISABLE_GRAVITY));
     }
 
     /**
      * 设置朝向目标单位
      * @param target 指向目标单位的指针
      */
     void MoveSplineInit::SetFacing(Unit const* target)
     {
         args.flags.EnableFacingTarget();
         args.facing.target = target->GetGUID().GetRawValue();
     }
 
     /**
      * 设置朝向角度
      * @param angle 角度值
      */
     void MoveSplineInit::SetFacing(float angle)
     {
         if (args.TransformForTransport)
         {
             if (Unit* vehicle = unit->GetVehicleBase())
                 angle -= vehicle->GetOrientation();
             else if (Transport* transport = unit->GetTransport())
                 angle -= transport->GetOrientation();
         }
 
         args.facing.angle = G3D::wrap(angle, 0.f, (float)G3D::twoPi());
         args.flags.EnableFacingAngle();
     }
 
     /**
      * 移动到指定位置
      * @param dest 目标位置
      * @param generatePath 是否生成路径
      * @param forceDestination 是否强制到达目标点
      */
     void MoveSplineInit::MoveTo(const Vector3& dest, bool generatePath, bool forceDestination)
     {
         if (generatePath)
         {
             PathGenerator path(unit);
             bool result = path.CalculatePath(dest.x, dest.y, dest.z, forceDestination);
             if (result && !(path.GetPathType() & PATHFIND_NOPATH))
             {
                 MovebyPath(path.GetPath());
                 return;
             }
         }
 
         args.path_Idx_offset = 0;
         args.path.resize(2);
         TransportPathTransform transform(unit, args.TransformForTransport);
         args.path[1] = transform(dest);
     }
 
     /**
      * 用于处理运输路径的坐标转换
      * @param input 输入坐标
      * @return 转换后的坐标
      */
     Vector3 TransportPathTransform::operator()(Vector3 input)
     {
         if (_transformForTransport)
             if (TransportBase* transport = _owner->GetDirectTransport())
                 transport->CalculatePassengerOffset(input.x, input.y, input.z);
 
         return input;
     }
 }
