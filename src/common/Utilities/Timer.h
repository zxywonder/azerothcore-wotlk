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

 #ifndef ACORE_TIMER_H
 #define ACORE_TIMER_H
 
 #include "Common.h"
 #include "Duration.h"
 
 // 时间格式枚举，用于控制时间字符串输出格式
 enum class TimeFormat : uint8
 {
     FullText,  // 完整文本格式，如：1 Days 2 Hours 3 Minutes 4 Seconds 5 Milliseconds
     ShortText, // 简写文本格式，如：1d 2h 3m 4s 5ms
     Numeric    // 数字格式，如：1:2:3:4:5
 };
 
 // 时间输出精度枚举，用于控制输出中包含的时间单位
 enum class TimeOutput : uint8
 {
     Days,         // 输出到天数，例如：1d
     Hours,        // 输出到小时，例如：1d 2h
     Minutes,      // 输出到分钟，例如：1d 2h 3m
     Seconds,      // 输出到秒，例如：1d 2h 3m 4s
     Milliseconds, // 输出到毫秒，例如：1d 2h 3m 4s 5ms
     Microseconds  // 输出到微秒，例如：1d 2h 3m 4s 5ms 6us
 };
 
 namespace Acore::Time
 {
     // 将字符串转换为时间值，模板函数，支持不同时间类型
     template <class T>
     AC_COMMON_API uint32 TimeStringTo(std::string_view timeString);
 
     // 将持续时间转换为时间字符串，指定输出精度和格式
     template<class T>
     AC_COMMON_API std::string ToTimeString(uint64 durationTime, TimeOutput timeOutput = TimeOutput::Seconds, TimeFormat timeFormat = TimeFormat::ShortText);
 
     // 从字符串表示的持续时间生成时间字符串
     template<class T>
     AC_COMMON_API std::string ToTimeString(std::string_view durationTime, TimeOutput timeOutput = TimeOutput::Seconds, TimeFormat timeFormat = TimeFormat::ShortText);
 
     // 以Microseconds类型输入，生成格式化时间字符串
     AC_COMMON_API std::string ToTimeString(Microseconds durationTime, TimeOutput timeOutput = TimeOutput::Seconds, TimeFormat timeFormat = TimeFormat::ShortText);
 
     // 将本地时间转换为UTC时间
     AC_COMMON_API time_t LocalTimeToUTCTime(time_t time);
     // 获取某时间点当天指定小时的时间戳
     AC_COMMON_API time_t GetLocalHourTimestamp(time_t time, uint8 hour, bool onlyAfterTime = true);
     // 将时间戳分解为时间结构体tm
     AC_COMMON_API std::tm TimeBreakdown(time_t t = 0);
     // 将秒数转换为时间戳字符串
     AC_COMMON_API std::string TimeToTimestampStr(Seconds time = 0s, std::string_view fmt = {});
     // 将时间转换为可读性好的字符串
     AC_COMMON_API std::string TimeToHumanReadable(Seconds time = 0s, std::string_view fmt = {});
 
     // 获取指定星期几和小时的下一个时间点
     AC_COMMON_API time_t GetNextTimeWithDayAndHour(int8 dayOfWeek, int8 hour); // dayOfWeek: 0（周日）到6（周六）
     // 获取指定月份和小时的下一个时间点
     AC_COMMON_API time_t GetNextTimeWithMonthAndHour(int8 month, int8 hour); // month: 0（一月）到11（十二月）
 
     // 获取指定时间中的秒数 [0, 60]
     AC_COMMON_API uint32 GetSeconds(Seconds time = 0s);
     // 获取指定时间中的分钟数 [0, 59]
     AC_COMMON_API uint32 GetMinutes(Seconds time = 0s);
     // 获取指定时间中的小时数 [0, 23]
     AC_COMMON_API uint32 GetHours(Seconds time = 0s);
     // 获取一周中的第几天（从周日开始）[0, 6]
     AC_COMMON_API uint32 GetDayInWeek(Seconds time = 0s);
     // 获取一月中的第几天 [1, 31]
     AC_COMMON_API uint32 GetDayInMonth(Seconds time = 0s);
     // 获取一年中的第几天 [0, 365]
     AC_COMMON_API uint32 GetDayInYear(Seconds time = 0s);
     // 获取月份（从一月开始）[0, 11]
     AC_COMMON_API uint32 GetMonth(Seconds time = 0s);
     // 获取年份（自1900年起）
     AC_COMMON_API uint32 GetYear(Seconds time = 0s);
 }
 
 // 线程安全的localtime函数
 AC_COMMON_API struct tm* localtime_r(time_t const* time, struct tm* result);
 
 // 获取应用程序启动时间
 inline TimePoint GetApplicationStartTime()
 {
     using namespace std::chrono;
 
     static const TimePoint ApplicationStartTime = steady_clock::now();
 
     return ApplicationStartTime;
 }
 
 // 获取当前时间距离启动的毫秒数
 inline Milliseconds GetTimeMS()
 {
     using namespace std::chrono;
 
     return duration_cast<milliseconds>(steady_clock::now() - GetApplicationStartTime());
 }
 
 // 计算两个毫秒时间点之间的差值
 inline Milliseconds GetMSTimeDiff(Milliseconds oldMSTime, Milliseconds newMSTime)
 {
     if (oldMSTime > newMSTime)
     {
         return oldMSTime - newMSTime;
     }
     else
     {
         return newMSTime - oldMSTime;
     }
 }
 
 // 获取当前时间距离启动的毫秒数（32位整型）
 inline uint32 getMSTime()
 {
     using namespace std::chrono;
 
     return uint32(duration_cast<milliseconds>(steady_clock::now() - GetApplicationStartTime()).count());
 }
 
 // 计算两个32位毫秒时间之间的差值，处理溢出情况
 inline uint32 getMSTimeDiff(uint32 oldMSTime, uint32 newMSTime)
 {
     // 如果新时间小于旧时间，说明可能发生了溢出
     if (oldMSTime > newMSTime)
     {
         return (0xFFFFFFFF - oldMSTime) + newMSTime;
     }
     else
     {
         return newMSTime - oldMSTime;
     }
 }
 
 // 计算一个32位毫秒时间和一个时间点之间的差值
 inline uint32 getMSTimeDiff(uint32 oldMSTime, TimePoint newTime)
 {
     using namespace std::chrono;
 
     uint32 newMSTime = uint32(duration_cast<milliseconds>(newTime - GetApplicationStartTime()).count());
     return getMSTimeDiff(oldMSTime, newMSTime);
 }
 
 // 获取当前时间与旧时间的毫秒差
 inline uint32 GetMSTimeDiffToNow(uint32 oldMSTime)
 {
     return getMSTimeDiff(oldMSTime, getMSTime());
 }
 
 // 获取当前时间与旧时间点的毫秒差（支持Milliseconds类型）
 inline Milliseconds GetMSTimeDiffToNow(Milliseconds oldMSTime)
 {
     return GetMSTimeDiff(oldMSTime, GetTimeMS());
 }
 
 // 获取当前时间的纪元秒数
 inline Seconds GetEpochTime()
 {
     using namespace std::chrono;
     return duration_cast<Seconds>(system_clock::now().time_since_epoch());
 }
 
 // 间隔计时器类，用于跟踪时间间隔是否已过
 struct IntervalTimer
 {
 public:
     IntervalTimer() = default;
 
     // 更新计时器状态
     void Update(time_t diff)
     {
         _current += diff;
         if (_current < 0)
         {
             _current = 0;
         }
     }
 
     // 判断时间间隔是否已经过去
     bool Passed()
     {
         return _current >= _interval;
     }
 
     // 重置计时器
     void Reset()
     {
         if (_current >= _interval)
         {
             _current %= _interval;
         }
     }
 
     // 设置当前计时器值
     void SetCurrent(time_t current)
     {
         _current = current;
     }
 
     // 设置间隔时间
     void SetInterval(time_t interval)
     {
         _interval = interval;
     }
 
     // 获取间隔时间
     [[nodiscard]] time_t GetInterval() const
     {
         return _interval;
     }
 
     // 获取当前时间累计值
     [[nodiscard]] time_t GetCurrent() const
     {
         return _current;
     }
 
 private:
     time_t _interval{0};
     time_t _current{0};
 };
 
 // 时间追踪器类，用于跟踪到期时间
 struct TimeTracker
 {
 public:
     // 构造函数，设置到期时间
     TimeTracker(time_t expiry)
         : i_expiryTime(expiry)
     {
     }
 
     // 更新剩余时间
     void Update(time_t diff)
     {
         i_expiryTime -= diff;
     }
 
     // 判断时间是否已过
     [[nodiscard]] bool Passed() const
     {
         return i_expiryTime <= 0;
     }
 
     // 重置到期时间
     void Reset(time_t interval)
     {
         i_expiryTime = interval;
     }
 
     // 获取到期时间
     [[nodiscard]] time_t GetExpiry() const
     {
         return i_expiryTime;
     }
 
 private:
     time_t i_expiryTime;
 };
 
 // 小型时间追踪器，使用int32存储时间
 struct TimeTrackerSmall
 {
 public:
     // 构造函数，设置到期时间
     TimeTrackerSmall(int32 expiry = 0)
         : i_expiryTime(expiry)
     {
     }
 
     // 更新剩余时间
     void Update(int32 diff)
     {
         i_expiryTime -= diff;
     }
 
     // 判断时间是否已过
     [[nodiscard]] bool Passed() const
     {
         return i_expiryTime <= 0;
     }
 
     // 重置到期时间
     void Reset(int32 interval)
     {
         i_expiryTime = interval;
     }
 
     // 获取到期时间
     [[nodiscard]] int32 GetExpiry() const
     {
         return i_expiryTime;
     }
 
 private:
     int32 i_expiryTime;
 };
 
 // 周期定时器类，用于周期性触发事件
 struct PeriodicTimer
 {
 public:
     // 构造函数，设置周期和初始到期时间
     PeriodicTimer(int32 period, int32 start_time)
         : i_period(period), i_expireTime(start_time)
     {
     }
 
     // 更新定时器，返回是否触发周期事件
     bool Update(const uint32 diff)
     {
         if ((i_expireTime -= diff) > 0)
         {
             return false;
         }
 
         i_expireTime += i_period > int32(diff) ? i_period : diff;
         return true;
     }
 
     // 设置周期和初始到期时间
     void SetPeriodic(int32 period, int32 start_time)
     {
         i_expireTime = start_time;
         i_period = period;
     }
 
     // 提供一个跟踪器接口
     void TUpdate(int32 diff) { i_expireTime -= diff; }
     [[nodiscard]] bool TPassed() const { return i_expireTime <= 0; }
     void TReset(int32 diff, int32 period)  { i_expireTime += period > diff ? period : diff; }
 
 private:
     int32 i_period;
     int32 i_expireTime;
 };
 
 #endif
