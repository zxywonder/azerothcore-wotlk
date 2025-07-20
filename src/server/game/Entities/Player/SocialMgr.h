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

#ifndef __ACORE_SOCIALMGR_H
#define __ACORE_SOCIALMGR_H

#include "DatabaseEnv.h"
#include "ObjectGuid.h"
#include <map>

// 前向声明
class Player;
class WorldPacket;

// 好友状态枚举
enum FriendStatus
{
    FRIEND_STATUS_OFFLINE   = 0x00,  // 好友离线
    FRIEND_STATUS_ONLINE    = 0x01,  // 好友在线
    FRIEND_STATUS_AFK       = 0x02,  // 好友离开键盘
    FRIEND_STATUS_DND       = 0x04,  // 好友请勿打扰
    FRIEND_STATUS_RAF       = 0x08   // 好友招募链接
};

// 社交标记枚举
enum SocialFlag
{
    SOCIAL_FLAG_FRIEND      = 0x01,  // 好友标记
    SOCIAL_FLAG_IGNORED     = 0x02,  // 忽略标记
    SOCIAL_FLAG_MUTED       = 0x04,  // 静音标记（猜测）
    SOCIAL_FLAG_UNK         = 0x08,  // 未知标记 - 似乎不是招募链接标记

    // 所有社交标记组合
    SOCIAL_FLAG_ALL         = SOCIAL_FLAG_FRIEND | SOCIAL_FLAG_IGNORED | SOCIAL_FLAG_MUTED
};

// 好友信息结构体
struct FriendInfo
{
    FriendStatus Status;  // 好友状态
    uint8 Flags;          // 好友标记
    uint32 Area;          // 所在区域
    uint8 Level;          // 等级
    uint8 Class;          // 职业
    std::string Note;     // 好友备注

    // 默认构造函数
    FriendInfo() : Status(FRIEND_STATUS_OFFLINE), Flags(0), Area(0), Level(0), Class(0), Note()
    { }

    // 带参数的构造函数
    FriendInfo(uint8 flags, std::string const& note) : Status(FRIEND_STATUS_OFFLINE), Flags(flags), Area(0), Level(0), Class(0), Note(note)
    { }
};

/// Results of friend related commands
// 好友相关命令结果枚举
enum FriendsResult : uint8
{
    FRIEND_DB_ERROR         = 0x00,  // 数据库错误
    FRIEND_LIST_FULL        = 0x01,  // 好友列表已满
    FRIEND_ONLINE           = 0x02,  // 好友在线
    FRIEND_OFFLINE          = 0x03,  // 好友离线
    FRIEND_NOT_FOUND        = 0x04,  // 未找到好友
    FRIEND_REMOVED          = 0x05,  // 好友已移除
    FRIEND_ADDED_ONLINE     = 0x06,  // 已添加在线好友
    FRIEND_ADDED_OFFLINE    = 0x07,  // 已添加离线好友
    FRIEND_ALREADY          = 0x08,  // 好友已存在
    FRIEND_SELF             = 0x09,  // 不能添加自己为好友
    FRIEND_ENEMY            = 0x0A,  // 不能添加敌对阵营玩家为好友
    FRIEND_IGNORE_FULL      = 0x0B,  // 忽略列表已满
    FRIEND_IGNORE_SELF      = 0x0C,  // 不能忽略自己
    FRIEND_IGNORE_NOT_FOUND = 0x0D,  // 未找到要忽略的玩家
    FRIEND_IGNORE_ALREADY   = 0x0E,  // 玩家已在忽略列表中
    FRIEND_IGNORE_ADDED     = 0x0F,  // 已添加到忽略列表
    FRIEND_IGNORE_REMOVED   = 0x10,  // 已从忽略列表移除
    FRIEND_IGNORE_AMBIGUOUS = 0x11,  // 名称不明确，请输入更完整的玩家服务器名
    FRIEND_MUTE_FULL        = 0x12,  // 静音列表已满
    FRIEND_MUTE_SELF        = 0x13,  // 不能静音自己
    FRIEND_MUTE_NOT_FOUND   = 0x14,  // 未找到要静音的玩家
    FRIEND_MUTE_ALREADY     = 0x15,  // 玩家已在静音列表中
    FRIEND_MUTE_ADDED       = 0x16,  // 已添加到静音列表
    FRIEND_MUTE_REMOVED     = 0x17,  // 已从静音列表移除
    FRIEND_MUTE_AMBIGUOUS   = 0x18,  // 名称不明确，请输入更完整的玩家服务器名
    FRIEND_UNK1             = 0x19,  // 客户端无消息
    FRIEND_UNK2             = 0x1A,
    FRIEND_UNK3             = 0x1B,
    FRIEND_UNKNOWN          = 0x1C   // 来自服务器的未知好友响应
};

