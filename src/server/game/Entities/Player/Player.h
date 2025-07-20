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

#ifndef _PLAYER_H
#define _PLAYER_H

#include "ArenaTeam.h"
#include "Battleground.h"
#include "CharmInfo.h"
#include "CharacterCache.h"
#include "CinematicMgr.h"
#include "DBCStores.h"
#include "DatabaseEnvFwd.h"
#include "EnumFlag.h"
#include "GroupReference.h"
#include "InstanceSaveMgr.h"
#include "Item.h"
#include "MapReference.h"
#include "ObjectMgr.h"
#include "Optional.h"
#include "PetDefines.h"
#include "PlayerSettings.h"
#include "PlayerTaxi.h"
#include "QuestDef.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "TradeData.h"
#include "Unit.h"
#include "WorldSession.h"
#include <string>
#include <vector>

struct CreatureTemplate;
struct Mail;
struct TrainerSpell;
struct VendorItem;

class AchievementMgr;
class ReputationMgr;
class Channel;
class CharacterCreateInfo;
class Creature;
class DynamicObject;
class Group;
class Guild;
class OutdoorPvP;
class Pet;
class PlayerMenu;
class PlayerSocial;
class SpellCastTargets;
class UpdateMask;

typedef std::deque<Mail*> PlayerMails;
typedef void(*bgZoneRef)(Battleground*, WorldPackets::WorldState::InitWorldStates&);

#define PLAYER_MAX_SKILLS           127
#define PLAYER_MAX_DAILY_QUESTS     25
#define PLAYER_EXPLORED_ZONES_SIZE  128

// corpse reclaim times
#define DEATH_EXPIRE_STEP (5*MINUTE)
#define MAX_DEATH_COUNT 3

#define PLAYER_SKILL_INDEX(x)       (PLAYER_SKILL_INFO_1_1 + ((x)*3))
#define PLAYER_SKILL_VALUE_INDEX(x) (PLAYER_SKILL_INDEX(x)+1)
#define PLAYER_SKILL_BONUS_INDEX(x) (PLAYER_SKILL_INDEX(x)+2)

#define SKILL_VALUE(x)         PAIR32_LOPART(x)
#define SKILL_MAX(x)           PAIR32_HIPART(x)
#define MAKE_SKILL_VALUE(v, m) MAKE_PAIR32(v, m)

#define SKILL_TEMP_BONUS(x)    int16(PAIR32_LOPART(x))
#define SKILL_PERM_BONUS(x)    int16(PAIR32_HIPART(x))
#define MAKE_SKILL_BONUS(t, p) MAKE_PAIR32(t, p)

// 注意：SPELLMOD_* 值实际上是光环类型
enum SpellModType
{
    SPELLMOD_FLAT = 107,                            // SPELL_AURA_ADD_FLAT_MODIFIER，添加固定值修正
    SPELLMOD_PCT = 108                             // SPELL_AURA_ADD_PCT_MODIFIER，添加百分比修正
};

// 2的n次幂值，Player::m_isunderwater 是一个位掩码。这些是Trinity内部值，不会发送给任何客户端
enum PlayerUnderwaterState
{
    UNDERWATER_NONE = 0x00,             // 不在任何水下环境中
    UNDERWATER_INWATER = 0x01,             // 地形类型是水，玩家受其影响
    UNDERWATER_INLAVA = 0x02,             // 地形类型是熔岩，玩家受其影响
    UNDERWATER_INSLIME = 0x04,             // 原注释有误，应为地形类型是黏液，玩家受其影响
    UNDERWATER_INDARKWATER = 0x08,             // 地形类型是黑水，玩家受其影响
    UNDERWATER_EXIST_TIMERS = 0x10              // 存在水下计时器
};

// 购买银行插槽的结果枚举
enum BuyBankSlotResult
{
    ERR_BANKSLOT_FAILED_TOO_MANY = 0,                    // 银行插槽已满，购买失败
    ERR_BANKSLOT_INSUFFICIENT_FUNDS = 1,                    // 资金不足，无法购买
    ERR_BANKSLOT_NOTBANKER = 2,                    // 不在银行管理员附近，无法购买
    ERR_BANKSLOT_OK = 3                     // 购买成功
};

// 玩家法术状态枚举
enum PlayerSpellState
{
    PLAYERSPELL_UNCHANGED = 0,                              // 法术状态未改变
    PLAYERSPELL_CHANGED = 1,                              // 法术状态已改变
    PLAYERSPELL_NEW = 2,                              // 新学习的法术
    PLAYERSPELL_REMOVED = 3,                              // 已移除的法术
    PLAYERSPELL_TEMPORARY = 4                               // 临时法术
};

// 玩家法术结构体
struct PlayerSpell
{
    PlayerSpellState State : 7; // UPPER CASE TO CAUSE CONSOLE ERRORS (CHECK EVERY USAGE)! 法术状态
    bool Active : 1; // UPPER CASE TO CAUSE CONSOLE ERRORS (CHECK EVERY USAGE)! 较低等级的法术不可用，但已学习
    uint8 specMask : 8;  // 专精掩码，用于表示该法术属于哪些专精
    // 判断该法术是否属于指定的专精
    bool IsInSpec(uint8 spec) { return (specMask & (1 << spec)); }
};

// 玩家天赋结构体
struct PlayerTalent
{
    PlayerSpellState State : 8; // UPPER CASE TO CAUSE CONSOLE ERRORS (CHECK EVERY USAGE)! 天赋状态
    uint8 specMask : 8;  // 专精掩码，用于表示该天赋属于哪些专精
    uint32 talentID;            // 天赋ID
    bool inSpellBook;           // 是否在法术书中
    // 判断该天赋是否属于指定的专精
    bool IsInSpec(uint8 spec) { return (specMask & (1 << spec)); }
};

// 天赋树枚举，对应天赋标签页
enum TalentTree // talent tabs
{
    TALENT_TREE_WARRIOR_ARMS = 161,                         // 战士 - 武器天赋树
    TALENT_TREE_WARRIOR_FURY = 164,                         // 战士 - 狂怒天赋树
    TALENT_TREE_WARRIOR_PROTECTION = 163,                   // 战士 - 防护天赋树
    TALENT_TREE_PALADIN_HOLY = 382,                         // 圣骑士 - 神圣天赋树
    TALENT_TREE_PALADIN_PROTECTION = 383,                   // 圣骑士 - 防护天赋树
    TALENT_TREE_PALADIN_RETRIBUTION = 381,                  // 圣骑士 - 惩戒天赋树
    TALENT_TREE_HUNTER_BEAST_MASTERY = 361,                 // 猎人 - 野兽控制天赋树
    TALENT_TREE_HUNTER_MARKSMANSHIP = 363,                  // 猎人 - 射击天赋树
    TALENT_TREE_HUNTER_SURVIVAL = 362,                      // 猎人 - 生存天赋树
    TALENT_TREE_ROGUE_ASSASSINATION = 182,                  // 盗贼 - 刺杀天赋树
    TALENT_TREE_ROGUE_COMBAT = 181,                         // 盗贼 - 战斗天赋树
    TALENT_TREE_ROGUE_SUBTLETY = 183,                       // 盗贼 - 敏锐天赋树
    TALENT_TREE_PRIEST_DISCIPLINE = 201,                    // 牧师 - 戒律天赋树
    TALENT_TREE_PRIEST_HOLY = 202,                          // 牧师 - 神圣天赋树
    TALENT_TREE_PRIEST_SHADOW = 203,                        // 牧师 - 暗影天赋树
    TALENT_TREE_DEATH_KNIGHT_BLOOD = 398,                   // 死亡骑士 - 鲜血天赋树
    TALENT_TREE_DEATH_KNIGHT_FROST = 399,                   // 死亡骑士 - 冰霜天赋树
    TALENT_TREE_DEATH_KNIGHT_UNHOLY = 400,                  // 死亡骑士 - 邪恶天赋树
    TALENT_TREE_SHAMAN_ELEMENTAL = 261,                     // 萨满祭司 - 元素天赋树
    TALENT_TREE_SHAMAN_ENHANCEMENT = 263,                   // 萨满祭司 - 增强天赋树
    TALENT_TREE_SHAMAN_RESTORATION = 262,                   // 萨满祭司 - 恢复天赋树
    TALENT_TREE_MAGE_ARCANE = 81,                           // 法师 - 奥术天赋树
    TALENT_TREE_MAGE_FIRE = 41,                             // 法师 - 火焰天赋树
    TALENT_TREE_MAGE_FROST = 61,                            // 法师 - 冰霜天赋树
    TALENT_TREE_WARLOCK_AFFLICTION = 302,                   // 术士 - 痛苦天赋树
    TALENT_TREE_WARLOCK_DEMONOLOGY = 303,                   // 术士 - 恶魔学识天赋树
    TALENT_TREE_WARLOCK_DESTRUCTION = 301,                  // 术士 - 毁灭天赋树
    TALENT_TREE_DRUID_BALANCE = 283,                        // 德鲁伊 - 平衡天赋树
    TALENT_TREE_DRUID_FERAL_COMBAT = 281,                   // 德鲁伊 - 野性战斗天赋树
    TALENT_TREE_DRUID_RESTORATION = 282                     // 德鲁伊 - 恢复天赋树
};

// 所有专精的掩码
#define SPEC_MASK_ALL 255

// 法术修正结构体，用于修改其他法术
struct SpellModifier
{
    // 构造函数，初始化法术修正器
    SpellModifier(Aura* _ownerAura = nullptr) : op(SPELLMOD_DAMAGE), type(SPELLMOD_FLAT), charges(0), mask(), ownerAura(_ownerAura) {}
    SpellModOp   op : 8;  // 法术修正操作类型
    SpellModType type : 8;  // 法术修正类型
    int16 charges : 16; // 剩余充能次数
    int32 value{ 0 };         // 修正值
    flag96 mask;            // 掩码
    uint32 spellId{ 0 };      // 关联的法术ID
    Aura* const ownerAura;  // 所属的光环
    uint32 priority{ 0 };     // 优先级
};

// 定义玩家天赋映射类型，键为 uint32 类型，值为 PlayerTalent 指针
typedef std::unordered_map<uint32, PlayerTalent*> PlayerTalentMap;
// 定义玩家法术映射类型，键为 uint32 类型，值为 PlayerSpell 指针
typedef std::unordered_map<uint32, PlayerSpell*> PlayerSpellMap;
// 定义法术修正器列表类型，存储 SpellModifier 指针
// 定义私聊白名单容器类型
typedef std::list<SpellModifier*> SpellModList;

// 定义私聊白名单容器类型
typedef GuidList WhisperListContainer;

// 定义法术冷却时间结构体
struct SpellCooldown
{
    uint32 end;         // 冷却结束时间
    uint16 category;    // 冷却类别
    uint32 itemid;      // 关联的物品 ID
    uint32 maxduration; // 最大冷却持续时间
    bool sendToSpectator : 1;  // 是否发送给观察者
    bool needSendToClient : 1; // 是否需要发送给客户端
};

// 定义法术冷却时间映射类型，键为 uint32 类型，值为 SpellCooldown 结构体
typedef std::map<uint32, SpellCooldown> SpellCooldowns;
// 定义实例时间映射类型，键为实例 ID，值为释放时间
// 定义训练师法术状态枚举typedef std::map<uint32, SpellCooldown> SpellCooldowns;
typedef std::unordered_map<uint32 /*instanceId*/, time_t/*releaseTime*/> InstanceTimeMap;

// 定义训练师法术状态枚举
enum TrainerSpellState
{
    TRAINER_SPELL_GREEN = 0,                 // 可学习的法术
    TRAINER_SPELL_RED = 1,                 // 不可学习的法术
    TRAINER_SPELL_GRAY = 2,                 // 灰色法术，不可见或不可用
    TRAINER_SPELL_GREEN_DISABLED = 10        // 自定义值，不发送给客户端：形式上可学习但实际不允许学习
};

// 定义动作按钮更新状态枚举
enum ActionButtonUpdateState
{
    ACTIONBUTTON_UNCHANGED = 0, // 动作按钮未改变
    ACTIONBUTTON_CHANGED = 1, // 动作按钮已改变
    ACTIONBUTTON_NEW = 2, // 新的动作按钮
    ACTIONBUTTON_DELETED = 3  // 已删除的动作按钮
};

// 定义动作按钮类型枚举
enum ActionButtonType
{
    ACTION_BUTTON_SPELL = 0x00, // 法术动作按钮
    ACTION_BUTTON_C = 0x01, // 点击动作按钮？
    ACTION_BUTTON_EQSET = 0x20, // 装备套装动作按钮
    ACTION_BUTTON_MACRO = 0x40, // 宏动作按钮
    ACTION_BUTTON_CMACRO = ACTION_BUTTON_C | ACTION_BUTTON_MACRO, // 点击宏动作按钮
    ACTION_BUTTON_ITEM = 0x80  // 物品动作按钮
};

// 定义声望来源枚举
enum ReputationSource
{
    REPUTATION_SOURCE_KILL,            // 通过击杀获取声望
    REPUTATION_SOURCE_QUEST,           // 通过任务获取声望
    REPUTATION_SOURCE_DAILY_QUEST,     // 通过日常任务获取声望
    REPUTATION_SOURCE_WEEKLY_QUEST,    // 通过周常任务获取声望
    REPUTATION_SOURCE_MONTHLY_QUEST,   // 通过月常任务获取声望
    REPUTATION_SOURCE_REPEATABLE_QUEST,// 通过可重复任务获取声望
    REPUTATION_SOURCE_SPELL            // 通过法术获取声望
};

// 定义任务音效枚举
enum QuestSound
{
    QUEST_SOUND_FAILURE = 847 // 任务失败音效
};

// 从打包数据中提取动作按钮的动作 ID
#define ACTION_BUTTON_ACTION(X) (uint32(X) & 0x00FFFFFF)
// 从打包数据中提取动作按钮的类型
#define ACTION_BUTTON_TYPE(X)   ((uint32(X) & 0xFF000000) >> 24)
// 动作按钮动作 ID 的最大值
#define MAX_ACTION_BUTTON_ACTION_VALUE (0x00FFFFFF+1)

// 定义动作按钮结构体
struct ActionButton
{
    ActionButton() = default;

    uint32 packedData{ 0 };              // 打包后的动作按钮数据
    ActionButtonUpdateState uState{ ACTIONBUTTON_NEW }; // 动作按钮更新状态

    // 获取动作按钮类型
    [[nodiscard]] ActionButtonType GetType() const { return ActionButtonType(ACTION_BUTTON_TYPE(packedData)); }
    // 获取动作按钮动作 ID
    [[nodiscard]] uint32 GetAction() const { return ACTION_BUTTON_ACTION(packedData); }
    // 设置动作按钮的动作和类型
    void SetActionAndType(uint32 action, ActionButtonType type)
    {
        uint32 newData = action | (uint32(type) << 24);
        if (newData != packedData || uState == ACTIONBUTTON_DELETED)
        {
            packedData = newData;
            if (uState != ACTIONBUTTON_NEW)
                uState = ACTIONBUTTON_CHANGED;
        }
    }
};

// 最大动作按钮数量，在 3.2.0 版本中确认
#define  MAX_ACTION_BUTTONS 144                            

// 定义动作按钮列表类型，键为 uint8 类型，值为 ActionButton 结构体
typedef std::map<uint8, ActionButton> ActionButtonList;

// 定义玩家创建信息物品结构体
struct PlayerCreateInfoItem
{
    PlayerCreateInfoItem(uint32 id, uint32 amount) : item_id(id), item_amount(amount) {}

    uint32 item_id;     // 物品 ID
    uint32 item_amount; // 物品数量
};

// 定义玩家创建信息物品列表类型
typedef std::list<PlayerCreateInfoItem> PlayerCreateInfoItems;

// 定义玩家职业等级信息结构体
struct PlayerClassLevelInfo
{
    PlayerClassLevelInfo() = default;
    uint32 basehealth{ 0 };   // 基础生命值
    uint32 basemana{ 0 };     // 基础法力值
};

struct PlayerClassInfo
{
    PlayerClassInfo() = default;

    // 职业等级信息数组指针，索引为 [level-1]，范围 0..MaxPlayerLevel-1
    PlayerClassLevelInfo* levelInfo{ nullptr };
};

// 定义玩家等级信息结构体
struct PlayerLevelInfo
{
    PlayerLevelInfo()
    {
        stats.fill(0);
    }

    // 存储玩家各项属性的数组
    std::array<uint32, MAX_STATS> stats = { };
};

// 定义玩家创建信息法术列表类型
typedef std::list<uint32> PlayerCreateInfoSpells;

// 定义玩家创建信息动作结构体
struct PlayerCreateInfoAction
{
    PlayerCreateInfoAction() = default;
    PlayerCreateInfoAction(uint8 _button, uint32 _action, uint8 _type) : button(_button), type(_type), action(_action) {}

    uint8 button{ 0 };  // 按钮编号
    uint8 type{ 0 };    // 动作类型
    uint32 action{ 0 }; // 动作 ID
};

// 定义玩家创建信息动作列表类型
typedef std::list<PlayerCreateInfoAction> PlayerCreateInfoActions;

// 定义玩家创建信息技能结构体
struct PlayerCreateInfoSkill
{
    uint16 SkillId; // 技能 ID
    uint16 Rank;    // 技能等级
};

// 定义玩家创建信息技能列表类型
typedef std::list<PlayerCreateInfoSkill> PlayerCreateInfoSkills;

// 定义玩家信息结构体
struct PlayerInfo
{
    // 通过 displayId 是否为 0 检查是否存在
    PlayerInfo() = default;

    uint32 mapId{ 0 };          // 地图 ID
    uint32 areaId{ 0 };         // 区域 ID
    float positionX{ 0.0f };    // X 坐标
    float positionY{ 0.0f };    // Y 坐标
    float positionZ{ 0.0f };    // Z 坐标
    float orientation{ 0.0f };  // 朝向
    uint16 displayId_m{ 0 };    // 男性显示 ID
    uint16 displayId_f{ 0 };    // 女性显示 ID
    PlayerCreateInfoItems item;          // 玩家创建信息物品列表
    PlayerCreateInfoSpells customSpells; // 自定义法术列表
    PlayerCreateInfoSpells  castSpells;  // 可施放法术列表
    PlayerCreateInfoActions action;      // 玩家创建信息动作列表
    PlayerCreateInfoSkills skills;       // 玩家创建信息技能列表

    // 玩家等级信息数组指针，索引为 [level-1]，范围 0..MaxPlayerLevel-1
    PlayerLevelInfo* levelInfo{ nullptr };
};

// 定义 PvP 信息结构体
struct PvPInfo
{
    PvPInfo() = default;

    bool IsHostile{ false };               // 是否处于敌对状态
    // 标记玩家是否处于强制开启 PvP 标记的区域
    bool IsInHostileArea{ false };
    // 标记玩家是否处于安全区或友好主城
    bool IsInNoPvPArea{ false };
    // 标记玩家是否处于自由 PvP 区域（如祖尔格拉布竞技场）
    bool IsInFFAPvPArea{ false };
    // 玩家取消 PvP 标记的时间（5 分钟后移除标记）
    time_t EndTimer{ 0 };
    // 玩家取消自由 PvP 标记的时间（30 秒后移除标记）
    time_t FFAPvPEndTimer{ 0 };
};

// 定义决斗状态枚举
enum DuelState
{
    DUEL_STATE_CHALLENGED,    // 已收到决斗挑战
    DUEL_STATE_COUNTDOWN,     // 决斗倒计时中
    DUEL_STATE_IN_PROGRESS,   // 决斗进行中
    DUEL_STATE_COMPLETED      // 决斗已完成
};

// 定义决斗信息结构体
struct DuelInfo
{
    DuelInfo(Player* opponent, Player* initiator, bool isMounted) : Opponent(opponent), Initiator(initiator), IsMounted(isMounted) {}

    Player* const Opponent;   // 对手玩家指针
    Player* const Initiator;  // 发起者玩家指针
    bool const IsMounted;     // 是否骑乘状态
    DuelState State = DUEL_STATE_CHALLENGED; // 决斗状态
    time_t StartTime = 0;     // 决斗开始时间
    time_t OutOfBoundsTime = 0; // 超出边界时间
};

// 定义区域结构体
struct Areas
{
    uint32 areaID;    // 区域 ID
    uint32 areaFlag;  // 区域标志
    float x1;         // X 坐标最小值
    float x2;         // X 坐标最大值
    float y1;         // Y 坐标最小值
    float y2;         // Y 坐标最大值
};

// 最大符文数量
#define MAX_RUNES       6

// 定义符文冷却时间枚举
enum RuneCooldowns
{
    RUNE_BASE_COOLDOWN = 10000, // 符文基础冷却时间
    // xinef: 最大可能的宽限期
    RUNE_GRACE_PERIOD = 2500,
    // 法术未命中时应用于符文的冷却时间
    RUNE_MISS_COOLDOWN = 1500,
};

// 定义符文类型枚举
enum RuneType
{
    RUNE_BLOOD = 0, // 鲜血符文
    RUNE_UNHOLY = 1, // 邪恶符文
    RUNE_FROST = 2, // 冰霜符文
    RUNE_DEATH = 3, // 死亡符文
    NUM_RUNE_TYPES = 4  // 符文类型数量
};

// 定义符文信息结构体
struct RuneInfo
{
    uint8 BaseRune;       // 基础符文类型
    uint8 CurrentRune;    // 当前符文类型
    uint32 Cooldown;      // 冷却时间
    uint32 GracePeriod;   // 宽限期
    AuraEffect const* ConvertAura; // 转换光环指针
};

// 定义符文结构体
struct Runes
{
    RuneInfo runes[MAX_RUNES]; // 符文信息数组
    uint8 runeState;           // 可用符文的掩码
    RuneType lastUsedRune;     // 最后使用的符文类型

    // 设置符文状态
    void SetRuneState(uint8 index, bool set = true)
    {
        if (set)
            runeState |= (1 << index);                      // 可用
        else
            runeState &= ~(1 << index);                     // 冷却中
    }
};

// 定义附魔持续时间结构体
struct EnchantDuration
{
    EnchantDuration() = default;
    EnchantDuration(Item* _item, EnchantmentSlot _slot, uint32 _leftduration) : item(_item), slot(_slot),
        leftduration(_leftduration) {
        ASSERT(item);
    };

    Item* item{ nullptr };          // 物品指针
    EnchantmentSlot slot{ MAX_ENCHANTMENT_SLOT }; // 附魔槽位
    uint32 leftduration{ 0 };       // 剩余持续时间
};

// 定义附魔持续时间列表类型
typedef std::list<EnchantDuration> EnchantDurationList;
// 定义物品持续时间列表类型
typedef std::list<Item*> ItemDurationList;

// 定义玩家移动类型枚举
enum PlayerMovementType
{
    MOVE_ROOT = 1, // 定身
    MOVE_UNROOT = 2, // 解除定身
    MOVE_WATER_WALK = 3, // 水上行走
    MOVE_LAND_WALK = 4  // 陆地行走
};

// 定义醉酒状态枚举
enum DrunkenState
{
    DRUNKEN_SOBER = 0, // 清醒状态
    DRUNKEN_TIPSY = 1, // 微醺状态
    DRUNKEN_DRUNK = 2, // 醉酒状态
    DRUNKEN_SMASHED = 3  // 烂醉状态
};

// 最大醉酒状态数量
#define MAX_DRUNKEN   4

// 定义玩家标志枚举，使用 uint32 类型
enum PlayerFlags : uint32
{
    PLAYER_FLAGS_GROUP_LEADER = 0x00000001, // 玩家是队伍领袖
    PLAYER_FLAGS_AFK = 0x00000002, // 玩家处于暂离状态
    PLAYER_FLAGS_DND = 0x00000004, // 玩家处于请勿打扰状态
    PLAYER_FLAGS_GM = 0x00000008, // 玩家是游戏管理员
    PLAYER_FLAGS_GHOST = 0x00000010, // 玩家处于幽灵状态
    PLAYER_FLAGS_RESTING = 0x00000020, // 玩家处于休息状态
    PLAYER_FLAGS_UNK6 = 0x00000040, // 未知标志 6
    // pre-3.0.3 PLAYER_FLAGS_FFA_PVP 标志，用于自由 PvP 状态
    PLAYER_FLAGS_UNK7 = 0x00000080,
    // 玩家参与了 PvP 战斗，会被争议守卫攻击
    PLAYER_FLAGS_CONTESTED_PVP = 0x00000100,
    PLAYER_FLAGS_IN_PVP = 0x00000200, // 玩家处于 PvP 状态
    PLAYER_FLAGS_HIDE_HELM = 0x00000400, // 隐藏头盔
    PLAYER_FLAGS_HIDE_CLOAK = 0x00000800, // 隐藏披风
    // 玩家已游戏很长时间
    PLAYER_FLAGS_PLAYED_LONG_TIME = 0x00001000,
    // 玩家游戏时间过长
    PLAYER_FLAGS_PLAYED_TOO_LONG = 0x00002000,
    PLAYER_FLAGS_IS_OUT_OF_BOUNDS = 0x00004000, // 玩家超出边界
    // <Dev> 前缀相关？
    PLAYER_FLAGS_DEVELOPER = 0x00008000,
    // pre-3.0.3 PLAYER_FLAGS_SANCTUARY 标志，玩家进入安全区
    PLAYER_FLAGS_UNK16 = 0x00010000,
    // 出租车基准测试模式（开启/关闭）（2.0.1）
    PLAYER_FLAGS_TAXI_BENCHMARK = 0x00020000,
    // 3.0.2，PvP 计时器激活（手动禁用 PvP 后）
    PLAYER_FLAGS_PVP_TIMER = 0x00040000,
    PLAYER_FLAGS_UBER = 0x00080000, // 未知标志
    PLAYER_FLAGS_UNK20 = 0x00100000, // 未知标志 20
    PLAYER_FLAGS_UNK21 = 0x00200000, // 未知标志 21
    PLAYER_FLAGS_COMMENTATOR2 = 0x00400000, // 解说员标志 2
    // 用于剑刃风暴和杀戮盛宴，仅允许使用带有 SPELL_ATTR0_USES_RANGED_SLOT、
    // SPELL_EFFECT_ATTACK 的法术，仅对当前玩家有效
    PLAYER_ALLOW_ONLY_ABILITY = 0x00800000,
    // 禁用所有近战技能，包括自动攻击
    PLAYER_FLAGS_UNK24 = 0x01000000,
    PLAYER_FLAGS_NO_XP_GAIN = 0x02000000, // 玩家无法获得经验值
    PLAYER_FLAGS_UNK26 = 0x04000000, // 未知标志 26
    PLAYER_FLAGS_UNK27 = 0x08000000, // 未知标志 27
    PLAYER_FLAGS_UNK28 = 0x10000000, // 未知标志 28
    PLAYER_FLAGS_UNK29 = 0x20000000, // 未知标志 29
    PLAYER_FLAGS_UNK30 = 0x40000000, // 未知标志 30
    PLAYER_FLAGS_UNK31 = 0x80000000, // 未知标志 31
};

