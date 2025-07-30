#ifndef AC_MOVESPLINEFLAG_H
#define AC_MOVESPLINEFLAG_H

#include "MovementTypedefs.h"
#include <string>

namespace Movement
{
#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif

    /**
     * MoveSplineFlag 类
     * 用于描述移动样条的标志位
     */
    class MoveSplineFlag
    {
    public:
        /**
         * 枚举 eFlags
         * 定义各种移动标志
         */
        enum eFlags
        {
            None                = 0x00000000,
            // x00-xFF(first byte) used as animation Ids storage in pair with Animation flag
            Done                = 0x00000100,           // 标记移动完成
            Falling             = 0x00000200,           // 单位正在下落（受重力影响，如跳跃、坠落）  影响高度计算，不能与 Parabolic 标志组合使用
            No_Spline           = 0x00000400,           // 不使用样条插值（直接设置位置，类似瞬移）
            Parabolic           = 0x00000800,           // 抛物线运动（如跳跃技能）  影响高度计算，不能与 Falling 标志组合使用
            Walkmode            = 0x00001000,           // 步行模式
            Flying              = 0x00002000,           // 平滑移动（Catmullrom 插值模式），飞行动画
            OrientationFixed    = 0x00004000,           // 击退效果（如战士的冲锋、BOSS的击飞技能） 模型方向固定
            Final_Point         = 0x00008000,           // 移动的终点是精确的（不进行路径优化）
            Final_Target        = 0x00010000,           // 移动朝向某个目标（如冲锋时面向敌人）
            Final_Angle         = 0x00020000,           // 移动结束时设置特定朝向
            Catmullrom          = 0x00040000,           // 使用 Catmullrom 插值模式
            Cyclic              = 0x00080000,           // 循环移动（如巡逻NPC来回走）
            Enter_Cycle         = 0x00100000,           // 进入循环移动模式
            Animation           = 0x00200000,           // 播放移动动画（如游泳、飞行）
            Frozen              = 0x00400000,           // 冻结移动（位置不变，但动画可能继续）
            TransportEnter      = 0x00800000,           // 单位在交通工具上（如船、飞艇）
            TransportExit       = 0x01000000,           // 离开载具时的特殊移动处理
            Unknown7            = 0x02000000,
            Unknown8            = 0x04000000,
            OrientationInversed = 0x08000000,           // 反转朝向（如后退移动）
            Unknown10           = 0x10000000,
            Unknown11           = 0x20000000,
            Unknown12           = 0x40000000,
            Unknown13           = 0x80000000,

            // 掩码
            Mask_Final_Facing   = Final_Point | Final_Target | Final_Angle,  // 最终朝向相关标志
            Mask_Animations     = 0xFF,                 // 存储动画ID，与 Animation 标志一起使用
            Mask_No_Monster_Move = Mask_Final_Facing | Mask_Animations | Done, // 不应包含在 SMSG_MONSTER_MOVE 数据包中的标志
            Mask_CatmullRom     = Flying | Catmullrom,  // 使用 CatmullRom 插值模式
            Mask_Unused         = No_Spline | Enter_Cycle | Frozen | Unknown7 | Unknown8 | Unknown10 | Unknown11 | Unknown12 | Unknown13 // 未使用或不支持的标志
        };

        // 原始数据访问
        inline uint32& raw() { return (uint32&) * this; }
        [[nodiscard]] inline const uint32& raw() const { return (const uint32&) * this; }

        // 构造函数
        MoveSplineFlag() { raw() = 0; }
        MoveSplineFlag(uint32 f) { raw() = f; }
        MoveSplineFlag(const MoveSplineFlag& f) { raw() = f.raw(); }
        MoveSplineFlag(MoveSplineFlag&&) = default;
        MoveSplineFlag& operator=(const MoveSplineFlag&) = default;
        MoveSplineFlag& operator=(MoveSplineFlag&&) = default;

        // 常量接口

