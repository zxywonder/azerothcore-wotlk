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

#ifndef ACORE_MAP_H
#define ACORE_MAP_H

#include "Cell.h"
#include "DBCStructure.h"
#include "DataMap.h"
#include "Define.h"
#include "DynamicTree.h"
#include "GameObjectModel.h"
#include "GridDefines.h"
#include "GridRefMgr.h"
#include "MapGridManager.h"
#include "MapRefMgr.h"
#include "ObjectDefines.h"
#include "ObjectGuid.h"
#include "PathGenerator.h"
#include "Position.h"
#include "SharedDefines.h"
#include "TaskScheduler.h"
#include "Timer.h"
#include "GridTerrainData.h"
#include <bitset>
#include <list>
#include <memory>
#include <shared_mutex>

class Unit;
class WorldPacket;
class InstanceScript;
class Group;
class InstanceSave;
class Object;
class WorldObject;
class TempSummon;
class Player;
class CreatureGroup;
struct ScriptInfo;
struct ScriptAction;
struct Position;
class Battleground;
class MapInstanced;
class InstanceMap;
class BattlegroundMap;
class Transport;
class StaticTransport;
class MotionTransport;
class PathGenerator;

enum WeatherState : uint32;

namespace VMAP
{
    enum class ModelIgnoreFlags : uint32;
}

namespace Acore
{
    struct ObjectUpdater;
    struct LargeObjectUpdater;
}

struct ScriptAction
{
    ObjectGuid sourceGUID;
    ObjectGuid targetGUID;
    ObjectGuid ownerGUID;     // owner of source if source is item
    ScriptInfo const *script; // pointer to static script data
};

#define DEFAULT_HEIGHT_SEARCH 50.0f                              // default search distance to find height at nearby locations
#define MIN_UNLOAD_DELAY 1                                       // immediate unload
#define UPDATABLE_OBJECT_LIST_RECHECK_TIMER 30 * IN_MILLISECONDS // Time to recheck update object list

struct PositionFullTerrainStatus
{
    PositionFullTerrainStatus() = default;
    uint32 areaId{0};
    float floorZ{INVALID_HEIGHT};
    bool outdoors{false};
    LiquidData liquidInfo;
};

enum LineOfSightChecks
{
    LINEOFSIGHT_CHECK_VMAP = 0x1,        // check static floor layout data
    LINEOFSIGHT_CHECK_GOBJECT_WMO = 0x2, // check dynamic game object data (wmo models)
    LINEOFSIGHT_CHECK_GOBJECT_M2 = 0x4,  // check dynamic game object data (m2 models)

    LINEOFSIGHT_CHECK_GOBJECT_ALL = LINEOFSIGHT_CHECK_GOBJECT_WMO | LINEOFSIGHT_CHECK_GOBJECT_M2,

    LINEOFSIGHT_ALL_CHECKS = LINEOFSIGHT_CHECK_VMAP | LINEOFSIGHT_CHECK_GOBJECT_ALL
};

// GCC have alternative #pragma pack(N) syntax and old gcc version not support pack(push, N), also any gcc version not support it at some platform
#if defined(__GNUC__)
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif

struct InstanceTemplate
{
    uint32 Parent;
    uint32 ScriptId;
    bool AllowMount;
};

enum LevelRequirementVsMode
{
    LEVELREQUIREMENT_HEROIC = 70
};

struct ZoneDynamicInfo
{
    ZoneDynamicInfo();

    uint32 MusicId;
    WeatherState WeatherId;
    float WeatherGrade;
    uint32 OverrideLightId;
    uint32 LightFadeInTime;
};

#if defined(__GNUC__)
#pragma pack()
#else
#pragma pack(pop)
#endif

typedef std::map<uint32 /*leaderDBGUID*/, CreatureGroup *> CreatureGroupHolderType;
typedef std::unordered_map<uint32 /*zoneId*/, ZoneDynamicInfo> ZoneDynamicInfoMap;
typedef std::set<MotionTransport *> TransportsContainer;

enum EncounterCreditType : uint8
{
    ENCOUNTER_CREDIT_KILL_CREATURE = 0,
    ENCOUNTER_CREDIT_CAST_SPELL = 1,
};

class Map : public GridRefMgr<MapGridType>
{
    friend class MapReference;
    friend class GridObjectLoader;

public:
    /**
     * 构造函数
     * @param id 地图ID
     * @param InstanceId 实例ID
     * @param SpawnMode 生成模式
     * @param _parent 父地图指针(可选)
     */
    Map(uint32 id, uint32 InstanceId, uint8 SpawnMode, Map *_parent = nullptr);
    ~Map() override;

    /**
     * 获取地图条目信息
     * @return 返回地图条目常量指针
     */
    [[nodiscard]] MapEntry const *GetEntry() const { return i_mapEntry; }

    // currently unused for normal maps
    /**
     * 检查地图是否可以卸载
     * @param diff 时间差(毫秒)
     * @return 如果可以卸载返回true，否则返回false
     */
    bool CanUnload(uint32 diff)
    {
        if (!m_unloadTimer)
            return false;

        if (m_unloadTimer <= diff)
            return true;

        m_unloadTimer -= diff;
        return false;
    }

    /**
     * 添加玩家到地图
     * @param 玩家指针
     * @return 添加成功返回true，否则返回false
     */
    virtual bool AddPlayerToMap(Player *);

    /**
     * 从地图移除玩家
     * @param 玩家指针
     * @param 是否立即移除标志
     */
    virtual void RemovePlayerFromMap(Player *, bool);

    /**
     * 玩家从地图解除链接后的处理
     */
    virtual void AfterPlayerUnlinkFromMap();
    template <class T>
    bool AddToMap(T *, bool checkTransport = false);
    template <class T>
    void RemoveFromMap(T *, bool);

    void MarkNearbyCellsOf(WorldObject *obj);

    virtual void Update(const uint32, const uint32, bool thread = true);

    /**
     * 获取可视范围
     * @return 返回当前可视范围
     */
    [[nodiscard]] float GetVisibilityRange() const { return m_VisibleDistance; }

    /**
     * 设置可视范围
     * @param range 新的可视范围值
     */
    void SetVisibilityRange(float range) { m_VisibleDistance = range; }
    void OnCreateMap();
    // function for setting up visibility distance for maps on per-type/per-Id basis
    virtual /**
             * 初始化可视距离
             */
        void
        InitVisibilityDistance();

    /**
     * 玩家重定位
     * @param 玩家指针
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param o 方向
     */
    void PlayerRelocation(Player *, float x, float y, float z, float o);

    /**
     * 生物重定位
     * @param creature 生物指针
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param o 方向
     */
    void CreatureRelocation(Creature *creature, float x, float y, float z, float o);

    /**
     * 游戏对象重定位
     * @param go 游戏对象指针
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param o 方向
     */
    void GameObjectRelocation(GameObject *go, float x, float y, float z, float o);

    /**
     * 动态对象重定位
     * @param go 动态对象指针
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param o 方向
     */
    void DynamicObjectRelocation(DynamicObject *go, float x, float y, float z, float o);

    template <class T, class CONTAINER>
    void Visit(const Cell &cell, TypeContainerVisitor<T, CONTAINER> &visitor);

    /**
     * 检查网格是否已加载
     * @param gridCoord 网格坐标
     * @return 如果已加载返回true，否则返回false
     */
    bool IsGridLoaded(GridCoord const &gridCoord) const;

    /**
     * 检查网格是否已加载(坐标版本)
     * @param x X坐标
     * @param y Y坐标
     * @return 如果已加载返回true，否则返回false
     */
    bool IsGridLoaded(float x, float y) const
    {
        return IsGridLoaded(Acore::ComputeGridCoord(x, y));
    }

    /**
     * 检查网格是否已创建
     * @param gridCoord 网格坐标
     * @return 如果已创建返回true，否则返回false
     */
    bool IsGridCreated(GridCoord const &gridCoord) const;

    /**
     * 检查网格是否已创建(坐标版本)
     * @param x X坐标
     * @param y Y坐标
     * @return 如果已创建返回true，否则返回false
     */
    bool IsGridCreated(float x, float y) const
    {
        return IsGridCreated(Acore::ComputeGridCoord(x, y));
    }

    /**
     * 加载指定坐标的网格
     * @param x X坐标
     * @param y Y坐标
     */
    void LoadGrid(float x, float y);

    /**
     * 加载所有网格
     */
    void LoadAllGrids();

    /**
     * 加载指定范围内的网格
     * @param center 中心位置
     * @param radius 半径
     */
    void LoadGridsInRange(Position const &center, float radius);

