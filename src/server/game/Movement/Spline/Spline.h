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

 #ifndef AC_SPLINE_H
 #define AC_SPLINE_H
 
 #include "Errors.h"
 #include "MovementTypedefs.h"
 #include <G3D/Vector3.h>
 #include <limits>
 
 namespace Movement
 {
     class SplineBase
     {
     public:
         typedef int index_type;
         typedef std::vector<Vector3> ControlArray;
 
         // 插值模式枚举，用于定义样条曲线的计算方式
         enum EvaluationMode
         {
             ModeLinear,                 // 线性插值
             ModeCatmullrom,             // Catmull-Rom 插值
             ModeBezier3_Unused,         // 未使用的 Bezier 三次插值
             UninitializedMode,          // 未初始化模式
             ModesEnd
         };
 
     protected:
         ControlArray points;            // 控制点数组
 
         index_type index_lo{0};         // 当前有效索引范围的下界
         index_type index_hi{0};         // 当前有效索引范围的上界
 
         uint8 m_mode{UninitializedMode};// 当前插值模式
         bool cyclic{false};             // 是否循环样条
 
         // 每个线段的计算步数，影响精度和性能
         enum
         {
             STEPS_PER_SEGMENT = 3
         };
         static_assert(STEPS_PER_SEGMENT > 0, "shouldn't be lesser than 1");
 
     protected:
         // 线性插值计算函数
         void EvaluateLinear(index_type, float, Vector3&) const;
         // Catmull-Rom 插值计算函数
         void EvaluateCatmullRom(index_type, float, Vector3&) const;
         // Bezier 三次插值计算函数（未使用）
         void EvaluateBezier3(index_type, float, Vector3&) const;
         typedef void (SplineBase::*EvaluationMethtod)(index_type, float, Vector3&) const;
         static EvaluationMethtod evaluators[ModesEnd];  // 插值函数指针数组
 
         // 线性插值导数计算函数
         void EvaluateDerivativeLinear(index_type, float, Vector3&) const;
         // Catmull-Rom 插值导数计算函数
         void EvaluateDerivativeCatmullRom(index_type, float, Vector3&) const;
         // Bezier 三次导数计算函数（未使用）
         void EvaluateDerivativeBezier3(index_type, float, Vector3&) const;
         static EvaluationMethtod derivative_evaluators[ModesEnd];  // 导数插值函数指针数组
 
         // 线段长度计算函数 - 线性
         [[nodiscard]] float SegLengthLinear(index_type) const;
         // 线段长度计算函数 - Catmull-Rom
         [[nodiscard]] float SegLengthCatmullRom(index_type) const;
         // 线段长度计算函数 - Bezier 三次
         [[nodiscard]] float SegLengthBezier3(index_type) const;
         typedef float (SplineBase::*SegLenghtMethtod)(index_type) const;
         static SegLenghtMethtod seglengths[ModesEnd];  // 线段长度计算函数指针数组
 
         // 初始化线性样条
         void InitLinear(const Vector3*, index_type, bool, index_type);
         // 初始化 Catmull-Rom 样条
         void InitCatmullRom(const Vector3*, index_type, bool, index_type);
         // 初始化 Bezier 三次样条
         void InitBezier3(const Vector3*, index_type, bool, index_type);
         typedef void (SplineBase::*InitMethtod)(const Vector3*, index_type, bool, index_type);
         static InitMethtod initializers[ModesEnd];  // 初始化函数指针数组
 
         // 未初始化样条时调用的默认方法（触发断言）
         void UninitializedSplineEvaluationMethod(index_type, float, Vector3&) const { ABORT(); }
         [[nodiscard]] float UninitializedSplineSegLenghtMethod(index_type) const { ABORT(); }
         void UninitializedSplineInitMethod(Vector3 const*, index_type, bool, index_type) { ABORT(); }
 
     public:
         explicit SplineBase()  = default;
 
         /**
          * 根据给定的段索引和段长度百分比计算位置
          * @param Idx - 样条段索引，应在 [first, last) 范围内
          * @param u - 段长度的百分比，范围 [0, 1]
          */
         void evaluate_percent(index_type Idx, float u, Vector3& c) const {(this->*evaluators[m_mode])(Idx, u, c);}
 
         /**
          * 计算给定段索引和段长度百分比的导数
          * @param Idx - 样条段索引，应在 [first, last) 范围内
          * @param u - 段长度的百分比，范围 [0, 1]
          */
         void evaluate_derivative(index_type Idx, float u, Vector3& hermite) const {(this->*derivative_evaluators[m_mode])(Idx, u, hermite);}
 
         /**
          * 获取样条索引的边界范围
          * 所有索引应在 [first, last) 范围内
          */
         [[nodiscard]] index_type first() const { return index_lo;}
         [[nodiscard]] index_type last()  const { return index_hi;}
 
         [[nodiscard]] bool empty() const { return index_lo == index_hi;}       // 判断是否为空
         [[nodiscard]] EvaluationMode mode() const { return (EvaluationMode)m_mode; }  // 获取当前插值模式
         [[nodiscard]] bool isCyclic() const { return cyclic;}                 // 判断是否为循环样条
 
         // Xinef: 仅用于样条初始化，请勿在其他场合使用！！！
         [[nodiscard]] const ControlArray& getPoints() const { return points;} // 获取控制点数组
         [[nodiscard]] index_type getPointCount() const { return points.size();} // 获取控制点数量
         [[nodiscard]] const Vector3& getPoint(index_type i) const { return points[i];} // 获取指定索引的控制点
 
         /**
          * 初始化样条，在未初始化前不要调用其他方法
          */
         void init_spline(const Vector3* controls, index_type count, EvaluationMode m);
         void init_cyclic_spline(const Vector3* controls, index_type count, EvaluationMode m, index_type cyclic_point);
 
         /**
          * 自定义初始化方法
          * 可以根据不同的初始化器进行初始化
          */
         template<class Init> inline void init_spline_custom(Init& initializer)
         {
             initializer(m_mode, cyclic, points, index_lo, index_hi);
         }
 
         void clear();  // 清除样条数据
 
         /**
          * 计算指定段的长度
          * 假设索引 i 在有效范围内
          */
         [[nodiscard]] float SegLength(index_type i) const { return (this->*seglengths[m_mode])(i);}
 
         [[nodiscard]] std::string ToString() const;  // 返回样条的字符串表示
     };
 
     template<typename length_type>
     class Spline : public SplineBase
     {
     public:
         typedef length_type LengthType;           // 长度类型定义
         typedef std::vector<length_type> LengthArray;  // 长度数组类型定义
 
     protected:
         LengthArray lengths;                      // 各段长度的累计数组
 
         /**
          * 根据给定长度计算对应的段索引
          * 确保索引在有效范围内
          */
         [[nodiscard]] index_type computeIndexInBounds(length_type length) const;
 
     public:
         explicit Spline() = default;
 
         /**
          * 根据给定的 t（总长度的百分比）计算位置
          * @param t - 总长度的百分比，范围 [0, 1]
          */
         void evaluate_percent(float t, Vector3& c) const;
 
         /**
          * 根据给定的 t（总长度的百分比）计算导数
          * @param t - 总长度的百分比，范围 [0, 1]
          */
         void evaluate_derivative(float t, Vector3& hermite) const;
 
         /**
          * 根据给定段索引和段长度百分比计算位置
          * @param u - 段长度的百分比，范围 [0, 1]
          * @param Idx - 样条段索引，应在 [first, last) 范围内
          */
         void evaluate_percent(index_type Idx, float u, Vector3& c) const { SplineBase::evaluate_percent(Idx, u, c);}
 
         /**
          * 根据给定段索引和段长度百分比计算导数
          * @param u - 段长度的百分比，范围 [0, 1]
          * @param Idx - 样条段索引，应在 [first, last) 范围内
          */
         void evaluate_derivative(index_type Idx, float u, Vector3& c) const { SplineBase::evaluate_derivative(Idx, u, c);}
 
         /**
          * 根据给定的 t（总长度的百分比）计算对应的段索引
          * 假设 t 在 [0, 1] 范围内
          */
         [[nodiscard]] index_type computeIndexInBounds(float t) const;
 
         /**
          * 根据给定的 t（总长度的百分比）计算对应的段索引和段内百分比
          */
         void computeIndex(float t, index_type& out_idx, float& out_u) const;
 
         /**
          * 初始化样条，在未初始化前不要调用其他方法
          */
         void init_spline(const Vector3* controls, index_type count, EvaluationMode m) { SplineBase::init_spline(controls, count, m);}
         void init_cyclic_spline(const Vector3* controls, index_type count, EvaluationMode m, index_type cyclic_point) { SplineBase::init_cyclic_spline(controls, count, m, cyclic_point);}
 
         /**
          * 使用 SplineBase::SegLength 方法初始化各段长度数组
          */
         void initLengths();
 
         /**
          * 使用自定义方式初始化各段长度数组
          * 注意：cacher 返回的值必须大于或等于前一个值
          */
         template<class T> inline void initLengths(T& cacher)
         {
             index_type i = index_lo;
             lengths.resize(index_hi + 1);
             length_type prev_length = 0, new_length = 0;
             while (i < index_hi)
             {
                 new_length = cacher(*this, i);
                 // 长度溢出时赋值为最大正值
                 if (new_length < 0)
                     new_length = std::numeric_limits<length_type>::max();
                 lengths[++i] = new_length;
 
                 ASSERT(prev_length <= new_length);
                 prev_length = new_length;
             }
         }
 
         /**
          * 返回整个样条的总长度
          */
         [[nodiscard]] length_type length() const { return lengths[index_hi];}
 
         /**
          * 返回指定节点之间的长度
          */
         [[nodiscard]] length_type length(index_type first, index_type last) const { return lengths[last] - lengths[first];}
 
         /**
          * 返回指定索引位置的累计长度
          */
         [[nodiscard]] length_type length(index_type Idx) const { return lengths[Idx];}
 
         /**
          * 设置指定索引位置的累计长度
          */
         void set_length(index_type i, length_type length) { lengths[i] = length;}
 
         /**
          * 清除样条数据
          */
         void clear();
     };
 }
 
 #include "SplineImpl.h"
 
 #endif // AC_SPLINE_H
