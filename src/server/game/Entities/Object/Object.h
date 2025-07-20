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

#ifndef _OBJECT_H
#define _OBJECT_H

#include "AreaDefines.h"
#include "Common.h"
#include "DataMap.h"
#include "EventProcessor.h"
#include "G3D/Vector3.h"
#include "GridDefines.h"
#include "GridReference.h"
#include "Map.h"
#include "ModelIgnoreFlags.h"
#include "ObjectDefines.h"
#include "ObjectGuid.h"
#include "Optional.h"
#include "Position.h"
#include "UpdateData.h"
#include "UpdateMask.h"
#include <memory>
#include <set>
#include <sstream>
#include <string>

#include "UpdateFields.h"
// 前置声明 ElunaEventProcessor 类
class ElunaEventProcessor;

// 临时召唤物类型枚举，定义不同类型的临时召唤物消失条件
enum TempSummonType
{
    // 在指定时间后消失，或当生物消失时消失
    TEMPSUMMON_TIMED_OR_DEAD_DESPAWN       = 1,             // despawns after a specified time OR when the creature disappears
    // 在指定时间后消失，或当生物死亡时消失
    TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN     = 2,             // despawns after a specified time OR when the creature dies
    // 在指定时间后消失
    TEMPSUMMON_TIMED_DESPAWN               = 3,             // despawns after a specified time
    // 在生物脱离战斗后指定时间消失
    TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT = 4,             // despawns after a specified time after the creature is out of combat
    // 在生物死亡后立即消失
    TEMPSUMMON_CORPSE_DESPAWN              = 5,             // despawns instantly after death
    // 在生物死亡后指定时间消失
    TEMPSUMMON_CORPSE_TIMED_DESPAWN        = 6,             // despawns after a specified time after death
    // 当生物消失时消失
    TEMPSUMMON_DEAD_DESPAWN                = 7,             // despawns when the creature disappears
    // 当调用 UnSummon() 时消失
    TEMPSUMMON_MANUAL_DESPAWN              = 8,             // despawns when UnSummon() is called
    // xinef: 仅内部使用，请勿使用
    TEMPSUMMON_DESPAWNED                   = 9,             // xinef: DONT USE, INTERNAL USE ONLY
    // 在生物脱离战斗且存活的状态下指定时间后消失
    TEMPSUMMON_TIMED_DESPAWN_OOC_ALIVE     = 10,            // despawns after a specified time after the creature is out of combat and alive
};

// 相位掩码枚举，定义不同的相位状态
enum PhaseMasks
{
    // 正常相位
    PHASEMASK_NORMAL   = 0x00000001,
    // 任意相位
    PHASEMASK_ANYWHERE = 0xFFFFFFFF
};

// 通知标志枚举，定义不同的通知类型
enum NotifyFlags
{
    // 无通知
    NOTIFY_NONE                     = 0x00,
    // AI 重新定位通知
    NOTIFY_AI_RELOCATION            = 0x01,
    // 可见性改变通知
    NOTIFY_VISIBILITY_CHANGED       = 0x02,
    // 所有通知
    NOTIFY_ALL                      = 0xFF
};

// 游戏对象召唤类型枚举，定义不同类型的游戏对象召唤物消失条件
enum GOSummonType
{
    // 在指定时间后消失，或当召唤者死亡时消失
    GO_SUMMON_TIMED_OR_CORPSE_DESPAWN = 0,      // despawns after a specified time OR when the summoner dies
    // 在指定时间后消失
    GO_SUMMON_TIMED_DESPAWN = 1                 // despawns after a specified time
};

// 前置声明各类类
class WorldPacket;
class UpdateData;
class ByteBuffer;
class WorldSession;
class Creature;
class Player;
class InstanceScript;
class GameObject;
class TempSummon;
class Vehicle;
class CreatureAI;
class ZoneScript;
class Unit;
class Transport;
class StaticTransport;
class MotionTransport;

// 前置声明 PositionFullTerrainStatus 结构体
struct PositionFullTerrainStatus;

// 定义 UpdateDataMapType 类型，用于存储玩家和其对应的更新数据的映射
typedef std::unordered_map<Player*, UpdateData> UpdateDataMapType;
// 定义 UpdatePlayerSet 类型，用于存储玩家的 GUID 集合
typedef GuidUnorderedSet UpdatePlayerSet;

// 定义心跳间隔常量
static constexpr Milliseconds HEARTBEAT_INTERVAL = 5s + 200ms;

// 基类 Object，代表游戏中的各种对象
class Object
{
public:
    // 虚析构函数，确保正确释放派生类对象
    virtual ~Object();

    // 判断对象是否在世界中
    [[nodiscard]] bool IsInWorld() const { return m_inWorld; }

    // 虚函数，将对象添加到世界中
    virtual void AddToWorld();
    // 虚函数，将对象从世界中移除
    virtual void RemoveFromWorld();

    // 获取对象的 GUID，如果对象为空则返回空 GUID
    [[nodiscard]] static ObjectGuid GetGUID(Object const* o) { return o ? o->GetGUID() : ObjectGuid::Empty; }
    // 获取当前对象的 GUID
    [[nodiscard]] ObjectGuid GetGUID() const { return GetGuidValue(OBJECT_FIELD_GUID); }
    // 获取当前对象打包后的 GUID
    [[nodiscard]] PackedGuid const& GetPackGUID() const { return m_PackGUID; }
    // 获取对象的条目 ID
    [[nodiscard]] uint32 GetEntry() const { return GetUInt32Value(OBJECT_FIELD_ENTRY); }
    // 设置对象的条目 ID
    void SetEntry(uint32 entry) { SetUInt32Value(OBJECT_FIELD_ENTRY, entry); }

    // 获取对象的缩放比例
    [[nodiscard]] float GetObjectScale() const { return GetFloatValue(OBJECT_FIELD_SCALE_X); }
    // 虚函数，设置对象的缩放比例
    virtual void SetObjectScale(float scale) { SetFloatValue(OBJECT_FIELD_SCALE_X, scale); }

    // 虚函数，获取对象的动态标志
    virtual uint32 GetDynamicFlags() const { return 0; }
    // 判断对象是否具有指定的动态标志
    bool HasDynamicFlag(uint32 flag) const { return (GetDynamicFlags() & flag) != 0; }
    // 虚函数，设置对象的动态标志
    virtual void SetDynamicFlag(uint32 flag) { ReplaceAllDynamicFlags(GetDynamicFlags() | flag); }
    // 虚函数，移除对象的指定动态标志
    virtual void RemoveDynamicFlag(uint32 flag) { ReplaceAllDynamicFlags(GetDynamicFlags() & ~flag); }
    // 虚函数，替换对象的所有动态标志
    virtual void ReplaceAllDynamicFlags([[maybe_unused]] uint32 flag) { }

    // 获取对象的类型 ID
    [[nodiscard]] TypeID GetTypeId() const { return m_objectTypeId; }
    // 判断对象是否具有指定的类型掩码
    [[nodiscard]] bool isType(uint16 mask) const { return (mask & m_objectType); }