    /**
     * 卸载指定网格
     * @param grid 网格引用
     * @return 卸载成功返回true，否则返回false
     */
    bool UnloadGrid(MapGridType &grid);

    /**
     * 卸载所有网格
     */
    virtual void UnloadAll();

    /**
     * 获取网格地形数据共享指针
     * @param gridCoord 网格坐标
     * @return 返回网格地形数据共享指针
     */
    std::shared_ptr<GridTerrainData> GetGridTerrainDataSharedPtr(GridCoord const &gridCoord);

    /**
     * 获取网格地形数据指针
     * @param gridCoord 网格坐标
     * @return 返回网格地形数据指针
     */
    GridTerrainData *GetGridTerrainData(GridCoord const &gridCoord);

    /**
     * 获取网格地形数据指针(坐标版本)
     * @param x X坐标
     * @param y Y坐标
     * @return 返回网格地形数据指针
     */
    GridTerrainData *GetGridTerrainData(float x, float y);

    /**
     * 获取地图ID
     * @return 返回当前地图ID
     */
    [[nodiscard]] uint32 GetId() const { return i_mapEntry->MapID; }

    /**
     * 获取父地图指针
     * @return 返回父地图常量指针
     */
    [[nodiscard]] Map const *GetParent() const { return m_parentMap; }

    /**
     * 获取MMap锁
     * @return 返回MMap共享互斥锁引用
     */
    [[nodiscard]] std::shared_mutex &GetMMapLock() const { return *(const_cast<std::shared_mutex *>(&MMapLock)); }

    // 延迟可见性处理相关
    std::unordered_set<Unit *> i_objectsForDelayedVisibility;

    /**
     * 处理延迟可见性
     */
    void HandleDelayedVisibility();

    /**
     * 获取指定坐标的高度
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param checkVMap 是否检查VMap(默认true)
     * @param maxSearchDist 最大搜索距离(默认DEFAULT_HEIGHT_SEARCH)
     * @return 返回高度值，如果找不到则返回INVALID_HEIGHT
     */
    [[nodiscard]] float GetHeight(float x, float y, float z, bool checkVMap = true, float maxSearchDist = DEFAULT_HEIGHT_SEARCH) const;

    /**
     * 获取网格高度
     * @param x X坐标
     * @param y Y坐标
     * @return 返回网格高度
     */
    [[nodiscard]] float GetGridHeight(float x, float y) const;

    /**
     * 获取最小高度
     * @param x X坐标
     * @param y Y坐标
     * @return 返回最小高度
     */
    [[nodiscard]] float GetMinHeight(float x, float y) const;

    /**
     * 获取指定位置的传送器
     * @param phase 相位
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param worldobject 世界对象指针(可选)
     * @return 返回传送器指针
     */
    Transport *GetTransportForPos(uint32 phase, float x, float y, float z, WorldObject *worldobject = nullptr);

    /**
     * 获取指定位置的完整地形状态
     * @param phaseMask 相位掩码
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param collisionHeight 碰撞高度
     * @param data 输出参数，存储地形状态数据
     * @param reqLiquidType 请求的液体类型(默认MAP_ALL_LIQUIDS)
     */
    void GetFullTerrainStatusForPosition(uint32 phaseMask, float x, float y, float z, float collisionHeight, PositionFullTerrainStatus &data, uint8 reqLiquidType = MAP_ALL_LIQUIDS);

    /**
     * 获取液体数据
     * @param phaseMask 相位掩码
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param collisionHeight 碰撞高度
     * @param ReqLiquidType 请求的液体类型
     * @return 返回液体数据常量引用
     */
    LiquidData const GetLiquidData(uint32 phaseMask, float x, float y, float z, float collisionHeight, uint8 ReqLiquidType);

    /**
     * 获取区域信息
     * @param phaseMask 相位掩码
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param mogpflags 输出参数，存储MOGP标志
     * @param adtId 输出参数，存储ADT ID
     * @param rootId 输出参数，存储根ID
     * @param groupId 输出参数，存储组ID
     * @return 如果成功获取返回true，否则返回false
     */
    [[nodiscard]] bool GetAreaInfo(uint32 phaseMask, float x, float y, float z, uint32 &mogpflags, int32 &adtId, int32 &rootId, int32 &groupId) const;

    /**
     * 获取区域ID
     * @param phaseMask 相位掩码
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 返回区域ID
     */
    [[nodiscard]] uint32 GetAreaId(uint32 phaseMask, float x, float y, float z) const;

    /**
     * 获取区域ID
     * @param phaseMask 相位掩码
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 返回区域ID
     */
    [[nodiscard]] uint32 GetZoneId(uint32 phaseMask, float x, float y, float z) const;

    /**
     * 获取区域和区域ID
     * @param phaseMask 相位掩码
     * @param zoneid 输出参数，存储区域ID
     * @param areaid 输出参数，存储区域ID
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     */
    void GetZoneAndAreaId(uint32 phaseMask, uint32 &zoneid, uint32 &areaid, float x, float y, float z) const;

    /**
     * 获取指定坐标的水位高度
     * @param x X坐标
     * @param y Y坐标
     * @return 返回水位高度
     */
    [[nodiscard]] float GetWaterLevel(float x, float y) const;
    /**
     * 检查指定位置是否在水中
     * @param phaseMask 相位掩码
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param collisionHeight 碰撞高度
     * @return 如果在水中返回true，否则返回false
     */
    [[nodiscard]] bool IsInWater(uint32 phaseMask, float x, float y, float z, float collisionHeight) const;
    /**
     * 检查指定位置是否在水下
     * @param phaseMask 相位掩码
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param collisionHeight 碰撞高度
     * @return 如果在水下返回true，否则返回false
     */
    [[nodiscard]] bool IsUnderWater(uint32 phaseMask, float x, float y, float z, float collisionHeight) const;
    /**
     * 检查指定位置是否有足够的水
     * @param searcher 搜索者对象指针
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 如果有足够的水返回true，否则返回false
     */
    [[nodiscard]] bool HasEnoughWater(WorldObject const *searcher, float x, float y, float z) const;
    /**
     * 根据液体数据检查是否有足够的水
     * @param searcher 搜索者对象指针
     * @param liquidData 液体数据常量引用
     * @return 如果有足够的水返回true，否则返回false
     */
    [[nodiscard]] bool HasEnoughWater(WorldObject const *searcher, LiquidData const &liquidData) const;

    /**
     * 移动移动列表中的所有生物
     */
    void MoveAllCreaturesInMoveList();
    /**
     * 移动移动列表中的所有游戏对象
     */
    void MoveAllGameObjectsInMoveList();
    /**
     * 移动移动列表中的所有动态对象
     */
    void MoveAllDynamicObjectsInMoveList();
    /**
     * 移除移除列表中的所有对象
     */
    void RemoveAllObjectsInRemoveList();
    /**
     * 移除地图中的所有玩家
     */
    virtual void RemoveAllPlayers();

    /**
     * 获取实例ID
     * @return 返回当前实例ID
     */
    [[nodiscard]] uint32 GetInstanceId() const { return i_InstanceId; }
    /**
     * 获取生成模式
     * @return 返回当前生成模式
     */
    [[nodiscard]] uint8 GetSpawnMode() const { return (i_spawnMode); }

    /**
     * 进入状态枚举
     */
    enum EnterState
    {
        CAN_ENTER = 0,
        CANNOT_ENTER_ALREADY_IN_MAP = 1,           // Player is already in the map
        CANNOT_ENTER_NO_ENTRY,                     // No map entry was found for the target map ID
        CANNOT_ENTER_UNINSTANCED_DUNGEON,          // No instance template was found for dungeon map
        CANNOT_ENTER_DIFFICULTY_UNAVAILABLE,       // Requested instance difficulty is not available for target map
        CANNOT_ENTER_NOT_IN_RAID,                  // Target instance is a raid instance and the player is not in a raid group
        CANNOT_ENTER_CORPSE_IN_DIFFERENT_INSTANCE, // Player is dead and their corpse is not in target instance
        CANNOT_ENTER_INSTANCE_BIND_MISMATCH,       // Player's permanent instance save is not compatible with their group's current instance bind
        CANNOT_ENTER_TOO_MANY_INSTANCES,           // Player has entered too many instances recently
        CANNOT_ENTER_MAX_PLAYERS,                  // Target map already has the maximum number of players allowed
        CANNOT_ENTER_ZONE_IN_COMBAT,               // A boss encounter is currently in progress on the target map
        CANNOT_ENTER_UNSPECIFIED_REASON
    };

