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

 #ifndef ACORE_WAYPOINTMOVEMENTGENERATOR_H
 #define ACORE_WAYPOINTMOVEMENTGENERATOR_H
 
 /** PathMovementGenerator 用于生成路径点
  * 和飞行路径的移动行为。其主要目的是生成活动，
  * 以便为玩家生成更新的数据包。
  */
 
 #include "MovementGenerator.h"
 #include "Player.h"
 #include "WaypointMgr.h"
 
 #define FLIGHT_TRAVEL_UPDATE  100
 #define TIMEDIFF_NEXT_WP      250
 
 /**
  * PathMovementBase 是一个基础类模板，用于处理路径移动的通用逻辑。
  * 提供了获取当前路径节点的功能。
  */
 template<class T, class P>
 class PathMovementBase
 {
 public:
     PathMovementBase() : i_path(), i_currentNode(0) {}
     PathMovementBase(P path) : i_path(path), i_currentNode(0) {}
     virtual ~PathMovementBase() {};
 
     /**
      * 获取当前节点索引
      * @return 当前节点索引
      */
     uint32 GetCurrentNode() const { return i_currentNode; }
 
 protected:
     P i_path;               // 存储当前路径
     uint32 i_currentNode;   // 当前路径节点索引
 };
 
 template<class T>
 class WaypointMovementGenerator;
 
 /**
  * 为 Creature 设计的 WaypointMovementGenerator 类
  * 继承自 MovementGeneratorMedium 和 PathMovementBase
  * 用于控制生物的路径点移动行为
  */
 template<>
 class WaypointMovementGenerator<Creature> : public MovementGeneratorMedium< Creature, WaypointMovementGenerator<Creature> >,
     public PathMovementBase<Creature, WaypointPath const*>
 {
 public:
     /**
      * 构造函数，初始化路径 ID、是否循环、是否停滞
      */
     WaypointMovementGenerator(uint32 _path_id = 0, bool _repeating = true, bool _stalled = false)
         : PathMovementBase((WaypointPath const*)nullptr), i_nextMoveTime(0), m_isArrivalDone(false), path_id(_path_id), repeating(_repeating), stalled(_stalled)  {}
     ~WaypointMovementGenerator() { i_path = nullptr; }
 
     /**
      * 初始化移动生成器
      */
     void DoInitialize(Creature*);
 
     /**
      * 结束移动生成器
      */
     void DoFinalize(Creature*);
 
     /**
      * 重置移动生成器
      */
     void DoReset(Creature*);
 
     /**
      * 更新移动状态
      */
     bool DoUpdate(Creature*, uint32 diff);
 
     /**
      * 暂停移动
      */
     void Pause(uint32 timer = 0);
 
     /**
      * 恢复移动
      */
     void Resume(uint32 overrideTimer/* = 0*/);
 
     /**
      * 移动通知，用于触发事件
      */
     void MovementInform(Creature*);
 
     /**
      * 获取移动生成器类型
      */
     MovementGeneratorType GetMovementGeneratorType() { return WAYPOINT_MOTION_TYPE; }
 
     // 以下是路径移动的具体实现
     void LoadPath(Creature*);
 
 private:
     /**
      * 停止移动，设置下一次移动时间
      */
     void Stop(int32 time) { i_nextMoveTime.Reset(time);}
 
     /**
      * 判断是否已停止
      */
     bool Stopped() { return !i_nextMoveTime.Passed();}
 
     /**
      * 判断是否可以移动
      */
     bool CanMove(int32 diff)
     {
         i_nextMoveTime.Update(diff);
         return i_nextMoveTime.Passed();
     }
 
     /**
      * 到达路径点时调用
      */
     void OnArrived(Creature*);
 
     /**
      * 开始移动
      */
     bool StartMove(Creature*);
 
     /**
      * 立即开始移动
      */
     void StartMoveNow(Creature* creature)
     {
         i_nextMoveTime.Reset(0);
         StartMove(creature);
     }
 
     TimeTrackerSmall i_nextMoveTime;  // 时间追踪器，用于控制下一次移动时间
     bool m_isArrivalDone;             // 是否已完成到达逻辑
     uint32 path_id;                   // 路径 ID
     bool repeating;                   // 是否重复路径
     bool stalled;                     // 是否停滞
 };
 
 /**
  * FlightPathMovementGenerator 用于生成玩家的飞行路径移动行为
  * 控制玩家在飞行过程中的移动和地图预加载
  */
 class FlightPathMovementGenerator : public MovementGeneratorMedium< Player, FlightPathMovementGenerator >,
     public PathMovementBase<Player, TaxiPathNodeList>
 {
     public:
         explicit FlightPathMovementGenerator(uint32 startNode = 0)
         {
             i_currentNode = startNode;
             _endGridX = 0.0f;
             _endGridY = 0.0f;
             _endMapId = 0;
             _preloadTargetNode = 0;
         }
 
         /**
          * 加载飞行路径
          */
         void LoadPath(Player* player);
 
         /**
          * 初始化飞行移动
          */
         void DoInitialize(Player*);
 
         /**
          * 重置飞行移动
          */
         void DoReset(Player*);
 
         /**
          * 结束飞行移动
          */
         void DoFinalize(Player*);
 
         /**
          * 更新飞行状态
          */
         bool DoUpdate(Player*, uint32);
 
         /**
          * 获取移动生成器类型
          */
         MovementGeneratorType GetMovementGeneratorType() override { return FLIGHT_MOTION_TYPE; }
 
         /**
          * 获取当前飞行路径
          */
         TaxiPathNodeList const& GetPath() { return i_path; }
 
         /**
          * 获取当前路径在地图结束时的节点索引
          */
         uint32 GetPathAtMapEnd() const;
 
         /**
          * 判断是否已到达终点
          */
         bool HasArrived() const { return (i_currentNode >= i_path.size()); }
 
         /**
          * 在传送后设置当前节点
          */
         void SetCurrentNodeAfterTeleport();
 
         /**
          * 跳过当前节点
          */
         void SkipCurrentNode() { ++i_currentNode; }
 
         /**
          * 触发与当前节点相关的事件
          */
         void DoEventIfAny(Player* player, TaxiPathNodeEntry const* node, bool departure);
 
         /**
          * 获取重置位置（用于传送）
          */
         bool GetResetPos(Player*, float& x, float& y, float& z);
 
         /**
          * 初始化终点地图坐标信息
          */
         void InitEndGridInfo();
 
         /**
          * 预加载终点地图
          */
         void PreloadEndGrid();
 
     private:
 
         float _endGridX;                  // 最后一个节点的 X 坐标
         float _endGridY;                  // 最后一个节点的 Y 坐标
         uint32 _endMapId;                 // 最后一个节点所在地图 ID
         uint32 _preloadTargetNode;        // 开始预加载的地图节点索引
 
         /**
          * 出租车路径切换信息结构体
          */
         struct TaxiNodeChangeInfo
         {
             uint32 PathIndex;  // 路径索引
             int32 Cost;        // 花费
         };
 
         std::deque<TaxiNodeChangeInfo> _pointsForPathSwitch;  // 路径切换点信息队列
 };
 
 #endif
