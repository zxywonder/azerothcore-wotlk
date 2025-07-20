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

 #ifndef ACORE_ESCORTMOVEMENTGENERATOR_H
 #define ACORE_ESCORTMOVEMENTGENERATOR_H
 
 #include "MovementGenerator.h"
 
 // EscortMovementGenerator 类用于管理生物或单位的护送移动行为
 // 它基于预计算的路径点数组，控制单位按照指定路径移动
 template<class T>
 class EscortMovementGenerator : public MovementGeneratorMedium< T, EscortMovementGenerator<T> >
 {
 public:
     // 构造函数，接受一个可选的路径点数组用于初始化
     // 参数: _path - 预先计算好的路径点数组指针
     EscortMovementGenerator(Movement::PointsArray* _path = nullptr) : i_recalculateSpeed(false)
     {
         if (_path)
             m_precomputedPath = *_path;
     }
 
     // 初始化函数，在单位开始执行该移动生成器时调用
     void DoInitialize(T*);
 
     // 结束时调用的清理函数
     void DoFinalize(T*);
 
     // 重置移动生成器，用于重新开始移动过程
     void DoReset(T*);
 
     // 每次更新移动状态时调用，用于处理逻辑更新
     // 参数: T* - 当前单位指针
     // 参数: uint32 - 时间差（毫秒）
     // 返回: bool - 是否继续执行该移动生成器
     bool DoUpdate(T*, uint32);
 
     // 当单位速度发生变化时调用，标记需要重新计算速度
     void unitSpeedChanged() { i_recalculateSpeed = true; }
 
     // 获取当前移动生成器的类型，用于识别
     MovementGeneratorType GetMovementGeneratorType() { return ESCORT_MOTION_TYPE; }
 
     // 获取与此移动生成器关联的 spline ID
     // 返回: uint32 - spline ID
     uint32 GetSplineId() const { return _splineId; }
 
 private:
     // 标记是否需要重新计算速度
     bool i_recalculateSpeed;
 
     // 存储预计算的路径点数组
     Movement::PointsArray m_precomputedPath;
 
     // 当前移动使用的 spline ID
     uint32 _splineId;
 };
 
 #endif