    /**
     * 检查玩家是否不能进入地图
     * @param player 玩家指针
     * @param loginCheck 是否为登录检查(默认false)
     * @return 返回进入状态枚举值
     */
    virtual EnterState CannotEnter(Player * /*player*/, bool /*loginCheck = false*/) { return CAN_ENTER; }

    /**
     * 获取地图名称
     * @return 返回地图名称的常量指针
     */
    [[nodiscard]] const char *GetMapName() const;

    // have meaning only for instanced map (that have set real difficulty)
    /**
     * 获取地图难度
     * @return 返回地图难度枚举值
     */
    [[nodiscard]] Difficulty GetDifficulty() const { return Difficulty(GetSpawnMode()); }
    /**
     * 检查是否为普通难度
     * @return 如果是普通难度返回true，否则返回false
     */
    [[nodiscard]] bool IsRegularDifficulty() const { return GetDifficulty() == REGULAR_DIFFICULTY; }
    /**
     * 获取地图难度信息
     * @return 返回地图难度信息的常量指针
     */
    [[nodiscard]] MapDifficulty const *GetMapDifficulty() const;

    /**
     * 检查地图是否可实例化
     * @return 如果可实例化返回true，否则返回false
     */
    [[nodiscard]] bool Instanceable() const { return i_mapEntry && i_mapEntry->Instanceable(); }
    /**
     * 检查地图是否为地下城
     * @return 如果是地下城返回true，否则返回false
     */
    [[nodiscard]] bool IsDungeon() const { return i_mapEntry && i_mapEntry->IsDungeon(); }
    /**
     * 检查地图是否为非团队地下城
     * @return 如果是非团队地下城返回true，否则返回false
     */
    [[nodiscard]] bool IsNonRaidDungeon() const { return i_mapEntry && i_mapEntry->IsNonRaidDungeon(); }
    /**
     * 检查地图是否为团队副本
     * @return 如果是团队副本返回true，否则返回false
     */
    [[nodiscard]] bool IsRaid() const { return i_mapEntry && i_mapEntry->IsRaid(); }
    /**
     * 检查地图是否为团队副本或英雄难度地下城
     * @return 如果是团队副本或英雄难度地下城返回true，否则返回false
     */
    [[nodiscard]] bool IsRaidOrHeroicDungeon() const { return IsRaid() || i_spawnMode > DUNGEON_DIFFICULTY_NORMAL; }
    /**
     * 检查地图是否为英雄难度
     * @return 如果是英雄难度返回true，否则返回false
     */
    [[nodiscard]] bool IsHeroic() const { return IsRaid() ? i_spawnMode >= RAID_DIFFICULTY_10MAN_HEROIC : i_spawnMode >= DUNGEON_DIFFICULTY_HEROIC; }
    /**
     * 检查地图是否为25人团队副本
     * @return 如果是25人团队副本返回true，否则返回false
     */
    [[nodiscard]] bool Is25ManRaid() const { return IsRaid() && i_spawnMode & RAID_DIFFICULTY_MASK_25MAN; } // since 25man difficulties are 1 and 3, we can check them like that
    /**
     * 检查地图是否为战场
     * @return 如果是战场返回true，否则返回false
     */
    [[nodiscard]] bool IsBattleground() const { return i_mapEntry && i_mapEntry->IsBattleground(); }
    /**
     * 检查地图是否为竞技场
     * @return 如果是竞技场返回true，否则返回false
     */
    [[nodiscard]] bool IsBattleArena() const { return i_mapEntry && i_mapEntry->IsBattleArena(); }
    /**
     * 检查地图是否为战场或竞技场
     * @return 如果是战场或竞技场返回true，否则返回false
     */
    [[nodiscard]] bool IsBattlegroundOrArena() const { return i_mapEntry && i_mapEntry->IsBattlegroundOrArena(); }
    /**
     * 检查地图是否为世界地图
     * @return 如果是世界地图返回true，否则返回false
     */
    [[nodiscard]] bool IsWorldMap() const { return i_mapEntry && i_mapEntry->IsWorldMap(); }

    /**
     * 获取地图入口位置
     * @param mapid 输出参数，存储入口所在地图ID
     * @param x 输出参数，存储入口的X坐标
     * @param y 输出参数，存储入口的Y坐标
     * @return 如果获取成功返回true，否则返回false
     */
    bool GetEntrancePos(int32 &mapid, float &x, float &y)
    {
        if (!i_mapEntry)
            return false;
        return i_mapEntry->GetEntrancePos(mapid, x, y);
    }

    /**
     * 将对象添加到待移除列表
     * @param obj 待添加到移除列表的世界对象指针
     */
    void AddObjectToRemoveList(WorldObject *obj);
    /**
     * 将对象添加到待切换列表
     * @param obj 待添加到切换列表的世界对象指针
     * @param on 切换状态
     */
    void AddObjectToSwitchList(WorldObject *obj, bool on);
    /**
     * 延迟更新
     * @param diff 时间差(毫秒)
     */
    virtual void DelayedUpdate(const uint32 diff);

    /**
     * 重置所有标记的单元格
     */
    void resetMarkedCells() { marked_cells.reset(); }
    /**
     * 检查指定ID的单元格是否被标记
     * @param pCellId 单元格ID
     * @return 如果被标记返回true，否则返回false
     */
    bool isCellMarked(uint32 pCellId) { return marked_cells.test(pCellId); }
    /**
     * 标记指定ID的单元格
     * @param pCellId 单元格ID
     */
    void markCell(uint32 pCellId) { marked_cells.set(pCellId); }

    /**
     * 检查地图中是否有玩家
     * @return 如果有玩家返回true，否则返回false
     */
    [[nodiscard]] bool HavePlayers() const { return !m_mapRefMgr.IsEmpty(); }
    /**
     * 获取地图中除GM外的玩家数量
     * @return 返回除GM外的玩家数量
     */
    [[nodiscard]] uint32 GetPlayersCountExceptGMs() const;

    /**
     * 向地图中添加世界对象
     * @param obj 待添加的世界对象指针
     */
    void AddWorldObject(WorldObject *obj) { i_worldObjects.insert(obj); }
    /**
     * 从地图中移除世界对象
     * @param obj 待移除的世界对象指针
     */
    void RemoveWorldObject(WorldObject *obj) { i_worldObjects.erase(obj); }

    /**
     * 向地图中的所有玩家发送数据包
     * @param data 待发送的数据包常量指针
     */
    void SendToPlayers(WorldPacket const *data) const;

    typedef MapRefMgr PlayerList;
    /**
     * 获取地图中的玩家列表
     * @return 返回玩家列表的常量引用
     */
    [[nodiscard]] PlayerList const &GetPlayers() const { return m_mapRefMgr; }

    // per-map script storage
    /**
     * 启动地图脚本
     * @param scripts 脚本集合
     * @param id 脚本ID
     * @param source 脚本源对象指针
     * @param target 脚本目标对象指针
     */
    void ScriptsStart(std::map<uint32, std::multimap<uint32, ScriptInfo>> const &scripts, uint32 id, Object *source, Object *target);
    /**
     * 启动脚本命令
     * @param script 脚本信息常量引用
     * @param delay 延迟时间(毫秒)
     * @param source 脚本源对象指针
     * @param target 脚本目标对象指针
     */
    void ScriptCommandStart(ScriptInfo const &script, uint32 delay, Object *source, Object *target);

    // must called with AddToWorld
    /**
     * 将对象添加到活动对象列表
     * @tparam T 对象类型
     * @param obj 待添加的对象指针
     */
    template <class T>
    void AddToActive(T *obj);

    // must called with RemoveFromWorld
    /**
     * 从活动对象列表中移除对象
     * @tparam T 对象类型
     * @param obj 待移除的对象指针
     */
    template <class T>
    void RemoveFromActive(T *obj);

    /**
     * 切换对象的网格容器
     * @tparam T 对象类型
     * @param obj 待切换的对象指针
     * @param on 切换状态
     */
    template <class T>
    void SwitchGridContainers(T *obj, bool on);
    CreatureGroupHolderType CreatureGroupHolder;

    /**
     * 更新玩家迭代器
     * @param player 玩家指针
     */
    void UpdateIteratorBack(Player *player);

