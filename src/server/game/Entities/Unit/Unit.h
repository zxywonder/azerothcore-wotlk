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

#ifndef __UNIT_H
#define __UNIT_H

#include "EnumFlag.h"
#include "EventProcessor.h"
#include "FollowerRefMgr.h"
#include "FollowerReference.h"
#include "HostileRefMgr.h"
#include "ItemTemplate.h"
#include "MotionMaster.h"
#include "Object.h"
#include "SharedDefines.h"
#include "SpellAuraDefines.h"
#include "SpellDefines.h"
#include "ThreatMgr.h"
#include "UnitDefines.h"
#include "UnitUtils.h"
#include <functional>
#include <utility>

#define WORLD_TRIGGER   12999

#define BASE_MINDAMAGE 1.0f
#define BASE_MAXDAMAGE 2.0f
#define BASE_ATTACK_TIME 2000

#define MAX_AGGRO_RADIUS 45.0f  // yards

static constexpr uint32 MAX_CREATURE_SPELLS = 8;
static constexpr uint32 infinityCooldownDelay = 0x9A7EC800; // used for set "infinity cooldowns" for spells and check, MONTH*IN_MILLISECONDS
static constexpr uint32 infinityCooldownDelayCheck = 0x4D3F6400; // MONTH*IN_MILLISECONDS/2;

struct CharmInfo;
struct FactionTemplateEntry;
struct SpellValue;

class AuraApplication;
class Aura;
class UnitAura;
class AuraEffect;
class Creature;
class Spell;
class SpellInfo;
class DynamicObject;
class GameObject;
class Item;
class Pet;
class PetAura;
class Minion;
class Guardian;
class UnitAI;
class Totem;
class Transport;
class StaticTransport;
class MotionTransport;
class Vehicle;
class TransportBase;
class SpellCastTargets;

// 定义一个存储 Unit 指针的列表类型，用于方便管理一组 Unit 对象
typedef std::list<Unit*> UnitList;
// 定义一个存储 Aura 指针和 uint8 配对的列表类型，通常用于存储驱散充能的相关信息
typedef std::list< std::pair<Aura*, uint8> > DispelChargesList;

// 前向声明魅惑类型枚举，其底层类型为 uint8，具体枚举值在别处定义
enum CharmType : uint8;

// 定义受害者状态枚举，用于描述在攻击交互中受害者所处的不同状态
enum VictimState
{
    VICTIMSTATE_INTACT = 0, // 攻击者未命中时设置，受害者状态保持完好
    VICTIMSTATE_HIT = 1, // 受害者受到普通攻击或格挡攻击
    VICTIMSTATE_DODGE = 2, // 受害者成功闪避了攻击
    VICTIMSTATE_PARRY = 3, // 受害者成功招架了攻击
    VICTIMSTATE_INTERRUPT = 4, // 攻击动作被中断
    VICTIMSTATE_BLOCKS = 5, // 疑似未使用，即使在完全格挡时也不会设置该状态
    VICTIMSTATE_EVADES = 6, // 受害者成功躲避了攻击
    VICTIMSTATE_IS_IMMUNE = 7, // 受害者对攻击免疫
    VICTIMSTATE_DEFLECTS = 8  // 受害者成功偏转了攻击
};

// 定义命中信息枚举，用于记录攻击命中的各种状态和特征
enum HitInfo
{
    HITINFO_NORMALSWING = 0x00000000, // 普通攻击挥击
    HITINFO_UNK1 = 0x00000001, // 需要正确的数据包结构
    HITINFO_AFFECTS_VICTIM = 0x00000002, // 攻击影响到了受害者
    HITINFO_OFFHAND = 0x00000004, // 使用副手武器进行攻击
    HITINFO_UNK2 = 0x00000008, // 未知用途
    HITINFO_MISS = 0x00000010, // 攻击未命中
    HITINFO_FULL_ABSORB = 0x00000020, // 伤害被完全吸收
    HITINFO_PARTIAL_ABSORB = 0x00000040, // 伤害被部分吸收
    HITINFO_FULL_RESIST = 0x00000080, // 伤害被完全抵抗
    HITINFO_PARTIAL_RESIST = 0x00000100, // 伤害被部分抵抗
    HITINFO_CRITICALHIT = 0x00000200, // 暴击伤害
    HITINFO_UNK10 = 0x00000400, // 未知用途
    HITINFO_UNK11 = 0x00000800, // 未知用途
    HITINFO_UNK12 = 0x00001000, // 未知用途
    HITINFO_BLOCK = 0x00002000, // 伤害被格挡
    HITINFO_UNK14 = 0x00004000, // 仅当存在近战法术 ID 时设置，受害者受到 0 伤害时无世界文本
    HITINFO_UNK15 = 0x00008000, // 可能与玩家受害者有关，可能和背后血溅视觉效果相关
    HITINFO_GLANCING = 0x00010000, // 擦过攻击
    HITINFO_CRUSHING = 0x00020000, // 碾压攻击
    HITINFO_NO_ANIMATION = 0x00040000, // 攻击无动画效果
    HITINFO_UNK19 = 0x00080000, // 未知用途
    HITINFO_UNK20 = 0x00100000, // 未知用途
    HITINFO_SWINGNOHITSOUND = 0x00200000, // 疑似未使用，挥击无命中音效
    HITINFO_UNK22 = 0x00400000, // 未知用途
    HITINFO_RAGE_GAIN = 0x00800000, // 攻击获得怒气
    HITINFO_FAKE_DAMAGE = 0x01000000  // 即使没有造成伤害也启用伤害动画，仅在无伤害时设置
};

// 定义单位属性修改器类型枚举，用于指定属性修改的方式
enum UnitModifierType
{
    BASE_VALUE = 0,    // 基础值修改
    BASE_PCT = 1,      // 基础百分比修改
    TOTAL_VALUE = 2,   // 总值修改
    TOTAL_PCT = 3,     // 总百分比修改
    MODIFIER_TYPE_END = 4 // 修改器类型结束标记
};

// 定义武器伤害范围枚举，用于表示武器伤害的最小值和最大值
enum WeaponDamageRange
{
    MINDAMAGE,          // 最小伤害
    MAXDAMAGE,          // 最大伤害

    MAX_WEAPON_DAMAGE_RANGE // 武器伤害范围枚举结束标记
};

// 定义单位属性修改枚举，用于指定可以修改的单位属性
enum UnitMods
{
    UNIT_MOD_STAT_STRENGTH,                                 // 力量属性，UNIT_MOD_STAT_STRENGTH 到 UNIT_MOD_STAT_SPIRIT 必须按 Stats 枚举索引顺序排列
    UNIT_MOD_STAT_AGILITY,                                  // 敏捷属性
    UNIT_MOD_STAT_STAMINA,                                  // 耐力属性
    UNIT_MOD_STAT_INTELLECT,                                // 智力属性
    UNIT_MOD_STAT_SPIRIT,                                   // 精神属性
    UNIT_MOD_HEALTH,                                        // 生命值
    UNIT_MOD_MANA,                                          // 法力值，UNIT_MOD_MANA 到 UNIT_MOD_RUNIC_POWER 必须按 Powers 枚举索引顺序排列
    UNIT_MOD_RAGE,                                          // 怒气值
    UNIT_MOD_FOCUS,                                         // 集中值
    UNIT_MOD_ENERGY,                                        // 能量值
    UNIT_MOD_HAPPINESS,                                     // 快乐值
    UNIT_MOD_RUNE,                                          // 符文值
    UNIT_MOD_RUNIC_POWER,                                   // 符文能量值
    UNIT_MOD_ARMOR,                                         // 护甲值，UNIT_MOD_ARMOR 到 UNIT_MOD_RESISTANCE_ARCANE 必须按 SpellSchools 枚举索引顺序排列
    UNIT_MOD_RESISTANCE_HOLY,                               // 神圣抗性
    UNIT_MOD_RESISTANCE_FIRE,                               // 火焰抗性
    UNIT_MOD_RESISTANCE_NATURE,                             // 自然抗性
    UNIT_MOD_RESISTANCE_FROST,                              // 冰霜抗性
    UNIT_MOD_RESISTANCE_SHADOW,                             // 暗影抗性
    UNIT_MOD_RESISTANCE_ARCANE,                             // 奥术抗性
    UNIT_MOD_ATTACK_POWER,                                  // 近战攻击强度
    UNIT_MOD_ATTACK_POWER_RANGED,                           // 远程攻击强度
    UNIT_MOD_DAMAGE_MAINHAND,                               // 主手伤害
    UNIT_MOD_DAMAGE_OFFHAND,                                // 副手伤害
    UNIT_MOD_DAMAGE_RANGED,                                 // 远程伤害
    UNIT_MOD_END,                                           // 单位属性修改枚举结束标记
    // 同义词
    UNIT_MOD_STAT_START = UNIT_MOD_STAT_STRENGTH,           // 属性修改起始标记（力量）
    UNIT_MOD_STAT_END = UNIT_MOD_STAT_SPIRIT + 1,           // 属性修改结束标记（精神之后）
    UNIT_MOD_RESISTANCE_START = UNIT_MOD_ARMOR,             // 抗性修改起始标记（护甲）
    UNIT_MOD_RESISTANCE_END = UNIT_MOD_RESISTANCE_ARCANE + 1, // 抗性修改结束标记（奥术抗性之后）
    UNIT_MOD_POWER_START = UNIT_MOD_MANA,                   // 能量值修改起始标记（法力值）
    UNIT_MOD_POWER_END = UNIT_MOD_RUNIC_POWER + 1           // 能量值修改结束标记（符文能量值之后）
};

// 定义基础修改组枚举，用于指定不同类型的基础属性修改组
enum BaseModGroup
{
    CRIT_PERCENTAGE,          // 暴击百分比
    RANGED_CRIT_PERCENTAGE,   // 远程暴击百分比
    OFFHAND_CRIT_PERCENTAGE,  // 副手暴击百分比
    SHIELD_BLOCK_VALUE,       // 盾牌格挡值
    BASEMOD_END               // 基础修改组枚举结束标记
};

// 定义基础修改类型枚举，用于指定基础属性的修改类型
enum BaseModType
{
    FLAT_MOD,  // 固定值修改
    PCT_MOD    // 百分比修改
};

// 定义修改结束标记，等于 PCT_MOD + 1
#define MOD_END (PCT_MOD+1)

// 定义死亡状态枚举类，底层类型为 uint8，用于描述单位的死亡状态
enum class DeathState : uint8
{
    Alive = 0, // 存活状态
    JustDied = 1, // 刚刚死亡状态
    Corpse = 2, // 尸体状态
    Dead = 3, // 已死亡状态
    JustRespawned = 4  // 刚刚复活状态
};

// 声明全局变量，存储不同移动类型的基础移动速度
extern float baseMoveSpeed[MAX_MOVE_TYPE];
// 声明全局变量，存储玩家不同移动类型的基础移动速度
extern float playerBaseMoveSpeed[MAX_MOVE_TYPE];

// 定义武器攻击类型枚举，底层类型为 uint8，用于指定攻击使用的武器类型
enum WeaponAttackType : uint8
{
    BASE_ATTACK = 0, // 基础攻击（通常为主手攻击）
    OFF_ATTACK = 1, // 副手攻击
    RANGED_ATTACK = 2, // 远程攻击
    MAX_ATTACK     // 最大攻击类型标记
};

// 定义战斗评级枚举，用于指定不同类型的战斗评级
enum CombatRating
{
    CR_WEAPON_SKILL = 0, // 武器技能
    CR_DEFENSE_SKILL = 1, // 防御技能
    CR_DODGE = 2, // 闪避评级
    CR_PARRY = 3, // 招架评级
    CR_BLOCK = 4, // 格挡评级
    CR_HIT_MELEE = 5, // 近战命中评级
    CR_HIT_RANGED = 6, // 远程命中评级
    CR_HIT_SPELL = 7, // 法术命中评级
    CR_CRIT_MELEE = 8, // 近战暴击评级
    CR_CRIT_RANGED = 9, // 远程暴击评级
    CR_CRIT_SPELL = 10, // 法术暴击评级
    CR_HIT_TAKEN_MELEE = 11, // 受到近战攻击的命中评级
    CR_HIT_TAKEN_RANGED = 12, // 受到远程攻击的命中评级
    CR_HIT_TAKEN_SPELL = 13, // 受到法术攻击的命中评级
    CR_CRIT_TAKEN_MELEE = 14, // 受到近战暴击的评级
    CR_CRIT_TAKEN_RANGED = 15, // 受到远程暴击的评级
    CR_CRIT_TAKEN_SPELL = 16, // 受到法术暴击的评级
    CR_HASTE_MELEE = 17, // 近战急速评级
    CR_HASTE_RANGED = 18, // 远程急速评级
    CR_HASTE_SPELL = 19, // 法术急速评级
    CR_WEAPON_SKILL_MAINHAND = 20, // 主手武器技能
    CR_WEAPON_SKILL_OFFHAND = 21, // 副手武器技能
    CR_WEAPON_SKILL_RANGED = 22, // 远程武器技能
    CR_EXPERTISE = 23, // 精准评级
    CR_ARMOR_PENETRATION = 24  // 护甲穿透评级
};

// 定义最大战斗评级常量
#define MAX_COMBAT_RATING         25

// 定义伤害效果类型枚举，底层类型为 uint8，用于指定伤害的类型
enum DamageEffectType : uint8
{
    DIRECT_DAMAGE = 0, // 用于普通武器伤害（非职业技能或法术）
    SPELL_DIRECT_DAMAGE = 1, // 法术或职业技能伤害
    DOT = 2, // 持续伤害
    HEAL = 3, // 治疗效果
    NODAMAGE = 4, // 用于伤害应用到生命值但不应用到法术打断标志等情况
    SELF_DAMAGE = 5  // 自我伤害
};

// 声明 Movement 命名空间下的 MoveSpline 类
namespace Movement
{
    class MoveSpline;
}

// 定义递减等级枚举，用于指定递减效果的不同等级
enum DiminishingLevels
{
    DIMINISHING_LEVEL_1 = 0, // 递减等级 1
    DIMINISHING_LEVEL_2 = 1, // 递减等级 2
    DIMINISHING_LEVEL_3 = 2, // 递减等级 3
    DIMINISHING_LEVEL_IMMUNE = 3, // 免疫递减等级
    DIMINISHING_LEVEL_4 = 3, // 递减等级 4（与免疫等级相同）
    DIMINISHING_LEVEL_TAUNT_IMMUNE = 4  // 嘲讽免疫递减等级
};

// 定义递减返回结构，用于存储递减效果的相关信息
struct DiminishingReturn
{
    // 构造函数，初始化递减组、时间和命中计数
    DiminishingReturn(DiminishingGroup group, uint32 t, uint32 count)
        : DRGroup(group), stack(0), hitTime(t), hitCount(count)
    {
    }

    DiminishingGroup        DRGroup : 16; // 递减组，占 16 位
    uint16                  stack : 16;   // 堆叠层数，占 16 位
    uint32                  hitTime;    // 命中时间
    uint32                  hitCount;   // 命中计数
};

// 定义近战攻击结果枚举，用于描述近战攻击的不同结果
enum MeleeHitOutcome
{
    MELEE_HIT_EVADE,     // 闪避
    MELEE_HIT_MISS,      // 未命中
    MELEE_HIT_DODGE,     // 躲闪
    MELEE_HIT_BLOCK,     // 格挡
    MELEE_HIT_PARRY,     // 招架
    MELEE_HIT_GLANCING,  // 擦过
    MELEE_HIT_CRIT,      // 暴击
    MELEE_HIT_CRUSHING,  // 碾压
    MELEE_HIT_NORMAL     // 普通命中
};

// 定义额外攻击法术枚举，用于指定额外攻击的法术 ID
enum ExtraAttackSpells
{
    SPELL_SWORD_SPECIALIZATION = 16459, // 剑专精法术 ID
    SPELL_HACK_AND_SLASH = 66923  // 砍杀法术 ID
};

// 定义驱散信息类，用于存储驱散操作的相关信息
class DispelInfo
{
public:
    // 构造函数，初始化驱散者、驱散法术 ID 和移除的充能数
    explicit DispelInfo(Unit* dispeller, uint32 dispellerSpellId, uint8 chargesRemoved) :
        _dispellerUnit(dispeller), _dispellerSpell(dispellerSpellId), _chargesRemoved(chargesRemoved) {
    }

    // 获取驱散者指针
    [[nodiscard]] Unit* GetDispeller() const { return _dispellerUnit; }
    // 获取驱散法术 ID
    [[nodiscard]] uint32 GetDispellerSpellId() const { return _dispellerSpell; }
    // 获取移除的充能数
    [[nodiscard]] uint8 GetRemovedCharges() const { return _chargesRemoved; }
    // 设置移除的充能数
    void SetRemovedCharges(uint8 amount)
    {
        _chargesRemoved = amount;
    }
private:
    Unit* _dispellerUnit;  // 驱散者指针
    uint32 _dispellerSpell; // 驱散法术 ID
    uint8 _chargesRemoved;  // 移除的充能数
};

// 定义纯净伤害结构，用于存储经过吸收和减免后的伤害信息
struct CleanDamage
{
    // 构造函数，初始化吸收伤害、减免伤害、攻击类型和命中结果
    CleanDamage(uint32 mitigated, uint32 absorbed, WeaponAttackType _attackType, MeleeHitOutcome _hitOutCome) :
        absorbed_damage(absorbed), mitigated_damage(mitigated), attackType(_attackType), hitOutCome(_hitOutCome) {
    }

    uint32 absorbed_damage;  // 吸收的伤害值
    uint32 mitigated_damage; // 减免的伤害值

    WeaponAttackType attackType; // 攻击类型
    MeleeHitOutcome hitOutCome;  // 近战命中结果
};

// 前向声明计算伤害信息结构
struct CalcDamageInfo;
// 前向声明法术非近战伤害结构
struct SpellNonMeleeDamage;

// 定义伤害信息类，用于存储伤害相关的各种信息
class DamageInfo
{
private:
    Unit* const m_attacker;         // 攻击者指针
    Unit* const m_victim;           // 受害者指针
    uint32 m_damage;                // 伤害值
    SpellInfo const* const m_spellInfo; // 法术信息指针
    SpellSchoolMask const m_schoolMask; // 法术学校掩码
    DamageEffectType const m_damageType; // 伤害效果类型
    WeaponAttackType m_attackType;  // 武器攻击类型
    uint32 m_absorb;                // 吸收的伤害值
    uint32 m_resist;                // 抵抗的伤害值
    uint32 m_block;                 // 格挡的伤害值
    uint32 m_cleanDamage;           // 纯净伤害值，仅用于怒气计算

    // 合并构造函数（用于触发效果）
    DamageInfo(DamageInfo const& dmg1, DamageInfo const& dmg2);

public:
    // 构造函数，初始化伤害信息
    explicit DamageInfo(Unit* _attacker, Unit* _victim, uint32 _damage, SpellInfo const* _spellInfo, SpellSchoolMask _schoolMask, DamageEffectType _damageType, uint32 cleanDamage = 0);
    // 合并包装构造函数
    explicit DamageInfo(CalcDamageInfo const& dmgInfo);
    // 带伤害索引的构造函数
    DamageInfo(CalcDamageInfo const& dmgInfo, uint8 damageIndex);
    // 根据法术非近战伤害构造函数
    DamageInfo(SpellNonMeleeDamage const& spellNonMeleeDamage, DamageEffectType damageType);

    // 修改伤害值
    void ModifyDamage(int32 amount);
    // 吸收伤害
    void AbsorbDamage(uint32 amount);
    // 抵抗伤害
    void ResistDamage(uint32 amount);
    // 格挡伤害
    void BlockDamage(uint32 amount);

    // 获取攻击者指针
    [[nodiscard]] Unit* GetAttacker() const { return m_attacker; };
    // 获取受害者指针
    [[nodiscard]] Unit* GetVictim() const { return m_victim; };
    // 获取法术信息指针
    [[nodiscard]] SpellInfo const* GetSpellInfo() const { return m_spellInfo; };
    // 获取法术学校掩码
    [[nodiscard]] SpellSchoolMask GetSchoolMask() const { return m_schoolMask; };
    // 获取伤害效果类型
    [[nodiscard]] DamageEffectType GetDamageType() const { return m_damageType; };
    // 获取武器攻击类型
    [[nodiscard]] WeaponAttackType GetAttackType() const { return m_attackType; };
    // 获取伤害值
    [[nodiscard]] uint32 GetDamage() const { return m_damage; };
    // 获取吸收的伤害值
    [[nodiscard]] uint32 GetAbsorb() const { return m_absorb; };
    // 获取抵抗的伤害值
    [[nodiscard]] uint32 GetResist() const { return m_resist; };
    // 获取格挡的伤害值
    [[nodiscard]] uint32 GetBlock() const { return m_block; };

    // 获取未减免的伤害值
    [[nodiscard]] uint32 GetUnmitigatedDamage() const;
};

// 定义治疗信息类，用于存储治疗相关的各种信息
class HealInfo
{
private:
    Unit* const m_healer;           // 治疗者指针
    Unit* const m_target;           // 治疗目标指针
    uint32 m_heal;                  // 治疗值
    uint32 m_effectiveHeal;         // 有效治疗值
    uint32 m_absorb;                // 吸收的治疗值
    SpellInfo const* const m_spellInfo; // 法术信息指针
    SpellSchoolMask const m_schoolMask; // 法术学校掩码
public:
    // 构造函数，初始化治疗信息
    explicit HealInfo(Unit* _healer, Unit* _target, uint32 _heal, SpellInfo const* _spellInfo, SpellSchoolMask _schoolMask)
        : m_healer(_healer), m_target(_target), m_heal(_heal), m_spellInfo(_spellInfo), m_schoolMask(_schoolMask)
    {
        m_absorb = 0;
        m_effectiveHeal = 0;
    }

