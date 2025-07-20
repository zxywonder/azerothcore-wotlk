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

/// \addtogroup world The World
/// @{
/// \file

#ifndef __WORLD_H
#define __WORLD_H

#include "DatabaseEnvFwd.h"
#include "IWorld.h"
#include "LockedQueue.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"
#include "Timer.h"
#include <atomic>
#include <list>
#include <map>
#include <unordered_map>

class Object;
class WorldPacket;
class WorldSocket;
class SystemMgr;

struct Realm;

AC_GAME_API extern Realm realm;

/// 枚举：服务器关闭标志
enum ShutdownMask : uint8
{
    SHUTDOWN_MASK_RESTART = 1, // 重启标志
    SHUTDOWN_MASK_IDLE    = 2, // 空闲关闭标志
};

/// 枚举：服务器退出代码
enum ShutdownExitCode : uint8
{
    SHUTDOWN_EXIT_CODE = 0, // 正常关闭
    ERROR_EXIT_CODE    = 1, // 错误退出
    RESTART_EXIT_CODE  = 2, // 重启
};

/// 不同对象刷新率的计时器枚举
enum WorldTimers
{
    WUPDATE_WEATHERS,           // 天气更新
    WUPDATE_UPTIME,             // 服务器运行时间更新
    WUPDATE_CORPSES,            // 尸体清理
    WUPDATE_EVENTS,             // 事件更新
    WUPDATE_CLEANDB,            // 数据库清理
    WUPDATE_AUTOBROADCAST,      // 自动广播
    WUPDATE_MAILBOXQUEUE,       // 邮件队列处理
    WUPDATE_PINGDB,             // 数据库心跳
    WUPDATE_5_SECS,             // 每5秒更新
    WUPDATE_WHO_LIST,           // 玩家列表更新
    WUPDATE_COUNT               // 计时器总数
};

/// 可用于 SMSG_AUTH_RESPONSE 数据包的计费计划标志
enum BillingPlanFlags
{
    SESSION_NONE            = 0x00, // 无
    SESSION_UNUSED          = 0x01, // 未使用
    SESSION_RECURRING_BILL  = 0x02, // 定期计费
    SESSION_FREE_TRIAL      = 0x04, // 免费试用
    SESSION_IGR             = 0x08, // IGR（游戏内充值）
    SESSION_USAGE           = 0x10, // 使用情况
    SESSION_TIME_MIXTURE    = 0x20, // 时间混合
    SESSION_RESTRICTED      = 0x40, // 受限
    SESSION_ENABLE_CAIS     = 0x80, // 启用 CAIS
};