    /**
     * 召唤生物
     * @param entry 生物条目ID
     * @param pos 召唤位置
     * @param properties 召唤属性条目指针，默认为nullptr
     * @param duration 召唤持续时间，默认为0
     * @param summoner 召唤者对象指针，默认为nullptr
     * @param spellId 召唤法术ID，默认为0
     * @param vehId 载具ID，默认为0
     * @param visibleBySummonerOnly 是否仅召唤者可见，默认为false
     * @return 返回召唤出的临时召唤生物指针
     */
    TempSummon *SummonCreature(uint32 entry, Position const &pos, SummonPropertiesEntry const *properties = nullptr, uint32 duration = 0, WorldObject *summoner = nullptr, uint32 spellId = 0, uint32 vehId = 0, bool visibleBySummonerOnly = false);
    /**
     * 召唤游戏对象
     * @param entry 游戏对象条目ID
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param ang 朝向角度
     * @param rotation0 旋转参数0
     * @param rotation1 旋转参数1
     * @param rotation2 旋转参数2
     * @param rotation3 旋转参数3
     * @param respawnTime 重生时间，默认为0
     * @param checkTransport 是否检查传送器，默认为true
     * @return 返回召唤出的游戏对象指针
     */
    GameObject *SummonGameObject(uint32 entry, float x, float y, float z, float ang, float rotation0, float rotation1, float rotation2, float rotation3, uint32 respawnTime, bool checkTransport = true);
    /**
     * 召唤游戏对象
     * @param entry 游戏对象条目ID
     * @param pos 召唤位置
     * @param rotation0 旋转参数0，默认为0.0f
     * @param rotation1 旋转参数1，默认为0.0f
     * @param rotation2 旋转参数2，默认为0.0f
     * @param rotation3 旋转参数3，默认为0.0f
     * @param respawnTime 重生时间，默认为100
     * @param checkTransport 是否检查传送器，默认为true
     * @return 返回召唤出的游戏对象指针
     */
    GameObject *SummonGameObject(uint32 entry, Position const &pos, float rotation0 = 0.0f, float rotation1 = 0.0f, float rotation2 = 0.0f, float rotation3 = 0.0f, uint32 respawnTime = 100, bool checkTransport = true);
    /**
     * 召唤生物组
     * @param group 生物组ID
     * @param list 存储召唤生物的列表指针，默认为nullptr
     */
    void SummonCreatureGroup(uint8 group, std::list<TempSummon *> *list = nullptr);

    /**
     * 根据GUID获取尸体对象
     * @param guid 尸体对象的GUID
     * @return 返回尸体对象指针，如果未找到则返回nullptr
     */
    Corpse *GetCorpse(ObjectGuid const guid);
    /**
     * 根据GUID获取生物对象
     * @param guid 生物对象的GUID
     * @return 返回生物对象指针，如果未找到则返回nullptr
     */
    Creature *GetCreature(ObjectGuid const guid);
    /**
     * 根据GUID获取游戏对象
     * @param guid 游戏对象的GUID
     * @return 返回游戏对象指针，如果未找到则返回nullptr
     */
    GameObject *GetGameObject(ObjectGuid const guid);
    /**
     * 根据GUID获取传送器对象
     * @param guid 传送器对象的GUID
     * @return 返回传送器对象指针，如果未找到则返回nullptr
     */
    Transport *GetTransport(ObjectGuid const guid);
    /**
     * 根据GUID获取动态对象
     * @param guid 动态对象的GUID
     * @return 返回动态对象指针，如果未找到则返回nullptr
     */
    DynamicObject *GetDynamicObject(ObjectGuid const guid);
    /**
     * 根据GUID获取宠物对象
     * @param guid 宠物对象的GUID
     * @return 返回宠物对象指针，如果未找到则返回nullptr
     */
    Pet *GetPet(ObjectGuid const guid);

    /**
     * 获取地图存储对象容器
     * @return 返回地图存储对象容器的引用
     */
    MapStoredObjectTypesContainer &GetObjectsStore() { return _objectsStore; }

    typedef std::unordered_multimap<ObjectGuid::LowType, Creature *> CreatureBySpawnIdContainer;
    /**
     * 获取按生成ID存储的生物容器
     * @return 返回按生成ID存储的生物容器的引用
     */
    CreatureBySpawnIdContainer &GetCreatureBySpawnIdStore() { return _creatureBySpawnIdStore; }

    typedef std::unordered_multimap<ObjectGuid::LowType, GameObject *> GameObjectBySpawnIdContainer;
    /**
     * 获取按生成ID存储的游戏对象容器
     * @return 返回按生成ID存储的游戏对象容器的引用
     */
    GameObjectBySpawnIdContainer &GetGameObjectBySpawnIdStore() { return _gameobjectBySpawnIdStore; }

    /**
     * 获取指定单元格中的尸体集合
     * @param cellId 单元格ID
     * @return 返回指定单元格中尸体集合的常量指针，如果未找到则返回nullptr
     */
    [[nodiscard]] std::unordered_set<Corpse *> const *GetCorpsesInCell(uint32 cellId) const
    {
        auto itr = _corpsesByCell.find(cellId);
        if (itr != _corpsesByCell.end())
            return &itr->second;

        return nullptr;
    }

    /**
     * 根据所有者GUID获取尸体对象
     * @param ownerGuid 所有者的GUID
     * @return 返回尸体对象指针，如果未找到则返回nullptr
     */
    [[nodiscard]] Corpse *GetCorpseByPlayer(ObjectGuid const &ownerGuid) const
    {
        auto itr = _corpsesByPlayer.find(ownerGuid);
        if (itr != _corpsesByPlayer.end())
            return itr->second;

        return nullptr;
    }

    /**
     * 将地图对象转换为可实例化地图对象
     * @return 如果地图可实例化则返回转换后的可实例化地图对象指针，否则返回nullptr
     */
    MapInstanced *ToMapInstanced()
    {
        if (Instanceable())
            return reinterpret_cast<MapInstanced *>(this);
        else
            return nullptr;
    }
    /**
     * 将地图对象转换为可实例化地图对象（常量版本）
     * @return 如果地图可实例化则返回转换后的可实例化地图对象常量指针，否则返回nullptr
     */
    [[nodiscard]] MapInstanced const *ToMapInstanced() const
    {
        if (Instanceable())
            return (const MapInstanced *)((MapInstanced *)this);
        else
            return nullptr;
    }

    /**
     * 将地图对象转换为实例地图对象
     * @return 如果地图是地下城则返回转换后的实例地图对象指针，否则返回nullptr
     */
    InstanceMap *ToInstanceMap()
    {
        if (IsDungeon())
            return reinterpret_cast<InstanceMap *>(this);
        else
            return nullptr;
    }
    /**
     * 将地图对象转换为实例地图对象（常量版本）
     * @return 如果地图是地下城则返回转换后的实例地图对象常量指针，否则返回nullptr
     */
    [[nodiscard]] InstanceMap const *ToInstanceMap() const
    {
        if (IsDungeon())
            return (const InstanceMap *)((InstanceMap *)this);
        else
            return nullptr;
    }

    /**
     * 将地图对象转换为战场地图对象
     * @return 如果地图是战场或竞技场则返回转换后的战场地图对象指针，否则返回nullptr
     */
    BattlegroundMap *ToBattlegroundMap()
    {
        if (IsBattlegroundOrArena())
            return reinterpret_cast<BattlegroundMap *>(this);
        else
            return nullptr;
    }
    /**
     * 将地图对象转换为战场地图对象（常量版本）
     * @return 如果地图是战场或竞技场则返回转换后的战场地图对象常量指针，否则返回nullptr
     */
    [[nodiscard]] BattlegroundMap const *ToBattlegroundMap() const
    {
        if (IsBattlegroundOrArena())
            return reinterpret_cast<BattlegroundMap const *>(this);
        return nullptr;
    }