    // 虚函数，为玩家构建创建更新块
    virtual void BuildCreateUpdateBlockForPlayer(UpdateData* data, Player* target);
    // 向玩家发送更新数据
    void SendUpdateToPlayer(Player* player);

    // 为玩家构建值更新块
    void BuildValuesUpdateBlockForPlayer(UpdateData* data, Player* target);
    // 构建超出范围的更新块
    void BuildOutOfRangeUpdateBlock(UpdateData* data) const;
    // 构建移动更新块
    void BuildMovementUpdateBlock(UpdateData* data, uint32 flags = 0) const;

    // 虚函数，为玩家销毁对象
    virtual void DestroyForPlayer(Player* target, bool onDeath = false) const;

    // 获取指定索引的 32 位有符号整数值
    [[nodiscard]] int32 GetInt32Value(uint16 index) const;
    // 获取指定索引的 32 位无符号整数值
    [[nodiscard]] uint32 GetUInt32Value(uint16 index) const;
    // 获取指定索引的 64 位无符号整数值
    [[nodiscard]] uint64 GetUInt64Value(uint16 index) const;
    // 获取指定索引的浮点数值
    [[nodiscard]] float GetFloatValue(uint16 index) const;
    // 获取指定索引和偏移量的 8 位无符号整数值
    [[nodiscard]] uint8 GetByteValue(uint16 index, uint8 offset) const;
    // 获取指定索引和偏移量的 16 位无符号整数值
    [[nodiscard]] uint16 GetUInt16Value(uint16 index, uint8 offset) const;
    // 获取指定索引的 GUID 值
    [[nodiscard]] ObjectGuid GetGuidValue(uint16 index) const;

    // 设置指定索引的 32 位有符号整数值
    void SetInt32Value(uint16 index, int32 value);
    // 设置指定索引的 32 位无符号整数值
    void SetUInt32Value(uint16 index, uint32 value);
    // 更新指定索引的 32 位无符号整数值
    void UpdateUInt32Value(uint16 index, uint32 value);
    // 设置指定索引的 64 位无符号整数值
    void SetUInt64Value(uint16 index, uint64 value);
    // 设置指定索引的浮点数值
    void SetFloatValue(uint16 index, float value);
    // 设置指定索引和偏移量的 8 位无符号整数值
    void SetByteValue(uint16 index, uint8 offset, uint8 value);
    // 设置指定索引和偏移量的 16 位无符号整数值
    void SetUInt16Value(uint16 index, uint8 offset, uint16 value);
    // 设置指定索引和偏移量的 16 位有符号整数值
    void SetInt16Value(uint16 index, uint8 offset, int16 value) { SetUInt16Value(index, offset, (uint16)value); }
    // 设置指定索引的 GUID 值
    void SetGuidValue(uint16 index, ObjectGuid value);
    // 设置指定索引的统计浮点数值
    void SetStatFloatValue(uint16 index, float value);
    // 设置指定索引的统计 32 位有符号整数值
    void SetStatInt32Value(uint16 index, int32 value);

    // 向指定索引添加 GUID 值
    bool AddGuidValue(uint16 index, ObjectGuid value);
    // 从指定索引移除 GUID 值
    bool RemoveGuidValue(uint16 index, ObjectGuid value);

    // 对指定索引的 32 位无符号整数值应用修改
    void ApplyModUInt32Value(uint16 index, int32 val, bool apply);
    // 对指定索引的 32 位有符号整数值应用修改
    void ApplyModInt32Value(uint16 index, int32 val, bool apply);
    // 对指定索引的 64 位无符号整数值应用修改
    void ApplyModUInt64Value(uint16 index, int32 val, bool apply);
    // 对指定索引的正浮点数值应用修改
    void ApplyModPositiveFloatValue(uint16 index, float val, bool apply);
    // 对指定索引的有符号浮点数值应用修改
    void ApplyModSignedFloatValue(uint16 index, float val, bool apply);
    // 对指定索引的浮点数值应用百分比修改
    void ApplyPercentModFloatValue(uint16 index, float val, bool apply);

    // 设置指定索引的标志位
    void SetFlag(uint16 index, uint32 newFlag);
    // 移除指定索引的标志位
    void RemoveFlag(uint16 index, uint32 oldFlag);
    // 切换指定索引的标志位
    void ToggleFlag(uint16 index, uint32 flag);
    // 判断指定索引是否具有指定的标志位
    [[nodiscard]] bool HasFlag(uint16 index, uint32 flag) const;
    // 对指定索引的标志位应用修改
    void ApplyModFlag(uint16 index, uint32 flag, bool apply);

    // 设置指定索引和偏移量的字节标志位
    void SetByteFlag(uint16 index, uint8 offset, uint8 newFlag);
    // 移除指定索引和偏移量的字节标志位
    void RemoveByteFlag(uint16 index, uint8 offset, uint8 newFlag);
    // 判断指定索引和偏移量是否具有指定的字节标志位
    [[nodiscard]] bool HasByteFlag(uint16 index, uint8 offset, uint8 flag) const;

    // 设置指定索引的 64 位标志位
    void SetFlag64(uint16 index, uint64 newFlag);
    // 移除指定索引的 64 位标志位
    void RemoveFlag64(uint16 index, uint64 oldFlag);
    // 切换指定索引的 64 位标志位
    void ToggleFlag64(uint16 index, uint64 flag);
    // 判断指定索引是否具有指定的 64 位标志位
    [[nodiscard]] bool HasFlag64(uint16 index, uint64 flag) const;
    // 对指定索引的 64 位标志位应用修改
    void ApplyModFlag64(uint16 index, uint64 flag, bool apply);

    // 清除更新掩码
    void ClearUpdateMask(bool remove);

    // 获取值的数量
    [[nodiscard]] uint16 GetValuesCount() const { return m_valuesCount; }

    // 虚函数，判断对象是否有指定任务，默认返回 false
    [[nodiscard]] virtual bool hasQuest(uint32 /* quest_id */) const { return false; }
    // 虚函数，判断对象是否参与指定任务，默认返回 false
    [[nodiscard]] virtual bool hasInvolvedQuest(uint32 /* quest_id */) const { return false; }
    // 虚函数，构建更新数据，默认不做任何操作
    virtual void BuildUpdate(UpdateDataMapType&, UpdatePlayerSet&) {}
    // 为玩家构建字段更新数据
    void BuildFieldsUpdate(Player*, UpdateDataMapType&);

    // 设置字段通知标志
    void SetFieldNotifyFlag(uint16 flag) { _fieldNotifyFlags |= flag; }
    // 移除字段通知标志
    void RemoveFieldNotifyFlag(uint16 flag) { _fieldNotifyFlags &= ~flag; }

    // FG: 一些临时辅助函数
    void ForceValuesUpdateAtIndex(uint32);

    // 判断对象是否为玩家
    [[nodiscard]] inline bool IsPlayer() const { return GetTypeId() == TYPEID_PLAYER; }
    // 将对象转换为玩家指针，如果不是玩家则返回 nullptr
    Player* ToPlayer() { if (IsPlayer()) return reinterpret_cast<Player*>(this); else return nullptr; }
    // 将常量对象转换为常量玩家指针，如果不是玩家则返回 nullptr
    [[nodiscard]] Player const* ToPlayer() const { if (IsPlayer()) return reinterpret_cast<Player const*>(this); else return nullptr; }

