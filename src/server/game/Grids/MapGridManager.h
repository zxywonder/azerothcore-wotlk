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

#ifndef MAP_GRID_MANAGER_H
#define MAP_GRID_MANAGER_H

#include "Common.h"
#include "GridDefines.h"
#include "MapDefines.h"
#include "MapGrid.h"

#include <mutex>

class Map;

class MapGridManager
{
public:
    // 构造函数，初始化地图指针，已创建的网格数量和已加载的网格数量
    MapGridManager(Map* map) : _map(map), _createdGridsCount(0), _loadedGridsCount(0) { }

    // 在指定坐标(x, y)创建一个网格
    void CreateGrid(uint16 const x, uint16 const y);
    // 加载指定坐标(x, y)的网格，返回是否加载成功
    bool LoadGrid(uint16 const x, uint16 const y);
    // 卸载指定坐标(x, y)的网格
    void UnloadGrid(uint16 const x, uint16 const y);
    // 检查指定坐标(x, y)的网格是否已创建
    bool IsGridCreated(uint16 const x, uint16 const y) const;
    // 检查指定坐标(x, y)的网格是否已加载
    bool IsGridLoaded(uint16 const x, uint16 const y) const;
    // 获取指定坐标(x, y)的网格指针
    MapGridType* GetGrid(uint16 const x, uint16 const y);

    // 静态方法，检查网格坐标(x, y)是否有效
    static bool IsValidGridCoordinates(uint16 const x, uint16 const y) { return (x < MAX_NUMBER_OF_GRIDS && y < MAX_NUMBER_OF_GRIDS); }

    // 获取已创建的网格数量
    uint32 GetCreatedGridsCount();
    // 获取已加载的网格数量
    uint32 GetLoadedGridsCount();
    // 获取指定坐标(x, y)网格中已创建的单元格数量
    uint32 GetCreatedCellsInGridCount(uint16 const x, uint16 const y);
    // 获取地图中已创建的单元格总数
    uint32 GetCreatedCellsInMapCount();

    // 检查地图中的网格是否全部已创建
    bool IsGridsFullyCreated() const;
    // 检查地图中的网格是否全部已加载
    bool IsGridsFullyLoaded() const;

private:
    // 地图指针
    Map* _map;

    // 已创建的网格数量
    uint32 _createdGridsCount;
    // 已加载的网格数量
    uint32 _loadedGridsCount;

    // 用于保护网格操作的互斥锁
    std::mutex _gridLock;
    // 二维数组，存储地图网格的智能指针
    std::unique_ptr<MapGridType> _mapGrid[MAX_NUMBER_OF_GRIDS][MAX_NUMBER_OF_GRIDS];
};

#endif
