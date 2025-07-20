#ifndef _REGULAR_GRID_H
#define _REGULAR_GRID_H

#include <G3D/PositionTrait.h>
#include <G3D/Ray.h>
#include <G3D/Table.h>

#include "Errors.h"

// 节点数组类，用于存储最多9个节点的固定大小数组
template <class Node>
class NodeArray
{
public:
    explicit NodeArray() { memset(&_nodes, 0, sizeof(_nodes)); }

    // 添加节点到数组中（如果尚未存在）
    void AddNode(Node* n)
    {
        for (uint8 i = 0; i < 9; ++i)
            if (_nodes[i] == 0)
            {
                _nodes[i] = n;
                return;
            }
            else if (_nodes[i] == n)
            {
                return;
            }
    }

    Node* _nodes[9]; // 存储节点的数组
};

// 节点创建器模板类，用于创建节点对象
template<class Node>
struct NodeCreator
{
    static Node* makeNode(int /*x*/, int /*y*/) { return new Node(); }
};

// 二维规则网格模板类，用于空间分区和碰撞检测
template<class T,
         class Node,
         class NodeCreatorFunc = NodeCreator<Node>,
         /*class BoundsFunc = BoundsTrait<T>,*/
         class PositionFunc = PositionTrait<T>
         >
class RegularGrid2D
{
public:
    // 网格单元数量（64x64）
    enum
    {
        CELL_NUMBER = 64,
    };

#define HGRID_MAP_SIZE  (533.33333f * 64.f)     // 地图尺寸（不要修改）
#define CELL_SIZE       float(HGRID_MAP_SIZE/(float)CELL_NUMBER) // 每个单元格的尺寸

    // 成员表类型，用于存储对象到节点数组的映射
    typedef G3D::Table<const T*, NodeArray<Node>> MemberTable;

    MemberTable memberTable; // 对象到节点数组的映射表
    Node* nodes[CELL_NUMBER][CELL_NUMBER]; // 二维网格中的各个节点

    // 构造函数，初始化网格节点为null
    RegularGrid2D()
    {
        memset(nodes, 0, sizeof(nodes));
    }

    // 析构函数，释放所有节点内存
    ~RegularGrid2D()
    {
        for (int x = 0; x < CELL_NUMBER; ++x)
            for (int y = 0; y < CELL_NUMBER; ++y)
            {
                delete nodes[x][y];
            }
    }

    // 插入一个对象到网格中
    void insert(const T& value)
    {
        G3D::Vector3 pos[9];
        pos[0] = value.GetBounds().corner(0);
        pos[1] = value.GetBounds().corner(1);
        pos[2] = value.GetBounds().corner(2);
        pos[3] = value.GetBounds().corner(3);
        pos[4] = (pos[0] + pos[1]) / 2.0f;
        pos[5] = (pos[1] + pos[2]) / 2.0f;
        pos[6] = (pos[2] + pos[3]) / 2.0f;
        pos[7] = (pos[3] + pos[0]) / 2.0f;
        pos[8] = (pos[0] + pos[2]) / 2.0f;

        NodeArray<Node> na;
        for (uint8 i = 0; i < 9; ++i)
        {
            Cell c = Cell::ComputeCell(pos[i].x, pos[i].y);
            if (!c.isValid())
            {
                continue;
            }
            Node& node = getGridFor(pos[i].x, pos[i].y);
            na.AddNode(&node);
        }

        for (uint8 i = 0; i < 9; ++i)
        {
            if (na._nodes[i])
            {
                na._nodes[i]->insert(value);
            }
            else
            {
                break;
            }
        }

        memberTable.set(&value, na);
    }

    // 从网格中移除一个对象
    void remove(const T& value)
    {
        NodeArray<Node>& na = memberTable[&value];
        for (uint8 i = 0; i < 9; ++i)
        {
            if (na._nodes[i])
            {
                na._nodes[i]->remove(value);
            }
            else
            {
                break;
            }
        }

        // 从成员表中移除该对象
        memberTable.remove(&value);
    }

    // 平衡网格中的所有节点（优化性能）
    void balance()
    {
        for (int x = 0; x < CELL_NUMBER; ++x)
            for (int y = 0; y < CELL_NUMBER; ++y)
                if (Node* n = nodes[x][y])
                {
                    n->balance();
                }
    }

    // 检查网格是否包含指定的对象
    bool contains(const T& value) const { return memberTable.containsKey(&value); }

    // 获取网格中对象的总数
    int size() const { return memberTable.size(); }