    // 判断对象是否为生物
    [[nodiscard]] inline bool IsCreature() const { return GetTypeId() == TYPEID_UNIT; }
    // 将对象转换为生物指针，如果不是生物则返回 nullptr
    Creature* ToCreature() { if (IsCreature()) return reinterpret_cast<Creature*>(this); else return nullptr; }
    // 将常量对象转换为常量生物指针，如果不是生物则返回 nullptr
    [[nodiscard]] Creature const* ToCreature() const { if (IsCreature()) return reinterpret_cast<Creature const*>(this); else return nullptr; }

    // 判断对象是否为单位
    [[nodiscard]] inline bool IsUnit() const { return isType(TYPEMASK_UNIT); }
    // 将对象转换为单位指针，如果不是单位则返回 nullptr
    Unit* ToUnit() { if (IsCreature() || IsPlayer()) return reinterpret_cast<Unit*>(this); else return nullptr; }
    // 将常量对象转换为常量单位指针，如果不是单位则返回 nullptr
    [[nodiscard]] Unit const* ToUnit() const { if (IsCreature() || IsPlayer()) return reinterpret_cast<Unit const*>(this); else return nullptr; }

    // 判断对象是否为游戏对象
    [[nodiscard]] inline bool IsGameObject() const { return GetTypeId() == TYPEID_GAMEOBJECT; }
    // 将对象转换为游戏对象指针，如果不是游戏对象则返回 nullptr
    GameObject* ToGameObject() { if (IsGameObject()) return reinterpret_cast<GameObject*>(this); else return nullptr; }
    // 将常量对象转换为常量游戏对象指针，如果不是游戏对象则返回 nullptr
    [[nodiscard]] GameObject const* ToGameObject() const { if (IsGameObject()) return reinterpret_cast<GameObject const*>(this); else return nullptr; }

    // 判断对象是否为尸体
    [[nodiscard]] inline bool IsCorpse() const { return GetTypeId() == TYPEID_CORPSE; }
    // 将对象转换为尸体指针，如果不是尸体则返回 nullptr
    Corpse* ToCorpse() { if (IsCorpse()) return reinterpret_cast<Corpse*>(this); else return nullptr; }
    // 将常量对象转换为常量尸体指针，如果不是尸体则返回 nullptr
    [[nodiscard]] Corpse const* ToCorpse() const { if (IsCorpse()) return reinterpret_cast<Corpse const*>(this); else return nullptr; }

    // 判断对象是否为动态对象
    [[nodiscard]] inline bool IsDynamicObject() const { return GetTypeId() == TYPEID_DYNAMICOBJECT; }
    // 将对象转换为动态对象指针，如果不是动态对象则返回 nullptr
    DynamicObject* ToDynObject() { if (IsDynamicObject()) return reinterpret_cast<DynamicObject*>(this); else return nullptr; }
    // 将常量对象转换为常量动态对象指针，如果不是动态对象则返回 nullptr
    [[nodiscard]] DynamicObject const* ToDynObject() const { if (IsDynamicObject()) return reinterpret_cast<DynamicObject const*>(this); else return nullptr; }

    // 判断对象是否为物品
    [[nodiscard]] inline bool IsItem() const { return GetTypeId() == TYPEID_ITEM; }

    // 虚函数，心跳函数，默认不做任何操作
    virtual void Heartbeat() {}

    // 虚函数，获取调试信息
    virtual std::string GetDebugInfo() const;

    // 自定义数据映射
    DataMap CustomData;

    // 模板函数，判断对象的条目 ID 是否等于给定的任意数量的条目 ID 之一
    template<typename... T>
    [[nodiscard]] bool EntryEquals(T... entries) const
    {
        return ((GetEntry() == entries) || ...);
    }

protected:
    // 默认构造函数
    Object();

    // 初始化值
    void _InitValues();
    // 创建对象
    void _Create(ObjectGuid::LowType guidlow, uint32 entry, HighGuid guidhigh);
    // 拼接指定索引范围的字段
    [[nodiscard]] std::string _ConcatFields(uint16 startIndex, uint16 size) const;
    // 将数据加载到数据字段中
    bool _LoadIntoDataField(std::string const& data, uint32 startOffset, uint32 count);

    // 获取玩家的更新字段数据
    uint32 GetUpdateFieldData(Player const* target, uint32*& flags) const;

    // 构建移动更新数据
    void BuildMovementUpdate(ByteBuffer* data, uint16 flags) const;
    // 虚函数，构建值更新数据
    virtual void BuildValuesUpdate(uint8 updateType, ByteBuffer* data, Player* target);

    // 对象类型掩码
    uint16 m_objectType;

    // 对象类型 ID
    TypeID m_objectTypeId;
    // 更新标志
    uint16 m_updateFlag;

    // 联合体，存储不同类型的值指针
    union
    {
        int32*  m_int32Values;
        uint32* m_uint32Values;
        float*  m_floatValues;
    };

    // 更改掩码
    UpdateMask _changesMask;

    // 值的数量
    uint16 m_valuesCount;

    // 字段通知标志
    uint16 _fieldNotifyFlags;

    // 纯虚函数，将对象添加到对象更新列表中
    virtual void AddToObjectUpdate() = 0;
    // 纯虚函数，将对象从对象更新列表中移除
    virtual void RemoveFromObjectUpdate() = 0;
    // 根据需要将对象添加到对象更新列表中
    void AddToObjectUpdateIfNeeded();

    // 对象是否已更新的标志
    bool m_objectUpdated;

private:
    // 对象是否在世界中的标志
    bool m_inWorld;

    // 打包后的 GUID
    PackedGuid m_PackGUID;

    // 输出断言的有用错误信息
    [[nodiscard]] bool PrintIndexError(uint32 index, bool set) const;
    // 禁止生成拷贝构造函数
    Object(const Object&);                              // prevent generation copy constructor
    // 禁止生成赋值运算符
    Object& operator=(Object const&);                   // prevent generation assigment operator
};

// 移动信息结构体，存储对象的移动相关信息
struct MovementInfo
{
    // 通用信息
    ObjectGuid guid;
    uint32 flags{0};
    uint16 flags2{0};
    Position pos;
    uint32 time{0};

    // 运输工具信息
    struct TransportInfo
    {
        // 重置运输工具信息
        void Reset()
        {
            guid.Clear();
            pos.Relocate(0.0f, 0.0f, 0.0f, 0.0f);
            seat = -1;
            time = 0;
            time2 = 0;
        }

        ObjectGuid guid;
        Position pos;
        int8 seat;
        uint32 time;
        uint32 time2;
    } transport;

    // 游泳/飞行信息
    float pitch{0.0f};

    // 坠落信息
    uint32 fallTime{0};

    // 跳跃信息
    struct JumpInfo
    {
        // 重置跳跃信息
        void Reset()
        {
            zspeed = sinAngle = cosAngle = xyspeed = 0.0f;
        }

