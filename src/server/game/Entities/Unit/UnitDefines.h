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

#include "Define.h"
#include "EnumFlag.h"

// UNIT_FIELD_BYTES_1 各字节偏移量枚举
enum UnitBytes1Offsets : uint8
{
    UNIT_BYTES_1_OFFSET_STAND_STATE     = 0,  // 站立状态偏移量
    UNIT_BYTES_1_OFFSET_PET_TALENTS     = 1,  // 宠物天赋偏移量
    UNIT_BYTES_1_OFFSET_VIS_FLAG        = 2,  // 可见标志偏移量
    UNIT_BYTES_1_OFFSET_ANIM_TIER       = 3   // 动画层级偏移量
};

// UNIT_FIELD_BYTES_1 的第 0 字节值，单位站立状态类型枚举
enum UnitStandStateType
{
    UNIT_STAND_STATE_STAND              = 0,  // 站立状态
    UNIT_STAND_STATE_SIT                = 1,  // 坐下状态
    UNIT_STAND_STATE_SIT_CHAIR          = 2,  // 坐在椅子上状态
    UNIT_STAND_STATE_SLEEP              = 3,  // 睡觉状态
    UNIT_STAND_STATE_SIT_LOW_CHAIR      = 4,  // 坐在矮椅子上状态
    UNIT_STAND_STATE_SIT_MEDIUM_CHAIR   = 5,  // 坐在中等椅子上状态
    UNIT_STAND_STATE_SIT_HIGH_CHAIR     = 6,  // 坐在高椅子上状态
    UNIT_STAND_STATE_DEAD               = 7,  // 死亡状态
    UNIT_STAND_STATE_KNEEL              = 8,  // 下跪状态
    UNIT_STAND_STATE_SUBMERGED          = 9   // 浸没状态
};

// UNIT_FIELD_BYTES_1 的第 2 字节标志值，单位站立标志枚举
enum UnitStandFlags
{
    UNIT_STAND_FLAGS_UNK1               = 0x01,  // 未知标志 1
    UNIT_STAND_FLAGS_CREEP              = 0x02,  // 潜行标志
    UNIT_STAND_FLAGS_UNTRACKABLE        = 0x04,  // 不可追踪标志
    UNIT_STAND_FLAGS_UNK4               = 0x08,  // 未知标志 4
    UNIT_STAND_FLAGS_UNK5               = 0x10,  // 未知标志 5
    UNIT_STAND_FLAGS_ALL                = 0xFF   // 所有站立标志
};

// UNIT_FIELD_BYTES_1 的第 3 字节标志值，单位字节 1 标志枚举
enum UnitBytes1_Flags
{
    UNIT_BYTE1_FLAG_GROUND              = 0x00,  // 地面标志
    UNIT_BYTE1_FLAG_ALWAYS_STAND        = 0x01,  // 始终站立标志
    UNIT_BYTE1_FLAG_HOVER               = 0x02,  // 悬停标志
    UNIT_BYTE1_FLAG_FLY                 = 0x03,  // 飞行标志
    UNIT_BYTE1_FLAG_SUBMERGED           = 0x04,  // 浸没标志
    UNIT_BYTE1_FLAG_ALL                 = 0xFF   // 所有字节 1 标志
};

// UNIT_FIELD_BYTES_2 的高字节（第 3 字节），变形形态枚举
enum ShapeshiftForm
{
    FORM_NONE                           = 0x00,  // 无变形形态
    FORM_CAT                            = 0x01,  // 猫形态
    FORM_TREE                           = 0x02,  // 树人形态
    FORM_TRAVEL                         = 0x03,  // 旅行形态
    FORM_AQUA                           = 0x04,  // 水栖形态
    FORM_BEAR                           = 0x05,  // 熊形态
    FORM_AMBIENT                        = 0x06,  // 环境形态
    FORM_GHOUL                          = 0x07,  // 食尸鬼形态
    FORM_DIREBEAR                       = 0x08,  // 恐怖熊形态
    FORM_STEVES_GHOUL                   = 0x09,  // Steves 食尸鬼形态
    FORM_THARONJA_SKELETON              = 0x0A,  // 塔隆亚骷髅形态
    FORM_TEST_OF_STRENGTH               = 0x0B,  // 力量试炼形态
    FORM_BLB_PLAYER                     = 0x0C,  // BLB 玩家形态
    FORM_SHADOW_DANCE                   = 0x0D,  // 暗影舞步形态
    FORM_CREATUREBEAR                   = 0x0E,  // 生物熊形态
    FORM_CREATURECAT                    = 0x0F,  // 生物猫形态
    FORM_GHOSTWOLF                      = 0x10,  // 幽灵狼形态
    FORM_BATTLESTANCE                   = 0x11,  // 战斗姿态
    FORM_DEFENSIVESTANCE                = 0x12,  // 防御姿态
    FORM_BERSERKERSTANCE                = 0x13,  // 狂暴姿态
    FORM_TEST                           = 0x14,  // 测试形态
    FORM_ZOMBIE                         = 0x15,  // 僵尸形态
    FORM_METAMORPHOSIS                  = 0x16,  // 变形形态
    FORM_UNDEAD                         = 0x19,  // 亡灵形态
    FORM_MASTER_ANGLER                  = 0x1A,  // 垂钓大师形态
    FORM_FLIGHT_EPIC                    = 0x1B,  // 史诗飞行形态
    FORM_SHADOW                         = 0x1C,  // 暗影形态
    FORM_FLIGHT                         = 0x1D,  // 飞行形态
    FORM_STEALTH                        = 0x1E,  // 潜行形态
    FORM_MOONKIN                        = 0x1F,  // 枭兽形态
    FORM_SPIRITOFREDEMPTION             = 0x20   // 救赎之魂形态
};

