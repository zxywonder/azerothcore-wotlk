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

 #ifndef AZEROTHCORE_GAMEOBJECT_H
 #define AZEROTHCORE_GAMEOBJECT_H
 
 #include "Common.h"
 #include "G3D/Quat.h"
 #include "GameObjectData.h"
 #include "LootMgr.h"
 #include "Object.h"
 #include "SharedDefines.h"
 #include "Unit.h"
 
 class GameObjectAI;
 class Transport;
 class StaticTransport;
 class MotionTransport;
 class OPvPCapturePoint;
 class Unit;
 class GameObjectModel;
 
 struct TransportAnimation;
 
 // 定义事件标志回调函数指针
 typedef void(*goEventFlag)(Player*, GameObject*, Battleground*);
 
 // 使用无序映射容器，性能优于map
 typedef std::unordered_map<uint32, GameObjectTemplate> GameObjectTemplateContainer;
 typedef std::unordered_map<uint32, GameObjectTemplateAddon> GameObjectTemplateAddonContainer;
 
 typedef std::unordered_map<uint32, GameObjectAddon> GameObjectAddonContainer;
 typedef std::vector<uint32> GameObjectQuestItemList;
 typedef std::unordered_map<uint32, GameObjectQuestItemList> GameObjectQuestItemMap;
 
 // 用于存储游戏对象的不同类型值的联合体
 union GameObjectValue
 {
     //11 GAMEOBJECT_TYPE_TRANSPORT
     struct
     {
         uint32 PathProgress;            // 路径进度
         TransportAnimation const* AnimationInfo; // 动画信息
     } Transport;
     //25 GAMEOBJECT_TYPE_FISHINGHOLE
     struct
     {
         uint32 MaxOpens;                // 最大打开次数
     } FishingHole;
     //29 GAMEOBJECT_TYPE_CAPTURE_POINT
     struct
     {
         OPvPCapturePoint* OPvPObj;      // 战场据点对象
     } CapturePoint;
     //33 GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING
     struct
     {
         uint32 Health;                  // 当前生命值
         uint32 MaxHealth;               // 最大生命值
     } Building;
 };
 
 // 游戏对象动作枚举
 enum class GameObjectActions : uint32
 {
     // 名称来自客户端可执行文件      // 注释
     None,                           // -NONE-
     AnimateCustom0,                 // 自定义动画0
     AnimateCustom1,                 // 自定义动画1
     AnimateCustom2,                 // 自定义动画2
     AnimateCustom3,                 // 自定义动画3
     Disturb,                        // 扰动                          // 触发陷阱
     Unlock,                         // 解锁                           // 重置GO_FLAG_LOCKED
     Lock,                           // 锁定                           // 设置GO_FLAG_LOCKED
     Open,                           // 打开                           // 设置GO_STATE_ACTIVE
     OpenAndUnlock,                  // 打开并解锁                    // 设置GO_STATE_ACTIVE并重置GO_FLAG_LOCKED
     Close,                          // 关闭                           // 设置GO_STATE_READY
     ToggleOpen,                     // 切换打开状态
     Destroy,                        // 摧毁                           // 设置GO_STATE_DESTROYED
     Rebuild,                        // 重建                           // 从GO_STATE_DESTROYED恢复
     Creation,                       // 创建
     Despawn,                        // 取消生成
     MakeInert,                      // 设置为无交互                   // 禁用交互
     MakeActive,                     // 设置为可交互                   // 启用交互
     CloseAndLock,                   // 关闭并锁定                     // 设置GO_STATE_READY并设置GO_FLAG_LOCKED
     UseArtKit0,                     // 使用ArtKit0                    // 46904: 121
     UseArtKit1,                     // 使用ArtKit1                    // 36639: 81, 46903: 122
     UseArtKit2,                     // 使用ArtKit2
     UseArtKit3,                     // 使用ArtKit3
     SetTapList,                     // 设置拾取者列表
 };
 
 // 容器状态转换:
 // [GO_NOT_READY]->GO_READY (关闭)->GO_ACTIVATED (打开) ->GO_JUST_DEACTIVATED->GO_READY        -> ...
 // 钓鱼浮标状态转换:
 // GO_NOT_READY  ->GO_READY (关闭)->GO_ACTIVATED (打开) ->GO_JUST_DEACTIVATED-><删除>
 // 关闭的门状态转换:
 // [GO_NOT_READY]->GO_READY (关闭)->GO_ACTIVATED (打开) ->GO_JUST_DEACTIVATED->GO_READY(关闭) -> ...
 // 打开的门状态转换:
 // [GO_NOT_READY]->GO_READY (打开) ->GO_ACTIVATED (关闭)->GO_JUST_DEACTIVATED->GO_READY(打开)  -> ...
 enum LootState
 {
     GO_NOT_READY,
     GO_READY,                                               // 可以就绪但可能已取消生成，此时无法激活直到生成
     GO_ACTIVATED,
     GO_JUST_DEACTIVATED
 };
 
 // 钓鱼浮标捕获时间
 #define FISHING_BOBBER_READY_TIME 5
 
 // 游戏对象类，继承自多个基类
 class GameObject : public WorldObject, public GridObject<GameObject>, public MovableMapObject, public UpdatableMapObject
 {
 public:
     explicit GameObject();        // 构造函数
     ~GameObject() override;       // 析构函数
 
     // 构建更新值
     void BuildValuesUpdate(uint8 updateType, ByteBuffer* data, Player* target) override;
 
     // 添加到世界
     void AddToWorld() override;
     // 从世界中移除
     void RemoveFromWorld() override;
     // 删除前清理
     void CleanupsBeforeDelete(bool finalCleanup = true) override;
 
     // 获取动态标志
     uint32 GetDynamicFlags() const override { return GetUInt32Value(GAMEOBJECT_DYNAMIC); }
     // 替换所有动态标志
     void ReplaceAllDynamicFlags(uint32 flag) override { SetUInt32Value(GAMEOBJECT_DYNAMIC, flag); }
 
     // 创建游戏对象
     virtual bool Create(ObjectGuid::LowType guidlow, uint32 name_id, Map* map, uint32 phaseMask, float x, float y, float z, float ang, G3D::Quat const& rotation, uint32 animprogress, GOState go_state, uint32 artKit = 0);
     // 更新游戏对象
     void Update(uint32 p_time) override;
     // 获取游戏对象模板
     [[nodiscard]] GameObjectTemplate const* GetGOInfo() const { return m_goInfo; }
     // 获取模板附加信息
     [[nodiscard]] GameObjectTemplateAddon const* GetTemplateAddon() const;
     // 获取游戏对象数据
     [[nodiscard]] GameObjectData const* GetGameObjectData() const { return m_goData; }
     // 获取游戏对象值
     [[nodiscard]] GameObjectValue const* GetGOValue() const { return &m_goValue; }
 
     // 判断是否是运输工具
     [[nodiscard]] bool IsTransport() const;
     // 判断是否是可摧毁建筑
     [[nodiscard]] bool IsDestructibleBuilding() const;
 
     // 获取生成ID
     [[nodiscard]] ObjectGuid::LowType GetSpawnId() const { return m_spawnId; }
 
     // 设置局部旋转角度
     void SetLocalRotationAngles(float z_rot, float y_rot, float x_rot);
     // 设置局部旋转
     void SetLocalRotation(G3D::Quat const& rot);
     // 设置运输路径旋转
     void SetTransportPathRotation(float qx, float qy, float qz, float qw);
     // 获取局部旋转
     [[nodiscard]] G3D::Quat const& GetLocalRotation() const { return m_localRotation; }
     // 获取打包的局部旋转
     [[nodiscard]] int64 GetPackedLocalRotation() const { return m_packedRotation; }
     // 获取世界旋转
     [[nodiscard]] G3D::Quat GetWorldRotation() const;
 
     // 覆盖WorldObject函数以实现正确的名称本地化
     [[nodiscard]] std::string const& GetNameForLocaleIdx(LocaleConstant locale_idx) const override;
 
     // 保存到数据库
     void SaveToDB(bool saveAddon = false);
     // 指定参数保存到数据库
     void SaveToDB(uint32 mapid, uint8 spawnMask, uint32 phaseMask, bool saveAddon = false);
     // 从数据库加载
     bool LoadFromDB(ObjectGuid::LowType guid, Map* map) { return LoadGameObjectFromDB(guid, map, false); }
     // 加载游戏对象
     bool LoadGameObjectFromDB(ObjectGuid::LowType guid, Map* map, bool addToMap = true);
     // 从数据库删除
     void DeleteFromDB();
 
     // 设置所有者GUID
     void SetOwnerGUID(ObjectGuid owner)
     {
         // 如果已有不同所有者，则中止
         if (owner && GetOwnerGUID() && GetOwnerGUID() != owner)
         {
             ABORT();
         }
         m_spawnedByDefault = false;                     // 所有有所有者的对象在延迟后取消生成
         SetGuidValue(OBJECT_FIELD_CREATED_BY, owner);
     }
     // 获取所有者GUID
     [[nodiscard]] ObjectGuid GetOwnerGUID() const { return GetGuidValue(OBJECT_FIELD_CREATED_BY); }
     // 获取所有者
     [[nodiscard]] Unit* GetOwner() const;
 
     // 设置法术ID
     void SetSpellId(uint32 id)
     {
         m_spawnedByDefault = false;                     // 所有召唤对象在延迟后取消生成
         m_spellId = id;
     }
     // 获取法术ID
     [[nodiscard]] uint32 GetSpellId() const { return m_spellId;}
 
     // 获取重生时间
     [[nodiscard]] time_t GetRespawnTime() const { return m_respawnTime; }
     // 获取扩展的重生时间
     [[nodiscard]] time_t GetRespawnTimeEx() const;
 
     // 设置重生时间
     void SetRespawnTime(int32 respawn);
     // 设置重生延迟
     void SetRespawnDelay(int32 respawn);
     // 重生
     void Respawn();
     // 判断是否已生成
     [[nodiscard]] bool isSpawned() const
     {
         return m_respawnDelayTime == 0 ||
                (m_respawnTime > 0 && !m_spawnedByDefault) ||
                (m_respawnTime == 0 && m_spawnedByDefault);
     }
     // 判断是否由默认生成
     [[nodiscard]] bool isSpawnedByDefault() const { return m_spawnedByDefault; }
     // 设置是否由默认生成
     void SetSpawnedByDefault(bool b) { m_spawnedByDefault = b; }
     // 获取重生延迟
     [[nodiscard]] uint32 GetRespawnDelay() const { return m_respawnDelayTime; }
     // 刷新
     void Refresh();
     // 取消生成或解除召唤
     void DespawnOrUnsummon(Milliseconds delay = 0ms, Seconds forcedRespawnTime = 0s);
     // 删除
     void Delete();
     // 获取钓鱼战利品
     void GetFishLoot(Loot* fishLoot, Player* lootOwner, bool junk = false);
     // 获取游戏对象类型
     [[nodiscard]] GameobjectTypes GetGoType() const { return GameobjectTypes(GetByteValue(GAMEOBJECT_BYTES_1, 1)); }
     // 设置游戏对象类型
     void SetGoType(GameobjectTypes type) { SetByteValue(GAMEOBJECT_BYTES_1, 1, type); }
     // 获取游戏对象状态
     [[nodiscard]] GOState GetGoState() const { return GOState(GetByteValue(GAMEOBJECT_BYTES_1, 0)); }
     // 设置游戏对象状态
     void SetGoState(GOState state);
     // 获取游戏对象ArtKit
     [[nodiscard]] uint8 GetGoArtKit() const { return GetByteValue(GAMEOBJECT_BYTES_1, 2); }
     // 设置游戏对象ArtKit
     void SetGoArtKit(uint8 artkit);
     // 获取游戏对象动画进度
     [[nodiscard]] uint8 GetGoAnimProgress() const { return GetByteValue(GAMEOBJECT_BYTES_1, 3); }
     // 设置游戏对象动画进度
     void SetGoAnimProgress(uint8 animprogress) { SetByteValue(GAMEOBJECT_BYTES_1, 3, animprogress); }
     // 静态方法设置游戏对象ArtKit
     static void SetGoArtKit(uint8 artkit, GameObject* go, ObjectGuid::LowType lowguid = 0);
 
     // 设置相位掩码
     void SetPhaseMask(uint32 newPhaseMask, bool update) override;
     // 启用/禁用碰撞
     void EnableCollision(bool enable);
 
     // 获取游戏对象标志
     GameObjectFlags GetGameObjectFlags() const { return GameObjectFlags(GetUInt32Value(GAMEOBJECT_FLAGS)); }
     // 检查是否具有指定标志
     bool HasGameObjectFlag(GameObjectFlags flags) const { return HasFlag(GAMEOBJECT_FLAGS, flags) != 0; }
     // 添加标志
     void SetGameObjectFlag(GameObjectFlags flags) { SetFlag(GAMEOBJECT_FLAGS, flags); }
     // 移除标志
     void RemoveGameObjectFlag(GameObjectFlags flags) { RemoveFlag(GAMEOBJECT_FLAGS, flags); }
     // 替换所有标志
     void ReplaceAllGameObjectFlags(GameObjectFlags flags) { SetUInt32Value(GAMEOBJECT_FLAGS, flags); }
 
     // 使用游戏对象
     void Use(Unit* user);
 
     // 获取战利品状态
     [[nodiscard]] LootState getLootState() const { return m_lootState; }
     // 设置战利品状态
     void SetLootState(LootState s, Unit* unit = nullptr);
 
     // 获取战利品模式
     [[nodiscard]] uint16 GetLootMode() const { return m_LootMode; }
     // 检查是否具有指定战利品模式
     [[nodiscard]] bool HasLootMode(uint16 lootMode) const { return m_LootMode & lootMode; }
     // 设置战利品模式
     void SetLootMode(uint16 lootMode) { m_LootMode = lootMode; }
     // 添加战利品模式
     void AddLootMode(uint16 lootMode) { m_LootMode |= lootMode; }
     // 移除战利品模式
     void RemoveLootMode(uint16 lootMode) { m_LootMode &= ~lootMode; }
     // 重置战利品模式
     void ResetLootMode() { m_LootMode = LOOT_MODE_DEFAULT; }
 
     // 添加到技能提升列表
     void AddToSkillupList(ObjectGuid playerGuid);
     // 检查是否在技能提升列表中
     [[nodiscard]] bool IsInSkillupList(ObjectGuid playerGuid) const;
 
     // 添加唯一使用
     void AddUniqueUse(Player* player);
     // 添加使用次数
     void AddUse() { ++m_usetimes; }
 
     // 获取使用次数
     [[nodiscard]] uint32 GetUseCount() const { return m_usetimes; }
     // 获取唯一用户数量
     [[nodiscard]] uint32 GetUniqueUseCount() const { return m_unique_users.size(); }
 
     // 保存重生时间
     void SaveRespawnTime() override { SaveRespawnTime(0); }
     // 指定延迟保存重生时间
     void SaveRespawnTime(uint32 forceDelay);
 
     // 战利品
     Loot        loot;
 
     // 获取战利品接收者
     [[nodiscard]] Player* GetLootRecipient() const;
     // 获取战利品接收者组
     [[nodiscard]] Group* GetLootRecipientGroup() const;
     // 设置战利品接收者
     void SetLootRecipient(Creature* creature);
     // 设置战利品接收者
     void SetLootRecipient(Map* map);
     // 检查玩家是否允许拾取
     bool IsLootAllowedFor(Player const* player) const;
     // 检查是否有战利品接收者
     [[nodiscard]] bool HasLootRecipient() const { return m_lootRecipient || m_lootRecipientGroup; }
     // 组战利品计时器（毫秒）
     uint32 m_groupLootTimer;
     // 抢夺组低GUID
     uint32 lootingGroupLowGUID;
     // 设置战利品生成时间
     void SetLootGenerationTime();
     // 获取战利品生成时间
     [[nodiscard]] uint32 GetLootGenerationTime() const { return m_lootGenerationTime; }
 
     // 获取链接的陷阱
     [[nodiscard]] GameObject* GetLinkedTrap();
     // 设置链接的陷阱
     void SetLinkedTrap(GameObject* linkedTrap) { m_linkedTrap = linkedTrap->GetGUID(); }
 
     // 检查是否有指定任务
     [[nodiscard]] bool hasQuest(uint32 quest_id) const override;
     // 检查是否参与指定任务
     [[nodiscard]] bool hasInvolvedQuest(uint32 quest_id) const override;
     // 激活任务
     bool ActivateToQuest(Player* target) const;
     // 使用门或按钮
     void UseDoorOrButton(uint32 time_to_restore = 0, bool alternative = false, Unit* user = nullptr);
     // 0 = 使用`gameobject`.`spawntimesecs`
     void ResetDoorOrButton();
 
     // 触发链接的游戏对象
     void TriggeringLinkedGameObject(uint32 trapEntry, Unit* target);
 
     // 检查是否永远不可见
     [[nodiscard]] bool IsNeverVisible() const override;
     // 检查是否对观察者始终可见
     bool IsAlwaysVisibleFor(WorldObject const* seer) const override;
     // 检查是否因取消生成而不可见
     [[nodiscard]] bool IsInvisibleDueToDespawn() const override;
 
     // 获取目标等级
     uint8 getLevelForTarget(WorldObject const* target) const override
     {
         if (Unit* owner = GetOwner())
             return owner->getLevelForTarget(target);
 
         return 1;
     }
 
     // 在指定范围内查找钓鱼洞
     GameObject* LookupFishingHoleAround(float range);
 
     // 对目标施放法术
     void CastSpell(Unit* target, uint32 spell);
     // 发送自定义动画
     void SendCustomAnim(uint32 anim);
     // 检查是否在指定范围内
     [[nodiscard]] bool IsInRange(float x, float y, float z, float radius) const;
 
     // 修改生命值
     void ModifyHealth(int32 change, Unit* attackerOrHealer = nullptr, uint32 spellId = 0);
     // 设置允许修改可摧毁建筑状态
     void SetDestructibleBuildingModifyState(bool allow) { m_allowModifyDestructibleBuilding = allow; }
     // 设置可摧毁建筑状态
     void SetDestructibleState(GameObjectDestructibleState state, Player* eventInvoker = nullptr, bool setHealth = false);
     // 获取可摧毁建筑状态
     [[nodiscard]] GameObjectDestructibleState GetDestructibleState() const
     {
         if (HasGameObjectFlag(GO_FLAG_DESTROYED))
             return GO_DESTRUCTIBLE_DESTROYED;
         if (HasGameObjectFlag(GO_FLAG_DAMAGED))
             return GO_DESTRUCTIBLE_DAMAGED;
         return GO_DESTRUCTIBLE_INTACT;
     }
 
     // 事件通知
     void EventInform(uint32 eventId);
 
     // 获取脚本ID
     [[nodiscard]] virtual uint32 GetScriptId() const;
     // 获取AI
     [[nodiscard]] GameObjectAI* AI() const { return m_AI; }
 
     // 获取AI名称
     [[nodiscard]] std::string const& GetAIName() const;
     // 设置显示ID
     void SetDisplayId(uint32 displayid);
     // 获取显示ID
     [[nodiscard]] uint32 GetDisplayId() const { return GetUInt32Value(GAMEOBJECT_DISPLAYID); }
 
     // 模型
     GameObjectModel* m_model;
     // 获取重生位置
     void GetRespawnPosition(float& x, float& y, float& z, float* ori = nullptr) const;
 
     // 设置位置
     void SetPosition(float x, float y, float z, float o);
     // 从位置设置
     void SetPosition(const Position& pos) { SetPosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation()); }
 
     // 检查是否是静态运输工具
     [[nodiscard]] bool IsStaticTransport() const { return GetGOInfo()->type == GAMEOBJECT_TYPE_TRANSPORT; }
     // 检查是否是移动运输工具
     [[nodiscard]] bool IsMotionTransport() const { return GetGOInfo()->type == GAMEOBJECT_TYPE_MO_TRANSPORT; }
 
     // 转换为运输工具
     Transport* ToTransport() { if (GetGOInfo()->type == GAMEOBJECT_TYPE_MO_TRANSPORT || GetGOInfo()->type == GAMEOBJECT_TYPE_TRANSPORT) return reinterpret_cast<Transport*>(this); else return nullptr; }
     // 常量版本转换为运输工具
     [[nodiscard]] Transport const* ToTransport() const { if (GetGOInfo()->type == GAMEOBJECT_TYPE_MO_TRANSPORT || GetGOInfo()->type == GAMEOBJECT_TYPE_TRANSPORT) return reinterpret_cast<Transport const*>(this); else return nullptr; }
 
     // 转换为静态运输工具
     StaticTransport* ToStaticTransport() { if (GetGOInfo()->type == GAMEOBJECT_TYPE_TRANSPORT) return reinterpret_cast<StaticTransport*>(this); else return nullptr; }
     // 常量版本转换为静态运输工具
     [[nodiscard]] StaticTransport const* ToStaticTransport() const { if (GetGOInfo()->type == GAMEOBJECT_TYPE_TRANSPORT) return reinterpret_cast<StaticTransport const*>(this); else return nullptr; }
 
     // 转换为移动运输工具
     MotionTransport* ToMotionTransport() { if (GetGOInfo()->type == GAMEOBJECT_TYPE_MO_TRANSPORT) return reinterpret_cast<MotionTransport*>(this); else return nullptr; }
     // 常量版本转换为移动运输工具
     [[nodiscard]] MotionTransport const* ToMotionTransport() const { if (GetGOInfo()->type == GAMEOBJECT_TYPE_MO_TRANSPORT) return reinterpret_cast<MotionTransport const*>(this); else return nullptr; }
 
     // 获取固定X坐标
     [[nodiscard]] float GetStationaryX() const override { if (GetGOInfo()->type != GAMEOBJECT_TYPE_MO_TRANSPORT) return m_stationaryPosition.GetPositionX(); return GetPositionX(); }
     // 获取固定Y坐标
     [[nodiscard]] float GetStationaryY() const override { if (GetGOInfo()->type != GAMEOBJECT_TYPE_MO_TRANSPORT) return m_stationaryPosition.GetPositionY(); return GetPositionY(); }
     // 获取固定Z坐标
     [[nodiscard]] float GetStationaryZ() const override { if (GetGOInfo()->type != GAMEOBJECT_TYPE_MO_TRANSPORT) return m_stationaryPosition.GetPositionZ(); return GetPositionZ(); }
     // 获取固定朝向
     [[nodiscard]] float GetStationaryO() const override { if (GetGOInfo()->type != GAMEOBJECT_TYPE_MO_TRANSPORT) return m_stationaryPosition.GetOrientation(); return GetOrientation(); }
 
     // 获取交互距离
     [[nodiscard]] float GetInteractionDistance() const;
 
     // 更新模型位置
     void UpdateModelPosition();
 
     // 检查是否在交互距离内
     [[nodiscard]] bool IsAtInteractDistance(Position const& pos, float radius) const;
     // 检查玩家是否在交互距离内
     [[nodiscard]] bool IsAtInteractDistance(Player const* player, SpellInfo const* spell = nullptr) const;
 
     // 检查是否在范围内
     [[nodiscard]] bool IsWithinDistInMap(Player const* player) const;
     using WorldObject::IsWithinDistInMap;
 
     // 获取锁定所需的法术
     [[nodiscard]] SpellInfo const* GetSpellForLock(Player const* player) const;
 
     // 游戏对象到事件标志映射
     static std::unordered_map<int, goEventFlag> gameObjectToEventFlag; // 游戏对象 -> 事件标志
 
     // 验证游戏对象类型
     [[nodiscard]] bool ValidateGameobjectType() const;
     // 检查是否是实例游戏对象
     [[nodiscard]] bool IsInstanceGameobject() const;
     // 将游戏对象状态转换为整数
     [[nodiscard]] uint8 GameobjectStateToInt(GOState* state) const;
 
     // 检查是否允许保存到数据库
     [[nodiscard]] bool IsAllowedToSaveToDB() const { return m_saveStateOnDb; };
 
     // 允许或禁止保存状态到数据库
     void AllowSaveToDB(bool enable) { m_saveStateOnDb = enable; };
 
     // 保存状态到数据库
     void SaveStateToDB();
 
     // 获取调试信息
     std::string GetDebugInfo() const override;
 
     // 检查是否需要更新
     bool IsUpdateNeeded() override;
 protected:
     // 初始化AI
     bool AIM_Initialize();
     // 创建模型
     GameObjectModel* CreateModel();
     // 更新模型
     void UpdateModel();                                 // 如果displayId更改则更新模型
     // 法术ID
     uint32      m_spellId;
     // 下次重生时间（秒）
     time_t      m_respawnTime;
     // 重生延迟时间（秒）
     uint32      m_respawnDelayTime;                     // 如果为0则当前GO状态不依赖于定时器
     // 取消生成延迟
     uint32      m_despawnDelay;
     // 重置生成时间
     Seconds     m_despawnRespawnTime;
     // 补货时间
     Seconds     m_restockTime;
     // 战利品状态
     LootState   m_lootState;
     // 是否由默认生成
     bool        m_spawnedByDefault;
     // 冷却时间
     uint32      m_cooldownTime;
     // 技能提升列表
     std::unordered_map<ObjectGuid, int32> m_SkillupList;
 
     // 仪式所有者GUID
     ObjectGuid m_ritualOwnerGUID;
     // 独特用户集合
     GuidSet m_unique_users;
     // 使用次数
     uint32 m_usetimes;
 
     // 座椅槽和用户映射
     typedef std::map<uint32, ObjectGuid> ChairSlotAndUser;
     ChairSlotAndUser ChairListSlots;
 
     // 生成ID
     ObjectGuid::LowType m_spawnId;
     // 游戏对象模板
     GameObjectTemplate const* m_goInfo;
     // 游戏对象数据
     GameObjectData const* m_goData;
     // 游戏对象值
     GameObjectValue m_goValue;
     // 允许修改可摧毁建筑
     bool m_allowModifyDestructibleBuilding;
 
     // 打包的旋转
     int64 m_packedRotation;
     // 局部旋转
     G3D::Quat m_localRotation;
     // 固定位置
     Position m_stationaryPosition;
 
     // 战利品接收者
     ObjectGuid m_lootRecipient;
     // 战利品接收者组低GUID
     ObjectGuid::LowType m_lootRecipientGroup;
     // 战利品模式
     uint16 m_LootMode;
     // 战利品生成时间
     uint32 m_lootGenerationTime;
 
     // 链接的陷阱
     ObjectGuid m_linkedTrap;
 
     // 战利品状态单位GUID
     ObjectGuid _lootStateUnitGUID;
 
 private:
     // 检查仪式列表
     void CheckRitualList();
     // 清空仪式列表
     void ClearRitualList();
     // 从所有者移除
     void RemoveFromOwner();
     // 切换门或按钮
     void SwitchDoorOrButton(bool activate, bool alternative = false);
     // 更新打包的旋转
     void UpdatePackedRotation();
 
     // 对象距离/大小 - 覆盖自Object::_IsWithinDist
     bool _IsWithinDist(WorldObject const* obj, float dist2compare, bool /*is3D*/, bool /*useBoundingRadius = true*/) const override
     {
         // 以下检查计算3D距离
         dist2compare += obj->GetObjectSize();
         return IsInRange(obj->GetPositionX(), obj->GetPositionY(), obj->GetPositionZ(), dist2compare);
     }
     // AI
     GameObjectAI* m_AI;
 
     // 是否允许保存状态到数据库
     bool m_saveStateOnDb = false;
 };
 #endif