    // 吸收治疗量
    void AbsorbHeal(uint32 amount)
    {
        amount = std::min(amount, GetHeal());
        m_absorb += amount;
        m_heal -= amount;

        amount = std::min(amount, GetEffectiveHeal());
        m_effectiveHeal -= amount;
    }

    // 设置治疗值
    void SetHeal(uint32 amount)
    {
        m_heal = amount;
    }

    // 设置有效治疗值
    void SetEffectiveHeal(uint32 amount)
    {
        m_effectiveHeal = amount;
    }

    // 获取治疗者指针
    [[nodiscard]] Unit* GetHealer() const { return m_healer; }
    // 获取治疗目标指针
    [[nodiscard]] Unit* GetTarget() const { return m_target; }
    // 获取治疗值
    [[nodiscard]] uint32 GetHeal() const { return m_heal; }
    // 获取有效治疗值
    [[nodiscard]] uint32 GetEffectiveHeal() const { return m_effectiveHeal; }
    // 获取吸收的治疗值
    [[nodiscard]] uint32 GetAbsorb() const { return m_absorb; }
    // 获取法术信息指针
    [[nodiscard]] SpellInfo const* GetSpellInfo() const { return m_spellInfo; };
    // 获取法术学校掩码
    [[nodiscard]] SpellSchoolMask GetSchoolMask() const { return m_schoolMask; };
};

// 定义触发事件信息类，用于存储触发事件的相关信息
class ProcEventInfo
{
private:
    Unit* const _actor;             // 触发者指针
    Unit* const _actionTarget;      // 动作目标指针
    Unit* const _procTarget;        // 触发目标指针
    uint32 _typeMask;               // 类型掩码
    uint32 _spellTypeMask;          // 法术类型掩码
    uint32 _spellPhaseMask;         // 法术阶段掩码
    uint32 _hitMask;                // 命中掩码
    uint32 _cooldown;               // 冷却时间
    Spell const* _spell;            // 法术指针
    DamageInfo* _damageInfo;        // 伤害信息指针
    HealInfo* _healInfo;            // 治疗信息指针
    SpellInfo const* const _triggeredByAuraSpell; // 由光环触发的法术信息指针
    int8 _procAuraEffectIndex;      // 触发光环效果索引
    std::optional<float> _chance;   // 触发几率

public:
    // 构造函数，初始化触发事件信息
    explicit ProcEventInfo(Unit* actor, Unit* actionTarget, Unit* procTarget, uint32 typeMask, uint32 spellTypeMask, uint32 spellPhaseMask, uint32 hitMask, Spell const* spell, DamageInfo* damageInfo, HealInfo* healInfo, SpellInfo const* triggeredByAuraSpell = nullptr, int8 procAuraEffectIndex = -1);
    // 获取触发者指针
    Unit* GetActor() { return _actor; };
    // 获取动作目标指针
    [[nodiscard]] Unit* GetActionTarget() const { return _actionTarget; }
    // 获取触发目标指针
    [[nodiscard]] Unit* GetProcTarget() const { return _procTarget; }
    // 获取类型掩码
    [[nodiscard]] uint32 GetTypeMask() const { return _typeMask; }
    // 获取法术类型掩码
    [[nodiscard]] uint32 GetSpellTypeMask() const { return _spellTypeMask; }
    // 获取法术阶段掩码
    [[nodiscard]] uint32 GetSpellPhaseMask() const { return _spellPhaseMask; }
    // 获取命中掩码
    [[nodiscard]] uint32 GetHitMask() const { return _hitMask; }
    // 获取法术信息指针
    [[nodiscard]] SpellInfo const* GetSpellInfo() const;
    // 获取法术学校掩码
    [[nodiscard]] SpellSchoolMask GetSchoolMask() const { return SPELL_SCHOOL_MASK_NONE; }
    // 获取触发法术指针
    [[nodiscard]] Spell const* GetProcSpell() const { return _spell; }
    // 获取伤害信息指针
    [[nodiscard]] DamageInfo* GetDamageInfo() const { return _damageInfo; }
    // 获取治疗信息指针
    [[nodiscard]] HealInfo* GetHealInfo() const { return _healInfo; }
    // 获取触发光环法术信息指针
    [[nodiscard]] SpellInfo const* GetTriggerAuraSpell() const { return _triggeredByAuraSpell; }
    // 获取触发光环效果索引
    [[nodiscard]] int8 GetTriggerAuraEffectIndex() const { return _procAuraEffectIndex; }
    // 获取触发冷却时间
    [[nodiscard]] uint32 GetProcCooldown() const { return _cooldown; }
    // 设置触发冷却时间
    void SetProcCooldown(uint32 cooldown) { _cooldown = cooldown; }
    // 获取触发几率
    [[nodiscard]] std::optional<float> GetProcChance() const { return _chance; }
    // 设置触发几率
    void SetProcChance(float chance) { _chance = chance; }
    // 重置触发几率
    void ResetProcChance() { _chance.reset(); }
};

// 用于 Unit::CalculateMeleeDamage 的结构体，需要创建类似 SMSG_ATTACKERSTATEUPDATE 操作码的结构
struct CalcDamageInfo
{
    Unit* attacker;             // 攻击者
    Unit* target;               // 伤害目标

    struct
    {
        uint32 damageSchoolMask; // 伤害法术学校掩码
        uint32 damage;           // 伤害值
        uint32 absorb;           // 吸收的伤害值
        uint32 resist;           // 抵抗的伤害值
    } damages[MAX_ITEM_PROTO_DAMAGES];

    uint32 blocked_amount;       // 格挡的伤害量
    uint32 HitInfo;              // 命中信息
    uint32 TargetState;          // 目标状态
    // 辅助信息
    WeaponAttackType attackType; // 武器攻击类型
    uint32 procAttacker;         // 攻击者触发标志
    uint32 procVictim;           // 受害者触发标志
    uint32 procEx;               // 额外触发标志
    uint32 cleanDamage;          // 仅用于怒气计算
    MeleeHitOutcome hitOutCome;  /// @todo: 移除该字段（需要使用 TargetState）
};

// 基于 SMSG_SPELLNONMELEEDAMAGELOG 操作码发送结构的法术非近战伤害信息结构体
struct SpellNonMeleeDamage
{
    // 构造函数，初始化法术非近战伤害信息
    SpellNonMeleeDamage(Unit* _attacker, Unit* _target, SpellInfo const* _spellInfo, uint32 _schoolMask)
        : target(_target), attacker(_attacker), spellInfo(_spellInfo), damage(0), overkill(0), schoolMask(_schoolMask),
        absorb(0), resist(0), physicalLog(false), unused(false), blocked(0), HitInfo(0), cleanDamage(0)
    {
    }

    Unit* target;               // 目标
    Unit* attacker;             // 攻击者
    SpellInfo const* spellInfo; // 法术信息
    uint32 damage;              // 伤害值
    uint32 overkill;            // 过量伤害（击杀时超出目标生命值的伤害）
    uint32 schoolMask;          // 法术学校掩码
    uint32 absorb;              // 吸收的伤害值
    uint32 resist;              // 抵抗的伤害值
    bool physicalLog;           // 是否为物理伤害日志
    bool unused;                // 未使用标志
    uint32 blocked;             // 格挡的伤害值
    uint32 HitInfo;             // 命中信息
    // 辅助信息
    uint32 cleanDamage;         // 纯净伤害值
};

// 定义法术周期性光环日志信息结构体，用于存储法术周期性光环的相关日志信息
struct SpellPeriodicAuraLogInfo
{
    // 构造函数，初始化法术周期性光环日志信息
    SpellPeriodicAuraLogInfo(AuraEffect const* _auraEff, uint32 _damage, uint32 _overDamage, uint32 _absorb, uint32 _resist, float _multiplier, bool _critical)
        : auraEff(_auraEff), damage(_damage), overDamage(_overDamage), absorb(_absorb), resist(_resist), multiplier(_multiplier), critical(_critical) {
    }

    AuraEffect const* auraEff;  // 光环效果指针
    uint32 damage;              // 伤害值
    uint32 overDamage;          // 过量伤害（击杀或过量治疗）
    uint32 absorb;              // 吸收的伤害值
    uint32 resist;              // 抵抗的伤害值
    float  multiplier;          // 伤害乘数
    bool   critical;            // 是否为暴击
};

// 创建触发标志，根据法术信息、攻击类型和正负性设置攻击者和受害者的触发标志
void createProcFlags(SpellInfo const* spellInfo, WeaponAttackType attackType, bool positive, uint32& procAttacker, uint32& procVictim);
// 创建扩展触发掩码，根据法术非近战伤害信息和未命中条件返回扩展触发掩码
uint32 createProcExtendMask(SpellNonMeleeDamage* damageInfo, SpellMissInfo missCondition);

// 定义最大名称词形变化数量常量
#define MAX_DECLINED_NAME_CASES 5

// 定义名称词形变化结构体，用于存储不同词形变化的名称
struct DeclinedName
{
    std::string name[MAX_DECLINED_NAME_CASES]; // 存储不同词形变化的名称
};

// 定义当前法术类型枚举，用于指定当前法术的类型
enum CurrentSpellTypes
{
    CURRENT_MELEE_SPELL = 0, // 当前近战法术
    CURRENT_GENERIC_SPELL = 1, // 当前通用法术
    CURRENT_CHANNELED_SPELL = 2, // 当前引导法术
    CURRENT_AUTOREPEAT_SPELL = 3  // 当前自动重复法术
};

// 定义第一个非近战法术常量
#define CURRENT_FIRST_NON_MELEE_SPELL 1
// 定义最大法术类型常量
#define CURRENT_MAX_SPELL             4

// 定义反应状态枚举类，底层类型为 uint8，用于指定单位的反应状态
enum ReactStates : uint8
{
    REACT_PASSIVE = 0, // 被动状态
    REACT_DEFENSIVE = 1, // 防御状态
    REACT_AGGRESSIVE = 2  // 主动攻击状态
};

// 定义命令状态枚举类，底层类型为 uint8，用于指定单位的命令状态
enum CommandStates : uint8
{
    COMMAND_STAY = 0, // 停留命令
    COMMAND_FOLLOW = 1, // 跟随命令
    COMMAND_ATTACK = 2, // 攻击命令
    COMMAND_ABANDON = 3  // 放弃命令
};

// 定义搜索方法枚举类，用于指定搜索时的匹配方式
enum class SearchMethod
{
    MatchAll, // 匹配所有条件
    MatchAny  // 匹配任意条件
};

// 定义共享视野列表类型，用于存储一组 Player 指针
typedef std::list<Player*> SharedVisionList;

// 定义攻击位置结构体，用于存储攻击位置信息
struct AttackPosition {
    // 构造函数，初始化攻击位置和占用状态
    AttackPosition(Position pos) : _pos(std::move(pos)), _taken(false) {}
    // 重载 == 运算符，用于与整数比较
    bool operator==(const int val)
    {
        return !val;
    };
    // 重载 = 运算符，用于赋值操作
    int operator=(const int val)
    {
        if (!val)
        {
            // _pos = nullptr;
            _taken = false;
            return 0; // 空指针
        }
        return 0; // 空指针
    };
    Position _pos;  // 攻击位置
    bool _taken;    // 是否被占用
};

// 定义清除特殊攻击的计时器起始值常量
#define REACTIVE_TIMER_START 5000

// 定义反应类型枚举，用于指定不同的反应攻击类型
enum ReactiveType
{
    REACTIVE_DEFENSE = 0, // 防御性反应攻击
    REACTIVE_HUNTER_PARRY = 1, // 猎人招架反应攻击
    REACTIVE_OVERPOWER = 2, // 压制反应攻击
    REACTIVE_WOLVERINE_BITE = 3, // 狼獾撕咬反应攻击

    MAX_REACTIVE            // 最大反应类型标记
};

/// 定义法术冷却时间标志枚举，用于指定 SMSG_SPELL_COOLDOWN 操作码中发送的冷却时间标志
enum SpellCooldownFlags
{
    SPELL_COOLDOWN_FLAG_NONE = 0x0,  // 无标志
    SPELL_COOLDOWN_FLAG_INCLUDE_GCD = 0x1,  ///< 除了数据包中指定的正常冷却时间外，还启动全局冷却时间
    SPELL_COOLDOWN_FLAG_INCLUDE_EVENT_COOLDOWNS = 0x2   ///< 为应该在事件上启动冷却时间的法术启动全局冷却时间，需要设置 SPELL_COOLDOWN_FLAG_INCLUDE_GCD
};

// 定义数据包冷却时间映射类型，用于存储法术 ID 和冷却时间的映射关系
typedef std::unordered_map<uint32, uint32> PacketCooldowns;

// 定义下一次攻击的延迟时间常量，用于防止客户端攻击动画出现问题
#define ATTACK_DISPLAY_DELAY 200
// 定义玩家最大潜行检测范围常量
#define MAX_PLAYER_STEALTH_DETECT_RANGE 30.0f               // 玩家检测目标的最大距离

// 前向声明法术触发事件条目结构体，仅内部使用
struct SpellProcEventEntry;

class Unit : public WorldObject
{
public:
    typedef std::unordered_set<Unit*> AttackerSet; // 攻击者集合
    typedef std::set<Unit*> ControlSet; // 控制单位集合

    typedef std::multimap<uint32, Aura*> AuraMap; // 按SpellID存储的Aura
    typedef std::pair<AuraMap::const_iterator, AuraMap::const_iterator> AuraMapBounds;
    typedef std::pair<AuraMap::iterator, AuraMap::iterator> AuraMapBoundsNonConst;

    typedef std::multimap<uint32, AuraApplication*> AuraApplicationMap; // 按SpellID存储的AuraApplication
    typedef std::pair<AuraApplicationMap::const_iterator, AuraApplicationMap::const_iterator> AuraApplicationMapBounds;
    typedef std::pair<AuraApplicationMap::iterator, AuraApplicationMap::iterator> AuraApplicationMapBoundsNonConst;

    typedef std::multimap<AuraStateType, AuraApplication*> AuraStateAurasMap; // 按AuraState存储的AuraApplication
    typedef std::pair<AuraStateAurasMap::const_iterator, AuraStateAurasMap::const_iterator> AuraStateAurasMapBounds;

    typedef std::list<AuraEffect*> AuraEffectList; // AuraEffect列表
    typedef std::list<Aura*> AuraList; // Aura列表
    typedef std::list<AuraApplication*> AuraApplicationList; // AuraApplication列表
    typedef std::list<DiminishingReturn> Diminishing; // 递减返回列表
    typedef GuidUnorderedSet ComboPointHolderSet; // 组合点持有者集合

    typedef std::map<uint8, AuraApplication*> VisibleAuraMap; // 可见Aura映射

    ~Unit() override;

    void Update(uint32 time) override; // 更新单位状态

    UnitAI* GetAI() { return i_AI; } // 获取AI
    void SetAI(UnitAI* newAI) { i_AI = newAI; } // 设置AI

    void AddToWorld() override; // 添加到世界
    void RemoveFromWorld() override; // 从世界中移除

    void CleanupBeforeRemoveFromMap(bool finalCleanup); // 在从地图移除前清理
    void CleanupsBeforeDelete(bool finalCleanup = true) override; // 删除前的清理

    [[nodiscard]] virtual bool isBeingLoaded() const { return false; } // 是否正在加载
    [[nodiscard]] bool IsDuringRemoveFromWorld() const { return m_duringRemoveFromWorld; } // 是否在移除过程中

    /*********************************************************/
    /***                    UNIT HELPERS                   ***/
    /*********************************************************/
    void SetUInt32Value(uint16 index, uint32 value); // 设置32位整数值

    [[nodiscard]] Unit* GetOwner() const; // 获取所有者

    // GUID相关方法
    [[nodiscard]] ObjectGuid GetOwnerGUID() const { return GetGuidValue(UNIT_FIELD_SUMMONEDBY); } // 获取召唤者GUID
    void SetOwnerGUID(ObjectGuid owner); // 设置召唤者GUID
    [[nodiscard]] ObjectGuid GetCreatorGUID() const { return GetGuidValue(UNIT_FIELD_CREATEDBY); } // 获取创建者GUID
    void SetCreatorGUID(ObjectGuid creator) { SetGuidValue(UNIT_FIELD_CREATEDBY, creator); } // 设置创建者GUID
    [[nodiscard]] ObjectGuid GetMinionGUID() const { return GetGuidValue(UNIT_FIELD_SUMMON); } // 获取随从GUID
    void SetMinionGUID(ObjectGuid guid) { SetGuidValue(UNIT_FIELD_SUMMON, guid); } // 设置随从GUID
    [[nodiscard]] ObjectGuid GetCharmerGUID() const { return GetGuidValue(UNIT_FIELD_CHARMEDBY); } // 获取控制者GUID
    void SetCharmerGUID(ObjectGuid owner) { SetGuidValue(UNIT_FIELD_CHARMEDBY, owner); } // 设置控制者GUID
    [[nodiscard]] ObjectGuid GetCharmGUID() const { return  GetGuidValue(UNIT_FIELD_CHARM); } // 获取被控制者GUID
    void SetPetGUID(ObjectGuid guid) { m_SummonSlot[SUMMON_SLOT_PET] = guid; } // 设置宠物GUID
    [[nodiscard]] ObjectGuid GetPetGUID() const { return m_SummonSlot[SUMMON_SLOT_PET]; } // 获取宠物GUID
    void SetCritterGUID(ObjectGuid guid) { SetGuidValue(UNIT_FIELD_CRITTER, guid); } // 设置小动物GUID
    [[nodiscard]] ObjectGuid GetCritterGUID() const { return GetGuidValue(UNIT_FIELD_CRITTER); } // 获取小动物GUID
    [[nodiscard]] ObjectGuid GetTransGUID() const override; // 获取变身GUID

    /// @todo: move this in Object ckass as others casting pointers
    Pet* ToPet() { if (IsPet()) return reinterpret_cast<Pet*>(this); else return nullptr; } // 转换为宠物
    Totem* ToTotem() { if (IsTotem()) return reinterpret_cast<Totem*>(this); else return nullptr; } // 转换为图腾
    TempSummon* ToTempSummon() { if (IsSummon()) return reinterpret_cast<TempSummon*>(this); else return nullptr; } // 转换为临时召唤
    [[nodiscard]] const TempSummon* ToTempSummon() const { if (IsSummon()) return reinterpret_cast<const TempSummon*>(this); else return nullptr; } // 转换为临时召唤

    // 单位状态
    void AddUnitState(uint32 f) { m_state |= f; } // 添加单位状态
    [[nodiscard]] bool HasUnitState(const uint32 f) const { return (m_state & f); } // 是否具有指定状态
    void ClearUnitState(uint32 f) { m_state &= ~f; } // 清除单位状态
    [[nodiscard]] uint32 GetUnitState() const { return m_state; } // 获取单位状态

    // 单位类型掩码
    [[nodiscard]] uint32 HasUnitTypeMask(uint32 mask) const { return mask & m_unitTypeMask; } // 是否具有指定类型掩码
    void AddUnitTypeMask(uint32 mask) { m_unitTypeMask |= mask; } // 添加类型掩码
    [[nodiscard]] uint32 GetUnitTypeMask() const { return m_unitTypeMask; } // 获取类型掩码

    // 单位标志
    UnitFlags GetUnitFlags() const { return UnitFlags(GetUInt32Value(UNIT_FIELD_FLAGS)); } // 获取单位标志
    bool HasUnitFlag(UnitFlags flags) const { return HasFlag(UNIT_FIELD_FLAGS, flags); } // 是否具有指定标志
    void SetUnitFlag(UnitFlags flags) { SetFlag(UNIT_FIELD_FLAGS, flags); } // 设置单位标志
    void RemoveUnitFlag(UnitFlags flags) { RemoveFlag(UNIT_FIELD_FLAGS, flags); } // 移除单位标志
    void ReplaceAllUnitFlags(UnitFlags flags) { SetUInt32Value(UNIT_FIELD_FLAGS, flags); } // 替换所有单位标志

    UnitFlags2 GetUnitFlags2() const { return UnitFlags2(GetUInt32Value(UNIT_FIELD_FLAGS_2)); } // 获取单位标志2
    bool HasUnitFlag2(UnitFlags2 flags) const { return HasFlag(UNIT_FIELD_FLAGS_2, flags); } // 是否具有指定标志2
    void SetUnitFlag2(UnitFlags2 flags) { SetFlag(UNIT_FIELD_FLAGS_2, flags); } // 设置单位标志2
    void RemoveUnitFlag2(UnitFlags2 flags) { RemoveFlag(UNIT_FIELD_FLAGS_2, flags); } // 移除单位标志2
    void ReplaceAllUnitFlags2(UnitFlags2 flags) { SetUInt32Value(UNIT_FIELD_FLAGS_2, flags); } // 替换所有单位标志2

    void SetEmoteState(Emote emoteState) { SetUInt32Value(UNIT_NPC_EMOTESTATE, emoteState); } // 设置表情状态
    void ClearEmoteState() { SetEmoteState(EMOTE_ONESHOT_NONE); } // 清除表情状态

