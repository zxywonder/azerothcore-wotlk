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

 #ifndef AC_MOVEPLINE_H
 #define AC_MOVEPLINE_H
 
 #include "MoveSplineInitArgs.h"
 #include "Spline.h"
 
 namespace Movement
 {
     // Location结构体继承自Vector3，用于表示带有朝向的位置
     struct Location : public Vector3
     {
         Location()  = default;
         // 构造函数，带位置和朝向
         Location(float x, float y, float z, float o) : Vector3(x, y, z), orientation(o) {}
         // 从Vector3构造
         Location(const Vector3& v) : Vector3(v) {}
         // 从Vector3和朝向构造
         Location(const Vector3& v, float o) : Vector3(v), orientation(o) {}
 
         float orientation{0}; // 朝向值
     };
 
     // MoveSpline类用于表示平滑的Catmull-Rom或线性曲线，以及沿该曲线移动的点
     // 曲线可以是循环的，此时移动也将循环
     // 点可以具有垂直加速度分量（用于坠落、抛物线运动）
     class MoveSpline
     {
     public:
         typedef Spline<int32> MySpline;
 
         // 更新结果枚举，表示移动状态更新的结果
         enum UpdateResult
         {
             Result_None         = 0x01,      // 没有特殊结果
             Result_Arrived      = 0x02,      // 已到达终点
             Result_NextCycle    = 0x04,      // 进入下一个循环
             Result_NextSegment  = 0x08,      // 进入下一段路径
             Result_JustArrived  = 0x10,      // 刚刚到达
         };
         friend class PacketBuilder; // PacketBuilder是友元类
 
     protected:
         MySpline        spline;                  // 实际的样条曲线
 
         FacingInfo      facing;                  // 面向信息
 
         uint32          m_Id;                    // 移动路径的唯一ID
 
         MoveSplineFlag  splineflags;             // 样条标志位
 
         int32           time_passed;             // 已经过的时间
 
         // 目前未使用，但可能在将来使用
         // float           duration_mod;
         // float           duration_mod_next;
 
         float           vertical_acceleration;   // 垂直加速度
         float           initialOrientation;      // 初始朝向
         int32           effect_start_time;       // 效果开始时间
         int32           point_Idx;               // 当前路径点索引
         int32           point_Idx_offset;        // 路径点索引偏移量
 
         void init_spline(const MoveSplineInitArgs& args); // 初始化样条曲线
 
     protected:
         // 获取路径点数组
         [[nodiscard]] const MySpline::ControlArray& getPath() const { return spline.getPoints(); }
 
         // 计算抛物线高度
         void computeParabolicElevation(float& el) const;
 
         // 计算坠落高度
         void computeFallElevation(float& el) const;
 
         // 更新状态，返回更新结果，参数为时间差
         UpdateResult _updateState(int32& ms_time_diff);
 
         // 获取下一个时间戳
         [[nodiscard]] int32 next_timestamp() const { return spline.length(point_Idx + 1); }
 
         // 获取当前段已用时间
         [[nodiscard]] int32 segment_time_elapsed() const { return next_timestamp() - time_passed; }
 
     public:
         // 获取已用时间（公开，用于WaypointMoveGenerator）
         [[nodiscard]] int32 timeElapsed() const { return Duration() - time_passed; }
 
         // 获取已过时间（公开，用于WaypointMoveGenerator）
         [[nodiscard]] int32 timePassed() const { return time_passed; }
 
         // 获取总持续时间
         [[nodiscard]] int32 Duration() const { return spline.length(); }
 
         // 获取内部样条曲线
         [[nodiscard]] MySpline const& _Spline() const { return spline; }
 
         // 获取当前样条索引
         [[nodiscard]] int32 _currentSplineIdx() const { return point_Idx; }
 
         // 完成样条计算
         void _Finalize();
 
         // 中断移动
         void _Interrupt() { splineflags.done = true; }
 
     public:
         // 初始化移动样条
         void Initialize(const MoveSplineInitArgs&);
 
         // 判断是否已初始化
         [[nodiscard]] bool Initialized() const { return !spline.empty(); }
 
         // 构造函数
         MoveSpline();
 
         // 模板函数：更新状态，使用UpdateHandler处理结果
         template<class UpdateHandler>
         void updateState(int32 difftime, UpdateHandler& handler)
         {
             ASSERT(Initialized());
             do
                 handler(_updateState(difftime));
             while (difftime > 0);
         }
 
         // 更新状态，直接处理时间差
         void updateState(int32 difftime)
         {
             ASSERT(Initialized());
             do _updateState(difftime);
             while (difftime > 0);
         }
 
         // 计算当前位置
         [[nodiscard]] Location ComputePosition() const;
 
         // 获取移动ID
         [[nodiscard]] uint32 GetId() const { return m_Id; }
 
         // 判断是否已完成
         [[nodiscard]] bool Finalized() const { return splineflags.done; }
 
         // 判断是否为循环路径
         [[nodiscard]] bool isCyclic() const { return splineflags.cyclic; }
 
         // 判断是否正在坠落
         [[nodiscard]] bool isFalling() const { return splineflags.falling; }
 
         // 判断是否为步行模式
         [[nodiscard]] bool isWalking() const { return splineflags.walkmode; }
 
         // 获取最终目的地
         [[nodiscard]] Vector3 FinalDestination() const { return Initialized() ? spline.getPoint(spline.last()) : Vector3(); }
 
         // 获取当前目的地
         [[nodiscard]] Vector3 CurrentDestination() const { return Initialized() ? spline.getPoint(point_Idx + 1) : Vector3(); }
 
         // 获取当前路径索引
         [[nodiscard]] int32 currentPathIdx() const;
 
         // 是否有动画
         [[nodiscard]] bool HasAnimation() const { return splineflags.animation; }
 
         // 获取动画类型
         [[nodiscard]] uint8 GetAnimationType() const { return splineflags.animId; }
 
         bool onTransport; // 是否在运输工具上
 
         // 转换为字符串（用于调试等）
         [[nodiscard]] std::string ToString() const;
 
         // 判断是否已开始移动
         [[nodiscard]] bool HasStarted() const
         {
             return time_passed > 0;
         }
     };
 }
 #endif // AC_MOVEPLINE_H