    /**
     * 获取指定位置的水面或地面高度
     * @param phasemask 相位掩码
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param ground 输出参数，存储地面高度指针，默认为nullptr
     * @param swim 是否为游泳状态，默认为false
     * @param collisionHeight 碰撞高度，默认为DEFAULT_COLLISION_HEIGHT
     * @return 返回水面或地面高度
     */
    float GetWaterOrGroundLevel(uint32 phasemask, float x, float y, float z, float *ground = nullptr, bool swim = false, float collisionHeight = DEFAULT_COLLISION_HEIGHT) const;
    /**
     * 获取指定位置的高度
     * @param phasemask 相位掩码
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param vmap 是否检查VMap，默认为true
     * @param maxSearchDist 最大搜索距离，默认为DEFAULT_HEIGHT_SEARCH
     * @return 返回指定位置的高度，如果未找到则返回INVALID_HEIGHT
     */
    [[nodiscard]] float GetHeight(uint32 phasemask, float x, float y, float z, bool vmap = true, float maxSearchDist = DEFAULT_HEIGHT_SEARCH) const;
    /**
     * 检查两点之间是否有视线
     * @param x1 起点X坐标
     * @param y1 起点Y坐标
     * @param z1 起点Z坐标
     * @param x2 终点X坐标
     * @param y2 终点Y坐标
     * @param z2 终点Z坐标
     * @param phasemask 相位掩码
     * @param checks 视线检查标志
     * @param ignoreFlags 模型忽略标志
     * @return 如果有视线返回true，否则返回false
     */
    [[nodiscard]] bool isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2, uint32 phasemask, LineOfSightChecks checks, VMAP::ModelIgnoreFlags ignoreFlags) const;
    /**
     * 检查源对象是否能到达目标位置，并获取有效的坐标
     * @param source 源对象常量指针
     * @param path 路径生成器指针
     * @param destX 输出参数，存储有效的目标X坐标
     * @param destY 输出参数，存储有效的目标Y坐标
     * @param destZ 输出参数，存储有效的目标Z坐标
     * @param failOnCollision 遇到碰撞是否失败，默认为true
     * @param failOnSlopes 遇到斜坡是否失败，默认为true
     * @return 如果能到达返回true，否则返回false
     */
    bool CanReachPositionAndGetValidCoords(WorldObject const *source, PathGenerator *path, float &destX, float &destY, float &destZ, bool failOnCollision = true, bool failOnSlopes = true) const;
    /**
     * 检查源对象是否能到达目标位置，并获取有效的坐标
     * @param source 源对象常量指针
     * @param destX 输出参数，存储有效的目标X坐标
     * @param destY 输出参数，存储有效的目标Y坐标
     * @param destZ 输出参数，存储有效的目标Z坐标
     * @param failOnCollision 遇到碰撞是否失败，默认为true
     * @param failOnSlopes 遇到斜坡是否失败，默认为true
     * @return 如果能到达返回true，否则返回false
     */
    bool CanReachPositionAndGetValidCoords(WorldObject const *source, float &destX, float &destY, float &destZ, bool failOnCollision = true, bool failOnSlopes = true) const;
    /**
     * 检查源对象从指定起点是否能到达目标位置，并获取有效的坐标
     * @param source 源对象常量指针
     * @param startX 起点X坐标
     * @param startY 起点Y坐标
     * @param startZ 起点Z坐标
     * @param destX 输出参数，存储有效的目标X坐标
     * @param destY 输出参数，存储有效的目标Y坐标
     * @param destZ 输出参数，存储有效的目标Z坐标
     * @param failOnCollision 遇到碰撞是否失败，默认为true
     * @param failOnSlopes 遇到斜坡是否失败，默认为true
     * @return 如果能到达返回true，否则返回false
     */
    bool CanReachPositionAndGetValidCoords(WorldObject const *source, float startX, float startY, float startZ, float &destX, float &destY, float &destZ, bool failOnCollision = true, bool failOnSlopes = true) const;
    /**
     * 检查源对象从指定起点移动到目标位置是否会发生碰撞，并获取有效的坐标
     * @param source 源对象常量指针
     * @param startX 起点X坐标
     * @param startY 起点Y坐标
     * @param startZ 起点Z坐标
     * @param destX 输出参数，存储有效的目标X坐标
     * @param destY 输出参数，存储有效的目标Y坐标
     * @param destZ 输出参数，存储有效的目标Z坐标
     * @param failOnCollision 遇到碰撞是否失败，默认为true
     * @return 如果未发生碰撞返回true，否则返回false
     */
    bool CheckCollisionAndGetValidCoords(WorldObject const *source, float startX, float startY, float startZ, float &destX, float &destY, float &destZ, bool failOnCollision = true) const;
    /**
     * 平衡动态树
     */
    void Balance() { _dynamicTree.balance(); }
    /**
     * 从动态树中移除游戏对象模型
     * @param model 待移除的游戏对象模型常量引用
     */
    void RemoveGameObjectModel(const GameObjectModel &model) { _dynamicTree.remove(model); }
    /**
     * 向动态树中插入游戏对象模型
     * @param model 待插入的游戏对象模型常量引用
     */
    void InsertGameObjectModel(const GameObjectModel &model) { _dynamicTree.insert(model); }
    /**
     * 检查动态树中是否包含指定的游戏对象模型
     * @param model 待检查的游戏对象模型常量引用
     * @return 如果包含返回true，否则返回false
     */
    [[nodiscard]] bool ContainsGameObjectModel(const GameObjectModel &model) const { return _dynamicTree.contains(model); }
    /**
     * 获取动态地图树
     * @return 返回动态地图树的常量引用
     */
    [[nodiscard]] DynamicMapTree const &GetDynamicMapTree() const { return _dynamicTree; }
    /**
     * 获取对象碰撞位置
     * @param phasemask 相位掩码
     * @param x1 起点X坐标
     * @param y1 起点Y坐标
     * @param z1 起点Z坐标
     * @param x2 终点X坐标
     * @param y2 终点Y坐标
     * @param z2 终点Z坐标
     * @param rx 输出参数，存储碰撞点X坐标
     * @param ry 输出参数，存储碰撞点Y坐标
     * @param rz 输出参数，存储碰撞点Z坐标
     * @param modifyDist 修正距离
     * @return 如果找到碰撞位置返回true，否则返回false
     */
    bool GetObjectHitPos(uint32 phasemask, float x1, float y1, float z1, float x2, float y2, float z2, float &rx, float &ry, float &rz, float modifyDist);
    /**
     * 获取指定位置游戏对象的地板高度
     * @param phasemask 相位掩码
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param maxSearchDist 最大搜索距离，默认为DEFAULT_HEIGHT_SEARCH
     * @return 返回游戏对象的地板高度
     */
    [[nodiscard]] float GetGameObjectFloor(uint32 phasemask, float x, float y, float z, float maxSearchDist = DEFAULT_HEIGHT_SEARCH) const
    {
        return _dynamicTree.getHeight(x, y, z, maxSearchDist, phasemask);
    }
    /*
        RESPAWN TIMES
    */
    /**
     * 获取关联对象的重生时间
     * @param guid 对象的GUID
     * @return 返回关联对象的重生时间，如果未找到则返回0
     */
    [[nodiscard]] time_t GetLinkedRespawnTime(ObjectGuid guid) const;
    /**
     * 获取生物的重生时间
     * @param dbGuid 生物的数据库GUID低位
     * @return 返回生物的重生时间，如果未找到则返回0
     */
    [[nodiscard]] time_t GetCreatureRespawnTime(ObjectGuid::LowType dbGuid) const
    {
        std::unordered_map<ObjectGuid::LowType /*dbGUID*/, time_t>::const_iterator itr = _creatureRespawnTimes.find(dbGuid);
        if (itr != _creatureRespawnTimes.end())
            return itr->second;

        return time_t(0);
    }

    /**
     * 获取游戏对象的重生时间
     * @param dbGuid 游戏对象的数据库GUID低位
     * @return 返回游戏对象的重生时间，如果未找到则返回0
     */
    [[nodiscard]] time_t GetGORespawnTime(ObjectGuid::LowType dbGuid) const
    {
        std::unordered_map<ObjectGuid::LowType /*dbGUID*/, time_t>::const_iterator itr = _goRespawnTimes.find(dbGuid);
        if (itr != _goRespawnTimes.end())
            return itr->second;

        return time_t(0);
    }

    /**
     * 保存生物的重生时间
     * @param dbGuid 生物的数据库GUID低位
     * @param respawnTime 重生时间引用
     */
    void SaveCreatureRespawnTime(ObjectGuid::LowType dbGuid, time_t &respawnTime);
    /**
     * 移除生物的重生时间记录
     * @param dbGuid 生物的数据库GUID低位
     */
    void RemoveCreatureRespawnTime(ObjectGuid::LowType dbGuid);
    /**
     * 保存游戏对象的重生时间
     * @param dbGuid 游戏对象的数据库GUID低位
     * @param respawnTime 重生时间引用
     */
    void SaveGORespawnTime(ObjectGuid::LowType dbGuid, time_t &respawnTime);
    /**
     * 移除游戏对象的重生时间记录
     * @param dbGuid 游戏对象的数据库GUID低位
     */
    void RemoveGORespawnTime(ObjectGuid::LowType dbGuid);
    /**
     * 加载重生时间记录
     */
    void LoadRespawnTimes();
    /**
     * 删除所有重生时间记录
     */
    void DeleteRespawnTimes();
    /**
     * 获取实例重置周期
     * @return 返回实例重置周期
     */
    [[nodiscard]] time_t GetInstanceResetPeriod() const { return _instanceResetPeriod; }

    /**
     * 更新玩家所在区域的统计信息
     * @param oldZone 旧区域ID
     * @param newZone 新区域ID
     */
    void UpdatePlayerZoneStats(uint32 oldZone, uint32 newZone);
    /**
     * 应用动态模式下的重生时间缩放
     * @param obj 世界对象常量指针
     * @param respawnDelay 原始重生延迟时间
     * @return 返回缩放后的重生延迟时间
     */
    [[nodiscard]] uint32 ApplyDynamicModeRespawnScaling(WorldObject const *obj, uint32 respawnDelay) const;

    TaskScheduler _creatureRespawnScheduler;

    /**
     * 调度生物重生
     * @param creatureGuid 生物的GUID
     * @param respawnTimer 重生计时器
     * @param pos 重生位置，默认为默认位置
     */
    void ScheduleCreatureRespawn(ObjectGuid /*creatureGuid*/, Milliseconds /*respawnTimer*/, Position pos = Position());

    /**
     * 加载尸体数据
     */
    void LoadCorpseData();
    /**
     * 删除尸体数据
     */
    void DeleteCorpseData();
    /**
     * 添加尸体对象
     * @param corpse 尸体对象指针
     */
    void AddCorpse(Corpse *corpse);
    /**
     * 移除尸体对象
     * @param corpse 尸体对象指针
     */
    void RemoveCorpse(Corpse *corpse);
    /**
     * 将尸体转换为白骨
     * @param ownerGuid 尸体所有者的GUID
     * @param insignia 是否保留徽章，默认为false
     * @return 返回转换后的白骨对象指针，如果转换失败则返回nullptr
     */
    Corpse *ConvertCorpseToBones(ObjectGuid const ownerGuid, bool insignia = false);
    /**
     * 移除过期的尸体
     */
    void RemoveOldCorpses();

    /**
     * 删除数据库中指定地图和实例的重生时间记录
     * @param mapId 地图ID
     * @param instanceId 实例ID
     */
    static void DeleteRespawnTimesInDB(uint16 mapId, uint32 instanceId);

    /**
     * 向玩家发送初始化传送器信息
     * @param player 玩家指针
     */
    void SendInitTransports(Player *player);
    /**
     * 向玩家发送移除传送器信息
     * @param player 玩家指针
     */
    void SendRemoveTransports(Player *player);
    /**
     * 向玩家发送区域动态信息
     * @param player 玩家指针
     */
    void SendZoneDynamicInfo(Player *player);
    /**
     * 向玩家发送自身初始化信息
     * @param player 玩家指针
     */
    void SendInitSelf(Player *player);

    /**
     * 向地图中的玩家播放直接音效
     * @param soundId 音效ID
     * @param zoneId 区域ID，默认为0
     */
    void PlayDirectSoundToMap(uint32 soundId, uint32 zoneId = 0);
    /**
     * 设置指定区域的音乐
     * @param zoneId 区域ID
     * @param musicId 音乐ID
     */
    void SetZoneMusic(uint32 zoneId, uint32 musicId);
    /**
     * 设置指定区域的天气
     * @param zoneId 区域ID
     * @param weatherId 天气状态ID
     * @param weatherGrade 天气等级
     */
    void SetZoneWeather(uint32 zoneId, WeatherState weatherId, float weatherGrade);
    /**
     * 设置指定区域的覆盖光照
     * @param zoneId 区域ID
     * @param lightId 光照ID
     * @param fadeInTime 淡入时间
     */
    void SetZoneOverrideLight(uint32 zoneId, uint32 lightId, Milliseconds fadeInTime);

    // Checks encounter state at kill/spellcast, originally in InstanceScript however not every map has instance script :(
    /**
     * 更新遭遇状态
     * @param type 遭遇信用类型
     * @param creditEntry 信用条目ID
     * @param source 触发源单位指针
     */
    void UpdateEncounterState(EncounterCreditType type, uint32 creditEntry, Unit *source);
    /**
     * 记录遭遇完成信息
     * @param type 遭遇信用类型
     * @param creditEntry 信用条目ID
     */
    void LogEncounterFinished(EncounterCreditType type, uint32 creditEntry);

    // Do whatever you want to all the players in map [including GameMasters], i.e.: param exec = [&](Player* p) { p->Whatever(); }
    /**
     * 对地图中的所有玩家执行指定操作
     * @param exec 执行操作的函数对象
     */
    void DoForAllPlayers(std::function<void(Player *)> exec);

    /**
     * 确保指定坐标的网格已创建
     * @param gridCoord 网格坐标
     */
    void EnsureGridCreated(GridCoord const &gridCoord);
    /**
     * 检查所有传送器是否为空
     * @return 如果所有传送器为空返回true，否则返回false
     */
    [[nodiscard]] bool AllTransportsEmpty() const; // pussywizard
    /**
     * 移除所有传送器上的乘客
     */
    void AllTransportsRemovePassengers(); // pussywizard
    /**
     * 获取所有传送器的容器
     * @return 返回所有传送器容器的常量引用
     */
    [[nodiscard]] TransportsContainer const &GetAllTransports() const { return _transports; }

    DataMap CustomData;

    /**
     * 生成指定高位GUID的低位GUID
     * @tparam high 高位GUID
     * @return 返回生成的低位GUID
     */
    template <HighGuid high>
    inline ObjectGuid::LowType GenerateLowGuid()
    {
        static_assert(ObjectGuidTraits<high>::MapSpecific, "Only map specific guid can be generated in Map context");
        return GetGuidSequenceGenerator<high>().Generate();
    }

    /**
     * 向更新对象集合中添加对象
     * @param obj 待添加的对象指针
     */
    void AddUpdateObject(Object *obj)
    {
        _updateObjects.insert(obj);
    }

    /**
     * 从更新对象集合中移除对象
     * @param obj 待移除的对象指针
     */
    void RemoveUpdateObject(Object *obj)
    {
        _updateObjects.erase(obj);
    }

    /**
     * 获取活动非玩家对象的数量
     * @return 返回活动非玩家对象的数量
     */
    std::size_t GetActiveNonPlayersCount() const
    {
        return m_activeNonPlayers.size();
    }

    /**
     * 获取地图的调试信息
     * @return 返回地图的调试信息字符串
     */
    virtual std::string GetDebugInfo() const;

    /**
     * 获取已创建的网格数量
     * @return 返回已创建的网格数量
     */
    uint32 GetCreatedGridsCount();
    /**
     * 获取已加载的网格数量
     * @return 返回已加载的网格数量
     */
    uint32 GetLoadedGridsCount();
    /**
     * 获取指定网格中已创建的单元格数量
     * @param x 网格X坐标
     * @param y 网格Y坐标
     * @return 返回指定网格中已创建的单元格数量
     */
    uint32 GetCreatedCellsInGridCount(uint16 const x, uint16 const y);
    /**
     * 获取地图中已创建的单元格总数
     * @return 返回地图中已创建的单元格总数
     */
    uint32 GetCreatedCellsInMapCount();

    /**
     * 将对象添加到待更新列表
     * @param obj 待添加的世界对象指针
     */
    void AddObjectToPendingUpdateList(WorldObject *obj);
    /**
     * 从地图更新列表中移除对象
     * @param obj 待移除的世界对象指针
     */
    void RemoveObjectFromMapUpdateList(WorldObject *obj);

    // 可更新对象列表类型定义，使用 vector 存储世界对象指针
    typedef std::vector<WorldObject *> UpdatableObjectList;
    // 待添加的可更新对象列表类型定义，使用无序集合存储世界对象指针
    typedef std::unordered_set<WorldObject *> PendingAddUpdatableObjectList;

