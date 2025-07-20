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

#ifndef _IMMAPMANAGER_H
#define _IMMAPMANAGER_H

// IMMapManger 接口
namespace MMAP
{
    // MMAP 加载结果枚举
    enum MMAP_LOAD_RESULT
    {
        MMAP_LOAD_RESULT_ERROR,  // 加载出错
        MMAP_LOAD_RESULT_OK,     // 加载成功
        MMAP_LOAD_RESULT_IGNORED // 加载被忽略
    };

    // IMMapMgr 类定义
    class IMMapMgr
    {
    private:
        // 是否启用寻路功能的标志
        bool iEnablePathFinding;

    public:
        // 构造函数，默认启用寻路功能
        IMMapMgr() : iEnablePathFinding(true) {}
        // 虚析构函数
        virtual ~IMMapMgr(void) {}

        // 设置是否启用寻路功能
        void setEnablePathFinding(bool value) { iEnablePathFinding = value; }
        // 获取是否启用寻路功能
        bool isEnablePathFinding() const { return (iEnablePathFinding); }
    };
}

#endif
