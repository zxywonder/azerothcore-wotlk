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

 #ifndef ACORE_MOTIONMASTER_H
 #define ACORE_MOTIONMASTER_H
 
 #include "Common.h"
 #include "ObjectGuid.h"
 #include "PathGenerator.h"
 #include "Position.h"
 #include "SharedDefines.h"
 #include "Spline/MoveSpline.h"
 #include <optional>
 #include <vector>
 
 class MovementGenerator;
 class Unit;
 
 // 用于显示路径点的生物Entry ID，仅GM可见
 #define VISUAL_WAYPOINT 1
 
 // 值 0 ... MAX_DB_MOTION_TYPE-1 用于数据库
 enum MovementGeneratorType
 {
     IDLE_MOTION_TYPE      = 0,                              // IdleMovementGenerator.h
     RANDOM_MOTION_TYPE    = 1,                              // RandomMovementGenerator.h
     WAYPOINT_MOTION_TYPE  = 2,                              // WaypointMovementGenerator.h
     MAX_DB_MOTION_TYPE    = 3,                              // *** 以下的运动类型不能在数据库中设置。
     ANIMAL_RANDOM_MOTION_TYPE = MAX_DB_MOTION_TYPE,         // AnimalRandomMovementGenerator.h
     CONFUSED_MOTION_TYPE  = 4,                              // ConfusedMovementGenerator.h
     CHASE_MOTION_TYPE     = 5,                              // TargetedMovementGenerator.h
     HOME_MOTION_TYPE      = 6,                              // HomeMovementGenerator.h
     FLIGHT_MOTION_TYPE    = 7,                              // WaypointMovementGenerator.h
     POINT_MOTION_TYPE     = 8,                              // PointMovementGenerator.h
     FLEEING_MOTION_TYPE   = 9,                              // FleeingMovementGenerator.h
     DISTRACT_MOTION_TYPE  = 10,                             // IdleMovementGenerator.h
     ASSISTANCE_MOTION_TYPE = 11,                            // PointMovementGenerator.h (逃离协助的第一部分)
     ASSISTANCE_DISTRACT_MOTION_TYPE = 12,                   // IdleMovementGenerator.h (逃离协助的第二部分)
     TIMED_FLEEING_MOTION_TYPE = 13,                         // FleeingMovementGenerator.h (逃离协助的另一种第二部分)
     FOLLOW_MOTION_TYPE    = 14,
     ROTATE_MOTION_TYPE    = 15,
     EFFECT_MOTION_TYPE    = 16,
     ESCORT_MOTION_TYPE    = 17,                             // xinef: EscortMovementGenerator.h
     NULL_MOTION_TYPE      = 18
 };
 
 // 运动槽位枚举
 enum MovementSlot
 {
     MOTION_SLOT_IDLE,       // 空闲槽位
     MOTION_SLOT_ACTIVE,     // 活动槽位
     MOTION_SLOT_CONTROLLED, // 受控槽位
     MAX_MOTION_SLOT         // 槽位最大数量
 };
 
 // 清理标志枚举
 enum MMCleanFlag
 {
     MMCF_NONE   = 0x00, // 无清理标志
     MMCF_UPDATE = 0x01, // 从更新中调用清除或过期
     MMCF_RESET  = 0x02, // 标志是否需要 top()->Reset()
     MMCF_INUSE  = 0x04, // 标志是否在 MotionMaster::UpdateMotion 中使用
 };
 
 // 旋转方向枚举
 enum RotateDirection
 {
     ROTATE_DIRECTION_LEFT,  // 左转
     ROTATE_DIRECTION_RIGHT  // 右转
 };
 
 // 追击范围结构体
 struct ChaseRange
 {
     ChaseRange(float range); // 使用单一范围构造
     ChaseRange(float _minRange, float _maxRange); // 使用最小和最大范围构造
     ChaseRange(float _minRange, float _minTolerance, float _maxTolerance, float _maxRange); // 使用详细范围构造
 
     float MinRange;     // 如果在此范围内必须移动（最小攻击范围）
     float MinTolerance; // 如果在范围内，将移动此距离
     float MaxRange;     // 如果在此范围外必须移动（最大攻击范围）
     float MaxTolerance; // 如果在范围外，将移动到此范围内
 };
 
 // 追击角度结构体
 struct ChaseAngle
 {
     ChaseAngle(float angle, float _tolerance = M_PI_4); // 构造函数
 
     float RelativeAngle; // 我们希望相对于目标的角度（0 = 前方，M_PI = 后方）
     float Tolerance;     // 容忍的角度范围
 
     [[nodiscard]] float UpperBound() const; // 获取上限角度
     [[nodiscard]] float LowerBound() const; // 获取下限角度
     [[nodiscard]] bool IsAngleOkay(float relativeAngle) const; // 判断角度是否合适
 };
 
 // 假设冲锋速度为每0.6秒25码
 #define SPEED_CHARGE    42.0f
 
 // 运动控制器类
 class MotionMaster //: private std::stack<MovementGenerator *>
 {
 private:
     typedef MovementGenerator* _Ty;
 
     void pop()
     {
         if (empty())
             return;
 
         Impl[_top] = nullptr;
         while (!empty() && !top())
             --_top;
     }
 
     [[nodiscard]] bool needInitTop() const
     {
         if (empty())
             return false;
         return _needInit[_top];
     }
     void InitTop();
 public:
     explicit MotionMaster(Unit* unit) : _expList(nullptr), _top(-1), _owner(unit), _cleanFlag(MMCF_NONE)
     {
         for (uint8 i = 0; i < MAX_MOTION_SLOT; ++i)
         {
             Impl[i] = nullptr;
             _needInit[i] = true;
         }
     }
     ~MotionMaster();
 
     void Initialize(); // 初始化所有运动
     void InitDefault(); // 初始化默认运动
 
     [[nodiscard]] bool empty() const { return (_top < 0); } // 判断是否为空
     [[nodiscard]] int size() const { return _top + 1; } // 获取运动数量
     [[nodiscard]] _Ty top() const
     {
         ASSERT(!empty());
         return Impl[_top];
     }
     [[nodiscard]] _Ty GetMotionSlot(int slot) const
     {
         ASSERT(slot >= 0);
         return Impl[slot];
     }
 
     [[nodiscard]] uint8 GetCleanFlags() const { return _cleanFlag; } // 获取清理标志
 
     void DirectDelete(_Ty curr); // 直接删除运动生成器
     void DelayedDelete(_Ty curr); // 延迟删除运动生成器
 
     void UpdateMotion(uint32 diff); // 更新运动状态
     void Clear(bool reset = true) // 清除所有运动
     {
         if (_cleanFlag & MMCF_UPDATE)
         {
             if (reset)
                 _cleanFlag |= MMCF_RESET;
             else
                 _cleanFlag &= ~MMCF_RESET;
             DelayedClean();
         }
         else
             DirectClean(reset);
     }
     void MovementExpired(bool reset = true) // 运动过期处理
     {
         if (_cleanFlag & MMCF_UPDATE)
         {
             if (reset)
                 _cleanFlag |= MMCF_RESET;
             else
                 _cleanFlag &= ~MMCF_RESET;
             DelayedExpire();
         }
         else
             DirectExpire(reset);
     }
 
     void MovementExpiredOnSlot(MovementSlot slot, bool reset = true) // 在特定槽位运动过期处理
     {
         if (!(_cleanFlag & MMCF_UPDATE))
             DirectExpireSlot(slot, reset);
     }
 
     void MoveIdle(); // 设置为空闲运动
     void MoveTargetedHome(bool walk = false); // 移动到目标家的位置
     void MoveRandom(float wanderDistance = 0.0f); // 随机移动
     void MoveFollow(Unit* target, float dist, float angle, MovementSlot slot = MOTION_SLOT_ACTIVE, bool inheritWalkState = true, bool inheritSpeed = true); // 跟随目标
     void MoveChase(Unit* target, std::optional<ChaseRange> dist = {}, std::optional<ChaseAngle> angle = {}); // 追击目标
     void MoveChase(Unit* target, float dist, float angle) { MoveChase(target, ChaseRange(dist), ChaseAngle(angle)); } // 重载：追击目标
     void MoveChase(Unit* target, float dist) { MoveChase(target, ChaseRange(dist)); } // 重载：追击目标
     void MoveCircleTarget(Unit* target); // 绕目标移动
     void MoveBackwards(Unit* target, float dist); // 向后移动
     void MoveForwards(Unit* target, float dist); // 向前移动
     void MoveConfused(); // 混乱移动
     void MoveFleeing(Unit* enemy, uint32 time = 0); // 逃跑移动
     void MovePoint(uint32 id, const Position& pos, bool generatePath = true, bool forceDestination = true)
     { MovePoint(id, pos.m_positionX, pos.m_positionY, pos.m_positionZ, generatePath, forceDestination, MOTION_SLOT_ACTIVE, pos.GetOrientation()); } // 移动到指定点
     void MovePoint(uint32 id, float x, float y, float z, bool generatePath = true, bool forceDestination = true, MovementSlot slot = MOTION_SLOT_ACTIVE, float orientation = 0.0f); // 移动到指定点
     void MoveSplinePath(Movement::PointsArray* path); // 移动样条路径
     void MoveSplinePath(uint32 path_id); // 移动样条路径
 
     // 以下两种移动类型仅用于具有着陆/起飞动画的生物
     void MoveLand(uint32 id, Position const& pos, float speed = 0.0f); // 着陆移动
     void MoveLand(uint32 id, float x, float y, float z, float speed = 0.0f); // 着陆移动
     void MoveTakeoff(uint32 id, Position const& pos, float speed = 0.0f, bool skipAnimation = false); // 起飞移动
     void MoveTakeoff(uint32 id, float x, float y, float z, float speed = 0.0f, bool skipAnimation = false); // 起飞移动
 
     void MoveCharge(float x, float y, float z, float speed = SPEED_CHARGE, uint32 id = EVENT_CHARGE, const Movement::PointsArray* path = nullptr, bool generatePath = false, float orientation = 0.0f, ObjectGuid targetGUID = ObjectGuid::Empty); // 冲锋移动
     void MoveCharge(PathGenerator const& path, float speed = SPEED_CHARGE, ObjectGuid targetGUID = ObjectGuid::Empty); // 冲锋移动
     void MoveKnockbackFrom(float srcX, float srcY, float speedXY, float speedZ); // 击退移动
     void MoveJumpTo(float angle, float speedXY, float speedZ); // 跳跃移动
     void MoveJump(Position const& pos, float speedXY, float speedZ, uint32 id = 0)
     { MoveJump(pos.m_positionX, pos.m_positionY, pos.m_positionZ, speedXY, speedZ, id); }; // 跳跃到指定位置
     void MoveJump(float x, float y, float z, float speedXY, float speedZ, uint32 id = 0, Unit const* target = nullptr); // 跳跃到指定位置
     void MoveFall(uint32 id = 0, bool addFlagForNPC = false); // 下落移动
 
     void MoveSeekAssistance(float x, float y, float z); // 寻求帮助移动
     void MoveSeekAssistanceDistract(uint32 timer); // 寻求帮助后分心
     void MoveTaxiFlight(uint32 path, uint32 pathnode); // 飞行移动
     void MoveDistract(uint32 time); // 分心移动
     void MovePath(uint32 path_id, bool repeatable); // 沿路径移动
     void MoveRotate(uint32 time, RotateDirection direction); // 旋转移动
 
     [[nodiscard]] MovementGeneratorType GetCurrentMovementGeneratorType() const; // 获取当前运动生成器类型
     [[nodiscard]] MovementGeneratorType GetMotionSlotType(int slot) const; // 获取指定槽位的运动生成器类型
     bool HasMovementGeneratorType(MovementGeneratorType type) const; // 判断是否包含指定类型的运动生成器
     [[nodiscard]] uint32 GetCurrentSplineId() const; // 获取当前样条ID
 
     void propagateSpeedChange(); // 传播速度变化
     void ReinitializeMovement(); // 重新初始化运动
 
     bool GetDestination(float& x, float& y, float& z); // 获取目的地坐标
 private:
     void Mutate(MovementGenerator* m, MovementSlot slot);                  // 使用Move*函数替代
 
     void DirectClean(bool reset); // 直接清理
     void DelayedClean(); // 延迟清理
 
     void DirectExpire(bool reset); // 直接过期
     void DirectExpireSlot(MovementSlot slot, bool reset); // 特定槽位直接过期
     void DelayedExpire(); // 延迟过期
 
     typedef std::vector<_Ty> ExpireList;
     ExpireList* _expList;
     _Ty Impl[MAX_MOTION_SLOT];
     int _top;
     Unit* _owner;
     bool _needInit[MAX_MOTION_SLOT];
     uint8 _cleanFlag;
 };
 #endif