private:
    /**
     * 初始化对象
     * @tparam T 对象类型
     * @param obj 对象指针
     */
    template <class T>
    void InitializeObject(T *obj);
    /**
     * 将生物添加到移动列表
     * @param c 生物指针
     */
    void AddCreatureToMoveList(Creature *c);
    /**
     * 从移动列表中移除生物
     * @param c 生物指针
     */
    void RemoveCreatureFromMoveList(Creature *c);
    /**
     * 将游戏对象添加到移动列表
     * @param go 游戏对象指针
     */
    void AddGameObjectToMoveList(GameObject *go);
    /**
     * 从移动列表中移除游戏对象
     * @param go 游戏对象指针
     */
    void RemoveGameObjectFromMoveList(GameObject *go);
    /**
     * 将动态对象添加到移动列表
     * @param go 动态对象指针
     */
    void AddDynamicObjectToMoveList(DynamicObject *go);
    /**
     * 从移动列表中移除动态对象
     * @param go 动态对象指针
     */
    void RemoveDynamicObjectFromMoveList(DynamicObject *go);

    // 待移动的生物列表
    std::vector<Creature *> _creaturesToMove;
    // 待移动的游戏对象列表
    std::vector<GameObject *> _gameObjectsToMove;
    // 待移动的动态对象列表
    std::vector<DynamicObject *> _dynamicObjectsToMove;

    /**
     * 确保指定单元格的网格已加载
     * @param cell 单元格引用
     * @return 如果网格已加载或成功加载返回 true，否则返回 false
     */
    bool EnsureGridLoaded(Cell const &cell);
    /**
     * 获取指定坐标的地图网格
     * @param x 网格 X 坐标
     * @param y 网格 Y 坐标
     * @return 地图网格指针
     */
    MapGridType *GetMapGrid(uint16 const x, uint16 const y);

    /**
     * 处理脚本逻辑
     */
    void ScriptsProcess();

    /**
     * 发送对象更新信息
     */
    void SendObjectUpdates();

