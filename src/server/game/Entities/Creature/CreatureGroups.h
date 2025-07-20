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

 #ifndef _FORMATIONS_H
 #define _FORMATIONS_H
 
 #include "Define.h"
 #include "ObjectGuid.h"
 #include "Unit.h"
 #include <map>
 #include <unordered_map>
 
 class Creature;
 class CreatureGroup;
 
 // 定义用于控制生物群体AI行为的标志位
 enum class GroupAIFlags : uint16
 {
     GROUP_AI_FLAG_MEMBER_ASSIST_LEADER          = 0x001, // 成员协助领袖
     GROUP_AI_FLAG_LEADER_ASSIST_MEMBER          = 0x002, // 领袖协助成员
     GROUP_AI_FLAG_EVADE_TOGETHER                = 0x004, // 一起逃跑
     GROUP_AI_FLAG_RESPAWN_ON_EVADE              = 0x008, // 逃跑后重生
     GROUP_AI_FLAG_DONT_RESPAWN_LEADER_ON_EVADE  = 0x010, // 逃跑后领袖不重生
     GROUP_AI_FLAG_ACQUIRE_NEW_TARGET_ON_EVADE   = 0x020, // 逃跑后重新获取目标
     //GROUP_AI_FLAG_UNK5                        = 0x040,
     //GROUP_AI_FLAG_UNK6                        = 0x080,
     //GROUP_AI_FLAG_UNK7                        = 0x100,
     GROUP_AI_FLAG_FOLLOW_LEADER                 = 0x200, // 跟随领袖
 
     GROUP_AI_FLAG_ASSIST_MASK                   = GROUP_AI_FLAG_MEMBER_ASSIST_LEADER | GROUP_AI_FLAG_LEADER_ASSIST_MEMBER, // 协助标志掩码
     GROUP_AI_FLAG_EVADE_MASK                    = GROUP_AI_FLAG_EVADE_TOGETHER | GROUP_AI_FLAG_RESPAWN_ON_EVADE, // 逃跑标志掩码
 
     // 用于验证有效标志位
     GROUP_AI_FLAG_SUPPORTED                     = GROUP_AI_FLAG_ASSIST_MASK | GROUP_AI_FLAG_EVADE_MASK | GROUP_AI_FLAG_DONT_RESPAWN_LEADER_ON_EVADE |
                                                   GROUP_AI_FLAG_FOLLOW_LEADER | GROUP_AI_FLAG_ACQUIRE_NEW_TARGET_ON_EVADE
 };
 
 // 群体信息结构体
 struct FormationInfo
 {
     FormationInfo() :
         leaderGUID(0),         // 领袖GUID
         follow_dist(0.0f),     // 跟随距离
         follow_angle(0.0f),    // 跟随角度
         groupAI(0),            // 群体AI标志
         point_1(0),            // 点1（用于路径点等）
         point_2(0)             // 点2
     {
     }
 
     ObjectGuid::LowType leaderGUID; // 领袖的低GUID
     float follow_dist;              // 跟随时的距离
     float follow_angle;             // 跟随时的角度
     uint16 groupAI;                 // 群体AI标志
     uint32 point_1;                 // 点1
     uint32 point_2;                 // 点2
 
     // 检查是否设置了指定的标志位
     bool HasGroupFlag(uint16 flag) const { return !!(groupAI & flag); }
 };
 
 // 生物群体信息类型定义（使用unordered_map存储）
 typedef std::unordered_map<ObjectGuid::LowType/*memberDBGUID*/, FormationInfo /*formationInfo*/>   CreatureGroupInfoType;
 
 // 群体管理器类
 class FormationMgr
 {
 public:
     FormationMgr() { }
     ~FormationMgr();
 
     // 获取单例实例
     static FormationMgr* instance();
 
     // 将生物添加到指定群体
     void AddCreatureToGroup(uint32 group_id, Creature* creature);
     // 从群体中移除生物
     void RemoveCreatureFromGroup(CreatureGroup* group, Creature* creature);
     // 加载生物群体信息
     void LoadCreatureFormations();
     // 存储生物群体信息的map
     CreatureGroupInfoType CreatureGroupMap;
 };
 
 // 生物群体类
 class CreatureGroup
 {
 public:
     // 定义群体成员类型
     typedef std::map<Creature*, FormationInfo>  CreatureGroupMemberType;
 
     // 群体不能创建为空
     explicit CreatureGroup(uint32 id) : m_leader(nullptr), m_groupID(id), m_Formed(false) {}
     ~CreatureGroup() {}
 
     // 获取领袖
     Creature* GetLeader() const { return m_leader; }
     // 获取群体ID
     uint32 GetId() const { return m_groupID; }
 
     // 检查群体是否为空
     bool IsEmpty() const { return m_members.empty(); }
     // 检查群体是否已形成
     bool IsFormed() const { return m_Formed; }
 
     // 获取成员列表
     const CreatureGroupMemberType& GetMembers() const { return m_members; }
 
     // 添加成员
     void AddMember(Creature* member);
     // 移除成员
     void RemoveMember(Creature* member);
     // 重置群体状态
     void FormationReset(bool dismiss, bool initMotionMaster);
 
     // 领袖移动到指定位置
     void LeaderMoveTo(float x, float y, float z, uint32 move_type);
     // 成员攻击目标
     void MemberEngagingTarget(Creature* member, Unit* target);
     // 为成员获取新目标
     Unit* GetNewTargetForMember(Creature* member);
     // 成员逃跑
     void MemberEvaded(Creature* member);
     // 群体重生
     void RespawnFormation(bool force = false);
     // 检查群体是否处于战斗状态
     [[nodiscard]] bool IsFormationInCombat();
     // 检查是否有成员存活
     [[nodiscard]] bool IsAnyMemberAlive(bool ignoreLeader = false);
 
 private:
     Creature* m_leader; // 领袖指针
     CreatureGroupMemberType m_members; // 成员列表
 
     uint32 m_groupID;    // 群体ID
     bool m_Formed;       // 是否已形成
 };
 
 // 获取群体管理器单例
 #define sFormationMgr FormationMgr::instance()
 
 #endif
