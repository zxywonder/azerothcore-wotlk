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

/// ö�٣��������رձ�־
enum ShutdownMask : uint8
{
    SHUTDOWN_MASK_RESTART = 1, // ������־
    SHUTDOWN_MASK_IDLE    = 2, // ���йرձ�־
};

/// ö�٣��������˳�����
enum ShutdownExitCode : uint8
{
    SHUTDOWN_EXIT_CODE = 0, // �����ر�
    ERROR_EXIT_CODE    = 1, // �����˳�
    RESTART_EXIT_CODE  = 2, // ����
};

/// ��ͬ����ˢ���ʵļ�ʱ��ö��
enum WorldTimers
{
    WUPDATE_WEATHERS,           // ��������
    WUPDATE_UPTIME,             // ����������ʱ�����
    WUPDATE_CORPSES,            // ʬ������
    WUPDATE_EVENTS,             // �¼�����
    WUPDATE_CLEANDB,            // ���ݿ�����
    WUPDATE_AUTOBROADCAST,      // �Զ��㲥
    WUPDATE_MAILBOXQUEUE,       // �ʼ����д���
    WUPDATE_PINGDB,             // ���ݿ�����
    WUPDATE_5_SECS,             // ÿ5�����
    WUPDATE_WHO_LIST,           // ����б����
    WUPDATE_COUNT               // ��ʱ������
};

/// ������ SMSG_AUTH_RESPONSE ���ݰ��ļƷѼƻ���־
enum BillingPlanFlags
{
    SESSION_NONE            = 0x00, // ��
    SESSION_UNUSED          = 0x01, // δʹ��
    SESSION_RECURRING_BILL  = 0x02, // ���ڼƷ�
    SESSION_FREE_TRIAL      = 0x04, // �������
    SESSION_IGR             = 0x08, // IGR����Ϸ�ڳ�ֵ��
    SESSION_USAGE           = 0x10, // ʹ�����
    SESSION_TIME_MIXTURE    = 0x20, // ʱ����
    SESSION_RESTRICTED      = 0x40, // ����
    SESSION_ENABLE_CAIS     = 0x80, // ���� CAIS
};

/// ����������ö��
enum RealmZone
{
    REALM_ZONE_UNKNOWN       = 0,  // δ֪����֧���κ�����
    REALM_ZONE_DEVELOPMENT   = 1,  // ��������֧���κ�����
    REALM_ZONE_UNITED_STATES = 2,  // ��������չ������
    REALM_ZONE_OCEANIC       = 3,  // ���ޣ���չ������
    REALM_ZONE_LATIN_AMERICA = 4,  // �������ޣ���չ������
    REALM_ZONE_TOURNAMENT_5  = 5,  // �������򣬴���ʱʹ�û����������¼ʱ֧���κ�����
    REALM_ZONE_KOREA         = 6,  // ��������������
    REALM_ZONE_TOURNAMENT_7  = 7,  // ��������ͬ��
    REALM_ZONE_ENGLISH       = 8,  // Ӣ��������չ������
    REALM_ZONE_GERMAN        = 9,  // ����������չ������
    REALM_ZONE_FRENCH        = 10, // ����������չ������
    REALM_ZONE_SPANISH       = 11, // ��������������չ������
    REALM_ZONE_RUSSIAN       = 12, // ���������������
    REALM_ZONE_TOURNAMENT_13 = 13, // ��������
    REALM_ZONE_TAIWAN        = 14, // ̨�壬��������
    REALM_ZONE_TOURNAMENT_15 = 15, // ��������
    REALM_ZONE_CHINA         = 16, // �й�����������
    REALM_ZONE_CN1           = 17, // �й�����1
    REALM_ZONE_CN2           = 18, // �й�����2
    REALM_ZONE_CN3           = 19, // �й�����3
    REALM_ZONE_CN4           = 20, // �й�����4
    REALM_ZONE_CN5           = 21, // �й�����5
    REALM_ZONE_CN6           = 22, // �й�����6
    REALM_ZONE_CN7           = 23, // �й�����7
    REALM_ZONE_CN8           = 24, // �й�����8
    REALM_ZONE_TOURNAMENT_25 = 25, // ��������
    REALM_ZONE_TEST_SERVER   = 26, // ���Է�����
    REALM_ZONE_TOURNAMENT_27 = 27, // ��������
    REALM_ZONE_QA_SERVER     = 28, // QA ���Է�����
    REALM_ZONE_CN9           = 29, // �й�����9
    REALM_ZONE_TEST_SERVER_2 = 30, // ���Է�����2
    REALM_ZONE_CN10          = 31, // �й�����10
    REALM_ZONE_CTC           = 32, // CTC
    REALM_ZONE_CNC           = 33, // CNC
    REALM_ZONE_CN1_4         = 34, // �й��������1/4
    REALM_ZONE_CN2_6_9       = 35, // �й��������2/6/9
    REALM_ZONE_CN3_7         = 36, // �й��������3/7
    REALM_ZONE_CN5_8         = 37  // �й��������5/8
};

