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

 #ifndef AZEROTHCORE_DYNAMICOBJECT_H
 #define AZEROTHCORE_DYNAMICOBJECT_H
 
 #include "Object.h"
 
 class Unit;
 class Aura;
 class SpellInfo;
 
 // 动态对象类型枚举
 enum DynamicObjectType
 {
     DYNAMIC_OBJECT_PORTAL           = 0x0,      // 未使用
     DYNAMIC_OBJECT_AREA_SPELL       = 0x1,      // 区域法术效果
     DYNAMIC_OBJECT_FARSIGHT_FOCUS   = 0x2,      // 远视焦点
 };
 
 // 动态对象类，继承自多个基础类
 class DynamicObject : public WorldObject, public GridObject<DynamicObject>, public MovableMapObject, public UpdatableMapObject
 {
 public:
     // 构造函数
     DynamicObject(bool isWorldObject);
     // 析构函数
     ~DynamicObject() override;
 
     // 添加到世界
     void AddToWorld() override;
     // 从世界中移除
     void RemoveFromWorld() override;
 
     // 删除前清理
     void CleanupsBeforeDelete(bool finalCleanup = true) override;
 
     // 创建动态对象
     bool CreateDynamicObject(ObjectGuid::LowType guidlow, Unit* caster, uint32 spellId, Position const& pos, float radius, DynamicObjectType type);
     // 更新逻辑
     void Update(uint32 p_time) override;
     // 移除动态对象
     void Remove();
     // 设置持续时间
     void SetDuration(int32 newDuration);
     // 获取持续时间
     [[nodiscard]] int32 GetDuration() const;
     // 延迟时间
     void Delay(int32 delaytime);
     // 绑定Aura
     void SetAura(Aura* aura);
     // 移除Aura
     void RemoveAura();
     // 设置视角
     void SetCasterViewpoint(bool updateViewerVisibility);
     // 移除视角
     void RemoveCasterViewpoint();
     // 获取施法者
     [[nodiscard]] Unit* GetCaster() const { return _caster; }
     // 绑定到施法者
     void BindToCaster();
     // 解除与施法者的绑定
     void UnbindFromCaster();
     // 获取法术ID
     [[nodiscard]] uint32 GetSpellId() const {  return GetUInt32Value(DYNAMICOBJECT_SPELLID); }
     // 获取施法者GUID
     [[nodiscard]] ObjectGuid GetCasterGUID() const { return GetGuidValue(DYNAMICOBJECT_CASTER); }
     // 获取作用半径
     [[nodiscard]] float GetRadius() const { return GetFloatValue(DYNAMICOBJECT_RADIUS); }
     // 是否是视角
     [[nodiscard]] bool IsViewpoint() const { return _isViewpoint; }
 
     // 获取旧的远视GUID
     ObjectGuid const& GetOldFarsightGUID() const { return _oldFarsightGUID; }
 
 protected:
     Aura* _aura;                          // Aura指针
     Aura* _removedAura;                   // 已移除的Aura指针
     Unit* _caster;                        // 施法者指针
     int32 _duration;                      // 持续时间（非Aura类型使用）
     bool _isViewpoint;                    // 是否是视角
     uint32 _updateViewerVisibilityTimer;  // 更新视角可见性计时器
     ObjectGuid _oldFarsightGUID;          // 旧的远视GUID
 };
 #endif
