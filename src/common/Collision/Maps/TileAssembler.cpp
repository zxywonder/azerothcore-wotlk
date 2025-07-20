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

// 包含头文件
#include "TileAssembler.h"
#include "BoundingIntervalHierarchy.h"
#include "MapDefines.h"
#include "MapTree.h"
#include "VMapDefinitions.h"
#include <boost/filesystem.hpp>
#include <iomanip>
#include <set>
#include <sstream>

// 使用命名空间中的类型
using G3D::Vector3;
using G3D::AABox;
using G3D::inf;
using std::pair;

// 模板特化，用于获取 VMAP::ModelSpawn* 类型对象的边界
template<> struct BoundsTrait<VMAP::ModelSpawn*>
{
    // 获取对象的边界
    static void GetBounds(const VMAP::ModelSpawn* const& obj, G3D::AABox& out) { out = obj->GetBounds(); }
};

namespace VMAP
{
    // 读取文件块并与指定内容比较
    bool readChunk(FILE* rf, char* dest, const char* compare, uint32 len)
    {
        // 读取文件块，如果读取长度不符则返回 false
        if (fread(dest, sizeof(char), len, rf) != len) { return false; }
        // 比较读取内容与指定内容是否一致
        return memcmp(dest, compare, len) == 0;
    }

    // 对输入向量进行变换
    Vector3 ModelPosition::transform(const Vector3& pIn) const
    {
        // 先进行缩放变换
        Vector3 out = pIn * iScale;
        // 再进行旋转变换
        out = iRotation * out;
        return (out);
    }

    //=================================================================

    // TileAssembler 构造函数，初始化源目录和目标目录
    TileAssembler::TileAssembler(const std::string& pSrcDirName, const std::string& pDestDirName)
        : iDestDir(pDestDirName), iSrcDir(pSrcDirName)
    {
        // 创建目标目录
        boost::filesystem::create_directory(iDestDir);
        //init();
    }

    // TileAssembler 析构函数
    TileAssembler::~TileAssembler()
    {
        //delete iCoordModelMapping;
    }

