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

 #ifndef AZEROTHCORE_CREATURE_H
 #define AZEROTHCORE_CREATURE_H
 
 #include "Cell.h"
 #include "CharmInfo.h"
 #include "Common.h"
 #include "CreatureData.h"
 #include "LootMgr.h"
 #include "Unit.h"
 #include <list>
 
 class SpellInfo;
 
 class CreatureAI;
 class Quest;
 class Player;
 class WorldSession;
 class CreatureGroup;
 
 // 生物攻击时允许的最大垂直坐标差异
 #define CREATURE_Z_ATTACK_RANGE 3
 
 #define MAX_VENDOR_ITEMS 150    // 3.x.x版本中SMSG_LIST_INVENTORY的最大物品数量限制
 
 class Creature : public Unit, public GridObject<Creature>, public MovableMapObject, public UpdatableMapObject
 {
 public:
     explicit Creature(bool isWorldObject = false);
     ~Creature() override;
 
     void AddToWorld() override;
     void RemoveFromWorld() override;
 
     // 获取生物的原始缩放比例
     float GetNativeObjectScale() const override;
     // 设置生物的缩放比例
     void SetObjectScale(float scale) override;
     // 设置生物的显示ID
     void SetDisplayId(uint32 displayId, float displayScale = 1.f) override;
     // 从模型索引设置显示
     void SetDisplayFromModel(uint32 modelIdx);
 
     // 让生物消失并死亡
     void DisappearAndDie();
 
     // 判断生物是否是带有图标对话的商贩
     [[nodiscard]] bool isVendorWithIconSpeak() const;
 
     // 创建生物
     bool Create(ObjectGuid::LowType guidlow, Map* map, uint32 phaseMask, uint32 Entry, uint32 vehId, float x, float y, float z, float ang, const CreatureData* data = nullptr);
     // 加载生物附加数据
     bool LoadCreaturesAddon(bool reload = false);
     // 选择等级
     void SelectLevel(bool changelevel = true);
     // 加载装备
     void LoadEquipment(int8 id = 1, bool force = false);
 
     // 获取生成ID
     [[nodiscard]] ObjectGuid::LowType GetSpawnId() const { return m_spawnId; }
 
     // 更新生物状态
     void Update(uint32 time) override;  // 覆盖Unit::Update
     // 获取重生位置
     void GetRespawnPosition(float& x, float& y, float& z, float* ori = nullptr, float* dist = nullptr) const;
 
     // 设置尸体延迟时间
     void SetCorpseDelay(uint32 delay) { m_corpseDelay = delay; }
     // 设置尸体移除时间
     void SetCorpseRemoveTime(uint32 delay);
     // 获取尸体延迟时间
     [[nodiscard]] uint32 GetCorpseDelay() const { return m_corpseDelay; }
     // 检查是否有额外标志
     [[nodiscard]] bool HasFlagsExtra(uint32 flag) const { return GetCreatureTemplate()->HasFlagsExtra(flag); }
     // 是否是种族领袖
     [[nodiscard]] bool IsRacialLeader() const { return GetCreatureTemplate()->RacialLeader; }
     // 是否是平民
     [[nodiscard]] bool IsCivilian() const { return HasFlagsExtra(CREATURE_FLAG_EXTRA_CIVILIAN); }
     // 是否是触发器
     [[nodiscard]] bool IsTrigger() const { return HasFlagsExtra(CREATURE_FLAG_EXTRA_TRIGGER); }
     // 是否是守卫
     [[nodiscard]] bool IsGuard() const { return HasFlagsExtra(CREATURE_FLAG_EXTRA_GUARD); }
     // 获取移动模板
     CreatureMovementData const& GetMovementTemplate() const;
     // 是否可以行走
     [[nodiscard]] bool CanWalk() const { return GetMovementTemplate().IsGroundAllowed(); }
     // 是否可以游泳
     [[nodiscard]] bool CanSwim() const override;
     // 是否可以进入水中
     [[nodiscard]] bool CanEnterWater() const override;
     // 是否可以飞行
     [[nodiscard]] bool CanFly()  const override { return GetMovementTemplate().IsFlightAllowed() || IsFlying(); }
     // 是否可以悬停
     [[nodiscard]] bool CanHover() const { return GetMovementTemplate().Ground == CreatureGroundMovementType::Hover || IsHovering(); }
     // 是否被定身
     [[nodiscard]] bool IsRooted() const { return GetMovementTemplate().IsRooted(); }
 
     // 获取默认移动类型
     MovementGeneratorType GetDefaultMovementType() const override { return m_defaultMovementType; }
     // 设置默认移动类型
     void SetDefaultMovementType(MovementGeneratorType mgt) { m_defaultMovementType = mgt; }
 
     /**
     * @brief 生物可以有三种反应状态：攻击、被动、中立
     * - 攻击：生物会攻击视野中的非友好单位
     * - 被动：生物不会攻击任何人
     * - 中立：只有被攻击时才会反击
     */
     void SetReactState(ReactStates state) { m_reactState = state; }
     // 获取反应状态
     [[nodiscard]] ReactStates GetReactState() const { return m_reactState; }
     // 检查是否具有指定反应状态
     [[nodiscard]] bool HasReactState(ReactStates state) const { return (m_reactState == state); }   /// @brief 检查生物是否具有指定的反应状态
     // 初始化反应状态
     void InitializeReactState();
 
     ///// @todo 重命名这个函数!!!!!
     bool isCanInteractWithBattleMaster(Player* player, bool msg) const;
     // 检查是否可以与玩家进行训练和重置天赋
     bool isCanTrainingAndResetTalentsOf(Player* player) const;
     // 检查是否是玩家的有效训练师
     [[nodiscard]] bool IsValidTrainerForPlayer(Player* player, uint32* npcFlags = nullptr) const;
     // 检查生物是否可以攻击目标
     bool CanCreatureAttack(Unit const* victim, bool skipDistCheck = false) const;
     // 加载法术模板免疫
     void LoadSpellTemplateImmunity();
     // 是否对指定法术免疫
     bool IsImmunedToSpell(SpellInfo const* spellInfo, Spell const* spell = nullptr) override;
 
     // 检查是否具有机制模板免疫
     [[nodiscard]] bool HasMechanicTemplateImmunity(uint32 mask) const;
     // 重定义Unit::IsImmunedToSpell
     // 检查是否对法术效果免疫
     bool IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index) const override;
     // 重定义Unit::IsImmunedToSpellEffect
     // 检查是否是精英怪
     [[nodiscard]] bool isElite() const
     {
         if (IsPet())
             return false;
 
         uint32 rank = GetCreatureTemplate()->rank;
         return rank != CREATURE_ELITE_NORMAL && rank != CREATURE_ELITE_RARE;
     }
 
     // 检查是否是世界Boss
     [[nodiscard]] bool isWorldBoss() const
     {
         if (IsPet())
             return false;
 
         return GetCreatureTemplate()->type_flags & CREATURE_TYPE_FLAG_BOSS_MOB;
     }
 
     // 检查是否是地下城Boss
     [[nodiscard]] bool IsDungeonBoss() const;
     // 检查是否免疫击退
     [[nodiscard]] bool IsImmuneToKnockback() const;
     // 检查是否避免AOE
     [[nodiscard]] bool IsAvoidingAOE() const { return HasFlagsExtra(CREATURE_FLAG_EXTRA_AVOID_AOE); }
 
     // 覆盖Unit::getLevelForTarget，支持Boss等级
     uint8 getLevelForTarget(WorldObject const* target) const override;
 
     // 检查是否处于逃跑模式
     [[nodiscard]] bool IsInEvadeMode() const { return HasUnitState(UNIT_STATE_EVADE); }
     // 检查是否在躲避攻击
     [[nodiscard]] bool IsEvadingAttacks() const { return IsInEvadeMode() || CanNotReachTarget(); }
 
     // 初始化AI
     bool AIM_Initialize(CreatureAI* ai = nullptr);
     // 初始化移动
     void Motion_Initialize();
 
     // 获取AI
     [[nodiscard]] CreatureAI* AI() const { return (CreatureAI*)i_AI; }
 
     // 设置行走状态
     bool SetWalk(bool enable) override;
     // 设置禁用重力状态
     bool SetDisableGravity(bool disable, bool packetOnly = false, bool updateAnimationTier = true) override;
     // 设置游泳状态
     bool SetSwim(bool enable) override;
     // 设置飞行状态
     bool SetCanFly(bool enable, bool packetOnly = false) override;
     // 设置水上行走状态
     bool SetWaterWalking(bool enable, bool packetOnly = false) override;
     // 设置羽毛降落状态
     bool SetFeatherFall(bool enable, bool packetOnly = false) override;
     // 设置悬停状态
     bool SetHover(bool enable, bool packetOnly = false, bool updateAnimationTier = true) override;
     // 检查是否有法术焦点
     bool HasSpellFocus(Spell const* focusSpell = nullptr) const;
 
     // 法术焦点信息
     struct
     {
         ::Spell const* Spell = nullptr;
         uint32 Delay = 0;         // 目标恢复原状的毫秒数（0表示无恢复）
         ObjectGuid Target;        // 施法时的"真实"目标
         float Orientation = 0.0f; // 施法时的"真实"朝向
     } _spellFocusInfo;
 
     // 获取盾牌格挡值
     [[nodiscard]] uint32 GetShieldBlockValue() const override
     {
         return (GetLevel() / 2 + uint32(GetStat(STAT_STRENGTH) / 20));
     }
 
     // 获取近战伤害学校掩码
     [[nodiscard]] SpellSchoolMask GetMeleeDamageSchoolMask(WeaponAttackType /*attackType*/ = BASE_ATTACK, uint8 /*damageIndex*/ = 0) const override { return m_meleeDamageSchoolMask; }
     // 设置近战伤害学校
     void SetMeleeDamageSchool(SpellSchools school) { m_meleeDamageSchoolMask = SpellSchoolMask(1 << school); }
 
     // 添加生物法术冷却
     void _AddCreatureSpellCooldown(uint32 spell_id, uint16 categoryId, uint32 end_time);
     // 添加法术冷却
     void AddSpellCooldown(uint32 spell_id, uint32 /*itemid*/, uint32 end_time, bool needSendToClient = false, bool forceSendToSpectator = false) override;
     // 检查是否有法术冷却
     [[nodiscard]] bool HasSpellCooldown(uint32 spell_id) const override;
     // 获取法术冷却时间
     [[nodiscard]] uint32 GetSpellCooldown(uint32 spell_id) const;
     // 禁止法术学校
     void ProhibitSpellSchool(SpellSchoolMask idSchoolMask, uint32 unTimeMs) override;
     // 检查法术学校是否被禁止
     [[nodiscard]] bool IsSpellProhibited(SpellSchoolMask idSchoolMask) const;
     // 清除禁止的法术计时器
     void ClearProhibitedSpellTimers();
 
     // 检查生物是否拥有指定法术
     [[nodiscard]] bool HasSpell(uint32 spellID) const override;
 
     // 更新移动标志
     void UpdateMovementFlags();
     // 获取随机ID
     uint32 GetRandomId(uint32 id1, uint32 id2, uint32 id3);
     // 更新生物条目
     bool UpdateEntry(uint32 entry, const CreatureData* data = nullptr, bool changelevel = true, bool updateAI = false);
     // 重载的UpdateEntry
     bool UpdateEntry(uint32 entry, bool updateAI) { return UpdateEntry(entry, nullptr, true, updateAI); }
     // 更新属性
     bool UpdateStats(Stats stat) override;
     // 更新所有属性
     bool UpdateAllStats() override;
     // 更新抗性
     void UpdateResistances(uint32 school) override;
     // 更新护甲
     void UpdateArmor() override;
     // 更新最大生命值
     void UpdateMaxHealth() override;
     // 更新最大能量值
     void UpdateMaxPower(Powers power) override;
     // 更新攻击强度和伤害
     void UpdateAttackPowerAndDamage(bool ranged = false) override;
     // 计算最小最大伤害
     void CalculateMinMaxDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, float& minDamage, float& maxDamage, uint8 damageIndex) override;
 
     // 加载对战百分比
     void LoadSparringPct();
     // 获取对战百分比
     [[nodiscard]] float GetSparringPct() const { return _sparringPct; }
 
     // 检查是否有武器
     bool HasWeapon(WeaponAttackType type) const override;
     // 检查是否有用于攻击的武器
     bool HasWeaponForAttack(WeaponAttackType type) const override { return (Unit::HasWeaponForAttack(type) && HasWeapon(type)); }
     // 设置是否可以双持
     void SetCanDualWield(bool value) override;
     // 获取原始装备ID
     [[nodiscard]] int8 GetOriginalEquipmentId() const { return m_originalEquipmentId; }
     // 获取当前装备ID
     uint8 GetCurrentEquipmentId() { return m_equipmentId; }
     // 设置当前装备ID
     void SetCurrentEquipmentId(uint8 id) { m_equipmentId = id; }
 
     // 获取法术伤害修正
     float GetSpellDamageMod(int32 Rank);
 
     // 获取商贩物品数据
     [[nodiscard]] VendorItemData const* GetVendorItems() const;
     // 获取商贩物品当前数量
     uint32 GetVendorItemCurrentCount(VendorItem const* vItem);
     // 更新商贩物品当前数量
     uint32 UpdateVendorItemCurrentCount(VendorItem const* vItem, uint32 used_count);
 
     // 获取训练师法术数据
     [[nodiscard]] TrainerSpellData const* GetTrainerSpells() const;
 
     // 获取生物模板
     [[nodiscard]] CreatureTemplate const* GetCreatureTemplate() const { return m_creatureInfo; }
     // 获取生物数据
     [[nodiscard]] CreatureData const* GetCreatureData() const { return m_creatureData; }
     // 设置检测距离
     void SetDetectionDistance(float dist){ m_detectionDistance = dist; }
     // 获取生物附加数据
     [[nodiscard]] CreatureAddon const* GetCreatureAddon() const;
 
     // 获取AI名称
     [[nodiscard]] std::string const& GetAIName() const;
     // 获取脚本名称
     [[nodiscard]] std::string GetScriptName() const;
     // 获取脚本ID
     [[nodiscard]] uint32 GetScriptId() const;
 
     // 重写WorldObject函数以实现名称本地化
     [[nodiscard]] std::string const& GetNameForLocaleIdx(LocaleConstant locale_idx) const override;
 
     // 设置死亡状态
     void setDeathState(DeathState s, bool despawn = false) override;    // 覆盖虚拟Unit::setDeathState
 
     // 从数据库加载生物
     bool LoadFromDB(ObjectGuid::LowType guid, Map* map, bool allowDuplicate = false) { return LoadCreatureFromDB(guid, map, false, allowDuplicate); }
     // 从数据库加载生物
     bool LoadCreatureFromDB(ObjectGuid::LowType guid, Map* map, bool addToMap = true, bool allowDuplicate = false);
     // 保存到数据库
     void SaveToDB();
 
     // 保存到数据库（虚函数）
     virtual void SaveToDB(uint32 mapid, uint8 spawnMask, uint32 phaseMask);   // 在Pet中重写
     // 从数据库删除
     virtual void DeleteFromDB();    // 在Pet中重写
 
     // 战利品
     Loot loot;
     // 获取战利品拾取者GUID
     [[nodiscard]] ObjectGuid GetLootRecipientGUID() const { return m_lootRecipient; }
     // 获取战利品拾取者
     [[nodiscard]] Player* GetLootRecipient() const;
     // 获取战利品拾取者队伍GUID
     [[nodiscard]] ObjectGuid::LowType GetLootRecipientGroupGUID() const { return m_lootRecipientGroup; }
     // 获取战利品拾取者队伍
     [[nodiscard]] Group* GetLootRecipientGroup() const;
     // 检查是否有战利品拾取者
     [[nodiscard]] bool hasLootRecipient() const { return m_lootRecipient || m_lootRecipientGroup; }
     // 检查玩家是否标记了该生物
     bool isTappedBy(Player const* player) const;    // 如果生物被玩家或其队伍成员标记则返回true
     // 检查是否可以生成偷窃战利品
     [[nodiscard]] bool CanGeneratePickPocketLoot() const;
     // 设置偷窃战利品时间
     void SetPickPocketLootTime();
     // 重置偷窃战利品时间
     void ResetPickPocketLootTime() { lootPickPocketRestoreTime = 0; }
 
     // 设置战利品拾取者
     void SetLootRecipient (Unit* unit, bool withGroup = true);
     // 所有战利品从尸体移除
     void AllLootRemovedFromCorpse();
 
     // 获取战利品模式
     [[nodiscard]] uint16 GetLootMode() const { return m_LootMode; }
     // 检查是否具有特定战利品模式
     [[nodiscard]] bool HasLootMode(uint16 lootMode) const { return m_LootMode & lootMode; }
     // 设置战利品模式
     void SetLootMode(uint16 lootMode) { m_LootMode = lootMode; }
     // 添加战利品模式
     void AddLootMode(uint16 lootMode) { m_LootMode |= lootMode; }
     // 移除战利品模式
     void RemoveLootMode(uint16 lootMode) { m_LootMode &= ~lootMode; }
     // 重置战利品模式
     void ResetLootMode() { m_LootMode = LOOT_MODE_DEFAULT; }
 
     // 寻找可以攻击目标的法术
     SpellInfo const* reachWithSpellAttack(Unit* victim);
     // 寻找可以治疗目标的法术
     SpellInfo const* reachWithSpellCure(Unit* victim);
 
     // 法术ID数组
     uint32 m_spells[MAX_CREATURE_SPELLS];
     // 法术冷却数据
     CreatureSpellCooldowns m_CreatureSpellCooldowns;
     // 禁止的学派时间
     uint32 m_ProhibitSchoolTime[7];
 
     // 检查是否可以开始攻击
     bool CanStartAttack(Unit const* u) const;
     // 获取仇恨范围
     float GetAggroRange(Unit const* target) const;
     // 获取攻击距离
     float GetAttackDistance(Unit const* player) const;
     // 获取检测范围
     [[nodiscard]] float GetDetectionRange() const { return m_detectionDistance; }
 
     // 发送AI反应
     void SendAIReaction(AiReaction reactionType);
 
     // 选择最近的目标
     [[nodiscard]] Unit* SelectNearestTarget(float dist = 0, bool playerOnly = false) const;
     // 选择最近的攻击距离内的目标
     [[nodiscard]] Unit* SelectNearestTargetInAttackDistance(float dist) const;
 
     // 逃跑求援
     void DoFleeToGetAssistance();
     // 呼叫帮助
     void CallForHelp(float fRadius, Unit* target = nullptr);
     // 呼叫援助
     void CallAssistance(Unit* target = nullptr);
     // 设置不呼叫援助
     void SetNoCallAssistance(bool val) { m_AlreadyCallAssistance = val; }
     // 设置不搜索援助
     void SetNoSearchAssistance(bool val) { m_AlreadySearchedAssistance = val; }
     // 是否已搜索援助
     bool HasSearchedAssistance() { return m_AlreadySearchedAssistance; }
     // 检查是否可以协助攻击
     bool CanAssistTo(Unit const* u, Unit const* enemy, bool checkfaction = true) const;
     // 检查目标是否可接受
     bool _IsTargetAcceptable(Unit const* target) const;
     // 检查是否可以忽略假死
     [[nodiscard]] bool CanIgnoreFeignDeath() const { return HasFlagsExtra(CREATURE_FLAG_EXTRA_IGNORE_FEIGN_DEATH); }
 
     // 当阵营改变时更新，如果实际阵营对任何人不具敌意，则禁用视线移动
     void UpdateMoveInLineOfSightState();
     // 检查是否禁用视线移动
     bool IsMoveInLineOfSightDisabled() { return m_moveInLineOfSightDisabled; }
     // 检查是否严格禁用视线移动
     bool IsMoveInLineOfSightStrictlyDisabled() { return m_moveInLineOfSightStrictlyDisabled; }
 
     // 移除尸体
     void RemoveCorpse(bool setSpawnTime = true, bool skipVisibility = false);
 
     // 取消生成或解除召唤
     void DespawnOrUnsummon(Milliseconds msTimeToDespawn, Seconds forcedRespawnTimer);
     // 重载的DespawnOrUnsummon
     void DespawnOrUnsummon(uint32 msTimeToDespawn = 0) { DespawnOrUnsummon(Milliseconds(msTimeToDespawn), 0s); };
     // 逃跑时取消生成
     void DespawnOnEvade(Seconds respawnDelay = 20s);
 
     // 获取重生时间
     [[nodiscard]] time_t const& GetRespawnTime() const { return m_respawnTime; }
     // 获取扩展的重生时间
     [[nodiscard]] time_t GetRespawnTimeEx() const;
     // 设置重生时间
     void SetRespawnTime(uint32 respawn);
     // 重生
     void Respawn(bool force = false);
     // 保存重生时间
     void SaveRespawnTime() override;
 
     // 获取重生延迟
     [[nodiscard]] uint32 GetRespawnDelay() const { return m_respawnDelay; }
     // 设置重生延迟
     void SetRespawnDelay(uint32 delay) { m_respawnDelay = delay; }
 
     // 获取战斗脉冲延迟
     uint32 GetCombatPulseDelay() const { return m_combatPulseDelay; }
     // 设置战斗脉冲延迟
     void SetCombatPulseDelay(uint32 delay) // (secs) interval at which the creature pulses the entire zone into combat (only works in dungeons)
     {
         m_combatPulseDelay = delay;
         if (m_combatPulseTime == 0 || m_combatPulseTime > delay)
             m_combatPulseTime = delay;
     }
 
     // 获取漫游距离
     [[nodiscard]] float GetWanderDistance() const { return m_wanderDistance; }
     // 设置漫游距离
     void SetWanderDistance(float dist) { m_wanderDistance = dist; }
 
     // 立即进行边界检查
     void DoImmediateBoundaryCheck() { m_boundaryCheckTime = 0; }
 
     // 组队战利品计时器
     uint32 m_groupLootTimer;    // (msecs)用于组队战利品的计时器
     // 抢夺战利品的队伍低GUID
     uint32 lootingGroupLowGUID;   // 用于查找正在拾取尸体的队伍
 
     // 发送区域被攻击消息
     void SendZoneUnderAttackMessage(Player* attacker);
 
     // 设置区域内战斗
     void SetInCombatWithZone();
 
     // 检查是否有指定任务
     [[nodiscard]] bool hasQuest(uint32 quest_id) const override;
     // 检查是否涉及指定任务
     [[nodiscard]] bool hasInvolvedQuest(uint32 quest_id)  const override;
 
     // 检查是否在恢复生命
     bool isRegeneratingHealth() { return m_regenHealth; }
     // 设置生命恢复
     void SetRegeneratingHealth(bool enable) { m_regenHealth = enable; }
     // 设置能量恢复
     void SetRegeneratingPower(bool enable) { m_regenPower = enable; }
     // 获取宠物自动法术数量
     [[nodiscard]] virtual uint8 GetPetAutoSpellSize() const { return MAX_SPELL_CHARM; }
     // 获取宠物自动法术
     [[nodiscard]] virtual uint32 GetPetAutoSpellOnPos(uint8 pos) const
     {
         if (pos >= MAX_SPELL_CHARM || m_charmInfo->GetCharmSpell(pos)->GetType() != ACT_ENABLED)
             return 0;
         else
             return m_charmInfo->GetCharmSpell(pos)->GetAction();
     }
 
     // 设置无法到达的目标
     void SetCannotReachTarget(ObjectGuid const& target = ObjectGuid::Empty);
     // 检查是否无法到达目标
     [[nodiscard]] bool CanNotReachTarget() const;
     // 检查是否不可达且需要回血
     [[nodiscard]] bool IsNotReachableAndNeedRegen() const;
 
     // 设置位置
     void SetPosition(float x, float y, float z, float o);
     void SetPosition(const Position& pos) { SetPosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation()); }
 
     // 设置家的位置
     void SetHomePosition(float x, float y, float z, float o) { m_homePosition.Relocate(x, y, z, o); }
     void SetHomePosition(const Position& pos) { m_homePosition.Relocate(pos); }
     void GetHomePosition(float& x, float& y, float& z, float& ori) const { m_homePosition.GetPosition(x, y, z, ori); }
     [[nodiscard]] Position const& GetHomePosition() const { return m_homePosition; }
 
     // 设置交通工具家的位置
     void SetTransportHomePosition(float x, float y, float z, float o) { m_transportHomePosition.Relocate(x, y, z, o); }
     void SetTransportHomePosition(const Position& pos) { m_transportHomePosition.Relocate(pos); }
     void GetTransportHomePosition(float& x, float& y, float& z, float& ori) const { m_transportHomePosition.GetPosition(x, y, z, ori); }
     [[nodiscard]] Position const& GetTransportHomePosition() const { return m_transportHomePosition; }
 
     // 获取路径ID
     [[nodiscard]] uint32 GetWaypointPath() const { return m_path_id; }
     // 加载路径
     void LoadPath(uint32 pathid) { m_path_id = pathid; }
 
     // 获取当前航点ID
     [[nodiscard]] uint32 GetCurrentWaypointID() const { return m_waypointID; }
     // 更新航点ID
     void UpdateWaypointID(uint32 wpID) { m_waypointID = wpID; }
 
     // 搜索编队
     void SearchFormation();
     // 获取编队
     [[nodiscard]] CreatureGroup const* GetFormation() const { return m_formation; }
     [[nodiscard]] CreatureGroup* GetFormation() { return m_formation; }
     // 设置编队
     void SetFormation(CreatureGroup* formation) { m_formation = formation; }
 
     // 选择受害者
     Unit* SelectVictim();
 
     // 设置禁用声誉奖励
     void SetReputationRewardDisabled(bool disable) { DisableReputationReward = disable; }
     // 检查是否禁用声誉奖励
     [[nodiscard]] bool IsReputationRewardDisabled() const { return DisableReputationReward; }
     // 设置禁用战利品奖励
     void SetLootRewardDisabled(bool disable) { DisableLootReward = disable; }
     // 检查是否禁用战利品奖励
     [[nodiscard]] bool IsLootRewardDisabled() const { return DisableLootReward; }
     // 检查伤害是否足够获得战利品和奖励
     [[nodiscard]] bool IsDamageEnoughForLootingAndReward() const;
     // 减少玩家伤害需求
     void LowerPlayerDamageReq(uint32 unDamage, bool damagedByPlayer = true);
     // 重置玩家伤害需求
     void ResetPlayerDamageReq();
     // 获取玩家伤害需求
     [[nodiscard]] uint32 GetPlayerDamageReq() const;
 
     // 获取原始条目
     [[nodiscard]] uint32 GetOriginalEntry() const { return m_originalEntry; }
     // 设置原始条目
     void SetOriginalEntry(uint32 entry) { m_originalEntry = entry; }
 
     // 获取伤害修正
     static float _GetDamageMod(int32 Rank);
 
     // 视距和战斗距离
     float m_SightDistance, m_CombatDistance;
 
     // 标记是否为临时世界对象（例如被控制时）
     bool m_isTempWorldObject; //true when possessed
 
     // 施法时处理目标朝向
     void SetTarget(ObjectGuid guid = ObjectGuid::Empty) override;
     // 清除目标
     void ClearTarget() { SetTarget(); };
     // 聚焦目标
     void FocusTarget(Spell const* focusSpell, WorldObject const* target);
     // 释放焦点
     void ReleaseFocus(Spell const* focusSpell);
     // 检查施法时是否阻止移动
     [[nodiscard]] bool IsMovementPreventedByCasting() const override;
 
     // 逃跑机制的一部分
     // 获取最后一次延长牵引绳时间的共享指针
     std::shared_ptr<time_t> const& GetLastLeashExtensionTimePtr() const;
     // 设置最后一次延长牵引绳时间的共享指针
     void SetLastLeashExtensionTimePtr(std::shared_ptr<time_t> const& timer);
     // 清除最后一次延长牵引绳时间的共享指针
     void ClearLastLeashExtensionTimePtr();
     // 获取最后一次延长牵引绳时间
     time_t GetLastLeashExtensionTime() const;
     // 更新延长牵引绳时间
     void UpdateLeashExtensionTime();
     // 获取牵引计时器
     uint8 GetLeashTimer() const;
 
     // 检查是否可以自由移动
     bool IsFreeToMove();
     // 移动检查间隔常量
     static constexpr uint32 MOVE_CIRCLE_CHECK_INTERVAL = 3000;
     static constexpr uint32 MOVE_BACKWARDS_CHECK_INTERVAL = 2000;
     static constexpr uint32 EXTEND_LEASH_CHECK_INTERVAL = 3000;
     // 移动检查计时器
     uint32 m_moveCircleMovementTime = MOVE_CIRCLE_CHECK_INTERVAL;
     uint32 m_moveBackwardsMovementTime = MOVE_BACKWARDS_CHECK_INTERVAL;
     uint32 m_extendLeashTime = EXTEND_LEASH_CHECK_INTERVAL;
 
     // 检查是否在非战斗状态下具有游泳标志
     [[nodiscard]] bool HasSwimmingFlagOutOfCombat() const
     {
         return !_isMissingSwimmingFlagOutOfCombat;
     }
     // 刷新游泳标志
     void RefreshSwimmingFlag(bool recheck = false);
 
     // 设置援助计时器
     void SetAssistanceTimer(uint32 value) { m_assistanceTimer = value; }
 
     // 修改威胁百分比
     void ModifyThreatPercentTemp(Unit* victim, int32 percent, Milliseconds duration);
 
     /**
      * @brief 恢复追击受害者的辅助函数。
      */
     void ResumeChasingVictim() { GetMotionMaster()->MoveChase(GetVictim()); };
 
     /**
      * @brief 返回生物是否能够施放该法术。
      */
     bool CanCastSpell(uint32 spellID) const;
 
     /**
      * @brief 获取召唤者的GUID（如果该生物是召唤物）
      */
     [[nodiscard]] ObjectGuid GetSummonerGUID() const;
 
     // 控制是否在AttackStart()中使用MoveChase()
     // 注意：如果在战斗中使用SetCombatMovement，它将不起作用 - 这只影响AttackStart()
     //       你应该采取必要的措施使其生效
     //       请注意，如果你修改了_isCombatMovementAllowed（例如使用SetCombatMovement），它不会在Reset()中重置
     //       它将保持你设置的最后一个值
     void SetCombatMovement(bool allowMovement);
     // 检查是否允许战斗移动
     bool IsCombatMovementAllowed() const { return _isCombatMovementAllowed; }
 
     // 获取调试信息
     std::string GetDebugInfo() const override;
 
     // 检查是否需要更新
     bool IsUpdateNeeded() override;
 
 protected:
     // 从原型创建生物
     bool CreateFromProto(ObjectGuid::LowType guidlow, uint32 Entry, uint32 vehId, const CreatureData* data = nullptr);
     // 初始化条目
     bool InitEntry(uint32 entry, const CreatureData* data = nullptr);
 
     // 商贩物品计数
     VendorItemCounts m_vendorItemCounts;
 
     // 获取生命修正
     static float _GetHealthMod(int32 Rank);
 
     // 战利品拾取者GUID
     ObjectGuid m_lootRecipient;
     // 战利品拾取者队伍GUID
     ObjectGuid::LowType m_lootRecipientGroup;
 
     /// 计时器
     time_t m_corpseRemoveTime;                          // (秒) 尸体消失时间
     time_t m_respawnTime;                               // (秒) 下次重生时间
     time_t m_respawnedTime;                             // (秒) 生物重生时间
     uint32 m_respawnDelay;                              // (秒) 尸体消失和重生之间的延迟
     uint32 m_corpseDelay;                               // (秒) 死亡和尸体消失之间的延迟
     float m_wanderDistance;                             // 漫游距离
     uint32 m_boundaryCheckTime;                         // (毫秒) 下次逃跑边界检查剩余时间
     uint16 m_transportCheckTimer;                       // 交通工具检查计时器
     uint32 lootPickPocketRestoreTime;                   // 偷窃战利品恢复时间
     uint32 m_combatPulseTime;                           // (毫秒) 下次区域战斗脉冲剩余时间
     uint32 m_combatPulseDelay;                          // 战斗脉冲延迟
 
     // 反应状态
     ReactStates m_reactState;                           // 用于AI，不是charmInfo
     // 恢复生命
     void RegenerateHealth();
     // 恢复能量
     void Regenerate(Powers power);
     // 默认移动类型
     MovementGeneratorType m_defaultMovementType;
     // 生成ID
     ObjectGuid::LowType m_spawnId;                      ///< 对于新或临时生物是0，对于保存的生物是lowguid
     // 装备ID
     uint8 m_equipmentId;
     // 原始装备ID（可以是-1）
     int8 m_originalEquipmentId; // can be -1
 
     // 已经呼叫帮助
     bool m_AlreadyCallAssistance;
     // 已经搜索帮助
     bool m_AlreadySearchedAssistance;
     // 恢复生命
     bool m_regenHealth;
     // 恢复能量
     bool m_regenPower;
     // AI锁定
     bool m_AI_locked;
 
     // 近战伤害学校掩码
     SpellSchoolMask m_meleeDamageSchoolMask;
     // 原始条目
     uint32 m_originalEntry;
 
     // 移动视线禁用
     bool m_moveInLineOfSightDisabled;
     // 移动视线严格禁用
     bool m_moveInLineOfSightStrictlyDisabled;
 
     // 家的位置
     Position m_homePosition;
     // 交通工具家的位置
     Position m_transportHomePosition;
 
     // 禁用声誉奖励
     bool DisableReputationReward;
     // 禁用战利品奖励
     bool DisableLootReward;
 
     // 生物模板
     CreatureTemplate const* m_creatureInfo;   // 在难度模式>0时可能与sObjectMgr->GetCreatureTemplate(GetEntry())不同
     // 生物数据
     CreatureData const* m_creatureData;
 
     // 检测距离
     float m_detectionDistance;
     // 战利品模式
     uint16 m_LootMode;  // 掩码，默认LOOT_MODE_DEFAULT，决定哪些战利品可被拾取
 
     // 对战百分比
     float _sparringPct;
 
     // 检查是否因取消生成而不可见
     [[nodiscard]] bool IsInvisibleDueToDespawn() const override;
     // 检查是否始终可以看到对象
     bool CanAlwaysSee(WorldObject const* obj) const override;
     // 检查是否对观察者始终可检测
     bool IsAlwaysDetectableFor(WorldObject const* seer) const override;
 
 private:
     // 强制取消生成
     void ForcedDespawn(uint32 timeMSToDespawn = 0, Seconds forcedRespawnTimer = 0s);
 
     // 检查是否可以周期性呼叫帮助
     [[nodiscard]] bool CanPeriodicallyCallForAssistance() const;
 
     // WaypointMovementGenerator变量
     uint32 m_waypointID;
     uint32 m_path_id;
 
     // 编队变量
     CreatureGroup* m_formation;
     // 触发刚重生
     bool TriggerJustRespawned;
 
     // 在协助其他生物的生物之间共享的计时器
     // 攻击一个生物会延长所有生物的牵引范围
     mutable std::shared_ptr<time_t> m_lastLeashExtensionTime;
 
     // 无法到达的目标
     ObjectGuid m_cannotReachTarget;
     // 无法到达计时器
     uint32 m_cannotReachTimer;
 
     // 法术焦点
     Spell const* _focusSpell;   ///> 在施法期间锁定目标以正确朝向
 
     // 是否缺少非战斗状态下的游泳标志
     bool _isMissingSwimmingFlagOutOfCombat;
 
     // 援助计时器
     uint32 m_assistanceTimer;
 
     // 玩家伤害需求
     uint32 _playerDamageReq;
     // 是否被玩家伤害
     bool _damagedByPlayer;
     // 是否允许战斗移动
     bool _isCombatMovementAllowed;
 };
 
 // 援助延迟事件
 class AssistDelayEvent : public BasicEvent
 {
 public:
     // 构造函数
     AssistDelayEvent(ObjectGuid victim, Creature* owner) : BasicEvent(), m_victim(victim), m_owner(owner) { }
 
     // 执行事件
     bool Execute(uint64 e_time, uint32 p_time) override;
     // 添加助手
     void AddAssistant(ObjectGuid guid) { m_assistants.push_back(guid); }
 
 private:
     // 私有默认构造函数
     AssistDelayEvent();
 
     // 受害者GUID
     ObjectGuid        m_victim;
     // 助手列表
     GuidList          m_assistants;
     // 所有者
     Creature*         m_owner;
 };
 
 // 强制取消生成延迟事件
 class ForcedDespawnDelayEvent : public BasicEvent
 {
 public:
     // 构造函数
     ForcedDespawnDelayEvent(Creature& owner, Seconds respawnTimer) : BasicEvent(), m_owner(owner), m_respawnTimer(respawnTimer) { }
     // 执行事件
     bool Execute(uint64 e_time, uint32 p_time) override;
 
 private:
     // 所有者
     Creature& m_owner;
     // 重生计时器
     Seconds const m_respawnTimer;
 };
 
 // 临时威胁修改事件
 class TemporaryThreatModifierEvent : public BasicEvent
 {
 public:
     // 构造函数
     TemporaryThreatModifierEvent(Creature& owner, ObjectGuid threatVictimGUID, float threatValue) : BasicEvent(), m_owner(owner), m_threatVictimGUID(threatVictimGUID), m_threatValue(threatValue) { }
     // 执行事件
     bool Execute(uint64 e_time, uint32 p_time) override;
 
 private:
     // 所有者
     Creature& m_owner;
     // 威胁目标GUID
     ObjectGuid m_threatVictimGUID;
     // 威胁值
     float m_threatValue;
 };
 
 #endif
