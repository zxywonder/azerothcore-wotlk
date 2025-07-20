#ifndef ACORE_IDLEMOVEMENTGENERATOR_H
#define ACORE_IDLEMOVEMENTGENERATOR_H

#include "MovementGenerator.h"

/// 闲置移动生成器，用于单位处于空闲状态时的移动行为
class IdleMovementGenerator : public MovementGenerator
{
public:
    /// 初始化函数，用于设置单位进入闲置状态
    void Initialize(Unit*) override;
    /// 结束时的清理操作，当前为空实现
    void Finalize(Unit*) override {  }
    /// 重置移动生成器，重新初始化
    void Reset(Unit*) override;
    /// 更新移动状态，始终返回true表示无需进一步处理
    bool Update(Unit*, uint32) override { return true; }
    /// 获取移动生成器类型，返回IDLE_MOTION_TYPE
    MovementGeneratorType GetMovementGeneratorType() override { return IDLE_MOTION_TYPE; }
};

/// 旋转移动生成器，用于单位原地旋转
class RotateMovementGenerator : public MovementGenerator
{
public:
    /// 构造函数，指定旋转时间与方向
    explicit RotateMovementGenerator(uint32 time, RotateDirection direction) : m_duration(time), m_maxDuration(time), m_direction(direction) {}

    /// 初始化单位开始旋转
    void Initialize(Unit*) override;
    /// 结束旋转并重置单位状态
    void Finalize(Unit*) override;
    /// 重置旋转状态，重新初始化
    void Reset(Unit* owner) override { Initialize(owner); }
    /// 更新旋转进度，处理时间递减与状态更新
    bool Update(Unit*, uint32) override;
    /// 获取移动生成器类型，返回ROTATE_MOTION_TYPE
    MovementGeneratorType GetMovementGeneratorType() override { return ROTATE_MOTION_TYPE; }

private:
    uint32 m_duration, m_maxDuration; ///< 当前剩余时间与最大持续时间
    RotateDirection m_direction;      ///< 旋转方向
};

/// 分心移动生成器，用于单位被分心时的状态
class DistractMovementGenerator : public MovementGenerator
{
public:
    /// 构造函数，设置分心持续时间
    explicit DistractMovementGenerator(uint32 timer) : m_timer(timer) {}

    /// 初始化单位进入分心状态
    void Initialize(Unit*) override;
    /// 结束分心状态，恢复单位正常行为
    void Finalize(Unit*) override;
    /// 重置分心状态，重新初始化
    void Reset(Unit* owner) override { Initialize(owner); }
    /// 更新分心计时，计时结束则完成
    bool Update(Unit*, uint32) override;
    /// 获取移动生成器类型，返回DISTRACT_MOTION_TYPE
    MovementGeneratorType GetMovementGeneratorType() override { return DISTRACT_MOTION_TYPE; }

private:
    uint32 m_timer; ///< 分心剩余时间
};

/// 助攻分心移动生成器，用于NPC协助战斗前的分心状态
class AssistanceDistractMovementGenerator : public DistractMovementGenerator
{
public:
    /// 构造函数，设置分心时间
    AssistanceDistractMovementGenerator(uint32 timer) :
        DistractMovementGenerator(timer) {}

    /// 获取移动生成器类型，返回ASSISTANCE_DISTRACT_MOTION_TYPE
    MovementGeneratorType GetMovementGeneratorType() override { return ASSISTANCE_DISTRACT_MOTION_TYPE; }
    /// 结束分心状态，并通知单位进入协助战斗状态
    void Finalize(Unit*) override;
};

#endif // ACORE_IDLEMOVEMENTGENERATOR_H