// 变形标志枚举
enum ShapeshiftFlags
{
    SHAPESHIFT_FLAG_STANCE                          = 0x00000001,   // 形态允许各种玩家活动，这些活动通常会导致 "变形时无法执行此操作" 错误（与 NPC/游戏对象交互、使用物品等）
    SHAPESHIFT_FLAG_NOT_TOGGLEABLE                  = 0x00000002,   // 未实现
    SHAPESHIFT_FLAG_PERSIST_ON_DEATH                = 0x00000004,   // 未实现
    SHAPESHIFT_FLAG_CAN_NPC_INTERACT                = 0x00000008,   // 形态无条件允许在变形时与 NPC 对话（即使其他活动被禁用）
    SHAPESHIFT_FLAG_DONT_USE_WEAPON                 = 0x00000010,   // 未实现
    SHAPESHIFT_FLAG_AGILITY_ATTACK_BONUS            = 0x00000020,   // 德鲁伊猫形态
    SHAPESHIFT_FLAG_CAN_USE_EQUIPPED_ITEMS          = 0x00000040,   // 未实现
    SHAPESHIFT_FLAG_CAN_USE_ITEMS                   = 0x00000080,   // 未实现
    SHAPESHIFT_FLAG_DONT_AUTO_UNSHIFT               = 0x00000100,   // 在客户端处理
    SHAPESHIFT_FLAG_CONSIDERED_DEAD                 = 0x00000200,   // 未实现
    SHAPESHIFT_FLAG_CAN_ONLY_CAST_SHAPESHIFT_SPELLS = 0x00000400,   // 未实现
    SHAPESHIFT_FLAG_STANCE_CANCEL_AT_FLIGHTMASTER   = 0x00000800,   // 未实现
    SHAPESHIFT_FLAG_NO_EMOTE_SOUNDS                 = 0x00001000,   // 未实现
    SHAPESHIFT_FLAG_NO_TRIGGER_TELEPORT             = 0x00002000,   // 未实现
    SHAPESHIFT_FLAG_CANNOT_CHANGE_EQUIPPED_ITEMS    = 0x00004000,   // 未实现
    SHAPESHIFT_FLAG_RESUMMON_PETS_ON_UNSHIFT        = 0x00008000,   // 未实现
    SHAPESHIFT_FLAG_CANNOT_USE_GAME_OBJECTS         = 0x00010000,   // 未实现
};

// UNIT_FIELD_BYTES_2 的低字节（第 0 字节），武器收放状态枚举
enum SheathState
{
    SHEATH_STATE_UNARMED                = 0,        // 未准备武器状态
    SHEATH_STATE_MELEE                  = 1,        // 准备近战武器状态
    SHEATH_STATE_RANGED                 = 2         // 准备远程武器状态
};

// 最大武器收放状态值
#define MAX_SHEATH_STATE    3

// UNIT_FIELD_BYTES_2 的第 1 字节，单位 PVP 状态标志枚举
enum UnitPVPStateFlags
{
    UNIT_BYTE2_FLAG_PVP                 = 0x01,  // PVP 标志
    UNIT_BYTE2_FLAG_UNK1                = 0x02,  // 未知标志 1
    UNIT_BYTE2_FLAG_FFA_PVP             = 0x04,  // 自由 PVP 标志
    UNIT_BYTE2_FLAG_SANCTUARY           = 0x08,  // 安全区标志
    UNIT_BYTE2_FLAG_UNK4                = 0x10,  // 未知标志 4
    UNIT_BYTE2_FLAG_UNK5                = 0x20,  // 未知标志 5
    UNIT_BYTE2_FLAG_UNK6                = 0x40,  // 未知标志 6
    UNIT_BYTE2_FLAG_UNK7                = 0x80   // 未知标志 7
};

// UNIT_FIELD_BYTES_2 的第 2 字节，单位重命名标志枚举
enum UnitRename
{
    UNIT_CAN_BE_RENAMED                 = 0x01,  // 单位可以被重命名
    UNIT_CAN_BE_ABANDONED               = 0x02   // 单位可以被放弃
};

