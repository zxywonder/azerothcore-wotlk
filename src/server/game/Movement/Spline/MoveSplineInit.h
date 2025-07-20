/*
 * This文件是AzerothCore项目的一部分。请参阅AUTHORS文件以获取版权声明
 *
 * 此程序是免费软件；你可以根据自由软件基金会发布的GNU Affero通用公共许可证的条款重新分发和/或修改它，
 * 许可证版本为3，或者（根据你的选择）任何更高版本。
 *
 * 此程序的发布是希望它会有用，但不提供任何保证；甚至不暗示对特定用途的适配性或适销性的保证。
 * 更多细节请参见GNU Affero通用公共许可证。
 *
 * 你应该已经收到一份GNU通用公共许可证的副本。如果没有，请访问<http://www.gnu.org/licenses/>.
 */

 #ifndef AC_MOVESPLINEINIT_H
 #define AC_MOVESPLINEINIT_H
 
 #include "MoveSplineInitArgs.h"
 #include "PathGenerator.h"
 #include "Unit.h"
 
 class Unit;
 
 namespace Movement
 {
     // xinef: 将声明移动到这里以便在MoveSplineInit.cpp之外访问
     // 根据移动标志选择速度类型
     UnitMoveType SelectSpeedType(uint32 moveFlags);
 
     // 动画类型枚举
     enum AnimType
     {
         ToGround    = 0, // 460 = ToGround，AnimationData.dbc中的索引
         FlyToFly    = 1, // 461 = FlyToFly?
         ToFly       = 2, // 458 = ToFly
         FlyToGround = 3  // 463 = FlyToGround
     };
 
     // 将坐标从全局坐标转换为运输工具的相对坐标
     class TransportPathTransform
     {
     public:
         TransportPathTransform(Unit* owner, bool transformForTransport)
             : _owner(owner), _transformForTransport(transformForTransport) { }
         Vector3 operator()(Vector3 input);
 
     private:
         Unit* _owner;
         bool _transformForTransport;
     };
 
     // xinef: 添加z坐标的悬停偏移
     class HoverMovementTransform
     {
     public:
         HoverMovementTransform(float z_offset) : _offset(z_offset) { }
         Vector3 operator()(Vector3 input)
         {
             input.z += _offset;
             return input;
         }
 
     private:
         float _offset;
     };
 
     /*  初始化并启动样条移动
      */
     class MoveSplineInit
     {
     public:
         explicit MoveSplineInit(Unit* m);
 
         /*  初始化的最终阶段，启动样条移动。
          */
         int32 Launch();
 
         /*  初始化的最终阶段，停止移动。
          */
         void Stop();
 
         /* 添加抛物线轨迹的移动
          * @param amplitude  - 抛物线的最大高度，值可以是负的也可以是正的
          * @param start_time - 移动开始时间和开始沿抛物线轨迹移动之间的时间延迟
          * 不能与最终动画结合使用
          */
         void SetParabolic(float amplitude, float start_time);
         /* 移动完成后播放动画
          * 不能与抛物线移动结合使用
          */
         void SetAnimation(AnimType anim);
 
         /* 添加最终的面向动画
          * 在路径完成之后设置单位的朝向到指定的点/角度
          * 你只能有一个最终朝向：之前的将被覆盖
          */
         void SetFacing(float angle);
         void SetFacing(Vector3 const& point);
         void SetFacing(Unit const* target);
 
         /* 初始化路径移动
          * @param path - 点的数组，不应为空
          * @param pointId - 路径第一个点的ID。例如：当第三个路径点完成时，会通知pointId + 3已完成
          */
         void MovebyPath(const PointsArray& path, int32 pointId = 0);
 
         /* 初始化从A到B的简单移动，A是当前单位的位置，B是目标位置
          */
         void MoveTo(const Vector3& destination, bool generatePath = false, bool forceDestination = false);
         void MoveTo(float x, float y, float z, bool generatePath = false, bool forceDestination = false);
 
         /* 设置路径第一个点的ID。当第N个路径点完成时，ILisener会收到pointId + N已完成的通知
          * 用于将路径拆分为多个部分的航路点移动
          */
         void SetFirstPointId(int32 pointId) { args.path_Idx_offset = pointId; }
 
         /* 启用CatmullRom样条插值模式（使路径平滑）
          * 如果未启用，将选择线性样条模式。默认禁用
          */
         void SetSmooth();
         /* 启用CatmullRom样条插值模式，并启用飞行动画。默认禁用
          */
         void SetFly();
         /* 启用行走模式。默认禁用
          */
         void SetWalk(bool enable);
         /* 使移动循环。默认禁用
          */
         void SetCyclic();
         /* 启用下落模式。默认禁用
          */
         void SetFall();
         /* 进入运输工具。默认禁用
          */
         void SetTransportEnter();
         /* 离开运输工具。默认禁用
          */
         void SetTransportExit();
         /* 反转单位模型的方向。默认禁用
          */
         void SetOrientationInversed();
         /* 固定单位的模型旋转。默认禁用
          */
         void SetOrientationFixed(bool enable);
 
         /* 设置速度（如果你想要自定义移动速度）
          * 如果未设置，速度将根据单位的速度和当前移动模式选择
          * 如果启用了下落模式则无效
          * 速度不应为负数
          */
         void SetVelocity(float velocity);
 
         PointsArray& Path() { return args.path; }
 
         /* 禁用运输工具坐标转换，用于已有原始偏移量的情况
         */
         void DisableTransportPathTransformations();
     protected:
         MoveSplineInitArgs args;
         Unit*  unit;
     };
 
     // 设置飞行模式
     inline void MoveSplineInit::SetFly() { args.flags.EnableFlying(); }
     // 设置行走模式
     inline void MoveSplineInit::SetWalk(bool enable) { args.flags.walkmode = enable; }
     // 设置平滑样条模式
     inline void MoveSplineInit::SetSmooth() { args.flags.EnableCatmullRom(); }
     // 设置循环模式
     inline void MoveSplineInit::SetCyclic() { args.flags.cyclic = true; }
     // 设置下落模式
     inline void MoveSplineInit::SetFall() { args.flags.EnableFalling(); }
     // 设置自定义速度
     inline void MoveSplineInit::SetVelocity(float vel) { args.velocity = vel; args.HasVelocity = true; }
     // 设置方向反转
     inline void MoveSplineInit::SetOrientationInversed() { args.flags.orientationInversed = true;}
     // 设置进入运输工具标志
     inline void MoveSplineInit::SetTransportEnter() { args.flags.EnableTransportEnter(); }
     // 设置离开运输工具标志
     inline void MoveSplineInit::SetTransportExit() { args.flags.EnableTransportExit(); }
     // 设置固定方向
     inline void MoveSplineInit::SetOrientationFixed(bool enable) { args.flags.orientationFixed = enable; }
 
     // 使用路径点初始化移动
     inline void MoveSplineInit::MovebyPath(const PointsArray& controls, int32 path_offset)
     {
         args.path_Idx_offset = path_offset;
         args.path.resize(controls.size());
         std::transform(controls.begin(), controls.end(), args.path.begin(), TransportPathTransform(unit, args.TransformForTransport));
     }
 
     // 使用坐标初始化移动
     inline void MoveSplineInit::MoveTo(float x, float y, float z, bool generatePath, bool forceDestination)
     {
         MoveTo(G3D::Vector3(x, y, z), generatePath, forceDestination);
     }
 
     // 设置抛物线运动
     inline void MoveSplineInit::SetParabolic(float amplitude, float time_shift)
     {
         args.time_perc = time_shift;
         args.parabolic_amplitude = amplitude;
         args.flags.EnableParabolic();
     }
 
     // 设置动画播放
     inline void MoveSplineInit::SetAnimation(AnimType anim)
     {
         args.time_perc = 0.f;
         args.flags.EnableAnimation((uint8)anim);
     }
 
     // 设置面向指定点
     inline void MoveSplineInit::SetFacing(Vector3 const& spot)
     {
         TransportPathTransform transform(unit, args.TransformForTransport);
         Vector3 finalSpot = transform(spot);
         args.facing.f.x = finalSpot.x;
         args.facing.f.y = finalSpot.y;
         args.facing.f.z = finalSpot.z;
         args.flags.EnableFacingPoint();
     }
 
     // 禁用运输工具路径坐标转换
     inline void MoveSplineInit::DisableTransportPathTransformations() { args.TransformForTransport = false; }
 }
 #endif // AC_MOVESPLINEINIT_H