        float zspeed, sinAngle, cosAngle, xyspeed;
    } jump;

    // 样条信息
    float splineElevation{0.0f};

    // 构造函数，初始化移动信息
    MovementInfo()
    {
        pos.Relocate(0.0f, 0.0f, 0.0f, 0.0f);
        transport.Reset();
        jump.Reset();
    }

    // 获取移动标志
    [[nodiscard]] uint32 GetMovementFlags() const { return flags; }
    // 设置移动标志
    void SetMovementFlags(uint32 flag) { flags = flag; }
    // 添加移动标志
    void AddMovementFlag(uint32 flag) { flags |= flag; }
    // 移除移动标志
    void RemoveMovementFlag(uint32 flag) { flags &= ~flag; }
    // 判断是否具有指定的移动标志
    [[nodiscard]] bool HasMovementFlag(uint32 flag) const { return flags & flag; }

    // 获取额外移动标志
    [[nodiscard]] uint16 GetExtraMovementFlags() const { return flags2; }
    // 添加额外移动标志
    void AddExtraMovementFlag(uint16 flag) { flags2 |= flag; }
    // 判断是否具有指定的额外移动标志
    [[nodiscard]] bool HasExtraMovementFlag(uint16 flag) const { return flags2 & flag; }

    // 设置坠落时间
    void SetFallTime(uint32 newFallTime) { fallTime = newFallTime; }

    // 输出调试信息
    void OutDebug();
};

// 网格对象模板类，用于管理对象在网格中的添加和移除
template<class T>
class GridObject
{
public:
    // 判断对象是否在网格中
    [[nodiscard]] bool IsInGrid() const { return _gridRef.isValid(); }
    // 将对象添加到网格中
    void AddToGrid(GridRefMgr<T>& m) { ASSERT(!IsInGrid()); _gridRef.link(&m, (T*)this); }
    // 将对象从网格中移除
    void RemoveFromGrid() { ASSERT(IsInGrid()); _gridRef.unlink(); }
private:
    // 网格引用
    GridReference<T> _gridRef;
};

// 带标志的值数组模板类，用于管理带标志的值
template <class T_VALUES, class T_FLAGS, class FLAG_TYPE, uint8 ARRAY_SIZE>
class FlaggedValuesArray32
{
public:
    // 构造函数，初始化值数组和标志
    FlaggedValuesArray32()
    {
        memset(&m_values, 0x00, sizeof(T_VALUES) * ARRAY_SIZE);
        m_flags = 0;
    }

    // 获取标志
    [[nodiscard]] T_FLAGS GetFlags() const { return m_flags; }
    // 判断是否具有指定的标志
    [[nodiscard]] bool HasFlag(FLAG_TYPE flag) const { return m_flags & (1 << flag); }
    // 添加指定的标志
    void AddFlag(FLAG_TYPE flag) { m_flags |= (1 << flag); }
    // 删除指定的标志
    void DelFlag(FLAG_TYPE flag) { m_flags &= ~(1 << flag); }

    // 获取指定标志对应的值
    [[nodiscard]] T_VALUES GetValue(FLAG_TYPE flag) const { return m_values[flag]; }
    // 设置指定标志对应的值
    void SetValue(FLAG_TYPE flag, T_VALUES value) { m_values[flag] = value; }
    // 为指定标志对应的值添加增量
    void AddValue(FLAG_TYPE flag, T_VALUES value) { m_values[flag] += value; }

private:
    // 值数组
    T_VALUES m_values[ARRAY_SIZE];
    // 标志
    T_FLAGS m_flags;
};

// 地图对象单元格移动状态枚举，定义对象在地图单元格中的移动状态
enum MapObjectCellMoveState
{
    // 不在移动列表中
    MAP_OBJECT_CELL_MOVE_NONE, //not in move list
    // 在移动列表中且活跃
    MAP_OBJECT_CELL_MOVE_ACTIVE, //in move list
    // 在移动列表中但不应移动
    MAP_OBJECT_CELL_MOVE_INACTIVE, //in move list but should not move
};

// 可移动地图对象类，用于管理可移动对象在地图中的单元格信息
class MovableMapObject
{
    // 声明友元类，允许 Map 类访问私有成员
    friend class Map; //map for moving creatures
    // 声明友元类，允许 ObjectGridLoader 类访问私有成员
    friend class ObjectGridLoader; //grid loader for loading creatures
    // 声明友元模板类，允许 RandomMovementGenerator 类访问私有成员
    template<class T> friend class RandomMovementGenerator;

protected:
    // 默认构造函数
    MovableMapObject()  = default;

    // 获取当前单元格信息
    [[nodiscard]] Cell const& GetCurrentCell() const { return _currentCell; }

private:
    // 设置当前单元格信息
    void SetCurrentCell(Cell const& cell) { _currentCell = cell; }

    // 当前单元格信息
    Cell _currentCell;
    // 地图对象单元格移动状态
    MapObjectCellMoveState _moveState{MAP_OBJECT_CELL_MOVE_NONE};
};

// 可更新地图对象类，用于管理对象在地图中的更新状态
class UpdatableMapObject
{
    // 声明友元类，允许 Map 类访问私有成员
    friend class Map;

public:
    // 更新状态枚举，定义对象的更新状态
    enum UpdateState : uint8
    {
        // 未在更新中
        NotUpdating,
        // 待添加到更新列表
        PendingAdd,
        // 正在更新中
        Updating
    };

protected:
    // 构造函数，初始化更新列表偏移量和更新状态
    UpdatableMapObject() : _mapUpdateListOffset(0), _mapUpdateState(NotUpdating) { }

private:
    // 设置地图更新列表偏移量
    void SetMapUpdateListOffset(std::size_t const offset)
    {
        ASSERT(_mapUpdateState == Updating, "Attempted to set update list offset when object is not in map update list");
        _mapUpdateListOffset = offset;
    }

    // 获取地图更新列表偏移量
    size_t GetMapUpdateListOffset() const
    {
        ASSERT(_mapUpdateState == Updating, "Attempted to get update list offset when object is not in map update list");
        return _mapUpdateListOffset;
    }

    // 设置更新状态
    void SetUpdateState(UpdateState state)
    {
        _mapUpdateState = state;
    }

    // 获取更新状态
    UpdateState GetUpdateState() const
    {
        return _mapUpdateState;
    }

private:
    // 地图更新列表偏移量
    std::size_t _mapUpdateListOffset;
    // 更新状态
    UpdateState _mapUpdateState;
};

// 世界对象类，继承自 Object 和 WorldLocation，代表世界中的对象
class WorldObject : public Object, public WorldLocation
{
protected:
    // 构造函数，参数表示对象是在网格对象列表还是世界对象列表中
    explicit WorldObject(bool isWorldObject); //note: here it means if it is in grid object list or world object list
public:
    // 析构函数
    ~WorldObject() override;

    // 虚函数，更新对象状态
    virtual void Update(uint32 diff);

    // 创建对象
    void _Create(ObjectGuid::LowType guidlow, HighGuid guidhigh, uint32 phaseMask);