// 单位类型掩码枚举
enum UnitTypeMask
{
    UNIT_MASK_NONE                      = 0x00000000,  // 无类型掩码
    UNIT_MASK_SUMMON                    = 0x00000001,  // 召唤物类型掩码
    UNIT_MASK_MINION                    = 0x00000002,  // 随从类型掩码
    UNIT_MASK_GUARDIAN                  = 0x00000004,  // 守护者类型掩码
    UNIT_MASK_TOTEM                     = 0x00000008,  // 图腾类型掩码
    UNIT_MASK_PET                       = 0x00000010,  // 宠物类型掩码
    UNIT_MASK_VEHICLE                   = 0x00000020,  // 载具类型掩码
    UNIT_MASK_PUPPET                    = 0x00000040,  // 傀儡类型掩码
    UNIT_MASK_HUNTER_PET                = 0x00000080,  // 猎人宠物类型掩码
    UNIT_MASK_CONTROLLABLE_GUARDIAN     = 0x00000100,  // 可控制守护者类型掩码
    UNIT_MASK_ACCESSORY                 = 0x00000200   // 附件类型掩码
};

// 单位状态枚举
enum UnitState
{
    UNIT_STATE_DIED                     = 0x00000001,       // 玩家拥有假死光环
    UNIT_STATE_MELEE_ATTACKING          = 0x00000002,       // 玩家正在近战攻击某人
    //UNIT_STATE_MELEE_ATTACK_BY        = 0x00000004,       // 玩家正在被近战攻击
    UNIT_STATE_STUNNED                  = 0x00000008,       // 单位被眩晕
    UNIT_STATE_ROAMING                  = 0x00000010,       // 单位正在闲逛
    UNIT_STATE_CHASE                    = 0x00000020,       // 单位正在追击
    //UNIT_STATE_SEARCHING              = 0x00000040,
    UNIT_STATE_FLEEING                  = 0x00000080,       // 单位正在逃跑
    UNIT_STATE_IN_FLIGHT                = 0x00000100,       // 玩家处于飞行模式
    UNIT_STATE_FOLLOW                   = 0x00000200,       // 单位正在跟随
    UNIT_STATE_ROOT                     = 0x00000400,       // 单位被定身
    UNIT_STATE_CONFUSED                 = 0x00000800,       // 单位处于困惑状态
    UNIT_STATE_DISTRACTED               = 0x00001000,       // 单位被分散注意力
    UNIT_STATE_ISOLATED                 = 0x00002000,       // 区域光环不影响其他玩家
    UNIT_STATE_ATTACK_PLAYER            = 0x00004000,       // 单位正在攻击玩家
    UNIT_STATE_CASTING                  = 0x00008000,       // 单位正在施法
    UNIT_STATE_POSSESSED                = 0x00010000,       // 单位被玩家直接控制（控制或载具）
    UNIT_STATE_CHARGING                 = 0x00020000,       // 单位正在冲锋
    UNIT_STATE_JUMPING                  = 0x00040000,       // 单位正在跳跃
    UNIT_STATE_MOVE                     = 0x00100000,       // 单位正在移动
    UNIT_STATE_ROTATING                 = 0x00200000,       // 单位正在旋转
    UNIT_STATE_EVADE                    = 0x00400000,       // 单位正在闪避
    UNIT_STATE_ROAMING_MOVE             = 0x00800000,       // 单位闲逛移动
    UNIT_STATE_CONFUSED_MOVE            = 0x01000000,       // 单位困惑移动
    UNIT_STATE_FLEEING_MOVE             = 0x02000000,       // 单位逃跑移动
    UNIT_STATE_CHASE_MOVE               = 0x04000000,       // 单位追击移动
    UNIT_STATE_FOLLOW_MOVE              = 0x08000000,       // 单位跟随移动
    UNIT_STATE_IGNORE_PATHFINDING       = 0x10000000,       // 在任何移动生成器中不使用寻路
    UNIT_STATE_NO_ENVIRONMENT_UPD       = 0x20000000,       // 不更新环境

    // 所有支持的单位状态
    UNIT_STATE_ALL_STATE_SUPPORTED = UNIT_STATE_DIED | UNIT_STATE_MELEE_ATTACKING | UNIT_STATE_STUNNED | UNIT_STATE_ROAMING | UNIT_STATE_CHASE
    | UNIT_STATE_FLEEING | UNIT_STATE_IN_FLIGHT | UNIT_STATE_FOLLOW | UNIT_STATE_ROOT | UNIT_STATE_CONFUSED
    | UNIT_STATE_DISTRACTED | UNIT_STATE_ISOLATED | UNIT_STATE_ATTACK_PLAYER | UNIT_STATE_CASTING
    | UNIT_STATE_POSSESSED | UNIT_STATE_CHARGING | UNIT_STATE_JUMPING | UNIT_STATE_MOVE | UNIT_STATE_ROTATING
    | UNIT_STATE_EVADE | UNIT_STATE_ROAMING_MOVE | UNIT_STATE_CONFUSED_MOVE | UNIT_STATE_FLEEING_MOVE
    | UNIT_STATE_CHASE_MOVE | UNIT_STATE_FOLLOW_MOVE | UNIT_STATE_IGNORE_PATHFINDING | UNIT_STATE_NO_ENVIRONMENT_UPD,

    // 不可攻击的单位状态
    UNIT_STATE_UNATTACKABLE             = UNIT_STATE_IN_FLIGHT,