    // 转换世界数据
    bool TileAssembler::convertWorld2()
    {
        // 读取地图生成数据
        bool success = readMapSpawns();
        if (!success)
        {
            return false;
        }

        // 导出地图数据
        for (MapData::iterator map_iter = mapData.begin(); map_iter != mapData.end() && success; ++map_iter)
        {
            // 构建全局地图树
            std::vector<ModelSpawn*> mapSpawns;
            UniqueEntryMap::iterator entry;
            printf("Calculating model bounds for map %u...\n", map_iter->first);
            for (entry = map_iter->second->UniqueEntries.begin(); entry != map_iter->second->UniqueEntries.end(); ++entry)
            {
                // M2模型在WDT/ADT放置数据中没有设置边界，零售端可能根本不用于视线计算
                if (entry->second.flags & MOD_M2)
                {
                    // 计算变换后的边界
                    if (!calculateTransformedBound(entry->second))
                    {
                        break;
                    }
                }
                else if (entry->second.flags & MOD_WORLDSPAWN) // WMO地图和地形地图使用不同的原点，需要调整
                {
                    /// @todo 移除提取器hack并取消注释以下行:
                    //entry->second.iPos += Vector3(533.33333f*32, 533.33333f*32, 0.f);
                    entry->second.iBound = entry->second.iBound + Vector3(533.33333f * 32, 533.33333f * 32, 0.f);
                }
                // 将模型生成数据添加到地图生成列表中
                mapSpawns.push_back(&(entry->second));
                // 插入已生成的模型文件名
                spawnedModelFiles.insert(entry->second.name);
            }

            printf("Creating map tree for map %u...\n", map_iter->first);
            // 创建边界区间层次结构树
            BIH pTree;

            try
            {
                // 构建树结构
                pTree.build(mapSpawns, BoundsTrait<ModelSpawn*>::GetBounds);
            }
            catch (std::exception& e)
            {
                printf("Exception ""%s"" when calling pTree.build", e.what());
                return false;
            }

            // ===> 可能将此代码移动到 StaticMapTree 类
            // 存储模型ID与节点索引的映射关系
            std::map<uint32, uint32> modelNodeIdx;
            for (uint32 i = 0; i < mapSpawns.size(); ++i)
            {
                modelNodeIdx.insert(pair<uint32, uint32>(mapSpawns[i]->ID, i));
            }

            // 写入地图树文件
            std::stringstream mapfilename;
            mapfilename << iDestDir << '/' << std::setfill('0') << std::setw(3) << map_iter->first << ".vmtree";
            FILE* mapfile = fopen(mapfilename.str().c_str(), "wb");
            if (!mapfile)
            {
                success = false;
                printf("Cannot open %s\n", mapfilename.str().c_str());
                break;
            }

            // 写入通用信息
            if (success && fwrite(VMAP_MAGIC, 1, 8, mapfile) != 8) { success = false; }
            // 打包全局图块ID
            uint32 globalTileID = StaticMapTree::packTileID(65, 65);
            // 获取全局图块的范围
            pair<TileMap::iterator, TileMap::iterator> globalRange = map_iter->second->TileEntries.equal_range(globalTileID);
            // 判断是否为分块地图
            char isTiled = globalRange.first == globalRange.second; // 只有没有地形(图块)的地图才有全局WMO
            if (success && fwrite(&isTiled, sizeof(char), 1, mapfile) != 1) { success = false; }
            // 写入节点信息
            if (success && fwrite("NODE", 4, 1, mapfile) != 1) { success = false; }
            if (success) { success = pTree.writeToFile(mapfile); }
            // 写入全局地图生成数据(WDT)，如果有(大多数实例)
            if (success && fwrite("GOBJ", 4, 1, mapfile) != 1) { success = false; }

            for (TileMap::iterator glob = globalRange.first; glob != globalRange.second && success; ++glob)
            {
                success = ModelSpawn::writeToFile(mapfile, map_iter->second->UniqueEntries[glob->second]);
            }

            fclose(mapfile);

            // <====

            // 写入地图图块文件，类似于ADT文件，只是带有额外的BSP树节点信息
            TileMap& tileEntries = map_iter->second->TileEntries;
            TileMap::iterator tile;
            for (tile = tileEntries.begin(); tile != tileEntries.end(); ++tile)
            {
                const ModelSpawn& spawn = map_iter->second->UniqueEntries[tile->second];
                if (spawn.flags & MOD_WORLDSPAWN) // WDT生成，当前保存为图块65/65...
                {
                    continue;
                }
                // 获取当前图块的生成数量
                uint32 nSpawns = tileEntries.count(tile->first);
                std::stringstream tilefilename;
                tilefilename.fill('0');
                tilefilename << iDestDir << '/' << std::setw(3) << map_iter->first << '_';
                uint32 x, y;
                // 解包图块ID获取坐标
                StaticMapTree::unpackTileID(tile->first, x, y);
                tilefilename << std::setw(2) << x << '_' << std::setw(2) << y << ".vmtile";
                if (FILE* tilefile = fopen(tilefilename.str().c_str(), "wb"))
                {
                    // 写入文件头
                    if (success && fwrite(VMAP_MAGIC, 1, 8, tilefile) != 8) { success = false; }
                    // 写入图块生成数量
                    if (success && fwrite(&nSpawns, sizeof(uint32), 1, tilefile) != 1) { success = false; }
                    // 写入图块生成数据
                    for (uint32 s = 0; s < nSpawns; ++s)
                    {
                        if (s)
                        {
                            ++tile;
                        }
                        const ModelSpawn& spawn2 = map_iter->second->UniqueEntries[tile->second];
                        success = success && ModelSpawn::writeToFile(tilefile, spawn2);
                        // 地图树节点在加载图块时需要更新的索引
                        std::map<uint32, uint32>::iterator nIdx = modelNodeIdx.find(spawn2.ID);
                        if (success && fwrite(&nIdx->second, sizeof(uint32), 1, tilefile) != 1) { success = false; }
                    }
                    fclose(tilefile);
                }
            }
            // break; //test, extract only first map; TODO: remvoe this line
        }

        // 添加临时游戏对象模型文件中列出的对象模型
        exportGameobjectModels();
        // 导出对象
        std::cout << "\nConverting Model Files" << std::endl;
        for (std::set<std::string>::iterator mfile = spawnedModelFiles.begin(); mfile != spawnedModelFiles.end(); ++mfile)
        {
            std::cout << "Converting " << *mfile << std::endl;
            if (!convertRawFile(*mfile))
            {
                std::cout << "error converting " << *mfile << std::endl;
                success = false;
                break;
            }
        }

        // 清理资源
        for (MapData::iterator map_iter = mapData.begin(); map_iter != mapData.end(); ++map_iter)
        {
            delete map_iter->second;
        }
        return success;
    }