    // 单元格结构体，表示网格中的一个单元
    struct Cell
    {
        int x, y;
        bool operator == (const Cell& c2) const { return x == c2.x && y == c2.y;}

        // 根据坐标计算对应的单元格
        static Cell ComputeCell(float fx, float fy)
        {
            Cell c = { int(fx * (1.f / CELL_SIZE) + (CELL_NUMBER / 2)), int(fy * (1.f / CELL_SIZE) + (CELL_NUMBER / 2)) };
            return c;
        }

        // 检查单元格坐标是否有效
        bool isValid() const { return x >= 0 && x < CELL_NUMBER && y >= 0 && y < CELL_NUMBER;}
    };

    // 根据坐标获取对应的网格节点
    Node& getGridFor(float fx, float fy)
    {
        Cell c = Cell::ComputeCell(fx, fy);
        return getGrid(c.x, c.y);
    }

    // 获取指定坐标的网格节点（如果不存在则创建）
    Node& getGrid(int x, int y)
    {
        ASSERT(x < CELL_NUMBER && y < CELL_NUMBER);
        if (!nodes[x][y])
        {
            nodes[x][y] = NodeCreatorFunc::makeNode(x, y);
        }
        return *nodes[x][y];
    }

    // 射线检测函数（带终点位置）
    template<typename RayCallback>
    void intersectRay(const G3D::Ray& ray, RayCallback& intersectCallback, float max_dist, const G3D::Vector3& end, bool stopAtFirstHit)
    {
        Cell cell = Cell::ComputeCell(ray.origin().x, ray.origin().y);
        if (!cell.isValid())
        {
            return;
        }

        Cell last_cell = Cell::ComputeCell(end.x, end.y);

        if (cell == last_cell)
        {
            if (Node* node = nodes[cell.x][cell.y])
            {
                node->intersectRay(ray, intersectCallback, max_dist, stopAtFirstHit);
            }
            return;
        }

        float voxel = (float)CELL_SIZE;
        float kx_inv = ray.invDirection().x, bx = ray.origin().x;
        float ky_inv = ray.invDirection().y, by = ray.origin().y;

        int stepX, stepY;
        float tMaxX, tMaxY;
        if (kx_inv >= 0)
        {
            stepX = 1;
            float x_border = (cell.x + 1) * voxel;
            tMaxX = (x_border - bx) * kx_inv;
        }
        else
        {
            stepX = -1;
            float x_border = (cell.x - 1) * voxel;
            tMaxX = (x_border - bx) * kx_inv;
        }

        if (ky_inv >= 0)
        {
            stepY = 1;
            float y_border = (cell.y + 1) * voxel;
            tMaxY = (y_border - by) * ky_inv;
        }
        else
        {
            stepY = -1;
            float y_border = (cell.y - 1) * voxel;
            tMaxY = (y_border - by) * ky_inv;
        }

        float tDeltaX = voxel * std::fabs(kx_inv);
        float tDeltaY = voxel * std::fabs(ky_inv);
        do
        {
            if (Node* node = nodes[cell.x][cell.y])
            {
                node->intersectRay(ray, intersectCallback, max_dist, stopAtFirstHit);
            }
            if (cell == last_cell)
            {
                break;
            }
            if (tMaxX < tMaxY)
            {
                tMaxX += tDeltaX;
                cell.x += stepX;
            }
            else
            {
                tMaxY += tDeltaY;
                cell.y += stepY;
            }
        } while (cell.isValid());
    }

    // 点检测函数，用于查询与指定点相交的对象
    template<typename IsectCallback>
    void intersectPoint(const G3D::Vector3& point, IsectCallback& intersectCallback)
    {
        Cell cell = Cell::ComputeCell(point.x, point.y);
        if (!cell.isValid())
        {
            return;
        }
        if (Node* node = nodes[cell.x][cell.y])
        {
            node->intersectPoint(point, intersectCallback);
        }
    }

    // 优化版本的射线检测函数，专门用于垂直方向的射线
    template<typename RayCallback>
    void intersectZAllignedRay(const G3D::Ray& ray, RayCallback& intersectCallback, float& max_dist)
    {
        Cell cell = Cell::ComputeCell(ray.origin().x, ray.origin().y);
        if (!cell.isValid())
        {
            return;
        }
        if (Node* node = nodes[cell.x][cell.y])
        {
            node->intersectRay(ray, intersectCallback, max_dist, false);
        }
    }
};

#undef CELL_SIZE
#undef HGRID_MAP_SIZE

#endif