// 定义枚举标志操作
DEFINE_ENUM_FLAG(PlayerFlags);

// 玩家字节偏移量枚举，待实现
enum PlayerBytesOffsets //@todo: Implement
{
    PLAYER_BYTES_OFFSET_SKIN_ID = 0, // 皮肤 ID 偏移量
    PLAYER_BYTES_OFFSET_FACE_ID = 1, // 脸型 ID 偏移量
    PLAYER_BYTES_OFFSET_HAIR_STYLE_ID = 2, // 发型 ID 偏移量
    PLAYER_BYTES_OFFSET_HAIR_COLOR_ID = 3  // 头发颜色 ID 偏移量
};

// 玩家字节 2 偏移量枚举，待实现
enum PlayerBytes2Offsets //@todo: Implement
{
    PLAYER_BYTES_2_OFFSET_FACIAL_STYLE = 0, // 面部风格偏移量
    PLAYER_BYTES_2_OFFSET_PARTY_TYPE = 1, // 队伍类型偏移量
    PLAYER_BYTES_2_OFFSET_BANK_BAG_SLOTS = 2, // 银行背包槽位偏移量
    PLAYER_BYTES_2_OFFSET_REST_STATE = 3  // 休息状态偏移量
};

// 玩家字节 3 偏移量枚举，待实现
enum PlayerBytes3Offsets //@todo: Implement
{
    PLAYER_BYTES_3_OFFSET_GENDER = 0, // 性别偏移量
    PLAYER_BYTES_3_OFFSET_INEBRIATION = 1, // 醉酒程度偏移量
    PLAYER_BYTES_3_OFFSET_PVP_TITLE = 2, // PvP 头衔偏移量
    PLAYER_BYTES_3_OFFSET_ARENA_FACTION = 3  // 竞技场阵营偏移量
};

// 玩家字段字节偏移量枚举，待实现
enum PlayerFieldBytesOffsets //@todo: Implement
{
    PLAYER_FIELD_BYTES_OFFSET_FLAGS = 0, // 标志偏移量
    PLAYER_FIELD_BYTES_OFFSET_RAF_GRANTABLE_LEVEL = 1, // 招募好友可授予等级偏移量
    PLAYER_FIELD_BYTES_OFFSET_ACTION_BAR_TOGGLES = 2, // 动作条切换偏移量
    PLAYER_FIELD_BYTES_OFFSET_LIFETIME_MAX_PVP_RANK = 3  // 终生最大 PvP 等级偏移量
};

// 玩家字段字节 2 偏移量枚举
enum PlayerFieldBytes2Offsets
{
    // uint16 类型！覆盖法术 ID 偏移量
    PLAYER_FIELD_BYTES_2_OFFSET_OVERRIDE_SPELLS_ID = 0,
    // 忽略能量恢复预测掩码偏移量
    PLAYER_FIELD_BYTES_2_OFFSET_IGNORE_POWER_REGEN_PREDICTION_MASK = 2,
    // 光环视觉效果偏移量
    PLAYER_FIELD_BYTES_2_OFFSET_AURA_VISION = 3
};

// 静态断言，确保 PLAYER_FIELD_BYTES_2_OFFSET_OVERRIDE_SPELLS_ID 对齐到 2 字节边界
static_assert((PLAYER_FIELD_BYTES_2_OFFSET_OVERRIDE_SPELLS_ID & 1) == 0, "PLAYER_FIELD_BYTES_2_OFFSET_OVERRIDE_SPELLS_ID must be aligned to 2 byte boundary");

// 玩家字节 2 覆盖法术 uint16 偏移量
#define PLAYER_BYTES_2_OVERRIDE_SPELLS_UINT16_OFFSET (PLAYER_FIELD_BYTES_2_OFFSET_OVERRIDE_SPELLS_ID / 2)

// 已知头衔数量
#define KNOWN_TITLES_SIZE   3
// 最大头衔索引，3 个 uint64 字段
#define MAX_TITLE_INDEX     (KNOWN_TITLES_SIZE*64)          

// 用于 PLAYER_FIELD_BYTES 值的枚举
enum PlayerFieldByteFlags
{
    PLAYER_FIELD_BYTE_TRACK_STEALTHED = 0x00000002, // 追踪潜行单位标志
    // 显示自动释放灵魂的剩余时间
    PLAYER_FIELD_BYTE_RELEASE_TIMER = 0x00000008,
    // 完全不显示 "释放灵魂" 窗口
    PLAYER_FIELD_BYTE_NO_RELEASE_WINDOW = 0x00000010
};

// 用于 PLAYER_FIELD_BYTES2 值的枚举
enum PlayerFieldByte2Flags
{
    PLAYER_FIELD_BYTE2_NONE = 0x00, // 无标志
    PLAYER_FIELD_BYTE2_STEALTH = 0x20, // 潜行标志
    PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW = 0x40  // 隐形光芒标志
};

// 镜像计时器类型枚举
enum MirrorTimerType
{
    FATIGUE_TIMER = 0, // 疲劳计时器
    BREATH_TIMER = 1, // 呼吸计时器
    FIRE_TIMER = 2  // 火焰计时器
};
// 最大计时器数量
#define MAX_TIMERS      3
// 禁用的镜像计时器值
#define DISABLED_MIRROR_TIMER   -1

// 2 的幂值，玩家额外标志枚举
enum PlayerExtraFlags
{
    // 游戏管理员能力
    PLAYER_EXTRA_GM_ON = 0x0001, // 游戏管理员开启
    PLAYER_EXTRA_ACCEPT_WHISPERS = 0x0004, // 接受私聊
    PLAYER_EXTRA_TAXICHEAT = 0x0008, // 出租车作弊
    PLAYER_EXTRA_GM_INVISIBLE = 0x0010, // 游戏管理员隐形
    // 在聊天消息中显示游戏管理员徽章
    PLAYER_EXTRA_GM_CHAT = 0x0020,
    // 标记玩家是否已拥有 310% 速度的飞行坐骑
    PLAYER_EXTRA_HAS_310_FLYER = 0x0040,
    // 标记玩家是否为观察者
    PLAYER_EXTRA_SPECTATOR_ON = 0x0080,
    // 存储 PvP 死亡状态，直到创建尸体
    PLAYER_EXTRA_PVP_DEATH = 0x0100,
    // 标记玩家在登录界面是否应看到死亡骑士宠物
    PLAYER_EXTRA_SHOW_DK_PET = 0x0400,
    PLAYER_EXTRA_GM_SPECTATOR = 0x0800, // 游戏管理员观察者
};

// 2 的幂值，登录时标志枚举
enum AtLoginFlags
{
    AT_LOGIN_NONE = 0x00, // 无标志
    AT_LOGIN_RENAME = 0x01, // 登录时重命名
    AT_LOGIN_RESET_SPELLS = 0x02, // 登录时重置法术
    AT_LOGIN_RESET_TALENTS = 0x04, // 登录时重置天赋
    AT_LOGIN_CUSTOMIZE = 0x08, // 登录时自定义
    AT_LOGIN_RESET_PET_TALENTS = 0x10, // 登录时重置宠物天赋
    AT_LOGIN_FIRST = 0x20, // 首次登录
    AT_LOGIN_CHANGE_FACTION = 0x40, // 登录时改变阵营
    AT_LOGIN_CHANGE_RACE = 0x80, // 登录时改变种族
    AT_LOGIN_RESET_AP = 0x100, // 登录时重置天赋点数
    AT_LOGIN_RESET_ARENA = 0x200, // 登录时重置竞技场
    AT_LOGIN_CHECK_ACHIEVS = 0x400, // 登录时检查成就
    AT_LOGIN_RESURRECT = 0x800  // 登录时复活
};

// 定义任务状态映射类型，键为 uint32 类型，值为 QuestStatusData 结构体
typedef std::map<uint32, QuestStatusData> QuestStatusMap;
// 定义已奖励任务集合类型
typedef std::unordered_set<uint32> RewardedQuestSet;

//               任务 ID,  是否保留
typedef std::map<uint32, bool> QuestStatusSaveMap;

// 任务槽位偏移量枚举
enum QuestSlotOffsets
{
    QUEST_ID_OFFSET = 0,     // 任务 ID 偏移量
    QUEST_STATE_OFFSET = 1,     // 任务状态偏移量
    QUEST_COUNTS_OFFSET = 2,     // 任务计数偏移量
    QUEST_TIME_OFFSET = 4      // 任务时间偏移量
};

// 最大任务偏移量
#define MAX_QUEST_OFFSET 5

// 任务槽位状态掩码枚举
enum QuestSlotStateMask
{
    QUEST_STATE_NONE = 0x0000, // 无状态
    QUEST_STATE_COMPLETE = 0x0001, // 任务完成状态
    QUEST_STATE_FAIL = 0x0002  // 任务失败状态
};

// 技能更新状态枚举
enum SkillUpdateState
{
    SKILL_UNCHANGED = 0, // 技能未改变
    SKILL_CHANGED = 1, // 技能已改变
    SKILL_NEW = 2, // 新技能
    SKILL_DELETED = 3  // 已删除的技能
};

// 定义技能状态数据结构体
struct SkillStatusData
{
    SkillStatusData(uint8 _pos, SkillUpdateState _uState) : pos(_pos), uState(_uState)
    {
    }
    uint8 pos;             // 技能位置
    SkillUpdateState uState; // 技能更新状态
};

// 定义技能状态映射类型，键为 uint32 类型，值为 SkillStatusData 结构体
typedef std::unordered_map<uint32, SkillStatusData> SkillStatusMap;

class Quest;
class Spell;
class Item;
class WorldSession;

// 定义玩家物品槽位枚举
enum PlayerSlots
{
    // 存储在玩家 m_items 数据中的第一个物品槽位
    PLAYER_SLOT_START = 0,
    // 存储在玩家 m_items 数据中的最后一个 +1 物品槽位
    PLAYER_SLOT_END = 150,
    PLAYER_SLOTS_COUNT = (PLAYER_SLOT_END - PLAYER_SLOT_START) // 玩家物品槽位数量
};

// 背包 0 槽位
#define INVENTORY_SLOT_BAG_0    255

// 装备槽位枚举，共 19 个槽位
enum EquipmentSlots
{
    EQUIPMENT_SLOT_START = 0,  // 装备槽位起始
    EQUIPMENT_SLOT_HEAD = 0,  // 头部装备槽位
    EQUIPMENT_SLOT_NECK = 1,  // 颈部装备槽位
    EQUIPMENT_SLOT_SHOULDERS = 2,  // 肩部装备槽位
    EQUIPMENT_SLOT_BODY = 3,  // 身体装备槽位
    EQUIPMENT_SLOT_CHEST = 4,  // 胸部装备槽位
    EQUIPMENT_SLOT_WAIST = 5,  // 腰部装备槽位
    EQUIPMENT_SLOT_LEGS = 6,  // 腿部装备槽位
    EQUIPMENT_SLOT_FEET = 7,  // 脚部装备槽位
    EQUIPMENT_SLOT_WRISTS = 8,  // 手腕装备槽位
    EQUIPMENT_SLOT_HANDS = 9,  // 手部装备槽位
    EQUIPMENT_SLOT_FINGER1 = 10, // 手指 1 装备槽位
    EQUIPMENT_SLOT_FINGER2 = 11, // 手指 2 装备槽位
    EQUIPMENT_SLOT_TRINKET1 = 12, // 饰品 1 装备槽位
    EQUIPMENT_SLOT_TRINKET2 = 13, // 饰品 2 装备槽位
    EQUIPMENT_SLOT_BACK = 14, // 背部装备槽位
    EQUIPMENT_SLOT_MAINHAND = 15, // 主手装备槽位
    EQUIPMENT_SLOT_OFFHAND = 16, // 副手装备槽位
    EQUIPMENT_SLOT_RANGED = 17, // 远程装备槽位
    EQUIPMENT_SLOT_TABARD = 18, // 战袍装备槽位
    EQUIPMENT_SLOT_END = 19  // 装备槽位结束
};

// 背包槽位枚举，共 4 个槽位
enum InventorySlots
{
    INVENTORY_SLOT_BAG_START = 19, // 背包槽位起始
    INVENTORY_SLOT_BAG_END = 23  // 背包槽位结束
};

// 背包物品槽位枚举，共 16 个槽位
enum InventoryPackSlots
{
    INVENTORY_SLOT_ITEM_START = 23, // 背包物品槽位起始
    INVENTORY_SLOT_ITEM_END = 39  // 背包物品槽位结束
};

// 银行物品槽位枚举，共 28 个槽位
enum BankItemSlots
{
    BANK_SLOT_ITEM_START = 39, // 银行物品槽位起始
    BANK_SLOT_ITEM_END = 67  // 银行物品槽位结束
};

// 银行背包槽位枚举，共 7 个槽位
enum BankBagSlots
{
    BANK_SLOT_BAG_START = 67, // 银行背包槽位起始
    BANK_SLOT_BAG_END = 74  // 银行背包槽位结束
};

// 回购槽位枚举，共 12 个槽位
enum BuyBackSlots
{
    // 存储在 m_items 中，不再有 m_buybackitems
    BUYBACK_SLOT_START = 74,
    BUYBACK_SLOT_END = 86  // 回购槽位结束
};

// 钥匙链槽位枚举，共 32 个槽位
enum KeyRingSlots
{
    KEYRING_SLOT_START = 86, // 钥匙链槽位起始
    KEYRING_SLOT_END = 118 // 钥匙链槽位结束
};

// 货币令牌槽位枚举，共 32 个槽位
enum CurrencyTokenSlots
{
    CURRENCYTOKEN_SLOT_START = 118, // 货币令牌槽位起始
    CURRENCYTOKEN_SLOT_END = 150  // 货币令牌槽位结束
};

// 装备套装更新状态枚举
enum EquipmentSetUpdateState
{
    EQUIPMENT_SET_UNCHANGED = 0, // 装备套装未改变
    EQUIPMENT_SET_CHANGED = 1, // 装备套装已改变
    EQUIPMENT_SET_NEW = 2, // 新的装备套装
    EQUIPMENT_SET_DELETED = 3  // 已删除的装备套装
};

// 定义装备套装结构体
struct EquipmentSet
{
    EquipmentSet() = default;

    uint64 Guid;               // 装备套装 GUID
    std::string Name;          // 装备套装名称
    std::string IconName;      // 装备套装图标名称
    uint32 IgnoreMask{ 0 };      // 忽略掩码
    ObjectGuid Items[EQUIPMENT_SLOT_END]; // 装备套装物品 GUID 数组
    EquipmentSetUpdateState state{ EQUIPMENT_SET_NEW }; // 装备套装更新状态
};

// 最大装备套装索引，客户端限制
#define MAX_EQUIPMENT_SET_INDEX 10                          

// 定义装备套装映射类型，键为 uint32 类型，值为 EquipmentSet 结构体
typedef std::map<uint32, EquipmentSet> EquipmentSets;

// 定义物品位置和数量结构体
struct ItemPosCount
{
    ItemPosCount(uint16 _pos, uint32 _count) : pos(_pos), count(_count) {}
    // 判断当前物品位置和数量是否包含在给定的向量中
    [[nodiscard]] bool isContainedIn(std::vector<ItemPosCount> const& vec) const;
    uint16 pos;     // 物品位置
    uint32 count;   // 物品数量
};
// 定义物品位置和数量向量类型
typedef std::vector<ItemPosCount> ItemPosCountVec;

// 定义保存的物品结构体
struct SavedItem
{
    Item* item;     // 物品指针
    uint16 dstpos;  // 目标位置

    SavedItem(Item* _item, uint16 dstpos) : item(_item), dstpos(dstpos) {}
};

// 定义传送中止原因枚举
enum TransferAbortReason
{
    TRANSFER_ABORT_NONE = 0x00, // 无中止原因
    TRANSFER_ABORT_ERROR = 0x01, // 传送错误
    // 传送中止：实例已满
    TRANSFER_ABORT_MAX_PLAYERS = 0x02,
    // 传送中止：实例未找到
    TRANSFER_ABORT_NOT_FOUND = 0x03,
    // 你最近进入的实例过多
    TRANSFER_ABORT_TOO_MANY_INSTANCES = 0x04,
    // 无法在战斗进行中进入区域
    TRANSFER_ABORT_ZONE_IN_COMBAT = 0x06,
    // 你必须安装 <TBC, WotLK> 扩展才能进入此区域
    TRANSFER_ABORT_INSUF_EXPAN_LVL = 0x07,
    // <普通, 英雄, 史诗> 难度模式对 %s 不可用
    TRANSFER_ABORT_DIFFICULTY = 0x08,
    // 在你摆脱巫妖王的控制之前，无法离开此地！
    TRANSFER_ABORT_UNIQUE_MESSAGE = 0x09,
    // 无法启动更多实例，请稍后再试
    TRANSFER_ABORT_TOO_MANY_REALM_INSTANCES = 0x0A,
    TRANSFER_ABORT_NEED_GROUP = 0x0B, // 3.1
    TRANSFER_ABORT_NOT_FOUND1 = 0x0C, // 3.1
    TRANSFER_ABORT_NOT_FOUND2 = 0x0D, // 3.1
    TRANSFER_ABORT_NOT_FOUND3 = 0x0E, // 3.2
    // 队伍中的所有玩家必须来自同一服务器
    TRANSFER_ABORT_REALM_ONLY = 0x0F,
    // 此时无法进入地图
    TRANSFER_ABORT_MAP_NOT_ALLOWED = 0x10,
};

// 定义实例重置警告类型枚举
enum InstanceResetWarningType
{
    // WARNING! %s 计划在 %d 小时后重置
    RAID_INSTANCE_WARNING_HOURS = 1,
    // WARNING! %s 计划在 %d 分钟后重置！
    RAID_INSTANCE_WARNING_MIN = 2,
    // WARNING! %s 计划在 %d 分钟后重置。请离开该区域，否则将被传送回绑定位置！
    RAID_INSTANCE_WARNING_MIN_SOON = 3,
    // 欢迎来到 %s。此团队副本实例计划在 %s 后重置
    RAID_INSTANCE_WELCOME = 4,
    RAID_INSTANCE_EXPIRED = 5 // 团队副本实例已过期
};

class InstanceSave;

// 定义休息标志枚举
enum RestFlag
{
    REST_FLAG_IN_TAVERN = 0x1, // 在酒馆中休息
    REST_FLAG_IN_CITY = 0x2, // 在城市中休息
    // 用于 AREA_FLAG_REST_ZONE_*
    REST_FLAG_IN_FACTION_AREA = 0x4,
};

// 定义传送选项枚举
enum TeleportToOptions
{
    TELE_TO_GM_MODE = 0x01, // 以游戏管理员模式传送
    TELE_TO_NOT_LEAVE_TRANSPORT = 0x02, // 传送时不离开载具
    TELE_TO_NOT_LEAVE_COMBAT = 0x04, // 传送时不离开战斗
    TELE_TO_NOT_UNSUMMON_PET = 0x08, // 传送时不召唤宠物
    TELE_TO_SPELL = 0x10, // 通过法术传送
    TELE_TO_NOT_LEAVE_VEHICLE = 0x20, // 传送时不离开载具
    TELE_TO_WITH_PET = 0x40, // 传送时携带宠物
    TELE_TO_NOT_LEAVE_TAXI = 0x80  // 传送时不离开出租车
};

/// 环境伤害类型枚举
enum EnviromentalDamage
{
    DAMAGE_EXHAUSTED = 0,      // 疲劳伤害
    DAMAGE_DROWNING = 1,      // 溺水伤害
    DAMAGE_FALL = 2,      // 坠落伤害
    DAMAGE_LAVA = 3,      // 熔岩伤害
    DAMAGE_SLIME = 4,      // 黏液伤害
    DAMAGE_FIRE = 5,      // 火焰伤害
    // 自定义情况，坠落无耐久损失
    DAMAGE_FALL_TO_VOID = 6
};

// 定义玩家聊天标签枚举
enum PlayerChatTag
{
    CHAT_TAG_NONE = 0x00, // 无聊天标签
    CHAT_TAG_AFK = 0x01, // 暂离聊天标签
    CHAT_TAG_DND = 0x02, // 请勿打扰聊天标签
    CHAT_TAG_GM = 0x04, // 游戏管理员聊天标签
    // 解说员标签，纯净客户端中不存在
    CHAT_TAG_COM = 0x08,
    CHAT_TAG_DEV = 0x10  // 开发者聊天标签
};

// 定义游戏时间索引枚举
enum PlayedTimeIndex
{
    PLAYED_TIME_TOTAL = 0, // 总游戏时间索引
    PLAYED_TIME_LEVEL = 1  // 当前等级游戏时间索引
};

// 最大游戏时间索引
#define MAX_PLAYED_TIME_INDEX 2

// 在玩家加载查询列表准备时使用，之后用于结果选择
enum PlayerLoginQueryIndex
{
    PLAYER_LOGIN_QUERY_LOAD_FROM = 0, // 从数据库加载基础信息
    PLAYER_LOGIN_QUERY_LOAD_AURAS = 3, // 加载光环信息
    PLAYER_LOGIN_QUERY_LOAD_SPELLS = 4, // 加载法术信息
    PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS = 5, // 加载任务状态
    PLAYER_LOGIN_QUERY_LOAD_DAILY_QUEST_STATUS = 6, // 加载日常任务状态
    PLAYER_LOGIN_QUERY_LOAD_REPUTATION = 7, // 加载声望信息
    PLAYER_LOGIN_QUERY_LOAD_INVENTORY = 8, // 加载背包信息
    PLAYER_LOGIN_QUERY_LOAD_ACTIONS = 9, // 加载动作按钮信息
    PLAYER_LOGIN_QUERY_LOAD_MAILS = 10, // 加载邮件信息
    PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS = 11, // 加载邮件物品信息
    PLAYER_LOGIN_QUERY_LOAD_SOCIAL_LIST = 13, // 加载社交列表
    PLAYER_LOGIN_QUERY_LOAD_HOME_BIND = 14, // 加载家绑定信息
    PLAYER_LOGIN_QUERY_LOAD_SPELL_COOLDOWNS = 15, // 加载法术冷却时间
    PLAYER_LOGIN_QUERY_LOAD_DECLINED_NAMES = 16, // 加载拒绝的名字
    PLAYER_LOGIN_QUERY_LOAD_ACHIEVEMENTS = 18, // 加载成就信息
    PLAYER_LOGIN_QUERY_LOAD_CRITERIA_PROGRESS = 19, // 加载成就进度
    PLAYER_LOGIN_QUERY_LOAD_EQUIPMENT_SETS = 20, // 加载装备套装信息
    PLAYER_LOGIN_QUERY_LOAD_ENTRY_POINT = 21, // 加载入口点信息
    PLAYER_LOGIN_QUERY_LOAD_GLYPHS = 22, // 加载雕文信息
    PLAYER_LOGIN_QUERY_LOAD_TALENTS = 23, // 加载天赋信息
    PLAYER_LOGIN_QUERY_LOAD_ACCOUNT_DATA = 24, // 加载账号数据
    PLAYER_LOGIN_QUERY_LOAD_SKILLS = 25, // 加载技能信息
    PLAYER_LOGIN_QUERY_LOAD_WEEKLY_QUEST_STATUS = 26, // 加载周常任务状态
    PLAYER_LOGIN_QUERY_LOAD_RANDOM_BG = 27, // 加载随机战场状态
    PLAYER_LOGIN_QUERY_LOAD_BANNED = 28, // 加载封禁信息
    PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_REW = 29, // 加载已奖励任务状态
    PLAYER_LOGIN_QUERY_LOAD_INSTANCE_LOCK_TIMES = 30, // 加载实例锁定时间
    PLAYER_LOGIN_QUERY_LOAD_SEASONAL_QUEST_STATUS = 31, // 加载季节性任务状态
    PLAYER_LOGIN_QUERY_LOAD_MONTHLY_QUEST_STATUS = 32, // 加载月常任务状态
    PLAYER_LOGIN_QUERY_LOAD_BREW_OF_THE_MONTH = 34, // 加载本月酿造信息
    PLAYER_LOGIN_QUERY_LOAD_CORPSE_LOCATION = 35, // 加载尸体位置
    PLAYER_LOGIN_QUERY_LOAD_CHARACTER_SETTINGS = 36, // 加载角色设置
    PLAYER_LOGIN_QUERY_LOAD_PET_SLOTS = 37, // 加载宠物槽位
    PLAYER_LOGIN_QUERY_LOAD_OFFLINE_ACHIEVEMENTS_UPDATES = 38, // 加载离线成就更新
    MAX_PLAYER_LOGIN_QUERY                              // 最大玩家登录查询索引
};

