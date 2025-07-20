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

 #include "MoveSpline.h"
 #include "Creature.h"
 #include "Log.h"
 #include <sstream>
 
 namespace Movement
 {
 
     // 计算当前移动点的位置
     Location MoveSpline::ComputePosition() const
     {
         ASSERT(Initialized());
 
         float u = 1.f;
         // 计算当前段的持续时间
         int32 seg_time = spline.length(point_Idx, point_Idx + 1);
         if (seg_time > 0)
             // 计算当前段的进度百分比
             u = (time_passed - spline.length(point_Idx)) / (float)seg_time;
         Location c;
         c.orientation = initialOrientation;
         // 获取当前位置坐标
         spline.evaluate_percent(point_Idx, u, c);
 
         // 根据标志位选择不同的高度计算方式
         if (splineflags.animation)
             ;// MoveSplineFlag::Animation 会禁用坠落或抛物线运动
         else if (splineflags.parabolic)
             computeParabolicElevation(c.z); // 计算抛物线高度
         else if (splineflags.falling)
             computeFallElevation(c.z); // 计算自由落体高度
 
         // 如果移动已完成且设置了朝向标志
         if (splineflags.done && splineflags.isFacing())
         {
             if (splineflags.final_angle)
                 c.orientation = facing.angle; // 使用指定角度
             else if (splineflags.final_point)
                 // 计算面向目标点的角度
                 c.orientation = std::atan2(facing.f.y - c.y, facing.f.x - c.x);
             // MoveSplineFlag::Final_Target 标志无需处理
         }
         else
         {
             // 如果没有固定朝向且不是坠落状态
             if (!splineflags.hasFlag(MoveSplineFlag::OrientationFixed | MoveSplineFlag::Falling))
             {
                 Vector3 hermite;
                 // 计算导数以获取方向
                 spline.evaluate_derivative(point_Idx, u, hermite);
                 c.orientation = std::atan2(hermite.y, hermite.x); // 计算方向角
             }
 
             if (splineflags.orientationInversed)
                 c.orientation = -c.orientation; // 反转方向
         }
         return c;
     }
 
     // 计算抛物线高度
     void MoveSpline::computeParabolicElevation(float& el) const
     {
         if (time_passed > effect_start_time)
         {
             // 转换为秒的时间
             float t_passedf = MSToSec(time_passed - effect_start_time);
             float t_durationf = MSToSec(Duration() - effect_start_time);
 
             // 使用抛物线公式计算高度
             el += (t_durationf - t_passedf) * 0.5f * vertical_acceleration * t_passedf;
         }
     }
 
     // 计算自由落体高度
     void MoveSpline::computeFallElevation(float& el) const
     {
         // 计算当前高度
         float z_now = spline.getPoint(spline.first()).z - Movement::computeFallElevation(MSToSec(time_passed), false);
         float final_z = FinalDestination().z; // 最终目标高度
         el = std::max(z_now, final_z); // 取较大值
     }
 
     // 根据距离和速度计算持续时间
     inline uint32 computeDuration(float length, float velocity)
     {
         return SecToMS(length / velocity);
     }
 
     // 坠落初始化器结构体
     struct FallInitializer
     {
         // 构造函数，设置起始高度
         FallInitializer(float _start_elevation) : start_elevation(_start_elevation) {}
         float start_elevation;
 
         // 操作符重载，计算坠落时间
         inline int32 operator()(Spline<int32>& s, int32 i)
         {
             return Movement::computeFallTime(start_elevation - s.getPoint(i + 1).z, false) * 1000.f;
         }
     };
 
     // 最小持续时间常量
     enum
     {
         minimal_duration = 1
     };
 
     // 普通初始化器结构体
     struct CommonInitializer
     {
         // 构造函数，设置速度和初始时间
         CommonInitializer(float _velocity) : velocityInv(1000.f / _velocity), _time(minimal_duration) {}
 
         // 操作符重载，计算段长度对应的时间
         inline int32 operator()(Spline<int32>& s, int32 i)
         {
             _time += (s.SegLength(i) * velocityInv);
             return _time;
         }
 
         float velocityInv; // 速度的倒数
         int32 _time;       // 当前时间
     };
 
     // 初始化样条曲线
     void MoveSpline::init_spline(const MoveSplineInitArgs& args)
     {
         // 两种评估模式：线性和Catmull-Rom
         const SplineBase::EvaluationMode modes[2] = {SplineBase::ModeLinear, SplineBase::ModeCatmullrom};
         if (args.flags.cyclic)
         {
             uint32 cyclic_point = 0;
             // MoveSplineFlag::Enter_Cycle 支持已移除
             //if (splineflags & SPLINEFLAG_ENTER_CYCLE)
             //cyclic_point = 1;   // 不应修改，来自客户端
             // 初始化循环样条
             spline.init_cyclic_spline(&args.path[0], args.path.size(), modes[args.flags.isSmooth()], cyclic_point);
         }
         else
         {
             // 初始化普通样条
             spline.init_spline(&args.path[0], args.path.size(), modes[args.flags.isSmooth()]);
         }
 
         // 初始化时间戳
         if (splineflags.falling)
         {
             FallInitializer init(spline.getPoint(spline.first()).z);
             spline.initLengths(init); // 使用坠落初始化器
         }
         else
         {
             CommonInitializer init(args.velocity);
             spline.initLengths(init); // 使用普通初始化器
         }
 
         // 如果总持续时间过短
         if (spline.length() < minimal_duration)
         {
             // 设置最小持续时间
             spline.set_length(spline.last(), spline.isCyclic() ? 1000 : 1);
         }
         point_Idx = spline.first(); // 设置初始索引
     }
 
     // 初始化移动样条
     void MoveSpline::Initialize(MoveSplineInitArgs const& args)
     {
         splineflags = args.flags; // 复制标志位
         facing = args.facing;     // 复制面向信息
         m_Id = args.splineId;     // 复制ID
         point_Idx_offset = args.path_Idx_offset; // 路径偏移量
         initialOrientation = args.initialOrientation; // 初始朝向
 
         time_passed = 0;          // 已过时间清零
         vertical_acceleration = 0.f; // 垂直加速度清零
         effect_start_time = 0;    // 效果开始时间清零
 
         // 检查是否是停止样条
         if (args.flags.done)
         {
             spline.clear();       // 清空样条
             return;
         }
 
         init_spline(args);        // 初始化样条
 
         // 初始化抛物线/动画参数
         if (args.flags & (MoveSplineFlag::Parabolic | MoveSplineFlag::Animation))
         {
             effect_start_time = Duration() * args.time_perc; // 计算效果开始时间
             if (args.flags.parabolic && effect_start_time < Duration())
             {
                 // 计算垂直加速度
                 float f_duration = MSToSec(Duration() - effect_start_time);
                 vertical_acceleration = args.parabolic_amplitude * 8.f / (f_duration * f_duration);
             }
         }
     }
 
     // 构造函数
     MoveSpline::MoveSpline() : m_Id(0), time_passed(0),
         vertical_acceleration(0.f), initialOrientation(0.f), effect_start_time(0), point_Idx(0), point_Idx_offset(0),
         onTransport(false)
     {
         splineflags.done = true; // 默认标记为已完成
     }
 
     /// ============================================================================================
 
     // 验证初始化参数
     bool MoveSplineInitArgs::Validate(Unit* unit) const
     {
 #define CHECK(exp) \
         if (!(exp)) \
         { \
             if (unit) \
                 LOG_ERROR("misc.movesplineinitargs", "MoveSplineInitArgs::Validate: expression '{}' failed for {}", #exp, unit->GetGUID().ToString()); \
             else \
                 LOG_ERROR("misc.movesplineinitargs", "MoveSplineInitArgs::Validate: expression '{}' failed for cyclic spline continuation", #exp); \
             return false;\
         }
         CHECK(path.size() > 1); // 路径点数量必须大于1
         CHECK(velocity > 0.01f); // 速度必须大于0.01
         CHECK(time_perc >= 0.f && time_perc <= 1.f); // 时间百分比必须在0-1之间
         //CHECK(_checkPathBounds());
         return true;
 #undef CHECK
     }
 
     // 检查路径点是否符合数据包格式限制
     bool MoveSplineInitArgs::_checkPathBounds() const
     {
         // 对于非CatmullRom运动，路径点偏移量有大小限制
         if (!(flags & MoveSplineFlag::Mask_CatmullRom) && path.size() > 2)
         {
             constexpr auto MAX_OFFSET = (1 << 11) / 2; // 最大偏移量
             Vector3 middle = (path.front() + path.back()) / 2; // 计算中点
             Vector3 offset;
             for (uint32 i = 1; i < path.size() - 1; ++i)
             {
                 offset = path[i] - middle; // 计算相对于中点的偏移
                 // 检查各轴向是否超出最大偏移量
                 if (std::fabs(offset.x) >= MAX_OFFSET || std::fabs(offset.y) >= MAX_OFFSET || std::fabs(offset.z) >= MAX_OFFSET)
                 {
                     LOG_ERROR("movement", "MoveSplineInitArgs::_checkPathBounds check failed");
                     return false;
                 }
             }
         }
         return true;
     }
 
     /// ============================================================================================
 
     // 更新状态
     MoveSpline::UpdateResult MoveSpline::_updateState(int32& ms_time_diff)
     {
         if (Finalized()) // 如果已完成
         {
             ms_time_diff = 0; // 时间差清零
             return Result_Arrived; // 返回到达结果
         }
 
         UpdateResult result = Result_None;
 
         // 计算当前段能更新的最大时间
         int32 minimal_diff = std::min(ms_time_diff, segment_time_elapsed());
         if (minimal_diff < 0)
             minimal_diff = 0;
 
         ASSERT(minimal_diff >= 0);
         time_passed += minimal_diff; // 更新已过时间
         ms_time_diff -= minimal_diff; // 减去已处理时间
 
         if (time_passed >= next_timestamp()) // 如果已到达下一个时间戳
         {
             ++point_Idx; // 更新点索引
             if (point_Idx < spline.last()) // 如果不是最后一个点
             {
                 result = Result_NextSegment; // 返回进入下一段结果
             }
             else // 如果到达最后一个点
             {
                 if (spline.isCyclic()) // 如果是循环路径
                 {
                     point_Idx = spline.first(); // 回到第一个点
                     time_passed = time_passed % Duration(); // 更新时间
                     result = Movement::MoveSpline::UpdateResult(Result_NextCycle | Result_JustArrived); // 返回循环和刚到达结果
                 }
                 else // 如果不是循环路径
                 {
                     _Finalize(); // 完成移动
                     ms_time_diff = 0; // 时间差清零
                     result = Movement::MoveSpline::UpdateResult(Result_Arrived | Result_JustArrived); // 返回到达和刚到达结果
                 }
             }
         }
 
         return result;
     }
 
     // 转换为字符串（用于调试输出）
     std::string MoveSpline::ToString() const
     {
         std::stringstream str;
         str << "MoveSpline" << std::endl;
         str << "spline Id: " << GetId() << std::endl;
         str << "flags: " << splineflags.ToString() << std::endl;
         if (splineflags.final_angle)
             str << "facing  angle: " << facing.angle;
         else if (splineflags.final_target)
             str << "facing target: " << facing.target;
         else if (splineflags.final_point)
             str << "facing  point: " << facing.f.x << " " << facing.f.y << " " << facing.f.z;
         str << std::endl;
         str << "time passed: " << time_passed << std::endl;
         str << "total  time: " << Duration() << std::endl;
         str << "spline point Id: " << point_Idx << std::endl;
         str << "path  point  Id: " << currentPathIdx() << std::endl;
         str << spline.ToString();
         return str.str();
     }
 
     // 完成移动
     void MoveSpline::_Finalize()
     {
         splineflags.done = true; // 标记为已完成
         point_Idx = spline.last() - 1; // 设置为倒数第二个点
         time_passed = Duration(); // 时间设为总持续时间
     }
 
     // 获取当前路径索引
     int32 MoveSpline::currentPathIdx() const
     {
         int32 point = point_Idx_offset + point_Idx - spline.first() + (int)Finalized(); // 计算路径索引
         if (isCyclic()) // 如果是循环路径
             point = point % (spline.last() - spline.first()); // 取模运算
         return point;
     }
 }
