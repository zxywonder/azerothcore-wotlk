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

 #ifndef TRANSPORTS_H
 #define TRANSPORTS_H
 
 #include "GameObject.h"   // 包含游戏对象相关定义
 #include "Timer.h"        // 包含定时器相关定义
 #include "TransportMgr.h" // 包含运输工具管理器定义
 #include "VehicleDefines.h" // 包含载具相关常量定义
 #include "ZoneScript.h"   // 包含区域脚本定义
 
 struct CreatureData;      // 前向声明CreatureData结构体
 
 class Transport : public GameObject, public TransportBase
 {
 public:
     Transport() : GameObject() {} // 构造函数，初始化为GameObject
 
     // 计算乘客在运输工具中的位置
     void CalculatePassengerPosition(float& x, float& y, float& z, float* o = nullptr) const override 
     { 
         TransportBase::CalculatePassengerPosition(x, y, z, o, GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation()); 
     }
 
     // 计算乘客相对于运输工具的偏移量
     void CalculatePassengerOffset(float& x, float& y, float& z, float* o = nullptr) const override 
     { 
         TransportBase::CalculatePassengerOffset(x, y, z, o, GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation()); 
     }
 
     // 定义乘客集合类型
     typedef std::set<WorldObject*> PassengerSet;
 
     // 添加乘客（纯虚函数，子类必须实现）
     virtual void AddPassenger(WorldObject* passenger, bool withAll = false) = 0;
 
     // 移除乘客（纯虚函数，子类必须实现）
     virtual void RemovePassenger(WorldObject* passenger, bool withAll = false) = 0;
 
     // 获取乘客集合
     PassengerSet const& GetPassengers() const { return _passengers; }
 
     // 获取路径进度
     uint32 GetPathProgress() const { return GetGOValue()->Transport.PathProgress; }
 
     // 设置路径进度
     void SetPathProgress(uint32 val) { m_goValue.Transport.PathProgress = val; }
 
 protected:
     PassengerSet _passengers; // 存储当前运输工具的乘客集合
 };
 
 class MotionTransport : public Transport
 {
     friend MotionTransport* TransportMgr::CreateTransport(uint32, ObjectGuid::LowType, Map*); // TransportMgr是友元类，可创建MotionTransport实例
 
     MotionTransport(); // 构造函数
 public:
     ~MotionTransport() override; // 析构函数
 
     // 创建移动运输工具
     bool CreateMoTrans(ObjectGuid::LowType guidlow, uint32 entry, uint32 mapid, float x, float y, float z, float ang, uint32 animprogress);
 
     // 删除前清理
     void CleanupsBeforeDelete(bool finalCleanup = true) override;
 
     // 构建更新数据
     void BuildUpdate(UpdateDataMapType& data_map, UpdatePlayerSet&) override;
 
     // 更新运输工具状态
     void Update(uint32 diff) override;
 
     // 延迟更新
     void DelayedUpdate(uint32 diff);
 
     // 更新运输工具位置
     void UpdatePosition(float x, float y, float z, float o);
 
     // 添加乘客
     void AddPassenger(WorldObject* passenger, bool withAll = false) override;
 
     // 移除乘客
     void RemovePassenger(WorldObject* passenger, bool withAll = false) override;
 
     // 创建NPC乘客
     Creature* CreateNPCPassenger(ObjectGuid::LowType guid, CreatureData const* data);
 
     // 创建游戏对象乘客
     GameObject* CreateGOPassenger(ObjectGuid::LowType guid, GameObjectData const* data);
 
     // 加载静态乘客
     void LoadStaticPassengers();
 
     // 获取静态乘客集合
     PassengerSet const& GetStaticPassengers() const { return _staticPassengers; }
 
     // 卸载静态乘客
     void UnloadStaticPassengers();
 
     // 卸载非静态乘客
     void UnloadNonStaticPassengers();
 
     // 设置乘客是否已加载
     void SetPassengersLoaded(bool loaded) { _passengersLoaded = loaded; }
 
     // 判断乘客是否已加载
     bool PassengersLoaded() const { return _passengersLoaded; }
 
     // 获取关键帧路径信息
     KeyFrameVec const& GetKeyFrames() const { return _transportInfo->keyFrames; }
 
     // 启用/禁用运输工具移动
     void EnableMovement(bool enabled);
 
     // 获取运输工具模板
     TransportTemplate const* GetTransportTemplate() const { return _transportInfo; }
 
     // 获取周期
     uint32 GetPeriod() const { return GetUInt32Value(GAMEOBJECT_LEVEL); }
 
     // 设置周期
     void SetPeriod(uint32 period) { SetUInt32Value(GAMEOBJECT_LEVEL, period); }
 
     // 获取调试信息
     std::string GetDebugInfo() const override;
 private:
     // 移动到下一个路径点
     void MoveToNextWaypoint();
 
     // 计算路径段上的位置
     float CalculateSegmentPos(float perc);
 
     // 传送运输工具到新地图
     bool TeleportTransport(uint32 newMapid, float x, float y, float z, float o);
 
     // 延迟传送运输工具
     void DelayedTeleportTransport();
 
     // 更新乘客位置
     void UpdatePassengerPositions(PassengerSet& passengers);
 
     // 如果存在事件，触发事件
     void DoEventIfAny(KeyFrame const& node, bool departure);
 
     // 判断运输工具是否正在移动
     bool IsMoving() const { return _isMoving; }
 
     // 设置是否正在移动
     void SetMoving(bool val) { _isMoving = val; }
 
     TransportTemplate const* _transportInfo; // 运输工具模板
     KeyFrameVec::const_iterator _currentFrame; // 当前路径点
     KeyFrameVec::const_iterator _nextFrame;    // 下一个路径点
     TimeTrackerSmall _positionChangeTimer;     // 位置变化计时器
     bool _isMoving;                            // 是否正在移动
     bool _pendingStop;                         // 是否有挂起的停止操作
 
     bool _triggeredArrivalEvent;               // 是否已触发到达事件
     bool _triggeredDepartureEvent;             // 是否已触发离开事件
 
     PassengerSet _staticPassengers;            // 静态乘客集合
     mutable std::mutex Lock;                   // 锁，用于线程安全
     bool _passengersLoaded;                    // 乘客是否已加载
     bool _delayedTeleport;                     // 是否需要延迟传送
 };
 
 class StaticTransport : public Transport
 {
 public:
     StaticTransport(); // 构造函数
     ~StaticTransport() override; // 析构函数
 
     // 创建静态运输工具
     bool Create(ObjectGuid::LowType guidlow, uint32 name_id, Map* map, uint32 phaseMask, float x, float y, float z, float ang, G3D::Quat const& rotation, uint32 animprogress, GOState go_state, uint32 artKit = 0) override;
 
     // 删除前清理
     void CleanupsBeforeDelete(bool finalCleanup = true) override;
 
     // 构建更新数据
     void BuildUpdate(UpdateDataMapType& data_map, UpdatePlayerSet&) override;
 
     // 更新运输工具状态
     void Update(uint32 diff) override;
 
     // 根据进度重新定位
     void RelocateToProgress(uint32 progress);
 
     // 更新位置
     void UpdatePosition(float x, float y, float z, float o);
 
     // 更新乘客位置
     void UpdatePassengerPositions();
 
     // 添加乘客
     void AddPassenger(WorldObject* passenger, bool withAll = false) override;
 
     // 移除乘客
     void RemovePassenger(WorldObject* passenger, bool withAll = false) override;
 
     // 获取暂停时间
     uint32 GetPauseTime() const { return GetUInt32Value(GAMEOBJECT_LEVEL); }
 
     // 设置暂停时间
     void SetPauseTime(uint32 val) { SetUInt32Value(GAMEOBJECT_LEVEL, val); }
 
     // 获取周期
     uint32 GetPeriod() const { return m_goValue.Transport.AnimationInfo ? m_goValue.Transport.AnimationInfo->TotalTime : GetPauseTime() + 2; }
 
 private:
     bool _needDoInitialRelocation; // 是否需要执行初始重新定位
 };
 
 #endif