    // 使用移动生成器检查和停止的实际移动状态（除了不可阻挡的飞行）
    UNIT_STATE_MOVING                   = UNIT_STATE_ROAMING_MOVE | UNIT_STATE_CONFUSED_MOVE | UNIT_STATE_FLEEING_MOVE | UNIT_STATE_CHASE_MOVE | UNIT_STATE_FOLLOW_MOVE,
    // 被控制的单位状态
    UNIT_STATE_CONTROLLED               = (UNIT_STATE_CONFUSED | UNIT_STATE_STUNNED | UNIT_STATE_FLEEING),
    // 失去控制的单位状态
    UNIT_STATE_LOST_CONTROL             = (UNIT_STATE_CONTROLLED | UNIT_STATE_JUMPING | UNIT_STATE_CHARGING),
    // 失明的单位状态
    UNIT_STATE_SIGHTLESS                = (UNIT_STATE_LOST_CONTROL | UNIT_STATE_EVADE),
    // 无法自动攻击的单位状态
    UNIT_STATE_CANNOT_AUTOATTACK        = (UNIT_STATE_LOST_CONTROL | UNIT_STATE_CASTING),
    // 无法转向的单位状态
    UNIT_STATE_CANNOT_TURN              = (UNIT_STATE_LOST_CONTROL | UNIT_STATE_ROTATING | UNIT_STATE_ROOT),

    // 因不同原因停止移动的单位状态
    UNIT_STATE_NOT_MOVE                 = UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_DIED | UNIT_STATE_DISTRACTED,
    // 忽略反加速作弊的单位状态
    UNIT_STATE_IGNORE_ANTISPEEDHACK     = UNIT_STATE_FLEEING | UNIT_STATE_CONFUSED | UNIT_STATE_CHARGING | UNIT_STATE_DISTRACTED | UNIT_STATE_POSSESSED,
    // 所有单位状态
    UNIT_STATE_ALL_STATE                = 0xffffffff        //(UNIT_STATE_STOPPED | UNIT_STATE_MOVING | UNIT_STATE_IN_COMBAT | UNIT_STATE_IN_FLIGHT)
};

// 用于 IsClass 钩子的类上下文枚举
enum ClassContext : uint8
{
    CLASS_CONTEXT_NONE                  = 0,    // 默认上下文
    CLASS_CONTEXT_INIT                  = 1,    // 初始化上下文
    CLASS_CONTEXT_TELEPORT              = 2,    // 传送上下文
    CLASS_CONTEXT_QUEST                 = 3,    // 任务上下文
    CLASS_CONTEXT_STATS                 = 4,    // 统计上下文
    CLASS_CONTEXT_TAXI                  = 5,    // 飞行点上下文
    CLASS_CONTEXT_SKILL                 = 6,    // 技能上下文
    CLASS_CONTEXT_TALENT_POINT_CALC     = 7,    // 天赋点计算上下文
    CLASS_CONTEXT_ABILITY               = 8,    // 技能上下文
    CLASS_CONTEXT_ABILITY_REACTIVE      = 9,    // 被动技能上下文
    CLASS_CONTEXT_PET                   = 10,   // 宠物上下文
    CLASS_CONTEXT_PET_CHARM             = 11,   // 宠物魅惑上下文
    CLASS_CONTEXT_EQUIP_RELIC           = 12,   // 装备圣物上下文
    CLASS_CONTEXT_EQUIP_SHIELDS         = 13,   // 装备盾牌上下文
    CLASS_CONTEXT_EQUIP_ARMOR_CLASS     = 14,   // 装备护甲等级上下文
    CLASS_CONTEXT_WEAPON_SWAP           = 15,   // 武器切换上下文
    CLASS_CONTEXT_GRAVEYARD             = 16,   // 墓地上下文
    CLASS_CONTEXT_CLASS_TRAINER         = 17    // 职业训练师上下文
};