    // 读取地图生成数据
    bool TileAssembler::readMapSpawns()
    {
        // 构建目录文件路径
        std::string fname = iSrcDir + "/dir_bin";
        FILE* dirf = fopen(fname.c_str(), "rb");
        if (!dirf)
        {
            printf("Could not read dir_bin file!\n");
            return false;
        }
        printf("Read coordinate mapping...\n");
        uint32 mapID, tileX, tileY, check = 0;
        G3D::Vector3 v1, v2;
        ModelSpawn spawn;
        while (!feof(dirf))
        {
            // 读取地图ID、图块X坐标、图块Y坐标、标志、名称集、唯一ID、位置、旋转、缩放、边界低点、边界高点、名称
            check = fread(&mapID, sizeof(uint32), 1, dirf);
            if (check == 0) // 到达文件末尾
            {
                break;
            }
            check += fread(&tileX, sizeof(uint32), 1, dirf);
            check += fread(&tileY, sizeof(uint32), 1, dirf);
            if (!ModelSpawn::readFromFile(dirf, spawn))
            {
                break;
            }

            MapSpawns* current;
            MapData::iterator map_iter = mapData.find(mapID);
            if (map_iter == mapData.end())
            {
                printf("spawning Map %d\n", mapID);
                mapData[mapID] = current = new MapSpawns();
            }
            else
            {
                current = map_iter->second;
            }

            // 插入唯一条目
            current->UniqueEntries.emplace(spawn.ID, spawn);
            // 插入图块条目
            current->TileEntries.insert(pair<uint32, uint32>(StaticMapTree::packTileID(tileX, tileY), spawn.ID));
        }
        // 检查文件读取是否出错
        bool success = (ferror(dirf) == 0);
        fclose(dirf);
        return success;
    }

    // 计算变换后的模型边界
    bool TileAssembler::calculateTransformedBound(ModelSpawn& spawn)
    {
        // 构建模型文件路径
        std::string modelFilename(iSrcDir);
        modelFilename.push_back('/');
        modelFilename.append(spawn.name);

        ModelPosition modelPosition;
        modelPosition.iDir = spawn.iRot;
        modelPosition.iScale = spawn.iScale;
        modelPosition.init();

        WorldModel_Raw raw_model;
        // 读取原始模型文件
        if (!raw_model.Read(modelFilename.c_str()))
        {
            return false;
        }

        // 获取模型组数量
        uint32 groups = raw_model.groupsArray.size();
        if (groups != 1)
        {
            printf("Warning: '%s' does not seem to be a M2 model!\n", modelFilename.c_str());
        }

        AABox modelBound;
        bool boundEmpty = true;

        for (uint32 g = 0; g < groups; ++g) // M2文件应该只有一组
        {
            std::vector<Vector3>& vertices = raw_model.groupsArray[g].vertexArray;

            if (vertices.empty())
            {
                std::cout << "error: model '" << spawn.name << "' has no geometry!" << std::endl;
                continue;
            }

            // 获取顶点数量
            uint32 nvectors = vertices.size();
            for (uint32 i = 0; i < nvectors; ++i)
            {
                // 对顶点进行变换
                Vector3 v = modelPosition.transform(vertices[i]);

                if (boundEmpty)
                {
                    modelBound = AABox(v, v), boundEmpty = false;
                }
                else
                {
                    // 合并边界
                    modelBound.merge(v);
                }
            }
        }
        // 更新模型边界
        spawn.iBound = modelBound + spawn.iPos;
        // 设置模型有边界标志
        spawn.flags |= MOD_HAS_BOUND;
        return true;
    }

#pragma pack(push, 1)
    // WMO液体头结构
    struct WMOLiquidHeader
    {
        int xverts, yverts, xtiles, ytiles;
        float pos_x;
        float pos_y;
        float pos_z;
        short material;
    };
#pragma pack(pop)
    //=================================================================
    // 转换原始模型文件
    bool TileAssembler::convertRawFile(const std::string& pModelFilename)
    {
        bool success = true;
        // 构建原始模型文件路径
        std::string filename = iSrcDir;
        if (filename.length() > 0)
        {
            filename.push_back('/');
        }
        filename.append(pModelFilename);

        WorldModel_Raw raw_model;
        // 读取原始模型文件
        if (!raw_model.Read(filename.c_str()))
        {
            return false;
        }

        // 写入世界模型
        WorldModel model;
        // 设置根WMO ID
        model.setRootWmoID(raw_model.RootWMOID);
        if (!raw_model.groupsArray.empty())
        {
            std::vector<GroupModel> groupsArray;

            // 获取模型组数量
            uint32 groups = raw_model.groupsArray.size();
            for (uint32 g = 0; g < groups; ++g)
            {
                GroupModel_Raw& raw_group = raw_model.groupsArray[g];
                // 创建组模型
                groupsArray.push_back(GroupModel(raw_group.mogpflags, raw_group.GroupWMOID, raw_group.bounds ));
                // 设置网格数据
                groupsArray.back().setMeshData(raw_group.vertexArray, raw_group.triangles);
                // 设置液体数据
                groupsArray.back().setLiquidData(raw_group.liquid);
            }

            // 设置组模型
            model.setGroupModels(groupsArray);
        }

        // 写入模型文件
        success = model.writeFile(iDestDir + "/" + pModelFilename + ".vmo");
        //std::cout << "readRawFile2: '" << pModelFilename << "' tris: " << nElements << " nodes: " << nNodes << std::endl;
        return success;
    }