// 社交管理器好友数量限制
#define SOCIALMGR_FRIEND_LIMIT  50u
// 社交管理器忽略列表数量限制
#define SOCIALMGR_IGNORE_LIMIT  50u

// 玩家社交类
class PlayerSocial
{
    friend class SocialMgr;

    public:
        // 构造函数
        PlayerSocial();
        // 添加/移除相关
        // 将玩家添加到社交列表
        bool AddToSocialList(ObjectGuid friend_guid, SocialFlag flag);
        // 从社交列表移除玩家
        void RemoveFromSocialList(ObjectGuid friend_guid, SocialFlag flag);
        // 设置好友备注
        void SetFriendNote(ObjectGuid friendGuid, std::string note);
        // 数据包发送相关
        // 发送社交列表
        void SendSocialList(Player* player, uint32 flags);
        // 其他功能
        // 检查是否有该好友
        bool HasFriend(ObjectGuid friend_guid) const;
        // 检查是否忽略了该玩家
        bool HasIgnore(ObjectGuid ignore_guid) const;
        // 获取玩家GUID
        ObjectGuid GetPlayerGUID() const { return m_playerGUID; }
        // 设置玩家GUID
        void SetPlayerGUID(ObjectGuid guid) { m_playerGUID = guid; }
        // 获取带有指定标记的社交关系数量
        uint32 GetNumberOfSocialsWithFlag(SocialFlag flag) const;
    private:
        // 检查联系人是否存在
        bool _checkContact(ObjectGuid guid, SocialFlag flags) const;
        // 玩家社交关系映射类型定义
        typedef std::map<ObjectGuid, FriendInfo> PlayerSocialMap;
        // 玩家社交关系映射
        PlayerSocialMap m_playerSocialMap;
        // 玩家GUID
        ObjectGuid m_playerGUID;
};

// 社交管理器类
class SocialMgr
{
    private:
        // 构造函数
        SocialMgr();
        // 析构函数
        ~SocialMgr();

    public:
        // 获取单例实例
        static SocialMgr* instance();
        // 其他功能
        // 移除玩家的社交数据
        void RemovePlayerSocial(ObjectGuid guid) { m_socialMap.erase(guid); }
        // 获取好友信息
        static void GetFriendInfo(Player* player, ObjectGuid friendGUID, FriendInfo& friendInfo);
        // 数据包管理
        // 创建好友状态数据包
        void MakeFriendStatusPacket(FriendsResult result, ObjectGuid friend_guid, WorldPacket* data);
        // 发送好友状态
        void SendFriendStatus(Player* player, FriendsResult result, ObjectGuid friend_guid, bool broadcast);
        // 向好友列表中的玩家广播数据包
        void BroadcastToFriendListers(Player* player, WorldPacket* packet);
        // 加载数据
        // 从数据库加载玩家社交数据
        PlayerSocial* LoadFromDB(PreparedQueryResult result, ObjectGuid guid);
    private:
        // 社交关系映射类型定义
        typedef std::map<ObjectGuid, PlayerSocial> SocialMap;
        // 社交关系映射
        SocialMap m_socialMap;
};

// 社交管理器单例实例宏定义
#define sSocialMgr SocialMgr::instance()

#endif
