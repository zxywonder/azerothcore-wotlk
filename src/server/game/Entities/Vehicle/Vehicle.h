/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Sofstware Foundation; either version 3 of the License, or (at your
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

 #ifndef __ACORE_VEHICLE_H
 #define __ACORE_VEHICLE_H
 
 #include "Unit.h"
 #include "VehicleDefines.h"
 
 struct VehicleEntry;
 class Unit;
 
 /// 车辆类，继承自TransportBase
 class Vehicle : public TransportBase
 {
 public:
     /// 安装车辆
     void Install();
     /// 卸载车辆
     void Uninstall();
     /// 重置车辆状态
     void Reset(bool evading = false);
     /// 安装所有配件
     void InstallAllAccessories(bool evading);
     /// 应用所有免疫状态
     void ApplyAllImmunities();
     /// 安装配件（可能被脚本调用）
     void InstallAccessory(uint32 entry, int8 seatId, bool minion, uint8 type, uint32 summonTime);
 
     /// 获取车辆的基础单位
     Unit* GetBase() const { return _me; }
     /// 获取车辆信息
     VehicleEntry const* GetVehicleInfo() const { return _vehicleInfo; }
     /// 获取生物条目ID
     uint32 GetCreatureEntry() const { return _creatureEntry; }
 
     /// 判断指定座位是否为空
     bool HasEmptySeat(int8 seatId) const;
     /// 获取指定座位上的乘客
     Unit* GetPassenger(int8 seatId) const;
     /// 获取下一个空座位
     int8 GetNextEmptySeat(int8 seatId, bool next) const;
     /// 获取乘客所在座位的附加信息
     VehicleSeatAddon const* GetSeatAddonForSeatOfPassenger(Unit const* passenger) const;
     /// 获取可用座位数量
     uint8 GetAvailableSeatCount() const;
 
     /// 添加乘客到指定座位
     bool AddPassenger(Unit* passenger, int8 seatId = -1);
     /// 弹出乘客
     void EjectPassenger(Unit* passenger, Unit* controller);
     /// 移除乘客
     void RemovePassenger(Unit* passenger);
     /// 重新定位所有乘客
     void RelocatePassengers();
     /// 移除所有乘客
     void RemoveAllPassengers();
     /// 解散车辆
     void Dismiss();
     /// 检查车辆是否正在使用
     bool IsVehicleInUse();
     /// 将车辆传送到指定坐标
     void TeleportVehicle(float x, float y, float z, float ang);
 
     /// 座位映射
     SeatMap Seats;
 
     /// 获取乘客所在座位的信息
     VehicleSeatEntry const* GetSeatForPassenger(Unit const* passenger);
     /// 获取乘客所在座位的迭代器
     SeatMap::iterator GetSeatIteratorForPassenger(Unit* passenger);
 
 protected:
     friend bool Unit::CreateVehicleKit(uint32 id, uint32 creatureEntry);
     /// 构造函数
     Vehicle(Unit* unit, VehicleEntry const* vehInfo, uint32 creatureEntry);
     friend void Unit::RemoveVehicleKit();
     /// 析构函数
     ~Vehicle() override;
 
 private:
     /// 状态枚举
     enum Status
     {
         STATUS_NONE,
         STATUS_INSTALLED,
         STATUS_UNINSTALLING,
     };
 
     /// 初始化基础单位的移动信息
     void InitMovementInfoForBase();
 
     /// 将提供的运输偏移量转换为全局坐标
     void CalculatePassengerPosition(float& x, float& y, float& z, float* o /*= nullptr*/) const override
     {
         TransportBase::CalculatePassengerPosition(x, y, z, o,
                 GetBase()->GetPositionX(), GetBase()->GetPositionY(),
                 GetBase()->GetPositionZ(), GetBase()->GetOrientation());
     }
 
     /// 将提供的全局坐标转换为本地偏移量
     void CalculatePassengerOffset(float& x, float& y, float& z, float* o /*= nullptr*/) const override
     {
         TransportBase::CalculatePassengerOffset(x, y, z, o,
                                                 GetBase()->GetPositionX(), GetBase()->GetPositionY(),
                                                 GetBase()->GetPositionZ(), GetBase()->GetOrientation());
     }
 
     /// 基础单位
     Unit* _me;
     /// 车辆信息
     VehicleEntry const* _vehicleInfo;
     /// 可用座位数（用于正确显示标志）
     uint32 _usableSeatNum;
     /// 生物条目ID（可能与_me->GetEntry()不同，例如玩家的情况）
     uint32 _creatureEntry;
     /// 当前状态
     Status _status;
 };
 
 /// 车辆消失事件类
 class VehicleDespawnEvent : public BasicEvent
 {
 public:
     /// 构造函数
     VehicleDespawnEvent(Unit& self, uint32 duration) : _self(self), _duration(duration) { }
     /// 执行事件
     bool Execute(uint64 e_time, uint32 p_time) override;
 
 protected:
     /// 引用的单位
     Unit& _self;
     /// 持续时间
     uint32 _duration;
 };
 
 #endif