// 定义玩家延迟操作枚举
enum PlayerDelayedOperations
{
    DELAYED_SAVE_PLAYER = 0x01, // 延迟保存玩家数据
    DELAYED_RESURRECT_PLAYER = 0x02, // 延迟复活玩家
    DELAYED_SPELL_CAST_DESERTER = 0x04, // 延迟施放逃兵法术
    // 标志，从战场传送后恢复坐骑状态
    DELAYED_BG_MOUNT_RESTORE = 0x08,
    // 标志，从战场传送后恢复出租车状态
    DELAYED_BG_TAXI_RESTORE = 0x10,
    // 标志，从战场传送后恢复队伍状态
    DELAYED_BG_GROUP_RESTORE = 0x20,
    DELAYED_VEHICLE_TELEPORT = 0x40, // 延迟载具传送
    DELAYED_END                  // 延迟操作结束
};

// 定义玩家被魅惑时的 AI 法术枚举
enum PlayerCharmedAISpells
{
    SPELL_T_STUN,         // 眩晕法术
    SPELL_ROOT_OR_FEAR,   // 定身或恐惧法术
    SPELL_INSTANT_DAMAGE, // 即时伤害法术
    SPELL_INSTANT_DAMAGE2,// 即时伤害法术 2
    SPELL_HIGH_DAMAGE1,   // 高伤害法术 1
    SPELL_HIGH_DAMAGE2,   // 高伤害法术 2
    SPELL_DOT_DAMAGE,     // 持续伤害法术
    SPELL_T_CHARGE,       // 冲锋法术
    SPELL_IMMUNITY,       // 免疫法术
    SPELL_FAST_RUN,       // 快速奔跑法术
    NUM_CAI_SPELLS        // 被魅惑 AI 法术数量
};

// 玩家召唤自动拒绝时间（秒）
#define MAX_PLAYER_SUMMON_DELAY                   (2*MINUTE)
// 最大金钱数量
#define MAX_MONEY_AMOUNT                       (0x7FFFFFFF-1)

// 定义进度要求结构体
struct ProgressionRequirement
{
    uint32 id;            // 进度要求 ID
    TeamId faction;       // 阵营
    std::string note;     // 备注
    uint32 priority;      // 优先级
    bool checkLeaderOnly; // 仅检查队长
};

// 定义地下城进度要求结构体
struct DungeonProgressionRequirements
{
    uint8  levelMin;      // 最小等级
    uint8  levelMax;      // 最大等级
    uint16 reqItemLevel;  // 所需物品等级
    std::vector<ProgressionRequirement*> quests;    // 所需任务列表
    std::vector<ProgressionRequirement*> items;     // 所需物品列表
    std::vector<ProgressionRequirement*> achievements; // 所需成就列表
};

// 定义角色删除方法枚举
enum CharDeleteMethod
{
    CHAR_DELETE_REMOVE = 0, // 从数据库中完全删除
    CHAR_DELETE_UNLINK = 1  // 角色与账号解绑，名字释放，游戏中显示为已删除
};

// 定义货币物品枚举
enum CurrencyItems
{
    ITEM_HONOR_POINTS_ID = 43308, // 荣誉点数物品 ID
    ITEM_ARENA_POINTS_ID = 43307  // 竞技场点数物品 ID
};

// 定义招募好友错误枚举
enum ReferAFriendError
{
    ERR_REFER_A_FRIEND_NONE = 0x00, // 无错误
    ERR_REFER_A_FRIEND_NOT_REFERRED_BY = 0x01, // 未被该玩家招募
    ERR_REFER_A_FRIEND_TARGET_TOO_HIGH = 0x02, // 目标等级过高
    ERR_REFER_A_FRIEND_INSUFFICIENT_GRANTABLE_LEVELS = 0x03, // 可授予等级不足
    ERR_REFER_A_FRIEND_TOO_FAR = 0x04, // 距离过远
    ERR_REFER_A_FRIEND_DIFFERENT_FACTION = 0x05, // 阵营不同
    ERR_REFER_A_FRIEND_NOT_NOW = 0x06, // 现在不可用
    ERR_REFER_A_FRIEND_GRANT_LEVEL_MAX_I = 0x07, // 授予等级达到上限
    ERR_REFER_A_FRIEND_NO_TARGET = 0x08, // 没有目标
    ERR_REFER_A_FRIEND_NOT_IN_GROUP = 0x09, // 不在同一队伍中
    ERR_REFER_A_FRIEND_SUMMON_LEVEL_MAX_I = 0x0A, // 召唤等级达到上限
    ERR_REFER_A_FRIEND_SUMMON_COOLDOWN = 0x0B, // 召唤冷却中
    ERR_REFER_A_FRIEND_INSUF_EXPAN_LVL = 0x0C, // 扩展等级不足
    ERR_REFER_A_FRIEND_SUMMON_OFFLINE_S = 0x0D  // 目标玩家离线
};

// 定义玩家休息状态枚举
enum PlayerRestState
{
    REST_STATE_RESTED = 0x01, // 已休息状态
    REST_STATE_NOT_RAF_LINKED = 0x02, // 未与招募好友链接状态
    REST_STATE_RAF_LINKED = 0x06  // 与招募好友链接状态
};

// 定义额外保存枚举
enum AdditionalSaving
{
    ADDITIONAL_SAVING_NONE = 0x00, // 无额外保存
    ADDITIONAL_SAVING_INVENTORY_AND_GOLD = 0x01, // 额外保存背包和金币
    ADDITIONAL_SAVING_QUEST_STATUS = 0x02  // 额外保存任务状态
};

// 定义玩家命令状态枚举
enum PlayerCommandStates
{
    CHEAT_NONE = 0x00,     // 无作弊状态
    CHEAT_GOD = 0x01,      // 无敌作弊状态
    CHEAT_CASTTIME = 0x02, // 施法时间作弊状态
    CHEAT_COOLDOWN = 0x04, // 冷却时间作弊状态
    CHEAT_POWER = 0x08,    // 能量作弊状态
    CHEAT_WATERWALK = 0x10 // 水上行走作弊状态
};

// 用于 OnGiveXP 玩家脚本钩子
enum PlayerXPSource
{
    XPSOURCE_KILL = 0,      // 通过击杀获取经验
    XPSOURCE_QUEST = 1,     // 通过任务获取经验
    XPSOURCE_QUEST_DF = 2,  // 通过动态任务获取经验
    XPSOURCE_EXPLORE = 3,   // 通过探索获取经验
    XPSOURCE_BATTLEGROUND = 4 // 通过战场获取经验
};

// 定义即时飞行对话动作枚举
enum InstantFlightGossipAction
{
    GOSSIP_ACTION_TOGGLE_INSTANT_FLIGHT = 500 // 切换即时飞行
};

// 定义表情广播文本 ID 枚举
enum EmoteBroadcastTextID
{
    EMOTE_BROADCAST_TEXT_ID_STRANGE_GESTURES = 91243 // 奇怪手势表情广播文本 ID
};

// 重载输出运算符，用于输出玩家出租车信息
std::ostringstream& operator<< (std::ostringstream& ss, PlayerTaxi const& taxi);

class Player;

// 战场数据持有者（pussywizard: 不存储在数据库中）
struct BGData
{
    BGData() = default;

    uint32 bgInstanceID{ 0 };          // 战场实例 ID
    BattlegroundTypeId bgTypeID{ BATTLEGROUND_TYPE_NONE }; // 战场类型 ID
    TeamId bgTeamId{ TEAM_NEUTRAL };   // 战场队伍 ID
    // 战场队列槽位，默认值为 PLAYER_MAX_BATTLEGROUND_QUEUES
    uint32 bgQueueSlot{ PLAYER_MAX_BATTLEGROUND_QUEUES };
    bool isInvited{ false };           // 是否已被邀请
    bool bgIsRandom{ false };          // 是否为随机战场

    GuidSet            bgAfkReporter; // 举报玩家 AFK 的玩家 GUID 集合
    uint8              bgAfkReportedCount{ 0 }; // 被举报 AFK 的次数
    time_t             bgAfkReportedTimer{ 0 }; // 被举报 AFK 的计时
};

// 入口点数据持有者（pussywizard: 存储在数据库中）
struct EntryPointData
{
    EntryPointData()
    {
        ClearTaxiPath();
    }

    uint32 mountSpell{ 0 };            // 坐骑法术 ID
    std::array<uint32, 2> taxiPath;  // 出租车路径
    WorldLocation joinPos;           // 加入位置

    // 清除出租车路径
    void ClearTaxiPath() { taxiPath.fill(0); }
    // 判断是否有出租车路径
    [[nodiscard]] bool HasTaxiPath() const { return taxiPath[0] && taxiPath[1]; }
};

// 定义待处理法术施放请求结构体
struct PendingSpellCastRequest
{
    uint32 spellId;          // 法术 ID
    uint32 category;         // 法术类别
    WorldPacket requestPacket; // 请求数据包
    bool isItem = false;     // 是否为物品触发的法术
    bool cancelInProgress = false; // 是否正在取消

    PendingSpellCastRequest(uint32 spellId, uint32 category, WorldPacket&& packet, bool item = false, bool cancel = false)
        : spellId(spellId), category(category), requestPacket(std::move(packet)), isItem(item), cancelInProgress(cancel) {
    }
};

class Player : public Unit, public GridObject<Player>
{
    friend class WorldSession;
    friend class CinematicMgr;
    friend void Item::AddToUpdateQueueOf(Player* player);
    friend void Item::RemoveFromUpdateQueueOf(Player* player);
public:
    explicit Player(WorldSession* session); // 构造函数，接受一个WorldSession指针
    ~Player() override; // 析构函数

    void CleanupsBeforeDelete(bool finalCleanup = true) override; // 删除前的清理操作

    void AddToWorld() override; // 将玩家添加到世界中
    void RemoveFromWorld() override; // 从世界中移除玩家