    // 导出游戏对象模型
    void TileAssembler::exportGameobjectModels()
    {
        // 打开临时游戏对象模型文件
        FILE* model_list = fopen((iSrcDir + "/" + "temp_gameobject_models").c_str(), "rb");
        if (!model_list)
        {
            return;
        }

        char ident[8];
        // 读取文件标识并检查
        if (fread(ident, 1, 8, model_list) != 8 || memcmp(ident, VMAP::RAW_VMAP_MAGIC, 8) != 0)
        {
            fclose(model_list);
            return;
        }

        // 打开游戏对象模型文件副本
        FILE* model_list_copy = fopen((iDestDir + "/" + GAMEOBJECT_MODELS).c_str(), "wb");
        if (!model_list_copy)
        {
            fclose(model_list);
            return;
        }

        // 写入VMAP魔术标识
        fwrite(VMAP::VMAP_MAGIC, 1, 8, model_list_copy);

        uint32 name_length, displayId;
        uint8 isWmo;
        char buff[500];
        while (!feof(model_list))
        {
            // 读取显示ID
            if (fread(&displayId, sizeof(uint32), 1, model_list) != 1)
                if (feof(model_list))   // EOF标志只有在读取失败后才会设置
                {
                    break;
                }

            // 读取是否为WMO、名称长度和名称
            if (fread(&isWmo, sizeof(uint8), 1, model_list) != 1
                    || fread(&name_length, sizeof(uint32), 1, model_list) != 1
                    || name_length >= sizeof(buff)
                    || fread(&buff, sizeof(char), name_length, model_list) != name_length)
            {
                std::cout << "\nFile 'temp_gameobject_models' seems to be corrupted" << std::endl;
                break;
            }

            // 构建模型名称
            std::string model_name(buff, name_length);

            WorldModel_Raw raw_model;
            // 读取原始模型文件
            if (!raw_model.Read((iSrcDir + "/" + model_name).c_str()))
            {
                continue;
            }

            // 插入已生成的模型文件名
            spawnedModelFiles.insert(model_name);
            AABox bounds;
            bool boundEmpty = true;
            for (uint32 g = 0; g < raw_model.groupsArray.size(); ++g)
            {
                std::vector<Vector3>& vertices = raw_model.groupsArray[g].vertexArray;

                // 获取顶点数量
                uint32 nvectors = vertices.size();
                for (uint32 i = 0; i < nvectors; ++i)
                {
                    Vector3& v = vertices[i];
                    if (boundEmpty)
                    {
                        bounds = AABox(v, v), boundEmpty = false;
                    }
                    else
                    {
                        // 合并边界
                        bounds.merge(v);
                    }
                }
            }

            // 写入显示ID
            fwrite(&displayId, sizeof(uint32), 1, model_list_copy);
            // 写入是否为WMO
            fwrite(&isWmo, sizeof(uint8), 1, model_list_copy);
            // 写入名称长度
            fwrite(&name_length, sizeof(uint32), 1, model_list_copy);
            // 写入名称
            fwrite(&buff, sizeof(char), name_length, model_list_copy);
            // 写入边界低点
            fwrite(&bounds.low(), sizeof(Vector3), 1, model_list_copy);
            // 写入边界高点
            fwrite(&bounds.high(), sizeof(Vector3), 1, model_list_copy);
        }

        // 关闭文件
        fclose(model_list);
        fclose(model_list_copy);
    }
    // 临时使用宏来简化读取/检查代码(读取失败时关闭文件并返回)
#define READ_OR_RETURN(V, S) if (fread((V), (S), 1, rf) != 1) { \
                                        fclose(rf); printf("readfail, op = %i\n", readOperation); return(false); }