// UNIT_FIELD_FLAGS 的值掩码，单位标志枚举
enum UnitFlags : uint32
{
    UNIT_FLAG_NONE                          = 0x00000000,
    UNIT_FLAG_SERVER_CONTROLLED             = 0x00000001,       // 仅当单位移动由服务器控制时设置 - 通过 SPLINE/MONSTER_MOVE 数据包，与 UNIT_FLAG_STUNNED 一起设置；仅设置给客户端控制的单位；当为所有者设置时，客户端函数 CGUnit_C::IsClientControlled 返回 false
    UNIT_FLAG_NON_ATTACKABLE                = 0x00000002,       // 不可攻击
    UNIT_FLAG_DISABLE_MOVE                  = 0x00000004,       // 禁用移动
    UNIT_FLAG_PLAYER_CONTROLLED             = 0x00000008,       // 由玩家控制，使用 _IMMUNE_TO_PC 而非 _IMMUNE_TO_NPC
    UNIT_FLAG_RENAME                        = 0x00000010,       // 可重命名标志
    UNIT_FLAG_PREPARATION                   = 0x00000020,       // 对具有 SPELL_ATTR5_NO_REAGENT_COST_WITH_AURA 的法术不消耗材料
    UNIT_FLAG_UNK_6                         = 0x00000040,       // 未知标志 6
    UNIT_FLAG_NOT_ATTACKABLE_1              = 0x00000080,       // ?? (UNIT_FLAG_PLAYER_CONTROLLED | UNIT_FLAG_NOT_ATTACKABLE_1) 是 NON_PVP_ATTACKABLE
    UNIT_FLAG_IMMUNE_TO_PC                  = 0x00000100,       // 禁用与玩家角色 (PC) 的战斗/协助 - 见 Unit::_IsValidAttackTarget, Unit::_IsValidAssistTarget
    UNIT_FLAG_IMMUNE_TO_NPC                 = 0x00000200,       // 禁用与非玩家角色 (NPC) 的战斗/协助 - 见 Unit::_IsValidAttackTarget, Unit::_IsValidAssistTarget
    UNIT_FLAG_LOOTING                       = 0x00000400,       // 正在进行拾取动画
    UNIT_FLAG_PET_IN_COMBAT                 = 0x00000800,       // 是否在战斗中?, 2.0.8
    UNIT_FLAG_PVP                           = 0x00001000,       // 在 3.0.3 中更改
    UNIT_FLAG_SILENCED                      = 0x00002000,       // 被沉默, 2.1.1
    UNIT_FLAG_CANNOT_SWIM                   = 0x00004000,       // 2.0.8
    UNIT_FLAG_SWIMMING                      = 0x00008000,       // 在水中显示游泳动画
    UNIT_FLAG_NON_ATTACKABLE_2              = 0x00010000,       // 移除可攻击图标，如果是自己，则不能协助自己，但可以施放目标为自己的法术 - 由 SPELL_AURA_MOD_UNATTACKABLE 添加
    UNIT_FLAG_PACIFIED                      = 0x00020000,       // 3.0.3 确认
    UNIT_FLAG_STUNNED                       = 0x00040000,       // 3.0.3 确认
    UNIT_FLAG_IN_COMBAT                     = 0x00080000,       // 单位处于战斗状态
    UNIT_FLAG_TAXI_FLIGHT                   = 0x00100000,       // 禁用客户端侧不允许在飞行点飞行时使用的法术（可能与坐骑有关？），可能与 0x4 标志一起使用
    UNIT_FLAG_DISARMED                      = 0x00200000,       // 3.0.3，禁用近战法术施放...，近战法术工具提示中添加 "需要近战武器"
    UNIT_FLAG_CONFUSED                      = 0x00400000,       // 单位处于困惑状态
    UNIT_FLAG_FLEEING                       = 0x00800000,       // 单位正在逃跑
    UNIT_FLAG_POSSESSED                     = 0x01000000,       // 由玩家直接控制（控制或载具）
    UNIT_FLAG_NOT_SELECTABLE                = 0x02000000,       // 单位不可被选中
    UNIT_FLAG_SKINNABLE                     = 0x04000000,       // 单位可被剥皮
    UNIT_FLAG_MOUNT                         = 0x08000000,       // 单位是坐骑
    UNIT_FLAG_UNK_28                        = 0x10000000,       // 未知标志 28
    UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT = 0x20000000,       // 防止从解析聊天文本中自动播放表情，例如 /say 中的 "lol"，以 ? 或 ! 结尾的消息，或使用 /yell
    UNIT_FLAG_SHEATHE                       = 0x40000000,       // 武器收鞘标志
    UNIT_FLAG_IMMUNE                        = 0x80000000        // 免疫伤害
};
DEFINE_ENUM_FLAG(UnitFlags);

// UNIT_FIELD_FLAGS_2 的值掩码，单位标志 2 枚举
enum UnitFlags2 : uint32
{
    UNIT_FLAG2_NONE                         = 0x00000000,
    UNIT_FLAG2_FEIGN_DEATH                  = 0x00000001,       // 假死标志
    UNIT_FLAG2_HIDE_BODY                    = 0x00000002,       // 隐藏单位模型（仅显示玩家装备）
    UNIT_FLAG2_IGNORE_REPUTATION            = 0x00000004,       // 忽略声望标志
    UNIT_FLAG2_COMPREHEND_LANG              = 0x00000008,       // 理解语言标志
    UNIT_FLAG2_MIRROR_IMAGE                 = 0x00000010,       // 镜像图像标志
    UNIT_FLAG2_DO_NOT_FADE_IN               = 0x00000020,       // 单位模型召唤时立即出现（不淡入）
    UNIT_FLAG2_FORCE_MOVEMENT               = 0x00000040,       // 强制移动标志
    UNIT_FLAG2_DISARM_OFFHAND               = 0x00000080,       // 禁用副手武器标志
    UNIT_FLAG2_DISABLE_PRED_STATS           = 0x00000100,       // 玩家已禁用预测统计（用于团队框架）
    UNIT_FLAG2_DISARM_RANGED                = 0x00000400,       // 此标志不禁用远程武器显示（可能需要额外标志？）
    UNIT_FLAG2_REGENERATE_POWER             = 0x00000800,       // 恢复能量标志
    UNIT_FLAG2_RESTRICT_PARTY_INTERACTION   = 0x00001000,       // 限制与队伍或团队的交互
    UNIT_FLAG2_PREVENT_SPELL_CLICK          = 0x00002000,       // 防止法术点击
    UNIT_FLAG2_ALLOW_ENEMY_INTERACT         = 0x00004000,       // 允许与敌人交互
    UNIT_FLAG2_CANNOT_TURN                  = 0x00008000,       // 无法转向标志
    UNIT_FLAG2_UNK2                         = 0x00010000,       // 未知标志 2
    UNIT_FLAG2_PLAY_DEATH_ANIM              = 0x00020000,       // 死亡时播放特殊死亡动画
    UNIT_FLAG2_ALLOW_CHEAT_SPELLS           = 0x00040000,       // 允许施放具有 AttributesEx7 & SPELL_ATTR7_DEBUG_SPELL 的法术
    UNIT_FLAG2_UNUSED_6                     = 0x01000000        // 未使用标志 6
};
DEFINE_ENUM_FLAG(UnitFlags2);