// xinef: ��Ը���ݽṹ
struct PetitionData
{
};

/// �����࣬����ȫ����Ϸ״̬����
class World: public IWorld
{
public:
    /// ���캯��
    World();

    /// ��������
    ~World() override;

    /// ��ȡ���絥��ʵ��
    static World* instance();

    /// ȫ��ѭ��������
    static uint32 m_worldLoopCounter;

    /// �ж��Ƿ�ܾ��ͻ�������
    [[nodiscard]] bool IsClosed() const override;

    /// ���÷������Ƿ�ر�
    void SetClosed(bool val) override;

    /// ��ȡ����������Ͱ�ȫ�ȼ�
    [[nodiscard]] AccountTypes GetPlayerSecurityLimit() const override { return _allowedSecurityLevel; }

    /// ��������������Ͱ�ȫ�ȼ�
    void SetPlayerSecurityLimit(AccountTypes sec) override;

    /// �����ݿ��������İ�ȫ�ȼ�
    void LoadDBAllowedSecurityLevel() override;

    /// �ж��Ƿ���������ƶ�
    [[nodiscard]] bool getAllowMovement() const override { return _allowMovement; }

    /// �����Ƿ���������ƶ�
    void SetAllowMovement(bool allow) override { _allowMovement = allow; }

    /// ��ȡĬ�ϵ�DBC���ػ�����
    [[nodiscard]] LocaleConstant GetDefaultDbcLocale() const override { return _defaultDbcLocale; }

    /// ��ȡ���ݴ洢·������dbc, maps��
    [[nodiscard]] std::string const& GetDataPath() const override { return _dataPath; }

    /// ��ȡ�´�ÿ����������ʱ��
    [[nodiscard]] Seconds GetNextDailyQuestsResetTime() const override { return _nextDailyQuestReset; }

    /// ��ȡ�´�ÿ����������ʱ��
    [[nodiscard]] Seconds GetNextWeeklyQuestsResetTime() const override { return _nextWeeklyQuestReset; }

    /// ��ȡ�´����ս������ʱ��
    [[nodiscard]] Seconds GetNextRandomBGResetTime() const override { return _nextRandomBGReset; }

