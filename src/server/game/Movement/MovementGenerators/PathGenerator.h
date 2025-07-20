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

 #ifndef _PATH_GENERATOR_H
 #define _PATH_GENERATOR_H
 
 #include "DetourNavMesh.h"
 #include "DetourNavMeshQuery.h"
 #include "MMapMgr.h"
 #include "MapDefines.h"
 #include "MoveSplineInitArgs.h"
 #include "SharedDefines.h"
 #include <G3D/Vector3.h>
 
 class Unit;
 class WorldObject;
 
 // 74*4.0f=296y number_of_points*interval = max_path_len
 // this is way more than actual evade range
 // I think we can safely cut those down even more
 #define MAX_PATH_LENGTH         74
 #define MAX_POINT_PATH_LENGTH   74
 
 #define SMOOTH_PATH_STEP_SIZE   4.0f // 光滑路径的步长
 #define SMOOTH_PATH_SLOP        0.3f // 光滑路径的容差
 #define DISALLOW_TIME_AFTER_FAIL    3 // secs 失败后禁止时间
 #define VERTEX_SIZE       3 // 顶点坐标大小
 #define INVALID_POLYREF   0 // 无效的多边形引用
 
 // 路径类型枚举
 enum PathType
 {
     PATHFIND_BLANK             = 0x00,   // 路径尚未构建
     PATHFIND_NORMAL            = 0x01,   // 正常路径
     PATHFIND_SHORTCUT          = 0x02,   // 穿越障碍物、地形、空中等（旧行为）
     PATHFIND_INCOMPLETE        = 0x04,   // 有部分路径可用 - 正在接近目标
     PATHFIND_NOPATH            = 0x08,   // 无有效路径或生成失败
     PATHFIND_NOT_USING_PATH    = 0x10,   // 当飞行/游泳或地图无mmaps数据时使用
     PATHFIND_SHORT             = 0x20,   // 路径长度大于等于其限制长度
     PATHFIND_FARFROMPOLY_START = 0x40,   // 起始位置远离多边形区域
     PATHFIND_FARFROMPOLY_END   = 0x80,   // 结束位置远离多边形区域
     PATHFIND_FARFROMPOLY       = PATHFIND_FARFROMPOLY_START | PATHFIND_FARFROMPOLY_END, // 起始或结束位置远离多边形区域
 };
 
 class PathGenerator
 {
     public:
         // 构造函数，初始化路径生成器，关联一个WorldObject对象
         explicit PathGenerator(WorldObject const* owner);
         // 析构函数
         ~PathGenerator();
 
         // 计算从当前对象到目标点的路径
         // 返回值：true表示新路径已计算，false表示无需改变
         bool CalculatePath(float destX, float destY, float destZ, bool forceDest = false);
         // 计算从指定起点到目标点的路径
         bool CalculatePath(float x, float y, float z, float destX, float destY, float destZ, bool forceDest);
         // 判断目标点的Z坐标是否无效
         [[nodiscard]] bool IsInvalidDestinationZ(Unit const* target) const;
         // 检查两个点之间的路径是否为可行走的攀爬路径（使用float数组）
         [[nodiscard]] bool IsWalkableClimb(float const* v1, float const* v2) const;
         // 检查两点之间是否为可行走的攀爬路径
         [[nodiscard]] bool IsWalkableClimb(float x, float y, float z, float destX, float destY, float destZ) const;
         // 静态方法：检查两点之间是否为可行走的攀爬路径（带源高度）
         [[nodiscard]] static bool IsWalkableClimb(float x, float y, float z, float destX, float destY, float destZ, float sourceHeight);
         // 判断给定路径是否为水中路径
         [[nodiscard]] bool IsWaterPath(Movement::PointsArray pathPoints) const;
         // 检查两点之间的路径是否为可游泳路径（使用float数组）
         [[nodiscard]] bool IsSwimmableSegment(float const* v1, float const* v2, bool checkSwim = true) const;
         // 检查两点之间的路径是否为可游泳路径
         [[nodiscard]] bool IsSwimmableSegment(float x, float y, float z, float destX, float destY, float destZ, bool checkSwim = true) const;
         // 获取两点之间攀爬所需的最小高度
         [[nodiscard]] static float GetRequiredHeightToClimb(float x, float y, float z, float destX, float destY, float destZ, float sourceHeight);
 
         // 设置选项 - 可选
 
         // 设置是否检查坡度（当启用时，跳过坡度过大的路径，与StraightPath不兼容）
         void SetSlopeCheck(bool checkSlope) { _slopeCheck = checkSlope; }
         // 设置是否使用直线路径
         void SetUseStraightPath(bool useStraightPath) { _useStraightPath = useStraightPath; }
         // 设置路径长度限制
         void SetPathLengthLimit(float distance) { _pointPathLimit = std::min<uint32>(uint32(distance/SMOOTH_PATH_STEP_SIZE), MAX_POINT_PATH_LENGTH); }
         // 设置是否使用射线检测
         void SetUseRaycast(bool useRaycast) { _useRaycast = useRaycast; }
 
         // 获取结果
 
         // 获取起始位置
         [[nodiscard]] G3D::Vector3 const& GetStartPosition() const { return _startPosition; }
         // 获取目标位置
         [[nodiscard]] G3D::Vector3 const& GetEndPosition() const { return _endPosition; }
         // 获取实际的目标位置
         [[nodiscard]] G3D::Vector3 const& GetActualEndPosition() const { return _actualEndPosition; }
 
         // 获取路径点数组
         [[nodiscard]] Movement::PointsArray const& GetPath() const { return _pathPoints; }
 
         // 获取路径类型
         [[nodiscard]] PathType GetPathType() const { return _type; }
 
         // 缩短路径直到距离目标点指定距离
         void ShortenPathUntilDist(G3D::Vector3 const& point, float dist);
 
         // 获取路径总长度
         [[nodiscard]] float getPathLength() const
         {
             float len = 0.0f;
             float dx, dy, dz;
             uint32 size = _pathPoints.size();
             if (size)
             {
                 dx = _pathPoints[0].x - _startPosition.x;
                 dy = _pathPoints[0].y - _startPosition.y;
                 dz = _pathPoints[0].z - _startPosition.z;
                 len += std::sqrt( dx * dx + dy * dy + dz * dz );
             }
             else
             {
                 return len;
             }
 
             for (uint32 i = 1; i < size; ++i)
             {
                 dx = _pathPoints[i].x - _pathPoints[i - 1].x;
                 dy = _pathPoints[i].y - _pathPoints[i - 1].y;
                 dz = _pathPoints[i].z - _pathPoints[i - 1].z;
                 len += std::sqrt( dx * dx + dy * dy + dz * dz );
             }
             return len;
         }
 
         // 清除路径数据
         void Clear()
         {
             _polyLength = 0;
             _pathPoints.clear();
         }
 
     private:
         dtPolyRef _pathPolyRefs[MAX_PATH_LENGTH];   // detour多边形引用数组
         uint32 _polyLength;                         // 路径中多边形的数量
 
         Movement::PointsArray _pathPoints;  // 实际路径点数组
         PathType _type;                     // 路径类型
 
         bool _useStraightPath;  // 是否使用直线路径（不用于移动路径）
         bool _forceDestination; // 是否强制到达目标点
         bool _slopeCheck;       // 是否检查坡度（与_straightPath不兼容）
         uint32 _pointPathLimit; // 路径点限制；取较小值（this, MAX_POINT_PATH_LENGTH）
         bool _useRaycast;       // 是否使用射线检测（用于直线路径）
 
         G3D::Vector3 _startPosition;        // 起始位置 {x, y, z}
         G3D::Vector3 _endPosition;          // 目标位置 {x, y, z}
         G3D::Vector3 _actualEndPosition;    // 实际可到达的最接近目标的位置
 
         WorldObject const* const _source;       // 正在移动的对象
         dtNavMesh const* _navMesh;              // 导航网格
         dtNavMeshQuery const* _navMeshQuery;    // 用于查找路径的查询对象
 
         dtQueryFilterExt _filter;  // 所有移动共用的过滤器，按需更新
 
         // 设置起始位置
         void SetStartPosition(G3D::Vector3 const& point) { _startPosition = point; }
         // 设置目标位置
         void SetEndPosition(G3D::Vector3 const& point) { _actualEndPosition = point; _endPosition = point; }
         // 设置实际的目标位置
         void SetActualEndPosition(G3D::Vector3 const& point) { _actualEndPosition = point; }
         // 规范化路径
         void NormalizePath();
 
         // 检查两点是否在指定范围内
         [[nodiscard]] bool InRange(G3D::Vector3 const& p1, G3D::Vector3 const& p2, float r, float h) const;
         // 计算两点之间的3D平方距离
         [[nodiscard]] float Dist3DSqr(G3D::Vector3 const& p1, G3D::Vector3 const& p2) const;
         // 检查两点是否在YZX范围内
         bool InRangeYZX(float const* v1, float const* v2, float r, float h) const;
 
         // 根据位置获取路径多边形
         dtPolyRef GetPathPolyByPosition(dtPolyRef const* polyPath, uint32 polyPathSize, float const* Point, float* Distance = nullptr) const;
         // 获取指定位置的多边形
         dtPolyRef GetPolyByLocation(float const* Point, float* Distance) const;
         // 检查指定位置是否有tile数据
         [[nodiscard]] bool HaveTile(G3D::Vector3 const& p) const;
 
         // 构建多边形路径
         void BuildPolyPath(G3D::Vector3 const& startPos, G3D::Vector3 const& endPos);
         // 构建点路径
         void BuildPointPath(float const* startPoint, float const* endPoint);
         // 构建快捷路径
         void BuildShortcut();
 
         // 获取导航地形类型
         [[nodiscard]] NavTerrain GetNavTerrain(float x, float y, float z) const;
         // 创建查询过滤器
         void CreateFilter();
         // 更新查询过滤器
         void UpdateFilter();
 
         // 光滑路径辅助函数
         uint32 FixupCorridor(dtPolyRef* path, uint32 npath, uint32 maxPath, dtPolyRef const* visited, uint32 nvisited);
         // 获取转向目标
         bool GetSteerTarget(float const* startPos, float const* endPos, float minTargetDist, dtPolyRef const* path, uint32 pathSize, float* steerPos,
                             unsigned char& steerPosFlag, dtPolyRef& steerPosRef);
         // 查找光滑路径
         dtStatus FindSmoothPath(float const* startPos, float const* endPos,
                               dtPolyRef const* polyPath, uint32 polyPathSize,
                               float* smoothPath, int* smoothPathSize, uint32 smoothPathMaxSize);
 
         // 添加远离多边形标志
         void AddFarFromPolyFlags(bool startFarFromPoly, bool endFarFromPoly);
 };
 
 #endif