/// 非玩家角色标志枚举
enum NPCFlags : uint32
{
    UNIT_NPC_FLAG_NONE                      = 0x00000000,       // 无 NPC 标志
    UNIT_NPC_FLAG_GOSSIP                    = 0x00000001,       // 有对话菜单
    UNIT_NPC_FLAG_QUESTGIVER                = 0x00000002,       // 是任务给予者
    UNIT_NPC_FLAG_UNK1                      = 0x00000004,       // 未知标志 1
    UNIT_NPC_FLAG_UNK2                      = 0x00000008,       // 未知标志 2
    UNIT_NPC_FLAG_TRAINER                   = 0x00000010,       // 是训练师
    UNIT_NPC_FLAG_TRAINER_CLASS             = 0x00000020,       // 是职业训练师
    UNIT_NPC_FLAG_TRAINER_PROFESSION        = 0x00000040,       // 是专业技能训练师
    UNIT_NPC_FLAG_VENDOR                    = 0x00000080,       // 是通用商人
    UNIT_NPC_FLAG_VENDOR_AMMO               = 0x00000100,       // 是弹药商人，综合商品商人
    UNIT_NPC_FLAG_VENDOR_FOOD               = 0x00000200,       // 是食物商人
    UNIT_NPC_FLAG_VENDOR_POISON             = 0x00000400,       // 是毒药商人
    UNIT_NPC_FLAG_VENDOR_REAGENT            = 0x00000800,       // 是材料商人
    UNIT_NPC_FLAG_REPAIR                    = 0x00001000,       // 可修理物品
    UNIT_NPC_FLAG_FLIGHTMASTER              = 0x00002000,       // 是飞行管理员
    UNIT_NPC_FLAG_SPIRITHEALER              = 0x00004000,       // 是灵魂医者
    UNIT_NPC_FLAG_SPIRITGUIDE               = 0x00008000,       // 是灵魂向导
    UNIT_NPC_FLAG_INNKEEPER                 = 0x00010000,       // 是旅店老板
    UNIT_NPC_FLAG_BANKER                    = 0x00020000,       // 是银行家
    UNIT_NPC_FLAG_PETITIONER                = 0x00040000,       // 处理公会/竞技场申请，0xC0000 = 公会申请，0x40000 = 竞技场队伍申请
    UNIT_NPC_FLAG_TABARDDESIGNER            = 0x00080000,       // 是公会徽章设计师
    UNIT_NPC_FLAG_BATTLEMASTER              = 0x00100000,       // 是战场管理员
    UNIT_NPC_FLAG_AUCTIONEER                = 0x00200000,       // 是拍卖师
    UNIT_NPC_FLAG_STABLEMASTER              = 0x00400000,       // 是兽栏管理员
    UNIT_NPC_FLAG_GUILD_BANKER              = 0x00800000,       // 是公会银行家，会使客户端发送 997 操作码
    UNIT_NPC_FLAG_SPELLCLICK                = 0x01000000,       // 启用法术点击，会使客户端发送 1015 操作码（法术点击）
    UNIT_NPC_FLAG_PLAYER_VEHICLE            = 0x02000000,       // 是玩家载具，有载具数据的坐骑玩家应该设置此标志
    UNIT_NPC_FLAG_MAILBOX                   = 0x04000000,       // 是邮箱

    // 商人标志掩码
    UNIT_NPC_FLAG_VENDOR_MASK = UNIT_NPC_FLAG_VENDOR | UNIT_NPC_FLAG_VENDOR_AMMO | UNIT_NPC_FLAG_VENDOR_POISON | UNIT_NPC_FLAG_VENDOR_REAGENT
};
DEFINE_ENUM_FLAG(NPCFlags);

// 单位移动类型枚举
enum UnitMoveType
{
    MOVE_WALK           = 0,  // 行走
    MOVE_RUN            = 1,  // 奔跑
    MOVE_RUN_BACK       = 2,  // 后退奔跑
    MOVE_SWIM           = 3,  // 游泳
    MOVE_SWIM_BACK      = 4,  // 后退游泳
    MOVE_TURN_RATE      = 5,  // 转身速率
    MOVE_FLIGHT         = 6,  // 飞行
    MOVE_FLIGHT_BACK    = 7,  // 后退飞行
    MOVE_PITCH_RATE     = 8   // 俯仰速率
};

