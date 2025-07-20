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

 #ifndef AZEROTHCORE_PET_H
 #define AZEROTHCORE_PET_H
 
 #include "CharmInfo.h"
 #include "PetDefines.h"
 #include "TemporarySummon.h"
 
 // 宠物失去快乐值的时间间隔（毫秒）
 constexpr auto PET_LOSE_HAPPINES_INTERVAL = 7500;
 // 每个快乐等级对应的数值大小
 constexpr auto HAPPINESS_LEVEL_SIZE = 333000;
 
 // 宠物技能结构体
 struct PetSpell
 {
     ActiveStates active;  // 技能激活状态
     PetSpellState state;  // 技能状态
     PetSpellType type;    // 技能类型
 };
 
 // 宠物技能映射类型定义
 typedef std::unordered_map<uint32, PetSpell> PetSpellMap;
 // 自动施法列表类型定义
 typedef std::vector<uint32> AutoSpellList;
 
 class Player;
 
 // 宠物类，继承自Guardian
 class Pet : public Guardian
 {
 public:
     // 构造函数，指定拥有者和宠物类型
     explicit Pet(Player* owner, PetType type = MAX_PET_TYPE);
     // 析构函数
     ~Pet() override = default;
 
     // 添加到世界中（覆盖父类方法）
     void AddToWorld() override;
     // 从世界中移除（覆盖父类方法）
     void RemoveFromWorld() override;
 
     // 获取原生对象缩放比例（覆盖父类方法）
     float GetNativeObjectScale() const override;
     // 设置显示ID（覆盖父类方法）
     void SetDisplayId(uint32 modelId, float displayScale = 1.f) override;
 
     // 获取宠物类型
     PetType getPetType() const { return m_petType; }
     // 设置宠物类型
     void setPetType(PetType type) { m_petType = type; }
     // 判断是否受控制
     bool isControlled() const { return getPetType() == SUMMON_PET || getPetType() == HUNTER_PET; }
     // 判断是否为临时召唤的宠物
     bool isTemporarySummoned() const { return m_duration > 0s; }
 
     // 判断是否为玩家的永久宠物（在角色窗口中有标签且设置UNIT_FIELD_PETNUMBER）
     bool IsPermanentPetFor(Player* owner) const;
 
     // 创建宠物
     bool Create(ObjectGuid::LowType guidlow, Map* map, uint32 phaseMask, uint32 Entry, uint32 pet_number);
     // 基于生物创建宠物基础信息
     bool CreateBaseAtCreature(Creature* creature);
     // 基于生物模板创建宠物基础信息
     bool CreateBaseAtCreatureInfo(CreatureTemplate const* cinfo, Unit* owner);
     // 基于驯服创建宠物
     bool CreateBaseAtTamed(CreatureTemplate const* cinfo, Map* map, uint32 phaseMask);
     // 获取加载宠物的信息
     static std::pair<PetStable::PetInfo const*, PetSaveMode> GetLoadPetInfo(PetStable const& stable, uint32 petEntry, uint32 petnumber, bool current);
     // 从数据库加载宠物
     bool LoadPetFromDB(Player* owner, uint32 petEntry, uint32 petnumber, bool current, uint32 healthPct = 0, bool fullMana = false);
     // 判断是否正在加载
     bool isBeingLoaded() const override { return m_loading; }
     // 保存宠物到数据库
     void SavePetToDB(PetSaveMode mode);
     // 填充宠物信息
     void FillPetInfo(PetStable::PetInfo* petInfo) const;
     // 移除宠物
     void Remove(PetSaveMode mode, bool returnreagent = false);
     // 从数据库删除宠物
     static void DeleteFromDB(ObjectGuid::LowType guidlow);
 
     // 设置死亡状态（覆盖Creature和Unit的方法）
     void setDeathState(DeathState s, bool despawn = false) override;
     // 更新宠物状态（覆盖Creature和Unit的方法）
     void Update(uint32 diff) override;
 
     // 获取自动施法技能数量
     uint8 GetPetAutoSpellSize() const override { return m_autospells.size(); }
     // 获取指定位置的自动施法技能
     uint32 GetPetAutoSpellOnPos(uint8 pos) const override
     {
         if (pos >= m_autospells.size())
             return 0;
         else
             return m_autospells[pos];
     }
 
     // 宠物失去快乐值
     void LoseHappiness();
     // 获取快乐状态
     HappinessState GetHappinessState();
     // 给宠物增加经验值
     void GivePetXP(uint32 xp);
     // 给宠物升级
     void GivePetLevel(uint8 level);
     // 与拥有者同步等级
     void SynchronizeLevelWithOwner();
     // 判断物品是否在宠物食谱中
     bool HaveInDiet(ItemTemplate const* item) const;
     // 获取当前食物的增益效果等级
     uint32 GetCurrentFoodBenefitLevel(uint32 itemlevel) const;
     // 设置持续时间
     void SetDuration(Milliseconds dur) { m_duration = dur; }
     // 获取持续时间
     Milliseconds GetDuration() const { return m_duration; }
 
     // 切换自动施法状态
     void ToggleAutocast(SpellInfo const* spellInfo, bool apply);
 
     // 判断宠物是否拥有指定技能
     bool HasSpell(uint32 spell) const override;
 
     // 学习宠物被动技能
     void LearnPetPassives();
     // 施放宠物光环
     void CastPetAuras(bool current);
 
     // 在目标可用时施放技能
     void CastWhenWillAvailable(uint32 spellid, Unit* spellTarget, ObjectGuid oldTarget, bool spellIsPositive = false);
     // 清除待施放技能
     void ClearCastWhenWillAvailable();
     // 移除技能冷却
     void RemoveSpellCooldown(uint32 spell_id, bool update /* = false */);
 
     // 保存技能冷却到数据库
     void _SaveSpellCooldowns(CharacterDatabaseTransaction trans);
     // 保存光环到数据库
     void _SaveAuras(CharacterDatabaseTransaction trans);
     // 保存技能到数据库
     void _SaveSpells(CharacterDatabaseTransaction trans);
 
     // 从数据库加载技能冷却
     void _LoadSpellCooldowns(PreparedQueryResult result);
     // 从数据库加载光环
     void _LoadAuras(PreparedQueryResult result, uint32 timediff);
     // 从数据库加载技能
     void _LoadSpells(PreparedQueryResult result);
 
     // 添加技能
     bool addSpell(uint32 spellId, ActiveStates active = ACT_DECIDE, PetSpellState state = PETSPELL_NEW, PetSpellType type = PETSPELL_NORMAL);
     // 学习技能
     bool learnSpell(uint32 spell_id);
     // 学习高级技能
     void learnSpellHighRank(uint32 spellid);
     // 初始化升级技能
     void InitLevelupSpellsForLevel();
     // 忘记技能
     bool unlearnSpell(uint32 spell_id, bool learn_prev, bool clear_ab = true);
     // 移除技能
     bool removeSpell(uint32 spell_id, bool learn_prev, bool clear_ab = true);
     // 清理动作条
     void CleanupActionBar();
     // 生成动作条数据
     std::string GenerateActionBarData() const;
 
     PetSpellMap     m_spells;         // 宠物技能映射
     AutoSpellList   m_autospells;     // 自动施法技能列表
 
     // 初始化宠物创建时的技能
     void InitPetCreateSpells();
 
     // 重置天赋
     bool resetTalents();
     // 为玩家的所有宠物重置天赋
     static void resetTalentsForAllPetsOf(Player* owner, Pet* online_pet = nullptr);
     // 初始化等级对应的天赋
     void InitTalentForLevel();
 
     // 获取指定等级的最大天赋点数
     uint8 GetMaxTalentPointsForLevel(uint8 level);
     // 获取剩余天赋点数
     uint8 GetFreeTalentPoints() { return GetByteValue(UNIT_FIELD_BYTES_1, 1); }
     // 设置剩余天赋点数
     void SetFreeTalentPoints(uint8 points) { SetByteValue(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_PET_TALENTS, points); }
 
     uint32  m_usedTalentCount; // 已使用的天赋点数
 
     // 获取用于团队更新的光环掩码
     uint64 GetAuraUpdateMaskForRaid() const { return m_auraRaidUpdateMask; }
     // 设置用于团队更新的光环掩码
     void SetAuraUpdateMaskForRaid(uint8 slot) { m_auraRaidUpdateMask |= (uint64(1) << slot); }
     // 重置用于团队更新的光环掩码
     void ResetAuraUpdateMaskForRaid() { m_auraRaidUpdateMask = 0; }
 
     // 获取被拒绝的名字
     DeclinedName const* GetDeclinedNames() const { return m_declinedname.get(); }
 
     // 标记宠物是否已被移除（防止在下一次更新时覆盖数据库状态）
     bool m_removed;
 
     // 获取拥有者
     Player* GetOwner() const;
     // 设置加载状态
     void SetLoading(bool load) { m_loading = load; }
 
     // 判断是否有临时技能
     [[nodiscard]] bool HasTempSpell() const { return m_tempspell != 0; }
 
     // 获取调试信息
     std::string GetDebugInfo() const override;
 
 protected:
     Player* m_owner;                  // 拥有者
     int32   m_happinessTimer;         // 快乐值计时器
     PetType m_petType;                // 宠物类型
     Milliseconds m_duration;          // 持续时间（主要用于召唤的守护者）
     uint64  m_auraRaidUpdateMask;     // 团队光环更新掩码
     bool    m_loading;                // 加载状态
     Milliseconds m_petRegenTimer;     // 宠物恢复计时器（用于焦点恢复）
 
     std::unique_ptr<DeclinedName> m_declinedname;  // 被拒绝的名字
 
     ObjectGuid m_tempspellTarget;     // 临时技能目标
     ObjectGuid m_tempoldTarget;       // 临时旧目标
     bool       m_tempspellIsPositive; // 临时技能是否为正面效果
     uint32     m_tempspell;           // 临时技能ID
 
 private:
     // 禁止调用Creature的保存方法
     void SaveToDB(uint32, uint8, uint32) override
     {
         ABORT();
     }
     // 禁止调用Creature的删除方法
     void DeleteFromDB() override
     {
         ABORT();
     }
 };
 #endif
