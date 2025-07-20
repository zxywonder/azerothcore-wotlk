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

 #ifndef AZEROTHCORE_TEMPSUMMON_H
 #define AZEROTHCORE_TEMPSUMMON_H
 
 #include "Creature.h"
 
 // 定义召唤者类型枚举
 enum SummonerType
 {
     SUMMONER_TYPE_CREATURE      = 0,   // 生物类型召唤者
     SUMMONER_TYPE_GAMEOBJECT    = 1,   // 游戏对象类型召唤者
     SUMMONER_TYPE_MAP           = 2    // 地图类型召唤者
 };
 
 /// 存储临时召唤数据的结构体
 struct TempSummonData
 {
     uint32 entry;        ///< 被召唤生物的Entry
     Position pos;        ///< 生物生成的位置
     TempSummonType type; ///< 召唤类型，详见TempSummonType定义
     uint32 time;         ///< 取消召唤时间，仅特定类型可用
 };
 
 // 临时召唤类，继承自Creature
 class TempSummon : public Creature
 {
 public:
     explicit TempSummon(SummonPropertiesEntry const* properties, ObjectGuid owner, bool isWorldObject);
     ~TempSummon() override = default;
     void Update(uint32 time) override;  // 更新函数
     virtual void InitStats(uint32 lifetime);  // 初始化属性
     virtual void InitSummon();  // 初始化召唤
     virtual void UnSummon(uint32 msTime = 0);  // 取消召唤
     void UpdateObjectVisibilityOnCreate() override;  // 创建时更新可见性
     void RemoveFromWorld() override;  // 从世界中移除
     void SetTempSummonType(TempSummonType type);  // 设置临时召唤类型
     void SaveToDB(uint32 /*mapid*/, uint8 /*spawnMask*/, uint32 /*phaseMask*/) override {}  // 不保存到数据库
     [[nodiscard]] WorldObject* GetSummoner() const;  // 获取召唤者对象
     [[nodiscard]] Unit* GetSummonerUnit() const;  // 获取单位类型的召唤者
     [[nodiscard]] Creature* GetSummonerCreatureBase() const;  // 获取生物类型的召唤者
     [[nodiscard]] GameObject* GetSummonerGameObject() const;  // 获取游戏对象类型的召唤者
     ObjectGuid GetSummonerGUID() const { return m_summonerGUID; }  // 获取召唤者GUID
     TempSummonType GetSummonType() const { return m_type; }  // 获取召唤类型
     uint32 GetTimer() { return m_timer; }  // 获取定时器
     void SetTimer(uint32 t) { m_timer = t; }  // 设置定时器
 
     void SetVisibleBySummonerOnly(bool visibleBySummonerOnly) { _visibleBySummonerOnly = visibleBySummonerOnly; }  // 设置仅召唤者可见
     [[nodiscard]] bool IsVisibleBySummonerOnly() const { return _visibleBySummonerOnly; }  // 是否仅召唤者可见
 
     const SummonPropertiesEntry* const m_Properties;  // 召唤属性
 
     std::string GetDebugInfo() const override;  // 获取调试信息
 
 private:
     TempSummonType m_type;  // 召唤类型
     uint32 m_timer;  // 定时器
     uint32 m_lifetime;  // 生存时间
     ObjectGuid m_summonerGUID;  // 召唤者GUID
     bool _visibleBySummonerOnly;  // 是否仅召唤者可见
 };
 
 // 随从类，继承自TempSummon
 class Minion : public TempSummon
 {
 public:
     Minion(SummonPropertiesEntry const* properties, ObjectGuid owner, bool isWorldObject);
     void InitStats(uint32 duration) override;  // 初始化属性
     void RemoveFromWorld() override;  // 从世界中移除
     [[nodiscard]] Unit* GetOwner() const;  // 获取拥有者
     [[nodiscard]] float GetFollowAngle() const override { return m_followAngle; }  // 获取跟随角度
     void SetFollowAngle(float angle) { m_followAngle = angle; }  // 设置跟随角度
     [[nodiscard]] bool IsPetGhoul() const {return GetEntry() == 26125 /*普通食尸鬼*/ || GetEntry() == 30230 /*复活盟友食尸鬼*/;}  // 判断是否是食尸鬼
     [[nodiscard]] bool IsGuardianPet() const;  // 判断是否是守护宠物
     void setDeathState(DeathState s, bool despawn = false) override;  // 设置死亡状态（重写Unit方法）
 
     std::string GetDebugInfo() const override;  // 获取调试信息
 protected:
     const ObjectGuid m_owner;  // 拥有者GUID
     float m_followAngle;  // 跟随角度
 };
 
 // 守护者类，继承自Minion
 class Guardian : public Minion
 {
 public:
     Guardian(SummonPropertiesEntry const* properties, ObjectGuid owner, bool isWorldObject);
     void InitStats(uint32 duration) override;  // 初始化属性
     bool InitStatsForLevel(uint8 level);  // 根据等级初始化属性
     void InitSummon() override;  // 初始化召唤
 
     bool UpdateStats(Stats stat) override;  // 更新单个属性
     bool UpdateAllStats() override;  // 更新所有属性
     void UpdateArmor() override;  // 更新护甲值
     void UpdateMaxHealth() override;  // 更新最大生命值
     void UpdateMaxPower(Powers power) override;  // 更新最大能量值
     void UpdateAttackPowerAndDamage(bool ranged = false) override;  // 更新攻击强度和伤害
     void UpdateDamagePhysical(WeaponAttackType attType) override;  // 更新物理伤害
 
     std::string GetDebugInfo() const override;  // 获取调试信息
 };
 
 // 提线木偶类，继承自Minion
 class Puppet : public Minion
 {
 public:
     Puppet(SummonPropertiesEntry const* properties, ObjectGuid owner);
     void InitStats(uint32 duration) override;  // 初始化属性
     void InitSummon() override;  // 初始化召唤
     void Update(uint32 time) override;  // 更新函数
     void RemoveFromWorld() override;  // 从世界中移除
 protected:
     [[nodiscard]] Player* GetOwner() const;  // 获取玩家拥有者
     const ObjectGuid m_owner;  // 拥有者GUID
 };
 
 // 强制取消召唤延迟事件类，继承自BasicEvent
 class ForcedUnsummonDelayEvent : public BasicEvent
 {
 public:
     ForcedUnsummonDelayEvent(TempSummon& owner) : BasicEvent(), m_owner(owner) { }
     bool Execute(uint64 e_time, uint32 p_time) override;  // 执行事件
 
 private:
     TempSummon& m_owner;  // 事件所属的临时召唤对象
 };
 #endif