// 最大移动类型值
#define MAX_MOVE_TYPE     9

// 移动标志枚举
enum MovementFlags
{
    MOVEMENTFLAG_NONE                       = 0x00000000,
    MOVEMENTFLAG_FORWARD                    = 0x00000001,       // 向前移动
    MOVEMENTFLAG_BACKWARD                   = 0x00000002,       // 向后移动
    MOVEMENTFLAG_STRAFE_LEFT                = 0x00000004,       // 向左平移
    MOVEMENTFLAG_STRAFE_RIGHT               = 0x00000008,       // 向右平移
    MOVEMENTFLAG_LEFT                       = 0x00000010,       // 向左转向
    MOVEMENTFLAG_RIGHT                      = 0x00000020,       // 向右转向
    MOVEMENTFLAG_PITCH_UP                   = 0x00000040,       // 向上俯仰
    MOVEMENTFLAG_PITCH_DOWN                 = 0x00000080,       // 向下俯仰
    MOVEMENTFLAG_WALKING                    = 0x00000100,       // 行走状态
    MOVEMENTFLAG_ONTRANSPORT                = 0x00000200,       // 用于在某些生物上飞行
    MOVEMENTFLAG_DISABLE_GRAVITY            = 0x00000400,       // 前 MOVEMENTFLAG_LEVITATING。当无法行走时使用
    MOVEMENTFLAG_ROOT                       = 0x00000800,       // 必须不与 MOVEMENTFLAG_MASK_MOVING 一起设置
    MOVEMENTFLAG_FALLING                    = 0x00001000,       // 这种类型的坠落会造成伤害
    MOVEMENTFLAG_FALLING_FAR                = 0x00002000,
    MOVEMENTFLAG_PENDING_STOP               = 0x00004000,
    MOVEMENTFLAG_PENDING_STRAFE_STOP        = 0x00008000,
    MOVEMENTFLAG_PENDING_FORWARD            = 0x00010000,
    MOVEMENTFLAG_PENDING_BACKWARD           = 0x00020000,
    MOVEMENTFLAG_PENDING_STRAFE_LEFT        = 0x00040000,
    MOVEMENTFLAG_PENDING_STRAFE_RIGHT       = 0x00080000,
    MOVEMENTFLAG_PENDING_ROOT               = 0x00100000,
    MOVEMENTFLAG_SWIMMING                   = 0x00200000,       // 游泳状态，也可能与飞行标志一起出现
    MOVEMENTFLAG_ASCENDING                  = 0x00400000,       // 飞行时按下 "空格"
    MOVEMENTFLAG_DESCENDING                 = 0x00800000,       // 下降状态
    MOVEMENTFLAG_CAN_FLY                    = 0x01000000,       // 单位可以飞行且也能行走
    MOVEMENTFLAG_FLYING                     = 0x02000000,       // 单位实际正在飞行。很确定这仅用于玩家。生物使用 disable_gravity
    MOVEMENTFLAG_SPLINE_ELEVATION           = 0x04000000,       // 用于飞行路径
    MOVEMENTFLAG_SPLINE_ENABLED             = 0x08000000,       // 用于飞行路径
    MOVEMENTFLAG_WATERWALKING               = 0x10000000,       // 防止单位掉入水中
    MOVEMENTFLAG_FALLING_SLOW               = 0x20000000,       // 激活潜行者的安全坠落法术（被动）
    MOVEMENTFLAG_HOVER                      = 0x40000000,       // 悬停，无法跳跃

    /// @todo: 检查 PITCH_UP 和 PITCH_DOWN 是否真的属于这里..
    // 移动中的标志掩码
    MOVEMENTFLAG_MASK_MOVING =
    MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT |
    MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING |
    MOVEMENTFLAG_SPLINE_ELEVATION,

    // 转向中的标志掩码
    MOVEMENTFLAG_MASK_TURNING =
    MOVEMENTFLAG_LEFT | MOVEMENTFLAG_RIGHT,

    // 飞行移动中的标志掩码
    MOVEMENTFLAG_MASK_MOVING_FLY =
    MOVEMENTFLAG_FLYING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING,

    /// @todo 如果需要: 在此掩码中添加更多仅玩家特有的标志
    // 仅玩家的标志掩码
    MOVEMENTFLAG_MASK_PLAYER_ONLY =
    MOVEMENTFLAG_FLYING,

    /// 有玩家状态操作码关联的移动标志
    MOVEMENTFLAG_MASK_HAS_PLAYER_STATUS_OPCODE = MOVEMENTFLAG_DISABLE_GRAVITY | MOVEMENTFLAG_ROOT |
    MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_WATERWALKING | MOVEMENTFLAG_FALLING_SLOW | MOVEMENTFLAG_HOVER
};