    /// ��ȡ��ҿɴﵽ������ܵȼ�
    [[nodiscard]] uint16 GetConfigMaxSkillValue() const override
    {
        uint16 lvl = uint16(getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
        return lvl > 60 ? 300 + ((lvl - 60) * 75) / 10 : lvl * 5;
    }

    /// ��ʼ����������
    void SetInitialWorldSettings() override;

    /// �����������ã�֧�����أ�
    void LoadConfigSettings(bool reload = false) override;

    /// �жϷ������Ƿ����ڹر�
    [[nodiscard]] bool IsShuttingDown() const override { return _shutdownTimer > 0; }

    /// ��ȡʣ��ر�ʱ��
    [[nodiscard]] uint32 GetShutDownTimeLeft() const override { return _shutdownTimer; }

    /// �����������ر�����
    void ShutdownServ(uint32 time, uint32 options, uint8 exitcode, std::string const& reason = std::string()) override;

    /// ȡ���������ر�
    void ShutdownCancel() override;

    /// ��ʾ�ر���Ϣ
    void ShutdownMsg(bool show = false, Player* player = nullptr, std::string const& reason = std::string()) override;

    /// ��ȡ�˳�����
    static uint8 GetExitCode() { return _exitCode; }

    /// ����ֹͣ������
    static void StopNow(uint8 exitcode) { _stopEvent = true; _exitCode = exitcode; }

    /// �жϷ������Ƿ���ֹͣ
    static bool IsStopped() { return _stopEvent; }

    /// ��������״̬
    void Update(uint32 diff) override;

    /// ���÷��������ø���ֵ
    void setRate(ServerConfigs index, float value) override;

    /// ��ȡ���������ø���ֵ
    float getRate(ServerConfigs index) const override;

    /// ���ò�������
    void setBoolConfig(ServerConfigs index, bool value) override;

    /// ��ȡ��������
    bool getBoolConfig(ServerConfigs index) const override;

    /// ���ø�������
    void setFloatConfig(ServerConfigs index, float value) override;

    /// ��ȡ��������
    float getFloatConfig(ServerConfigs index) const override;

    /// ������������
    void setIntConfig(ServerConfigs index, uint32 value) override;

    /// ��ȡ��������
    uint32 getIntConfig(ServerConfigs index) const override;

    /// �����ַ�������
    void setStringConfig(ServerConfigs index, std::string const& value) override;

    /// ��ȡ�ַ�������
    std::string_view getStringConfig(ServerConfigs index) const override;

    /// �ж��Ƿ�ΪPvP������
    [[nodiscard]] bool IsPvPRealm() const override;

    /// �ж��Ƿ�ΪFFA PvP������
    [[nodiscard]] bool IsFFAPvPRealm() const override;

    // ��ȡ��½���ɼ�����
    static float GetMaxVisibleDistanceOnContinents()    { return _maxVisibleDistanceOnContinents; }

    // ��ȡʵ�����ɼ�����
    static float GetMaxVisibleDistanceInInstances()     { return _maxVisibleDistanceInInstances;  }

    // ��ȡս���;��������ɼ�����
    static float GetMaxVisibleDistanceInBGArenas()      { return _maxVisibleDistanceInBGArenas;   }

    // ��ȡ��һ��Who�б�����ӳ�
    uint32 GetNextWhoListUpdateDelaySecs() override;

    /// ����������ָ��
    void ProcessCliCommands() override;

    /// ���������ָ�����
    void QueueCliCommand(CliCommandHolder* commandHolder) override { _cliCmdQueue.add(commandHolder); }

    /// ǿ����Ϸ�¼�����
    void ForceGameEventUpdate() override;

    /// �����˺Ž�ɫ����
    void UpdateRealmCharCount(uint32 accid) override;

    /// ��ȡ���õ�DBC���ػ�����
    [[nodiscard]] LocaleConstant GetAvailableDbcLocale(LocaleConstant locale) const override { if (_availableDbcLocaleMask & (1 << locale)) return locale; else return _defaultDbcLocale; }

    /// �������ݿ�汾
    void LoadDBVersion() override;

    /// ��ȡ���ݿ�汾
    [[nodiscard]] char const* GetDBVersion() const override { return _dbVersion.c_str(); }

    /// ����������صĹ⻷
    void UpdateAreaDependentAuras() override;

    /// ��ȡ�����־
    [[nodiscard]] uint32 GetCleaningFlags() const override { return _cleaningFlags; }

    /// ���������־
    void   SetCleaningFlags(uint32 flags) override { _cleaningFlags = flags; }

    /// �����¼���صļ���������
    void   ResetEventSeasonalQuests(uint16 event_id) override;

    /// ��ȡ����������
    [[nodiscard]] std::string const& GetRealmName() const override { return _realmName; }

    /// ���÷���������
    void SetRealmName(std::string name) override { _realmName = name; }

    /// �Ƴ����ڵ�ʬ��
    void RemoveOldCorpses() override;

protected:
    /// ������Ϸʱ��
    void _UpdateGameTime();

    /// �����˺Ž�ɫ�����ص�
    void _UpdateRealmCharCount(PreparedQueryResult resultCharCount,uint32 accountId);

    /// ��ʼ��ÿ����������ʱ��
    void InitDailyQuestResetTime();

    /// ��ʼ��ÿ����������ʱ��
    void InitWeeklyQuestResetTime();

    /// ��ʼ��ÿ����������ʱ��
    void InitMonthlyQuestResetTime();

    /// ��ʼ�����ս������ʱ��
    void InitRandomBGResetTime();

    /// ��ʼ���������¼�ɾ��ʱ��
    void InitCalendarOldEventsDeletionTime();

    /// ��ʼ����������ʱ��
    void InitGuildResetTime();

    /// ����ÿ������
    void ResetDailyQuests();

    /// ����ÿ������
    void ResetWeeklyQuests();

    /// ����ÿ������
    void ResetMonthlyQuests();

    /// �������ս��
    void ResetRandomBG();

    /// ɾ�������еľ��¼�
    void CalendarDeleteOldEvents();

    /// ���ù�������
    void ResetGuildCap();

private:
    WorldConfig _worldConfig;

    static std::atomic_long _stopEvent; // ֹͣ�¼���־
    static uint8 _exitCode; // �˳�����
    uint32 _shutdownTimer; // �رյ���ʱ
    uint32 _shutdownMask; // �رձ�־
    std::string _shutdownReason; // �ر�ԭ��

    uint32 _cleaningFlags; // �����־

    bool _isClosed; // �Ƿ�ر�

    IntervalTimer _timers[WUPDATE_COUNT]; // ������¼�ʱ��
    Seconds _mail_expire_check_timer; // �ʼ����ڼ��ʱ��

    AccountTypes _allowedSecurityLevel; // �������Ͱ�ȫ�ȼ�
    LocaleConstant _defaultDbcLocale;                     // Ĭ��DBC���ػ�����
    uint32 _availableDbcLocaleMask;                       // ����DBC���ػ�����
    void DetectDBCLang(); // ���DBC����
    bool _allowMovement; // �Ƿ������ƶ�
    std::string _dataPath; // ����·��

    // �����Ӿ��루���ٷ��ʣ�
    static float _maxVisibleDistanceOnContinents;
    static float _maxVisibleDistanceInInstances;
    static float _maxVisibleDistanceInBGArenas;

    std::string _realmName; // ����������

    // CLI������У��̰߳�ȫ��
    LockedQueue<CliCommandHolder*> _cliCmdQueue;

    // �´�����ս��������ʱ��
    Seconds _nextDailyQuestReset;
    Seconds _nextWeeklyQuestReset;
    Seconds _nextMonthlyQuestReset;
    Seconds _nextRandomBGReset;
    Seconds _nextCalendarOldEventsDeletionTime;
    Seconds _nextGuildReset;

    // ʹ�õ����ݿ�汾
    std::string _dbVersion;
    uint32 _dbClientCacheVersion;

    // �������ݿ��ѯ�ص�
    void ProcessQueryCallbacks();
    QueryCallbackProcessor _queryProcessor;

    /**
     * @brief ��World�Ự���ʱ���ã�������������¼���Ƕ��е���
     *
     * @param session ������ɵ�World�Ự
     */
    inline void FinalizePlayerWorldSession(WorldSession* session);
};

std::unique_ptr<IWorld>& getWorldInstance();
#define sWorld getWorldInstance()

#endif
/// @}