    // NPC标志
    NPCFlags GetNpcFlags() const { return NPCFlags(GetUInt32Value(UNIT_NPC_FLAGS)); } // 获取NPC标志
    bool HasNpcFlag(NPCFlags flags) const { return HasFlag(UNIT_NPC_FLAGS, flags) != 0; } // 是否具有指定NPC标志
    void SetNpcFlag(NPCFlags flags) { SetFlag(UNIT_NPC_FLAGS, flags); } // 设置NPC标志
    void RemoveNpcFlag(NPCFlags flags) { RemoveFlag(UNIT_NPC_FLAGS, flags); } // 移除NPC标志
    void ReplaceAllNpcFlags(NPCFlags flags) { SetUInt32Value(UNIT_NPC_FLAGS, flags); } // 替换所有NPC标志

    uint32 GetDynamicFlags() const override { return GetUInt32Value(UNIT_DYNAMIC_FLAGS); } // 获取动态标志
    void ReplaceAllDynamicFlags(uint32 flag) override { SetUInt32Value(UNIT_DYNAMIC_FLAGS, flag); } // 替换所有动态标志

    // 移动标志
    void AddUnitMovementFlag(uint32 f) { m_movementInfo.flags |= f; } // 添加移动标志
    void RemoveUnitMovementFlag(uint32 f) { m_movementInfo.flags &= ~f; } // 移除移动标志
    [[nodiscard]] bool HasUnitMovementFlag(uint32 f) const { return (m_movementInfo.flags & f) == f; } // 是否具有指定移动标志
    [[nodiscard]] uint32 GetUnitMovementFlags() const { return m_movementInfo.flags; } // 获取移动标志
    void SetUnitMovementFlags(uint32 f) { m_movementInfo.flags = f; } // 设置移动标志

    void AddExtraUnitMovementFlag(uint16 f) { m_movementInfo.flags2 |= f; } // 添加额外移动标志
    void RemoveExtraUnitMovementFlag(uint16 f) { m_movementInfo.flags2 &= ~f; } // 移除额外移动标志
    [[nodiscard]] uint16 HasExtraUnitMovementFlag(uint16 f) const { return m_movementInfo.flags2 & f; } // 是否具有指定额外移动标志
    [[nodiscard]] uint16 GetExtraUnitMovementFlags() const { return m_movementInfo.flags2; } // 获取额外移动标志
    void SetExtraUnitMovementFlags(uint16 f) { m_movementInfo.flags2 = f; } // 设置额外移动标志

    /*********************************************************/
    /***           UNIT TYPES, CLASSES, RACES...           ***/
    /*********************************************************/

    // 单位类型方法
    [[nodiscard]] bool IsSummon() const { return m_unitTypeMask & UNIT_MASK_SUMMON; } // 是否为召唤物
    [[nodiscard]] bool IsGuardian() const { return m_unitTypeMask & UNIT_MASK_GUARDIAN; } // 是否为守护者
    [[nodiscard]] bool IsControllableGuardian() const { return m_unitTypeMask & UNIT_MASK_CONTROLLABLE_GUARDIAN; } // 是否为可控守护者
    [[nodiscard]] bool IsPet() const { return m_unitTypeMask & UNIT_MASK_PET; } // 是否为宠物
    [[nodiscard]] bool IsHunterPet() const { return m_unitTypeMask & UNIT_MASK_HUNTER_PET; } // 是否为猎人宠物
    [[nodiscard]] bool IsTotem() const { return m_unitTypeMask & UNIT_MASK_TOTEM; } // 是否为图腾
    [[nodiscard]] bool IsVehicle() const { return m_unitTypeMask & UNIT_MASK_VEHICLE; } // 是否为载具

    // NPC类型方法
    [[nodiscard]] bool IsVendor()       const { return HasNpcFlag(UNIT_NPC_FLAG_VENDOR); } // 是否为商人
    [[nodiscard]] bool IsTrainer()      const { return HasNpcFlag(UNIT_NPC_FLAG_TRAINER); } // 是否为训练师
    [[nodiscard]] bool IsQuestGiver()   const { return HasNpcFlag(UNIT_NPC_FLAG_QUESTGIVER); } // 是否为任务给予者
    [[nodiscard]] bool IsGossip()       const { return HasNpcFlag(UNIT_NPC_FLAG_GOSSIP); } // 是否有闲聊
    [[nodiscard]] bool IsTaxi()         const { return HasNpcFlag(UNIT_NPC_FLAG_FLIGHTMASTER); } // 是否为飞行管理员
    [[nodiscard]] bool IsGuildMaster()  const { return HasNpcFlag(UNIT_NPC_FLAG_PETITIONER); } // 是否为公会大师
    [[nodiscard]] bool IsBattleMaster() const { return HasNpcFlag(UNIT_NPC_FLAG_BATTLEMASTER); } // 是否为战场大师
    [[nodiscard]] bool IsBanker()       const { return HasNpcFlag(UNIT_NPC_FLAG_BANKER); } // 是否为银行家
    [[nodiscard]] bool IsInnkeeper()    const { return HasNpcFlag(UNIT_NPC_FLAG_INNKEEPER); } // 是否为旅馆老板
    [[nodiscard]] bool IsSpiritHealer() const { return HasNpcFlag(UNIT_NPC_FLAG_SPIRITHEALER); } // 是否为灵魂治疗者
    [[nodiscard]] bool IsSpiritGuide()  const { return HasNpcFlag(UNIT_NPC_FLAG_SPIRITGUIDE); } // 是否为灵魂向导
    [[nodiscard]] bool IsTabardDesigner() const { return HasNpcFlag(UNIT_NPC_FLAG_TABARDDESIGNER); } // 是否为纹章设计师
    [[nodiscard]] bool IsAuctioner()    const { return HasNpcFlag(UNIT_NPC_FLAG_AUCTIONEER); } // 是否为拍卖师
    [[nodiscard]] bool IsArmorer()      const { return HasNpcFlag(UNIT_NPC_FLAG_REPAIR); } // 是否为修理商
    [[nodiscard]] bool IsServiceProvider() const
    {
        return HasNpcFlag(UNIT_NPC_FLAG_VENDOR | UNIT_NPC_FLAG_TRAINER | UNIT_NPC_FLAG_FLIGHTMASTER |
            UNIT_NPC_FLAG_PETITIONER | UNIT_NPC_FLAG_BATTLEMASTER | UNIT_NPC_FLAG_BANKER |
            UNIT_NPC_FLAG_INNKEEPER | UNIT_NPC_FLAG_SPIRITHEALER |
            UNIT_NPC_FLAG_SPIRITGUIDE | UNIT_NPC_FLAG_TABARDDESIGNER | UNIT_NPC_FLAG_AUCTIONEER);
    } // 是否为服务提供者
    [[nodiscard]] bool IsSpiritService() const { return HasNpcFlag(UNIT_NPC_FLAG_SPIRITHEALER | UNIT_NPC_FLAG_SPIRITGUIDE); } // 是否为灵魂服务

    // 生物类型
    [[nodiscard]] uint32 GetCreatureType() const; // 获取生物类型
    [[nodiscard]] uint32 GetCreatureTypeMask() const
    {
        uint32 creatureType = GetCreatureType();
        return (creatureType >= 1) ? (1 << (creatureType - 1)) : 0;
    } // 获取生物类型掩码
    [[nodiscard]] bool IsCritter() const { return GetCreatureType() == CREATURE_TYPE_CRITTER; } // 是否为小动物

    // 种族方法
    [[nodiscard]] uint8 getRace(bool original = false) const; // 获取种族
    void setRace(uint8 race); // 设置种族
    [[nodiscard]] uint32 getRaceMask() const { return 1 << (getRace(true) - 1); } // 获取种族掩码
    [[nodiscard]] DisplayRace GetDisplayRaceFromModelId(uint32 modelId) const; // 根据模型ID获取显示种族
    [[nodiscard]] DisplayRace GetDisplayRace() const { return GetDisplayRaceFromModelId(GetDisplayId()); }; // 获取显示种族

    // 职业方法
    [[nodiscard]] uint8 getClass() const { return GetByteValue(UNIT_FIELD_BYTES_0, 1); } // 获取职业
    [[nodiscard]] virtual bool IsClass(Classes unitClass, [[maybe_unused]] ClassContext context = CLASS_CONTEXT_NONE) const { return (getClass() == unitClass); } // 是否为指定职业
    [[nodiscard]] uint32 getClassMask() const { return 1 << (getClass() - 1); } // 获取职业掩码

    // 性别方法
    [[nodiscard]] uint8 getGender() const { return GetByteValue(UNIT_FIELD_BYTES_0, 2); } // 获取性别

