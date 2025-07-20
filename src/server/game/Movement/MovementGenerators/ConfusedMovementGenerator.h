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

 #ifndef ACORE_CONFUSEDGENERATOR_H
 #define ACORE_CONFUSEDGENERATOR_H
 
 #include "MovementGenerator.h"
 #include "Timer.h"
 
 #define MAX_CONF_WAYPOINTS 24 //! 允许在i_nextMove始终为最小定时器的情况下产生12秒的混乱移动
 
 template<class T>
 class ConfusedMovementGenerator : public MovementGeneratorMedium< T, ConfusedMovementGenerator<T> >
 {
 public:
     explicit ConfusedMovementGenerator() : i_nextMoveTime(1) {} // 构造函数，初始化下一次移动时间
 
     void DoInitialize(T*);    // 初始化混乱移动生成器
     void DoFinalize(T*);      // 完成或清理混乱移动状态
     void DoReset(T*);         // 重置移动生成器
     bool DoUpdate(T*, uint32); // 更新移动状态，执行混乱移动逻辑
 
     MovementGeneratorType GetMovementGeneratorType() { return CONFUSED_MOTION_TYPE; } // 返回运动生成器类型为混乱类型
 private:
     void _InitSpecific(T*, bool&, bool&); // 特定初始化逻辑，可能用于不同派生类的定制实现
     TimeTracker i_nextMoveTime;          // 跟踪下一次移动的时间
     float i_waypoints[MAX_CONF_WAYPOINTS + 1][3]; // 存储混乱移动路径点坐标（x, y, z）
     uint32 i_nextMove;                    // 下一步移动的索引
 };
 #endif
