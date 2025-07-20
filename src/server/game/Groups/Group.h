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

 #ifndef AZEROTHCORE_GROUP_H
 #define AZEROTHCORE_GROUP_H
 
 #include "DBCEnums.h"
 #include "DataMap.h"
 #include "GroupRefMgr.h"
 #include "LootMgr.h"
 #include "QueryResult.h"
 #include "SharedDefines.h"
 #include <functional>
 
 class Battlefield;
 class Battleground;
 class Creature;
 class GroupReference;
 class InstanceSave;
 class Map;
 class Player;
 class Unit;
 class WorldObject;
 class WorldPacket;
 class WorldSession;
 
 struct MapEntry;
 
 #define MAXGROUPSIZE 5
 #define MAXRAIDSIZE 40
 #define MAX_RAID_SUBGROUPS MAXRAIDSIZE/MAXGROUPSIZE
 #define TARGETICONCOUNT 8
 
 // RollVote 枚举定义了玩家在ROLL点时的投票类型
 enum RollVote : uint8
 {
     PASS              = 0, // 放弃
     NEED              = 1, // 需要
     GREED             = 2, // 贪婪
     DISENCHANT        = 3, // 分解
     NOT_EMITED_YET    = 4, // 尚未发出
     NOT_VALID         = 5  // 无效
 };
 
 // GroupMemberOnlineStatus 枚举定义了组成员的在线状态标志
 enum GroupMemberOnlineStatus
 {
     MEMBER_STATUS_OFFLINE   = 0x0000, // 离线
     MEMBER_STATUS_ONLINE    = 0x0001, // 在线，对应 Lua_UnitIsConnected
     MEMBER_STATUS_PVP       = 0x0002, // 处于PVP状态，对应 Lua_UnitIsPVP
     MEMBER_STATUS_DEAD      = 0x0004, // 死亡，对应 Lua_UnitIsDead
     MEMBER_STATUS_GHOST     = 0x0008, // 鬼魂状态，对应 Lua_UnitIsGhost
     MEMBER_STATUS_PVP_FFA   = 0x0010, // 自由PVP，对应 Lua_UnitIsPVPFreeForAll
     MEMBER_STATUS_UNK3      = 0x0020, // 未知用途，用于 Lua_GetPlayerMapPosition/Lua_GetBattlefieldFlagPosition
     MEMBER_STATUS_AFK       = 0x0040, // 挂机，对应 Lua_UnitIsAFK
     MEMBER_STATUS_DND       = 0x0080, // 忙碌，对应 Lua_UnitIsDND
 };
 
 // GroupMemberFlags 枚举定义了组成员的标志
 enum GroupMemberFlags
 {
     MEMBER_FLAG_ASSISTANT   = 0x01, // 助理
     MEMBER_FLAG_MAINTANK    = 0x02, // 主坦克
     MEMBER_FLAG_MAINASSIST  = 0x04, // 主助手
 };
 
 // GroupMemberAssignment 枚举定义了组成员的角色分配
 enum GroupMemberAssignment
 {
     GROUP_ASSIGN_MAINTANK   = 0, // 主坦克
     GROUP_ASSIGN_MAINASSIST = 1, // 主助手
 };
 
 // GroupType 枚举定义了组的类型
 enum GroupType
 {
     GROUPTYPE_NORMAL         = 0x00, // 普通组
     GROUPTYPE_BG             = 0x01, // 战场组
     GROUPTYPE_RAID           = 0x02, // 团队组
     GROUPTYPE_BGRAID         = GROUPTYPE_BG | GROUPTYPE_RAID, // 战场团队组（掩码）
     GROUPTYPE_LFG_RESTRICTED = 0x04, // 有限制的地下城查找器组（脚本使用 Script_HasLFGRestrictions()）
     GROUPTYPE_LFG            = 0x08, // 地下城查找器组
 };
 
 // GroupUpdateFlags 枚举定义了组更新时使用的标志
 enum GroupUpdateFlags
 {
     GROUP_UPDATE_FLAG_NONE              = 0x00000000, // 无更新
     GROUP_UPDATE_FLAG_STATUS            = 0x00000001, // uint16，状态标志
     GROUP_UPDATE_FLAG_CUR_HP            = 0x00000002, // uint32，当前生命值
     GROUP_UPDATE_FLAG_MAX_HP            = 0x00000004, // uint32，最大生命值
     GROUP_UPDATE_FLAG_POWER_TYPE        = 0x00000008, // uint8，能量类型
     GROUP_UPDATE_FLAG_CUR_POWER         = 0x00000010, // uint16，当前能量
     GROUP_UPDATE_FLAG_MAX_POWER         = 0x00000020, // uint16，最大能量
     GROUP_UPDATE_FLAG_LEVEL             = 0x00000040, // uint16，等级
     GROUP_UPDATE_FLAG_ZONE              = 0x00000080, // uint16，区域
     GROUP_UPDATE_FLAG_POSITION          = 0x00000100, // uint16, uint16，坐标位置
     GROUP_UPDATE_FLAG_AURAS             = 0x00000200, // uint64 掩码，每个设置的位对应 uint32 法术ID + uint8 未知
     GROUP_UPDATE_FLAG_PET_GUID          = 0x00000400, // uint64 宠物GUID
     GROUP_UPDATE_FLAG_PET_NAME          = 0x00000800, // 宠物名称，以空字符结尾的字符串
     GROUP_UPDATE_FLAG_PET_MODEL_ID      = 0x00001000, // uint16 模型ID
     GROUP_UPDATE_FLAG_PET_CUR_HP        = 0x00002000, // uint32 宠物当前生命值
     GROUP_UPDATE_FLAG_PET_MAX_HP        = 0x00004000, // uint32 宠物最大生命值
     GROUP_UPDATE_FLAG_PET_POWER_TYPE    = 0x00008000, // uint8 宠物能量类型
     GROUP_UPDATE_FLAG_PET_CUR_POWER     = 0x00010000, // uint16 宠物当前能量
     GROUP_UPDATE_FLAG_PET_MAX_POWER     = 0x00020000, // uint16 宠物最大能量
     GROUP_UPDATE_FLAG_PET_AURAS         = 0x00040000, // uint64 掩码，每个设置的位对应 uint32 法术ID + uint8 未知（宠物光环）
     GROUP_UPDATE_FLAG_VEHICLE_SEAT      = 0x00080000, // uint32 载具座位ID（来自 VehicleSeat.dbc 的索引）
     GROUP_UPDATE_PET                    = 0x0007FC00, // 所有与宠物相关的标志
     GROUP_UPDATE_FULL                   = 0x0007FFFF, // 所有已知标志
 };
 
 // lfgGroupFlags 枚举定义了地下城查找器组的标志
 enum lfgGroupFlags
 {
     GROUP_LFG_FLAG_APPLY_RANDOM_BUFF        = 0x001, // 应用随机BUFF
     GROUP_LFG_FLAG_IS_RANDOM_INSTANCE       = 0x002, // 是随机副本
     GROUP_LFG_FLAG_IS_HEROIC                = 0x004  // 是英雄难度
 };
 
 // DifficultyPreventionChangeType 枚举定义了难度更改限制的类型
 enum DifficultyPreventionChangeType
 {
     DIFFICULTY_PREVENTION_CHANGE_NONE                   = 0, // 无更改
     DIFFICULTY_PREVENTION_CHANGE_RECENTLY_CHANGED       = 1, // 最近更改过
     DIFFICULTY_PREVENTION_CHANGE_BOSS_KILLED            = 2  // 首领已击杀
 };
 
 // 定义GROUP_UPDATE_FLAGS_COUNT为20
 #define GROUP_UPDATE_FLAGS_COUNT          20
 // 组更新标志对应的更新数据长度
 static const uint8 GroupUpdateLength[GROUP_UPDATE_FLAGS_COUNT] = { 0, 2, 2, 2, 1, 2, 2, 2, 2, 4, 8, 8, 1, 2, 2, 2, 1, 2, 2, 8};
 
 // Roll 类用于处理ROLL点逻辑
 class Roll : public LootValidatorRef
 {
 public:
     // 构造函数，初始化ROLL数据
     Roll(ObjectGuid _guid, LootItem const& li);
     // 析构函数
     ~Roll();
     // 设置ROLL对应的战利品
     void setLoot(Loot* pLoot);
     // 获取ROLL对应的战利品
     Loot* getLoot();
     // 建立目标对象的链接
     void targetObjectBuildLink();

     ObjectGuid itemGUID; // 战利品GUID
     uint32 itemid; // 物品ID
     int32  itemRandomPropId; // 随机属性ID
     uint32 itemRandomSuffix; // 随机后缀
     uint8 itemCount; // 物品数量
     typedef std::map<ObjectGuid, RollVote> PlayerVote;
     PlayerVote playerVote; // 存储玩家投票（位置对应组中玩家的位置）
     uint8 totalPlayersRolling; // 总共ROLL点玩家数
     uint8 totalNeed; // 需要人数
     uint8 totalGreed; // 贪婪人数
     uint8 totalPass; // 放弃人数
     uint8 itemSlot; // 物品槽位
     uint8 rollVoteMask; // ROLL投票掩码
 };
 
 // Group 类用于管理玩家组
 class Group
 {
 public:
     // MemberSlot 结构体用于存储组成员信息
     struct MemberSlot
     {
         ObjectGuid  guid; // 成员GUID
         std::string name; // 成员名称
         uint8       group; // 所在子组
         uint8       flags; // 标志
         uint8       roles; // 角色
     };
     typedef std::list<MemberSlot> MemberSlotList;
     typedef MemberSlotList::const_iterator member_citerator;
 
 protected:
     typedef MemberSlotList::iterator member_witerator;
     typedef std::set<Player*> InvitesList;
 
     typedef std::vector<Roll*> Rolls;
 
 public:
     // 构造函数
     Group();
     // 析构函数
     ~Group();
 
     // 组操作方法
     bool   Create(Player* leader); // 创建组
     bool   LoadGroupFromDB(Field* field); // 从数据库加载组信息
     void   LoadMemberFromDB(ObjectGuid::LowType guidLow, uint8 memberFlags, uint8 subgroup, uint8 roles); // 从数据库加载成员信息
     bool   AddInvite(Player* player); // 添加邀请
     void   RemoveInvite(Player* player); // 移除邀请
     void   RemoveAllInvites(); // 移除所有邀请
     bool   AddLeaderInvite(Player* player); // 添加领导邀请
     bool   AddMember(Player* player); // 添加成员
     bool   RemoveMember(ObjectGuid guid, const RemoveMethod& method = GROUP_REMOVEMETHOD_DEFAULT, ObjectGuid kicker = ObjectGuid::Empty, const char* reason = nullptr); // 移除成员
     void   ChangeLeader(ObjectGuid guid); // 更换组长
     void   SetLootMethod(LootMethod method); // 设置拾取方式
     void   SetLooterGuid(ObjectGuid guid); // 设置拾取者GUID
     void   SetMasterLooterGuid(ObjectGuid guid); // 设置主拾取者GUID
     void   UpdateLooterGuid(WorldObject* pLootedObject, bool ifneed = false); // 更新拾取者GUID
     void   SetLootThreshold(ItemQualities threshold); // 设置拾取品质阈值
     void   Disband(bool hideDestroy = false); // 解散组
     void   SetLfgRoles(ObjectGuid guid, const uint8 roles); // 设置LFG角色
 
     // 属性访问方法
     bool IsFull() const; // 是否已满
     bool isLFGGroup(bool restricted = false)  const; // 是否是LFG组
     bool isRaidGroup() const; // 是否是团队组
     bool isBFGroup()   const; // 是否是战场组
     bool isBGGroup()   const;
     bool IsCreated()   const; // 是否已创建
     GroupType GetGroupType() const; // 获取组类型
     ObjectGuid GetLeaderGUID() const; // 获取组长GUID
     Player* GetLeader(); // 获取组长对象
     ObjectGuid GetGUID() const; // 获取组GUID
     const char* GetLeaderName() const; // 获取组长名称
     LootMethod GetLootMethod() const; // 获取拾取方式
     ObjectGuid GetLooterGuid() const; // 获取拾取者GUID
     ObjectGuid GetMasterLooterGuid() const; // 获取主拾取者GUID
     ItemQualities GetLootThreshold() const; // 获取拾取品质阈值
 
     // 成员操作方法
     bool IsMember(ObjectGuid guid) const; // 是否是组成员
     bool IsLeader(ObjectGuid guid) const; // 是否是组长
     ObjectGuid GetMemberGUID(const std::string& name); // 通过名称获取成员GUID
     bool IsAssistant(ObjectGuid guid) const; // 是否是助理
 
     Player* GetInvited(ObjectGuid guid) const; // 获取被邀请的玩家
     Player* GetInvited(const std::string& name) const; // 通过名称获取被邀请的玩家
 
     bool SameSubGroup(ObjectGuid guid1, ObjectGuid guid2) const; // 判断两个玩家是否在同一子组
     bool SameSubGroup(ObjectGuid guid1, MemberSlot const* slot2) const; // 判断玩家和成员槽是否在同一子组
     bool SameSubGroup(Player const* member1, Player const* member2) const; // 判断两个玩家对象是否在同一子组
     bool HasFreeSlotSubGroup(uint8 subgroup) const; // 判断子组是否有空位
 
     MemberSlotList const& GetMemberSlots() const { return m_memberSlots; } // 获取成员槽列表
     GroupReference* GetFirstMember() { return m_memberMgr.getFirst(); } // 获取第一个成员引用
     GroupReference const* GetFirstMember() const { return m_memberMgr.getFirst(); } // 获取第一个成员引用（常量）
     uint32 GetMembersCount() const { return m_memberSlots.size(); } // 获取成员数量
     uint32 GetInviteeCount() const { return m_invitees.size(); } // 获取邀请数量
 
     uint8 GetMemberGroup(ObjectGuid guid) const; // 获取成员所在子组
 
     void ConvertToLFG(bool restricted = true); // 转换为LFG组
     bool CheckLevelForRaid(); // 检查团队等级要求
     void ConvertToRaid(); // 转换为团队组
 
     void SetBattlegroundGroup(Battleground* bg); // 设置战场组
     void SetBattlefieldGroup(Battlefield* bf); // 设置战场组
     GroupJoinBattlegroundResult CanJoinBattlegroundQueue(Battleground const* bgTemplate, BattlegroundQueueTypeId bgQueueTypeId, uint32 MinPlayerCount, uint32 MaxPlayerCount, bool isRated, uint32 arenaSlot); // 判断是否可以加入战场队列
 
     void ChangeMembersGroup(ObjectGuid guid, uint8 group); // 更改成员所在子组
     void SetTargetIcon(uint8 id, ObjectGuid whoGuid, ObjectGuid targetGuid); // 设置目标图标
     void SetGroupMemberFlag(ObjectGuid guid, bool apply, GroupMemberFlags flag); // 设置组成员标志
     void RemoveUniqueGroupMemberFlag(GroupMemberFlags flag); // 移除唯一的组成员标志
 
     Difficulty GetDifficulty(bool isRaid) const; // 获取难度
     Difficulty GetDungeonDifficulty() const; // 获取副本难度
     Difficulty GetRaidDifficulty() const; // 获取团队副本难度
     void SetDungeonDifficulty(Difficulty difficulty); // 设置副本难度
     void SetRaidDifficulty(Difficulty difficulty); // 设置团队副本难度
     uint16 InInstance(); // 判断是否在实例中
     void ResetInstances(uint8 method, bool isRaid, Player* leader); // 重置所有实例
 
     // -no description-
     //void SendInit(WorldSession* session);
     void SendTargetIconList(WorldSession* session); // 发送目标图标列表
     void SendUpdate(); // 发送组更新信息
     void SendUpdateToPlayer(ObjectGuid playerGUID, MemberSlot* slot = nullptr); // 发送组更新信息给指定玩家
     void UpdatePlayerOutOfRange(Player* player); // 更新掉线玩家状态
     // ignore: 忽略的玩家GUID
     void BroadcastPacket(WorldPacket const* packet, bool ignorePlayersInBGRaid, int group = -1, ObjectGuid ignore = ObjectGuid::Empty); // 广播数据包
     void BroadcastReadyCheck(WorldPacket const* packet); // 广播准备检查
     void OfflineReadyCheck(); // 离线玩家准备检查
 
     /*********************************************************/
     /***                   LOOT SYSTEM                     ***/
     /*********************************************************/
 
     bool isRollLootActive() const; // 是否有ROLL点拾取活动
     void SendLootStartRoll(uint32 CountDown, uint32 mapid, const Roll& r); // 发送开始ROLL点
     void SendLootStartRollToPlayer(uint32 countDown, uint32 mapId, Player* p, bool canNeed, Roll const& r); // 发送开始ROLL点给指定玩家
     void SendLootRoll(ObjectGuid SourceGuid, ObjectGuid TargetGuid, uint8 RollNumber, uint8 RollType, const Roll& r, bool autoPass = false); // 发送ROLL点结果
     void SendLootRollWon(ObjectGuid SourceGuid, ObjectGuid TargetGuid, uint8 RollNumber, uint8 RollType, const Roll& r); // 发送ROLL点获胜结果
     void SendLootAllPassed(Roll const& roll); // 发送所有玩家放弃
     void SendLooter(Creature* creature, Player* pLooter); // 发送拾取者信息
     void GroupLoot(Loot* loot, WorldObject* pLootedObject); // 组拾取
     void NeedBeforeGreed(Loot* loot, WorldObject* pLootedObject); // 需要在贪婪前
     void MasterLoot(Loot* loot, WorldObject* pLootedObject); // 主拾取
     Rolls::iterator GetRoll(ObjectGuid Guid); // 获取ROLL对象
     void CountTheRoll(Rolls::iterator roll, Map* allowedMap); // 计算ROLL结果
     bool CountRollVote(ObjectGuid playerGUID, ObjectGuid Guid, uint8 Choise); // 记录投票
     void EndRoll(Loot* loot, Map* allowedMap); // 结束ROLL
 
     // 与分解ROLL相关
     void ResetMaxEnchantingLevel(); // 重置最大附魔等级
 
     void LinkMember(GroupReference* pRef); // 链接成员
 
     // FG: evil hacks
     void BroadcastGroupUpdate(void); // 广播组更新
 
     // LFG
     void AddLfgBuffFlag() { m_lfgGroupFlags |= GROUP_LFG_FLAG_APPLY_RANDOM_BUFF; } // 添加随机BUFF标志
     void AddLfgRandomInstanceFlag() { m_lfgGroupFlags |= GROUP_LFG_FLAG_IS_RANDOM_INSTANCE; } // 添加随机副本标志
     void AddLfgHeroicFlag() { m_lfgGroupFlags |= GROUP_LFG_FLAG_IS_HEROIC; } // 添加英雄难度标志
     bool IsLfgWithBuff() const { return isLFGGroup() && (m_lfgGroupFlags & GROUP_LFG_FLAG_APPLY_RANDOM_BUFF); } // 是否带有随机BUFF的LFG组
     bool IsLfgRandomInstance() const { return isLFGGroup() && (m_lfgGroupFlags & GROUP_LFG_FLAG_IS_RANDOM_INSTANCE); } // 是否是随机副本LFG组
     bool IsLfgHeroic() const { return isLFGGroup() && (m_lfgGroupFlags & GROUP_LFG_FLAG_IS_HEROIC); } // 是否是英雄难度LFG组
 
     // 难度更改
     uint32 GetDifficultyChangePreventionTime() const; // 获取难度更改限制时间
     DifficultyPreventionChangeType GetDifficultyChangePreventionReason() const { return _difficultyChangePreventionType; } // 获取难度更改限制原因
     void SetDifficultyChangePrevention(DifficultyPreventionChangeType type); // 设置难度更改限制
     void DoForAllMembers(std::function<void(Player*)> const& worker); // 对所有成员执行操作
 
     DataMap CustomData; // 自定义数据
 
 protected:
     void _homebindIfInstance(Player* player); // 如果在副本中则设置回城
     void _cancelHomebindIfInstance(Player* player); // 如果在副本中则取消回城
 
     void _initRaidSubGroupsCounter(); // 初始化团队子组计数器
     member_citerator _getMemberCSlot(ObjectGuid Guid) const; // 获取成员槽（常量）
     member_witerator _getMemberWSlot(ObjectGuid Guid); // 获取成员槽（可写）
     void SubGroupCounterIncrease(uint8 subgroup); // 子组计数增加
     void SubGroupCounterDecrease(uint8 subgroup); // 子组计数减少
     void ToggleGroupMemberFlag(member_witerator slot, uint8 flag, bool apply); // 切换组成员标志
 
     MemberSlotList      m_memberSlots; // 成员槽列表
     GroupRefMgr     m_memberMgr; // 组成员管理器
     InvitesList         m_invitees; // 邀请列表
     ObjectGuid          m_leaderGuid; // 组长GUID
     std::string         m_leaderName; // 组长名称
     GroupType           m_groupType; // 组类型
     Difficulty          m_dungeonDifficulty; // 副本难度
     Difficulty          m_raidDifficulty; // 团队副本难度
     Battlefield*        m_bfGroup; // 战场组
     Battleground*       m_bgGroup; // 战场组
     ObjectGuid          m_targetIcons[TARGETICONCOUNT]; // 目标图标
     LootMethod          m_lootMethod; // 拾取方式
     ItemQualities       m_lootThreshold; // 拾取品质阈值
     ObjectGuid          m_looterGuid; // 拾取者GUID
     ObjectGuid          m_masterLooterGuid; // 主拾取者GUID
     Rolls               RollId; // ROLL列表
     uint8*              m_subGroupsCounts; // 子组计数
     ObjectGuid          m_guid; // 组GUID
     uint32              m_counter; // 用于 SMSG_GROUP_LIST
     uint32              m_maxEnchantingLevel; // 最大附魔等级
     uint8               m_lfgGroupFlags; // LFG组标志
 
     // Xinef: 难度更改限制
     uint32 _difficultyChangePreventionTime; // 难度更改限制时间
     DifficultyPreventionChangeType _difficultyChangePreventionType; // 难度更改限制类型
 };
 #endif