    // 阵营方法
    [[nodiscard]] uint32 GetFaction() const { return GetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE); } // 获取阵营
    [[nodiscard]] FactionTemplateEntry const* GetFactionTemplateEntry() const; // 获取阵营模板
    void SetFaction(uint32 faction); // 设置阵营
    void RestoreFaction(); // 恢复阵营
    [[nodiscard]] uint32 GetOldFactionId() const { return _oldFactionId; } // 获取旧阵营ID

    /*********************************************************/
    /***            METHODS RELATED TO COMBATS             ***/
    /*********************************************************/
    // 目标
    [[nodiscard]] ObjectGuid GetTarget() const { return GetGuidValue(UNIT_FIELD_TARGET); } // 获取目标GUID
    virtual void SetTarget(ObjectGuid /*guid*/ = ObjectGuid::Empty) = 0; // 设置目标

    bool isTargetableForAttack(bool checkFakeDeath = true, Unit const* byWho = nullptr) const; // 是否可被攻击
    bool IsValidAttackTarget(Unit const* target, SpellInfo const* bySpell = nullptr) const; // 是否为有效攻击目标
    bool _IsValidAttackTarget(Unit const* target, SpellInfo const* bySpell, WorldObject const* obj = nullptr) const; // 内部验证攻击目标
    bool IsValidAssistTarget(Unit const* target) const; // 是否为有效协助目标
    bool _IsValidAssistTarget(Unit const* target, SpellInfo const* bySpell) const; // 内部验证协助目标

    // 战斗范围
    [[nodiscard]] float GetCombatReach() const override { return m_floatValues[UNIT_FIELD_COMBATREACH]; } // 获取战斗范围
    [[nodiscard]] float GetMeleeReach() const { float reach = m_floatValues[UNIT_FIELD_COMBATREACH]; return reach > MIN_MELEE_REACH ? reach : MIN_MELEE_REACH; } // 获取近战范围
    [[nodiscard]] bool IsWithinRange(Unit const* obj, float dist) const; // 是否在指定距离内
    bool IsWithinCombatRange(Unit const* obj, float dist2compare) const; // 是否在战斗范围内
    bool IsWithinMeleeRange(Unit const* obj, float dist = 0.f) const; // 是否在近战范围内
    float GetMeleeRange(Unit const* target) const; // 获取近战范围

    void setAttackTimer(WeaponAttackType type, int32 time) { m_attackTimer[type] = time; } // 设置攻击计时器
    void resetAttackTimer(WeaponAttackType type = BASE_ATTACK); // 重置攻击计时器
    [[nodiscard]] int32 getAttackTimer(WeaponAttackType type) const { return m_attackTimer[type]; } // 获取攻击计时器
    [[nodiscard]] bool isAttackReady(WeaponAttackType type = BASE_ATTACK) const { return m_attackTimer[type] <= 0; } // 是否准备好攻击

    virtual SpellSchoolMask GetMeleeDamageSchoolMask(WeaponAttackType attackType = BASE_ATTACK, uint8 damageIndex = 0) const = 0; // 获取近战伤害学派掩码
    bool GetRandomContactPoint(Unit const* target, float& x, float& y, float& z, bool force = false) const; // 获取随机接触点

    [[nodiscard]] Unit* getAttackerForHelper() const; // 获取攻击者用于帮助

    bool Attack(Unit* victim, bool meleeAttack); // 攻击目标

    void CastStop(uint32 except_spellid = 0, bool withInstant = true); // 停止施法
    bool AttackStop(); // 停止攻击
    void RemoveAllAttackers(); // 移除所有攻击者
    [[nodiscard]] AttackerSet const& getAttackers() const { return m_attackers; } // 获取攻击者集合
    [[nodiscard]] bool GetMeleeAttackPoint(Unit* attacker, Position& pos); // 获取近战攻击点
    [[nodiscard]] bool isAttackingPlayer() const; // 是否在攻击玩家
    [[nodiscard]] Unit* GetVictim() const { return m_attacking; } // 获取受害者

    void CombatStop(bool includingCast = false); // 停止战斗
    void CombatStopWithPets(bool includingCast = false); // 停止战斗包括宠物
    void StopAttackFaction(uint32 faction_id); // 停止攻击指定阵营
    void StopAttackingInvalidTarget(); // 停止攻击无效目标
    Unit* SelectNearbyTarget(Unit* exclude = nullptr, float dist = NOMINAL_MELEE_RANGE) const; // 选择附近目标
    Unit* SelectNearbyNoTotemTarget(Unit* exclude = nullptr, float dist = NOMINAL_MELEE_RANGE) const; // 选择附近非图腾目标
    void SendMeleeAttackStop(Unit* victim = nullptr); // 发送近战攻击停止
    void SendMeleeAttackStart(Unit* victim, Player* sendTo = nullptr); // 发送近战攻击开始

    [[nodiscard]] uint32 GetAttackTime(WeaponAttackType att) const
    {
        float f_BaseAttackTime = GetFloatValue(static_cast<uint16>(UNIT_FIELD_BASEATTACKTIME) + att) / m_modAttackSpeedPct[att];
        return (uint32)f_BaseAttackTime;
    } // 获取攻击时间

    void SetAttackTime(WeaponAttackType att, uint32 val) { SetFloatValue(static_cast<uint16>(UNIT_FIELD_BASEATTACKTIME) + att, val * m_modAttackSpeedPct[att]); } // 设置攻击时间
    void ApplyAttackTimePercentMod(WeaponAttackType att, float val, bool apply); // 应用攻击时间百分比修正
    void ApplyCastTimePercentMod(float val, bool apply); // 应用施法时间百分比修正

    void SetImmuneToAll(bool apply, bool keepCombat = false) { SetImmuneToPC(apply, keepCombat); SetImmuneToNPC(apply, keepCombat); } // 设置对所有免疫
    bool IsImmuneToAll() const { return IsImmuneToPC() && IsImmuneToNPC(); } // 是否对所有免疫
    void SetImmuneToPC(bool apply, bool keepCombat = false); // 设置对玩家免疫
    bool IsImmuneToPC() const { return HasUnitFlag(UNIT_FLAG_IMMUNE_TO_PC); } // 是否对玩家免疫
    void SetImmuneToNPC(bool apply, bool keepCombat = false); // 设置对NPC免疫
    bool IsImmuneToNPC() const { return HasUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC); } // 是否对NPC免疫

    bool IsEngaged() const { return IsInCombat(); } // 是否参与战斗
    bool IsEngagedBy(Unit const* who) const { return IsInCombatWith(who); } // 是否被指定单位攻击

    [[nodiscard]] bool IsInCombat() const { return HasUnitFlag(UNIT_FLAG_IN_COMBAT); } // 是否在战斗中
    bool IsInCombatWith(Unit const* who) const; // 是否与指定单位战斗

    [[nodiscard]] bool IsPetInCombat() const { return HasUnitFlag(UNIT_FLAG_PET_IN_COMBAT); } // 宠物是否在战斗中
    void CombatStart(Unit* target, bool initialAggro = true); // 开始战斗
    void CombatStartOnCast(Unit* target, bool initialAggro = true, uint32 duration = 0); // 施法时开始战斗
    void SetInCombatState(bool PvP, Unit* enemy = nullptr, uint32 duration = 0); // 设置战斗状态
    void SetInCombatWith(Unit* enemy, uint32 duration = 0); // 设置与指定单位战斗
    void ClearInCombat(); // 清除战斗状态
    void ClearInPetCombat(); // 清除宠物战斗状态
    [[nodiscard]] uint32 GetCombatTimer() const { return m_CombatTimer; } // 获取战斗计时器
    void SetCombatTimer(uint32 timer) { m_CombatTimer = timer; } // 设置战斗计时器

    // 威胁相关方法
    [[nodiscard]] bool CanHaveThreatList(bool skipAliveCheck = false) const; // 是否可以有威胁列表
    void AddThreat(Unit* victim, float fThreat, SpellSchoolMask schoolMask = SPELL_SCHOOL_MASK_NORMAL, SpellInfo const* threatSpell = nullptr); // 添加威胁
    float ApplyTotalThreatModifier(float fThreat, SpellSchoolMask schoolMask = SPELL_SCHOOL_MASK_NORMAL); // 应用总威胁修正
    void TauntApply(Unit* victim); // 应用嘲讽
    void TauntFadeOut(Unit* taunter); // 嘲讽消退
    ThreatMgr& GetThreatMgr() { return m_ThreatMgr; } // 获取威胁管理器
    ThreatMgr const& GetThreatMgr() const { return m_ThreatMgr; } // 获取威胁管理器
    void addHatedBy(HostileReference* pHostileReference) { m_HostileRefMgr.insertFirst(pHostileReference); }; // 添加仇恨
    void removeHatedBy(HostileReference* /*pHostileReference*/) { /* 无操作 */ }
    HostileRefMgr& getHostileRefMgr() { return m_HostileRefMgr; } // 获取仇恨管理器

    // 重定向威胁
    void SetRedirectThreat(ObjectGuid guid, uint32 pct) { _redirectThreatInfo.Set(guid, pct); } // 设置威胁重定向
    void ResetRedirectThreat() { SetRedirectThreat(ObjectGuid::Empty, 0); } // 重置威胁重定向
    void ModifyRedirectThreat(int32 amount) { _redirectThreatInfo.ModifyThreatPct(amount); } // 修改威胁重定向百分比
    uint32 GetRedirectThreatPercent() { return _redirectThreatInfo.GetThreatPct(); } // 获取威胁重定向百分比
    [[nodiscard]] Unit* GetRedirectThreatTarget() const; // 获取威胁重定向目标

    void SetLastDamagedTargetGuid(ObjectGuid const& guid) { _lastDamagedTargetGuid = guid; } // 设置最后受伤目标GUID
    [[nodiscard]] ObjectGuid const& GetLastDamagedTargetGuid() const { return _lastDamagedTargetGuid; } // 获取最后受伤目标GUID

    void AttackerStateUpdate(Unit* victim, WeaponAttackType attType = BASE_ATTACK, bool extra = false, bool ignoreCasting = false); // 攻击者状态更新

    // 武器系统
    [[nodiscard]] bool haveOffhandWeapon() const; // 是否有副手武器
    [[nodiscard]] bool CanDualWield() const { return m_canDualWield; } // 是否可以双持
    virtual void SetCanDualWield(bool value) { m_canDualWield = value; } // 设置是否可以双持

    virtual bool HasWeapon(WeaponAttackType type) const = 0; // 是否有指定类型武器
    inline bool HasMainhandWeapon() const { return HasWeapon(BASE_ATTACK); } // 是否有主手武器
    inline bool HasOffhandWeapon() const { return HasWeapon(OFF_ATTACK); } // 是否有副手武器
    inline bool HasRangedWeapon() const { return HasWeapon(RANGED_ATTACK); } // 是否有远程武器

    inline bool hasMainhandWeaponForAttack() const { return HasWeaponForAttack(BASE_ATTACK); } // 攻击时是否有主手武器
    virtual bool HasWeaponForAttack(WeaponAttackType type) const { return CanUseAttackType(type); } // 攻击时是否有指定类型武器
    inline bool HasMainhandWeaponForAttack() const { return HasWeaponForAttack(BASE_ATTACK); } // 攻击时是否有主手武器
    inline bool HasOffhandWeaponForAttack() const { return HasWeaponForAttack(OFF_ATTACK); } // 攻击时是否有副手武器
    inline bool HasRangedWeaponForAttack() const { return HasWeaponForAttack(RANGED_ATTACK); } // 攻击时是否有远程武器
    [[nodiscard]] bool CanUseAttackType(uint8 attacktype) const
    {
        switch (attacktype)
        {
        case BASE_ATTACK:
            return !HasUnitFlag(UNIT_FLAG_DISARMED);
        case OFF_ATTACK:
            return !HasUnitFlag2(UNIT_FLAG2_DISARM_OFFHAND);
        case RANGED_ATTACK:
            return !HasUnitFlag2(UNIT_FLAG2_DISARM_RANGED);
        default:
            return true;
        }
    } // 是否可以使用指定攻击类型

    // 额外攻击
    void HandleProcExtraAttackFor(Unit* victim, uint32 count); // 处理额外攻击
    void SetLastExtraAttackSpell(uint32 spellId) { _lastExtraAttackSpell = spellId; } // 设置最后一次额外攻击法术
    [[nodiscard]] uint32 GetLastExtraAttackSpell() const { return _lastExtraAttackSpell; } // 获取最后一次额外攻击法术
    void AddExtraAttacks(uint32 count); // 添加额外攻击

    // 组合点系统
    [[nodiscard]] uint8 GetComboPoints(Unit const* who = nullptr) const { return (who && m_comboTarget != who) ? 0 : m_comboPoints; } // 获取组合点
    [[nodiscard]] uint8 GetComboPoints(ObjectGuid const& guid) const { return (m_comboTarget && m_comboTarget->GetGUID() == guid) ? m_comboPoints : 0; } // 获取组合点
    [[nodiscard]] Unit* GetComboTarget() const { return m_comboTarget; } // 获取组合点目标
    [[nodiscard]] ObjectGuid const GetComboTargetGUID() const { return m_comboTarget ? m_comboTarget->GetGUID() : ObjectGuid::Empty; } // 获取组合点目标GUID

    void AddComboPoints(Unit* target, int8 count); // 添加组合点
    void AddComboPoints(int8 count) { AddComboPoints(nullptr, count); } // 添加组合点
    void ClearComboPoints(); // 清除组合点

    void AddComboPointHolder(Unit* unit) { m_ComboPointHolders.insert(unit); } // 添加组合点持有者
    void RemoveComboPointHolder(Unit* unit) { m_ComboPointHolders.erase(unit); } // 移除组合点持有者
    void ClearComboPointHolders(); // 清除组合点持有者

    // 玩家对战
    void SetContestedPvP(Player* attackedPlayer = nullptr, bool lookForNearContestedGuards = true); // 设置争夺PvP
    [[nodiscard]] bool IsContestedGuard() const
    {
        if (FactionTemplateEntry const* entry = GetFactionTemplateEntry())
            return entry->IsContestedGuardFaction();

        return false;
    } // 是否为争夺守卫
    [[nodiscard]] bool RespondsToCallForHelp() const
    {
        if (FactionTemplateEntry const* entry = GetFactionTemplateEntry())
            return entry->FactionRespondsToCallForHelp();

        return false;
    } // 是否响应求救
    [[nodiscard]] bool IsInSanctuary() const { return HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY); } // 是否在圣地
    [[nodiscard]] bool IsPvP() const { return HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_PVP); } // 是否为PvP
    [[nodiscard]] bool IsFFAPvP() const { return HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP); } // 是否为自由PvP
    void SetPvP(bool state)
    {
        if (state)
            SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_PVP);
        else
            RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_PVP);
    } // 设置PvP状态

    /*********************************************************/
    /***              METHODS RELATED TO STATS             ***/
    /*********************************************************/
    // 属性系统
    [[nodiscard]] float GetStat(Stats stat) const { return float(GetUInt32Value(static_cast<uint16>(UNIT_FIELD_STAT0) + stat)); } // 获取属性
    void SetStat(Stats stat, int32 val) { SetStatInt32Value(static_cast<uint16>(UNIT_FIELD_STAT0) + stat, val); } // 设置属性

    [[nodiscard]] Stats GetStatByAuraGroup(UnitMods unitMod) const; // 获取属性通过光环组

    [[nodiscard]] float GetCreateStat(Stats stat) const { return m_createStats[stat]; } // 获取创建属性
    void SetCreateStat(Stats stat, float val) { m_createStats[stat] = val; } // 设置创建属性

    [[nodiscard]] float GetPosStat(Stats stat) const { return GetFloatValue(static_cast<uint16>(UNIT_FIELD_POSSTAT0) + stat); } // 获取正属性
    [[nodiscard]] float GetNegStat(Stats stat) const { return GetFloatValue(static_cast<uint16>(UNIT_FIELD_NEGSTAT0) + stat); } // 获取负属性

    void InitStatBuffMods()
    {
        for (uint8 i = STAT_STRENGTH; i < MAX_STATS; ++i) SetFloatValue(static_cast<uint16>(UNIT_FIELD_POSSTAT0) + i, 0);
        for (uint8 i = STAT_STRENGTH; i < MAX_STATS; ++i) SetFloatValue(static_cast<uint16>(UNIT_FIELD_NEGSTAT0) + i, 0);
    } // 初始化属性增益

    bool HandleStatModifier(UnitMods unitMod, UnitModifierType modifierType, float amount, bool apply); // 处理属性修正
    void SetModifierValue(UnitMods unitMod, UnitModifierType modifierType, float value) { m_auraModifiersGroup[unitMod][modifierType] = value; } // 设置修正值
    [[nodiscard]] float GetModifierValue(UnitMods unitMod, UnitModifierType modifierType) const; // 获取修正值
    [[nodiscard]] float GetTotalStatValue(Stats stat, float additionalValue = 0.0f) const; // 获取总属性值

    void SetCanModifyStats(bool modifyStats) { m_canModifyStats = modifyStats; } // 设置是否可以修改属性
    [[nodiscard]] bool CanModifyStats() const { return m_canModifyStats; } // 是否可以修改属性

    void ApplyStatBuffMod(Stats stat, float val, bool apply) { ApplyModSignedFloatValue((val > 0 ? static_cast<uint16>(UNIT_FIELD_POSSTAT0) + stat : static_cast<uint16>(UNIT_FIELD_NEGSTAT0) + stat), val, apply); } // 应用属性增益
    void ApplyStatPercentBuffMod(Stats stat, float val, bool apply); // 应用属性百分比增益

    // 单位等级方法
    [[nodiscard]] uint8 GetLevel() const { return uint8(GetUInt32Value(UNIT_FIELD_LEVEL)); } // 获取等级
    uint8 getLevelForTarget(WorldObject const* /*target*/) const override { return GetLevel(); } // 获取目标等级
    void SetLevel(uint8 lvl, bool showLevelChange = true); // 设置等级

    // 生命值方法
    [[nodiscard]] uint32 GetHealth()    const { return GetUInt32Value(UNIT_FIELD_HEALTH); } // 获取生命值
    [[nodiscard]] uint32 GetMaxHealth() const { return GetUInt32Value(UNIT_FIELD_MAXHEALTH); } // 获取最大生命值
    [[nodiscard]] float GetHealthPct() const { return GetMaxHealth() ? 100.f * GetHealth() / GetMaxHealth() : 0.0f; } // 获取生命百分比
    int32 GetHealthGain(int32 dVal); // 获取生命恢复
    [[nodiscard]] uint32 GetCreateHealth() const { return GetUInt32Value(UNIT_FIELD_BASE_HEALTH); } // 获取初始生命值

    [[nodiscard]] bool IsFullHealth() const { return GetHealth() == GetMaxHealth(); } // 是否满生命值

    [[nodiscard]] bool HealthBelowPct(int32 pct) const { return GetHealth() < CountPctFromMaxHealth(pct); } // 生命值是否低于指定百分比
    [[nodiscard]] bool HealthBelowPctDamaged(int32 pct, uint32 damage) const { return int64(GetHealth()) - int64(damage) < int64(CountPctFromMaxHealth(pct)); } // 受伤后生命值是否低于指定百分比
    [[nodiscard]] bool HealthAbovePct(int32 pct) const { return GetHealth() > CountPctFromMaxHealth(pct); } // 生命值是否高于指定百分比
    [[nodiscard]] bool HealthAbovePctHealed(int32 pct, uint32 heal) const { return uint64(GetHealth()) + uint64(heal) > CountPctFromMaxHealth(pct); } // 治疗后生命值是否高于指定百分比

    [[nodiscard]] uint32 CountPctFromMaxHealth(int32 pct) const { return CalculatePct(GetMaxHealth(), pct); } // 计算最大生命值的百分比
    [[nodiscard]] uint32 CountPctFromCurHealth(int32 pct) const { return CalculatePct(GetHealth(), pct); } // 计算当前生命值的百分比

    void SetHealth(uint32 val); // 设置生命值
    void SetMaxHealth(uint32 val); // 设置最大生命值
    inline void SetFullHealth() { SetHealth(GetMaxHealth()); } // 设置满生命值
    int32 ModifyHealth(int32 val); // 修改生命值
    void SetCreateHealth(uint32 val) { SetUInt32Value(UNIT_FIELD_BASE_HEALTH, val); } // 设置初始生命值

    // 法力值方法
    [[nodiscard]] Powers getPowerType() const { return Powers(GetByteValue(UNIT_FIELD_BYTES_0, 3)); } // 获取法力类型
    [[nodiscard]] virtual bool HasActivePowerType(Powers power) { return getPowerType() == power; } // 是否有指定法力类型
    [[nodiscard]] Powers GetPowerTypeByAuraGroup(UnitMods unitMod) const; // 获取光环组对应的法力类型

    [[nodiscard]] uint32 GetPower(Powers power) const { return GetUInt32Value(static_cast<uint16>(UNIT_FIELD_POWER1) + power); } // 获取法力值
    [[nodiscard]] uint32 GetMaxPower(Powers power) const { return GetUInt32Value(static_cast<uint16>(UNIT_FIELD_MAXPOWER1) + power); } // 获取最大法力值
    [[nodiscard]] float GetPowerPct(Powers power) const { return GetMaxPower(power) ? 100.f * GetPower(power) / GetMaxPower(power) : 0.0f; } // 获取法力百分比
    [[nodiscard]] uint32 GetCreatePowers(Powers power) const; // 获取初始法力值

    void setPowerType(Powers power); // 设置法力类型
    void SetPower(Powers power, uint32 val, bool withPowerUpdate = true, bool fromRegenerate = false); // 设置法力值
    void SetMaxPower(Powers power, uint32 val); // 设置最大法力值

    int32 ModifyPower(Powers power, int32 val, bool withPowerUpdate = true); // 修改法力值
    int32 ModifyPowerPct(Powers power, float pct, bool apply = true); // 修改法力百分比

    void RewardRage(uint32 damage, uint32 weaponSpeedHitFactor, bool attacker); // 奖励怒气

    [[nodiscard]] uint32 GetCreateMana() const { return GetUInt32Value(UNIT_FIELD_BASE_MANA); } // 获取初始法力
    void SetCreateMana(uint32 val) { SetUInt32Value(UNIT_FIELD_BASE_MANA, val); } // 设置初始法力
    [[nodiscard]] bool CanRestoreMana(SpellInfo const* spellInfo) const; // 是否可以恢复法力
    void SetLastManaUse(uint32 spellCastTime) { m_lastManaUse = spellCastTime; } // 设置最后一次使用法力时间
    [[nodiscard]] bool IsUnderLastManaUseEffect() const; // 是否在最后一次法力使用效果中

    float GetAPMultiplier(WeaponAttackType attType, bool normalized); // 获取攻击强度乘数

    // 次要属性
    [[nodiscard]] uint32 GetArmor() const { return GetResistance(SPELL_SCHOOL_NORMAL); } // 获取护甲
    void SetArmor(int32 val) { SetResistance(SPELL_SCHOOL_NORMAL, val); } // 设置护甲

    [[nodiscard]] float GetUnitDodgeChance() const; // 获取躲闪几率
    [[nodiscard]] float GetUnitParryChance() const; // 获取招架几率
    [[nodiscard]] float GetUnitBlockChance() const; // 获取格挡几率

    [[nodiscard]] float GetUnitMissChance(WeaponAttackType attType) const; // 获取未命中几率
    float GetUnitCriticalChance(WeaponAttackType attackType, Unit const* victim) const; // 获取暴击几率
    MeleeHitOutcome RollMeleeOutcomeAgainst(Unit const* victim, WeaponAttackType attType) const; // 掷得近战命中结果
    MeleeHitOutcome RollMeleeOutcomeAgainst(Unit const* victim, WeaponAttackType attType, int32 crit_chance, int32 miss_chance, int32 dodge_chance, int32 parry_chance, int32 block_chance) const; // 掷得近战命中结果

    // 韧性
    static void ApplyResilience(Unit const* victim, float* crit, int32* damage, bool isCrit, CombatRating type); // 应用韧性
    [[nodiscard]] bool CanApplyResilience() const { return m_applyResilience; } // 是否应用韧性

    // 技能值
    [[nodiscard]] virtual uint32 GetShieldBlockValue() const = 0; // 获取盾牌格挡值
    [[nodiscard]] uint32 GetShieldBlockValue(uint32 soft_cap, uint32 hard_cap) const
    {
        uint32 value = GetShieldBlockValue();
        if (value >= hard_cap)
        {
            value = (soft_cap + hard_cap) / 2;
        }
        else if (value > soft_cap)
        {
            value = soft_cap + ((value - soft_cap) / 2);
        }

        return value;
    } // 获取盾牌格挡值

    uint32 GetUnitMeleeSkill(Unit const* target = nullptr) const { return (target ? getLevelForTarget(target) : GetLevel()) * 5; } // 获取近战技能等级
    uint32 GetDefenseSkillValue(Unit const* target = nullptr) const; // 获取防御技能值
    uint32 GetWeaponSkillValue(WeaponAttackType attType, Unit const* target = nullptr) const; // 获取武器技能值

    // 抗性方法
    int32 GetMechanicResistChance(SpellInfo const* spell); // 获取机制抗性几率
    [[nodiscard]] uint32 GetResistance(SpellSchoolMask mask) const; // 获取抗性
    [[nodiscard]] uint32 GetResistance(SpellSchools school) const { return GetUInt32Value(static_cast<uint16>(UNIT_FIELD_RESISTANCES) + school); } // 获取抗性
    static float GetEffectiveResistChance(Unit const* owner, SpellSchoolMask schoolMask, Unit const* victim); // 获取有效抗性几率
    [[nodiscard]] float GetResistanceBuffMods(SpellSchools school, bool positive) const { return GetFloatValue(positive ? static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE) + school : static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE) + +school); } // 获取抗性增益

    void SetResistance(SpellSchools school, int32 val) { SetStatInt32Value(static_cast<uint16>(UNIT_FIELD_RESISTANCES) + school, val); } // 设置抗性
    void SetResistanceBuffMods(SpellSchools school, bool positive, float val) { SetFloatValue(positive ? static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE) + school : static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE) + +school, val); } // 设置抗性增益

    void ApplyResistanceBuffModsMod(SpellSchools school, bool positive, float val, bool apply) { ApplyModSignedFloatValue(positive ? static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE) + school : static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE) + +school, val, apply); } // 应用抗性增益修正
    void ApplyResistanceBuffModsPercentMod(SpellSchools school, bool positive, float val, bool apply) { ApplyPercentModFloatValue(positive ? static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE) + school : static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE) + +school, val, apply); } // 应用抗性增益百分比修正

    ////////////    需要分类   ////////////////
    uint16 GetMaxSkillValueForLevel(Unit const* target = nullptr) const { return (target ? getLevelForTarget(target) : GetLevel()) * 5; } // 获取最大技能值
    [[nodiscard]] float GetTotalAuraModValue(UnitMods unitMod) const; // 获取总光环修正值

    [[nodiscard]] SpellSchools GetSpellSchoolByAuraGroup(UnitMods unitMod) const; // 获取光环组对应的法术学派

    float GetTotalAttackPowerValue(WeaponAttackType attType, Unit* pVictim = nullptr) const; // 获取总攻击强度值
    [[nodiscard]] float GetWeaponDamageRange(WeaponAttackType attType, WeaponDamageRange type, uint8 damageIndex = 0) const; // 获取武器伤害范围
    void SetBaseWeaponDamage(WeaponAttackType attType, WeaponDamageRange damageRange, float value, uint8 damageIndex = 0) { m_weaponDamage[attType][damageRange][damageIndex] = value; } // 设置基础武器伤害

    // 虚函数
    virtual bool UpdateStats(Stats stat) = 0; // 更新属性
    virtual bool UpdateAllStats() = 0; // 更新所有属性
    virtual void UpdateResistances(uint32 school) = 0; // 更新抗性
    virtual void UpdateAllResistances(); // 更新所有抗性
    virtual void UpdateArmor() = 0; // 更新护甲
    virtual void UpdateMaxHealth() = 0; // 更新最大生命值
    virtual void UpdateMaxPower(Powers power) = 0; // 更新最大法力值
    virtual void UpdateAttackPowerAndDamage(bool ranged = false) = 0; // 更新攻击强度和伤害
    virtual void UpdateDamagePhysical(WeaponAttackType attType); // 更新物理伤害

    /*********************************************************/
    /***       METHODS RELATED TO DAMAGE CACULATIONS       ***/
    /*********************************************************/
    static uint32 DealDamage(Unit* attacker, Unit* victim, uint32 damage, CleanDamage const* cleanDamage = nullptr, DamageEffectType damagetype = DIRECT_DAMAGE, SpellSchoolMask damageSchoolMask = SPELL_SCHOOL_MASK_NORMAL, SpellInfo const* spellProto = nullptr, bool durabilityLoss = true, bool allowGM = false, Spell const* spell = nullptr); // 处理伤害
    void DealMeleeDamage(CalcDamageInfo* damageInfo, bool durabilityLoss); // 处理近战伤害
    void DealSpellDamage(SpellNonMeleeDamage* damageInfo, bool durabilityLoss, Spell const* spell = nullptr); // 处理法术伤害
    static void DealDamageMods(Unit const* victim, uint32& damage, uint32* absorb); // 处理伤害修正

    static void Kill(Unit* killer, Unit* victim, bool durabilityLoss = true, WeaponAttackType attackType = BASE_ATTACK, SpellInfo const* spellProto = nullptr, Spell const* spell = nullptr); // 杀死单位
    void KillSelf(bool durabilityLoss = true, WeaponAttackType attackType = BASE_ATTACK, SpellInfo const* spellProto = nullptr, Spell const* spell = nullptr) { Kill(this, this, durabilityLoss, attackType, spellProto, spell); }; // 自杀

    // 计算方法
    uint32 CalculateDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, uint8 itemDamagesMask = 0); // 计算伤害
    virtual void CalculateMinMaxDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, float& minDamage, float& maxDamage, uint8 damageIndex = 0) = 0; // 计算最小最大伤害
    void CalculateMeleeDamage(Unit* victim, CalcDamageInfo* damageInfo, WeaponAttackType attackType = BASE_ATTACK, const bool sittingVictim = false); // 计算近战伤害

    void CalculateSpellDamageTaken(SpellNonMeleeDamage* damageInfo, int32 damage, SpellInfo const* spellInfo, WeaponAttackType attackType = BASE_ATTACK, bool crit = false);
    // 计算法术造成的伤害减免，考虑抗性、护甲等因素

    int32 CalculateSpellDamage(Unit const* target, SpellInfo const* spellProto, uint8 effect_index, int32 const* basePoints = nullptr) const;
    // 根据目标、法术信息和效果索引计算法术的基础伤害值

    float CalculateDefaultCoefficient(SpellInfo const* spellInfo, DamageEffectType damagetype) const;
    // 根据法术信息和伤害类型计算默认的伤害系数

    // 近战伤害加成
    uint32 MeleeDamageBonusDone(Unit* pVictim, uint32 damage, WeaponAttackType attType, SpellInfo const* spellProto = nullptr, SpellSchoolMask damageSchoolMask = SPELL_SCHOOL_MASK_NORMAL);
    // 计算本单位对目标造成的近战伤害加成（如增伤效果）

    uint32 MeleeDamageBonusTaken(Unit* attacker, uint32 pdamage, WeaponAttackType attType, SpellInfo const* spellProto = nullptr, SpellSchoolMask damageSchoolMask = SPELL_SCHOOL_MASK_NORMAL);
    // 计算本单位受到的近战伤害被调整后的值（如减伤效果）

    // 法术伤害加成
    int32 SpellBaseDamageBonusDone(SpellSchoolMask schoolMask);
    // 计算对指定法术学派的伤害加成（基础值）

    int32 SpellBaseDamageBonusTaken(SpellSchoolMask schoolMask, bool isDoT = false);
    // 计算本单位受到的指定法术学派伤害的减免（基础值）

    float SpellPctDamageModsDone(Unit* victim, SpellInfo const* spellProto, DamageEffectType damagetype);
    // 计算对指定目标造成的法术伤害百分比加成

    uint32 SpellDamageBonusDone(Unit* victim, SpellInfo const* spellProto, uint32 pdamage, DamageEffectType damagetype, uint8 effIndex, float TotalMod = 0.0f, uint32 stack = 1);
    // 计算法术伤害加成的总值，包括各种增益效果

    uint32 SpellDamageBonusTaken(Unit* caster, SpellInfo const* spellProto, uint32 pdamage, DamageEffectType damagetype, uint32 stack = 1);
    // 计算本单位受到的法术伤害被调整后的值，包括各种减伤效果

    // 群体伤害减免
    int32 CalculateAOEDamageReduction(int32 damage, uint32 schoolMask, bool npcCaster) const;
    // 计算群体伤害的减免值，考虑职业和等级差异

    // 护甲减免
    static bool IsDamageReducedByArmor(SpellSchoolMask damageSchoolMask, SpellInfo const* spellInfo = nullptr, uint8 effIndex = MAX_SPELL_EFFECTS);
    // 判断某种法术伤害是否会被护甲减免

    static uint32 CalcArmorReducedDamage(Unit const* attacker, Unit const* victim, const uint32 damage, SpellInfo const* spellInfo, uint8 attackerLevel = 0, WeaponAttackType attackType = MAX_ATTACK);
    // 根据攻击者和受害者的等级与护甲计算护甲减免后的伤害值

    // 韧性减免 - 玩家或玩家宠物的韧性（-1%），上限100%
    [[nodiscard]] uint32 GetMeleeDamageReduction(uint32 damage) const { return GetCombatRatingDamageReduction(CR_CRIT_TAKEN_MELEE, 2.0f, 100.0f, damage); }
    // 获取近战伤害的韧性减免值

    [[nodiscard]] uint32 GetRangedDamageReduction(uint32 damage) const { return GetCombatRatingDamageReduction(CR_CRIT_TAKEN_RANGED, 2.0f, 100.0f, damage); }
    // 获取远程伤害的韧性减免值

    [[nodiscard]] uint32 GetSpellDamageReduction(uint32 damage) const { return GetCombatRatingDamageReduction(CR_CRIT_TAKEN_SPELL, 2.0f, 100.0f, damage); }
    // 获取法术伤害的韧性减免值

    // 暴击伤害 - 韧性：玩家或玩家宠物的韧性 (-1%)
    static uint32 SpellCriticalDamageBonus(Unit const* caster, SpellInfo const* spellProto, uint32 damage, Unit const* victim);
    // 计算法术暴击伤害加成，考虑施法者和目标的属性

    [[nodiscard]] float GetMeleeCritChanceReduction() const { return GetCombatRatingReduction(CR_CRIT_TAKEN_MELEE); }
    // 获取近战暴击概率的韧性减免

    [[nodiscard]] float GetRangedCritChanceReduction() const { return GetCombatRatingReduction(CR_CRIT_TAKEN_RANGED); }
    // 获取远程暴击概率的韧性减免

    [[nodiscard]] float GetSpellCritChanceReduction() const { return GetCombatRatingReduction(CR_CRIT_TAKEN_SPELL); }
    // 获取法术暴击概率的韧性减免

    [[nodiscard]] uint32 GetMeleeCritDamageReduction(uint32 damage) const { return GetCombatRatingDamageReduction(CR_CRIT_TAKEN_MELEE, 2.2f, 33.0f, damage); }
    // 获取近战暴击伤害的韧性减免

    [[nodiscard]] uint32 GetRangedCritDamageReduction(uint32 damage) const { return GetCombatRatingDamageReduction(CR_CRIT_TAKEN_RANGED, 2.2f, 33.0f, damage); }
    // 获取远程暴击伤害的韧性减免

    [[nodiscard]] uint32 GetSpellCritDamageReduction(uint32 damage) const { return GetCombatRatingDamageReduction(CR_CRIT_TAKEN_SPELL, 2.2f, 33.0f, damage); }
    // 获取法术暴击伤害的韧性减免

    /*********************************************************/
    /***         与控制单位（CHARM）系统相关的方法           ***/
    /*********************************************************/
    CharmInfo* GetCharmInfo() { return m_charmInfo; }
    // 获取控制信息

    CharmInfo* InitCharmInfo();
    // 初始化控制信息

    [[nodiscard]] Unit* GetCharmer() const;
    // 获取控制本单位的单位
    Unit* GetCharm() const;
    [[nodiscard]] Unit* GetCharmerOrOwner() const { return GetCharmerGUID() ? GetCharmer() : GetOwner(); }
    // 获取控制者或所有者单位

    [[nodiscard]] Unit* GetCharmerOrOwnerOrSelf() const
    {
        if (Unit* u = GetCharmerOrOwner())
            return u;

        return (Unit*)this;
    }
    // 获取控制者、所有者或本单位

    [[nodiscard]] Player* GetCharmerOrOwnerPlayerOrPlayerItself() const;
    // 获取控制者、所有者或自身玩家

    [[nodiscard]] ObjectGuid GetCharmerOrOwnerGUID() const { return GetCharmerGUID() ? GetCharmerGUID() : GetOwnerGUID(); }
    // 获取控制者或所有者的GUID

    [[nodiscard]] ObjectGuid GetCharmerOrOwnerOrOwnGUID() const
    {
        if (ObjectGuid guid = GetCharmerOrOwnerGUID())
            return guid;

        return GetGUID();
    }
    // 获取控制者、所有者或自身的GUID

    [[nodiscard]] Player* GetAffectingPlayer() const;
    // 获取影响本单位的玩家

    [[nodiscard]] Unit* GetFirstControlled() const;
    // 获取第一个被控制的单位

    [[nodiscard]] bool IsControlledByPlayer() const { return m_ControlledByPlayer; }
    // 判断本单位是否由玩家控制

    [[nodiscard]] bool IsCreatedByPlayer() const { return m_CreatedByPlayer; }
    // 判断本单位是否由玩家创建

    [[nodiscard]] bool IsCharmedOwnedByPlayerOrPlayer() const { return GetCharmerOrOwnerOrOwnGUID().IsPlayer(); }
    // 判断本单位的控制者、所有者或自身是否为玩家

    [[nodiscard]] bool IsCharmed() const { return GetCharmerGUID(); }
    // 判断本单位是否被控制

    [[nodiscard]] bool isPossessed() const { return HasUnitState(UNIT_STATE_POSSESSED); }
    // 判断本单位是否被附身

    [[nodiscard]] bool isPossessedByPlayer() const { return HasUnitState(UNIT_STATE_POSSESSED) && GetCharmerGUID().IsPlayer(); }
    // 判断本单位是否被玩家附身

    [[nodiscard]] bool isPossessing() const
    {
        if (Unit* u = GetCharm())
            return u->isPossessed();
        else
            return false;
    }
    // 判断本单位是否正在附身其他单位

    bool isPossessing(Unit* u) const { return u->isPossessed() && GetCharmGUID() == u->GetGUID(); }
    // 判断本单位是否正在附身指定单位

    void SetCharm(Unit* target, bool apply);
    // 设置对指定单位的控制状态

    bool SetCharmedBy(Unit* charmer, CharmType type, AuraApplication const* aurApp = nullptr);
    // 设置本单位被指定单位控制

    void UpdateCharmAI();
    // 更新控制单位的AI

    void RemoveCharmedBy(Unit* charmer);
    // 移除指定单位对本单位的控制

    void RemoveCharmAuras();
    // 移除控制相关的增益效果

    void RemoveAllControlled(bool onDeath = false);
    // 移除所有被控制的单位

    void DeleteCharmInfo();
    // 删除控制信息

    /*********************************************************/
    /***          与冷却时间（COOLDOWNS）相关的方法           ***/
    /*********************************************************/
    [[nodiscard]] virtual bool HasSpellCooldown(uint32 /*spell_id*/) const { return false; }
    // 判断指定法术是否有冷却时间（虚函数，可被覆盖）

    [[nodiscard]] virtual bool HasSpellItemCooldown(uint32 /*spell_id*/, uint32 /*itemid*/) const { return false; }
    // 判断指定法术和物品是否有冷却时间（虚函数，可被覆盖）

    virtual void AddSpellCooldown(uint32 /*spell_id*/, uint32 /*itemid*/, uint32 /*end_time*/, bool needSendToClient = false, bool forceSendToSpectator = false)
    {
        // workaround for unused parameters
        (void)needSendToClient;
        (void)forceSendToSpectator;
    }
    // 添加法术冷却时间（虚函数，可被覆盖）

    void BuildCooldownPacket(WorldPacket& data, uint8 flags, uint32 spellId, uint32 cooldown);
    // 构建冷却时间数据包

    void BuildCooldownPacket(WorldPacket& data, uint8 flags, PacketCooldowns const& cooldowns);
    // 构建包含多个冷却时间的数据包

    /*********************************************************/
    /***            与增益效果（AURAS）相关的方法            ***/
    /*********************************************************/
    Aura* AddAura(uint32 spellId, Unit* target);
    // 为指定目标添加指定法术ID的增益效果

    Aura* AddAura(SpellInfo const* spellInfo, uint8 effMask, Unit* target);
    // 为指定目标添加基于法术信息的增益效果

    void SetAuraStack(uint32 spellId, Unit* target, uint32 stack);
    // 设置指定法术在目标上的叠加层数

    // 增益效果应用/移除辅助函数 - 建议不要直接使用这些
    Aura* _TryStackingOrRefreshingExistingAura(SpellInfo const* newAura, uint8 effMask, Unit* caster, int32* baseAmount = nullptr, Item* castItem = nullptr, ObjectGuid casterGUID = ObjectGuid::Empty, bool periodicReset = false);
    // 尝试叠加或刷新已有的增益效果

    void _AddAura(UnitAura* aura, Unit* caster);
    // 添加增益效果到单位

    AuraApplication* _CreateAuraApplication(Aura* aura, uint8 effMask);
    // 创建增益效果的应用对象

    void _ApplyAuraEffect(Aura* aura, uint8 effIndex);
    // 应用增益效果的指定效果

    void _ApplyAura(AuraApplication* aurApp, uint8 effMask);
    // 应用增益效果

    void _UnapplyAura(AuraApplicationMap::iterator& i, AuraRemoveMode removeMode);
    // 移除增益效果

    void _UnapplyAura(AuraApplication* aurApp, AuraRemoveMode removeMode);
    // 移除指定的增益效果

    void _RemoveNoStackAuraApplicationsDueToAura(Aura* aura);
    // 移除因指定增益效果而无法叠加的增益应用

    void _RemoveNoStackAurasDueToAura(Aura* aura);
    // 移除因指定增益效果而无法叠加的增益效果

    bool _IsNoStackAuraDueToAura(Aura* appliedAura, Aura* existingAura) const;
    // 判断已应用的增益效果是否因现有增益而无法叠加

    void _RegisterAuraEffect(AuraEffect* aurEff, bool apply);
    // 注册或取消注册增益效果

    // m_ownedAuras 容器管理
    AuraMap& GetOwnedAuras() { return m_ownedAuras; }
    [[nodiscard]] AuraMap const& GetOwnedAuras() const { return m_ownedAuras; }

    void RemoveOwnedAura(AuraMap::iterator& i, AuraRemoveMode removeMode = AURA_REMOVE_BY_DEFAULT);
    // 从m_ownedAuras中移除指定增益效果

    void RemoveOwnedAura(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0, AuraRemoveMode removeMode = AURA_REMOVE_BY_DEFAULT);
    // 移除指定法术ID的增益效果

    void RemoveOwnedAura(Aura* aura, AuraRemoveMode removeMode = AURA_REMOVE_BY_DEFAULT);
    // 移除指定的增益效果

    Aura* GetOwnedAura(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, ObjectGuid itemCasterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0, Aura* except = nullptr) const;
    // 获取指定法术ID的增益效果

    // m_appliedAuras 容器管理
    AuraApplicationMap& GetAppliedAuras() { return m_appliedAuras; }
    [[nodiscard]] AuraApplicationMap const& GetAppliedAuras() const { return m_appliedAuras; }

    void RemoveAura(AuraApplicationMap::iterator& i, AuraRemoveMode mode = AURA_REMOVE_BY_DEFAULT);
    // 从m_appliedAuras中移除指定增益效果

    void RemoveAura(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0, AuraRemoveMode removeMode = AURA_REMOVE_BY_DEFAULT);
    // 移除指定法术ID的增益效果

    void RemoveAura(AuraApplication* aurApp, AuraRemoveMode mode = AURA_REMOVE_BY_DEFAULT);
    // 移除指定的增益效果应用

    void RemoveAura(Aura* aur, AuraRemoveMode mode = AURA_REMOVE_BY_DEFAULT);
    // 移除指定的增益效果

    void RemoveBindSightAuras();
    // 移除绑定视野的增益效果

    // 通过谓词移除增益效果
    void RemoveAppliedAuras(std::function<bool(AuraApplication const*)> const& check);
    // 根据条件移除应用中的增益效果

    void RemoveOwnedAuras(std::function<bool(Aura const*)> const& check);
    // 根据条件移除拥有的增益效果

    // 优化的重载方法，利用map的键
    void RemoveAppliedAuras(uint32 spellId, std::function<bool(AuraApplication const*)> const& check);
    // 移除指定法术ID的应用中的增益效果

    void RemoveOwnedAuras(uint32 spellId, std::function<bool(Aura const*)> const& check);
    // 移除指定法术ID的拥有的增益效果

    void RemoveAurasDueToSpell(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0, AuraRemoveMode removeMode = AURA_REMOVE_BY_DEFAULT);
    // 移除因指定法术而存在的增益效果

    void RemoveAuraFromStack(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, AuraRemoveMode removeMode = AURA_REMOVE_BY_DEFAULT);
    // 从堆栈中移除指定法术ID的增益效果

    void RemoveAurasDueToSpellByDispel(uint32 spellId, uint32 dispellerSpellId, ObjectGuid casterGUID, Unit* dispeller, uint8 chargesRemoved = 1);
    // 通过驱散移除指定法术的增益效果

    void RemoveAurasDueToSpellBySteal(uint32 spellId, ObjectGuid casterGUID, Unit* stealer);
    // 通过窃取移除指定法术的增益效果

    void RemoveAurasDueToItemSpell(uint32 spellId, ObjectGuid castItemGuid);
    // 移除因指定物品法术而存在的增益效果

    void RemoveAurasByType(AuraType auraType, ObjectGuid casterGUID = ObjectGuid::Empty, Aura* except = nullptr, bool negative = true, bool positive = true);
    // 移除指定类型的增益效果

    void RemoveNotOwnSingleTargetAuras();
    // 移除非本单位单目标的增益效果

    void RemoveAurasWithInterruptFlags(uint32 flag, uint32 except = 0, bool isAutoshot = false);
    // 移除具有指定中断标志的增益效果

    void RemoveAurasWithAttribute(uint32 flags);
    // 移除具有指定属性的增益效果

    void RemoveAurasWithFamily(SpellFamilyNames family, uint32 familyFlag1, uint32 familyFlag2, uint32 familyFlag3, ObjectGuid casterGUID);
    // 移除属于指定法术家族的增益效果

    void RemoveAurasWithMechanic(uint32 mechanic_mask, AuraRemoveMode removemode = AURA_REMOVE_BY_DEFAULT, uint32 except = 0);
    // 移除具有指定机制的增益效果

    void RemoveMovementImpairingAuras(bool withRoot);
    // 移除影响移动的增益效果

    void RemoveAurasByShapeShift();
    // 移除形态转换相关的增益效果

    void RemoveAreaAurasDueToLeaveWorld();
    // 移除因离开世界而存在的区域增益效果

    void RemoveAllAuras();
    // 移除所有增益效果

    void RemoveArenaAuras();
    // 移除竞技场相关的增益效果

    void RemoveAllAurasOnDeath();
    // 在死亡时移除所有增益效果

    void RemoveAllAurasRequiringDeadTarget();
    // 移除需要目标死亡的增益效果

    void RemoveAllAurasExceptType(AuraType type);
    // 移除所有除指定类型外的增益效果

    void RemoveEvadeAuras();
    // 移除逃避相关的增益效果

    void DelayOwnedAuras(uint32 spellId, ObjectGuid caster, int32 delaytime);
    // 延迟指定法术ID的增益效果

    void _RemoveAllAuraStatMods();
    // 移除所有增益效果对属性的修改

    void _ApplyAllAuraStatMods();
    // 应用所有增益效果对属性的修改

    [[nodiscard]] AuraEffectList const& GetAuraEffectsByType(AuraType type) const { return m_modAuras[type]; }
    // 获取指定类型的增益效果列表

    AuraList& GetSingleCastAuras() { return m_scAuras; }
    [[nodiscard]] AuraList const& GetSingleCastAuras() const { return m_scAuras; }

    [[nodiscard]] AuraEffect* GetAuraEffect(uint32 spellId, uint8 effIndex, ObjectGuid casterGUID = ObjectGuid::Empty) const;
    // 获取指定法术ID和效果索引的增益效果

    [[nodiscard]] AuraEffect* GetAuraEffectOfRankedSpell(uint32 spellId, uint8 effIndex, ObjectGuid casterGUID = ObjectGuid::Empty) const;
    // 获取指定等级法术的效果

    [[nodiscard]] AuraEffect* GetAuraEffect(AuraType type, SpellFamilyNames name, uint32 iconId, uint8 effIndex) const;
    // 获取指定类型、法术家族、图标和效果索引的增益效果

    [[nodiscard]] AuraEffect* GetAuraEffect(AuraType type, SpellFamilyNames family, uint32 familyFlag1, uint32 familyFlag2, uint32 familyFlag3, ObjectGuid casterGUID = ObjectGuid::Empty) const;
    // 获取指定类型、法术家族和标志的增益效果

    [[nodiscard]] AuraEffect* GetAuraEffectDummy(uint32 spellid) const;
    // 获取指定法术ID的虚拟增益效果

    [[nodiscard]] inline AuraEffect* GetDummyAuraEffect(SpellFamilyNames name, uint32 iconId, uint8 effIndex) const { return GetAuraEffect(SPELL_AURA_DUMMY, name, iconId, effIndex); }
    // 获取指定法术家族、图标和效果索引的虚拟增益效果

    AuraApplication* GetAuraApplication(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, ObjectGuid itemCasterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0, AuraApplication* except = nullptr) const;
    // 获取指定法术ID的增益效果应用

    [[nodiscard]] Aura* GetAura(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, ObjectGuid itemCasterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0) const;
    // 获取指定法术ID的增益效果

    AuraApplication* GetAuraApplicationOfRankedSpell(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, ObjectGuid itemCasterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0, AuraApplication* except = nullptr) const;
    // 获取指定等级法术的增益效果应用

    [[nodiscard]] Aura* GetAuraOfRankedSpell(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, ObjectGuid itemCasterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0) const;
    // 获取指定等级法术的增益效果

    void GetDispellableAuraList(Unit* caster, uint32 dispelMask, DispelChargesList& dispelList, SpellInfo const* dispelSpell);
    // 获取可驱散的增益效果列表

    [[nodiscard]] bool HasAuraEffect(uint32 spellId, uint8 effIndex, ObjectGuid caster = ObjectGuid::Empty) const;
    // 判断是否存在指定法术ID和效果索引的增益效果

    [[nodiscard]] uint32 GetAuraCount(uint32 spellId) const;
    // 获取指定法术ID的增益效果数量

    /**
    * @brief 检查单位是否具有任何或所有指定的增益效果。
    *
    * @param sm 使用的搜索方法
    *           - SearchMethod::MatchAll : 检查单位上是否具有所有指定的法术ID。
    *           - SearchMethod::MatchAny : 检查单位上是否具有任意一个指定的法术ID。
    *
    * @param spellIds 要检查的法术ID列表。
    *
    * @return 如果满足搜索条件则返回true，否则返回false。
    */
    bool HasAuras(SearchMethod sm, std::vector<uint32>& spellIds) const;

    /**
     * @brief 检查单位是否具有任意一个指定的增益效果。
     *
     * @tparam Auras 可以是任何可转换为uint32的类型。
     * @param spellIds 要检查的法术ID列表。
     *
     * @return 如果单位具有任意一个指定的增益效果则返回true，否则返回false。
     */
    template <typename... Auras>
    bool HasAnyAuras(Auras... spellIds) const
    {
        std::vector<uint32> spellList = { static_cast<uint32>(spellIds)... };
        return HasAuras(SearchMethod::MatchAny, spellList);
    }

    /**
     * @brief 检查单位是否具有所有指定的增益效果。
     *
     * @tparam Auras 可以是任何可转换为uint32的类型。
     * @param spellIds 要检查的法术ID列表。
     *
     * @return 如果单位具有所有指定的增益效果则返回true，否则返回false。
     */
    template <typename... Auras>
    bool HasAllAuras(Auras... spellIds) const
    {
        std::vector<uint32> spellList = { static_cast<uint32>(spellIds)... };
        return HasAuras(SearchMethod::MatchAll, spellList);
    }
    // 判断是否存在指定法术ID的增益效果
    [[nodiscard]] bool HasAura(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, ObjectGuid itemCasterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0) const;
    
    // 判断是否存在指定类型的增益效果
    [[nodiscard]] bool HasAuraType(AuraType auraType) const;
    
    // 判断是否存在指定施法者触发的指定类型的增益效果
    [[nodiscard]] bool HasAuraTypeWithCaster(AuraType auratype, ObjectGuid caster) const;
    
    // 判断是否存在指定miscvalue值的指定类型的增益效果
    [[nodiscard]] bool HasAuraTypeWithMiscvalue(AuraType auratype, int32 miscvalue) const;
    
    // 判断是否存在影响指定法术的指定类型的增益效果
    bool HasAuraTypeWithAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;
    
    // 判断是否存在指定值的指定类型的增益效果
    [[nodiscard]] bool HasAuraTypeWithValue(AuraType auratype, int32 value) const;
    
    // 判断是否存在指定触发法术的指定类型的增益效果
    [[nodiscard]] bool HasAuraTypeWithTriggerSpell(AuraType auratype, uint32 triggerSpell) const;
    
    // 判断是否存在具有指定中断标志的负面增益效果
    bool HasNegativeAuraWithInterruptFlag(uint32 flag, ObjectGuid guid = ObjectGuid::Empty);
    
    // 判断是否存在可见的指定类型的增益效果
    [[nodiscard]] bool HasVisibleAuraType(AuraType auraType) const;
    
    // 判断是否存在具有指定属性的负面增益效果
    bool HasNegativeAuraWithAttribute(uint32 flag, ObjectGuid guid = ObjectGuid::Empty);
    
    // 判断是否存在具有指定机制的增益效果
    [[nodiscard]] bool HasAuraWithMechanic(uint32 mechanicMask) const;
    
    // 判断是否存在指定法术家族标志的指定类型的增益效果
    [[nodiscard]] bool HasAuraTypeWithFamilyFlags(AuraType auraType, uint32 familyName, uint32 familyFlags) const;
  
    // 判断是否存在可被伤害打断的指定类型的增益效果
    [[nodiscard]] bool HasBreakableByDamageAuraType(AuraType type, uint32 excludeAura = 0) const;

    // 判断是否存在可被伤害打断的控制类增益效果
    bool HasBreakableByDamageCrowdControlAura(Unit* excludeCasterChannel = nullptr) const;

    // 判断指定法术是否被脚本覆盖
    AuraEffect* IsScriptOverriden(SpellInfo const* spell, int32 script) const;
    
    // 获取指定施法者造成的疾病数量
    uint32 GetDiseasesByCaster(ObjectGuid casterGUID, uint8 mode = 0);
    
    // 获取指定施法者造成的持续伤害效果数量
    [[nodiscard]] uint32 GetDoTsByCaster(ObjectGuid casterGUID) const;
    
    // 获取指定类型的区域独占增益效果的总修改值
    [[nodiscard]] int32 GetTotalAuraModifierAreaExclusive(AuraType auratype) const;
    
    // 获取指定类型的增益效果的总修改值
    [[nodiscard]] int32 GetTotalAuraModifier(AuraType auratype) const;
    
    // 获取指定类型的增益效果的总倍率修改值
    [[nodiscard]] float GetTotalAuraMultiplier(AuraType auratype) const;
    
    // 获取指定类型的正向增益效果的最大修改值
    int32 GetMaxPositiveAuraModifier(AuraType auratype);
    
    // 获取指定类型的负向增益效果的最大修改值
    [[nodiscard]] int32 GetMaxNegativeAuraModifier(AuraType auratype) const;
    
    // 获取指定misc_mask的指定类型的增益效果的总修改值
    [[nodiscard]] int32 GetTotalAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const;
    

    [[nodiscard]] float GetTotalAuraMultiplierByMiscMask(AuraType auratype, uint32 misc_mask) const;
    // 获取指定misc_mask的指定类型的增益效果的总倍率修改值

    int32 GetMaxPositiveAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask, const AuraEffect* except = nullptr) const;
    // 获取指定misc_mask的指定类型的正向增益效果的最大修改值

    [[nodiscard]] int32 GetMaxNegativeAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const;
    // 获取指定misc_mask的指定类型的负向增益效果的最大修改值

    [[nodiscard]] int32 GetTotalAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const;
    // 获取指定misc_value的指定类型的增益效果的总修改值

    [[nodiscard]] float GetTotalAuraMultiplierByMiscValue(AuraType auratype, int32 misc_value) const;
    // 获取指定misc_value的指定类型的增益效果的总倍率修改值

    [[nodiscard]] int32 GetMaxPositiveAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const;
    // 获取指定misc_value的指定类型的正向增益效果的最大修改值

    [[nodiscard]] int32 GetMaxNegativeAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const;
    // 获取指定misc_value的指定类型的负向增益效果的最大修改值

    int32 GetTotalAuraModifierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;
    // 获取影响指定法术的指定类型的增益效果的总修改值

    float GetTotalAuraMultiplierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;
    // 获取影响指定法术的指定类型的增益效果的总倍率修改值

    int32 GetMaxPositiveAuraModifierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;
    // 获取影响指定法术的指定类型的正向增益效果的最大修改值

    int32 GetMaxNegativeAuraModifierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;
    // 获取影响指定法术的指定类型的负向增益效果的最大修改值

    VisibleAuraMap const* GetVisibleAuras() { return &m_visibleAuras; }
    // 获取可见的增益效果映射

    AuraApplication* GetVisibleAura(uint8 slot)
    {
        VisibleAuraMap::iterator itr = m_visibleAuras.find(slot);
        if (itr != m_visibleAuras.end())
            return itr->second;
        return nullptr;
    }
    // 获取指定槽位的可见增益效果

    void SetVisibleAura(uint8 slot, AuraApplication* aur) { m_visibleAuras[slot] = aur; UpdateAuraForGroup(slot); }
    // 设置指定槽位的可见增益效果

    void RemoveVisibleAura(uint8 slot) { m_visibleAuras.erase(slot); UpdateAuraForGroup(slot); }
    // 移除指定槽位的可见增益效果

    void ModifyAuraState(AuraStateType flag, bool apply);
    // 修改增益状态

    uint32 BuildAuraStateUpdateForTarget(Unit* target) const;
    // 构建增益状态更新数据包

    bool HasAuraState(AuraStateType flag, SpellInfo const* spellProto = nullptr, Unit const* Caster = nullptr) const;
    // 判断是否存在指定的增益状态

    /*********************************************************/
    /***         与触发系统（PROCS）相关的方法             ***/
    /*********************************************************/
    bool CanProc() { return !m_procDeep; }
    // 判断是否可以触发效果

    void SetCantProc(bool apply);
    // 设置是否禁止触发效果

    static void ProcDamageAndSpell(Unit* actor, Unit* victim, uint32 procAttacker, uint32 procVictim, uint32 procEx, uint32 amount, WeaponAttackType attType = BASE_ATTACK, SpellInfo const* procSpellInfo = nullptr, SpellInfo const* procAura = nullptr, int8 procAuraEffectIndex = -1, Spell const* procSpell = nullptr, DamageInfo* damageInfo = nullptr, HealInfo* healInfo = nullptr, uint32 procPhase = 2 /*PROC_SPELL_PHASE_HIT*/);
    // 处理伤害和法术触发事件

    void ProcDamageAndSpellFor(bool isVictim, Unit* target, uint32 procFlag, uint32 procExtra, WeaponAttackType attType, SpellInfo const* procSpellInfo, uint32 damage, SpellInfo const* procAura = nullptr, int8 procAuraEffectIndex = -1, Spell const* procSpell = nullptr, DamageInfo* damageInfo = nullptr, HealInfo* healInfo = nullptr, uint32 procPhase = 2 /*PROC_SPELL_PHASE_HIT*/);
    // 处理特定目标的伤害和法术触发事件

    void GetProcAurasTriggeredOnEvent(std::list<AuraApplication*>& aurasTriggeringProc, std::list<AuraApplication*>* procAuras, ProcEventInfo eventInfo);
    // 获取在指定事件上触发的增益效果

    void TriggerAurasProcOnEvent(CalcDamageInfo& damageInfo);
    // 触发与伤害信息相关的增益效果

    void TriggerAurasProcOnEvent(std::list<AuraApplication*>* myProcAuras, std::list<AuraApplication*>* targetProcAuras, Unit* actionTarget, uint32 typeMaskActor, uint32 typeMaskActionTarget, uint32 spellTypeMask, uint32 spellPhaseMask, uint32 hitMask, Spell* spell, DamageInfo* damageInfo, HealInfo* healInfo);
    // 触发指定的增益效果

    void TriggerAurasProcOnEvent(ProcEventInfo& eventInfo, std::list<AuraApplication*>& procAuras);
    // 触发指定事件的增益效果

    [[nodiscard]] float GetWeaponProcChance() const;
    // 获取武器触发概率

    float GetPPMProcChance(uint32 WeaponSpeed, float PPM, SpellInfo const* spellProto) const;
    // 获取每分钟触发次数的概率

    /*********************************************************/
    /***            与法术（SPELLS）相关的方法             ***/
    /*********************************************************/
    [[nodiscard]] Player* GetSpellModOwner() const;
    // 获取法术修改者的玩家对象

    [[nodiscard]] Spell* GetCurrentSpell(CurrentSpellTypes spellType) const { return m_currentSpells[spellType]; }
    // 获取当前正在施放的指定类型法术

    [[nodiscard]] Spell* GetCurrentSpell(uint32 spellType) const { return m_currentSpells[spellType]; }
    // 获取当前正在施放的指定法术

    [[nodiscard]] Spell* FindCurrentSpellBySpellId(uint32 spell_id) const;
    // 查找当前正在施放的指定法术ID的法术

    [[nodiscard]] int32 GetCurrentSpellCastTime(uint32 spell_id) const;
    // 获取指定法术ID的当前施法时间

    [[nodiscard]] bool virtual HasSpell(uint32 /*spellID*/) const { return false; }
    // 判断单位是否拥有指定法术（虚函数，可被覆盖）

    void SetCurrentCastedSpell(Spell* pSpell);
    // 设置当前正在施放的法术

    virtual void ProhibitSpellSchool(SpellSchoolMask /*idSchoolMask*/, uint32 /*unTimeMs*/) {}
    // 禁止指定法术学派（虚函数，可被覆盖）

    void SetInstantCast(bool set) { _instantCast = set; }
    // 设置是否允许瞬发施法

    [[nodiscard]] bool CanInstantCast() const { return _instantCast; }
    // 判断是否允许瞬发施法

    // set withDelayed to true to account delayed spells as casted
    // delayed+channeled spells are always accounted as casted
    // we can skip channeled or delayed checks using flags
    [[nodiscard]] bool IsNonMeleeSpellCast(bool withDelayed, bool skipChanneled = false, bool skipAutorepeat = false, bool isAutoshoot = false, bool skipInstant = true) const;
    // 判断是否存在非近战法术正在施放

    // set withDelayed to true to interrupt delayed spells too
    // delayed+channeled spells are always interrupted
    void InterruptNonMeleeSpells(bool withDelayed, uint32 spellid = 0, bool withInstant = true, bool bySelf = false);
    // 中断非近战法术

    // 目标依赖的范围检查
    float GetSpellMaxRangeForTarget(Unit const* target, SpellInfo const* spellInfo) const;
    // 获取指定目标和法术信息的最大施法距离

    float GetSpellMinRangeForTarget(Unit const* target, SpellInfo const* spellInfo) const;
    // 获取指定目标和法术信息的最小施法距离

    // 法术中断
    [[nodiscard]] uint32 GetInterruptMask() const { return m_interruptMask; }
    // 获取法术中断掩码

    void AddInterruptMask(uint32 mask) { m_interruptMask |= mask; }
    // 添加法术中断掩码

    void UpdateInterruptMask();
    // 更新法术中断掩码

    void InterruptSpell(CurrentSpellTypes spellType, bool withDelayed = true, bool withInstant = true, bool bySelf = false);
    // 中断指定类型的法术

    bool isSpellBlocked(Unit* victim, SpellInfo const* spellProto, WeaponAttackType attackType = BASE_ATTACK);
    // 判断法术是否被目标格挡

    void FinishSpell(CurrentSpellTypes spellType, bool ok = true);
    // 完成指定类型的法术施放

    // 法术命中方法
    Unit* GetMagicHitRedirectTarget(Unit* victim, SpellInfo const* spellInfo);
    // 获取魔法命中重定向的目标

    Unit* GetMeleeHitRedirectTarget(Unit* victim, SpellInfo const* spellInfo = nullptr);
    // 获取近战命中重定向的目标

    [[nodiscard]] float MeleeSpellMissChance(Unit const* victim, WeaponAttackType attType, int32 skillDiff, uint32 spellId) const;
    // 计算近战法术未命中的概率

    [[nodiscard]] SpellMissInfo MeleeSpellHitResult(Unit* victim, SpellInfo const* spell);
    // 计算近战法术的命中结果

    [[nodiscard]] SpellMissInfo MagicSpellHitResult(Unit* victim, SpellInfo const* spell);
    // 计算魔法法术的命中结果

    [[nodiscard]] SpellMissInfo SpellHitResult(Unit* victim, SpellInfo const* spell, bool canReflect = false);
    // 计算法术的命中结果，考虑反射

    [[nodiscard]] SpellMissInfo SpellHitResult(Unit* victim, Spell const* spell, bool canReflect = false);
    // 计算法术的命中结果，考虑反射

    // 治疗法术
    static int32 DealHeal(Unit* healer, Unit* victim, uint32 addhealth);
    // 处理治疗效果

    void SendHealSpellLog(HealInfo const& healInfo, bool critical = false);
    // 发送治疗法术日志

    int32 HealBySpell(HealInfo& healInfo, bool critical = false);
    // 通过法术进行治疗

    int32 SpellBaseHealingBonusDone(SpellSchoolMask schoolMask);
    // 计算对指定法术学派的治疗加成（基础值）

    int32 SpellBaseHealingBonusTaken(SpellSchoolMask schoolMask);
    // 计算本单位受到的指定法术学派治疗的加成（基础值）

    float SpellPctHealingModsDone(Unit* victim, SpellInfo const* spellProto, DamageEffectType damagetype);
    // 计算对指定目标造成的治疗百分比加成

    uint32 SpellHealingBonusDone(Unit* victim, SpellInfo const* spellProto, uint32 healamount, DamageEffectType damagetype, uint8 effIndex, float TotalMod = 0.0f, uint32 stack = 1);
    // 计算法术治疗加成的总值，包括各种增益效果

    uint32 SpellHealingBonusTaken(Unit* caster, SpellInfo const* spellProto, uint32 healamount, DamageEffectType damagetype, uint32 stack = 1);
    // 计算本单位受到的法术治疗被调整后的值，包括各种减益效果

    static uint32 SpellCriticalHealingBonus(Unit const* caster, SpellInfo const* spellProto, uint32 damage, Unit const* victim);
    // 计算法术暴击治疗加成，考虑施法者和目标的属性

    static void CalcAbsorbResist(DamageInfo& dmgInfo, bool Splited = false);
    // 计算吸收和抵抗的伤害

    static void CalcHealAbsorb(HealInfo& healInfo);
    // 计算治疗吸收效果

    // 能量恢复法术
    void SendEnergizeSpellLog(Unit* victim, uint32 SpellID, uint32 Damage, Powers powertype);
    // 发送能量恢复法术日志

    void EnergizeBySpell(Unit* victim, uint32 SpellID, uint32 Damage, Powers powertype);
    // 通过法术恢复能量

    // 法术免疫
    void ApplySpellImmune(uint32 spellId, uint32 op, uint32 type, bool apply, SpellImmuneBlockType blockType = SPELL_BLOCK_TYPE_ALL);
    // 应用法术免疫

    void ApplySpellDispelImmunity(SpellInfo const* spellProto, DispelType type, bool apply);
    // 应用法术驱散免疫

    virtual bool IsImmunedToSpell(SpellInfo const* spellInfo, Spell const* spell = nullptr);
    // 判断是否免疫指定法术

    [[nodiscard]] bool IsImmunedToDamage(SpellSchoolMask meleeSchoolMask) const;
    // 判断是否免疫指定学派的伤害

    [[nodiscard]] bool IsImmunedToDamage(SpellInfo const* spellInfo) const;
    // 判断是否免疫指定法术的伤害

    [[nodiscard]] bool IsImmunedToDamage(Spell const* spell) const;
    // 判断是否免疫指定法术的伤害

    [[nodiscard]] bool IsImmunedToSchool(SpellSchoolMask meleeSchoolMask) const;
    // 判断是否免疫指定学派的伤害

    [[nodiscard]] bool IsImmunedToSchool(SpellInfo const* spellInfo) const;
    // 判断是否免疫指定法术的学派伤害

    [[nodiscard]] bool IsImmunedToSchool(Spell const* spell) const;
    // 判断是否免疫指定法术的学派伤害

    [[nodiscard]] bool IsImmunedToDamageOrSchool(SpellSchoolMask meleeSchoolMask) const;
    // 判断是否免疫指定学派的伤害或该学派本身

    bool IsImmunedToDamageOrSchool(SpellInfo const* spellInfo) const;
    // 判断是否免疫指定法术的伤害或学派

    virtual bool IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index) const;
    // 判断是否免疫指定法术效果

    // 暴击概率
    bool isBlockCritical();
    // 判断是否可以暴击

    float SpellDoneCritChance(Unit const* /*victim*/, SpellInfo const* spellProto, SpellSchoolMask schoolMask, WeaponAttackType attackType, bool skipEffectCheck) const;
    // 计算法术暴击概率

    float SpellTakenCritChance(Unit const* caster, SpellInfo const* spellProto, SpellSchoolMask schoolMask, float doneChance, WeaponAttackType attackType, bool skipEffectCheck) const;
    // 计算受到法术暴击的概率

    // 其他修改器
    float ApplyEffectModifiers(SpellInfo const* spellProto, uint8 effect_index, float value) const;
    // 应用效果修改器

    int32 CalcSpellDuration(SpellInfo const* spellProto);
    // 计算法术持续时间

    int32 ModSpellDuration(SpellInfo const* spellProto, Unit const* target, int32 duration, bool positive, uint32 effectMask);
    // 修改法术持续时间

    void  ModSpellCastTime(SpellInfo const* spellProto, int32& castTime, Spell* spell = nullptr);
    // 修改法术施法时间

    float CalculateLevelPenalty(SpellInfo const* spellProto) const;
    // 计算等级惩罚

    uint32 GetCastingTimeForBonus(SpellInfo const* spellProto, DamageEffectType damagetype, uint32 CastingTime) const;
    // 获取用于加成计算的施法时间

    void CastDelayedSpellWithPeriodicAmount(Unit* caster, uint32 spellId, AuraType auraType, int32 addAmount, uint8 effectIndex = 0);
    // 延迟施放带有周期性效果的法术

    // 法术施放结果方法
    SpellCastResult CastSpell(SpellCastTargets const& targets, SpellInfo const* spellInfo, CustomSpellValues const* value, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    // 施放法术，指定目标、法术信息、自定义值等参数

    SpellCastResult CastSpell(Unit* victim, uint32 spellId, bool triggered, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    // 施放法术，指定目标、法术ID、触发状态等参数

    SpellCastResult CastSpell(Unit* victim, uint32 spellId, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    // 施放法术，指定目标、法术ID、触发标志等参数

    SpellCastResult CastSpell(Unit* victim, SpellInfo const* spellInfo, bool triggered, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    // 施放法术，指定目标、法术信息、触发状态等参数

    SpellCastResult CastSpell(Unit* victim, SpellInfo const* spellInfo, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    // 施放法术，指定目标、法术信息、触发标志等参数

    SpellCastResult CastSpell(float x, float y, float z, uint32 spellId, bool triggered, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    // 施放法术，指定坐标、法术ID、触发状态等参数

    SpellCastResult CastSpell(GameObject* go, uint32 spellId, bool triggered, Item* castItem = nullptr, AuraEffect* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    // 施放法术，指定游戏对象、法术ID、触发状态等参数

    SpellCastResult CastCustomSpell(Unit* victim, uint32 spellId, int32 const* bp0, int32 const* bp1, int32 const* bp2, bool triggered, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    // 施放自定义法术，指定目标、法术ID、基础值等参数

    SpellCastResult CastCustomSpell(uint32 spellId, SpellValueMod mod, int32 value, Unit* victim, bool triggered, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    // 施放自定义法术，指定法术ID、修改类型、值、目标等参数

    SpellCastResult CastCustomSpell(uint32 spellId, SpellValueMod mod, int32 value, Unit* victim = nullptr, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    // 施放自定义法术，指定法术ID、修改类型、值、目标、触发标志等参数

    SpellCastResult CastCustomSpell(uint32 spellId, CustomSpellValues const& value, Unit* victim = nullptr, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    // 施放自定义法术，指定法术ID、自定义值、目标、触发标志等参数

    /*********************************************************/
    /***     与游戏对象（GAMEOBJECT）和动态对象相关的方法    ***/
    /*********************************************************/
    void _RegisterDynObject(DynamicObject* dynObj);
    // 注册动态对象

    void _UnregisterDynObject(DynamicObject* dynObj);
    // 注销动态对象

    DynamicObject* GetDynObject(uint32 spellId);
    // 获取指定法术ID的动态对象

    bool RemoveDynObject(uint32 spellId);
    // 移除指定法术ID的动态对象

    void RemoveAllDynObjects();
    // 移除所有动态对象

    [[nodiscard]] GameObject* GetGameObject(uint32 spellId) const;
    // 获取指定法术ID的游戏对象

    void AddGameObject(GameObject* gameObj);
    // 添加游戏对象

    void RemoveGameObject(GameObject* gameObj, bool del);
    // 移除游戏对象

    void RemoveGameObject(uint32 spellid, bool del);
    // 移除指定法术ID的游戏对象

    void RemoveAllGameObjects();
    // 移除所有游戏对象

    /*********************************************************/
    /***           与移动（MOVEMENTS）相关的方法            ***/
    /*********************************************************/
    [[nodiscard]] bool IsPolymorphed() const;
    // 判断是否被变形

    [[nodiscard]] bool isFrozen() const;
    // 判断是否被冻结

    [[nodiscard]] bool IsInFlight()  const { return HasUnitState(UNIT_STATE_IN_FLIGHT); }
    // 判断是否处于飞行状态

    [[nodiscard]] bool IsLevitating() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY); }
    // 判断是否处于悬浮状态

    [[nodiscard]] bool IsWalking() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_WALKING); }
    // 判断是否处于行走状态

    [[nodiscard]] bool isMoving() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_MASK_MOVING); }
    // 判断是否处于移动状态

    [[nodiscard]] bool isTurning() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_MASK_TURNING); }
    // 判断是否处于转向状态

    [[nodiscard]] bool IsHovering() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_HOVER); }
    // 判断是否处于悬停状态

    [[nodiscard]] bool isSwimming() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_SWIMMING); }
    // 判断是否处于游泳状态

    [[nodiscard]] virtual bool CanFly() const = 0;
    // 判断是否可以飞行（纯虚函数，必须实现）

    [[nodiscard]] bool IsFlying() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FLYING | MOVEMENTFLAG_DISABLE_GRAVITY); }
    // 判断是否处于飞行状态

    [[nodiscard]] bool IsFalling() const;
    // 判断是否处于下落状态

    [[nodiscard]] float GetHoverHeight() const { return IsHovering() ? GetFloatValue(UNIT_FIELD_HOVERHEIGHT) : 0.0f; }
    // 获取悬停高度

    [[nodiscard]] virtual bool IsMovementPreventedByCasting() const;
    // 判断施法是否阻止移动（虚函数，可被覆盖）

    [[nodiscard]] virtual bool CanEnterWater() const = 0;
    // 判断是否可以进入水中（纯虚函数，必须实现）

    [[nodiscard]] virtual bool CanSwim() const;
    // 判断是否可以游泳（虚函数，可被覆盖）

    [[nodiscard]] bool CanFreeMove() const
    {
        return !HasUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING | UNIT_STATE_IN_FLIGHT |
            UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_DISTRACTED) && !GetOwnerGUID();
    }
    // 判断是否可以自由移动

    void KnockbackFrom(float x, float y, float speedXY, float speedZ);
    // 被击退

    void JumpTo(float speedXY, float speedZ, bool forward = true);
    // 跳跃

    void JumpTo(WorldObject* obj, float speedZ);
    // 跳向指定对象

    void UpdateSpeed(UnitMoveType mtype, bool forced);
    // 更新移动速度

    [[nodiscard]] float GetSpeed(UnitMoveType mtype) const;
    // 获取移动速度

    [[nodiscard]] float GetSpeedRate(UnitMoveType mtype) const { return m_speed_rate[mtype]; }
    // 获取移动速度比率

    void SetSpeed(UnitMoveType mtype, float rate, bool forced = false);
    // 设置移动速度

    void SetSpeedRate(UnitMoveType mtype, float rate) { m_speed_rate[mtype] = rate; }
    // 设置移动速度比率

    void propagateSpeedChange() { GetMotionMaster()->propagateSpeedChange(); }
    // 传播速度变化

    void SendMonsterMove(float NewPosX, float NewPosY, float NewPosZ, uint32 TransitTime, SplineFlags sf = SPLINEFLAG_WALK_MODE);
    // 发送怪物移动数据包

    void MonsterMoveWithSpeed(float x, float y, float z, float speed);
    // 以指定速度移动怪物

    virtual bool SetWalk(bool enable);
    // 设置行走状态

    virtual bool SetDisableGravity(bool disable, bool packetOnly = false, bool updateAnimationTier = true);
    // 设置是否禁用重力

    virtual bool SetSwim(bool enable);
    // 设置是否游泳

    virtual bool SetCanFly(bool enable, bool packetOnly = false);
    // 设置是否可以飞行

    virtual bool SetWaterWalking(bool enable, bool packetOnly = false);
    // 设置是否可以水上行走

    virtual bool SetFeatherFall(bool enable, bool packetOnly = false);
    // 设置是否可以羽毛飘落

    virtual bool SetHover(bool enable, bool packetOnly = false, bool updateAnimationTier = true);
    // 设置是否可以悬停

    MotionMaster* GetMotionMaster() { return i_motionMaster; }
    // 获取运动控制器

    [[nodiscard]] const MotionMaster* GetMotionMaster() const { return i_motionMaster; }

    [[nodiscard]] virtual MovementGeneratorType GetDefaultMovementType() const;
    // 获取默认的移动类型（虚函数，可被覆盖）

    [[nodiscard]] bool IsStopped() const { return !(HasUnitState(UNIT_STATE_MOVING)); }
    // 判断是否停止移动

    void StopMoving();
    // 停止移动

    void StopMovingOnCurrentPos();
    // 在当前位置停止移动

    virtual void PauseMovement(uint32 timer = 0, uint8 slot = 0); // timer in ms
    // 暂停移动

    void ResumeMovement(uint32 timer = 0, uint8 slot = 0);
    // 恢复移动

    void SetControlled(bool apply, UnitState state, Unit* source = nullptr, bool isFear = false);
    // 设置是否被控制

    void DisableRotate(bool apply);
    // 禁用旋转

    void DisableSpline();
    // 禁用样

    /*********************************************************/
    /***                  MISC METHODS                     ***/
    /*********************************************************/
    // SheathState
    [[nodiscard]] SheathState GetSheath() const { return SheathState(GetByteValue(UNIT_FIELD_BYTES_2, 0)); } // 获取当前鞘状态
    virtual void SetSheath(SheathState sheathed) { SetByteValue(UNIT_FIELD_BYTES_2, 0, sheathed); } // 设置鞘状态

    // StandState
    [[nodiscard]] uint8 getStandState() const { return GetByteValue(UNIT_FIELD_BYTES_1, 0); } // 获取站立状态
    [[nodiscard]] bool IsSitState() const; // 判断是否为坐姿状态
    [[nodiscard]] bool IsStandState() const; // 判断是否为站立状态
    void SetStandState(uint8 state); // 设置站立状态

    void  SetStandFlags(uint8 flags) { SetByteFlag(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_VIS_FLAG, flags); } // 添加站立标志
    void  RemoveStandFlags(uint8 flags) { RemoveByteFlag(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_VIS_FLAG, flags); } // 移除站立标志

    // DeathState
    DeathState getDeathState() { return m_deathState; }; // 获取死亡状态
    virtual void setDeathState(DeathState s, bool despawn = false);           // 设置死亡状态，Creature/Player/Pet中重写

    [[nodiscard]] bool IsAlive() const { return (m_deathState == DeathState::Alive); }; // 判断是否存活
    [[nodiscard]] bool isDying() const { return (m_deathState == DeathState::JustDied); }; // 判断是否濒死
    [[nodiscard]] bool isDead() const { return (m_deathState == DeathState::Dead || m_deathState == DeathState::Corpse); }; // 判断是否死亡或尸体状态

    // Spell Aura helpers
    [[nodiscard]] bool HasGhostAura()               const { return HasAuraType(SPELL_AURA_GHOST); }; // 是否具有幽灵状态的增益
    [[nodiscard]] bool HasMountedAura()             const { return HasAuraType(SPELL_AURA_MOUNTED); }; // 是否具有骑乘状态的增益
    [[nodiscard]] bool HasWaterWalkAura()           const { return HasAuraType(SPELL_AURA_WATER_WALK); }; // 是否具有水上行走的增益
    [[nodiscard]] bool HasFeatherFallAura()         const { return HasAuraType(SPELL_AURA_FEATHER_FALL); }; // 是否具有羽毛降落的增益
    [[nodiscard]] bool HasHoverAura()               const { return HasAuraType(SPELL_AURA_HOVER); }; // 是否具有悬停的增益
    [[nodiscard]] bool HasFlyAura()                 const { return HasAuraType(SPELL_AURA_FLY); }; // 是否具有飞行的增益
    [[nodiscard]] bool HasSpiritOfRedemptionAura()  const { return HasAuraType(SPELL_AURA_SPIRIT_OF_REDEMPTION); }; // 是否具有救赎之魂的增益
    [[nodiscard]] bool HasPreventsFleeingAura()     const { return HasAuraType(SPELL_AURA_PREVENTS_FLEEING); }; // 是否具有防止逃跑的增益
    [[nodiscard]] bool HasPreventDurabilityLossAura()  const { return HasAuraType(SPELL_AURA_PREVENT_DURABILITY_LOSS); }; // 是否具有防止耐久损失的增益
    [[nodiscard]] bool HasPreventResurectionAura()  const { return HasAuraType(SPELL_AURA_PREVENT_RESURRECTION); }; // 是否具有防止复活的增益
    [[nodiscard]] bool HasTransformAura()           const { return HasAuraType(SPELL_AURA_TRANSFORM); }; // 是否具有变形的增益
    [[nodiscard]] bool HasInterruptRegenAura()      const { return HasAuraType(SPELL_AURA_INTERRUPT_REGEN); }; // 是否具有中断回血的增益
    [[nodiscard]] bool HasNoPVPCreditAura()         const { return HasAuraType(SPELL_AURA_NO_PVP_CREDIT); }; // 是否具有不获得PVP积分的增益
    [[nodiscard]] bool HasWaterBreathingAura()      const { return HasAuraType(SPELL_AURA_WATER_BREATHING); }; // 是否具有水下呼吸的增益
    [[nodiscard]] bool HasIgnoreHitDirectionAura()  const { return HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION); }; // 是否具有忽略攻击方向的增益
    [[nodiscard]] bool HasSpellMagnetAura()         const { return HasAuraType(SPELL_AURA_SPELL_MAGNET); }; // 是否具有法术磁铁的增益
    [[nodiscard]] bool HasOpenStableAura()          const { return HasAuraType(SPELL_AURA_OPEN_STABLE); }; // 是否具有打开马厩的增益
    [[nodiscard]] bool HasCloneCasterAura()         const { return HasAuraType(SPELL_AURA_CLONE_CASTER); }; // 是否具有克隆施法者的增益
    [[nodiscard]] bool HasReflectSpellsAura()       const { return HasAuraType(SPELL_AURA_REFLECT_SPELLS); }; // 是否具有反射法术的增益
    [[nodiscard]] bool HasDetectAmoreAura()         const { return HasAuraType(SPELL_AURA_DETECT_AMORE); }; // 是否具有探测爱情的增益
    [[nodiscard]] bool HasAllowOnlyAbilityAura()    const { return HasAuraType(SPELL_AURA_ALLOW_ONLY_ABILITY); }; // 是否具有仅允许特定能力的增益
    [[nodiscard]] bool HasPeriodicDummyAura()       const { return HasAuraType(SPELL_AURA_PERIODIC_DUMMY); }; // 是否具有周期性虚拟的增益
    [[nodiscard]] bool HasControlVehicleAura()      const { return HasAuraType(SPELL_AURA_CONTROL_VEHICLE); }; // 是否具有控制载具的增益
    [[nodiscard]] bool HasAOECharmAura()            const { return HasAuraType(SPELL_AURA_AOE_CHARM); }; // 是否具有范围魅惑的增益
    [[nodiscard]] bool HasDetectSpellsAura()        const { return HasAuraType(SPELL_AURA_DEFLECT_SPELLS); }; // 是否具有法术偏转的增益
    [[nodiscard]] bool HasPacifySilenceAura()       const { return HasAuraType(SPELL_AURA_MOD_PACIFY_SILENCE); } // 是否具有沉默与安抚的增益
    [[nodiscard]] bool HasSilenceAura()             const { return HasAuraType(SPELL_AURA_MOD_SILENCE); } // 是否具有沉默的增益
    [[nodiscard]] bool HasShapeshiftAura()          const { return HasAuraType(SPELL_AURA_MOD_SHAPESHIFT); } // 是否具有变形形态的增益
    [[nodiscard]] bool HasDecreaseSpeedAura()       const { return HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED); } // 是否具有减速的增益
    [[nodiscard]] bool HasPacifyAura()              const { return HasAuraType(SPELL_AURA_MOD_PACIFY); } // 是否具有安抚的增益
    [[nodiscard]] bool HasIgnoreTargetResistAura()  const { return HasAuraType(SPELL_AURA_MOD_IGNORE_TARGET_RESIST); } // 是否具有忽略目标抗性的增益
    [[nodiscard]] bool HasIncreaseMountedSpeedAura() const { return HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED); } // 是否具有骑乘加速的增益
    [[nodiscard]] bool HasIncreaseMountedFlightSpeedAura() const { return HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED); } // 是否具有骑乘飞行加速的增益
    [[nodiscard]] bool HasThreatAura()              const { return HasAuraType(SPELL_AURA_MOD_THREAT); } // 是否具有威胁修改的增益
    [[nodiscard]] bool HasAttackerSpellCritChanceAura() const { return HasAuraType(SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE); } // 是否具有攻击者法术暴击几率修改的增益
    [[nodiscard]] bool HasUnattackableAura()        const { return HasAuraType(SPELL_AURA_MOD_UNATTACKABLE); } // 是否具有不可攻击的增益
    [[nodiscard]] bool HasHealthRegenInCombatAura() const { return HasAuraType(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT); } // 是否具有战斗中生命恢复的增益
    [[nodiscard]] bool HasRegenDuringCombatAura()   const { return HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT); } // 是否具有战斗期间恢复的增益
    [[nodiscard]] bool HasFearAura()                const { return HasAuraType(SPELL_AURA_MOD_FEAR); } // 是否具有恐惧的增益
    [[nodiscard]] bool HasConfuseAura()             const { return HasAuraType(SPELL_AURA_MOD_CONFUSE); } // 是否具有混乱的增益
    [[nodiscard]] bool HasRootAura()                const { return HasAuraType(SPELL_AURA_MOD_ROOT); } // 是否具有定身的增益
    [[nodiscard]] bool HasStunAura()                const { return HasAuraType(SPELL_AURA_MOD_STUN); } // 是否具有眩晕的增益
    [[nodiscard]] bool HasTauntAura()               const { return HasAuraType(SPELL_AURA_MOD_TAUNT); } // 是否具有嘲讽的增益
    [[nodiscard]] bool HasStealthAura()             const { return HasAuraType(SPELL_AURA_MOD_STEALTH); } // 是否具有潜行的增益
    [[nodiscard]] bool HasStealthDetectAura()       const { return HasAuraType(SPELL_AURA_MOD_STEALTH_DETECT); } // 是否具有潜行探测的增益
    [[nodiscard]] bool HasInvisibilityAura()        const { return HasAuraType(SPELL_AURA_MOD_INVISIBILITY); } // 是否具有隐形的增益
    [[nodiscard]] bool HasInvisibilityDetectAura()  const { return HasAuraType(SPELL_AURA_MOD_INVISIBILITY_DETECT); } // 是否具有隐形探测的增益

    // React methods
    bool IsHostileTo(Unit const* unit) const; // 判断是否对指定单位敌对
    [[nodiscard]] bool IsHostileToPlayers() const; // 判断是否对所有玩家敌对
    bool IsFriendlyTo(Unit const* unit) const; // 判断是否对指定单位友好
    [[nodiscard]] bool IsNeutralToAll() const; // 判断是否对所有人中立

    // Reactive attacks
    void ClearAllReactives(); // 清除所有反应性攻击
    void StartReactiveTimer(ReactiveType reactive) { m_reactiveTimer[reactive] = REACTIVE_TIMER_START; } // 开始反应性计时器
    void UpdateReactives(uint32 p_time); // 更新反应性攻击

    // Diminish returns system
    DiminishingLevels GetDiminishing(DiminishingGroup group); // 获取指定分组的递减等级
    void IncrDiminishing(DiminishingGroup group); // 增加指定分组的递减等级
    float ApplyDiminishingToDuration(DiminishingGroup group, int32& duration, Unit* caster, DiminishingLevels Level, int32 limitduration); // 应用递减效果到持续时间
    void ApplyDiminishingAura(DiminishingGroup group, bool apply); // 应用或移除递减增益
    void ClearDiminishings() { m_Diminishing.clear(); } // 清除所有递减状态

    // Group methods
    bool IsInPartyWith(Unit const* unit) const; // 判断是否与指定单位在同一个队伍中
    bool IsInRaidWith(Unit const* unit) const; // 判断是否与指定单位在同一个团队中
    void GetPartyMembers(std::list<Unit*>& units); // 获取队伍成员列表
    Unit* GetNextRandomRaidMemberOrPet(float radius); // 获取随机的团队成员或宠物
    void UpdateAuraForGroup(uint8 slot); // 更新队伍中的增益效果

    // Reputations system
    ReputationRank GetReactionTo(Unit const* target, bool checkOriginalFaction = false) const; // 获取对目标的声望反应
    ReputationRank GetFactionReactionTo(FactionTemplateEntry const* factionTemplateEntry, Unit const* target) const; // 获取指定阵营模板对目标的反应

    // Shared vision
    SharedVisionList const& GetSharedVisionList() { return m_sharedVision; } // 获取共享视野列表
    void AddPlayerToVision(Player* player); // 将玩家添加到共享视野中
    void RemovePlayerFromVision(Player* player); // 从共享视野中移除玩家
    [[nodiscard]] bool HasSharedVision() const { return !m_sharedVision.empty(); } // 判断是否具有共享视野

    // Virtual items
    uint32 GetVirtualItemId(uint32 slot) const; // 获取虚拟物品ID
    void SetVirtualItem(uint32 slot, uint32 itemId); // 设置虚拟物品

    // Mount methods
    [[nodiscard]] bool IsMounted() const { return HasUnitFlag(UNIT_FLAG_MOUNT); } // 判断是否处于骑乘状态
    [[nodiscard]] uint32 GetMountID() const { return GetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID); } // 获取骑乘显示ID
    void Mount(uint32 mount, uint32 vehicleId = 0, uint32 creatureEntry = 0); // 骑乘指定生物
    void Dismount(); // 下马
    [[nodiscard]] bool IsInDisallowedMountForm() const; // 判断是否处于不允许的骑乘形态

    // Followers
    void addFollower(FollowerReference* pRef) { m_FollowingRefMgr.insertFirst(pRef); } // 添加跟随者引用
    void removeFollower(FollowerReference* /*pRef*/) { /* nothing to do yet */ } // 移除跟随者引用
    [[nodiscard]] virtual float GetFollowAngle() const { return static_cast<float>(M_PI / 2); } // 获取跟随角度

    // Pets, guardians, minions...
    [[nodiscard]] Guardian* GetGuardianPet() const; // 获取守护宠物
    [[nodiscard]] Minion* GetFirstMinion() const; // 获取第一个随从
    [[nodiscard]] Creature* GetCompanionPet() const; // 获取同伴宠物

    
    Pet* CreateTamedPetFrom(Creature* creatureTarget, uint32 spell_id = 0); // 从目标生物创建驯服的宠物
    Pet* CreateTamedPetFrom(uint32 creatureEntry, uint32 spell_id = 0); // 从生物条目创建驯服的宠物
    bool InitTamedPet(Pet* pet, uint8 level, uint32 spell_id); // 初始化驯服的宠物

    void SetMinion(Minion* minion, bool apply); // 设置随从
    void GetAllMinionsByEntry(std::list<Creature*>& Minions, uint32 entry); // 获取特定条目的所有随从
    void RemoveAllMinionsByEntry(uint32 entry); // 移除特定条目的所有随从

    void AddPetAura(PetAura const* petSpell); // 添加宠物增益
    void RemovePetAura(PetAura const* petSpell); // 移除宠物增益
    void CastPetAura(PetAura const* aura); // 施放宠物增益
    bool IsPetAura(Aura const* aura); // 判断是否为宠物增益

    void PetSpellFail(SpellInfo const* spellInfo, Unit* target, uint32 result); // 宠物法术失败处理

    void UnsummonAllTotems(bool onDeath = false); // 召唤所有图腾

    // Vehicules
    [[nodiscard]] TransportBase* GetDirectTransport() const;    /// 返回当前单位所在的运输工具（如果是载具和运输工具，返回载具）

    bool CreateVehicleKit(uint32 id, uint32 creatureEntry); // 创建载具套件
    void RemoveVehicleKit(); // 移除载具套件
    [[nodiscard]] Vehicle* GetVehicleKit()const { return m_vehicleKit; } // 获取载具套件
    [[nodiscard]] Vehicle* GetVehicle()   const { return m_vehicle; } // 获取载具
    bool IsOnVehicle(Unit const* vehicle) const { return m_vehicle && m_vehicle == vehicle->GetVehicleKit(); } // 判断是否在指定载具上
    [[nodiscard]] Unit* GetVehicleBase()  const; // 获取载具的基础单位
    [[nodiscard]] Creature* GetVehicleCreatureBase() const; // 获取载具的基础生物

    void EnterVehicle(Unit* base, int8 seatId = -1); // 进入载具
    void EnterVehicleUnattackable(Unit* base, int8 seatId = -1); // 以不可攻击状态进入载具
    void ExitVehicle(Position const* exitPosition = nullptr); // 离开载具
    void ChangeSeat(int8 seatId, bool next = true); // 更换座位

    // Should only be called by AuraEffect::HandleAuraControlVehicle(AuraApplication const* auraApp, uint8 mode, bool apply) const;
    bool HandleSpellClick(Unit* clicker, int8 seatId = -1); // 处理点击载具
    void _ExitVehicle(Position const* exitPosition = nullptr); // 强制离开载具
    void _EnterVehicle(Vehicle* vehicle, int8 seatId, AuraApplication const* aurApp = nullptr); // 强制进入载具

    // Emote
    void HandleEmoteCommand(uint32 emoteId); // 处理表情命令

    // Chat
    virtual void Talk(std::string_view text, ChatMsg msgType, Language language, float textRange, WorldObject const* target); // 发送聊天信息
    virtual void Say(std::string_view text, Language language, WorldObject const* target = nullptr); // 说话
    virtual void Yell(std::string_view text, Language language, WorldObject const* target = nullptr); // 大喊
    virtual void TextEmote(std::string_view text, WorldObject const* target = nullptr, bool isBossEmote = false); // 文本表情
    virtual void Whisper(std::string_view text, Language language, Player* target, bool isBossWhisper = false); // 耳语
    virtual void Talk(uint32 textId, ChatMsg msgType, float textRange, WorldObject const* target); // 发送聊天信息（通过文本ID）
    virtual void Say(uint32 textId, WorldObject const* target = nullptr); // 说话（通过文本ID）
    virtual void Yell(uint32 textId, WorldObject const* target = nullptr); // 大喊（通过文本ID）
    virtual void TextEmote(uint32 textId, WorldObject const* target = nullptr, bool isBossEmote = false); // 文本表情（通过文本ID）
    virtual void Whisper(uint32 textId, Player* target, bool isBossWhisper = false); // 耳语（通过文本ID）

    // ShapeShitForm (use by druid)
    [[nodiscard]] ShapeshiftForm GetShapeshiftForm() const { return ShapeshiftForm(GetByteValue(UNIT_FIELD_BYTES_2, 3)); } // 获取变形形态
    void SetShapeshiftForm(ShapeshiftForm form); // 设置变形形态
    bool IsAttackSpeedOverridenShapeShift() const; // 判断是否覆盖了攻击速度的变形形态
    [[nodiscard]] bool IsInFeralForm() const
    {
        ShapeshiftForm form = GetShapeshiftForm();
        return form == FORM_CAT || form == FORM_BEAR || form == FORM_DIREBEAR || form == FORM_GHOSTWOLF; // Xinef: added shamans Ghost Wolf, should behave exactly like druid forms
    }

    // Unit transform
    void setTransForm(uint32 spellid) { m_transform = spellid; } // 设置变形法术ID
    [[nodiscard]] uint32 getTransForm() const { return m_transform; } // 获取变形法术ID
    void DeMorph(); // 取消变形

    // Unit models
    virtual float GetNativeObjectScale() const { return 1.0f; } // 获取原生对象缩放比例
    virtual void RecalculateObjectScale(); // 重新计算对象缩放比例
    [[nodiscard]] uint32 GetDisplayId() const { return GetUInt32Value(UNIT_FIELD_DISPLAYID); } // 获取显示ID
    virtual void SetDisplayId(uint32 modelId, float displayScale = 1.f); // 设置显示模型ID
    [[nodiscard]] uint32 GetNativeDisplayId() const { return GetUInt32Value(UNIT_FIELD_NATIVEDISPLAYID); } // 获取原生显示ID
    void RestoreDisplayId(); // 恢复显示ID
    void SetNativeDisplayId(uint32 displayId) { SetUInt32Value(UNIT_FIELD_NATIVEDISPLAYID, displayId); } // 设置原生显示ID

    [[nodiscard]] uint32 GetModelForForm(ShapeshiftForm form, uint32 spellId); // 获取特定变形形态的模型

    // Unit positons
    [[nodiscard]] virtual bool IsInWater() const; // 判断是否在水中
    [[nodiscard]] virtual bool IsUnderWater() const; // 判断是否在水面下

    void NearTeleportTo(Position& pos, bool casting = false, bool vehicleTeleport = false, bool withPet = false, bool removeTransport = false); // 传送到附近位置
    void NearTeleportTo(float x, float y, float z, float orientation, bool casting = false, bool vehicleTeleport = false, bool withPet = false, bool removeTransport = false); // 传送到附近坐标

    void SetInFront(WorldObject const* target); // 设置面对目标
    void SetFacingTo(float ori); // 设置朝向
    void SetFacingToObject(WorldObject* object); // 设置朝向指定对象
    void SetTimedFacingToObject(WorldObject* object, uint32 time); // 在指定时间后重置朝向

    bool isInAccessiblePlaceFor(Creature const* c) const; // 判断位置是否对指定生物可访问
    bool isInFrontInMap(Unit const* target, float distance, float arc = M_PI) const; // 判断目标是否在前方
    bool isInBackInMap(Unit const* target, float distance, float arc = M_PI) const; // 判断目标是否在后方

    [[nodiscard]] float GetCollisionHeight() const override; // 获取碰撞高度
    [[nodiscard]] float GetCollisionWidth() const override; // 获取碰撞宽度
    [[nodiscard]] float GetCollisionRadius() const override; // 获取碰撞半径

    void UpdateOrientation(float orientation); // 更新朝向

    void UpdateHeight(float newZ); // 更新高度

    virtual bool UpdatePosition(float x, float y, float z, float ang, bool teleport = false); // 更新位置
    bool UpdatePosition(const Position& pos, bool teleport = false) { return UpdatePosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), teleport); } // 通过Position更新位置

    void ProcessPositionDataChanged(PositionFullTerrainStatus const& data) override; // 处理位置数据变更
    virtual void ProcessTerrainStatusUpdate(); // 处理地形状态更新

    // Visibility & Phase system
    [[nodiscard]] bool IsVisible() const { return m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GM) <= SEC_PLAYER; } // 判断是否可见
    void SetVisible(bool x); // 设置可见性
    void SetModelVisible(bool on); // 设置模型可见性
    [[nodiscard]] uint32 GetPhaseByAuras() const; // 获取由增益决定的阶段
    void SetPhaseMask(uint32 newPhaseMask, bool update) override;// overwrite WorldObject::SetPhaseMask // 设置阶段掩码
    void UpdateObjectVisibility(bool forced = true, bool fromUpdate = false) override; // 更新对象可见性

    // Pointers
    void AddPointedBy(SafeUnitPointer* sup) { SafeUnitPointerSet.insert(sup); } // 添加指向此Unit的SafeUnitPointer
    void RemovePointedBy(SafeUnitPointer* sup) { SafeUnitPointerSet.erase(sup); } // 移除指向此Unit的SafeUnitPointer
    static void HandleSafeUnitPointersOnDelete(Unit* thisUnit); // 在Unit删除时处理SafeUnitPointer

    // Senders methods
    void SendAttackStateUpdate(CalcDamageInfo* damageInfo); // 发送攻击状态更新
    void SendAttackStateUpdate(uint32 HitInfo, Unit* target, uint8 SwingType, SpellSchoolMask damageSchoolMask, uint32 Damage, uint32 AbsorbDamage, uint32 Resist, VictimState TargetState, uint32 BlockedAmount); // 发送攻击状态更新

    void SendComboPoints(); // 发送连击点数

    void SendPlaySpellVisual(uint32 id); // 播放法术视觉效果
    void SendPlaySpellImpact(ObjectGuid guid, uint32 id); // 发送法术命中效果

    void SendPetActionFeedback(uint8 msg); // 发送宠物动作反馈
    void SendPetTalk(uint32 pettalk); // 发送宠物对话
    void SendPetAIReaction(ObjectGuid guid); // 发送宠物AI反应

    void SendPeriodicAuraLog(SpellPeriodicAuraLogInfo* pInfo); // 发送周期性增益日志

    void SendSpellNonMeleeDamageLog(SpellNonMeleeDamage* log); // 发送非近战法术伤害日志
    void SendSpellNonMeleeReflectLog(SpellNonMeleeDamage* log, Unit* attacker); // 发送反射非近战法术伤害日志
    void SendSpellNonMeleeDamageLog(Unit* target, SpellInfo const* spellInfo, uint32 Damage, SpellSchoolMask damageSchoolMask, uint32 AbsorbedDamage, uint32 Resist, bool PhysicalDamage, uint32 Blocked, bool CriticalHit = false, bool Split = false); // 发送非近战法术伤害日志
    void SendSpellMiss(Unit* target, uint32 spellID, SpellMissInfo missInfo); // 发送法术未命中信息
    void SendSpellDamageResist(Unit* target, uint32 spellId); // 发送法术抵抗信息
    void SendSpellDamageImmune(Unit* target, uint32 spellId); // 发送法术免疫信息

    void SendTameFailure(uint8 result); // 发送驯服失败信息

    void SendTeleportPacket(Position& pos); // 发送传送数据包

    void SendMovementFlagUpdate(bool self = false); // 发送移动标志更新
    void SendMovementWaterWalking(Player* sendTo); // 发送水上行走移动数据
    void SendMovementFeatherFall(Player* sendTo); // 发送羽毛降落移动数据
    void SendMovementHover(Player* sendTo); // 发送悬停移动数据

    void SendChangeCurrentVictimOpcode(HostileReference* pHostileReference); // 发送切换当前受害者操作码
    void SendClearThreatListOpcode(); // 发送清空威胁列表操作码
    void SendRemoveFromThreatListOpcode(HostileReference* pHostileReference); // 发送从威胁列表中移除操作码
    void SendThreatListUpdate(); // 发送威胁列表更新
    void SendClearTarget(); // 发送清除目标

    // Misc functions
    void ExecuteDelayedUnitRelocationEvent(); // 执行延迟的单位重新定位事件
    void ExecuteDelayedUnitAINotifyEvent(); // 执行延迟的单位AI通知事件

    void BuildHeartBeatMsg(WorldPacket* data) const; // 构建心跳消息
    void BuildMovementPacket(ByteBuffer* data) const; // 构建移动数据包

    // Debug
    void OutDebugInfo() const; // 输出调试信息
    std::string GetDebugInfo() const override; // 获取调试信息

    //----------- Public variables ----------//
    uint32 m_extraAttacks; // 额外攻击次数
    bool m_canDualWield; // 是否可以双持武器

    ControlSet m_Controlled; // 被控制的单位集合

    SafeUnitPointer m_movedByPlayer; // 移动此单位的玩家指针

    ObjectGuid m_SummonSlot[MAX_SUMMON_SLOT]; // 召唤槽
    ObjectGuid m_ObjectSlot[MAX_GAMEOBJECT_SLOT]; // 游戏对象槽

    float m_modMeleeHitChance; // 近战命中几率修正
    float m_modRangedHitChance; // 远程命中几率修正
    float m_modSpellHitChance; // 法术命中几率修正
    int32 m_baseSpellCritChance; // 基础法术暴击几率

    float m_threatModifier[MAX_SPELL_SCHOOL]; // 威胁修正
    float m_modAttackSpeedPct[3]; // 攻击速度百分比修正

    SpellImmuneList m_spellImmune[MAX_SPELL_IMMUNITY]; // 法术免疫列表
    uint32 m_lastSanctuaryTime; // 上次圣所时间

    // pet auras
    typedef std::set<PetAura const*> PetAuraSet; // 宠物增益集合
    PetAuraSet m_petAuras; // 宠物增益

    bool IsAIEnabled; // AI是否启用
    bool NeedChangeAI; // 是否需要更换AI

    bool m_ControlledByPlayer; // 是否由玩家控制
    bool m_CreatedByPlayer; // 是否由玩家创建

    // Safe mover
    std::set<SafeUnitPointer*> SafeUnitPointerSet; // 安全单位指针集合

    // Relocation Nofier optimization
    Position m_last_notify_position; // 上次通知的位置
    uint32 m_last_notify_mstime; // 上次通知的时间
    uint16 m_delayed_unit_relocation_timer; // 延迟单位重新定位计时器
    uint16 m_delayed_unit_ai_notify_timer; // 延迟单位AI通知计时器
    bool bRequestForcedVisibilityUpdate; // 请求强制可见性更新

    // Movement info
    Movement::MoveSpline* movespline; // 移动样条

