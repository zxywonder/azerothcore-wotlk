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

 #ifndef AZEROTHCORE_CORPSE_H
 #define AZEROTHCORE_CORPSE_H
 
 #include "DatabaseEnv.h"
 #include "GridDefines.h"
 #include "LootMgr.h"
 #include "Object.h"
 
 // 尸体类型枚举
 enum CorpseType
 {
     CORPSE_BONES             = 0,       // 骨头（不可复活）
     CORPSE_RESURRECTABLE_PVE = 1,       // 可复活的PVE尸体
     CORPSE_RESURRECTABLE_PVP = 2        // 可复活的PVP尸体
 };
 #define MAX_CORPSE_TYPE        3        // 最大尸体类型数
 
 // 客户端复活对话框显示半径
 #define CORPSE_RECLAIM_RADIUS 39
 
 // 尸体标志位
 enum CorpseFlags
 {
     CORPSE_FLAG_NONE        = 0x00,     // 无标志
     CORPSE_FLAG_BONES       = 0x01,     // 骨头标志
     CORPSE_FLAG_UNK1        = 0x02,     // 未知标志1
     CORPSE_FLAG_UNK2        = 0x04,     // 未知标志2
     CORPSE_FLAG_HIDE_HELM   = 0x08,     // 隐藏头盔
     CORPSE_FLAG_HIDE_CLOAK  = 0x10,     // 隐藏披风
     CORPSE_FLAG_LOOTABLE    = 0x20      // 可拾取
 };
 
 // 尸体类，继承自WorldObject和GridObject
 class Corpse : public WorldObject, public GridObject<Corpse>
 {
 public:
     // 构造函数，默认为骨头类型
     explicit Corpse(CorpseType type = CORPSE_BONES);
     // 析构函数
     ~Corpse() override;
 
     // 将尸体添加到世界
     void AddToWorld() override;
     // 从世界中移除尸体
     void RemoveFromWorld() override;
 
     // 构建值更新数据包
     void BuildValuesUpdate(uint8 updateType, ByteBuffer* data, Player* target) override;
 
     // 创建尸体，使用guidlow
     bool Create(ObjectGuid::LowType guidlow);
     // 创建尸体，关联到玩家owner
     bool Create(ObjectGuid::LowType guidlow, Player* owner);
 
     // 保存尸体到数据库
     void SaveToDB();
     // 从数据库加载尸体数据
     bool LoadCorpseFromDB(ObjectGuid::LowType guid, Field* fields);
 
     // 从数据库删除尸体
     void DeleteFromDB(CharacterDatabaseTransaction trans);
     // 静态方法：根据拥有者GUID删除尸体
     static void DeleteFromDB(ObjectGuid const ownerGuid, CharacterDatabaseTransaction trans);
 
     // 获取尸体拥有者GUID
     [[nodiscard]] ObjectGuid GetOwnerGUID() const { return GetGuidValue(CORPSE_FIELD_OWNER); }
 
     // 获取死亡时间
     [[nodiscard]] time_t const& GetGhostTime() const { return m_time; }
     // 重置死亡时间
     void ResetGhostTime();
     // 获取尸体类型
     [[nodiscard]] CorpseType GetType() const { return m_type; }
 
     // 获取所在单元坐标
     [[nodiscard]] CellCoord const& GetCellCoord() const { return _cellCoord; }
     // 设置所在单元坐标
     void SetCellCoord(CellCoord const& cellCoord) { _cellCoord = cellCoord; }
 
     // 拾取物品数据
     Loot loot;                                          // remove insignia ONLY at BG
     // 拾取者玩家指针
     Player* lootRecipient;
 
     // 检查尸体是否已过期
     [[nodiscard]] bool IsExpired(time_t t) const;
 
 private:
     CorpseType m_type;                  // 尸体类型
     time_t m_time;                      // 死亡时间
     CellCoord _cellCoord;               // 单元坐标
 };
 #endif