    // 将对象添加到世界中
    void AddToWorld() override;
    // 将对象从世界中移除
    void RemoveFromWorld() override;

    // 获取相对于搜索者的 2D 近点
    void GetNearPoint2D(WorldObject const* searcher, float& x, float& y, float distance, float absAngle, Position const* startPos = nullptr) const;
    // 获取 2D 近点
    void GetNearPoint2D(float& x, float& y, float distance, float absAngle, Position const* startPos = nullptr) const;
    // 获取相对于搜索者的近点
    void GetNearPoint(WorldObject const* searcher, float& x, float& y, float& z, float searcher_size, float distance2d, float absAngle, float controlZ = 0, Position const* startPos = nullptr) const;
    // 获取虚空近点
    void GetVoidClosePoint(float& x, float& y, float& z, float size, float distance2d = 0, float relAngle = 0, float controlZ = 0) const;
    // 获取近点
    bool GetClosePoint(float& x, float& y, float& z, float size, float distance2d = 0, float angle = 0, WorldObject const* forWho = nullptr, bool force = false) const;
    // 移动位置
    void MovePosition(Position& pos, float dist, float angle);
    // 获取近位置
    Position GetNearPosition(float dist, float angle);
    // 移动位置直到第一次碰撞
    void MovePositionToFirstCollision(Position& pos, float dist, float angle);
    // 获取第一次碰撞位置
    Position GetFirstCollisionPosition(float startX, float startY, float startZ, float destX, float destY);
    // 获取第一次碰撞位置
    Position GetFirstCollisionPosition(float destX, float destY, float destZ);
    // 获取第一次碰撞位置
    Position GetFirstCollisionPosition(float dist, float angle);
    // 获取随机近位置
    Position GetRandomNearPosition(float radius);

    // 获取接触点
    void GetContactPoint(WorldObject const* obj, float& x, float& y, float& z, float distance2d = CONTACT_DISTANCE) const;
    // 获取冲锋接触点
    void GetChargeContactPoint(WorldObject const* obj, float& x, float& y, float& z, float distance2d = CONTACT_DISTANCE) const;

    // 获取对象大小
    [[nodiscard]] float GetObjectSize() const;

    // 虚函数，获取战斗范围，默认返回 0.0f，仅在 Unit 类中重写
    [[nodiscard]] virtual float GetCombatReach() const { return 0.0f; } // overridden (only) in Unit
    // 更新地面位置的 Z 坐标
    void UpdateGroundPositionZ(float x, float y, float& z) const;
    // 更新允许的位置 Z 坐标
    void UpdateAllowedPositionZ(float x, float y, float& z, float* groundZ = nullptr) const;

    // 获取随机点
    void GetRandomPoint(const Position& srcPos, float distance, float& rand_x, float& rand_y, float& rand_z) const;
    // 获取随机点
    [[nodiscard]] Position GetRandomPoint(const Position& srcPos, float distance) const;

    // 获取实例 ID
    [[nodiscard]] uint32 GetInstanceId() const { return m_InstanceId; }

    // 虚函数，设置相位掩码
    virtual void SetPhaseMask(uint32 newPhaseMask, bool update);
    // 获取相位掩码
    [[nodiscard]] uint32 GetPhaseMask() const { return m_phaseMask; }
    // 判断是否与另一个世界对象处于相同相位
    bool InSamePhase(WorldObject const* obj) const { return InSamePhase(obj->GetPhaseMask()); }
    // 判断是否与指定相位掩码处于相同相位
    [[nodiscard]] bool InSamePhase(uint32 phasemask) const { return m_useCombinedPhases ? GetPhaseMask() & phasemask : GetPhaseMask() == phasemask; }

    // 获取区域 ID
    [[nodiscard]] uint32 GetZoneId() const;
    // 获取地区 ID
    [[nodiscard]] uint32 GetAreaId() const;
    // 获取区域 ID 和地区 ID
    void GetZoneAndAreaId(uint32& zoneid, uint32& areaid) const;
    // 判断是否在户外
    [[nodiscard]] bool IsOutdoors() const;
    // 获取液体数据
    [[nodiscard]] LiquidData const& GetLiquidData() const;

    // 获取实例脚本
    [[nodiscard]] InstanceScript* GetInstanceScript() const;

    // 获取对象名称
    [[nodiscard]] std::string const& GetName() const { return m_name; }
    // 设置对象名称
    void SetName(std::string const& newname) { m_name = newname; }

    // 虚函数，获取指定语言索引的名称，默认返回对象名称
    [[nodiscard]] virtual std::string const& GetNameForLocaleIdx(LocaleConstant /*locale_idx*/) const { return m_name; }

    // 获取与另一个世界对象的距离
    float GetDistance(WorldObject const* obj) const;
    // 获取与指定位置的距离
    [[nodiscard]] float GetDistance(const Position& pos) const;
    // 获取与指定坐标的距离
    [[nodiscard]] float GetDistance(float x, float y, float z) const;
    // 获取与另一个世界对象的 2D 距离
    float GetDistance2d(WorldObject const* obj) const;
    // 获取与指定坐标的 2D 距离
    [[nodiscard]] float GetDistance2d(float x, float y) const;
    // 获取与另一个世界对象的 Z 轴距离
    float GetDistanceZ(WorldObject const* obj) const;