#define READ_OR_RETURN_WITH_DELETE(V, S) if (fread((V), (S), 1, rf) != 1) { \
                                        fclose(rf); printf("readfail, op = %i\n", readOperation); delete[] V; return(false); };
#define CMP_OR_RETURN(V, S)  if (strcmp((V), (S)) != 0)        { \
                                        fclose(rf); printf("cmpfail, %s!=%s\n", V, S);return(false); }

    // 读取组模型原始数据
    bool GroupModel_Raw::Read(FILE* rf)
    {
        char blockId[5];
        blockId[4] = 0;
        int blocksize;
        int readOperation = 0;

        // 读取MOGP标志
        READ_OR_RETURN(&mogpflags, sizeof(uint32));
        // 读取组WMO ID
        READ_OR_RETURN(&GroupWMOID, sizeof(uint32));

        Vector3 vec1, vec2;
        // 读取第一个向量
        READ_OR_RETURN(&vec1, sizeof(Vector3));

        // 读取第二个向量
        READ_OR_RETURN(&vec2, sizeof(Vector3));
        // 设置边界
        bounds.set(vec1, vec2);

        // 读取液体标志
        READ_OR_RETURN(&liquidflags, sizeof(uint32));

        // 这个会被使用吗？它到底有什么用？
        uint32 branches;
        // 读取块ID
        READ_OR_RETURN(&blockId, 4);
        // 检查块ID是否为 "GRP "
        CMP_OR_RETURN(blockId, "GRP ");
        // 读取块大小
        READ_OR_RETURN(&blocksize, sizeof(int));
        // 读取分支数量
        READ_OR_RETURN(&branches, sizeof(uint32));
        for (uint32 b = 0; b < branches; ++b)
        {
            uint32 indexes;
            // 读取每个分支的索引(目前未使用)
            READ_OR_RETURN(&indexes, sizeof(uint32));
        }

        // ---- 读取索引
        // 读取块ID
        READ_OR_RETURN(&blockId, 4);
        // 检查块ID是否为 "INDX"
        CMP_OR_RETURN(blockId, "INDX");
        // 读取块大小
        READ_OR_RETURN(&blocksize, sizeof(int));
        uint32 nindexes;
        // 读取索引数量
        READ_OR_RETURN(&nindexes, sizeof(uint32));
        if (nindexes > 0)
        {
            // 分配索引数组
            uint16* indexarray = new uint16[nindexes];
            // 读取索引数据
            READ_OR_RETURN_WITH_DELETE(indexarray, nindexes * sizeof(uint16));
            // 预留三角形空间
            triangles.reserve(nindexes / 3);
            for (uint32 i = 0; i < nindexes; i += 3)
            {
                // 创建三角形
                triangles.push_back(MeshTriangle(indexarray[i], indexarray[i + 1], indexarray[i + 2]));
            }

            // 释放索引数组
            delete[] indexarray;
        }

        // ---- 读取向量
        // 读取块ID
        READ_OR_RETURN(&blockId, 4);
        // 检查块ID是否为 "VERT"
        CMP_OR_RETURN(blockId, "VERT");
        // 读取块大小
        READ_OR_RETURN(&blocksize, sizeof(int));
        uint32 nvectors;
        // 读取向量数量
        READ_OR_RETURN(&nvectors, sizeof(uint32));

        if (nvectors > 0)
        {
            // 分配向量数组
            float* vectorarray = new float[nvectors * 3];
            // 读取向量数据
            READ_OR_RETURN_WITH_DELETE(vectorarray, nvectors * sizeof(float) * 3);
            for (uint32 i = 0; i < nvectors; ++i)
            {
                // 添加顶点
                vertexArray.push_back( Vector3(vectorarray + 3 * i));
            }

            // 释放向量数组
            delete[] vectorarray;
        }
        // ----- 读取液体数据
        liquid = nullptr;
        if (liquidflags & 3)
        {
            // 读取块ID
            READ_OR_RETURN(&blockId, 4);
            // 检查块ID是否为 "LIQU"
            CMP_OR_RETURN(blockId, "LIQU");
            // 读取块大小
            READ_OR_RETURN(&blocksize, sizeof(int));
            uint32 liquidType;
            // 读取液体类型
            READ_OR_RETURN(&liquidType, sizeof(uint32));
            if (liquidflags & 1)
            {
                WMOLiquidHeader hlq;
                // 读取WMO液体头
                READ_OR_RETURN(&hlq, sizeof(WMOLiquidHeader));
                // 创建WMO液体对象
                liquid = new WmoLiquid(hlq.xtiles, hlq.ytiles, Vector3(hlq.pos_x, hlq.pos_y, hlq.pos_z), liquidType);
                uint32 size = hlq.xverts * hlq.yverts;
                // 读取液体高度数据
                READ_OR_RETURN(liquid->GetHeightStorage(), size * sizeof(float));
                size = hlq.xtiles * hlq.ytiles;
                // 读取液体标志数据
                READ_OR_RETURN(liquid->GetFlagsStorage(), size);
            }
            else
            {
                // 创建空的WMO液体对象
                liquid = new WmoLiquid(0, 0, Vector3::zero(), liquidType);
                liquid->GetHeightStorage()[0] = bounds.high().z;
            }
        }

        return true;
    }

    // GroupModel_Raw 析构函数，释放液体数据
    GroupModel_Raw::~GroupModel_Raw()
    {
        delete liquid;
    }

    // 读取原始世界模型文件
    bool WorldModel_Raw::Read(const char* path)
    {
        // 打开原始模型文件
        FILE* rf = fopen(path, "rb");
        if (!rf)
        {
            printf("ERROR: Can't open raw model file: %s\n", path);
            return false;
        }

        char ident[9];
        ident[8] = '\0';
        int readOperation = 0;

        // 读取文件标识
        READ_OR_RETURN(&ident, 8);
        // 检查文件标识是否为 RAW_VMAP_MAGIC
        CMP_OR_RETURN(ident, RAW_VMAP_MAGIC);

        // 我们需要读取一个整数。这在导出期间需要，这里需要跳过它
        uint32 tempNVectors;
        // 读取临时向量数量
        READ_OR_RETURN(&tempNVectors, sizeof(tempNVectors));

        uint32 groups;
        // 读取模型组数量
        READ_OR_RETURN(&groups, sizeof(uint32));
        // 读取根WMO ID
        READ_OR_RETURN(&RootWMOID, sizeof(uint32));

        // 调整组数组大小
        groupsArray.resize(groups);
        bool succeed = true;
        for (uint32 g = 0; g < groups && succeed; ++g)
        {
            // 读取每个组模型数据
            succeed = groupsArray[g].Read(rf);
        }

        if (succeed) /// 如果函数有任何错误，rf 将在 Read 内部释放。
        {
            // 关闭文件
            fclose(rf);
        }
        return succeed;
    }

    // 取消临时使用的宏定义
#undef READ_OR_RETURN
#undef CMP_OR_RETURN
}