protected:
    explicit Unit(bool isWorldObject); // 构造函数

    void BuildValuesUpdate(uint8 updateType, ByteBuffer* data, Player* target) override; // 构建值更新数据

    void _UpdateSpells(uint32 time); // 更新法术
    void _DeleteRemovedAuras(); // 删除移除的增益

    void _UpdateAutoRepeatSpell(); // 更新自动重复法术

    bool CanSparringWith(Unit const* attacker) const;   ///@brief: 检查单位是否适合进行训练伤害。仅当攻击者和受害者都是生物时才有效。

    bool IsAlwaysVisibleFor(WorldObject const* seer) const override; // 判断是否始终对观察者可见
    bool IsAlwaysDetectableFor(WorldObject const* seer) const override; // 判断是否始终对观察者可探测

    void SetFeared(bool apply, Unit* fearedBy = nullptr, bool isFear = false); // 设置恐惧状态
    void SetConfused(bool apply); // 设置混乱状态
    void SetStunned(bool apply); // 设置眩晕状态
    void SetRooted(bool apply, bool isStun = false); // 设置定身状态

    //----------- Protected variables ----------//
    UnitAI* i_AI; // AI指针
    UnitAI* i_disabledAI; // 禁用的AI指针

    uint8 m_realRace; // 真实种族
    uint8 m_race; // 种族

    bool m_AutoRepeatFirstCast; // 是否是自动重复的第一个施法

    int32 m_attackTimer[MAX_ATTACK]; // 攻击计时器

    float m_createStats[MAX_STATS]; // 创建时的属性

    AttackerSet m_attackers; // 攻击者集合
    Unit* m_attacking; // 当前攻击的目标

    DeathState m_deathState; // 死亡状态

    int32 m_procDeep; // 处理深度

    typedef std::list<DynamicObject*> DynObjectList; // 动态对象列表
    DynObjectList m_dynObj; // 动态对象

    typedef GuidList GameObjectList; // 游戏对象列表
    GameObjectList m_gameObj; // 游戏对象
    uint32 m_transform; // 变形法术ID

    Spell* m_currentSpells[CURRENT_MAX_SPELL]; // 当前施放的法术

    AuraMap m_ownedAuras; // 拥有的增益
    AuraApplicationMap m_appliedAuras; // 应用的增益
    AuraList m_removedAuras; // 被移除的增益
    AuraMap::iterator m_auraUpdateIterator; // 增益更新迭代器
    uint32 m_removedAurasCount; // 被移除的增益数量

    AuraEffectList m_modAuras[TOTAL_AURAS]; // 修改类型的增益效果
    AuraList m_scAuras;                        // 单次施放的增益
    AuraApplicationList m_interruptableAuras;  // 可中断的增益
    AuraStateAurasMap m_auraStateAuras;        // 用于提升增益状态检查性能
    uint32 m_interruptMask; // 中断掩码

    float m_auraModifiersGroup[UNIT_MOD_END][MODIFIER_TYPE_END]; // 增益修饰符组
    float m_weaponDamage[MAX_ATTACK][MAX_WEAPON_DAMAGE_RANGE][MAX_ITEM_PROTO_DAMAGES]; // 武器伤害
    bool m_canModifyStats; // 是否可以修改属性
    VisibleAuraMap m_visibleAuras; // 可见增益

    float m_speed_rate[MAX_MOVE_TYPE]; // 移动速度比率

    CharmInfo* m_charmInfo; // 魅惑信息
    SharedVisionList m_sharedVision; // 共享视野列表

    MotionMaster* i_motionMaster; // 动作管理器

    uint32 m_reactiveTimer[MAX_REACTIVE]; // 反应性计时器
    int32 m_regenTimer; // 回复计时器

    ThreatMgr m_ThreatMgr; // 威胁管理器
    typedef std::map<ObjectGuid, float> CharmThreatMap; // 魅惑威胁映射
    CharmThreatMap _charmThreatInfo; // 魅惑威胁信息

    Vehicle* m_vehicle; // 载具
    Vehicle* m_vehicleKit; // 载具套件

    uint32 m_unitTypeMask; // 单位类型掩码
    LiquidTypeEntry const* _lastLiquid; // 最后一次进入的液体类型

    // xinef: apply resilience
    bool m_applyResilience; // 是否应用韧性
    bool _instantCast; // 是否为瞬发

    uint32 m_rootTimes; // 定身次数