    // 判断是否是自身或与另一个世界对象在同一地图中
    bool IsSelfOrInSameMap(WorldObject const* obj) const;
    // 判断是否与另一个世界对象在同一地图中
    bool IsInMap(WorldObject const* obj) const;
    // 判断是否在指定 3D 距离范围内
    [[nodiscard]] bool IsWithinDist3d(float x, float y, float z, float dist) const;
    // 判断是否在指定 3D 距离范围内
    bool IsWithinDist3d(const Position* pos, float dist) const;
    // 判断是否在指定 2D 距离范围内
    [[nodiscard]] bool IsWithinDist2d(float x, float y, float dist) const;
    // 判断是否在指定 2D 距离范围内
    bool IsWithinDist2d(const Position* pos, float dist) const;
    // 仅在确定两个对象在同一地图时使用，判断是否在指定距离范围内
    bool IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D = true, bool useBoundingRadius = true) const;
    // 判断是否在同一地图中且在指定距离范围内
    bool IsWithinDistInMap(WorldObject const* obj, float dist2compare, bool is3D = true, bool useBoundingRadius = true) const;
    // 判断是否在视线范围内
    [[nodiscard]] bool IsWithinLOS(float x, float y, float z, VMAP::ModelIgnoreFlags ignoreFlags = VMAP::ModelIgnoreFlags::Nothing, LineOfSightChecks checks = LINEOFSIGHT_ALL_CHECKS) const;
    // 判断是否在同一地图中且在视线范围内
    [[nodiscard]] bool IsWithinLOSInMap(WorldObject const* obj, VMAP::ModelIgnoreFlags ignoreFlags = VMAP::ModelIgnoreFlags::Nothing, LineOfSightChecks checks = LINEOFSIGHT_ALL_CHECKS, Optional<float> collisionHeight = { }, Optional<float> combatReach = { }) const;
    // 获取命中球体的点
    [[nodiscard]] Position GetHitSpherePointFor(Position const& dest, Optional<float> collisionHeight = { }, Optional<float> combatReach = { }) const;
    // 获取命中球体的点
    void GetHitSpherePointFor(Position const& dest, float& x, float& y, float& z, Optional<float> collisionHeight = { }, Optional<float> combatReach = { }) const;
    // 判断两个对象与当前对象的距离顺序
    bool GetDistanceOrder(WorldObject const* obj1, WorldObject const* obj2, bool is3D = true) const;
    // 判断另一个对象是否在指定距离范围内
    bool IsInRange(WorldObject const* obj, float minRange, float maxRange, bool is3D = true) const;
    // 判断指定坐标是否在 2D 距离范围内
    [[nodiscard]] bool IsInRange2d(float x, float y, float minRange, float maxRange) const;
    // 判断指定坐标是否在 3D 距离范围内
    [[nodiscard]] bool IsInRange3d(float x, float y, float z, float minRange, float maxRange) const;
    // 判断目标对象是否在当前对象前方
    bool isInFront(WorldObject const* target, float arc = M_PI) const;
    // 判断目标对象是否在当前对象后方
    bool isInBack(WorldObject const* target, float arc = M_PI) const;

    // 判断当前对象是否在两个对象之间
    bool IsInBetween(WorldObject const* obj1, WorldObject const* obj2, float size = 0) const;

    // 虚函数，在删除对象前进行清理操作，用于析构函数或大规模生物删除前
    virtual void CleanupsBeforeDelete(bool finalCleanup = true);  // used in destructor or explicitly before mass creature delete to remove cross-references to already deleted units

    // 虚函数，向对象集合发送消息
    virtual void SendMessageToSet(WorldPacket const* data, bool self) const;
    // 虚函数，向指定范围内的对象集合发送消息
    virtual void SendMessageToSetInRange(WorldPacket const* data, float dist, bool self) const;
    // 虚函数，向对象集合发送消息，跳过指定接收者
    virtual void SendMessageToSet(WorldPacket const* data, Player const* skipped_rcvr) const;

    // 虚函数，获取目标对象的等级，默认返回 1
    virtual uint8 getLevelForTarget(WorldObject const* /*target*/) const { return 1; }

    // 播放有距离限制的音效
    void PlayDistanceSound(uint32 sound_id, Player* target = nullptr);
    // 直接播放音效
    void PlayDirectSound(uint32 sound_id, Player* target = nullptr);
    // 播放指定半径范围内的音效
    void PlayRadiusSound(uint32 sound_id, float radius);
    // 直接播放音乐
    void PlayDirectMusic(uint32 music_id, Player* target = nullptr);
    // 播放指定半径范围内的音乐
    void PlayRadiusMusic(uint32 music_id, float radius);

    // 发送对象消失动画
    void SendObjectDeSpawnAnim(ObjectGuid guid);

    // 虚函数，保存重生时间，默认不做任何操作
    virtual void SaveRespawnTime() {}
    // 将对象添加到移除列表
    void AddObjectToRemoveList();

    // 获取网格激活范围
    [[nodiscard]] float GetGridActivationRange() const;
    // 获取可见性范围
    [[nodiscard]] float GetVisibilityRange() const;
    // 虚函数，获取视野范围
    virtual float GetSightRange(WorldObject const* target = nullptr) const;
    // 判断是否能看到或检测到另一个对象
    //bool CanSeeOrDetect(WorldObject const* obj, bool ignoreStealth = false, bool distanceCheck = false) const;
    bool CanSeeOrDetect(WorldObject const* obj, bool ignoreStealth = false, bool distanceCheck = false, bool checkAlert = false) const;

    // 潜行信息数组
    FlaggedValuesArray32<int32, uint32, StealthType, TOTAL_STEALTH_TYPES> m_stealth;
    // 潜行检测信息数组
    FlaggedValuesArray32<int32, uint32, StealthType, TOTAL_STEALTH_TYPES> m_stealthDetect;

    // 隐形信息数组
    FlaggedValuesArray32<int32, uint32, InvisibilityType, TOTAL_INVISIBILITY_TYPES> m_invisibility;
    // 隐形检测信息数组
    FlaggedValuesArray32<int32, uint32, InvisibilityType, TOTAL_INVISIBILITY_TYPES> m_invisibilityDetect;

    // 服务器端可见性信息数组
    FlaggedValuesArray32<int32, uint32, ServerSideVisibilityType, TOTAL_SERVERSIDE_VISIBILITY_TYPES> m_serverSideVisibility;
    // 服务器端可见性检测信息数组
    FlaggedValuesArray32<int32, uint32, ServerSideVisibilityType, TOTAL_SERVERSIDE_VISIBILITY_TYPES> m_serverSideVisibilityDetect;

    // 低级数据包操作：发送播放音乐指令
    void SendPlayMusic(uint32 Music, bool OnlySelf);

    // 虚函数，设置地图
    virtual void SetMap(Map* map);
    // 虚函数，重置地图
    virtual void ResetMap();
    // 获取当前地图指针
    [[nodiscard]] Map* GetMap() const { ASSERT(m_currMap); return m_currMap; }
    // 查找当前地图指针
    [[nodiscard]] Map* FindMap() const { return m_currMap; }
    // 用于检查对象不在世界中时所有的 GetMap() 调用

    // 设置区域脚本
    void SetZoneScript();
    // 清除区域脚本
    void ClearZoneScript();
    // 获取区域脚本指针
    [[nodiscard]] ZoneScript* GetZoneScript() const { return m_zoneScript; }

    // 召唤生物
    TempSummon* SummonCreature(uint32 id, const Position& pos, TempSummonType spwtype = TEMPSUMMON_MANUAL_DESPAWN, uint32 despwtime = 0, uint32 vehId = 0, SummonPropertiesEntry const* properties = nullptr, bool visibleBySummonerOnly = false) const;
    // 召唤生物
    TempSummon* SummonCreature(uint32 id, float x, float y, float z, float ang = 0, TempSummonType spwtype = TEMPSUMMON_MANUAL_DESPAWN, uint32 despwtime = 0, SummonPropertiesEntry const* properties = nullptr, bool visibleBySummonerOnly = false);
    // 召唤游戏对象
    GameObject* SummonGameObject(uint32 entry, float x, float y, float z, float ang, float rotation0, float rotation1, float rotation2, float rotation3, uint32 respawnTime, bool checkTransport = true, GOSummonType summonType = GO_SUMMON_TIMED_OR_CORPSE_DESPAWN);
    // 召唤触发器
    Creature*   SummonTrigger(float x, float y, float z, float ang, uint32 dur, bool setLevel = false, CreatureAI * (*GetAI)(Creature*) = nullptr);
    // 召唤生物组
    void SummonCreatureGroup(uint8 group, std::list<TempSummon*>* list = nullptr);

    // 查找最近的生物
    [[nodiscard]] Creature*   FindNearestCreature(uint32 entry, float range, bool alive = true) const;
    // 查找最近的游戏对象
    [[nodiscard]] GameObject* FindNearestGameObject(uint32 entry, float range, bool onlySpawned = false) const;
    // 查找最近的指定类型的游戏对象
    [[nodiscard]] GameObject* FindNearestGameObjectOfType(GameobjectTypes type, float range) const;

    // 选择最近的玩家
    [[nodiscard]] Player* SelectNearestPlayer(float distance = 0) const;
    // 获取指定范围内指定条目的游戏对象列表
    void GetGameObjectListWithEntryInGrid(std::list<GameObject*>& lList, uint32 uiEntry, float fMaxSearchRange) const;
    // 获取指定范围内指定多个条目的游戏对象列表
    void GetGameObjectListWithEntryInGrid(std::list<GameObject*>& gameobjectList, std::vector<uint32> const& entries, float maxSearchRange) const;
    // 获取指定范围内指定条目的生物列表
    void GetCreatureListWithEntryInGrid(std::list<Creature*>& lList, uint32 uiEntry, float fMaxSearchRange) const;
    // 获取指定范围内指定多个条目的生物列表
    void GetCreatureListWithEntryInGrid(std::list<Creature*>& creatureList, std::vector<uint32> const& entries, float maxSearchRange) const;
    // 获取指定范围内的死亡生物列表
    void GetDeadCreatureListInGrid(std::list<Creature*>& lList, float maxSearchRange, bool alive = false) const;

    // 为附近的玩家销毁对象
    void DestroyForNearbyPlayers();
    // 虚函数，更新对象的可见性
    virtual void UpdateObjectVisibility(bool forced = true, bool fromUpdate = false);
    // 虚函数，在创建对象时更新对象的可见性
    virtual void UpdateObjectVisibilityOnCreate() { UpdateObjectVisibility(true); }
    // 重写 BuildUpdate 函数，构建更新数据
    void BuildUpdate(UpdateDataMapType& data_map, UpdatePlayerSet& player_set) override;
    // 获取指定范围内指定条目的生物列表
    void GetCreaturesWithEntryInRange(std::list<Creature*>& creatureList, float radius, uint32 entry);

    // 设置位置数据更新标志
    void SetPositionDataUpdate();
    // 更新位置数据
    void UpdatePositionData();

    // 重写 AddToObjectUpdate 函数，将对象添加到对象更新列表中
    void AddToObjectUpdate() override;
    // 重写 RemoveFromObjectUpdate 函数，将对象从对象更新列表中移除
    void RemoveFromObjectUpdate() override;

    // 重定位和可见性系统函数：添加通知标志
    void AddToNotify(uint16 f);
    // 重定位和可见性系统函数：移除通知标志
    void RemoveFromNotify(uint16 f) { m_notifyflags &= ~f; }
    // 重定位和可见性系统函数：判断是否需要通知
    [[nodiscard]] bool isNeedNotify(uint16 f) const { return m_notifyflags & f;}
    // 重定位和可见性系统函数：获取通知标志
    [[nodiscard]] uint16 GetNotifyFlags() const { return m_notifyflags; }
    // 重定位和可见性系统函数：判断通知是否已执行
    [[nodiscard]] bool NotifyExecuted(uint16 f) const { return m_executed_notifies & f;}
    // 重定位和可见性系统函数：设置通知已执行
    void SetNotified(uint16 f) { m_executed_notifies |= f;}
    // 重定位和可见性系统函数：重置所有通知
    void ResetAllNotifies() { m_notifyflags = 0; m_executed_notifies = 0; }

    // 判断对象是否为活跃对象
    [[nodiscard]] bool isActiveObject() const { return m_isActive; }
    // 设置对象是否为活跃对象
    void setActive(bool isActiveObject);
    // 判断对象是否在远距离可见
    [[nodiscard]] bool IsFarVisible() const { return m_isFarVisible; }
    // 判断对象的可见性是否被覆盖
    [[nodiscard]] bool IsVisibilityOverridden() const { return m_visibilityDistanceOverride.has_value(); }
    // 设置对象的可见性距离覆盖类型
    void SetVisibilityDistanceOverride(VisibilityDistanceType type);
    // 设置对象是否为世界对象
    void SetWorldObject(bool apply);
    // 判断对象是否为永久世界对象
    [[nodiscard]] bool IsPermanentWorldObject() const { return m_isWorldObject; }
    // 判断对象是否为世界对象
    [[nodiscard]] bool IsWorldObject() const;

    // 判断对象是否在冬拥湖区域
    [[nodiscard]] bool IsInWintergrasp() const
    {
        return GetMapId() == MAP_NORTHREND && GetPositionX() > 3733.33331f && GetPositionX() < 5866.66663f && GetPositionY() > 1599.99999f && GetPositionY() < 4799.99997f;
    }

    // 最后使用的脚本 ID
    uint32  LastUsedScriptID;

    // 运输工具相关操作：获取运输工具指针
    [[nodiscard]] Transport* GetTransport() const { return m_transport; }
    // 运输工具相关操作：获取运输工具的 X 偏移量
    [[nodiscard]] float GetTransOffsetX() const { return m_movementInfo.transport.pos.GetPositionX(); }
    // 运输工具相关操作：获取运输工具的 Y 偏移量
    [[nodiscard]] float GetTransOffsetY() const { return m_movementInfo.transport.pos.GetPositionY(); }
    // 运输工具相关操作：获取运输工具的 Z 偏移量
    [[nodiscard]] float GetTransOffsetZ() const { return m_movementInfo.transport.pos.GetPositionZ(); }
    // 运输工具相关操作：获取运输工具的朝向偏移量
    [[nodiscard]] float GetTransOffsetO() const { return m_movementInfo.transport.pos.GetOrientation(); }
    // 运输工具相关操作：获取运输工具的时间
    [[nodiscard]] uint32 GetTransTime()   const { return m_movementInfo.transport.time; }
    // 运输工具相关操作：获取运输工具的座位号
    [[nodiscard]] int8 GetTransSeat()     const { return m_movementInfo.transport.seat; }
    // 运输工具相关操作：获取运输工具的 GUID
    [[nodiscard]] virtual ObjectGuid GetTransGUID()   const;
    // 运输工具相关操作：设置运输工具指针
    void SetTransport(Transport* t) { m_transport = t; }

    // 移动信息
    MovementInfo m_movementInfo;

    // 获取静止位置的 X 坐标
    [[nodiscard]] virtual float GetStationaryX() const { return GetPositionX(); }
    // 获取静止位置的 Y 坐标
    [[nodiscard]] virtual float GetStationaryY() const { return GetPositionY(); }
    // 获取静止位置的 Z 坐标
    [[nodiscard]] virtual float GetStationaryZ() const { return GetPositionZ(); }
    // 获取静止位置的朝向
    [[nodiscard]] virtual float GetStationaryO() const { return GetOrientation(); }

    // 获取地图的水面或地面高度
    [[nodiscard]] float GetMapWaterOrGroundLevel(Position pos, float* ground = nullptr) const
    {
        return GetMapWaterOrGroundLevel(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), ground);
    };
    // 获取地图的水面或地面高度
    [[nodiscard]] float GetMapWaterOrGroundLevel(float x, float y, float z, float* ground = nullptr) const;
    // 获取地图的高度
    [[nodiscard]] float GetMapHeight(float x, float y, float z, bool vmap = true, float distanceToSearch = 50.0f) const; // DEFAULT_HEIGHT_SEARCH in map.h

    // 获取地面的 Z 坐标
    [[nodiscard]] float GetFloorZ() const;
    // 获取水中的最小高度
    [[nodiscard]] float GetMinHeightInWater() const;

    // 虚函数，获取碰撞高度，默认返回 0.0f
    [[nodiscard]] virtual float GetCollisionHeight() const { return 0.0f; }
    // 虚函数，获取碰撞宽度，默认返回对象大小
    [[nodiscard]] virtual float GetCollisionWidth() const { return GetObjectSize(); }
    // 虚函数，获取碰撞半径，默认返回对象大小的一半
    [[nodiscard]] virtual float GetCollisionRadius() const { return GetObjectSize() / 2; }

    // 添加允许的拾取者
    void AddAllowedLooter(ObjectGuid guid);
    // 重置允许的拾取者
    void ResetAllowedLooters();
    // 设置允许的拾取者
    void SetAllowedLooters(GuidUnorderedSet const looters);
    // 判断是否有指定的允许拾取者
    [[nodiscard]] bool HasAllowedLooter(ObjectGuid guid) const;
    // 获取允许的拾取者集合
    [[nodiscard]] GuidUnorderedSet const& GetAllowedLooters() const;
    // 移除指定的允许拾取者
    void RemoveAllowedLooter(ObjectGuid guid);

    // 虚函数，判断是否需要更新
    virtual bool IsUpdateNeeded();
    // 判断是否可以添加到地图更新列表中
    bool CanBeAddedToMapUpdateList();

    // 重写 GetDebugInfo 函数，获取调试信息
    std::string GetDebugInfo() const override;

    // 事件处理器指针
    ElunaEventProcessor* elunaEvents;
    // 事件处理器
    EventProcessor m_Events;