// 移动标志 2 枚举
enum MovementFlags2
{
    MOVEMENTFLAG2_NONE                      = 0x00000000,
    MOVEMENTFLAG2_NO_STRAFE                 = 0x00000001,       // 禁止平移
    MOVEMENTFLAG2_NO_JUMPING                = 0x00000002,       // 禁止跳跃
    MOVEMENTFLAG2_UNK3                      = 0x00000004,        // 覆盖各种客户端检查
    MOVEMENTFLAG2_FULL_SPEED_TURNING        = 0x00000008,       // 全速转向
    MOVEMENTFLAG2_FULL_SPEED_PITCHING       = 0x00000010,       // 全速俯仰
    MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING     = 0x00000020,       // 始终允许俯仰
    MOVEMENTFLAG2_UNK7                      = 0x00000040,       // 未知标志 7
    MOVEMENTFLAG2_UNK8                      = 0x00000080,       // 未知标志 8
    MOVEMENTFLAG2_UNK9                      = 0x00000100,       // 未知标志 9
    MOVEMENTFLAG2_UNK10                     = 0x00000200,       // 未知标志 10
    MOVEMENTFLAG2_INTERPOLATED_MOVEMENT     = 0x00000400,       // 插值移动
    MOVEMENTFLAG2_INTERPOLATED_TURNING      = 0x00000800,       // 插值转向
    MOVEMENTFLAG2_INTERPOLATED_PITCHING     = 0x00001000,       // 插值俯仰
    MOVEMENTFLAG2_UNK14                     = 0x00002000,       // 未知标志 14
    MOVEMENTFLAG2_UNK15                     = 0x00004000,       // 未知标志 15
    MOVEMENTFLAG2_UNK16                     = 0x00008000        // 未知标志 16
};

// 样条标志枚举
enum SplineFlags
{
    SPLINEFLAG_NONE                         = 0x00000000,
    SPLINEFLAG_FORWARD                      = 0x00000001,       // 向前
    SPLINEFLAG_BACKWARD                     = 0x00000002,       // 向后
    SPLINEFLAG_STRAFE_LEFT                  = 0x00000004,       // 向左平移
    SPLINEFLAG_STRAFE_RIGHT                 = 0x00000008,       // 向右平移
    SPLINEFLAG_TURN_LEFT                    = 0x00000010,       // 向左转向
    SPLINEFLAG_TURN_RIGHT                   = 0x00000020,       // 向右转向
    SPLINEFLAG_PITCH_UP                     = 0x00000040,       // 向上俯仰
    SPLINEFLAG_PITCH_DOWN                   = 0x00000080,       // 向下俯仰
    SPLINEFLAG_DONE                         = 0x00000100,       // 完成
    SPLINEFLAG_FALLING                      = 0x00000200,       // 坠落
    SPLINEFLAG_NO_SPLINE                    = 0x00000400,       // 无样条
    SPLINEFLAG_TRAJECTORY                   = 0x00000800,       // 轨迹
    SPLINEFLAG_WALK_MODE                    = 0x00001000,       // 行走模式
    SPLINEFLAG_FLYING                       = 0x00002000,       // 飞行
    SPLINEFLAG_KNOCKBACK                    = 0x00004000,       // 击退
    SPLINEFLAG_FINAL_POINT                  = 0x00008000,       // 最终点
    SPLINEFLAG_FINAL_TARGET                 = 0x00010000,       // 最终目标
    SPLINEFLAG_FINAL_FACING                 = 0x00020000,       // 最终朝向
    SPLINEFLAG_CATMULL_ROM                  = 0x00040000,       // 卡特穆尔 - 罗样条
    SPLINEFLAG_CYCLIC                       = 0x00080000,       // 循环
    SPLINEFLAG_ENTER_CYCLE                  = 0x00100000,       // 进入循环
    SPLINEFLAG_ANIMATION_TIER               = 0x00200000,       // 动画层级
    SPLINEFLAG_FROZEN                       = 0x00400000,       // 冻结
    SPLINEFLAG_TRANSPORT                    = 0x00800000,       // 载具
    SPLINEFLAG_TRANSPORT_EXIT               = 0x01000000,       // 离开载具
    SPLINEFLAG_UNKNOWN7                     = 0x02000000,       // 未知标志 7
    SPLINEFLAG_UNKNOWN8                     = 0x04000000,       // 未知标志 8
    SPLINEFLAG_ORIENTATION_INVERTED         = 0x08000000,       // 方向反转
    SPLINEFLAG_USE_PATH_SMOOTHING           = 0x10000000,       // 使用路径平滑
    SPLINEFLAG_ANIMATION                    = 0x20000000,       // 动画
    SPLINEFLAG_UNCOMPRESSED_PATH            = 0x40000000,       // 未压缩路径
    SPLINEFLAG_UNKNOWN10                    = 0x80000000        // 未知标志 10
};

// 样条类型枚举
enum SplineType
{
    SPLINETYPE_NORMAL           = 0,  // 普通样条
    SPLINETYPE_STOP             = 1,  // 停止样条
    SPLINETYPE_FACING_SPOT      = 2,  // 面向点样条
    SPLINETYPE_FACING_TARGET    = 3,  // 面向目标样条
    SPLINETYPE_FACING_ANGLE     = 4   // 面向角度样条
};