    void SetObjectScale(float scale) override // 设置对象缩放比例
    {
        Unit::SetObjectScale(scale);
        SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, scale * DEFAULT_WORLD_OBJECT_SIZE); // 设置包围半径
        SetFloatValue(UNIT_FIELD_COMBATREACH, scale * DEFAULT_COMBAT_REACH); // 设置战斗范围
    }

    [[nodiscard]] bool hasSpanishClient() // 检查客户端是否为西班牙语
    {
        return GetSession()->GetSessionDbLocaleIndex() == LOCALE_esES || GetSession()->GetSessionDbLocaleIndex() == LOCALE_esMX;
    }

    bool TeleportTo(uint32 mapid, float x, float y, float z, float orientation, uint32 options = 0, Unit* target = nullptr, bool newInstance = false); // 传送到指定位置
    bool TeleportTo(WorldLocation const& loc, uint32 options = 0, Unit* target = nullptr) // 通过WorldLocation传送到指定位置
    {
        return TeleportTo(loc.GetMapId(), loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ(), loc.GetOrientation(), options, target);
    }
    bool TeleportToEntryPoint(); // 传送到入口点

    void SetSummonPoint(uint32 mapid, float x, float y, float z, uint32 delay = 0, bool asSpectator = false); // 设置召唤点
    [[nodiscard]] bool IsSummonAsSpectator() const; // 是否以观察者身份召唤
    void SetSummonAsSpectator(bool on) { m_summon_asSpectator = on; } // 设置是否以观察者身份召唤
    void SummonIfPossible(bool agree, ObjectGuid summoner_guid); // 如果可能，召唤玩家
    [[nodiscard]] time_t GetSummonExpireTimer() const { return m_summon_expire; } // 获取召唤过期时间

    bool Create(ObjectGuid::LowType guidlow, CharacterCreateInfo* createInfo); // 创建玩家角色

    void Update(uint32 time) override; // 更新玩家状态

    PlayerFlags GetPlayerFlags() const { return PlayerFlags(GetUInt32Value(PLAYER_FLAGS)); } // 获取玩家标志
    bool HasPlayerFlag(PlayerFlags flags) const { return HasFlag(PLAYER_FLAGS, flags) != 0; } // 检查是否具有指定标志
    void SetPlayerFlag(PlayerFlags flags) { SetFlag(PLAYER_FLAGS, flags); } // 设置玩家标志
    void RemovePlayerFlag(PlayerFlags flags) { RemoveFlag(PLAYER_FLAGS, flags); } // 移除玩家标志
    void ReplaceAllPlayerFlags(PlayerFlags flags) { SetUInt32Value(PLAYER_FLAGS, flags); } // 替换所有玩家标志
    void SetPlayerFlag(uint32 flag, bool value) { // 设置指定标志的值
        if (value)
            SetFlag(PLAYER_FLAGS, flag);
        else
            RemoveFlag(PLAYER_FLAGS, flag);
    }

    static bool BuildEnumData(PreparedQueryResult result, WorldPacket* data); // 构建枚举数据

    [[nodiscard]] bool IsClass(Classes playerClass, ClassContext context = CLASS_CONTEXT_NONE) const override; // 检查玩家是否为指定职业

    void SetInWater(bool apply); // 设置是否在水中

    [[nodiscard]] bool IsInWater() const override { return m_isInWater; } // 检查是否在水中
    [[nodiscard]] bool IsFalling() const; // 检查是否在下落
    bool IsInAreaTriggerRadius(AreaTrigger const* trigger, float delta = 0.f) const; // 检查是否在区域触发半径内

    void SendInitialPacketsBeforeAddToMap(); // 添加到地图前发送初始数据包
    void SendInitialPacketsAfterAddToMap(); // 添加到地图后发送初始数据包
    void SendTransferAborted(uint32 mapid, TransferAbortReason reason, uint8 arg = 0); // 发送传送中止消息
    void SendInstanceResetWarning(uint32 mapid, Difficulty difficulty, uint32 time, bool onEnterMap); // 发送实例重置警告

    bool CanInteractWithQuestGiver(Object* questGiver); // 检查是否可以与任务给予者互动
    Creature* GetNPCIfCanInteractWith(ObjectGuid guid, uint32 npcflagmask); // 获取可以互动的NPC
    [[nodiscard]] GameObject* GetGameObjectIfCanInteractWith(ObjectGuid guid, GameobjectTypes type) const; // 获取可以互动的游戏对象

    void ToggleAFK(); // 切换AFK状态
    void ToggleDND(); // 切换勿扰状态
    [[nodiscard]] bool isAFK() const { return HasPlayerFlag(PLAYER_FLAGS_AFK); } // 检查是否为AFK状态
    [[nodiscard]] bool isDND() const { return HasPlayerFlag(PLAYER_FLAGS_DND); } // 检查是否为勿扰状态
    [[nodiscard]] uint8 GetChatTag() const; // 获取聊天标签
    std::string autoReplyMsg; // 自动回复消息

    uint32 GetBarberShopCost(uint8 newhairstyle, uint8 newhaircolor, uint8 newfacialhair, BarberShopStyleEntry const* newSkin = nullptr); // 获取理发店费用

    PlayerSocial* GetSocial() { return m_social; } // 获取社交信息

    PlayerTaxi m_taxi; // 玩家的飞行路径管理器
    void InitTaxiNodesForLevel() { m_taxi.InitTaxiNodesForLevel(getRace(), getClass(), GetLevel()); } // 根据等级初始化飞行节点
    bool ActivateTaxiPathTo(std::vector<uint32> const& nodes, Creature* npc = nullptr, uint32 spellid = 1); // 激活飞行路径
    bool ActivateTaxiPathTo(uint32 taxi_path_id, uint32 spellid = 1); // 通过路径ID激活飞行路径
    void CleanupAfterTaxiFlight(); // 飞行后清理
    void ContinueTaxiFlight(); // 继续飞行
    void SendTaxiNodeStatusMultiple(); // 发送多个飞行节点状态

    [[nodiscard]] bool IsCommentator() const { return HasPlayerFlag(PLAYER_FLAGS_COMMENTATOR2); } // 检查是否为评论员
    void SetCommentator(bool on) { ApplyModFlag(PLAYER_FLAGS, PLAYER_FLAGS_COMMENTATOR2, on); } // 设置评论员状态
    [[nodiscard]] bool IsDeveloper() const { return HasPlayerFlag(PLAYER_FLAGS_DEVELOPER); } // 检查是否为开发者
    void SetDeveloper(bool on) { ApplyModFlag(PLAYER_FLAGS, PLAYER_FLAGS_DEVELOPER, on); } // 设置开发者状态
    [[nodiscard]] bool isAcceptWhispers() const { return m_ExtraFlags & PLAYER_EXTRA_ACCEPT_WHISPERS; } // 检查是否接受私语
    void SetAcceptWhispers(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_ACCEPT_WHISPERS; else m_ExtraFlags &= ~PLAYER_EXTRA_ACCEPT_WHISPERS; } // 设置接受私语状态
    [[nodiscard]] bool IsGameMaster() const { return m_ExtraFlags & PLAYER_EXTRA_GM_ON; } // 检查是否为游戏管理员
    void SetGameMaster(bool on); // 设置游戏管理员状态
    [[nodiscard]] bool isGMChat() const { return m_ExtraFlags & PLAYER_EXTRA_GM_CHAT; } // 检查是否为GM聊天
    void SetGMChat(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_GM_CHAT; else m_ExtraFlags &= ~PLAYER_EXTRA_GM_CHAT; } // 设置GM聊天状态
    [[nodiscard]] bool IsGMSpectator() const { return m_ExtraFlags & PLAYER_EXTRA_GM_SPECTATOR; } // 检查是否为GM观察者
    void SetGMSpectator(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_GM_SPECTATOR; else m_ExtraFlags &= ~PLAYER_EXTRA_GM_SPECTATOR; } // 设置GM观察者状态

    [[nodiscard]] bool isTaxiCheater() const { return m_ExtraFlags & PLAYER_EXTRA_TAXICHEAT; } // 检查是否为飞行作弊者
    void SetTaxiCheater(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_TAXICHEAT; else m_ExtraFlags &= ~PLAYER_EXTRA_TAXICHEAT; } // 设置飞行作弊状态
    [[nodiscard]] bool isGMVisible() const { return !(m_ExtraFlags & PLAYER_EXTRA_GM_INVISIBLE); } // 检查是否对GM可见
    void SetGMVisible(bool on); // 设置GM可见状态
    bool Has310Flyer(bool checkAllSpells, uint32 excludeSpellId = 0); // 检查是否具有310%飞行速度
    void SetHas310Flyer(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_HAS_310_FLYER; else m_ExtraFlags &= ~PLAYER_EXTRA_HAS_310_FLYER; } // 设置310%飞行速度状态
    void SetPvPDeath(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_PVP_DEATH; else m_ExtraFlags &= ~PLAYER_EXTRA_PVP_DEATH; } // 设置PvP死亡状态

    void GiveXP(uint32 xp, Unit* victim, float group_rate = 1.0f, bool isLFGReward = false); // 给予经验值
    void GiveLevel(uint8 level); // 给予等级

    void InitStatsForLevel(bool reapplyMods = false); // 根据等级初始化属性

    [[nodiscard]] bool HasActivePowerType(Powers power) override; // 检查是否具有活动的能量类型

    // .cheat命令相关
    [[nodiscard]] bool GetCommandStatus(uint32 command) const { return _activeCheats & command; } // 获取命令状态
    void SetCommandStatusOn(uint32 command) { _activeCheats |= command; } // 启用命令
    void SetCommandStatusOff(uint32 command) { _activeCheats &= ~command; } // 禁用命令

    // 游戏时间相关
    time_t m_logintime; // 登录时间
    time_t m_Last_tick; // 上一次心跳时间
    uint32 m_Played_time[MAX_PLAYED_TIME_INDEX]; // 已玩游戏时间
    uint32 GetTotalPlayedTime() { return m_Played_time[PLAYED_TIME_TOTAL]; } // 获取总游戏时间
    uint32 GetLevelPlayedTime() { return m_Played_time[PLAYED_TIME_LEVEL]; } // 获取当前等级的游戏时间

    void setDeathState(DeathState s, bool despawn = false) override; // 设置死亡状态

    void SetRestState(uint32 triggerId); // 设置休息状态
    void RemoveRestState(); // 移除休息状态
    uint32 GetXPRestBonus(uint32 xp); // 获取经验休息奖励
    [[nodiscard]] float GetRestBonus() const { return _restBonus; } // 获取休息奖励
    void SetRestBonus(float restBonusNew); // 设置休息奖励

    [[nodiscard]] bool HasRestFlag(RestFlag restFlag) const { return (_restFlagMask & restFlag) != 0; } // 检查是否具有休息标志
    void SetRestFlag(RestFlag restFlag, uint32 triggerId = 0); // 设置休息标志
    void RemoveRestFlag(RestFlag restFlag); // 移除休息标志
    [[nodiscard]] uint32 GetInnTriggerId() const { return _innTriggerId; } // 获取旅馆触发ID

    PetStable* GetPetStable() { return m_petStable.get(); } // 获取宠物栏
    PetStable& GetOrInitPetStable(); // 获取或初始化宠物栏
    [[nodiscard]] PetStable const* GetPetStable() const { return m_petStable.get(); } // 获取常量宠物栏

    [[nodiscard]] Pet* GetPet() const; // 获取当前宠物
    Pet* SummonPet(uint32 entry, float x, float y, float z, float ang, PetType petType, Milliseconds duration = 0s, uint32 healthPct = 0); // 召唤宠物
    void RemovePet(Pet* pet, PetSaveMode mode, bool returnreagent = false); // 移除宠物
    bool CanPetResurrect(); // 检查宠物是否可以复活
    bool IsExistPet(); // 检查宠物是否存在
    Pet* CreatePet(Creature* creatureTarget, uint32 spellID = 0); // 创建宠物
    Pet* CreatePet(uint32 creatureEntry, uint32 spellID = 0); // 通过生物ID创建宠物

    [[nodiscard]] uint32 GetPhaseMaskForSpawn() const; // 获取生成时的相位掩码

    /// 处理常规聊天中的消息，基于声明的语言和配置中预定义的范围。
    void Say(std::string_view text, Language language, WorldObject const* = nullptr) override;
    void Say(uint32 textId, WorldObject const* target = nullptr) override;
    /// 处理常规聊天中的喊叫消息，基于声明的语言和配置中预定义的范围。
    void Yell(std::string_view text, Language language, WorldObject const* = nullptr) override;
    void Yell(uint32 textId, WorldObject const* target = nullptr) override;
    /// 输出一个通用文本，应该是动作。
    void TextEmote(std::string_view text, WorldObject const* = nullptr, bool = false) override;
    void TextEmote(uint32 textId, WorldObject const* target = nullptr, bool isBossEmote = false) override;
    /// 处理来自插件和玩家的私语消息，基于发送者、接收者GUID和语言。
    void Whisper(std::string_view text, Language language, Player* receiver, bool = false) override;
    void Whisper(uint32 textId, Player* target, bool isBossWhisper = false) override;

    /*********************************************************/
    /***                    STORAGE SYSTEM                 ***/
    /*********************************************************/

    void SetVirtualItemSlot(uint8 i, Item* item); // 设置虚拟物品槽位
    void SetSheath(SheathState sheathed) override; // 设置武器收起状态
    uint8 FindEquipSlot(ItemTemplate const* proto, uint32 slot, bool swap) const; // 查找装备槽位
    uint32 GetItemCount(uint32 item, bool inBankAlso = false, Item* skipItem = nullptr) const; // 获取物品数量
    uint32 GetItemCountWithLimitCategory(uint32 limitCategory, Item* skipItem = nullptr) const; // 获取具有限制类别的物品数量
    [[nodiscard]] Item* GetItemByGuid(ObjectGuid guid) const; // 通过GUID获取物品
    [[nodiscard]] Item* GetItemByEntry(uint32 entry) const; // 通过条目ID获取物品
    [[nodiscard]] Item* GetItemByPos(uint16 pos) const; // 通过位置获取物品
    [[nodiscard]] Item* GetItemByPos(uint8 bag, uint8 slot) const; // 通过包和槽位获取物品
    [[nodiscard]] Bag* GetBagByPos(uint8 slot) const; // 通过槽位获取包
    [[nodiscard]] uint32 GetFreeInventorySpace() const; // 获取空闲背包空间
    [[nodiscard]] inline Item* GetUseableItemByPos(uint8 bag, uint8 slot) const // 检查是否可以使用该位置的物品
    {
        if (!CanUseAttackType(GetAttackBySlot(slot)))
            return nullptr;
        return GetItemByPos(bag, slot);
    }
    [[nodiscard]] Item* GetWeaponForAttack(WeaponAttackType attackType, bool useable = false) const; // 获取用于攻击的武器
    bool HasWeapon(WeaponAttackType type) const override { return GetWeaponForAttack(type, false); } // 检查是否具有武器
    bool HasWeaponForAttack(WeaponAttackType type) const override { return (Unit::HasWeaponForAttack(type) && GetWeaponForAttack(type, true)); } // 检查是否具有可用于攻击的武器
    [[nodiscard]] Item* GetShield(bool useable = false) const; // 获取盾牌
    static uint8 GetAttackBySlot(uint8 slot); // 通过槽位获取攻击类型
    std::vector<Item*>& GetItemUpdateQueue() { return m_itemUpdateQueue; } // 获取物品更新队列
    static bool IsInventoryPos(uint16 pos) { return IsInventoryPos(pos >> 8, pos & 255); } // 检查是否为库存位置
    static bool IsInventoryPos(uint8 bag, uint8 slot);
    static bool IsEquipmentPos(uint16 pos) { return IsEquipmentPos(pos >> 8, pos & 255); } // 检查是否为装备位置
    static bool IsEquipmentPos(uint8 bag, uint8 slot);
    static bool IsBagPos(uint16 pos); // 检查是否为包位置
    static bool IsBankPos(uint16 pos) { return IsBankPos(pos >> 8, pos & 255); } // 检查是否为银行位置
    static bool IsBankPos(uint8 bag, uint8 slot);
    bool IsValidPos(uint16 pos, bool explicit_pos) { return IsValidPos(pos >> 8, pos & 255, explicit_pos); } // 检查位置是否有效
    bool IsValidPos(uint8 bag, uint8 slot, bool explicit_pos);
    [[nodiscard]] uint8 GetBankBagSlotCount() const { return GetByteValue(PLAYER_BYTES_2, 2); } // 获取银行包槽位数量
    void SetBankBagSlotCount(uint8 count) { SetByteValue(PLAYER_BYTES_2, 2, count); } // 设置银行包槽位数量
    [[nodiscard]] bool HasItemCount(uint32 item, uint32 count = 1, bool inBankAlso = false) const; // 检查是否具有指定数量的物品
    bool HasItemFitToSpellRequirements(SpellInfo const* spellInfo, Item const* ignoreItem = nullptr) const; // 检查物品是否符合法术要求
    bool CanNoReagentCast(SpellInfo const* spellInfo) const; // 检查是否可以无材料施法
    [[nodiscard]] bool HasItemOrGemWithIdEquipped(uint32 item, uint32 count, uint8 except_slot = NULL_SLOT) const; // 检查是否装备了指定ID的物品或宝石
    [[nodiscard]] bool HasItemOrGemWithLimitCategoryEquipped(uint32 limitCategory, uint32 count, uint8 except_slot = NULL_SLOT) const; // 检查是否装备了具有限制类别的物品或宝石
    InventoryResult CanTakeMoreSimilarItems(Item* pItem) const { return CanTakeMoreSimilarItems(pItem->GetEntry(), pItem->GetCount(), pItem); } // 检查是否可以获取更多相同物品
    [[nodiscard]] InventoryResult CanTakeMoreSimilarItems(uint32 entry, uint32 count) const { return CanTakeMoreSimilarItems(entry, count, nullptr); } // 检查是否可以获取更多相同物品
    InventoryResult CanStoreNewItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, uint32 item, uint32 count, uint32* no_space_count = nullptr) const // 检查是否可以存储新物品
    {
        return CanStoreItem(bag, slot, dest, item, count, nullptr, false, no_space_count);
    }
    InventoryResult CanStoreItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, Item* pItem, bool swap = false) const // 检查是否可以存储物品
    {
        if (!pItem)
            return EQUIP_ERR_ITEM_NOT_FOUND;
        uint32 count = pItem->GetCount();
        return CanStoreItem(bag, slot, dest, pItem->GetEntry(), count, pItem, swap, nullptr);
    }
    InventoryResult CanStoreItems(Item** pItem, int32 count) const; // 检查是否可以存储多个物品
    InventoryResult CanEquipNewItem(uint8 slot, uint16& dest, uint32 item, bool swap) const; // 检查是否可以装备新物品
    InventoryResult CanEquipItem(uint8 slot, uint16& dest, Item* pItem, bool swap, bool not_loading = true) const; // 检查是否可以装备物品

    InventoryResult CanEquipUniqueItem(Item* pItem, uint8 except_slot = NULL_SLOT, uint32 limit_count = 1) const; // 检查是否可以装备唯一物品
    InventoryResult CanEquipUniqueItem(ItemTemplate const* itemProto, uint8 except_slot = NULL_SLOT, uint32 limit_count = 1) const; // 检查是否可以装备唯一物品
    [[nodiscard]] InventoryResult CanUnequipItems(uint32 item, uint32 count) const; // 检查是否可以卸下物品
    [[nodiscard]] InventoryResult CanUnequipItem(uint16 src, bool swap) const; // 检查是否可以卸下物品
    InventoryResult CanBankItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, Item* pItem, bool swap, bool not_loading = true) const; // 检查是否可以存入银行
    InventoryResult CanUseItem(Item* pItem, bool not_loading = true) const; // 检查是否可以使用物品
    [[nodiscard]] bool HasItemTotemCategory(uint32 TotemCategory) const; // 检查是否具有图腾类别
    bool IsTotemCategoryCompatiableWith(ItemTemplate const* pProto, uint32 requiredTotemCategoryId) const; // 检查图腾类别是否兼容
    InventoryResult CanUseItem(ItemTemplate const* pItem) const; // 检查是否可以使用物品
    [[nodiscard]] InventoryResult CanUseAmmo(uint32 item) const; // 检查是否可以使用弹药
    InventoryResult CanRollForItemInLFG(ItemTemplate const* item, WorldObject const* lootedObject) const; // 检查是否可以在LFG中掷骰子获取物品
    Item* StoreNewItem(ItemPosCountVec const& pos, uint32 item, bool update, int32 randomPropertyId = 0); // 存储新物品
    Item* StoreNewItem(ItemPosCountVec const& pos, uint32 item, bool update, int32 randomPropertyId, AllowedLooterSet& allowedLooters); // 存储新物品
    Item* StoreItem(ItemPosCountVec const& pos, Item* pItem, bool update); // 存储物品
    Item* EquipNewItem(uint16 pos, uint32 item, bool update); // 装备新物品
    Item* EquipItem(uint16 pos, Item* pItem, bool update); // 装备物品
    void AutoUnequipOffhandIfNeed(bool force = false); // 自动卸下副手
    bool StoreNewItemInBestSlots(uint32 item_id, uint32 item_count); // 在最佳槽位存储新物品
    void AutoStoreLoot(uint8 bag, uint8 slot, uint32 loot_id, LootStore const& store, bool broadcast = false); // 自动存储战利品
    void AutoStoreLoot(uint32 loot_id, LootStore const& store, bool broadcast = false) { AutoStoreLoot(NULL_BAG, NULL_SLOT, loot_id, store, broadcast); } // 自动存储战利品
    LootItem* StoreLootItem(uint8 lootSlot, Loot* loot, InventoryResult& msg); // 存储战利品项
    void UpdateLootAchievements(LootItem* item, Loot* loot); // 更新战利品成就
    void UpdateTitansGrip(); // 更新泰坦之握

    InventoryResult CanTakeMoreSimilarItems(uint32 entry, uint32 count, Item* pItem, uint32* no_space_count = nullptr) const; // 检查是否可以获取更多相似物品
    InventoryResult CanStoreItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, uint32 entry, uint32 count, Item* pItem = nullptr, bool swap = false, uint32* no_space_count = nullptr) const; // 检查是否可以存储物品

    void AddRefundReference(ObjectGuid itemGUID); // 添加退款引用
    void DeleteRefundReference(ObjectGuid itemGUID); // 删除退款引用

    void ApplyEquipCooldown(Item* pItem); // 应用装备冷却
    void SetAmmo(uint32 item); // 设置弹药
    void RemoveAmmo(); // 移除弹药
    [[nodiscard]] float GetAmmoDPS() const { return m_ammoDPS; } // 获取弹药DPS
    bool CheckAmmoCompatibility(ItemTemplate const* ammo_proto) const; // 检查弹药兼容性
    void QuickEquipItem(uint16 pos, Item* pItem); // 快速装备物品
    void VisualizeItem(uint8 slot, Item* pItem); // 可视化物品
    void SetVisibleItemSlot(uint8 slot, Item* pItem); // 设置可见物品槽位
    Item* BankItem(ItemPosCountVec const& dest, Item* pItem, bool update) // 银行物品
    {
        return StoreItem(dest, pItem, update);
    }
    Item* BankItem(uint16 pos, Item* pItem, bool update); // 银行物品
    void RemoveItem(uint8 bag, uint8 slot, bool update, bool swap = false); // 移除物品
    void MoveItemFromInventory(uint8 bag, uint8 slot, bool update); // 从库存中移动物品
    // 在交易、拍卖、公会银行、邮件中使用
    void MoveItemToInventory(ItemPosCountVec const& dest, Item* pItem, bool update, bool in_characterInventoryDB = false); // 移动物品到库存
    // 在交易、公会银行、邮件中使用
    void RemoveItemDependentAurasAndCasts(Item* pItem); // 移除物品依赖的光环和施法
    void DestroyItem(uint8 bag, uint8 slot, bool update); // 销毁物品
    void DestroyItemCount(uint32 item, uint32 count, bool update, bool unequip_check = false); // 销毁指定数量的物品
    void DestroyItemCount(Item* item, uint32& count, bool update); // 销毁指定数量的物品
    void DestroyConjuredItems(bool update); // 销毁召唤物品
    void DestroyZoneLimitedItem(bool update, uint32 new_zone); // 销毁区域限制物品
    void SplitItem(uint16 src, uint16 dst, uint32 count); // 分割物品
    void SwapItem(uint16 src, uint16 dst); // 交换物品
    void AddItemToBuyBackSlot(Item* pItem, uint32 money); // 添加物品到购买回槽位
    Item* GetItemFromBuyBackSlot(uint32 slot); // 从购买回槽位获取物品
    void RemoveItemFromBuyBackSlot(uint32 slot, bool del); // 从购买回槽位移除物品
    [[nodiscard]] uint32 GetMaxKeyringSize() const { return KEYRING_SLOT_END - KEYRING_SLOT_START; } // 获取钥匙环最大大小
    void SendEquipError(InventoryResult msg, Item* pItem, Item* pItem2 = nullptr, uint32 itemid = 0); // 发送装备错误
    void SendBuyError(BuyResult msg, Creature* creature, uint32 item, uint32 param); // 发送购买错误
    void SendSellError(SellResult msg, Creature* creature, ObjectGuid guid, uint32 param); // 发送出售错误
    void AddWeaponProficiency(uint32 newflag) { m_WeaponProficiency |= newflag; } // 添加武器专精
    void AddArmorProficiency(uint32 newflag) { m_ArmorProficiency |= newflag; } // 添加护甲专精
    [[nodiscard]] uint32 GetWeaponProficiency() const { return m_WeaponProficiency; } // 获取武器专精
    [[nodiscard]] uint32 GetArmorProficiency() const { return m_ArmorProficiency; } // 获取护甲专精

    [[nodiscard]] bool IsTwoHandUsed() const // 检查是否使用双手武器
    {
        Item* mainItem = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        return mainItem && mainItem->GetTemplate()->InventoryType == INVTYPE_2HWEAPON && !CanTitanGrip();
    }
    void SendNewItem(Item* item, uint32 count, bool received, bool created, bool broadcast = false, bool sendChatMessage = true); // 发送新物品
    bool BuyItemFromVendorSlot(ObjectGuid vendorguid, uint32 vendorslot, uint32 item, uint8 count, uint8 bag, uint8 slot); // 从供应商槽位购买物品
    bool _StoreOrEquipNewItem(uint32 vendorslot, uint32 item, uint8 count, uint8 bag, uint8 slot, int32 price, ItemTemplate const* pProto, Creature* pVendor, VendorItem const* crItem, bool bStore); // 存储或装备新物品

    [[nodiscard]] float GetReputationPriceDiscount(Creature const* creature) const; // 获取声望价格折扣
    [[nodiscard]] float GetReputationPriceDiscount(FactionTemplateEntry const* factionTemplate) const; // 获取声望价格折扣

    [[nodiscard]] Player* GetTrader() const { return m_trade ? m_trade->GetTrader() : nullptr; } // 获取交易者
    [[nodiscard]] TradeData* GetTradeData() const { return m_trade; } // 获取交易数据
    void TradeCancel(bool sendback, TradeStatus status = TRADE_STATUS_TRADE_CANCELED); // 取消交易

    CinematicMgr* GetCinematicMgr() const { return _cinematicMgr; } // 获取过场动画管理器

    void UpdateEnchantTime(uint32 time); // 更新附魔时间
    void UpdateSoulboundTradeItems(); // 更新灵魂绑定交易物品
    void AddTradeableItem(Item* item); // 添加可交易物品
    void RemoveTradeableItem(Item* item); // 移除可交易物品
    void UpdateItemDuration(uint32 time, bool realtimeonly = false); // 更新物品持续时间
    void AddEnchantmentDurations(Item* item); // 添加附魔持续时间
    void RemoveEnchantmentDurations(Item* item); // 移除附魔持续时间
    void RemoveEnchantmentDurationsReferences(Item* item); // 移除附魔持续时间引用
    void RemoveArenaEnchantments(EnchantmentSlot slot); // 移除竞技场附魔
    void AddEnchantmentDuration(Item* item, EnchantmentSlot slot, uint32 duration); // 添加附魔持续时间
    void ApplyEnchantment(Item* item, EnchantmentSlot slot, bool apply, bool apply_dur = true, bool ignore_condition = false); // 应用附魔
    void ApplyEnchantment(Item* item, bool apply); // 应用附魔
    void UpdateSkillEnchantments(uint16 skill_id, uint16 curr_value, uint16 new_value); // 更新技能附魔
    void SendEnchantmentDurations(); // 发送附魔持续时间
    void UpdateEnchantmentDurations(); // 更新附魔持续时间
    void BuildEnchantmentsInfoData(WorldPacket* data); // 构建附魔信息数据
    void AddItemDurations(Item* item); // 添加物品持续时间
    void RemoveItemDurations(Item* item); // 移除物品持续时间
    void SendItemDurations(); // 发送物品持续时间
    void LoadCorpse(PreparedQueryResult result); // 加载尸体
    void LoadPet(); // 加载宠物

    bool AddItem(uint32 itemId, uint32 count); // 添加物品

    /*********************************************************/
    /***                    GOSSIP SYSTEM                  ***/
    /*********************************************************/

    void PrepareGossipMenu(WorldObject* source, uint32 menuId = 0, bool showQuests = false); // 准备闲聊菜单
    void SendPreparedGossip(WorldObject* source); // 发送准备好的闲聊
    void OnGossipSelect(WorldObject* source, uint32 gossipListId, uint32 menuId); // 闲聊选择事件

    uint32 GetGossipTextId(uint32 menuId, WorldObject* source); // 获取闲聊文本ID
    uint32 GetGossipTextId(WorldObject* source); // 获取闲聊文本ID
    static uint32 GetDefaultGossipMenuForSource(WorldObject* source); // 获取默认闲聊菜单

    void ToggleInstantFlight(); // 切换即时飞行

    /*********************************************************/
    /***                    QUEST SYSTEM                   ***/
    /*********************************************************/

    int32 GetQuestLevel(Quest const* quest) const { return quest && (quest->GetQuestLevel() > 0) ? quest->GetQuestLevel() : GetLevel(); } // 获取任务等级

    void PrepareQuestMenu(ObjectGuid guid); // 准备任务菜单
    void SendPreparedQuest(ObjectGuid guid); // 发送准备好的任务
    [[nodiscard]] bool IsActiveQuest(uint32 quest_id) const; // 检查任务是否活跃
    Quest const* GetNextQuest(ObjectGuid guid, Quest const* quest); // 获取下一个任务
    bool CanSeeStartQuest(Quest const* quest); // 检查是否可以看到开始任务
    bool CanTakeQuest(Quest const* quest, bool msg); // 检查是否可以接受任务
    bool CanAddQuest(Quest const* quest, bool msg); // 检查是否可以添加任务
    bool CanCompleteQuest(uint32 quest_id, const QuestStatusData* q_savedStatus = nullptr); // 检查是否可以完成任务
    bool CanCompleteRepeatableQuest(Quest const* quest); // 检查是否可以完成重复任务
    bool CanRewardQuest(Quest const* quest, bool msg); // 检查是否可以奖励任务
    bool CanRewardQuest(Quest const* quest, uint32 reward, bool msg); // 检查是否可以奖励任务
    void AddQuestAndCheckCompletion(Quest const* quest, Object* questGiver); // 添加任务并检查完成
    void AddQuest(Quest const* quest, Object* questGiver); // 添加任务
    void AbandonQuest(uint32 quest_id); // 放弃任务
    void CompleteQuest(uint32 quest_id); // 完成任务
    void IncompleteQuest(uint32 quest_id); // 未完成任务
    void RewardQuest(Quest const* quest, uint32 reward, Object* questGiver, bool announce = true, bool isLFGReward = false); // 奖励任务
    void SetRewardedQuest(uint32 quest_id); // 设置已奖励任务
    void FailQuest(uint32 quest_id); // 任务失败
    bool SatisfyQuestSkill(Quest const* qInfo, bool msg) const; // 检查技能是否满足任务要求
    bool SatisfyQuestLevel(Quest const* qInfo, bool msg) const; // 检查等级是否满足任务要求
    bool SatisfyQuestLog(bool msg); // 检查任务日志是否满足要求
    bool SatisfyQuestPreviousQuest(Quest const* qInfo, bool msg) const; // 检查前置任务是否满足
    bool SatisfyQuestClass(Quest const* qInfo, bool msg) const; // 检查职业是否满足任务要求
    bool SatisfyQuestRace(Quest const* qInfo, bool msg) const; // 检查种族是否满足任务要求
    bool SatisfyQuestReputation(Quest const* qInfo, bool msg) const; // 检查声望是否满足任务要求
    bool SatisfyQuestStatus(Quest const* qInfo, bool msg) const; // 检查任务状态是否满足
    bool SatisfyQuestConditions(Quest const* qInfo, bool msg); // 检查条件是否满足任务要求
    bool SatisfyQuestTimed(Quest const* qInfo, bool msg) const; // 检查时间是否满足任务要求
    bool SatisfyQuestExclusiveGroup(Quest const* qInfo, bool msg) const; // 检查独占组是否满足任务要求
    bool SatisfyQuestNextChain(Quest const* qInfo, bool msg) const; // 检查下一个任务链是否满足
    bool SatisfyQuestPrevChain(Quest const* qInfo, bool msg) const; // 检查上一个任务链是否满足
    bool SatisfyQuestDay(Quest const* qInfo, bool msg) const; // 检查日期是否满足任务要求
    bool SatisfyQuestWeek(Quest const* qInfo, bool msg) const; // 检查周数是否满足任务要求
    bool SatisfyQuestMonth(Quest const* qInfo, bool msg) const; // 检查月份是否满足任务要求
    bool SatisfyQuestSeasonal(Quest const* qInfo, bool msg) const; // 检查季节是否满足任务要求
    bool GiveQuestSourceItem(Quest const* quest); // 给予任务来源物品
    bool TakeQuestSourceItem(uint32 questId, bool msg); // 获取任务来源物品
    uint32 CalculateQuestRewardXP(Quest const* quest); // 计算任务奖励经验
    [[nodiscard]] bool GetQuestRewardStatus(uint32 quest_id) const; // 获取任务奖励状态
    [[nodiscard]] QuestStatus GetQuestStatus(uint32 quest_id) const; // 获取任务状态
    void SetQuestStatus(uint32 questId, QuestStatus status, bool update = true); // 设置任务状态
    void RemoveActiveQuest(uint32 questId, bool update = true); // 移除活跃任务
    void RemoveRewardedQuest(uint32 questId, bool update = true); // 移除已奖励任务
    void SendQuestUpdate(uint32 questId); // 发送任务更新
    QuestGiverStatus GetQuestDialogStatus(Object* questGiver); // 获取任务给予者对话状态
    float GetQuestRate(bool isDFQuest = false); // 获取任务奖励倍率
    void SetDailyQuestStatus(uint32 quest_id); // 设置每日任务状态
    bool IsDailyQuestDone(uint32 quest_id); // 检查每日任务是否完成
    void SetWeeklyQuestStatus(uint32 quest_id); // 设置每周任务状态
    void SetMonthlyQuestStatus(uint32 quest_id); // 设置每月任务状态
    void SetSeasonalQuestStatus(uint32 quest_id); // 设置季节性任务状态
    void ResetDailyQuestStatus(); // 重置每日任务状态
    void ResetWeeklyQuestStatus(); // 重置每周任务状态
    void ResetMonthlyQuestStatus(); // 重置每月任务状态
    void ResetSeasonalQuestStatus(uint16 event_id); // 重置季节性任务状态

    [[nodiscard]] uint16 FindQuestSlot(uint32 quest_id) const; // 查找任务槽位
    [[nodiscard]] uint32 GetQuestSlotQuestId(uint16 slot) const { return GetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_ID_OFFSET); } // 获取任务槽位的任务ID
    [[nodiscard]] uint32 GetQuestSlotState(uint16 slot)   const { return GetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_STATE_OFFSET); } // 获取任务槽位的状态
    [[nodiscard]] uint16 GetQuestSlotCounter(uint16 slot, uint8 counter) const { return (uint16)(GetUInt64Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET) >> (counter * 16)); } // 获取任务槽位的计数器
    [[nodiscard]] uint32 GetQuestSlotTime(uint16 slot)    const { return GetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_TIME_OFFSET); } // 获取任务槽位的时间
    void SetQuestSlot(uint16 slot, uint32 quest_id, uint32 timer = 0) // 设置任务槽位
    {
        SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_ID_OFFSET, quest_id);
        SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_STATE_OFFSET, 0);
        SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET, 0);
        SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET + 1, 0);
        SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_TIME_OFFSET, timer);
    }
    void SetQuestSlotCounter(uint16 slot, uint8 counter, uint16 count) // 设置任务槽位的计数器
    {
        uint64 val = GetUInt64Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET);
        val &= ~((uint64)0xFFFF << (counter * 16));
        val |= ((uint64)count << (counter * 16));
        SetUInt64Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET, val);
    }
    void SetQuestSlotState(uint16 slot, uint32 state) { SetFlag(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_STATE_OFFSET, state); } // 设置任务槽位状态
    void RemoveQuestSlotState(uint16 slot, uint32 state) { RemoveFlag(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_STATE_OFFSET, state); } // 移除任务槽位状态
    void SetQuestSlotTimer(uint16 slot, uint32 timer) { SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_TIME_OFFSET, timer); } // 设置任务槽位计时器
    void SwapQuestSlot(uint16 slot1, uint16 slot2) // 交换任务槽位
    {
        for (int i = 0; i < MAX_QUEST_OFFSET; ++i)
        {
            uint32 temp1 = GetUInt32Value(PLAYER_QUEST_LOG_1_1 + MAX_QUEST_OFFSET * slot1 + i);
            uint32 temp2 = GetUInt32Value(PLAYER_QUEST_LOG_1_1 + MAX_QUEST_OFFSET * slot2 + i);

            SetUInt32Value(PLAYER_QUEST_LOG_1_1 + MAX_QUEST_OFFSET * slot1 + i, temp2);
            SetUInt32Value(PLAYER_QUEST_LOG_1_1 + MAX_QUEST_OFFSET * slot2 + i, temp1);
        }
    }
    uint16 GetReqKillOrCastCurrentCount(uint32 quest_id, int32 entry); // 获取任务击杀或施法当前计数
    void AreaExploredOrEventHappens(uint32 questId); // 区域探索或事件发生
    void GroupEventHappens(uint32 questId, WorldObject const* pEventObject); // 组事件发生
    void ItemAddedQuestCheck(uint32 entry, uint32 count); // 物品添加任务检查
    void ItemRemovedQuestCheck(uint32 entry, uint32 count); // 物品移除任务检查
    void KilledMonster(CreatureTemplate const* cInfo, ObjectGuid guid); // 击杀怪物
    void KilledMonsterCredit(uint32 entry, ObjectGuid guid = ObjectGuid::Empty); // 击杀怪物信用
    void KilledPlayerCredit(uint16 count = 1); // 击杀玩家信用
    void KilledPlayerCreditForQuest(uint16 count, Quest const* quest); // 击杀玩家信用（针对特定任务）
    void KillCreditGO(uint32 entry, ObjectGuid guid = ObjectGuid::Empty); // 击杀游戏对象信用
    void TalkedToCreature(uint32 entry, ObjectGuid guid); // 与生物交谈
    void MoneyChanged(uint32 value); // 金钱变化
    void ReputationChanged(FactionEntry const* factionEntry); // 声望变化
    void ReputationChanged2(FactionEntry const* factionEntry); // 声望变化
    [[nodiscard]] bool HasQuestForItem(uint32 itemId, uint32 excludeQuestId = 0, bool turnIn = false, bool* showInLoot = nullptr) const; // 检查是否有任务需要该物品
    [[nodiscard]] bool HasQuestForGO(int32 GOId) const; // 检查是否有任务需要该游戏对象
    [[nodiscard]] bool HasQuest(uint32 questId) const; // 检查是否有任务
    void UpdateForQuestWorldObjects(); // 更新任务世界对象
    [[nodiscard]] bool CanShareQuest(uint32 quest_id) const; // 检查是否可以共享任务

    void SendQuestComplete(uint32 quest_id); // 发送任务完成
    void SendQuestReward(Quest const* quest, uint32 XP); // 发送任务奖励
    void SendQuestFailed(uint32 questId, InventoryResult reason = EQUIP_ERR_OK); // 发送任务失败
    void SendQuestTimerFailed(uint32 quest_id); // 发送任务计时器失败
    void SendCanTakeQuestResponse(uint32 msg) const; // 发送可以接受任务响应
    void SendQuestConfirmAccept(Quest const* quest, Player* pReceiver); // 发送任务确认接受
    void SendPushToPartyResponse(Player const* player, uint8 msg) const; // 发送推送到队伍响应
    void SendQuestUpdateAddItem(Quest const* quest, uint32 item_idx, uint16 count); // 发送任务更新添加物品
    void SendQuestUpdateAddCreatureOrGo(Quest const* quest, ObjectGuid guid, uint32 creatureOrGO_idx, uint16 old_count, uint16 add_count); // 发送任务更新添加生物或游戏对象
    void SendQuestUpdateAddPlayer(Quest const* quest, uint16 old_count, uint16 add_count); // 发送任务更新添加玩家

    ObjectGuid GetDivider() { return m_divider; } // 获取分隔符
    void SetDivider(ObjectGuid guid = ObjectGuid::Empty) { m_divider = guid; } // 设置分隔符

    uint32 GetInGameTime() { return m_ingametime; } // 获取游戏内时间

    void SetInGameTime(uint32 time) { m_ingametime = time; } // 设置游戏内时间

    void AddTimedQuest(uint32 quest_id) { m_timedquests.insert(quest_id); } // 添加定时任务
    void RemoveTimedQuest(uint32 quest_id) { m_timedquests.erase(quest_id); } // 移除定时任务

    [[nodiscard]] bool HasPvPForcingQuest() const; // 检查是否有PvP强制任务

    /*********************************************************/
    /***                   LOAD SYSTEM                     ***/
    /*********************************************************/

    bool LoadFromDB(ObjectGuid guid, CharacterDatabaseQueryHolder const& holder); // 从数据库加载玩家
    [[nodiscard]] bool isBeingLoaded() const override; // 检查是否正在加载

    void Initialize(ObjectGuid::LowType guid); // 初始化
    static uint32 GetZoneIdFromDB(ObjectGuid guid); // 从数据库获取区域ID
    static bool   LoadPositionFromDB(uint32& mapid, float& x, float& y, float& z, float& o, bool& in_flight, ObjectGuid::LowType guid); // 从数据库加载位置

    static bool IsValidGender(uint8 Gender) { return Gender <= GENDER_FEMALE; } // 检查性别是否有效

    /*********************************************************/
    /***                   SAVE SYSTEM                     ***/
    /*********************************************************/

    void SaveToDB(bool create, bool logout); // 保存到数据库
    void SaveToDB(CharacterDatabaseTransaction trans, bool create, bool logout); // 保存到数据库
    void SaveInventoryAndGoldToDB(CharacterDatabaseTransaction trans); // 快速保存物品和金币
    void SaveGoldToDB(CharacterDatabaseTransaction trans); // 保存金币
    void _SaveSkills(CharacterDatabaseTransaction trans); // 保存技能

    static void Customize(CharacterCustomizeInfo const* customizeInfo, CharacterDatabaseTransaction trans); // 自定义角色
    static void SavePositionInDB(uint32 mapid, float x, float y, float z, float o, uint32 zone, ObjectGuid guid); // 保存位置到数据库
    static void SavePositionInDB(WorldLocation const& loc, uint16 zoneId, ObjectGuid guid, CharacterDatabaseTransaction trans); // 保存位置到数据库

    static void DeleteFromDB(ObjectGuid::LowType lowGuid, uint32 accountId, bool updateRealmChars, bool deleteFinally); // 从数据库删除
    static void DeleteOldCharacters(); // 删除旧角色
    static void DeleteOldCharacters(uint32 keepDays); // 删除旧角色

    static void DeleteOldRecoveryItems(); // 删除旧恢复物品
    static void DeleteOldRecoveryItems(uint32 keepDays); // 删除旧恢复物品

    bool m_mailsUpdated; // 邮件是否已更新

    void SetBindPoint(ObjectGuid guid); // 设置绑定点
    void SendTalentWipeConfirm(ObjectGuid guid); // 发送天赋重置确认
    void ResetPetTalents(); // 重置宠物天赋
    void CalcRage(uint32 damage, bool attacker); // 计算怒气
    void RegenerateAll(); // 所有资源再生
    void Regenerate(Powers power); // 特定资源再生
    void RegenerateHealth(); // 生命值再生
    void setRegenTimerCount(uint32 time) { m_regenTimerCount = time; } // 设置再生计时器
    void setWeaponChangeTimer(uint32 time) { m_weaponChangeTimer = time; } // 设置武器切换计时器

    [[nodiscard]] uint32 GetMoney() const { return GetUInt32Value(PLAYER_FIELD_COINAGE); } // 获取玩家金币
    bool ModifyMoney(int32 amount, bool sendError = true); // 修改玩家金币
    [[nodiscard]] bool HasEnoughMoney(uint32 amount) const { return (GetMoney() >= amount); } // 检查是否有足够的金币
    [[nodiscard]] bool HasEnoughMoney(int32 amount) const
    {
        if (amount > 0)
            return (GetMoney() >= (uint32)amount);
        return true;
    }

    void SetMoney(uint32 value)
    {
        SetUInt32Value(PLAYER_FIELD_COINAGE, value); // 设置金币值
        MoneyChanged(value); // 金币改变通知
        UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED); // 更新成就条件
    }

    [[nodiscard]] RewardedQuestSet const& getRewardedQuests() const { return m_RewardedQuests; } // 获取已奖励的任务集合
    QuestStatusMap& getQuestStatusMap() { return m_QuestStatus; } // 获取任务状态映射
    QuestStatusSaveMap& GetQuestStatusSaveMap() { return m_QuestStatusSave; } // 获取任务保存状态映射

    [[nodiscard]] std::size_t GetRewardedQuestCount() const { return m_RewardedQuests.size(); } // 获取已奖励任务数量
    [[nodiscard]] bool IsQuestRewarded(uint32 quest_id) const
    {
        return m_RewardedQuests.find(quest_id) != m_RewardedQuests.end(); // 检查任务是否已奖励
    }

    [[nodiscard]] Unit* GetSelectedUnit() const; // 获取选中的单位
    [[nodiscard]] Player* GetSelectedPlayer() const; // 获取选中的玩家

    void SetTarget(ObjectGuid /*guid*/ = ObjectGuid::Empty) override {} /// 用于服务器端目标更改，不适用于玩家
    void SetSelection(ObjectGuid guid); // 设置选择的目标

    void SendMailResult(uint32 mailId, MailResponseType mailAction, MailResponseResult mailError, uint32 equipError = 0, ObjectGuid::LowType item_guid = 0, uint32 item_count = 0); // 发送邮件结果
    void SendNewMail(); // 发送新邮件
    void UpdateNextMailTimeAndUnreads(); // 更新下次邮件时间和未读邮件
    void AddNewMailDeliverTime(time_t deliver_time); // 添加新邮件投递时间

    void RemoveMail(uint32 id); // 移除邮件

    void AddMail(Mail* mail) { m_mail.push_front(mail); }// 用于从 WorldSession::SendMailTo 调用
    uint32 GetMailSize() { return m_mail.size(); } // 获取邮件数量
    Mail* GetMail(uint32 id); // 获取指定ID的邮件

    [[nodiscard]] PlayerMails const& GetMails() const { return m_mail; } // 获取所有邮件
    void SendItemRetrievalMail(uint32 itemEntry, uint32 count); // 发送物品检索邮件
    void SendItemRetrievalMail(std::vector<std::pair<uint32, uint32>> mailItems); // 发送物品检索邮件

    /*********************************************************/
    /*** MAILED ITEMS SYSTEM ***/
    /*********************************************************/

    uint8 unReadMails; // 未读邮件数量
    time_t m_nextMailDelivereTime; // 下次邮件投递时间

    typedef std::unordered_map<ObjectGuid::LowType, Item*> ItemMap;

    ItemMap mMitems;                                    // 模板在 objectmgr.cpp 中定义

    Item* GetMItem(ObjectGuid::LowType itemLowGuid) // 获取指定低GUID的物品
    {
        ItemMap::const_iterator itr = mMitems.find(itemLowGuid);
        return itr != mMitems.end() ? itr->second : nullptr;
    }

    void AddMItem(Item* it) // 添加物品到邮件物品列表
    {
        ASSERT(it);
        // ASSERT 已删除，因为物品可以在加载前添加
        mMitems[it->GetGUID().GetCounter()] = it;
    }

    bool RemoveMItem(ObjectGuid::LowType itemLowGuid) // 从邮件物品列表中移除指定物品
    {
        return !!mMitems.erase(itemLowGuid);
    }

    void PetSpellInitialize(); // 宠物法术初始化
    void CharmSpellInitialize(); // 控制法术初始化
    void PossessSpellInitialize(); // 占据法术初始化
    void VehicleSpellInitialize(); // 载具法术初始化
    void SendRemoveControlBar(); // 发送移除控制条
    [[nodiscard]] bool HasSpell(uint32 spell) const override; // 检查是否拥有指定法术
    [[nodiscard]] bool HasActiveSpell(uint32 spell) const;            // 在法术书中显示
    TrainerSpellState GetTrainerSpellState(TrainerSpell const* trainer_spell) const; // 获取训练师法术状态
    [[nodiscard]] bool IsSpellFitByClassAndRace(uint32 spell_id) const; // 检查法术是否适合职业和种族
    bool IsNeedCastPassiveSpellAtLearn(SpellInfo const* spellInfo) const; // 检查学习时是否需要施放被动法术

    void SendProficiency(ItemClass itemClass, uint32 itemSubclassMask); // 发送熟练度
    void SendInitialSpells(); // 发送初始法术
    void SendLearnPacket(uint32 spellId, bool learn); // 发送学习法术包
    bool addSpell(uint32 spellId, uint8 addSpecMask, bool updateActive, bool temporary = false, bool learnFromSkill = false); // 添加法术
    bool _addSpell(uint32 spellId, uint8 addSpecMask, bool temporary, bool learnFromSkill = false); // 内部添加法术
    void learnSpell(uint32 spellId, bool temporary = false, bool learnFromSkill = false); // 学习法术
    void removeSpell(uint32 spellId, uint8 removeSpecMask, bool onlyTemporary); // 移除法术
    void resetSpells(); // 重置法术
    void LearnCustomSpells(); // 学习自定义法术
    void LearnDefaultSkills(); // 学习默认技能
    void LearnDefaultSkill(uint32 skillId, uint16 rank); // 学习默认技能等级
    void learnQuestRewardedSpells(); // 学习任务奖励法术
    void learnQuestRewardedSpells(Quest const* quest); // 学习特定任务奖励法术
    void learnSpellHighRank(uint32 spellid); // 学习高阶法术
    void SetReputation(uint32 factionentry, float value); // 设置声望
    [[nodiscard]] uint32 GetReputation(uint32 factionentry) const; // 获取声望
    std::string const& GetGuildName(); // 获取公会名称
    [[nodiscard]] uint32 GetFreeTalentPoints() const { return GetUInt32Value(PLAYER_CHARACTER_POINTS1); } // 获取可用天赋点数
    void SetFreeTalentPoints(uint32 points); // 设置可用天赋点数
    bool resetTalents(bool noResetCost = false); // 重置天赋
    [[nodiscard]] uint32 resetTalentsCost() const; // 获取重置天赋费用
    bool IsMaxLevel() const; // 检查是否达到最高等级
    void InitTalentForLevel(); // 根据等级初始化天赋
    void BuildPlayerTalentsInfoData(WorldPacket* data); // 构建玩家天赋信息数据包
    void BuildPetTalentsInfoData(WorldPacket* data); // 构建宠物天赋信息数据包
    void SendTalentsInfoData(bool pet); // 发送天赋信息数据包
    void LearnTalent(uint32 talentId, uint32 talentRank, bool command = false); // 学习天赋
    void LearnPetTalent(ObjectGuid petGuid, uint32 talentId, uint32 talentRank); // 学习宠物天赋

    bool addTalent(uint32 spellId, uint8 addSpecMask, uint8 oldTalentRank); // 添加天赋
    void _removeTalent(PlayerTalentMap::iterator& itr, uint8 specMask); // 移除天赋
    void _removeTalent(uint32 spellId, uint8 specMask); // 移除天赋
    void _removeTalentAurasAndSpells(uint32 spellId); // 移除天赋光环和法术
    void _addTalentAurasAndSpells(uint32 spellId); // 添加天赋光环和法术
    [[nodiscard]] bool HasTalent(uint32 spell_id, uint8 spec) const; // 检查是否拥有特定专精的天赋

    [[nodiscard]] uint32 CalculateTalentsPoints() const; // 计算已使用天赋点数
    void SetBonusTalentCount(uint32 count) { m_extraBonusTalentCount = count; }; // 设置额外天赋点数
    uint32 GetBonusTalentCount() { return m_extraBonusTalentCount; }; // 获取额外天赋点数
    void AddBonusTalent(uint32 count) { m_extraBonusTalentCount += count; }; // 增加额外天赋点数
    void RemoveBonusTalent(uint32 count) { m_extraBonusTalentCount -= count; }; // 减少额外天赋点数

    // 双专精
    void UpdateSpecCount(uint8 count); // 更新专精数量
    [[nodiscard]] uint8 GetActiveSpec() const { return m_activeSpec; } // 获取当前激活专精
    [[nodiscard]] uint8 GetActiveSpecMask() const { return (1 << m_activeSpec); } // 获取当前激活专精掩码
    void SetActiveSpec(uint8 spec) { m_activeSpec = spec; } // 设置当前激活专精
    [[nodiscard]] uint8 GetSpecsCount() const { return m_specsCount; } // 获取专精数量
    void SetSpecsCount(uint8 count) { m_specsCount = count; } // 设置专精数量
    void ActivateSpec(uint8 spec); // 激活专精
    void LoadActions(PreparedQueryResult result); // 加载动作
    void GetTalentTreePoints(uint8(&specPoints)[3]) const; // 获取天赋树点数
    [[nodiscard]] uint8 GetMostPointsTalentTree() const; // 获取点数最多的天赋树
    bool HasTankSpec(); // 检查是否有坦克专精
    bool HasMeleeSpec(); // 检查是否有近战专精
    bool HasCasterSpec(); // 检查是否有施法者专精
    bool HasHealSpec(); // 检查是否有治疗专精
    uint32 GetSpec(int8 spec = -1); // 获取专精

    void InitGlyphsForLevel(); // 根据等级初始化雕文
    void SetGlyphSlot(uint8 slot, uint32 slottype) { SetUInt32Value(PLAYER_FIELD_GLYPH_SLOTS_1 + slot, slottype); } // 设置雕文槽位
    [[nodiscard]] uint32 GetGlyphSlot(uint8 slot) const { return GetUInt32Value(PLAYER_FIELD_GLYPH_SLOTS_1 + slot); } // 获取雕文槽位
    void SetGlyph(uint8 slot, uint32 glyph, bool save) // 设置雕文
    {
        m_Glyphs[m_activeSpec][slot] = glyph;
        SetUInt32Value(PLAYER_FIELD_GLYPHS_1 + slot, glyph);

        if (save)
            SetNeedToSaveGlyphs(true);
    }
    [[nodiscard]] uint32 GetGlyph(uint8 slot) const { return m_Glyphs[m_activeSpec][slot]; } // 获取雕文

    [[nodiscard]] uint32 GetFreePrimaryProfessionPoints() const { return GetUInt32Value(PLAYER_CHARACTER_POINTS2); } // 获取可用专业技能点数
    void SetFreePrimaryProfessions(uint16 profs) { SetUInt32Value(PLAYER_CHARACTER_POINTS2, profs); } // 设置可用专业技能
    void InitPrimaryProfessions(); // 初始化主专业

    [[nodiscard]] PlayerSpellMap const& GetSpellMap() const { return m_spells; } // 获取法术映射
    PlayerSpellMap& GetSpellMap() { return m_spells; } // 获取法术映射

    [[nodiscard]] SpellCooldowns const& GetSpellCooldownMap() const { return m_spellCooldowns; } // 获取法术冷却映射
    SpellCooldowns& GetSpellCooldownMap() { return m_spellCooldowns; } // 获取法术冷却映射

    SkillStatusMap const& GetSkillStatusMap() const { return mSkillStatus; } // 获取技能状态映射
    SkillStatusMap& GetSkillStatusMap() { return mSkillStatus; } // 获取技能状态映射

    void AddSpellMod(SpellModifier* mod, bool apply); // 添加法术修改
    bool IsAffectedBySpellmod(SpellInfo const* spellInfo, SpellModifier* mod, Spell* spell = nullptr); // 检查是否受法术修改影响
    bool HasSpellMod(SpellModifier* mod, Spell* spell); // 检查是否有法术修改
    template <class T>
    void ApplySpellMod(uint32 spellId, SpellModOp op, T& basevalue, Spell* spell = nullptr, bool temporaryPet = false); // 应用法术修改
    void RemoveSpellMods(Spell* spell); // 移除法术修改
    void RestoreSpellMods(Spell* spell, uint32 ownerAuraId = 0, Aura* aura = nullptr); // 恢复法术修改
    void RestoreAllSpellMods(uint32 ownerAuraId = 0, Aura* aura = nullptr); // 恢复所有法术修改
    void DropModCharge(SpellModifier* mod, Spell* spell); // 减少修改充能
    void SetSpellModTakingSpell(Spell* spell, bool apply); // 设置法术修改施法

    [[nodiscard]] bool HasSpellCooldown(uint32 spell_id) const override; // 检查法术冷却
    [[nodiscard]] bool HasSpellItemCooldown(uint32 spell_id, uint32 itemid) const override; // 检查法术物品冷却
    [[nodiscard]] uint32 GetSpellCooldownDelay(uint32 spell_id) const; // 获取法术冷却延迟
    void AddSpellAndCategoryCooldowns(SpellInfo const* spellInfo, uint32 itemId, Spell* spell = nullptr, bool infinityCooldown = false); // 添加法术和类别冷却
    void AddSpellCooldown(uint32 spell_id, uint32 itemid, uint32 end_time, bool needSendToClient = false, bool forceSendToSpectator = false) override; // 添加法术冷却
    void _AddSpellCooldown(uint32 spell_id, uint16 categoryId, uint32 itemid, uint32 end_time, bool needSendToClient = false, bool forceSendToSpectator = false); // 内部添加法术冷却
    void ModifySpellCooldown(uint32 spellId, int32 cooldown); // 修改法术冷却
    void SendCooldownEvent(SpellInfo const* spellInfo, uint32 itemId = 0, Spell* spell = nullptr, bool setCooldown = true); // 发送冷却事件
    void ProhibitSpellSchool(SpellSchoolMask idSchoolMask, uint32 unTimeMs) override; // 禁止法术学派
    void RemoveSpellCooldown(uint32 spell_id, bool update = false); // 移除法术冷却
    void SendClearCooldown(uint32 spell_id, Unit* target); // 发送清除冷却

    GlobalCooldownMgr& GetGlobalCooldownMgr() { return m_GlobalCooldownMgr; } // 获取全局冷却管理器

    void RemoveCategoryCooldown(uint32 cat); // 移除类别冷却
    void RemoveArenaSpellCooldowns(bool removeActivePetCooldowns = false); // 移除竞技场法术冷却
    void RemoveAllSpellCooldown(); // 移除所有法术冷却
    void _LoadSpellCooldowns(PreparedQueryResult result); // 加载法术冷却
    void _SaveSpellCooldowns(CharacterDatabaseTransaction trans, bool logout); // 保存法术冷却
    uint32 GetLastPotionId() { return m_lastPotionId; } // 获取最后使用的药水ID
    void SetLastPotionId(uint32 item_id) { m_lastPotionId = item_id; } // 设置最后使用的药水ID
    void UpdatePotionCooldown(Spell* spell = nullptr); // 更新药水冷却

    void setResurrectRequestData(ObjectGuid guid, uint32 mapId, float X, float Y, float Z, uint32 health, uint32 mana) // 设置复活请求数据
    {
        m_resurrectGUID = guid;
        m_resurrectMap = mapId;
        m_resurrectX = X;
        m_resurrectY = Y;
        m_resurrectZ = Z;
        m_resurrectHealth = health;
        m_resurrectMana = mana;
    }
    void clearResurrectRequestData() { setResurrectRequestData(ObjectGuid::Empty, 0, 0.0f, 0.0f, 0.0f, 0, 0); } // 清除复活请求数据
    [[nodiscard]] bool isResurrectRequestedBy(ObjectGuid guid) const { return m_resurrectGUID && m_resurrectGUID == guid; } // 检查是否由指定GUID请求复活
    [[nodiscard]] bool isResurrectRequested() const { return m_resurrectGUID; } // 检查是否有复活请求
    void ResurectUsingRequestData(); // 使用请求数据复活

    [[nodiscard]] uint8 getCinematic() const
    {
        return m_cinematic;
    }
    void setCinematic(uint8 cine)
    {
        m_cinematic = cine;
    }

    ActionButton* addActionButton(uint8 button, uint32 action, uint8 type); // 添加动作按钮
    void removeActionButton(uint8 button); // 移除动作按钮
    ActionButton const* GetActionButton(uint8 button); // 获取动作按钮
    void SendInitialActionButtons() const { SendActionButtons(1); } // 发送初始动作按钮
    void SendActionButtons(uint32 state) const; // 发送动作按钮状态
    bool IsActionButtonDataValid(uint8 button, uint32 action, uint8 type); // 检查动作按钮数据有效性

    PvPInfo pvpInfo; // PvP信息
    void UpdatePvPState(); // 更新PvP状态
    void UpdateFFAPvPState(bool reset = true); // 更新自由PvP状态
    void SetPvP(bool state) // 设置PvP状态
    {
        Unit::SetPvP(state);
        if (!m_Controlled.empty())
            for (auto& itr : m_Controlled)
                itr->SetPvP(state);
    }
    void UpdatePvP(bool state, bool _override = false); // 更新PvP状态
    void UpdateZone(uint32 newZone, uint32 newArea, bool force = false); // 更新区域
    void UpdateArea(uint32 newArea); // 更新子区域
    void SetNeedZoneUpdate(bool needUpdate) { m_needZoneUpdate = needUpdate; } // 设置需要区域更新

    void UpdateZoneDependentAuras(uint32 zone_id);    // 更新区域依赖的光环
    void UpdateAreaDependentAuras(uint32 area_id);    // 更新子区域依赖的光环

    void UpdateAfkReport(time_t currTime); // 更新AFK报告
    void UpdatePvPFlag(time_t currTime); // 更新PvP标志
    void UpdateFFAPvPFlag(time_t currTime); // 更新自由PvP标志
    void UpdateContestedPvP(uint32 currTime); // 更新争夺PvP
    void SetContestedPvPTimer(uint32 newTime) { m_contestedPvPTimer = newTime; } // 设置争夺PvP计时器
    void ResetContestedPvP() // 重置争夺PvP
    {
        ClearUnitState(UNIT_STATE_ATTACK_PLAYER);
        RemovePlayerFlag(PLAYER_FLAGS_CONTESTED_PVP);
        m_contestedPvPTimer = 0;
    }

    /** todo: -maybe move UpdateDuelFlag+DuelComplete to independent DuelHandler.. **/
    std::unique_ptr<DuelInfo> duel; // 决斗信息
    void UpdateDuelFlag(time_t currTime); // 更新决斗标志
    void CheckDuelDistance(time_t currTime); // 检查决斗距离
    void DuelComplete(DuelCompleteType type); // 决斗完成
    void SendDuelCountdown(uint32 counter); // 发送决斗倒计时

    bool IsGroupVisibleFor(Player const* p) const; // 检查是否对指定玩家可见
    bool IsInSameGroupWith(Player const* p) const; // 检查是否在同一队伍
    bool IsInSameRaidWith(Player const* p) const { return p == this || (GetGroup() != nullptr && GetGroup() == p->GetGroup()); } // 检查是否在同一团队
    void UninviteFromGroup(); // 从队伍中移除
    static void RemoveFromGroup(Group* group, ObjectGuid guid, RemoveMethod method = GROUP_REMOVEMETHOD_DEFAULT, ObjectGuid kicker = ObjectGuid::Empty, const char* reason = nullptr); // 从队伍中移除
    void RemoveFromGroup(RemoveMethod method = GROUP_REMOVEMETHOD_DEFAULT) { RemoveFromGroup(GetGroup(), GetGUID(), method); } // 从队伍中移除
    void SendUpdateToOutOfRangeGroupMembers(); // 发送更新给范围外的队伍成员

    void SetInGuild(uint32 GuildId) // 设置公会ID
    {
        SetUInt32Value(PLAYER_GUILDID, GuildId);
        // xinef: 更新全局存储
        sCharacterCache->UpdateCharacterGuildId(GetGUID(), GetGuildId());
    }
    void SetRank(uint8 rankId) { SetUInt32Value(PLAYER_GUILDRANK, rankId); } // 设置公会等级
    [[nodiscard]] uint8 GetRank() const { return uint8(GetUInt32Value(PLAYER_GUILDRANK)); } // 获取公会等级
    void SetGuildIdInvited(uint32 GuildId) { m_GuildIdInvited = GuildId; } // 设置邀请的公会ID
    [[nodiscard]] uint32 GetGuildId() const { return GetUInt32Value(PLAYER_GUILDID); } // 获取公会ID
    [[nodiscard]] Guild* GetGuild() const; // 获取公会
    uint32 GetGuildIdInvited() { return m_GuildIdInvited; } // 获取邀请的公会ID
    static void RemovePetitionsAndSigns(ObjectGuid guid, uint32 type); // 移除请愿和签名

    // 竞技场队伍
    void SetInArenaTeam(uint32 ArenaTeamId, uint8 slot, uint8 type) // 设置竞技场队伍信息
    {
        SetArenaTeamInfoField(slot, ARENA_TEAM_ID, ArenaTeamId);
        SetArenaTeamInfoField(slot, ARENA_TEAM_TYPE, type);
    }
    void SetArenaTeamInfoField(uint8 slot, ArenaTeamInfoType type, uint32 value); // 设置竞技场队伍字段
    [[nodiscard]] uint32 GetArenaPersonalRating(uint8 slot) const; // 获取个人竞技场评分
    static uint32 GetArenaTeamIdFromDB(ObjectGuid guid, uint8 slot); // 从数据库获取竞技场队伍ID
    static void LeaveAllArenaTeams(ObjectGuid guid); // 离开所有竞技场队伍
    [[nodiscard]] uint32 GetArenaTeamId(uint8 slot) const; // 获取竞技场队伍ID
    void SetArenaTeamIdInvited(uint32 ArenaTeamId) { m_ArenaTeamIdInvited = ArenaTeamId; } // 设置邀请的竞技场队伍ID
    uint32 GetArenaTeamIdInvited() { return m_ArenaTeamIdInvited; } // 获取邀请的竞技场队伍ID

    [[nodiscard]] Difficulty GetDifficulty(bool isRaid) const { return isRaid ? m_raidDifficulty : m_dungeonDifficulty; } // 获取难度
    [[nodiscard]] Difficulty GetDungeonDifficulty() const { return m_dungeonDifficulty; } // 获取副本难度
    [[nodiscard]] Difficulty GetRaidDifficulty() const { return m_raidDifficulty; } // 获取团队副本难度
    [[nodiscard]] Difficulty GetStoredRaidDifficulty() const { return m_raidMapDifficulty; } // 获取存储的团队副本难度
    void SetDungeonDifficulty(Difficulty dungeon_difficulty) { m_dungeonDifficulty = dungeon_difficulty; } // 设置副本难度
    void SetRaidDifficulty(Difficulty raid_difficulty) { m_raidDifficulty = raid_difficulty; } // 设置团队副本难度
    void StoreRaidMapDifficulty() { m_raidMapDifficulty = GetMap()->GetDifficulty(); } // 存储团队副本难度

    bool UpdateSkill(uint32 skill_id, uint32 step); // 更新技能
    bool UpdateSkillPro(uint16 SkillId, int32 Chance, uint32 step); // 更新技能熟练度

    bool UpdateCraftSkill(uint32 spellid); // 更新制造技能
    bool UpdateGatherSkill(uint32 SkillId, uint32 SkillValue, uint32 RedLevel, uint32 Multiplicator = 1); // 更新采集技能
    bool UpdateFishingSkill(); // 更新钓鱼技能

    [[nodiscard]] uint32 GetBaseDefenseSkillValue() const { return GetBaseSkillValue(SKILL_DEFENSE); } // 获取基础防御技能值
    [[nodiscard]] uint32 GetBaseWeaponSkillValue(WeaponAttackType attType) const; // 获取基础武器技能值

    uint32 GetSpellByProto(ItemTemplate* proto); // 获取物品原型对应的法术

    float GetHealthBonusFromStamina(); // 获取耐力带来的生命值加成
    float GetManaBonusFromIntellect(); // 获取智力带来的法力值加成

    bool UpdateStats(Stats stat) override; // 更新属性
    bool UpdateAllStats() override; // 更新所有属性
    void ApplySpellPenetrationBonus(int32 amount, bool apply); // 应用法术穿透加成
    void UpdateResistances(uint32 school) override; // 更新抗性
    void UpdateArmor() override; // 更新护甲
    void UpdateMaxHealth() override; // 更新最大生命值
    void UpdateMaxPower(Powers power) override; // 更新最大资源值
    void ApplyFeralAPBonus(int32 amount, bool apply); // 应用野性攻击强度加成
    void UpdateAttackPowerAndDamage(bool ranged = false) override; // 更新攻击强度和伤害
    void UpdateShieldBlockValue(); // 更新盾牌格挡值
    void ApplySpellPowerBonus(int32 amount, bool apply); // 应用法术强度加成
    void UpdateSpellDamageAndHealingBonus(); // 更新法术伤害和治疗加成
    void ApplyRatingMod(CombatRating cr, int32 value, bool apply); // 应用战斗评分修改
    void UpdateRating(CombatRating cr); // 更新战斗评分
    void UpdateAllRatings(); // 更新所有战斗评分

    void CalculateMinMaxDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, float& minDamage, float& maxDamage, uint8 damageIndex) override; // 计算最小最大伤害

    void UpdateDefenseBonusesMod(); // 更新防御加成
    inline void RecalculateRating(CombatRating cr) { ApplyRatingMod(cr, 0, true); } // 重新计算战斗评分
    float GetMeleeCritFromAgility(); // 获取敏捷带来的近战暴击
    void GetDodgeFromAgility(float& diminishing, float& nondiminishing); // 获取敏捷带来的闪避
    [[nodiscard]] float GetMissPercentageFromDefence() const; // 获取防御带来的未命中百分比
    float GetSpellCritFromIntellect(); // 获取智力带来的法术暴击
    float OCTRegenHPPerSpirit(); // 获取精神带来的生命值恢复
    float OCTRegenMPPerSpirit(); // 获取精神带来的法力值恢复
    [[nodiscard]] float GetRatingMultiplier(CombatRating cr) const; // 获取战斗评分乘数
    [[nodiscard]] float GetRatingBonusValue(CombatRating cr) const; // 获取战斗评分加成值
    uint32 GetBaseSpellPowerBonus() { return m_baseSpellPower; } // 获取基础法术强度
    [[nodiscard]] int32 GetSpellPenetrationItemMod() const { return m_spellPenetrationItemMod; } // 获取物品带来的法术穿透

    [[nodiscard]] float GetExpertiseDodgeOrParryReduction(WeaponAttackType attType) const; // 获取专精带来的闪避或招架减少
    void UpdateBlockPercentage(); // 更新格挡百分比
    void UpdateCritPercentage(WeaponAttackType attType); // 更新暴击百分比
    void UpdateAllCritPercentages(); // 更新所有暴击百分比
    void UpdateParryPercentage(); // 更新招架百分比
    void UpdateDodgePercentage(); // 更新闪避百分比
    void UpdateMeleeHitChances(); // 更新近战命中几率
    void UpdateRangedHitChances(); // 更新远程命中几率
    void UpdateSpellHitChances(); // 更新法术命中几率

    void UpdateAllSpellCritChances(); // 更新所有法术暴击几率
    void UpdateSpellCritChance(uint32 school); // 更新特定学派法术暴击几率
    void UpdateArmorPenetration(int32 amount); // 更新护甲穿透
    void UpdateExpertise(WeaponAttackType attType); // 更新专精
    void ApplyManaRegenBonus(int32 amount, bool apply); // 应用法力恢复加成
    void ApplyHealthRegenBonus(int32 amount, bool apply); // 应用生命恢复加成
    void UpdateManaRegen(); // 更新法力恢复
    void UpdateEnergyRegen(); // 更新能量恢复
    void UpdateRuneRegen(RuneType rune); // 更新符文恢复

    [[nodiscard]] ObjectGuid GetLootGUID() const { return m_lootGuid; } // 获取掠夺GUID
    void SetLootGUID(ObjectGuid guid) { m_lootGuid = guid; } // 设置掠夺GUID

    void RemovedInsignia(Player* looterPlr); // 移除徽章

    [[nodiscard]] WorldSession* GetSession() const { return m_session; } // 获取会话
    void SetSession(WorldSession* sess) { m_session = sess; } // 设置会话

    void BuildCreateUpdateBlockForPlayer(UpdateData* data, Player* target) override; // 为玩家构建创建更新块
    void DestroyForPlayer(Player* target, bool onDeath = false) const override; // 销毁玩家
    void SendLogXPGain(uint32 GivenXP, Unit* victim, uint32 BonusXP, bool recruitAFriend = false, float group_rate = 1.0f); // 发送经验获得日志

    // 通知器
    void SendAttackSwingCantAttack(); // 发送无法攻击
    void SendAttackSwingCancelAttack(); // 发送取消攻击
    void SendAttackSwingDeadTarget(); // 发送目标死亡
    void SendAttackSwingNotInRange(); // 发送目标不在范围内
    void SendAttackSwingBadFacingAttack(); // 发送面对方向错误
    void SendAutoRepeatCancel(Unit* target); // 发送自动重复取消
    void SendExplorationExperience(uint32 Area, uint32 Experience); // 发送探索经验

    void SendDungeonDifficulty(bool IsInGroup); // 发送副本难度
    void SendRaidDifficulty(bool IsInGroup, int32 forcedDifficulty = -1); // 发送团队副本难度
    static void ResetInstances(ObjectGuid guid, uint8 method, bool isRaid); // 重置实例
    void SendResetInstanceSuccess(uint32 MapId); // 发送实例重置成功
    void SendResetInstanceFailed(uint32 reason, uint32 MapId); // 发送实例重置失败
    void SendResetFailedNotify(uint32 mapid); // 发送重置失败通知

    bool UpdatePosition(float x, float y, float z, float orientation, bool teleport = false) override; // 更新位置
    bool UpdatePosition(const Position& pos, bool teleport = false) { return UpdatePosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), teleport); } // 更新位置

    void ProcessTerrainStatusUpdate() override; // 处理地形状态更新

    void SendMessageToSet(WorldPacket const* data, bool self) const override; // 向集合发送消息
    void SendMessageToSetInRange(WorldPacket const* data, float dist, bool self) const override; // 向范围内集合发送消息
    void SendMessageToSetInRange(WorldPacket const* data, float dist, bool self, bool includeMargin, bool ownTeamOnly, bool required3dDist = false) const; // 向范围内集合发送消息
    void SendMessageToSet(WorldPacket const* data, Player const* skipped_rcvr) const override; // 向集合发送消息并跳过指定接收者
    void SendTeleportAckPacket(); // 发送传送确认包

    [[nodiscard]] Corpse* GetCorpse() const; // 获取尸体
    void SpawnCorpseBones(bool triggerSave = true); // 生成尸体骨头
    Corpse* CreateCorpse(); // 创建尸体
    void RemoveCorpse(); // 移除尸体
    void KillPlayer(); // 杀死玩家
    static void OfflineResurrect(ObjectGuid const guid, CharacterDatabaseTransaction trans); // 离线复活
    [[nodiscard]] bool HasCorpse() const { return _corpseLocation.GetMapId() != MAPID_INVALID; } // 检查是否有尸体
    [[nodiscard]] WorldLocation GetCorpseLocation() const { return _corpseLocation; } // 获取尸体位置
    uint32 GetResurrectionSpellId(); // 获取复活法术ID
    void ResurrectPlayer(float restore_percent, bool applySickness = false); // 复活玩家
    void BuildPlayerRepop(); // 构建玩家复活
    void RepopAtGraveyard(); // 在墓地复活

    void SendDurabilityLoss(); // 发送耐久度损失
    void DurabilityLossAll(double percent, bool inventory); // 全部耐久度损失
    void DurabilityLoss(Item* item, double percent); // 物品耐久度损失
    void DurabilityPointsLossAll(int32 points, bool inventory); // 全部耐久度点数损失
    void DurabilityPointsLoss(Item* item, int32 points); // 物品耐久度点数损失
    void DurabilityPointLossForEquipSlot(EquipmentSlots slot); // 装备槽位耐久度点数损失
    uint32 DurabilityRepairAll(bool cost, float discountMod, bool guildBank); // 全部修理
    uint32 DurabilityRepair(uint16 pos, bool cost, float discountMod, bool guildBank); // 修理

    void UpdateMirrorTimers(); // 更新镜像计时器
    void StopMirrorTimers() // 停止镜像计时器
    {
        StopMirrorTimer(FATIGUE_TIMER);
        StopMirrorTimer(BREATH_TIMER);
        StopMirrorTimer(FIRE_TIMER);
    }
    bool IsMirrorTimerActive(MirrorTimerType type) { return m_MirrorTimer[type] == getMaxTimer(type); } // 检查镜像计时器是否激活

    void SetMovement(PlayerMovementType pType); // 设置移动类型

    bool CanJoinConstantChannelInZone(ChatChannelsEntry const* channel, AreaTableEntry const* zone); // 检查是否可以加入常量频道

    void JoinedChannel(Channel* c); // 加入频道
    void LeftChannel(Channel* c); // 离开频道
    void CleanupChannels(); // 清理频道
    void ClearChannelWatch(); // 清除频道监视
    void UpdateLFGChannel(); // 更新寻找队伍频道
    void UpdateLocalChannels(uint32 newZone); // 更新本地频道

    void UpdateDefense(); // 更新防御
    void UpdateWeaponSkill(Unit* victim, WeaponAttackType attType, Item* item = nullptr); // 更新武器技能
    void UpdateCombatSkills(Unit* victim, WeaponAttackType attType, bool defence, Item* item = nullptr); // 更新战斗技能

    void SetSkill(uint16 id, uint16 step, uint16 currVal, uint16 maxVal); // 设置技能
    [[nodiscard]] uint16 GetMaxSkillValue(uint32 skill) const;        // 获取最大技能值
    [[nodiscard]] uint16 GetPureMaxSkillValue(uint32 skill) const;    // 获取纯最大技能值
    [[nodiscard]] uint16 GetSkillValue(uint32 skill) const;           // 获取技能值
    [[nodiscard]] uint16 GetBaseSkillValue(uint32 skill) const;       // 获取基础技能值
    [[nodiscard]] uint16 GetPureSkillValue(uint32 skill) const;       // 获取纯技能值
    [[nodiscard]] int16 GetSkillPermBonusValue(uint32 skill) const; // 获取永久技能加成
    [[nodiscard]] int16 GetSkillTempBonusValue(uint32 skill) const; // 获取临时技能加成
    [[nodiscard]] uint16 GetSkillStep(uint16 skill) const;            // 获取技能步骤
    [[nodiscard]] bool HasSkill(uint32 skill) const; // 检查是否有技能
    void learnSkillRewardedSpells(uint32 id, uint32 value); // 学习技能奖励法术

    WorldLocation& GetTeleportDest() { return teleportStore_dest; } // 获取传送目标
    [[nodiscard]] bool IsBeingTeleported() const { return mSemaphoreTeleport_Near != 0 || mSemaphoreTeleport_Far != 0; } // 检查是否正在传送
    [[nodiscard]] bool IsBeingTeleportedNear() const { return mSemaphoreTeleport_Near != 0; } // 检查是否正在近传
    [[nodiscard]] bool IsBeingTeleportedFar() const { return mSemaphoreTeleport_Far != 0; } // 检查是否正在远传
    void SetSemaphoreTeleportNear(time_t tm) { mSemaphoreTeleport_Near = tm; } // 设置近传信号量
    void SetSemaphoreTeleportFar(time_t tm) { mSemaphoreTeleport_Far = tm; } // 设置远传信号量
    [[nodiscard]] time_t GetSemaphoreTeleportNear() const { return mSemaphoreTeleport_Near; } // 获取近传信号量
    [[nodiscard]] time_t GetSemaphoreTeleportFar() const { return mSemaphoreTeleport_Far; } // 获取远传信号量
    void ProcessDelayedOperations(); // 处理延迟操作
    [[nodiscard]] uint32 GetDelayedOperations() const { return m_DelayedOperations; } // 获取延迟操作
    void ScheduleDelayedOperation(uint32 operation) // 安排延迟操作
    {
        if (operation < DELAYED_END)
            m_DelayedOperations |= operation;
    }

    void CheckAreaExploreAndOutdoor(); // 检查区域探索和户外

    static TeamId TeamIdForRace(uint8 race); // 根据种族获取团队ID
    [[nodiscard]] TeamId GetTeamId(bool original = false) const { return original ? TeamIdForRace(getRace(true)) : m_team; }; // 获取团队ID
    void SetFactionForRace(uint8 race); // 设置种族对应阵营
    void setTeamId(TeamId teamid) { m_team = teamid; }; // 设置团队ID

    void InitDisplayIds(); // 初始化显示ID

    bool IsAtGroupRewardDistance(WorldObject const* pRewardSource) const; // 检查是否在组奖励距离内
    bool IsAtLootRewardDistance(WorldObject const* pRewardSource) const; // 检查是否在掠夺奖励距离内
    bool IsAtRecruitAFriendDistance(WorldObject const* pOther) const; // 检查是否在招募好友距离内
    void RewardPlayerAndGroupAtKill(Unit* victim, bool isBattleGround); // 在击杀时奖励玩家和队伍
    void RewardPlayerAndGroupAtEvent(uint32 creature_id, WorldObject* pRewardSource); // 在事件时奖励玩家和队伍
    bool isHonorOrXPTarget(Unit* victim) const; // 检查是否是荣誉或经验目标

    bool GetsRecruitAFriendBonus(bool forXP); // 检查是否获得招募好友加成
    uint8 GetGrantableLevels() { return m_grantableLevels; } // 获取可授予等级
    void SetGrantableLevels(uint8 val) { m_grantableLevels = val; } // 设置可授予等级

    ReputationMgr& GetReputationMgr() { return *m_reputationMgr; } // 获取声望管理器
    [[nodiscard]] ReputationMgr const& GetReputationMgr() const { return *m_reputationMgr; } // 获取声望管理器
    [[nodiscard]] ReputationRank GetReputationRank(uint32 faction_id) const; // 获取声望等级
    void RewardReputation(Unit* victim); // 奖励声望
    void RewardReputation(Quest const* quest); // 奖励声望

    float CalculateReputationGain(ReputationSource source, uint32 creatureOrQuestLevel, float rep, int32 faction, bool noQuestBonus = false); // 计算声望获得

    void UpdateSkillsForLevel(); // 根据等级更新技能
    void UpdateSkillsToMaxSkillsForLevel();             // 用于.levelup命令
    void ModifySkillBonus(uint32 skillid, int32 val, bool talent); // 修改技能加成

    /**
     * 天赋点数奖励。
     * 使用方式：
     * 1). 热更新情况（发生在角色在线时，例如 PlayerScript:OnAchiComplete）:
     *     调用此函数后，角色可以通过调用函数 player->InitTalentForLevel() 来奖励天赋点数。
     *
     * 2). 数据初始化情况（例如 PlayerScript:OnLoadFromDB）
     */
    void RewardExtraBonusTalentPoints(uint32 bonusTalentPoints); // 奖励额外天赋点数

    /*********************************************************/
    /***                  PVP SYSTEM                       ***/
    /*********************************************************/
    void UpdateHonorFields(); // 更新荣誉字段
    bool RewardHonor(Unit* victim, uint32 groupsize, int32 honor = -1, bool awardXP = true); // 奖励荣誉
    [[nodiscard]] uint32 GetHonorPoints() const { return GetUInt32Value(PLAYER_FIELD_HONOR_CURRENCY); } // 获取荣誉点数
    [[nodiscard]] uint32 GetArenaPoints() const { return GetUInt32Value(PLAYER_FIELD_ARENA_CURRENCY); } // 获取竞技场点数
    void ModifyHonorPoints(int32 value, CharacterDatabaseTransaction trans = CharacterDatabaseTransaction(nullptr));      //! 如果指定了trans，则荣誉保存查询将添加到trans
    void ModifyArenaPoints(int32 value, CharacterDatabaseTransaction trans = CharacterDatabaseTransaction(nullptr));      //! 如果指定了trans，则竞技场点数保存查询将添加到trans
    [[nodiscard]] uint32 GetMaxPersonalArenaRatingRequirement(uint32 minarenaslot) const; // 获取个人竞技场评分要求
    void SetHonorPoints(uint32 value); // 设置荣誉点数
    void SetArenaPoints(uint32 value); // 设置竞技场点数

    // 决斗健康和法力重置方法
    void SaveHealthBeforeDuel() { healthBeforeDuel = GetHealth(); } // 保存决斗前健康值
    void SaveManaBeforeDuel() { manaBeforeDuel = GetPower(POWER_MANA); } // 保存决斗前法力值
    void RestoreHealthAfterDuel() { SetHealth(healthBeforeDuel); } // 恢复决斗后健康值
    void RestoreManaAfterDuel() { SetPower(POWER_MANA, manaBeforeDuel); } // 恢复决斗后法力值

    //PvP系统结束

    [[nodiscard]] inline SpellCooldowns GetSpellCooldowns() const { return m_spellCooldowns; } // 获取法术冷却

    void SetDrunkValue(uint8 newDrunkValue, uint32 itemId = 0); // 设置醉酒值
    [[nodiscard]] uint8 GetDrunkValue() const { return GetByteValue(PLAYER_BYTES_3, 1); } // 获取醉酒值
    [[nodiscard]] int32 GetFakeDrunkValue() const { return GetInt32Value(PLAYER_FAKE_INEBRIATION); } // 获取假醉酒值
    void UpdateInvisibilityDrunkDetect(); // 更新隐身醉酒检测
    static DrunkenState GetDrunkenstateByValue(uint8 value); // 根据值获取醉酒状态

    [[nodiscard]] uint32 GetDeathTimer() const { return m_deathTimer; } // 获取死亡计时器
    [[nodiscard]] uint32 GetCorpseReclaimDelay(bool pvp) const; // 获取尸体回收延迟
    void UpdateCorpseReclaimDelay(); // 更新尸体回收延迟
    int32 CalculateCorpseReclaimDelay(bool load = false); // 计算尸体回收延迟
    void SendCorpseReclaimDelay(uint32 delay); // 发送尸体回收延迟

    [[nodiscard]] uint32 GetShieldBlockValue() const override;                 // 覆盖Unit版本（虚拟）
    [[nodiscard]] bool CanParry() const { return m_canParry; } // 检查是否可以招架
    void SetCanParry(bool value); // 设置是否可以招架
    [[nodiscard]] bool CanBlock() const { return m_canBlock; } // 检查是否可以格挡
    void SetCanBlock(bool value); // 设置是否可以格挡
    [[nodiscard]] bool CanTitanGrip() const { return m_canTitanGrip; } // 检查是否可以泰坦之握
    void SetCanTitanGrip(bool value); // 设置是否可以泰坦之握
    [[nodiscard]] bool CanTameExoticPets() const { return IsGameMaster() || HasAuraType(SPELL_AURA_ALLOW_TAME_PET_TYPE); } // 检查是否可以驯养异域宠物

    void SetRegularAttackTime(); // 设置常规攻击时间
    void SetBaseModValue(BaseModGroup modGroup, BaseModType modType, float value) { m_auraBaseMod[modGroup][modType] = value; } // 设置基础修改值
    void HandleBaseModValue(BaseModGroup modGroup, BaseModType modType, float amount, bool apply); // 处理基础修改值
    [[nodiscard]] float GetBaseModValue(BaseModGroup modGroup, BaseModType modType) const; // 获取基础修改值
    [[nodiscard]] float GetTotalBaseModValue(BaseModGroup modGroup) const; // 获取总基础修改值
    [[nodiscard]] float GetTotalPercentageModValue(BaseModGroup modGroup) const { return m_auraBaseMod[modGroup][FLAT_MOD] + m_auraBaseMod[modGroup][PCT_MOD]; } // 获取总百分比修改值
    void _ApplyAllStatBonuses(); // 应用所有属性加成
    void _RemoveAllStatBonuses(); // 移除所有属性加成

    void ResetAllPowers(); // 重置所有资源

    SpellSchoolMask GetMeleeDamageSchoolMask(WeaponAttackType attackType = BASE_ATTACK, uint8 damageIndex = 0) const override; // 获取近战伤害学派掩码

    void _ApplyWeaponDependentAuraMods(Item* item, WeaponAttackType attackType, bool apply); // 应用武器依赖的光环修改
    void _ApplyWeaponDependentAuraCritMod(Item* item, WeaponAttackType attackType, AuraEffect const* aura, bool apply); // 应用武器依赖的暴击光环修改
    void _ApplyWeaponDependentAuraDamageMod(Item* item, WeaponAttackType attackType, AuraEffect const* aura, bool apply); // 应用武器依赖的伤害光环修改

    void _ApplyItemMods(Item* item, uint8 slot, bool apply); // 应用物品修改
    void _RemoveAllItemMods(); // 移除所有物品修改
    void _ApplyAllItemMods(); // 应用所有物品修改
    void _ApplyAllLevelScaleItemMods(bool apply); // 应用所有等级缩放物品修改
    void _ApplyItemBonuses(ItemTemplate const* proto, uint8 slot, bool apply, bool only_level_scale = false); // 应用物品加成
    void _ApplyWeaponDamage(uint8 slot, ItemTemplate const* proto, ScalingStatValuesEntry const* ssv, bool apply); // 应用武器伤害
    void _ApplyAmmoBonuses(); // 应用弹药加成
    bool EnchantmentFitsRequirements(uint32 enchantmentcondition, int8 slot); // 检查附魔是否符合要求
    void ToggleMetaGemsActive(uint8 exceptslot, bool apply); // 切换元宝石激活
    void CorrectMetaGemEnchants(uint8 slot, bool apply); // 修正元宝石附魔
    void InitDataForForm(bool reapplyMods = false); // 初始化形态数据

    void ApplyItemEquipSpell(Item* item, bool apply, bool form_change = false); // 应用物品装备法术
    void ApplyEquipSpell(SpellInfo const* spellInfo, Item* item, bool apply, bool form_change = false); // 应用装备法术
    void UpdateEquipSpellsAtFormChange(); // 更新形态变化时的装备法术
    void CastItemCombatSpell(Unit* target, WeaponAttackType attType, uint32 procVictim, uint32 procEx); // 施放物品战斗法术
    void CastItemUseSpell(Item* item, SpellCastTargets const& targets, uint8 cast_count, uint32 glyphIndex); // 施放物品使用法术
    void CastItemCombatSpell(Unit* target, WeaponAttackType attType, uint32 procVictim, uint32 procEx, Item* item, ItemTemplate const* proto); // 施放物品战斗法术

    void SendEquipmentSetList(); // 发送装备集列表
    void SetEquipmentSet(uint32 index, EquipmentSet eqset); // 设置装备集
    void DeleteEquipmentSet(uint64 setGuid); // 删除装备集

    void SendInitWorldStates(uint32 zoneId, uint32 areaId); // 发送初始化世界状态
    void SendUpdateWorldState(uint32 variable, uint32 value) const; // 发送更新世界状态
    void SendDirectMessage(WorldPacket const* data) const; // 发送直接消息
    void SendBGWeekendWorldStates(); // 发送战场周末世界状态
    void SendBattlefieldWorldStates(); // 发送战场世界状态


    void GetAurasForTarget(Unit* target, bool force = false); // 获取指定目标的光环信息

    PlayerMenu* PlayerTalkClass; // 玩家交互菜单类
    std::vector<ItemSetEffect*> ItemSetEff; // 装备套装效果列表

    void SendLoot(ObjectGuid guid, LootType loot_type); // 发送战利品信息
    void SendLootError(ObjectGuid guid, LootError error); // 发送战利品错误信息
    void SendLootRelease(ObjectGuid guid); // 发送战利品释放信息
    void SendNotifyLootItemRemoved(uint8 lootSlot); // 通知战利品槽位物品被移除
    void SendNotifyLootMoneyRemoved(); // 通知战利品中的金钱被移除

    /*********************************************************/
    /***               BATTLEGROUND SYSTEM                 ***/
    /*********************************************************/

    [[nodiscard]] bool InBattleground() const { return m_bgData.bgInstanceID != 0; } // 判断是否在战场中
    [[nodiscard]] bool InArena() const; // 判断是否在竞技场中
    [[nodiscard]] uint32 GetBattlegroundId() const { return m_bgData.bgInstanceID; } // 获取战场实例ID
    [[nodiscard]] BattlegroundTypeId GetBattlegroundTypeId() const { return m_bgData.bgTypeID; } // 获取战场类型ID
    [[nodiscard]] uint32 GetCurrentBattlegroundQueueSlot() const { return m_bgData.bgQueueSlot; } // 获取当前战场队列槽位
    [[nodiscard]] bool IsInvitedForBattlegroundInstance() const { return m_bgData.isInvited; } // 判断是否被邀请进入战场实例
    [[nodiscard]] bool IsCurrentBattlegroundRandom() const { return m_bgData.bgIsRandom; } // 判断当前战场是否为随机战场
    BGData& GetBGData() { return m_bgData; } // 获取战场数据
    void SetBGData(BGData& bgdata) { m_bgData = bgdata; } // 设置战场数据
    [[nodiscard]] Battleground* GetBattleground(bool create = false) const; // 获取战场对象，必要时可创建

    [[nodiscard]] bool InBattlegroundQueue(bool ignoreArena = false) const; // 判断是否处于战场队列中
    [[nodiscard]] bool IsDeserter() const { return HasAura(26013); } // 判断是否为逃兵

    [[nodiscard]] BattlegroundQueueTypeId GetBattlegroundQueueTypeId(uint32 index) const; // 根据索引获取战场队列类型ID
    [[nodiscard]] uint32 GetBattlegroundQueueIndex(BattlegroundQueueTypeId bgQueueTypeId) const; // 根据战场队列类型ID获取索引
    [[nodiscard]] bool IsInvitedForBattlegroundQueueType(BattlegroundQueueTypeId bgQueueTypeId) const; // 判断是否被邀请进入指定类型的战场队列
    [[nodiscard]] bool InBattlegroundQueueForBattlegroundQueueType(BattlegroundQueueTypeId bgQueueTypeId) const; // 判断是否在指定类型的战场队列中

    void SetBattlegroundId(uint32 id, BattlegroundTypeId bgTypeId, uint32 queueSlot, bool invited, bool isRandom, TeamId teamId); // 设置战场ID及相关信息
    uint32 AddBattlegroundQueueId(BattlegroundQueueTypeId val); // 添加战场队列ID
    bool HasFreeBattlegroundQueueId() const; // 判断是否有空闲的战场队列ID
    void RemoveBattlegroundQueueId(BattlegroundQueueTypeId val); // 移除战场队列ID
    void SetInviteForBattlegroundQueueType(BattlegroundQueueTypeId bgQueueTypeId, uint32 instanceId); // 设置指定队列类型的邀请信息
    bool IsInvitedForBattlegroundInstance(uint32 instanceId) const; // 判断是否被邀请进入指定实例的战场

    [[nodiscard]] TeamId GetBgTeamId() const { return m_bgData.bgTeamId != TEAM_NEUTRAL ? m_bgData.bgTeamId : GetTeamId(); } // 获取战场队伍ID

    void LeaveBattleground(Battleground* bg = nullptr); // 离开战场
    [[nodiscard]] bool CanJoinToBattleground() const; // 判断是否可以加入战场
    bool CanReportAfkDueToLimit(); // 判断是否可以因AFK限制报告
    void ReportedAfkBy(Player* reporter); // 记录被报告AFK
    void ClearAfkReports() { m_bgData.bgAfkReporter.clear(); } // 清除AFK报告记录

    [[nodiscard]] bool GetBGAccessByLevel(BattlegroundTypeId bgTypeId) const; // 判断是否根据等级允许进入战场
    bool CanUseBattlegroundObject(GameObject* gameobject) const; // 判断是否可以使用战场对象
    [[nodiscard]] bool isTotalImmune() const; // 判断是否完全免疫
    [[nodiscard]] bool CanCaptureTowerPoint() const; // 判断是否可以占领塔点

    bool GetRandomWinner() { return m_IsBGRandomWinner; } // 获取是否为随机获胜者
    void SetRandomWinner(bool isWinner); // 设置是否为随机获胜者

    /*********************************************************/
    /***               OUTDOOR PVP SYSTEM                  ***/
    /*********************************************************/

    [[nodiscard]] OutdoorPvP* GetOutdoorPvP() const; // 获取户外PvP对象
    // 如果玩家处于户外PvP目标占领的活跃状态，返回true，否则返回false
    bool IsOutdoorPvPActive();

    /*********************************************************/
    /***              ENVIROMENTAL SYSTEM                  ***/
    /*********************************************************/

    bool IsImmuneToEnvironmentalDamage(); // 判断是否免疫环境伤害
    uint32 EnvironmentalDamage(EnviromentalDamage type, uint32 damage); // 处理环境伤害

    /*********************************************************/
    /***               FLOOD FILTER SYSTEM                 ***/
    /*********************************************************/

    struct ChatFloodThrottle
    {
        enum Index
        {
            REGULAR = 0, // 普通聊天
            ADDON = 1, // 插件聊天
            MAX
        };

        time_t Time = 0; // 当前时间
        uint32 Count = 0; // 消息计数
    };

    void UpdateSpeakTime(ChatFloodThrottle::Index index); // 更新发言时间
    [[nodiscard]] bool CanSpeak() const; // 判断是否允许发言

    /*********************************************************/
    /***                 VARIOUS SYSTEMS                   ***/
    /*********************************************************/
    void UpdateFallInformationIfNeed(MovementInfo const& minfo, uint16 opcode); // 如需要，更新坠落信息
    SafeUnitPointer m_mover; // 移动单元指针
    WorldObject* m_seer; // 视角观察者
    std::set<Unit*> m_isInSharedVisionOf; // 共享视野的单位集合
    void SetFallInformation(uint32 time, float z)
    {
        m_lastFallTime = time;
        m_lastFallZ = z;
    } // 设置坠落信息
    void HandleFall(MovementInfo const& movementInfo); // 处理坠落事件

    [[nodiscard]] bool canFlyInZone(uint32 mapid, uint32 zone, SpellInfo const* bySpell); // 判断是否可以在指定区域飞行

    void SetClientControl(Unit* target, bool allowMove, bool packetOnly = false); // 设置客户端控制权限

    void SetMover(Unit* target); // 设置移动单元

    void SetSeer(WorldObject* target) { m_seer = target; } // 设置视角观察者
    void SetViewpoint(WorldObject* target, bool apply); // 设置视角
    [[nodiscard]] WorldObject* GetViewpoint() const; // 获取视角
    void StopCastingCharm(Aura* except = nullptr); // 停止施放魅惑效果
    void StopCastingBindSight(Aura* except = nullptr); // 停止施放绑定视野效果

    [[nodiscard]] uint32 GetSaveTimer() const { return m_nextSave; } // 获取保存计时器
    void SetSaveTimer(uint32 timer) { m_nextSave = timer; } // 设置保存计时器

    // 回忆位置
    uint32 m_recallMap;
    float  m_recallX;
    float  m_recallY;
    float  m_recallZ;
    float  m_recallO;
    void   SaveRecallPosition(); // 保存回忆位置

    void SetHomebind(WorldLocation const& loc, uint32 areaId); // 设置绑定家的位置

    // 家绑定坐标
    uint32 m_homebindMapId;
    uint16 m_homebindAreaId;
    float m_homebindX;
    float m_homebindY;
    float m_homebindZ;

    [[nodiscard]] WorldLocation GetStartPosition() const; // 获取起始位置

    [[nodiscard]] WorldLocation const& GetEntryPoint() const { return m_entryPointData.joinPos; } // 获取入口点位置
    void SetEntryPoint(); // 设置入口点

    // 当前在客户端可见的对象
    GuidUnorderedSet m_clientGUIDs;
    std::vector<Unit*> m_newVisible; // 新可见的单位列表（pussywizard）

    [[nodiscard]] bool HaveAtClient(WorldObject const* u) const; // 判断客户端是否可见该对象
    [[nodiscard]] bool HaveAtClient(ObjectGuid guid) const; // 判断客户端是否可见指定GUID的对象

    [[nodiscard]] bool IsNeverVisible() const override; // 判断是否永远不可见

    bool IsVisibleGloballyFor(Player const* player) const; // 判断是否对指定玩家全局可见

    void GetInitialVisiblePackets(Unit* target); // 获取初始可见数据包
    void UpdateObjectVisibility(bool forced = true, bool fromUpdate = false) override; // 更新对象可见性
    void UpdateVisibilityForPlayer(bool mapChange = false); // 更新玩家可见性
    void UpdateVisibilityOf(WorldObject* target); // 更新指定对象的可见性
    void UpdateTriggerVisibility(); // 更新触发器可见性

    template<class T>
    void UpdateVisibilityOf(T* target, UpdateData& data, std::vector<Unit*>& visibleNow); // 模板函数：更新指定对象的可见性

    uint8 m_forced_speed_changes[MAX_MOVE_TYPE]; // 强制速度变化计数器

    [[nodiscard]] bool HasAtLoginFlag(AtLoginFlags f) const { return m_atLoginFlags & f; } // 判断是否包含指定登录标志
    void SetAtLoginFlag(AtLoginFlags f) { m_atLoginFlags |= f; } // 设置登录标志
    void RemoveAtLoginFlag(AtLoginFlags flags, bool persist = false); // 移除登录标志

    bool IsUsingLfg(); // 判断是否使用查找队伍系统
    bool inRandomLfgDungeon(); // 判断是否在随机副本中

    typedef std::set<uint32> DFQuestsDoneList;
    DFQuestsDoneList m_DFQuests; // 完成的DF任务列表

    // 临时移除的宠物缓存
    [[nodiscard]] uint32 GetTemporaryUnsummonedPetNumber() const { return m_temporaryUnsummonedPetNumber; } // 获取临时未召唤的宠物编号
    void SetTemporaryUnsummonedPetNumber(uint32 petnumber) { m_temporaryUnsummonedPetNumber = petnumber; } // 设置临时未召唤的宠物编号
    void UnsummonPetTemporaryIfAny(); // 如果存在临时召唤的宠物则解除召唤
    void ResummonPetTemporaryUnSummonedIfAny(); // 如果存在临时未召唤的宠物则重新召唤
    [[nodiscard]] bool IsPetNeedBeTemporaryUnsummoned() const { return GetSession()->PlayerLogout() || !IsInWorld() || !IsAlive() || IsMounted()/*+in flight*/ || GetVehicle() || IsBeingTeleported(); } // 判断宠物是否需要临时解除召唤
    bool CanResummonPet(uint32 spellid); // 判断是否可以重新召唤宠物

    void SendCinematicStart(uint32 CinematicSequenceId) const; // 发送过场动画开始
    void SendMovieStart(uint32 MovieId); // 发送电影开始

    uint32 DoRandomRoll(uint32 minimum, uint32 maximum); // 执行随机掷骰

    [[nodiscard]] uint16 GetMaxSkillValueForLevel() const; // 获取当前等级的最大技能值
    bool IsFFAPvP(); // 判断是否为自由混战PvP
    bool IsPvP(); // 判断是否为PvP

    /*********************************************************/
    /***                 INSTANCE SYSTEM                   ***/
    /*********************************************************/

    void UpdateHomebindTime(uint32 time); // 更新绑定家的时间

    uint32 m_HomebindTimer; // 绑定家计时器
    bool m_InstanceValid; // 实例是否有效
    void BindToInstance(); // 绑定到实例
    void SetPendingBind(uint32 instanceId, uint32 bindTimer) { _pendingBindId = instanceId; _pendingBindTimer = bindTimer; } // 设置待绑定实例
    [[nodiscard]] bool HasPendingBind() const { return _pendingBindId > 0; } // 判断是否有待绑定实例
    [[nodiscard]] uint32 GetPendingBind() const { return _pendingBindId; } // 获取待绑定实例ID
    void SendRaidInfo(); // 发送团队副本信息
    void SendSavedInstances(); // 发送已保存的实例信息
    void PrettyPrintRequirementsQuestList(const std::vector<const ProgressionRequirement*>& missingQuests) const; // 打印缺少的任务需求列表
    void PrettyPrintRequirementsAchievementsList(const std::vector<const ProgressionRequirement*>& missingAchievements) const; // 打印缺少的成就需求列表
    void PrettyPrintRequirementsItemsList(const std::vector<const ProgressionRequirement*>& missingItems) const; // 打印缺少的物品需求列表
    bool Satisfy(DungeonProgressionRequirements const* ar, uint32 target_map, bool report = false); // 检查是否满足副本进度要求
    bool CheckInstanceLoginValid(); // 检查实例登录有效性
    [[nodiscard]] bool CheckInstanceCount(uint32 instanceId) const; // 检查实例数量

    void AddInstanceEnterTime(uint32 instanceId, time_t enterTime)
    {
        if (_instanceResetTimes.find(instanceId) == _instanceResetTimes.end())
            _instanceResetTimes.insert(InstanceTimeMap::value_type(instanceId, enterTime + HOUR));
    } // 添加进入实例的时间

    // 最后使用的宠物编号（用于战场）
    [[nodiscard]] uint32 GetLastPetNumber() const { return m_lastpetnumber; }
    void SetLastPetNumber(uint32 petnumber) { m_lastpetnumber = petnumber; }
    [[nodiscard]] uint32 GetLastPetSpell() const { return m_oldpetspell; }
    void SetLastPetSpell(uint32 petspell) { m_oldpetspell = petspell; }

    /*********************************************************/
    /***                   GROUP SYSTEM                    ***/
    /*********************************************************/

    Group* GetGroupInvite() { return m_groupInvite; } // 获取组队邀请
    void SetGroupInvite(Group* group) { m_groupInvite = group; } // 设置组队邀请
    Group* GetGroup() { return m_group.getTarget(); } // 获取当前组
    [[nodiscard]] const Group* GetGroup() const { return (const Group*)m_group.getTarget(); } // 获取当前组（常量版本）
    GroupReference& GetGroupRef() { return m_group; } // 获取组引用
    void SetGroup(Group* group, int8 subgroup = -1); // 设置组
    [[nodiscard]] uint8 GetSubGroup() const { return m_group.getSubGroup(); } // 获取子组
    [[nodiscard]] uint32 GetGroupUpdateFlag() const { return m_groupUpdateMask; } // 获取组更新标志
    void SetGroupUpdateFlag(uint32 flag) { m_groupUpdateMask |= flag; } // 设置组更新标志
    [[nodiscard]] uint64 GetAuraUpdateMaskForRaid() const { return m_auraRaidUpdateMask; } // 获取团队光环更新掩码
    void SetAuraUpdateMaskForRaid(uint8 slot) { m_auraRaidUpdateMask |= (uint64(1) << slot); } // 设置团队光环更新掩码
    Player* GetNextRandomRaidMember(float radius); // 获取下一个随机团队成员
    [[nodiscard]] PartyResult CanUninviteFromGroup(ObjectGuid targetPlayerGUID = ObjectGuid::Empty) const; // 判断是否可以从组中移除指定玩家

    // 战场组系统
    void SetBattlegroundOrBattlefieldRaid(Group* group, int8 subgroup = -1); // 设置战场或战场团队
    void RemoveFromBattlegroundOrBattlefieldRaid(); // 从战场或战场团队中移除
    Group* GetOriginalGroup() { return m_originalGroup.getTarget(); } // 获取原始组
    GroupReference& GetOriginalGroupRef() { return m_originalGroup; } // 获取原始组引用
    [[nodiscard]] uint8 GetOriginalSubGroup() const { return m_originalGroup.getSubGroup(); } // 获取原始子组
    void SetOriginalGroup(Group* group, int8 subgroup = -1); // 设置原始组

    void SetPassOnGroupLoot(bool bPassOnGroupLoot) { m_bPassOnGroupLoot = bPassOnGroupLoot; } // 设置是否传递团队战利品
    [[nodiscard]] bool GetPassOnGroupLoot() const { return m_bPassOnGroupLoot; } // 获取是否传递团队战利品

    MapReference& GetMapRef() { return m_mapRef; } // 获取地图引用

    // 设置玩家的地图并添加引用
    void SetMap(Map* map) override;
    void ResetMap() override;

    bool CanTeleport() { return m_canTeleport; } // 判断是否允许传送
    void SetCanTeleport(bool value) { m_canTeleport = value; } // 设置是否允许传送
    bool CanKnockback() { return m_canKnockback; } // 判断是否允许击退
    void SetCanKnockback(bool value) { m_canKnockback = value; } // 设置是否允许击退

    bool isAllowedToLoot(Creature const* creature); // 判断是否允许掠夺生物

    [[nodiscard]] DeclinedName const* GetDeclinedNames() const { return m_declinedname; } // 获取拒绝的名字信息
    [[nodiscard]] uint8 GetRunesState() const { return m_runes->runeState; } // 获取符文状态
    [[nodiscard]] RuneType GetBaseRune(uint8 index) const { return RuneType(m_runes->runes[index].BaseRune); } // 获取基础符文
    [[nodiscard]] RuneType GetCurrentRune(uint8 index) const { return RuneType(m_runes->runes[index].CurrentRune); } // 获取当前符文
    [[nodiscard]] uint32 GetRuneCooldown(uint8 index) const { return m_runes->runes[index].Cooldown; } // 获取符文冷却时间
    [[nodiscard]] uint32 GetGracePeriod(uint8 index) const { return m_runes->runes[index].GracePeriod; } // 获取符文宽限期
    uint32 GetRuneBaseCooldown(uint8 index, bool skipGrace); // 获取符文的基础冷却时间
    [[nodiscard]] bool IsBaseRuneSlotsOnCooldown(RuneType runeType) const; // 判断基础符文槽是否处于冷却中
    RuneType GetLastUsedRune() { return m_runes->lastUsedRune; } // 获取最后使用的符文
    void SetLastUsedRune(RuneType type) { m_runes->lastUsedRune = type; } // 设置最后使用的符文
    void SetBaseRune(uint8 index, RuneType baseRune) { m_runes->runes[index].BaseRune = baseRune; } // 设置基础符文
    void SetCurrentRune(uint8 index, RuneType currentRune) { m_runes->runes[index].CurrentRune = currentRune; } // 设置当前符文
    void SetRuneCooldown(uint8 index, uint32 cooldown) { m_runes->runes[index].Cooldown = cooldown; m_runes->SetRuneState(index, (cooldown == 0)); } // 设置符文冷却时间
    void SetGracePeriod(uint8 index, uint32 period) { m_runes->runes[index].GracePeriod = period; } // 设置宽限期
    void SetRuneConvertAura(uint8 index, AuraEffect const* aura) { m_runes->runes[index].ConvertAura = aura; } // 设置符文转换效果
    void AddRuneByAuraEffect(uint8 index, RuneType newType, AuraEffect const* aura) { SetRuneConvertAura(index, aura); ConvertRune(index, newType); } // 添加符文效果
    void RemoveRunesByAuraEffect(AuraEffect const* aura); // 移除符文效果
    void RestoreBaseRune(uint8 index); // 恢复基础符文
    void ConvertRune(uint8 index, RuneType newType); // 转换符文
    void ResyncRunes(uint8 count); // 重新同步符文
    void AddRunePower(uint8 index); // 添加符文能量
    void InitRunes(); // 初始化符文

    void SendRespondInspectAchievements(Player* player) const; // 发送成就检查响应
    [[nodiscard]] bool HasAchieved(uint32 achievementId) const; // 判断是否拥有指定成就
    void ResetAchievements(); // 重置成就
    void CheckAllAchievementCriteria(); // 检查所有成就条件
    void ResetAchievementCriteria(AchievementCriteriaCondition condition, uint32 value, bool evenIfCriteriaComplete = false); // 重置成就条件
    void UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscValue1 = 0, uint32 miscValue2 = 0, Unit* unit = nullptr); // 更新成就条件
    void StartTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry, uint32 timeLost = 0); // 开始计时成就
    void RemoveTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry); // 移除计时成就
    void CompletedAchievement(AchievementEntry const* entry); // 完成成就
    [[nodiscard]] AchievementMgr* GetAchievementMgr() const { return m_achievementMgr; } // 获取成就管理器

    void SetCreationTime(Seconds creationTime) { m_creationTime = creationTime; } // 设置创建时间
    [[nodiscard]] Seconds GetCreationTime() const { return m_creationTime; } // 获取创建时间

    [[nodiscard]] bool HasTitle(uint32 bitIndex) const; // 判断是否拥有指定标题
    bool HasTitle(CharTitlesEntry const* title) const { return HasTitle(title->bit_index); } // 判断是否拥有指定标题条目
    void SetTitle(CharTitlesEntry const* title, bool lost = false); // 设置标题
    void SetCurrentTitle(CharTitlesEntry const* title, bool clear = false) { SetUInt32Value(PLAYER_CHOSEN_TITLE, clear ? 0 : title->bit_index); }; // 设置当前标题

    //bool isActiveObject() const { return true; }
    bool CanSeeSpellClickOn(Creature const* creature) const; // 判断是否可以看见对生物的点击施法
    [[nodiscard]] bool CanSeeVendor(Creature const* creature) const; // 判断是否可以看见商人

    [[nodiscard]] uint32 GetChampioningFaction() const { return m_ChampioningFaction; } // 获取支持的阵营
    void SetChampioningFaction(uint32 faction) { m_ChampioningFaction = faction; } // 设置支持的阵营
    Spell* m_spellModTakingSpell; // 正在施放的修改法术

    float GetAverageItemLevel(); // 获取平均物品等级
    float GetAverageItemLevelForDF(); // 获取Dragonflight版本的平均物品等级
    bool isDebugAreaTriggers; // 是否启用区域触发器调试

    void ClearWhisperWhiteList() { WhisperList.clear(); } // 清除密语白名单
    void AddWhisperWhiteList(ObjectGuid guid) { WhisperList.push_back(guid); } // 添加密语白名单
    bool IsInWhisperWhiteList(ObjectGuid guid); // 判断是否在密语白名单中
    void RemoveFromWhisperWhiteList(ObjectGuid guid) { WhisperList.remove(guid); } // 从密语白名单中移除

    bool SetDisableGravity(bool disable, bool packetOnly = false, bool updateAnimationTier = true) override; // 设置禁用重力
    bool SetCanFly(bool apply, bool packetOnly = false) override; // 设置飞行能力
    bool SetWaterWalking(bool apply, bool packetOnly = false) override; // 设置水上行走能力
    bool SetFeatherFall(bool apply, bool packetOnly = false) override; // 设置羽毛坠落能力
    bool SetHover(bool enable, bool packetOnly = false, bool updateAnimationTier = true) override; // 设置悬停能力

    [[nodiscard]] bool CanFly() const override { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_CAN_FLY); } // 判断是否可以飞行
    [[nodiscard]] bool CanEnterWater() const override { return true; } // 判断是否可以进入水中

    // 保存相关
    void AdditionalSavingAddMask(uint8 mask) { m_additionalSaveTimer = 2000; m_additionalSaveMask |= mask; } // 添加额外保存掩码
    // 竞技场观众系统
    [[nodiscard]] bool IsSpectator() const { return m_ExtraFlags & PLAYER_EXTRA_SPECTATOR_ON; } // 判断是否为观众
    void SetIsSpectator(bool on); // 设置是否为观众
    [[nodiscard]] bool NeedSendSpectatorData() const; // 判断是否需要发送观众数据
    void SetPendingSpectatorForBG(uint32 bgInstanceId) { m_pendingSpectatorForBG = bgInstanceId; } // 设置待加入的战场实例
    [[nodiscard]] bool HasPendingSpectatorForBG(uint32 bgInstanceId) const { return m_pendingSpectatorForBG == bgInstanceId; } // 判断是否有待加入的战场实例
    void SetPendingSpectatorInviteInstanceId(uint32 bgInstanceId) { m_pendingSpectatorInviteInstanceId = bgInstanceId; } // 设置待接受的观众邀请实例
    [[nodiscard]] uint32 GetPendingSpectatorInviteInstanceId() const { return m_pendingSpectatorInviteInstanceId; } // 获取待接受的观众邀请实例
    bool HasReceivedSpectatorResetFor(ObjectGuid guid) { return m_receivedSpectatorResetFor.find(guid) != m_receivedSpectatorResetFor.end(); } // 判断是否已收到观众重置请求
    void ClearReceivedSpectatorResetFor() { m_receivedSpectatorResetFor.clear(); } // 清除收到的观众重置请求
    void AddReceivedSpectatorResetFor(ObjectGuid guid) { m_receivedSpectatorResetFor.insert(guid); } // 添加收到的观众重置请求
    void RemoveReceivedSpectatorResetFor(ObjectGuid guid) { m_receivedSpectatorResetFor.erase(guid); } // 移除收到的观众重置请求
    uint32 m_pendingSpectatorForBG; // 待加入的战场实例ID
    uint32 m_pendingSpectatorInviteInstanceId; // 待接受的观众邀请实例ID
    GuidSet m_receivedSpectatorResetFor; // 收到的观众重置请求集合

    // 死亡骑士舞动符文武器
    void setRuneWeaponGUID(ObjectGuid guid) { m_drwGUID = guid; }; // 设置舞动符文武器GUID
    ObjectGuid getRuneWeaponGUID() { return m_drwGUID; }; // 获取舞动符文武器GUID
    ObjectGuid m_drwGUID; // 舞动符文武器GUID

    [[nodiscard]] bool CanSeeDKPet() const { return m_ExtraFlags & PLAYER_EXTRA_SHOW_DK_PET; } // 判断是否可以看到死亡骑士宠物
    void SetShowDKPet(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_SHOW_DK_PET; else m_ExtraFlags &= ~PLAYER_EXTRA_SHOW_DK_PET; }; // 设置是否可以看到死亡骑士宠物
    void PrepareCharmAISpells(); // 准备控制宠物的AI技能
    uint32 m_charmUpdateTimer; // 控制宠物AI更新计时器

    bool NeedToSaveGlyphs() { return m_NeedToSaveGlyphs; } // 判断是否需要保存雕文
    void SetNeedToSaveGlyphs(bool val) { m_NeedToSaveGlyphs = val; } // 设置是否需要保存雕文

    uint32 GetMountBlockId() { return m_MountBlockId; } // 获取坐骑阻断ID
    void SetMountBlockId(uint32 mount) { m_MountBlockId = mount; } // 设置坐骑阻断ID

    [[nodiscard]] float GetRealParry() const { return m_realParry; } // 获取真实躲闪值
    [[nodiscard]] float GetRealDodge() const { return m_realDodge; } // 获取真实招架值
    // mt maps
    [[nodiscard]] const PlayerTalentMap& GetTalentMap() const { return m_talents; } // 获取天赋映射
    [[nodiscard]] uint32 GetNextSave() const { return m_nextSave; } // 获取下一次保存时间
    [[nodiscard]] SpellModList const& GetSpellModList(uint32 type) const { return m_spellMods[type]; } // 获取法术修改列表

    void SetServerSideVisibility(ServerSideVisibilityType type, AccountTypes sec); // 设置服务器端可见性
    void SetServerSideVisibilityDetect(ServerSideVisibilityType type, AccountTypes sec); // 设置服务器端可见性检测

    static std::unordered_map<int, bgZoneRef> bgZoneIdToFillWorldStates; // 区域ID到FillInitialWorldStates的映射

    void SetFarSightDistance(float radius); // 设置远视距离
    void ResetFarSightDistance(); // 重置远视距离
    [[nodiscard]] Optional<float> GetFarSightDistance() const; // 获取远视距离

    float GetSightRange(WorldObject const* target = nullptr) const override; // 获取视野范围

    std::string GetPlayerName(); // 获取玩家名称

    // 设置
    [[nodiscard]] PlayerSetting GetPlayerSetting(std::string source, uint8 index); // 获取玩家设置
    void UpdatePlayerSetting(std::string source, uint8 index, uint32 value); // 更新玩家设置

    void SendSystemMessage(std::string_view msg, bool escapeCharacters = false); // 发送系统消息

    std::string GetDebugInfo() const override; // 获取调试信息

    /*********************************************************/
    /***               SPELL QUEUE SYSTEM                  ***/
    /*********************************************************/