private:
    bool IsTriggeredAtSpellProcEvent(Unit* victim, Aura* aura, WeaponAttackType attType, bool isVictim, bool active, SpellProcEventEntry const*& spellProcEvent, ProcEventInfo const& eventInfo); // 判断是否触发法术触发事件
    bool HandleDummyAuraProc(Unit* victim, uint32 damage, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 procFlag, uint32 procEx, uint32 cooldown, ProcEventInfo const& eventInfo); // 处理虚拟增益触发
    bool HandleAuraProc(Unit* victim, uint32 damage, Aura* triggeredByAura, SpellInfo const* procSpell, uint32 procFlag, uint32 procEx, uint32 cooldown, bool* handled); // 处理增益触发
    bool HandleProcTriggerSpell(Unit* victim, uint32 damage, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 procFlag, uint32 procEx, uint32 cooldown, uint32 procPhase, ProcEventInfo& eventInfo); // 处理触发法术
    bool HandleOverrideClassScriptAuraProc(Unit* victim, uint32 damage, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 cooldown); // 处理覆盖类脚本增益触发
    bool HandleAuraRaidProcFromChargeWithValue(AuraEffect* triggeredByAura); // 处理充能触发的团队增益
    bool HandleAuraRaidProcFromCharge(AuraEffect* triggeredByAura); // 处理充能触发的团队增益

    void UpdateSplineMovement(uint32 t_diff); // 更新样条移动
    void UpdateSplinePosition(); // 更新样条位置

    // player or player's pet
    [[nodiscard]] float GetCombatRatingReduction(CombatRating cr) const; // 获取战斗评级减少
    [[nodiscard]] uint32 GetCombatRatingDamageReduction(CombatRating cr, float rate, float cap, uint32 damage) const; // 获取战斗评级造成的伤害减少

    void PatchValuesUpdate(ByteBuffer& valuesUpdateBuf, BuildValuesCachePosPointers& posPointers, Player* target); // 修补值更新数据
    void InvalidateValuesUpdateCache() { _valuesUpdateCache.clear(); } // 使值更新缓存失效

    [[nodiscard]] float processDummyAuras(float TakenTotalMod) const; // 处理虚拟增益

    void _addAttacker(Unit* pAttacker) { m_attackers.insert(pAttacker); }   ///@note: 仅在Unit::Attack()中调用
    void _removeAttacker(Unit* pAttacker) { m_attackers.erase(pAttacker); } ///@note: 仅在Unit::AttackStop()中调用

    //----------- Private variables ----------//
    uint32 m_state;                                     // 状态，子类不应修改
    uint32 m_CombatTimer; // 战斗计时器
    uint32 m_lastManaUse;                               // 上次使用魔法的时间（毫秒）
    //TimeTrackerSmall m_movesplineTimer;

    Diminishing m_Diminishing; // 递减系统
    // Manage all Units that are threatened by us
    HostileRefMgr m_HostileRefMgr; // 管理所有被我们威胁的单位

    FollowerRefMgr m_FollowingRefMgr; // 跟随者引用管理器

    Unit* m_comboTarget; // 连击目标
    int8 m_comboPoints; // 连击点数
    std::unordered_set<Unit*> m_ComboPointHolders; // 持有连击点数的单位集合

    RedirectThreatInfo _redirectThreatInfo; // 威胁重定向信息

    bool m_cleanupDone; // 锁定以防止在清理后添加内容
    bool m_duringRemoveFromWorld; // 锁定以防止在开始从世界移除后添加内容

    uint32 _oldFactionId;           ///< 魅惑前的阵营ID
    bool _isWalkingBeforeCharm;     ///< 魅惑前是否在行走？

    uint32 _lastExtraAttackSpell; // 上次额外攻击的法术
    std::unordered_map<ObjectGuid /*guid*/, uint32 /*count*/> extraAttacksTargets; // 额外攻击目标
    ObjectGuid _lastDamagedTargetGuid; // 上次受到伤害的目标GUID

    typedef std::unordered_map<uint64 /*visibleFlag(uint32) + updateType(uint8)*/, BuildValuesCachedBuffer>  ValuesUpdateCache;
    ValuesUpdateCache _valuesUpdateCache; // 值更新缓存

};