protected:
    // 对象名称
    std::string m_name;
    // 对象是否活跃的标志
    bool m_isActive;
    // 对象是否在远距离可见的标志
    bool m_isFarVisible;
    // 可见性距离覆盖值
    Optional<float> m_visibilityDistanceOverride;
    // 对象是否为世界对象的标志
    const bool m_isWorldObject;
    // 区域脚本指针
    ZoneScript* m_zoneScript;

    // 处理位置数据变化
    virtual void ProcessPositionDataChanged(PositionFullTerrainStatus const& data);
    // 区域 ID
    uint32 _zoneId;
    // 地区 ID
    uint32 _areaId;
    // 地面的 Z 坐标
    float _floorZ;
    // 是否在户外的标志
    bool _outdoors;
    // 液体数据
    LiquidData _liquidData;
    // 是否需要更新位置数据的标志
    bool _updatePositionData;

    // 运输工具指针
    Transport* m_transport;

    // 这些函数主要用于 Relocate() 和尸体/玩家特定的操作...
    // 仅在 LoadFromDB()/Create() 函数中使用，其他地方请勿使用！
    // 地图 ID/实例 ID 应在 SetMap() 函数中设置！
    // 设置位置的地图 ID
    void SetLocationMapId(uint32 _mapId) { m_mapId = _mapId; }
    // 设置位置的实例 ID
    void SetLocationInstanceId(uint32 _instanceId) { m_InstanceId = _instanceId; }

    // 虚函数，判断对象是否永远不可见，默认判断对象是否不在世界中
    [[nodiscard]] virtual bool IsNeverVisible() const { return !IsInWorld(); }
    // 虚函数，判断对象是否对指定观察者永远可见，默认返回 false
    virtual bool IsAlwaysVisibleFor(WorldObject const* /*seer*/) const { return false; }
    // 虚函数，判断对象是否因消失而不可见，默认返回 false
    [[nodiscard]] virtual bool IsInvisibleDueToDespawn() const { return false; }
    // 与 IsAlwaysVisibleFor 的区别：1. 在距离检查之后；2. 使用所有者或魅惑者作为观察者
    // 虚函数，判断对象是否对指定观察者永远可检测，默认返回 false
    virtual bool IsAlwaysDetectableFor(WorldObject const* /*seer*/) const { return false; }