protected:
    // Type specific code for add/remove to/from grid
    /**
     * 将对象添加到指定单元格的网格中
     * @tparam T 对象类型
     * @param object 对象指针
     * @param cell 单元格引用
     */
    template <class T>
    void AddToGrid(T *object, Cell const &cell);

    // 互斥锁，用于保证线程安全
    std::mutex Lock;
    // MMap 共享互斥锁，用于保证 MMap 相关操作的线程安全
    std::shared_mutex MMapLock;

    // 地图网格管理器
    MapGridManager _mapGridManager;
    // 地图条目常量指针
    MapEntry const *i_mapEntry;
    // 生成模式
    uint8 i_spawnMode;
    // 实例 ID
    uint32 i_InstanceId;
    // 卸载计时器
    uint32 m_unloadTimer;
    // 可视距离
    float m_VisibleDistance;
    // 动态地图树
    DynamicMapTree _dynamicTree;
    // 实例重置周期
    time_t _instanceResetPeriod; // pussywizard

    // 地图引用管理器
    MapRefMgr m_mapRefMgr;
    // 地图引用迭代器
    MapRefMgr::iterator m_mapRefIter;

    // 活跃的非玩家对象集合类型定义
    typedef std::set<WorldObject *> ActiveNonPlayers;
    // 活跃的非玩家对象集合
    ActiveNonPlayers m_activeNonPlayers;
    // 活跃的非玩家对象集合迭代器
    ActiveNonPlayers::iterator m_activeNonPlayersIter;

    // Objects that must update even in inactive grids without activating them
    // 即使在非活跃网格中也需要更新的传送器集合
    TransportsContainer _transports;
    // 传送器更新迭代器
    TransportsContainer::iterator _transportsUpdateIter;

private:
    /**
     * 获取脚本中的玩家源对象或目标对象
     * @param source 源对象指针
     * @param target 目标对象指针
     * @param scriptInfo 脚本信息常量指针
     * @return 玩家指针，如果未找到则返回 nullptr
     */
    Player *_GetScriptPlayerSourceOrTarget(Object *source, Object *target, const ScriptInfo *scriptInfo) const;
    /**
     * 获取脚本中的生物源对象或目标对象
     * @param source 源对象指针
     * @param target 目标对象指针
     * @param scriptInfo 脚本信息常量指针
     * @param bReverse 是否反转查找逻辑，默认为 false
     * @return 生物指针，如果未找到则返回 nullptr
     */
    Creature *_GetScriptCreatureSourceOrTarget(Object *source, Object *target, const ScriptInfo *scriptInfo, bool bReverse = false) const;
    /**
     * 获取脚本中的单位对象
     * @param obj 对象指针
     * @param isSource 是否为源对象
     * @param scriptInfo 脚本信息常量指针
     * @return 单位指针，如果未找到则返回 nullptr
     */
    Unit *_GetScriptUnit(Object *obj, bool isSource, const ScriptInfo *scriptInfo) const;
    /**
     * 获取脚本中的玩家对象
     * @param obj 对象指针
     * @param isSource 是否为源对象
     * @param scriptInfo 脚本信息常量指针
     * @return 玩家指针，如果未找到则返回 nullptr
     */
    Player *_GetScriptPlayer(Object *obj, bool isSource, const ScriptInfo *scriptInfo) const;
    /**
     * 获取脚本中的生物对象
     * @param obj 对象指针
     * @param isSource 是否为源对象
     * @param scriptInfo 脚本信息常量指针
     * @return 生物指针，如果未找到则返回 nullptr
     */
    Creature *_GetScriptCreature(Object *obj, bool isSource, const ScriptInfo *scriptInfo) const;
    /**
     * 获取脚本中的世界对象
     * @param obj 对象指针
     * @param isSource 是否为源对象
     * @param scriptInfo 脚本信息常量指针
     * @return 世界对象指针，如果未找到则返回 nullptr
     */
    WorldObject *_GetScriptWorldObject(Object *obj, bool isSource, const ScriptInfo *scriptInfo) const;
    /**
     * 处理脚本中的门对象
     * @param source 源对象指针
     * @param target 目标对象指针
     * @param scriptInfo 脚本信息常量指针
     */
    void _ScriptProcessDoor(Object *source, Object *target, const ScriptInfo *scriptInfo) const;
    /**
     * 查找指定世界对象附近的游戏对象
     * @param pWorldObject 世界对象指针
     * @param guid 游戏对象低阶 GUID
     * @return 游戏对象指针，如果未找到则返回 nullptr
     */
    GameObject *_FindGameObject(WorldObject *pWorldObject, ObjectGuid::LowType guid) const;

    // used for fast base_map (e.g. MapInstanced class object) search for
    // InstanceMaps and BattlegroundMaps...
    //  父地图指针，用于快速查找基础地图
    Map *m_parentMap;

    // 标记的单元格位集合
    std::bitset<TOTAL_NUMBER_OF_CELLS_PER_MAP * TOTAL_NUMBER_OF_CELLS_PER_MAP> marked_cells;

    // 脚本锁标志
    bool i_scriptLock;
    // 待移除的世界对象集合
    std::unordered_set<WorldObject *> i_objectsToRemove;
    // 待切换的世界对象映射，值表示切换状态
    std::map<WorldObject *, bool> i_objectsToSwitch;
    // 世界对象集合
    std::unordered_set<WorldObject *> i_worldObjects;

    // 脚本调度映射类型定义，键为时间，值为脚本动作
    typedef std::multimap<time_t, ScriptAction> ScriptScheduleMap;
    // 脚本调度映射
    ScriptScheduleMap m_scriptSchedule;

    /**
     * 从世界中删除对象
     * @tparam T 对象类型
     * @param obj 对象指针
     */
    template <class T>
    void DeleteFromWorld(T *);

    /**
     * 将世界对象添加到活跃对象集合中
     * @param obj 世界对象指针
     */
    void AddToActiveHelper(WorldObject *obj)
    {
        m_activeNonPlayers.insert(obj);
    }

    /**
     * 从活跃对象集合中移除世界对象
     * @param obj 世界对象指针
     */
    void RemoveFromActiveHelper(WorldObject *obj)
    {
        // Map::Update for active object in proccess
        if (m_activeNonPlayersIter != m_activeNonPlayers.end())
        {
            ActiveNonPlayers::iterator itr = m_activeNonPlayers.find(obj);
            if (itr == m_activeNonPlayers.end())
                return;
            if (itr == m_activeNonPlayersIter)
                ++m_activeNonPlayersIter;
            m_activeNonPlayers.erase(itr);
        }
        else
            m_activeNonPlayers.erase(obj);
    }

    /**
     * 更新非玩家对象
     * @param diff 时间差(毫秒)
     */
    void UpdateNonPlayerObjects(uint32 const diff);

    /**
     * 将对象添加到更新列表中
     * @param obj 世界对象指针
     */
    void _AddObjectToUpdateList(WorldObject *obj);
    /**
     * 从更新列表中移除对象
     * @param obj 世界对象指针
     */
    void _RemoveObjectFromUpdateList(WorldObject *obj);

    // 生物重生时间映射，键为生物数据库 GUID，值为重生时间
    std::unordered_map<ObjectGuid::LowType /*dbGUID*/, time_t> _creatureRespawnTimes;
    // 游戏对象重生时间映射，键为游戏对象数据库 GUID，值为重生时间
    std::unordered_map<ObjectGuid::LowType /*dbGUID*/, time_t> _goRespawnTimes;

    // 区域玩家数量映射，键为区域 ID，值为玩家数量
    std::unordered_map<uint32, uint32> _zonePlayerCountMap;

    // 区域动态信息映射
    ZoneDynamicInfoMap _zoneDynamicInfo;
    // 默认光照 ID
    uint32 _defaultLight;

    /**
     * 获取指定高阶 GUID 的 GUID 序列生成器
     * @tparam high 高阶 GUID
     * @return GUID 序列生成器引用
     */
    template <HighGuid high>
    inline ObjectGuidGeneratorBase &GetGuidSequenceGenerator()
    {
        auto itr = _guidGenerators.find(high);
        if (itr == _guidGenerators.end())
            itr = _guidGenerators.insert(std::make_pair(high, std::unique_ptr<ObjectGuidGenerator<high>>(new ObjectGuidGenerator<high>()))).first;

        return *itr->second;
    }

    // GUID 生成器映射，键为高阶 GUID，值为 GUID 生成器指针
    std::map<HighGuid, std::unique_ptr<ObjectGuidGeneratorBase>> _guidGenerators;
    // 地图存储对象类型容器
    MapStoredObjectTypesContainer _objectsStore;
    // 按生成 ID 存储的生物容器
    CreatureBySpawnIdContainer _creatureBySpawnIdStore;
    // 按生成 ID 存储的游戏对象容器
    GameObjectBySpawnIdContainer _gameobjectBySpawnIdStore;
    // 按单元格 ID 存储的尸体集合映射
    std::unordered_map<uint32 /*cellId*/, std::unordered_set<Corpse *>> _corpsesByCell;
    // 按玩家 GUID 存储的尸体映射
    std::unordered_map<ObjectGuid, Corpse *> _corpsesByPlayer;
    // 尸体骨头集合
    std::unordered_set<Corpse *> _corpseBones;

    // 待更新的对象集合
    std::unordered_set<Object *> _updateObjects;

    // 可更新对象列表
    UpdatableObjectList _updatableObjectList;
    // 待添加的可更新对象列表
    PendingAddUpdatableObjectList _pendingAddUpdatableObjectList;
    // 可更新对象列表重新检查计时器
    IntervalTimer _updatableObjectListRecheckTimer;
};