        [[nodiscard]] bool isSmooth() const { return raw() & Mask_CatmullRom; }      // 是否使用平滑插值
        [[nodiscard]] bool isLinear() const { return !isSmooth(); }                  // 是否使用线性插值
        [[nodiscard]] bool isFacing() const { return raw() & Mask_Final_Facing; }    // 是否设置最终朝向

        [[nodiscard]] uint8 getAnimationId() const { return animId; }                // 获取动画ID
        [[nodiscard]] bool hasAllFlags(uint32 f) const { return (raw() & f) == f; }  // 是否包含所有指定标志
        [[nodiscard]] bool hasFlag(uint32 f) const { return (raw() & f) != 0; }      // 是否包含指定标志
        uint32 operator & (uint32 f) const { return (raw() & f); }                   // 按位与操作
        uint32 operator | (uint32 f) const { return (raw() | f); }                   // 按位或操作
        [[nodiscard]] std::string ToString() const;                                  // 转换为字符串表示

        // 非常量接口

        void operator &= (uint32 f) { raw() &= f; }      // 按位与赋值
        void operator |= (uint32 f) { raw() |= f; }      // 按位或赋值

        void EnableAnimation(uint8 anim) { raw() = (raw() & ~(Mask_Animations | Falling | Parabolic)) | Animation | anim; } // 启用动画
        void EnableParabolic() { raw() = (raw() & ~(Mask_Animations | Falling | Animation)) | Parabolic; } // 启用抛物线运动
        void EnableFalling() { raw() = (raw() & ~(Mask_Animations | Parabolic | Animation)) | Falling; } // 启用下落运动
        void EnableFlying() { raw() = (raw() & ~(Falling | Catmullrom)) | Flying; } // 启用飞行模式
        void EnableCatmullRom() { raw() = (raw() & ~Flying) | Catmullrom; } // 启用 CatmullRom 插值
        void EnableFacingPoint() { raw() = (raw() & ~Mask_Final_Facing) | Final_Point; } // 启用面向点
        void EnableFacingAngle() { raw() = (raw() & ~Mask_Final_Facing) | Final_Angle; } // 启用面向角度
        void EnableFacingTarget() { raw() = (raw() & ~Mask_Final_Facing) | Final_Target; } // 启用面向目标
        void EnableTransportEnter() { raw() = (raw() & ~TransportExit) | TransportEnter; } // 启用进入载具
        void EnableTransportExit() { raw() = (raw() & ~TransportEnter) | TransportExit; } // 启用离开载具

        // 位域成员
        uint8 animId              : 8;                    // 动画ID
        bool done                : 1;                    // 是否完成
        bool falling             : 1;                    // 是否下落
        bool no_spline           : 1;                    // 是否不使用样条
        bool parabolic           : 1;                    // 是否抛物线运动
        bool walkmode            : 1;                    // 是否步行模式
        bool flying              : 1;                    // 是否飞行
        bool orientationFixed    : 1;                    // 方向是否固定
        bool final_point         : 1;                    // 最终点方向
        bool final_target        : 1;                    // 最终目标方向
        bool final_angle         : 1;                    // 最终角度方向
        bool catmullrom          : 1;                    // 是否使用 Catmullrom 插值
        bool cyclic              : 1;                    // 是否循环路径
        bool enter_cycle         : 1;                    // 是否进入循环
        bool animation           : 1;                    // 是否播放动画
        bool frozen              : 1;                    // 是否冻结
        bool transportEnter      : 1;                    // 是否进入载具
        bool transportExit       : 1;                    // 是否离开载具
        bool unknown7            : 1;                    // 未知标志7
        bool unknown8            : 1;                    // 未知标志8
        bool orientationInversed : 1;                    // 方向是否反转
        bool unknown10           : 1;                    // 未知标志10
        bool unknown11           : 1;                    // 未知标志11
        bool unknown12           : 1;                    // 未知标志12
        bool unknown13           : 1;                    // 未知标志13
    };
#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif
}

#endif // AC_MOVESPLINEFLAG_H
