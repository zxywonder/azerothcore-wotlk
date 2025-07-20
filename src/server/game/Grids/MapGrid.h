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

#ifndef MAP_GRID_H
#define MAP_GRID_H

#include "GridCell.h"
#include "GridReference.h"

class GridTerrainData;

template
<
    class WORLD_OBJECT_TYPES,  // 世界对象类型
    class GRID_OBJECT_TYPES    // 网格对象类型
>
class MapGrid
{
public:
    // 定义网格单元格类型
    typedef GridCell<WORLD_OBJECT_TYPES, GRID_OBJECT_TYPES> GridCellType;

    // 构造函数，初始化网格的坐标，标记对象数据未加载，地形数据为空
    MapGrid(uint16 const x, uint16 const y)
        : _x(x), _y(y), _objectDataLoaded(false), _terrainData(nullptr) { }

    // 获取网格的唯一标识符
    uint32 GetId() const { return _y * MAX_NUMBER_OF_GRIDS + _x; }

    // 获取网格的 X 坐标
    uint16 GetX() const { return _x; }
    // 获取网格的 Y 坐标
    uint16 GetY() const { return _y; }

    // 检查对象数据是否已加载
    bool IsObjectDataLoaded() const { return _objectDataLoaded; }
    // 标记对象数据已加载
    void SetObjectDataLoaded() { _objectDataLoaded = true; }

    // 在指定坐标的单元格中添加一个世界对象
    template<class SPECIFIC_OBJECT> void AddWorldObject(uint16 const x, uint16 const y, SPECIFIC_OBJECT* obj)
    {
        GetOrCreateCell(x, y).AddWorldObject(obj);
    }

    // 从指定坐标的单元格中移除一个世界对象
    template<class SPECIFIC_OBJECT> void RemoveWorldObject(uint16 const x, uint16 const y, SPECIFIC_OBJECT* obj)
    {
        GetOrCreateCell(x, y).RemoveWorldObject(obj);
    }

    // 在指定坐标的单元格中添加一个网格对象
    template<class SPECIFIC_OBJECT> void AddGridObject(uint16 const x, uint16 const y, SPECIFIC_OBJECT* obj)
    {
        GetOrCreateCell(x, y).AddGridObject(obj);
    }

    // 从指定坐标的单元格中移除一个网格对象
    template<class SPECIFIC_OBJECT> void RemoveGridObject(uint16 const x, uint16 const y, SPECIFIC_OBJECT* obj)
    {
        GetOrCreateCell(x, y).RemoveGridObject(obj);
    }

    // 访问所有单元格
    template<class T, class TT>
    void VisitAllCells(TypeContainerVisitor<T, TypeMapContainer<TT> >& visitor)
    {
        for (auto& cellX : _cells)
        {
            for (auto& cellY : cellX)
            {
                if (!cellY)
                    continue;

                cellY->Visit(visitor);
            }
        }
    }

    // 访问单个单元格
    template<class T, class TT>
    void VisitCell(uint16 const x, uint16 const y, TypeContainerVisitor<T, TypeMapContainer<TT> >& visitor)
    {
        GridCellType* gridCell = GetCell(x, y);
        if (!gridCell)
            return;

        gridCell->Visit(visitor);
    }

    // 将当前网格链接到指定的管理器
    void link(GridRefMgr<MapGrid<WORLD_OBJECT_TYPES, GRID_OBJECT_TYPES>>* pTo)
    {
        _gridReference.link(pTo, this);
    }

    // 获取地形数据的原始指针
    GridTerrainData* GetTerrainData() const { return _terrainData.get(); }
    // 获取地形数据的共享指针
    std::shared_ptr<GridTerrainData> GetTerrainDataSharedPtr() { return _terrainData; }
    // 设置地形数据
    void SetTerrainData(std::shared_ptr<GridTerrainData> terrainData) { _terrainData = terrainData; }

    // 获取已创建的单元格数量
    uint32 GetCreatedCellsCount()
    {
        uint32 count = 0;
        for (auto& cellX : _cells)
        {
            for (auto& cellY : cellX)
            {
                if (!cellY)
                    continue;

                ++count;
            }
        }
        return count;
    }

private:
    // 如果单元格尚未创建，则创建并返回该单元格
    GridCellType& GetOrCreateCell(uint16 const x, uint16 const y)
    {
        GridCellType* cell = GetCell(x, y);
        if (!cell)
            _cells[x][y] = std::make_unique<GridCellType>();

        return *_cells[x][y];
    }

    // 获取指定坐标的单元格指针
    GridCellType* GetCell(uint16 const x, uint16 const y)
    {
        ASSERT(x < MAX_NUMBER_OF_CELLS && y < MAX_NUMBER_OF_CELLS);
        return _cells[x][y].get();
    }

    // 常量版本的获取指定坐标的单元格指针
    GridCellType const* GetCell(uint16 const x, uint16 const y) const
    {
        ASSERT(x < MAX_NUMBER_OF_CELLS && y < MAX_NUMBER_OF_CELLS);
        return _cells[x][y].get();
    }

    // 网格的 X 坐标
    uint16 _x;
    // 网格的 Y 坐标
    uint16 _y;

    // 标记对象数据是否已加载
    bool _objectDataLoaded;
    // N * N 的单元格数组
    std::array<std::array<std::unique_ptr<GridCellType>, MAX_NUMBER_OF_CELLS>, MAX_NUMBER_OF_CELLS> _cells; 
    // 网格引用
    GridReference<MapGrid<WORLD_OBJECT_TYPES, GRID_OBJECT_TYPES>> _gridReference;

    // 实例将共享父地图的地形数据
    std::shared_ptr<GridTerrainData> _terrainData;
};

#endif