protected:
    uint32 GetSpellQueueWindow() const; // 获取法术队列窗口时间
    void ProcessSpellQueue(); // 处理法术队列

public:
    std::deque<PendingSpellCastRequest> SpellQueue; // 法术队列
    const PendingSpellCastRequest* GetCastRequest(uint32 category) const; // 获取指定类别的施法请求
    bool CanExecutePendingSpellCastRequest(SpellInfo const* spellInfo); // 判断是否可以执行指定法术的施法请求
    void ExecuteOrCancelSpellCastRequest(PendingSpellCastRequest* castRequest, bool isCancel = false); // 执行或取消施法请求
    bool CanRequestSpellCast(SpellInfo const* spellInfo); // 判断是否可以请求施法

protected:
    // 游戏管理员密语白名单
    WhisperListContainer WhisperList;

    // 性能变量
    bool m_NeedToSaveGlyphs; // 是否需要保存雕文
    // 坐骑阻断问题
    uint32 m_MountBlockId; // 坐骑阻断ID
    // 真实属性
    float m_realDodge; // 真实躲闪值
    float m_realParry; // 真实招架值

    uint32 m_charmAISpells[NUM_CAI_SPELLS]; // 控制宠物的AI技能

    uint32 m_AreaID; // 当前区域ID
    uint32 m_regenTimerCount; // 回复计时器计数
    uint32 m_foodEmoteTimerCount; // 食物表情计时器计数
    float m_powerFraction[MAX_POWERS]; // 能量分数
    uint32 m_contestedPvPTimer; // 争夺PvP计时器

    /*********************************************************/
    /***               BATTLEGROUND SYSTEM                 ***/
    /*********************************************************/

    struct BgBattlegroundQueueID_Rec
    {
        BattlegroundQueueTypeId bgQueueTypeId; // 战场队列类型ID
        uint32 invitedToInstance; // 被邀请的实例ID
    };

    std::array<BgBattlegroundQueueID_Rec, PLAYER_MAX_BATTLEGROUND_QUEUES> _BgBattlegroundQueueID; // 战场队列ID数组
    BGData m_bgData; // 战场数据
    bool m_IsBGRandomWinner; // 是否为随机获胜者

    /*********************************************************/
    /***                   ENTRY POINT                     ***/
    /*********************************************************/

    EntryPointData m_entryPointData; // 入口点数据

    /*********************************************************/
    /***                    QUEST SYSTEM                   ***/
    /*********************************************************/

    // 我们只允许一个定时任务同时进行。以下可以是简单值而不是集合。
    typedef std::set<uint32> QuestSet;
    typedef std::set<uint32> SeasonalQuestSet;
    typedef std::unordered_map<uint32, SeasonalQuestSet> SeasonalEventQuestMap;
    QuestSet m_timedquests; // 定时任务
    QuestSet m_weeklyquests; // 每周任务
    QuestSet m_monthlyquests; // 每月任务
    SeasonalEventQuestMap m_seasonalquests; // 季节性任务

    ObjectGuid m_divider; // 分隔符GUID
    uint32 m_ingametime; // 游戏内时间

    /*********************************************************/
    /***                   LOAD SYSTEM                     ***/
    /*********************************************************/

    void _LoadActions(PreparedQueryResult result); // 加载动作数据
    void _LoadAuras(PreparedQueryResult result, uint32 timediff); // 加载光环数据
    void _LoadGlyphAuras(); // 加载雕文光环
    void _LoadInventory(PreparedQueryResult result, uint32 timeDiff); // 加载背包数据
    void _LoadMail(PreparedQueryResult mailsResult, PreparedQueryResult mailItemsResult); // 加载邮件数据
    static Item* _LoadMailedItem(ObjectGuid const& playerGuid, Player* player, uint32 mailId, Mail* mail, Field* fields); // 加载邮件物品
    void _LoadQuestStatus(PreparedQueryResult result); // 加载任务状态
    void _LoadQuestStatusRewarded(PreparedQueryResult result); // 加载已奖励的任务状态
    void _LoadDailyQuestStatus(PreparedQueryResult result); // 加载每日任务状态
    void _LoadWeeklyQuestStatus(PreparedQueryResult result); // 加载每周任务状态
    void _LoadMonthlyQuestStatus(PreparedQueryResult result); // 加载每月任务状态
    void _LoadSeasonalQuestStatus(PreparedQueryResult result); // 加载季节性任务状态
    void _LoadRandomBGStatus(PreparedQueryResult result); // 加载随机战场状态
    void _LoadGroup(); // 加载组数据
    void _LoadSkills(PreparedQueryResult result); // 加载技能数据
    void _LoadSpells(PreparedQueryResult result); // 加载法术数据
    void _LoadFriendList(PreparedQueryResult result); // 加载好友列表
    bool _LoadHomeBind(PreparedQueryResult result); // 加载绑定家数据
    void _LoadDeclinedNames(PreparedQueryResult result); // 加载拒绝的名字数据
    void _LoadArenaTeamInfo(); // 加载竞技队信息
    void _LoadEquipmentSets(PreparedQueryResult result); // 加载装备套装数据
    void _LoadEntryPointData(PreparedQueryResult result); // 加载入口点数据
    void _LoadGlyphs(PreparedQueryResult result); // 加载雕文数据
    void _LoadTalents(PreparedQueryResult result); // 加载天赋数据
    void _LoadInstanceTimeRestrictions(PreparedQueryResult result); // 加载实例时间限制
    void _LoadBrewOfTheMonth(PreparedQueryResult result); // 加载本月啤酒数据
    void _LoadCharacterSettings(PreparedQueryResult result); // 加载角色设置
    void _LoadPetStable(uint8 petStableSlots, PreparedQueryResult result); // 加载宠物马厩数据

    /*********************************************************/
    /***                   SAVE SYSTEM                     ***/
    /*********************************************************/

        void _SaveActions(CharacterDatabaseTransaction trans); // 保存动作数据
    void _SaveAuras(CharacterDatabaseTransaction trans, bool logout); // 保存光环数据
    void _SaveInventory(CharacterDatabaseTransaction trans); // 保存背包数据
    void _SaveMail(CharacterDatabaseTransaction trans); // 保存邮件数据
    void _SaveQuestStatus(CharacterDatabaseTransaction trans); // 保存任务状态
    void _SaveDailyQuestStatus(CharacterDatabaseTransaction trans); // 保存每日任务状态
    void _SaveWeeklyQuestStatus(CharacterDatabaseTransaction trans); // 保存每周任务状态
    void _SaveMonthlyQuestStatus(CharacterDatabaseTransaction trans); // 保存每月任务状态
    void _SaveSeasonalQuestStatus(CharacterDatabaseTransaction trans); // 保存季节性任务状态
    void _SaveSpells(CharacterDatabaseTransaction trans); // 保存法术数据
    void _SaveEquipmentSets(CharacterDatabaseTransaction trans); // 保存装备套装数据
    void _SaveEntryPoint(CharacterDatabaseTransaction trans); // 保存入口点数据
    void _SaveGlyphs(CharacterDatabaseTransaction trans); // 保存雕文数据
    void _SaveTalents(CharacterDatabaseTransaction trans); // 保存天赋数据
    void _SaveStats(CharacterDatabaseTransaction trans); // 保存属性数据
    void _SaveCharacter(bool create, CharacterDatabaseTransaction trans); // 保存角色数据
    void _SaveInstanceTimeRestrictions(CharacterDatabaseTransaction trans); // 保存实例时间限制
    void _SavePlayerSettings(CharacterDatabaseTransaction trans); // 保存玩家设置

    /*********************************************************/
    /***              ENVIRONMENTAL SYSTEM                 ***/
    /*********************************************************/
    void HandleSobering(); // 处理清醒状态
    void SendMirrorTimer(MirrorTimerType Type, uint32 MaxValue, uint32 CurrentValue, int32 Regen); // 发送镜像计时器
    void StopMirrorTimer(MirrorTimerType Type); // 停止镜像计时器
    void HandleDrowning(uint32 time_diff); // 处理溺水
    int32 getMaxTimer(MirrorTimerType timer); // 获取最大计时器时间

    /*********************************************************/
    /***                  HONOR SYSTEM                     ***/
    /*********************************************************/
    time_t m_lastHonorUpdateTime; // 上次荣誉更新时间

    void outDebugValues() const; // 输出调试值
    ObjectGuid m_lootGuid; // 战利品GUID

    TeamId m_team; // 队伍ID
    uint32 m_nextSave; // 下次保存时间（pussywizard）
    uint16 m_additionalSaveTimer; // 额外保存计时器（pussywizard）
    uint8 m_additionalSaveMask; // 额外保存掩码（pussywizard）
    uint16 m_hostileReferenceCheckTimer; // 敌对引用检查计时器（pussywizard）
    std::array<ChatFloodThrottle, ChatFloodThrottle::MAX> m_chatFloodData; // 聊天洪水控制数据
    Difficulty m_dungeonDifficulty; // 地下城难度
    Difficulty m_raidDifficulty; // 团队副本难度
    Difficulty m_raidMapDifficulty; // 团队副本地图难度

    uint32 m_atLoginFlags; // 登录标志

    Item* m_items[PLAYER_SLOTS_COUNT]; // 玩家物品槽
    uint32 m_currentBuybackSlot; // 当前回购槽位

    std::vector<Item*> m_itemUpdateQueue; // 物品更新队列
    bool m_itemUpdateQueueBlocked; // 物品更新队列是否被阻塞

    uint32 m_ExtraFlags; // 额外标志

    QuestStatusMap m_QuestStatus; // 任务状态映射
    QuestStatusSaveMap m_QuestStatusSave; // 任务状态保存映射

    RewardedQuestSet m_RewardedQuests; // 已奖励任务集合
    QuestStatusSaveMap m_RewardedQuestsSave; // 已奖励任务保存映射
    void SendQuestGiverStatusMultiple(); // 发送任务给予者状态（多个）

    SkillStatusMap mSkillStatus; // 技能状态映射

    uint32 m_GuildIdInvited; // 被邀请的公会ID
    uint32 m_ArenaTeamIdInvited; // 被邀请的竞技队ID

    PlayerMails m_mail; // 玩家邮件
    PlayerSpellMap m_spells; // 玩家法术映射
    PlayerTalentMap m_talents; // 玩家天赋映射
    uint32 m_lastPotionId; // 最后使用的战斗中健康/魔法药水ID，用于阻止下一次使用

    GlobalCooldownMgr m_GlobalCooldownMgr; // 全局冷却管理器

    uint8 m_activeSpec; // 当前激活的专精
    uint8 m_specsCount; // 专精数量

    uint32 m_Glyphs[MAX_TALENT_SPECS][MAX_GLYPH_SLOT_INDEX]; // 雕文数据

    ActionButtonList m_actionButtons; // 动作按钮列表

    float m_auraBaseMod[BASEMOD_END][MOD_END]; // 光环基础修改值
    int32 m_baseRatingValue[MAX_COMBAT_RATING]; // 基础评分值
    uint32 m_baseSpellPower; // 基础法术强度
    uint32 m_baseFeralAP; // 基础野性攻击强度
    uint32 m_baseManaRegen; // 基础法力回复
    uint32 m_baseHealthRegen; // 基础生命回复
    int32 m_spellPenetrationItemMod; // 法术穿透物品修改值

    SpellModList m_spellMods[MAX_SPELLMOD]; // 法术修改列表
    //uint32 m_pad;
    //        Spell* m_spellModTakingSpell;  // 用于在Spell::finish中消耗充能的法术

    EnchantDurationList m_enchantDuration; // 附魔持续时间列表
    ItemDurationList m_itemDuration; // 物品持续时间列表
    ItemDurationList m_itemSoulboundTradeable; // 可交易灵魂绑定物品列表
    std::mutex m_soulboundTradableLock; // 灵魂绑定交易锁

    ObjectGuid m_resurrectGUID; // 复活GUID
    uint32 m_resurrectMap; // 复活地图
    float m_resurrectX, m_resurrectY, m_resurrectZ; // 复活坐标
    uint32 m_resurrectHealth, m_resurrectMana; // 复活生命和法力值

    WorldSession* m_session; // 世界会话

    typedef std::list<Channel*> JoinedChannelsList;
    JoinedChannelsList m_channels; // 加入的频道列表

    uint8 m_cinematic; // 过场动画编号

    TradeData* m_trade; // 交易数据

    bool   m_DailyQuestChanged; // 每日任务是否改变
    bool   m_WeeklyQuestChanged; // 每周任务是否改变
    bool   m_MonthlyQuestChanged; // 每月任务是否改变
    bool   m_SeasonalQuestChanged; // 季节性任务是否改变
    time_t m_lastDailyQuestTime; // 上次每日任务时间

    uint32 m_drunkTimer; // 醉酒计时器
    uint32 m_weaponChangeTimer; // 武器切换计时器

    uint32 m_zoneUpdateId; // 区域更新ID
    uint32 m_zoneUpdateTimer; // 区域更新计时器
    uint32 m_areaUpdateId; // 区域ID更新

    uint32 m_deathTimer; // 死亡计时器
    time_t m_deathExpireTime; // 死亡过期时间

    uint32 m_WeaponProficiency; // 武器熟练度
    uint32 m_ArmorProficiency; // 护甲熟练度
    bool m_canParry; // 是否可以招架
    bool m_canBlock; // 是否可以格挡
    bool m_canTitanGrip; // 是否可以泰坦之握
    uint8 m_swingErrorMsg; // 攻击错误消息
    float m_ammoDPS; // 弹药DPS

    float m_Expertise; // 专精
    float m_OffhandExpertise; // 副手专精

    ////////////////////Rest System/////////////////////
    time_t _restTime; // 休息时间
    uint32 _innTriggerId; // 旅馆触发器ID
    float _restBonus; // 休息奖励
    uint32 _restFlagMask; // 休息标志掩码
    ////////////////////Rest System/////////////////////
    uint32 m_resetTalentsCost; // 重置天赋成本
    time_t m_resetTalentsTime; // 重置天赋时间
    uint32 m_usedTalentCount; // 使用的天赋点数
    uint32 m_questRewardTalentCount; // 任务奖励的天赋点数
    uint32 m_extraBonusTalentCount; // 额外奖励的天赋点数

    // 社交
    PlayerSocial* m_social; // 玩家社交数据

    // 组队
    GroupReference m_group; // 组队引用
    GroupReference m_originalGroup; // 原始组队引用
    Group* m_groupInvite; // 组队邀请
    uint32 m_groupUpdateMask; // 组更新掩码
    uint64 m_auraRaidUpdateMask; // 团队光环更新掩码
    bool m_bPassOnGroupLoot; // 是否传递团队战利品

    // 最后使用的宠物编号（用于战场）
    uint32 m_lastpetnumber; // 最后宠物编号

    // 玩家召唤
    time_t m_summon_expire; // 召唤过期时间
    uint32 m_summon_mapid; // 召唤地图ID
    float  m_summon_x; // 召唤X坐标
    float  m_summon_y; // 召唤Y坐标
    float  m_summon_z; // 召唤Z坐标
    bool   m_summon_asSpectator; // 是否作为观众召唤

    DeclinedName* m_declinedname; // 拒绝的名字
    Runes* m_runes; // 符文数据
    EquipmentSets m_EquipmentSets; // 装备套装

    bool CanAlwaysSee(WorldObject const* obj) const override; // 判断是否总是可见该对象

    bool IsAlwaysDetectableFor(WorldObject const* seer) const override; // 判断是否总是可被指定观察者检测到

    uint8 m_grantableLevels; // 可授予的等级

    bool m_needZoneUpdate; // 是否需要区域更新

