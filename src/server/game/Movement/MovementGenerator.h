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

 #ifndef ACORE_MOVEMENTGENERATOR_H
 #define ACORE_MOVEMENTGENERATOR_H
 
 #include "Define.h"
 #include "FactoryHolder.h"
 #include "MotionMaster.h"
 
 class Unit;
 
 // MovementGenerator 是所有移动生成器的基类，用于管理单位的移动行为
 class MovementGenerator
 {
 public:
     virtual ~MovementGenerator();
 
     // 初始化单位的移动行为
     virtual void Initialize(Unit*) = 0;
 
     // 当移动行为结束时调用，用于清理资源
     virtual void Finalize(Unit*) = 0;
 
     // 重置移动行为
     virtual void Reset(Unit*) = 0;
 
     // 更新单位的移动状态，time_diff 为经过的时间（毫秒）
     virtual bool Update(Unit*, uint32 time_diff) = 0;
 
     // 获取当前移动生成器的类型
     virtual MovementGeneratorType GetMovementGeneratorType() = 0;
 
     // 获取 spline ID，用于任务系统（如护送任务）
     virtual uint32 GetSplineId() const { return 0; }  // Xinef: Escort system
 
     // 当单位速度发生变化时调用
     virtual void unitSpeedChanged() { }
 
     // 暂停移动，timer 为暂停时间（毫秒）
     virtual void Pause(uint32 /* timer = 0*/) {}
     // 恢复移动，overrideTimer 用于覆盖当前暂停时间
     virtual void Resume(uint32 /* overrideTimer = 0*/) {}
 
     // 用于逃离逻辑，获取重置位置坐标
     virtual bool GetResetPosition(float& /*x*/, float& /*y*/, float& /*z*/) { return false; }
 };
 
 // MovementGeneratorMedium 是一个模板类，用于适配不同类型的单位和移动生成器
 template<class T, class D>
 class MovementGeneratorMedium : public MovementGenerator
 {
 public:
     void Initialize(Unit* u) override
     {
         // 将 Unit 转换为具体类型 T，并调用 DoInitialize 方法
         //u->AssertIsType<T>();
         (static_cast<D*>(this))->DoInitialize(static_cast<T*>(u));
     }
 
     void Finalize(Unit* u) override
     {
         // 将 Unit 转换为具体类型 T，并调用 DoFinalize 方法
         //u->AssertIsType<T>();
         (static_cast<D*>(this))->DoFinalize(static_cast<T*>(u));
     }
 
     void Reset(Unit* u) override
     {
         // 将 Unit 转换为具体类型 T，并调用 DoReset 方法
         //u->AssertIsType<T>();
         (static_cast<D*>(this))->DoReset(static_cast<T*>(u));
     }
 
     bool Update(Unit* u, uint32 time_diff) override
     {
         // 将 Unit 转换为具体类型 T，并调用 DoUpdate 方法
         //u->AssertIsType<T>();
         return (static_cast<D*>(this))->DoUpdate(static_cast<T*>(u), time_diff);
     }
 };
 
 // MovementGeneratorCreator 是 MovementGenerator 的工厂持有者
 typedef FactoryHolder<MovementGenerator, Unit, MovementGeneratorType> MovementGeneratorCreator;
 
 // MovementGeneratorFactory 是一个模板工厂类，用于创建指定类型的 MovementGenerator
 template<class Movement>
 struct MovementGeneratorFactory : public MovementGeneratorCreator
 {
     // 构造函数，指定该工厂创建的移动生成器类型
     MovementGeneratorFactory(MovementGeneratorType movementGeneratorType) : MovementGeneratorCreator(movementGeneratorType) { }
 
     // 创建一个移动生成器实例
     MovementGenerator* Create(Unit* /*object*/) const
     {
         return new Movement();
     }
 };
 
 // IdleMovementFactory 是空闲移动生成器的工厂类
 struct IdleMovementFactory : public MovementGeneratorCreator
 {
     // 构造函数，指定该工厂创建 IDLE 类型的移动生成器
     IdleMovementFactory() : MovementGeneratorCreator(IDLE_MOTION_TYPE) { }
 
     // 创建一个空闲移动生成器实例
     MovementGenerator* Create(Unit* object) const override;
 };
 
 // MovementGeneratorRegistry 是所有移动生成器工厂的注册表
 typedef MovementGeneratorCreator::FactoryHolderRegistry MovementGeneratorRegistry;
 
 // 定义一个全局的移动生成器注册表实例
 #define sMovementGeneratorRegistry MovementGeneratorRegistry::instance()
 
 #endif