namespace Acore
{
    // Binary predicate for sorting Units based on percent value of a power
    class PowerPctOrderPred
    {
    public:
        PowerPctOrderPred(Powers power, bool ascending = true) : _power(power), _ascending(ascending) {}

        bool operator()(WorldObject const* objA, WorldObject const* objB) const
        {
            Unit const* a = objA->ToUnit();
            Unit const* b = objB->ToUnit();
            float rA = (a && a->GetMaxPower(_power)) ? float(a->GetPower(_power)) / float(a->GetMaxPower(_power)) : 0.0f;
            float rB = (b && b->GetMaxPower(_power)) ? float(b->GetPower(_power)) / float(b->GetMaxPower(_power)) : 0.0f;
            return _ascending ? rA < rB : rA > rB;
        }

        bool operator()(Unit const* a, Unit const* b) const
        {
            float rA = a->GetMaxPower(_power) ? float(a->GetPower(_power)) / float(a->GetMaxPower(_power)) : 0.0f;
            float rB = b->GetMaxPower(_power) ? float(b->GetPower(_power)) / float(b->GetMaxPower(_power)) : 0.0f;
            return _ascending ? rA < rB : rA > rB;
        }

    private:
        Powers const _power;
        bool const _ascending;
    };

    // Binary predicate for sorting Units based on percent value of health
    class HealthPctOrderPred
    {
    public:
        HealthPctOrderPred(bool ascending = true) : _ascending(ascending) {}