private:
    // 当前对象所在的地图指针
    Map* m_currMap;                                    //current object's Map location
    // 心跳计时器
    Milliseconds _heartbeatTimer;
    // 对象所在地图的 ID
    //uint32 m_mapId;                                     // object at map with map_id
    // 对象所在地图实例的 ID
    uint32 m_InstanceId;                                // in map copy with instance id
    // 对象的相位掩码
    uint32 m_phaseMask;                                 // in area phase state
    // 是否使用组合相位的标志
    bool m_useCombinedPhases;                           // true (default): use phaseMask as bit mask combining up to 32 phases
    // false: use phaseMask to represent single phases only (up to 4294967295 phases)

    // 通知标志
    uint16 m_notifyflags;
    // 已执行的通知标志
    uint16 m_executed_notifies;

    // 虚函数，判断两个对象之间的距离是否在指定范围内
    virtual bool _IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D, bool useBoundingRadius = true) const;

    // 判断是否永远无法看到另一个对象
    bool CanNeverSee(WorldObject const* obj) const;
    // 虚函数，判断是否总能看到另一个对象，默认返回 false
    virtual bool CanAlwaysSee(WorldObject const* /*obj*/) const { return false; }
    // 判断是否能检测到另一个对象
    //bool CanDetect(WorldObject const* obj, bool ignoreStealth, bool checkClient) const;
    bool CanDetect(WorldObject const* obj, bool ignoreStealth, bool checkClient, bool checkAlert = false) const;
    // 判断是否能检测到另一个对象的隐形状态
    bool CanDetectInvisibilityOf(WorldObject const* obj) const;
    // 判断是否能检测到另一个对象的潜行状态
    //bool CanDetectStealthOf(WorldObject const* obj) const;
    bool CanDetectStealthOf(WorldObject const* obj, bool checkAlert = false) const;

    // 允许的拾取者集合
    GuidUnorderedSet _allowedLooters;
};

namespace Acore
{
    // 二元谓词类，用于根据与参考世界对象的距离对世界对象进行排序
    class ObjectDistanceOrderPred
    {
    public:
        // 构造函数，初始化参考对象和排序顺序
        ObjectDistanceOrderPred(WorldObject const* pRefObj, bool ascending = true) : m_refObj(pRefObj), m_ascending(ascending) {}
        // 重载 () 运算符，用于比较两个世界对象与参考对象的距离
        bool operator()(WorldObject const* pLeft, WorldObject const* pRight) const
        {
            return m_ascending ? m_refObj->GetDistanceOrder(pLeft, pRight) : !m_refObj->GetDistanceOrder(pLeft, pRight);
        }
    private:
        // 参考世界对象指针
        WorldObject const* m_refObj;
        // 是否升序排序的标志
        const bool m_ascending;
    };
}

#endif