/// 服务器区域枚举
enum RealmZone
{
    REALM_ZONE_UNKNOWN       = 0,  // 未知区域，支持任何语言
    REALM_ZONE_DEVELOPMENT   = 1,  // 开发区域，支持任何语言
    REALM_ZONE_UNITED_STATES = 2,  // 美国，扩展拉丁语
    REALM_ZONE_OCEANIC       = 3,  // 澳洲，扩展拉丁语
    REALM_ZONE_LATIN_AMERICA = 4,  // 拉丁美洲，扩展拉丁语
    REALM_ZONE_TOURNAMENT_5  = 5,  // 比赛区域，创建时使用基础拉丁语，登录时支持任何语言
    REALM_ZONE_KOREA         = 6,  // 韩国，东亚语言
    REALM_ZONE_TOURNAMENT_7  = 7,  // 比赛区域，同上
    REALM_ZONE_ENGLISH       = 8,  // 英语区域，扩展拉丁语
    REALM_ZONE_GERMAN        = 9,  // 德语区域，扩展拉丁语
    REALM_ZONE_FRENCH        = 10, // 法语区域，扩展拉丁语
    REALM_ZONE_SPANISH       = 11, // 西班牙语区域，扩展拉丁语
    REALM_ZONE_RUSSIAN       = 12, // 俄语区域，西里尔语
    REALM_ZONE_TOURNAMENT_13 = 13, // 比赛区域
    REALM_ZONE_TAIWAN        = 14, // 台湾，东亚语言
    REALM_ZONE_TOURNAMENT_15 = 15, // 比赛区域
    REALM_ZONE_CHINA         = 16, // 中国，东亚语言
    REALM_ZONE_CN1           = 17, // 中国区域1
    REALM_ZONE_CN2           = 18, // 中国区域2
    REALM_ZONE_CN3           = 19, // 中国区域3
    REALM_ZONE_CN4           = 20, // 中国区域4
    REALM_ZONE_CN5           = 21, // 中国区域5
    REALM_ZONE_CN6           = 22, // 中国区域6
    REALM_ZONE_CN7           = 23, // 中国区域7
    REALM_ZONE_CN8           = 24, // 中国区域8
    REALM_ZONE_TOURNAMENT_25 = 25, // 比赛区域
    REALM_ZONE_TEST_SERVER   = 26, // 测试服务器
    REALM_ZONE_TOURNAMENT_27 = 27, // 比赛区域
    REALM_ZONE_QA_SERVER     = 28, // QA 测试服务器
    REALM_ZONE_CN9           = 29, // 中国区域9
    REALM_ZONE_TEST_SERVER_2 = 30, // 测试服务器2
    REALM_ZONE_CN10          = 31, // 中国区域10
    REALM_ZONE_CTC           = 32, // CTC
    REALM_ZONE_CNC           = 33, // CNC
    REALM_ZONE_CN1_4         = 34, // 中国组合区域1/4
    REALM_ZONE_CN2_6_9       = 35, // 中国组合区域2/6/9
    REALM_ZONE_CN3_7         = 36, // 中国组合区域3/7
    REALM_ZONE_CN5_8         = 37  // 中国组合区域5/8
};

// xinef: 请愿数据结构
struct PetitionData
{
};

/// 世界类，负责全局游戏状态管理
class World: public IWorld
{
public:
    /// 构造函数
    World();

    /// 析构函数
    ~World() override;

    /// 获取世界单例实例
    static World* instance();

    /// 全局循环计数器
    static uint32 m_worldLoopCounter;

    /// 判断是否拒绝客户端连接
    [[nodiscard]] bool IsClosed() const override;

    /// 设置服务器是否关闭
    void SetClosed(bool val) override;

    /// 获取玩家允许的最低安全等级
    [[nodiscard]] AccountTypes GetPlayerSecurityLimit() const override { return _allowedSecurityLevel; }

    /// 设置玩家允许的最低安全等级
    void SetPlayerSecurityLimit(AccountTypes sec) override;

    /// 从数据库加载允许的安全等级
    void LoadDBAllowedSecurityLevel() override;

    /// 判断是否允许对象移动
    [[nodiscard]] bool getAllowMovement() const override { return _allowMovement; }

    /// 设置是否允许对象移动
    void SetAllowMovement(bool allow) override { _allowMovement = allow; }

    /// 获取默认的DBC本地化设置
    [[nodiscard]] LocaleConstant GetDefaultDbcLocale() const override { return _defaultDbcLocale; }

    /// 获取数据存储路径（如dbc, maps）
    [[nodiscard]] std::string const& GetDataPath() const override { return _dataPath; }

    /// 获取下次每日任务重置时间
    [[nodiscard]] Seconds GetNextDailyQuestsResetTime() const override { return _nextDailyQuestReset; }

    /// 获取下次每周任务重置时间
    [[nodiscard]] Seconds GetNextWeeklyQuestsResetTime() const override { return _nextWeeklyQuestReset; }

    /// 获取下次随机战场重置时间
    [[nodiscard]] Seconds GetNextRandomBGResetTime() const override { return _nextRandomBGReset; }