private:
    // 内部通用的CanStore/StoreItem函数部分
    InventoryResult CanStoreItem_InSpecificSlot(uint8 bag, uint8 slot, ItemPosCountVec& dest, ItemTemplate const* pProto, uint32& count, bool swap, Item* pSrcItem) const; // 在特定槽位存储物品的可行性检查
    InventoryResult CanStoreItem_InBag(uint8 bag, ItemPosCountVec& dest, ItemTemplate const* pProto, uint32& count, bool merge, bool non_specialized, Item* pSrcItem, uint8 skip_bag, uint8 skip_slot) const;
    // 检查物品是否可以存入指定的背包中，并计算存储位置和数量
    InventoryResult CanStoreItem_InInventorySlots(uint8 slot_begin, uint8 slot_end, ItemPosCountVec& dest, ItemTemplate const* pProto, uint32& count, bool merge, Item* pSrcItem, uint8 skip_bag, uint8 skip_slot) const;
    // 检查物品是否可以存入指定的库存插槽范围内，并计算存储位置和数量
    Item* _StoreItem(uint16 pos, Item* pItem, uint32 count, bool clone, bool update);
    // 将物品存储到指定位置，可能克隆物品或更新现有物品数量
    Item* _LoadItem(CharacterDatabaseTransaction trans, uint32 zoneId, uint32 timeDiff, Field* fields);
    // 从数据库加载物品数据并创建物品对象

    CinematicMgr* _cinematicMgr;  // 管理玩家当前播放的过场动画

    typedef GuidSet RefundableItemsSet;
    RefundableItemsSet m_refundableItems;  // 可退款的物品集合
    void SendRefundInfo(Item* item);       // 发送物品的退款信息给玩家
    void RefundItem(Item* item);           // 对指定物品进行退款操作

    // 已知货币不会在任何时刻被移除（0表示在界面中显示）
    void AddKnownCurrency(uint32 itemId);  // 添加已知货币类型到玩家数据中

    void AdjustQuestReqItemCount(Quest const* quest, QuestStatusData& questStatusData);
    // 调整任务所需物品数量的状态数据

    [[nodiscard]] bool MustDelayTeleport() const { return m_bMustDelayTeleport; } // 判断是否需要延迟传送（例如在更新期间）
    void SetMustDelayTeleport(bool setting) { m_bMustDelayTeleport = setting; }   // 设置是否需要延迟传送标志
    [[nodiscard]] bool HasDelayedTeleport() const { return m_bHasDelayedTeleport; } // 判断是否有延迟的传送
    void SetHasDelayedTeleport(bool setting) { m_bHasDelayedTeleport = setting; }   // 设置是否有延迟传送标志

    MapReference m_mapRef;  // 玩家在地图上的引用

    void UpdateCharmedAI();  // 更新被魅惑生物的AI逻辑

    uint32 m_lastFallTime;   // 上次坠落时间，用于坠落伤害计算
    float  m_lastFallZ;      // 上次坠落位置的Z坐标

    int32 m_MirrorTimer[MAX_TIMERS];     // 镜像计时器（如水下呼吸、死亡冷却等）
    uint8 m_MirrorTimerFlags;           // 当前镜像计时器状态标志
    uint8 m_MirrorTimerFlagsLast;       // 上一次的镜像计时器状态标志
    bool m_isInWater;                   // 当前是否处于水中

    // 当前传送数据
    WorldLocation teleportStore_dest;   // 传送目标位置
    uint32 teleportStore_options;       // 传送选项标志
    time_t mSemaphoreTeleport_Near;     // 近程传送信号量时间戳
    time_t mSemaphoreTeleport_Far;      // 远程传送信号量时间戳

    uint32 m_DelayedOperations;         // 延迟操作的标志位
    bool m_bMustDelayTeleport;          // 是否必须延迟传送
    bool m_bHasDelayedTeleport;         // 是否已有延迟的传送
    bool m_canTeleport;                 // 是否允许传送
    bool m_canKnockback;                // 是否允许击退

    std::unique_ptr<PetStable> m_petStable;  // 宠物栏管理

    // 临时移除的宠物缓存
    uint32 m_temporaryUnsummonedPetNumber;  // 临时解除召唤的宠物编号
    uint32 m_oldpetspell;                   // 旧的宠物技能

    AchievementMgr* m_achievementMgr;  // 成就管理器
    ReputationMgr* m_reputationMgr;   // 声望管理器

    SpellCooldowns m_spellCooldowns;  // 法术冷却时间管理

    uint32 m_ChampioningFaction;  // 当前冠军阵营ID

    InstanceTimeMap _instanceResetTimes;  // 实例副本重置时间记录
    uint32 _pendingBindId;                // 挂起的绑定实例ID
    uint32 _pendingBindTimer;             // 挂起的绑定定时器

    uint32 _activeCheats;  // 当前激活的作弊模式标志

    // 决斗前的生命值和法力值备份
    uint32 healthBeforeDuel;
    uint32 manaBeforeDuel;

    bool m_isInstantFlightOn;  // 是否启用了即时飞行

    uint32 m_flightSpellActivated;  // 当前激活的飞行法术ID

    WorldLocation _corpseLocation;  // 玩家尸体的位置信息

    Optional<float> _farSightDistance = { };  // 远视距离（如猎人监视技能）

    bool _wasOutdoor;  // 是否之前处于户外环境

    PlayerSettingMap m_charSettingsMap;  // 玩家设置映射表

    Seconds m_creationTime;  // 玩家角色创建时间
};

void AddItemsSetItem(Player* player, Item* item);       // 将物品添加到玩家的物品集合中
void RemoveItemsSetItem(Player* player, ItemTemplate const* proto);  // 从玩家的物品集合中移除指定类型的物品模板
#endif