        bool operator()(WorldObject const* objA, WorldObject const* objB) const
        {
            Unit const* a = objA->ToUnit();
            Unit const* b = objB->ToUnit();
            float rA = (a && a->GetMaxHealth()) ? float(a->GetHealth()) / float(a->GetMaxHealth()) : 0.0f;
            float rB = (b && b->GetMaxHealth()) ? float(b->GetHealth()) / float(b->GetMaxHealth()) : 0.0f;
            return _ascending ? rA < rB : rA > rB;
        }

        bool operator() (Unit const* a, Unit const* b) const
        {
            float rA = a->GetMaxHealth() ? float(a->GetHealth()) / float(a->GetMaxHealth()) : 0.0f;
            float rB = b->GetMaxHealth() ? float(b->GetHealth()) / float(b->GetMaxHealth()) : 0.0f;
            return _ascending ? rA < rB : rA > rB;
        }

    private:
        bool const _ascending;
    };
}

class RedirectSpellEvent : public BasicEvent
{
public:
    RedirectSpellEvent(Unit& self, ObjectGuid auraOwnerGUID, AuraEffect* auraEffect) : _self(self), _auraOwnerGUID(auraOwnerGUID), _auraEffect(auraEffect) {}
    bool Execute(uint64 e_time, uint32 p_time) override;

protected:
    Unit& _self;
    ObjectGuid _auraOwnerGUID;
    AuraEffect* _auraEffect;
};
#endif