/**
 * 实例重置方法枚举
 */
enum InstanceResetMethod
{
    INSTANCE_RESET_ALL,               // reset all option under portrait, resets only normal 5-mans
    INSTANCE_RESET_CHANGE_DIFFICULTY, // on changing difficulty
    INSTANCE_RESET_GLOBAL,            // global id reset
    INSTANCE_RESET_GROUP_JOIN,        // on joining group
    INSTANCE_RESET_GROUP_LEAVE        // on leaving group
};

/**
 * 实例地图类，继承自 Map 类
 */
class InstanceMap : public Map
{
public:
    /**
     * 构造函数
     * @param id 地图 ID
     * @param InstanceId 实例 ID
     * @param SpawnMode 生成模式
     * @param _parent 父地图指针
     */
    InstanceMap(uint32 id, uint32 InstanceId, uint8 SpawnMode, Map *_parent);
    /**
     * 析构函数
     */
    ~InstanceMap() override;
    /**
     * 将玩家添加到实例地图
     * @param player 玩家指针
     * @return 添加成功返回 true，否则返回 false
     */
    bool AddPlayerToMap(Player *) override;
    /**
     * 从实例地图移除玩家
     * @param player 玩家指针
     * @param 是否立即移除标志
     */
    void RemovePlayerFromMap(Player *, bool) override;
    /**
     * 玩家从实例地图解除链接后的处理
     */
    void AfterPlayerUnlinkFromMap() override;
    /**
     * 更新实例地图
     * @param 参数1 第一个时间参数
     * @param 参数2 第二个时间参数
     * @param thread 是否在单独线程中更新，默认为 true
     */
    void Update(const uint32, const uint32, bool thread = true) override;
    /**
     * 创建实例脚本
     * @param load 是否加载脚本
     * @param data 脚本数据
     * @param completedEncounterMask 已完成的遭遇战掩码
     */
    void CreateInstanceScript(bool load, std::string data, uint32 completedEncounterMask);
    /**
     * 重置实例地图
     * @param method 重置方法
     * @param globalSkipList 全局跳过列表指针，默认为 nullptr
     * @return 重置成功返回 true，否则返回 false
     */
    bool Reset(uint8 method, GuidList *globalSkipList = nullptr);
    /**
     * 获取脚本 ID
     * @return 脚本 ID
     */
    [[nodiscard]] uint32 GetScriptId() const { return i_script_id; }
    /**
     * 获取脚本名称
     * @return 脚本名称常量引用
     */
    [[nodiscard]] std::string const &GetScriptName() const;
    /**
     * 获取实例脚本指针
     * @return 实例脚本指针
     */
    [[nodiscard]] InstanceScript *GetInstanceScript() { return instance_data; }
    /**
     * 获取实例脚本常量指针
     * @return 实例脚本常量指针
     */
    [[nodiscard]] InstanceScript const *GetInstanceScript() const { return instance_data; }
    /**
     * 永久绑定所有玩家
     */
    void PermBindAllPlayers();
    /**
     * 卸载所有网格
     */
    void UnloadAll() override;
    /**
     * 检查玩家是否不能进入实例地图
     * @param player 玩家指针
     * @param loginCheck 是否为登录检查，默认为 false
     * @return 进入状态枚举值
     */
    EnterState CannotEnter(Player *player, bool loginCheck = false) override;
    /**
     * 发送重置警告信息
     * @param timeLeft 剩余时间(毫秒)
     */
    void SendResetWarnings(uint32 timeLeft) const;

    /**
     * 获取最大玩家数量
     * @return 最大玩家数量
     */
    [[nodiscard]] uint32 GetMaxPlayers() const;
    /**
     * 获取最大重置延迟时间
     * @return 最大重置延迟时间
     */
    [[nodiscard]] uint32 GetMaxResetDelay() const;

    /**
     * 初始化可视距离
     */
    void InitVisibilityDistance() override;

    /**
     * 获取调试信息
     * @return 调试信息字符串
     */
    std::string GetDebugInfo() const override;

private:
    // 卸载后是否重置标志
    bool m_resetAfterUnload;
    // 空实例时是否卸载标志
    bool m_unloadWhenEmpty;
    // 实例脚本指针
    InstanceScript *instance_data;
    // 脚本 ID
    uint32 i_script_id;
};

/**
 * 战场地图类，继承自 Map 类
 */
class BattlegroundMap : public Map
{
public:
    /**
     * 构造函数
     * @param id 地图 ID
     * @param InstanceId 实例 ID
     * @param _parent 父地图指针
     * @param spawnMode 生成模式
     */
    BattlegroundMap(uint32 id, uint32 InstanceId, Map *_parent, uint8 spawnMode);
    /**
     * 析构函数
     */
    ~BattlegroundMap() override;

    /**
     * 将玩家添加到战场地图
     * @param player 玩家指针
     * @return 添加成功返回 true，否则返回 false
     */
    bool AddPlayerToMap(Player *) override;
    /**
     * 从战场地图移除玩家
     * @param player 玩家指针
     * @param 是否立即移除标志
     */
    void RemovePlayerFromMap(Player *, bool) override;
    /**
     * 检查玩家是否不能进入战场地图
     * @param player 玩家指针
     * @param loginCheck 是否为登录检查，默认为 false
     * @return 进入状态枚举值
     */
    EnterState CannotEnter(Player *player, bool loginCheck = false) override;
    /**
     * 设置卸载标志
     */
    void SetUnload();
    // void UnloadAll(bool pForce);
    /**
     * 移除战场地图中的所有玩家
     */
    void RemoveAllPlayers() override;

    /**
     * 初始化可视距离
     */
    void InitVisibilityDistance() override;
    /**
     * 获取战场指针
     * @return 战场指针
     */
    Battleground *GetBG() { return m_bg; }
    /**
     * 设置战场指针
     * @param bg 战场指针
     */
    void SetBG(Battleground *bg) { m_bg = bg; }

private:
    // 战场指针
    Battleground *m_bg;
};

/**
 * 访问指定单元格中的对象
 * @tparam T 对象类型
 * @tparam CONTAINER 容器类型
 * @param cell 单元格引用
 * @param visitor 类型容器访问器引用
 */
template <class T, class CONTAINER>
inline void Map::Visit(Cell const &cell, TypeContainerVisitor<T, CONTAINER> &visitor)
{
    uint32 const grid_x = cell.GridX();
    uint32 const grid_y = cell.GridY();

    // If grid is not loaded, nothing to visit.
    if (!IsGridLoaded(GridCoord(grid_x, grid_y)))
        return;

    GetMapGrid(grid_x, grid_y)->VisitCell(cell.CellX(), cell.CellY(), visitor);
}

#endif