    /// 获取玩家可达到的最大技能等级
    [[nodiscard]] uint16 GetConfigMaxSkillValue() const override
    {
        uint16 lvl = uint16(getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
        return lvl > 60 ? 300 + ((lvl - 60) * 75) / 10 : lvl * 5;
    }

    /// 初始化世界设置
    void SetInitialWorldSettings() override;

    /// 加载配置设置（支持重载）
    void LoadConfigSettings(bool reload = false) override;

    /// 判断服务器是否正在关闭
    [[nodiscard]] bool IsShuttingDown() const override { return _shutdownTimer > 0; }

    /// 获取剩余关闭时间
    [[nodiscard]] uint32 GetShutDownTimeLeft() const override { return _shutdownTimer; }

    /// 启动服务器关闭流程
    void ShutdownServ(uint32 time, uint32 options, uint8 exitcode, std::string const& reason = std::string()) override;

    /// 取消服务器关闭
    void ShutdownCancel() override;

    /// 显示关闭消息
    void ShutdownMsg(bool show = false, Player* player = nullptr, std::string const& reason = std::string()) override;

    /// 获取退出代码
    static uint8 GetExitCode() { return _exitCode; }

    /// 立即停止服务器
    static void StopNow(uint8 exitcode) { _stopEvent = true; _exitCode = exitcode; }

    /// 判断服务器是否已停止
    static bool IsStopped() { return _stopEvent; }

    /// 更新世界状态
    void Update(uint32 diff) override;

    /// 设置服务器配置浮点值
    void setRate(ServerConfigs index, float value) override;

    /// 获取服务器配置浮点值
    float getRate(ServerConfigs index) const override;

    /// 设置布尔配置
    void setBoolConfig(ServerConfigs index, bool value) override;

    /// 获取布尔配置
    bool getBoolConfig(ServerConfigs index) const override;

    /// 设置浮点配置
    void setFloatConfig(ServerConfigs index, float value) override;

    /// 获取浮点配置
    float getFloatConfig(ServerConfigs index) const override;

    /// 设置整数配置
    void setIntConfig(ServerConfigs index, uint32 value) override;

    /// 获取整数配置
    uint32 getIntConfig(ServerConfigs index) const override;

    /// 设置字符串配置
    void setStringConfig(ServerConfigs index, std::string const& value) override;

    /// 获取字符串配置
    std::string_view getStringConfig(ServerConfigs index) const override;

    /// 判断是否为PvP服务器
    [[nodiscard]] bool IsPvPRealm() const override;

    /// 判断是否为FFA PvP服务器
    [[nodiscard]] bool IsFFAPvPRealm() const override;

    // 获取大陆最大可见距离
    static float GetMaxVisibleDistanceOnContinents()    { return _maxVisibleDistanceOnContinents; }

    // 获取实例最大可见距离
    static float GetMaxVisibleDistanceInInstances()     { return _maxVisibleDistanceInInstances;  }

    // 获取战场和竞技场最大可见距离
    static float GetMaxVisibleDistanceInBGArenas()      { return _maxVisibleDistanceInBGArenas;   }

    // 获取下一次Who列表更新延迟
    uint32 GetNextWhoListUpdateDelaySecs() override;

    /// 处理命令行指令
    void ProcessCliCommands() override;

    /// 添加命令行指令到队列
    void QueueCliCommand(CliCommandHolder* commandHolder) override { _cliCmdQueue.add(commandHolder); }

    /// 强制游戏事件更新
    void ForceGameEventUpdate() override;

    /// 更新账号角色数量
    void UpdateRealmCharCount(uint32 accid) override;

    /// 获取可用的DBC本地化设置
    [[nodiscard]] LocaleConstant GetAvailableDbcLocale(LocaleConstant locale) const override { if (_availableDbcLocaleMask & (1 << locale)) return locale; else return _defaultDbcLocale; }

    /// 加载数据库版本
    void LoadDBVersion() override;

    /// 获取数据库版本
    [[nodiscard]] char const* GetDBVersion() const override { return _dbVersion.c_str(); }

    /// 更新区域相关的光环
    void UpdateAreaDependentAuras() override;

    /// 获取清理标志
    [[nodiscard]] uint32 GetCleaningFlags() const override { return _cleaningFlags; }

    /// 设置清理标志
    void   SetCleaningFlags(uint32 flags) override { _cleaningFlags = flags; }

    /// 重置事件相关的季节性任务
    void   ResetEventSeasonalQuests(uint16 event_id) override;

    /// 获取服务器名称
    [[nodiscard]] std::string const& GetRealmName() const override { return _realmName; }

    /// 设置服务器名称
    void SetRealmName(std::string name) override { _realmName = name; }

    /// 移除过期的尸体
    void RemoveOldCorpses() override;

protected:
    /// 更新游戏时间
    void _UpdateGameTime();

    /// 更新账号角色数量回调
    void _UpdateRealmCharCount(PreparedQueryResult resultCharCount,uint32 accountId);

    /// 初始化每日任务重置时间
    void InitDailyQuestResetTime();

    /// 初始化每周任务重置时间
    void InitWeeklyQuestResetTime();

    /// 初始化每月任务重置时间
    void InitMonthlyQuestResetTime();

    /// 初始化随机战场重置时间
    void InitRandomBGResetTime();

    /// 初始化日历旧事件删除时间
    void InitCalendarOldEventsDeletionTime();

    /// 初始化公会重置时间
    void InitGuildResetTime();

    /// 重置每日任务
    void ResetDailyQuests();

    /// 重置每周任务
    void ResetWeeklyQuests();

    /// 重置每月任务
    void ResetMonthlyQuests();

    /// 重置随机战场
    void ResetRandomBG();

    /// 删除日历中的旧事件
    void CalendarDeleteOldEvents();

    /// 重置公会上限
    void ResetGuildCap();

private:
    WorldConfig _worldConfig;

    static std::atomic_long _stopEvent; // 停止事件标志
    static uint8 _exitCode; // 退出代码
    uint32 _shutdownTimer; // 关闭倒计时
    uint32 _shutdownMask; // 关闭标志
    std::string _shutdownReason; // 关闭原因

    uint32 _cleaningFlags; // 清理标志

    bool _isClosed; // 是否关闭

    IntervalTimer _timers[WUPDATE_COUNT]; // 各类更新计时器
    Seconds _mail_expire_check_timer; // 邮件过期检查时间

    AccountTypes _allowedSecurityLevel; // 允许的最低安全等级
    LocaleConstant _defaultDbcLocale;                     // 默认DBC本地化设置
    uint32 _availableDbcLocaleMask;                       // 可用DBC本地化掩码
    void DetectDBCLang(); // 检测DBC语言
    bool _allowMovement; // 是否允许移动
    std::string _dataPath; // 数据路径

    // 最大可视距离（快速访问）
    static float _maxVisibleDistanceOnContinents;
    static float _maxVisibleDistanceInInstances;
    static float _maxVisibleDistanceInBGArenas;

    std::string _realmName; // 服务器名称

    // CLI命令队列（线程安全）
    LockedQueue<CliCommandHolder*> _cliCmdQueue;

    // 下次任务、战场等重置时间
    Seconds _nextDailyQuestReset;
    Seconds _nextWeeklyQuestReset;
    Seconds _nextMonthlyQuestReset;
    Seconds _nextRandomBGReset;
    Seconds _nextCalendarOldEventsDeletionTime;
    Seconds _nextGuildReset;

    // 使用的数据库版本
    std::string _dbVersion;
    uint32 _dbClientCacheVersion;

    // 处理数据库查询回调
    void ProcessQueryCallbacks();
    QueryCallbackProcessor _queryProcessor;

    /**
     * @brief 当World会话完成时调用，无论是正常登录还是队列弹出
     *
     * @param session 即将完成的World会话
     */
    inline void FinalizePlayerWorldSession(WorldSession* session);
};

std::unique_ptr<IWorld>& getWorldInstance();
#define sWorld getWorldInstance()

#endif
/// @}