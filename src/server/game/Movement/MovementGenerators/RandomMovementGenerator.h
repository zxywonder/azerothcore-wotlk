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

 #ifndef ACORE_RANDOMMOTIONGENERATOR_H
 #define ACORE_RANDOMMOTIONGENERATOR_H
 
 #include "MovementGenerator.h"
 #include "PathGenerator.h"
 #include "Timer.h"
 
 // 定义随机移动生成器使用的点数
 #define RANDOM_POINTS_NUMBER        12
 // 定义每个点的连接数
 #define RANDOM_LINKS_COUNT          7
 // 地面单位的最小游荡距离
 #define MIN_WANDER_DISTANCE_GROUND  1.0f
 // 空中单位的最小游荡距离
 #define MIN_WANDER_DISTANCE_AIR     10.0f
 // 路径长度的最大系数
 #define MAX_PATH_LENGHT_FACTOR      1.85f
 
 template<class T>
 class RandomMovementGenerator : public MovementGeneratorMedium< T, RandomMovementGenerator<T> >
 {
 public:
     // 构造函数，初始化随机移动生成器，wanderDistance为游荡距离
     RandomMovementGenerator(float wanderDistance = 0.0f) : _nextMoveTime(0), _moveCount(0), _wanderDistance(wanderDistance), _pathGenerator(nullptr), _currentPoint(RANDOM_POINTS_NUMBER)
     {
         // 初始位置重置为原点
         _initialPosition.Relocate(0.0f, 0.0f, 0.0f, 0.0f);
         // 预留足够的空间以避免频繁重新分配内存
         _destinationPoints.reserve(RANDOM_POINTS_NUMBER);
 
         // 初始化每个点的连接关系，形成一个循环结构
         for (uint8 i = 0; i < RANDOM_POINTS_NUMBER; ++i)
         {
             _validPointsVector[i].reserve(RANDOM_LINKS_COUNT);
             for (uint8 j = 0; j < RANDOM_LINKS_COUNT; ++j)
                 _validPointsVector[i].push_back((i + j + RANDOM_POINTS_NUMBER / 2 - RANDOM_LINKS_COUNT / 2) % RANDOM_POINTS_NUMBER);
         }
 
         // 最后一个点连接所有其他点
         _validPointsVector[RANDOM_POINTS_NUMBER].reserve(RANDOM_POINTS_NUMBER);
         for (uint8 i = 0; i < RANDOM_POINTS_NUMBER; ++i)
             _validPointsVector[RANDOM_POINTS_NUMBER].push_back(i);
     }
 
     // 析构函数声明
     ~RandomMovementGenerator();
 
     // 设置随机移动的目标位置
     void _setRandomLocation(T*);
     // 初始化移动生成器
     void DoInitialize(T*);
     // 完成移动生成器的最终操作
     void DoFinalize(T*);
     // 重置移动生成器
     void DoReset(T*);
     // 更新移动状态，返回是否继续执行
     bool DoUpdate(T*, const uint32);
     // 获取重置位置（用于重置时）
     bool GetResetPosition(float& x, float& y, float& z);
     // 获取当前运动生成器的类型
     MovementGeneratorType GetMovementGeneratorType() { return RANDOM_MOTION_TYPE; }
 
 private:
     TimeTrackerSmall _nextMoveTime;  // 下一次移动的时间追踪器
     uint8 _moveCount;                // 移动次数计数器
     float _wanderDistance;           // 游荡距离
     std::unique_ptr<PathGenerator> _pathGenerator;  // 路径生成器指针
     std::vector<G3D::Vector3> _destinationPoints;   // 目标点列表
     std::vector<uint8> _validPointsVector[RANDOM_POINTS_NUMBER + 1];  // 有效点连接表
     uint8 _currentPoint;             // 当前所在点索引
     std::map<uint16, Movement::PointsArray> _preComputedPaths;  // 预计算的路径缓存
     Position _initialPosition, _currDestPosition;  // 初始位置和当前目标位置
 };
 #endif
