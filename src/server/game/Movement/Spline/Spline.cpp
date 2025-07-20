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

 #include "Spline.h"
 #include <G3D/Matrix4.h>
 #include <sstream>
 
 namespace Movement
 {
 
     // 定义不同插值模式对应的评估方法
     SplineBase::EvaluationMethtod SplineBase::evaluators[SplineBase::ModesEnd] =
     {
         &SplineBase::EvaluateLinear,               // 线性插值
         &SplineBase::EvaluateCatmullRom,           // Catmull-Rom 插值
         &SplineBase::EvaluateBezier3,              // 三次贝塞尔插值
         &SplineBase::UninitializedSplineEvaluationMethod, // 未初始化的方法
     };
 
     // 定义不同插值模式对应的导数评估方法
     SplineBase::EvaluationMethtod SplineBase::derivative_evaluators[SplineBase::ModesEnd] =
     {
         &SplineBase::EvaluateDerivativeLinear,               // 线性插值的导数
         &SplineBase::EvaluateDerivativeCatmullRom,           // Catmull-Rom 插值的导数
         &SplineBase::EvaluateDerivativeBezier3,              // 三次贝塞尔插值的导数
         &SplineBase::UninitializedSplineEvaluationMethod,    // 未初始化的方法
     };
 
     // 定义不同插值模式对应的段长度计算方法
     SplineBase::SegLenghtMethtod SplineBase::seglengths[SplineBase::ModesEnd] =
     {
         &SplineBase::SegLengthLinear,               // 线性插值段长度
         &SplineBase::SegLengthCatmullRom,           // Catmull-Rom 插值段长度
         &SplineBase::SegLengthBezier3,              // 三次贝塞尔插值段长度
         &SplineBase::UninitializedSplineSegLenghtMethod, // 未初始化的方法
     };
 
     // 定义不同插值模式对应的初始化方法
     SplineBase::InitMethtod SplineBase::initializers[SplineBase::ModesEnd] =
     {
         //&SplineBase::InitLinear,
         &SplineBase::InitCatmullRom,    // 即使是线性模式也使用CatmullRom初始化（客户端结构限制）
         &SplineBase::InitCatmullRom,    // Catmull-Rom 初始化
         &SplineBase::InitBezier3,       // 三次贝塞尔初始化
         &SplineBase::UninitializedSplineInitMethod, // 未初始化的方法
     };
 
     ///////////
 
     using G3D::Matrix4;
 
     // Catmull-Rom 插值系数矩阵
     static const Matrix4 s_catmullRomCoeffs(
         -0.5f, 1.5f, -1.5f, 0.5f,
         1.f, -2.5f, 2.f, -0.5f,
         -0.5f, 0.f,  0.5f, 0.f,
         0.f,  1.f,  0.f,  0.f);
 
     // 三次贝塞尔插值系数矩阵
     static const Matrix4 s_Bezier3Coeffs(
         -1.f,  3.f, -3.f, 1.f,
         3.f, -6.f,  3.f, 0.f,
         -3.f,  3.f,  0.f, 0.f,
         1.f,  0.f,  0.f, 0.f);
 
     // 使用矩阵进行插值计算
     inline void C_Evaluate(const Vector3* vertice, float t, const Matrix4& matr, Vector3& result)
     {
         Vector4 tvec(t * t * t, t * t, t, 1.f);
         Vector4 weights(tvec * matr); // 计算权重
 
         // 使用权重计算最终坐标
         result = vertice[0] * weights[0] + vertice[1] * weights[1]
                  + vertice[2] * weights[2] + vertice[3] * weights[3];
     }
 
     // 计算插值导数
     inline void C_Evaluate_Derivative(const Vector3* vertice, float t, const Matrix4& matr, Vector3& result)
     {
         Vector4 tvec(3.f * t * t, 2.f * t, 1.f, 0.f);
         Vector4 weights(tvec * matr); // 计算导数权重
 
         // 使用权重计算导数
         result = vertice[0] * weights[0] + vertice[1] * weights[1]
                  + vertice[2] * weights[2] + vertice[3] * weights[3];
     }
 
     // 线性插值实现
     void SplineBase::EvaluateLinear(index_type index, float u, Vector3& result) const
     {
         ASSERT(index >= index_lo && index < index_hi);
         result = points[index] + (points[index + 1] - points[index]) * u;
     }
 
     // Catmull-Rom 插值实现
     void SplineBase::EvaluateCatmullRom( index_type index, float t, Vector3& result) const
     {
         ASSERT(index >= index_lo && index < index_hi);
         C_Evaluate(&points[index - 1], t, s_catmullRomCoeffs, result);
     }
 
     // 三次贝塞尔插值实现
     void SplineBase::EvaluateBezier3(index_type index, float t, Vector3& result) const
     {
         index *= 3u;
         ASSERT(index >= index_lo && index < index_hi);
         C_Evaluate(&points[index], t, s_Bezier3Coeffs, result);
     }
 
     // 线性插值导数计算
     void SplineBase::EvaluateDerivativeLinear(index_type index, float, Vector3& result) const
     {
         ASSERT(index >= index_lo && index < index_hi);
         result = points[index + 1] - points[index];
     }
 
     // Catmull-Rom 插值导数计算
     void SplineBase::EvaluateDerivativeCatmullRom(index_type index, float t, Vector3& result) const
     {
         ASSERT(index >= index_lo && index < index_hi);
         C_Evaluate_Derivative(&points[index - 1], t, s_catmullRomCoeffs, result);
     }
 
     // 三次贝塞尔插值导数计算
     void SplineBase::EvaluateDerivativeBezier3(index_type index, float t, Vector3& result) const
     {
         index *= 3u;
         ASSERT(index >= index_lo && index < index_hi);
         C_Evaluate_Derivative(&points[index], t, s_Bezier3Coeffs, result);
     }
 
     // 计算线性段长度
     float SplineBase::SegLengthLinear(index_type index) const
     {
         ASSERT(index >= index_lo && index < index_hi);
         return (points[index] - points[index + 1]).length();
     }
 
     // 使用Catmull-Rom方法计算段长度
     float SplineBase::SegLengthCatmullRom(index_type index) const
     {
         ASSERT(index >= index_lo && index < index_hi);
 
         Vector3 curPos, nextPos;
         const Vector3* p = &points[index - 1];
         curPos = nextPos = p[1];
 
         index_type i = 1;
         double length = 0;
         while (i <= STEPS_PER_SEGMENT)
         {
             C_Evaluate(p, float(i) / float(STEPS_PER_SEGMENT), s_catmullRomCoeffs, nextPos);
             length += (nextPos - curPos).length();
             curPos = nextPos;
             ++i;
         }
         return length;
     }
 
     // 使用三次贝塞尔方法计算段长度
     float SplineBase::SegLengthBezier3(index_type index) const
     {
         index *= 3u;
         ASSERT(index >= index_lo && index < index_hi);
 
         Vector3 curPos, nextPos;
         const Vector3* p = &points[index];
 
         C_Evaluate(p, 0.f, s_Bezier3Coeffs, nextPos);
         curPos = nextPos;
 
         index_type i = 1;
         double length = 0;
         while (i <= STEPS_PER_SEGMENT)
         {
             C_Evaluate(p, float(i) / float(STEPS_PER_SEGMENT), s_Bezier3Coeffs, nextPos);
             length += (nextPos - curPos).length();
             curPos = nextPos;
             ++i;
         }
         return length;
     }
 
     // 初始化样条曲线
     void SplineBase::init_spline(const Vector3* controls, index_type count, EvaluationMode m)
     {
         m_mode = m;
         cyclic = false;
 
         (this->*initializers[m_mode])(controls, count, cyclic, 0);
     }
 
     // 初始化循环样条曲线
     void SplineBase::init_cyclic_spline(const Vector3* controls, index_type count, EvaluationMode m, index_type cyclic_point)
     {
         m_mode = m;
         cyclic = true;
 
         (this->*initializers[m_mode])(controls, count, cyclic, cyclic_point);
     }
 
     // 线性初始化方法
     void SplineBase::InitLinear(const Vector3* controls, index_type count, bool cyclic, index_type cyclic_point)
     {
         ASSERT(count >= 2);
         const int real_size = count + 1;
 
         points.resize(real_size);
 
         memcpy(&points[0], controls, sizeof(Vector3) * count);
 
         // 前后两个索引用于特殊“虚拟点”，用于C_Evaluate和C_Evaluate_Derivative方法正常工作
         if (cyclic)
             points[count] = controls[cyclic_point];
         else
             points[count] = controls[count - 1];
 
         index_lo = 0;
         index_hi = cyclic ? count : (count - 1);
     }
 
     // Catmull-Rom 初始化方法
     void SplineBase::InitCatmullRom(const Vector3* controls, index_type count, bool cyclic, index_type cyclic_point)
     {
         const int real_size = count + (cyclic ? (1 + 2) : (1 + 1));
 
         points.resize(real_size);
 
         int lo_index = 1;
         int high_index = lo_index + count - 1;
 
         memcpy(&points[lo_index], controls, sizeof(Vector3) * count);
 
         // 前后两个索引用于特殊“虚拟点”，用于C_Evaluate和C_Evaluate_Derivative方法正常工作
         if (cyclic)
         {
             if (cyclic_point == 0)
                 points[0] = controls[count - 1];
             else
                 points[0] = controls[0].lerp(controls[1], -1);
 
             points[high_index + 1] = controls[cyclic_point];
             points[high_index + 2] = controls[cyclic_point + 1];
         }
         else
         {
             points[0] = controls[0].lerp(controls[1], -1);
             points[high_index + 1] = controls[count - 1];
         }
 
         index_lo = lo_index;
         index_hi = high_index + (cyclic ? 1 : 0);
     }
 
     // 三次贝塞尔初始化方法
     void SplineBase::InitBezier3(const Vector3* controls, index_type count, bool /*cyclic*/, index_type /*cyclic_point*/)
     {
         index_type c = count / 3u * 3u;
         index_type t = c / 3u;
 
         points.resize(c);
         memcpy(&points[0], controls, sizeof(Vector3) * c);
 
         index_lo = 0;
         index_hi = t - 1;
         //mov_assert(points.size() % 3 == 0);
     }
 
     // 清空数据
     void SplineBase::clear()
     {
         index_lo = 0;
         index_hi = 0;
         points.clear();
     }
 
     // 转换为字符串输出
     std::string SplineBase::ToString() const
     {
         std::stringstream str;
         const char* mode_str[ModesEnd] = {"Linear", "CatmullRom", "Bezier3", "Uninitialized"};
 
         index_type count = this->points.size();
         str << "mode: " << mode_str[mode()] << std::endl;
         str << "points count: " << count << std::endl;
         for (index_type i = 0; i < count; ++i)
             str << "point " << i << " : " << points[i].toString() << std::endl;
 
         return str.str();
     }
 }
