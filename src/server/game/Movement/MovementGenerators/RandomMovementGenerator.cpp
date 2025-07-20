#include "RandomMovementGenerator.h"
#include "Creature.h"
#include "CreatureGroups.h"
#include "Map.h"
#include "MapMgr.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "ObjectAccessor.h"
#include "Spell.h"
#include "Util.h"
#include "World.h"

template<class T>
RandomMovementGenerator<T>::~RandomMovementGenerator() { }

template RandomMovementGenerator<Creature>::~RandomMovementGenerator();

template<>
void RandomMovementGenerator<Creature>::_setRandomLocation(Creature* creature)
{
    // 如果creature为空或处于非移动状态，直接返回
    if (!creature)
        return;

    if (creature->_moveState != MAP_OBJECT_CELL_MOVE_NONE)
        return;

    // 如果当前点没有可用目标点
    if (_validPointsVector[_currentPoint].empty())
    {
        // 如果当前点是初始位置点，无法移动，直接返回
        if (_currentPoint == RANDOM_POINTS_NUMBER)
            return;

        // 返回初始位置，并标记当前点为初始位置点
        _currentPoint = RANDOM_POINTS_NUMBER;
        _currDestPosition.Relocate(_initialPosition);
        creature->AddUnitState(UNIT_STATE_ROAMING_MOVE);

        // 初始化移动路径到初始位置
        Movement::MoveSplineInit init(creature);
        init.MoveTo(_currDestPosition.GetPositionX(), _currDestPosition.GetPositionY(), _currDestPosition.GetPositionZ());
        init.SetWalk(true);
        init.Launch();

        // 如果是编队领袖，通知编队成员移动
        if (creature->GetFormation() && creature->GetFormation()->GetLeader() == creature)
            creature->GetFormation()->LeaderMoveTo(_currDestPosition.GetPositionX(), _currDestPosition.GetPositionY(), _currDestPosition.GetPositionZ(), 0);

        return;
    }

    // 随机选择一个目标点
    uint8 random = urand(0, _validPointsVector[_currentPoint].size() - 1);
    std::vector<uint8>::iterator randomIter = _validPointsVector[_currentPoint].begin() + random;
    uint8 newPoint = *randomIter;
    uint16 pathIdx = uint16(_currentPoint * RANDOM_POINTS_NUMBER + newPoint);

    // 如果新点没有可用路径，移除该点并返回
    if (_validPointsVector[newPoint].empty())
    {
        _validPointsVector[_currentPoint].erase(randomIter);
        return;
    }

    Movement::PointsArray& finalPath = _preComputedPaths[pathIdx];

    // 如果路径未计算，进行路径计算
    if (finalPath.empty())
    {
        Map* map = creature->GetMap();
        float x = _destinationPoints[newPoint].x, y = _destinationPoints[newPoint].y, z = _destinationPoints[newPoint].z;

        // 检查坐标是否有效
        if (!Acore::IsValidMapCoord(x, y))
        {
            _validPointsVector[_currentPoint].erase(randomIter);
            _preComputedPaths.erase(pathIdx);
            return;
        }

        float ground = INVALID_HEIGHT;
        float levelZ = creature->GetMapWaterOrGroundLevel(x, y, z, &ground);
        float newZ = INVALID_HEIGHT;

        // 飞行生物处理
        if (creature->CanFly())
            newZ = std::max<float>(levelZ, z + rand_norm() * _wanderDistance / 2.0f);
        // 水下点处理
        else if (ground < levelZ)
        {
            if (!creature->CanEnterWater())
            {
                _validPointsVector[_currentPoint].erase(randomIter);
                _preComputedPaths.erase(pathIdx);
                return;
            }
            else
            {
                if (levelZ > INVALID_HEIGHT)
                    newZ = std::min<float>(levelZ - 2.0f, z + rand_norm() * _wanderDistance / 2.0f);
                newZ = std::max<float>(ground, newZ);
            }
        }
        // 地面点处理
        else
        {
            if (levelZ <= INVALID_HEIGHT || !creature->CanWalk())
            {
                _validPointsVector[_currentPoint].erase(randomIter);
                _preComputedPaths.erase(pathIdx);
                return;
            }
        }

        // 更新Z坐标
        creature->UpdateAllowedPositionZ(x, y, newZ);

        if (newZ > INVALID_HEIGHT)
        {
            // 如果目标点不在视线范围内，移除该路径
            if (!creature->IsWithinLOS(x, y, newZ))
            {
                _validPointsVector[_currentPoint].erase(randomIter);
                _preComputedPaths.erase(pathIdx);
                return;
            }

            // 设置两点直线路径
            finalPath.push_back(G3D::Vector3(creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ()));
            finalPath.push_back(G3D::Vector3(x, y, newZ));
        }
        else // 地面移动路径计算
        {
            if (!_pathGenerator)
                _pathGenerator = std::make_unique<PathGenerator>(creature);
            else
                _pathGenerator->Clear();

            // 计算路径
            bool result = _pathGenerator->CalculatePath(x, y, levelZ, false);
            if (result && !(_pathGenerator->GetPathType() & PATHFIND_NOPATH))
            {
                float pathLen = _pathGenerator->getPathLength();

                // 路径过长则移除
                if (pathLen * pathLen > creature->GetExactDistSq(x, y, levelZ) * MAX_PATH_LENGHT_FACTOR * MAX_PATH_LENGHT_FACTOR)
                {
                    _validPointsVector[_currentPoint].erase(randomIter);
                    _preComputedPaths.erase(pathIdx);
                    return;
                }

                // 获取路径并检查路径质量
                finalPath = _pathGenerator->GetPath();
                Movement::PointsArray::iterator itr = finalPath.begin();
                Movement::PointsArray::iterator itrNext = finalPath.begin() + 1;
                float zDiff, distDiff;

                for (; itrNext != finalPath.end(); ++itr, ++itrNext)
                {
                    distDiff = std::sqrt(((*itr).x - (*itrNext).x) * ((*itr).x - (*itrNext).x) + ((*itr).y - (*itrNext).y) * ((*itr).y - (*itrNext).y));
                    zDiff = std::fabs((*itr).z - (*itrNext).z);

                    // 高度差过大或坡度太陡，移除路径
                    if (zDiff > 2.0f ||
                            (G3D::fuzzyNe(zDiff, 0.0f) && distDiff / zDiff < 2.15f)) // ~25度
                    {
                        _validPointsVector[_currentPoint].erase(randomIter);
                        _preComputedPaths.erase(pathIdx);
                        return;
                    }

                    // 路径不在视线范围内，移除路径
                    if (!map->isInLineOfSight((*itr).x, (*itr).y, (*itr).z + 2.f, (*itrNext).x, (*itrNext).y, (*itrNext).z + 2.f, creature->GetPhaseMask(),
                        LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags::Nothing))
                    {
                        _validPointsVector[_currentPoint].erase(randomIter);
                        _preComputedPaths.erase(pathIdx);
                        return;
                    }
                }

                // 路径无效，移除路径
                if (finalPath.size() < 2)
                {
                    _validPointsVector[_currentPoint].erase(randomIter);
                    _preComputedPaths.erase(pathIdx);
                    return;
                }
            }
            else
            {
                _validPointsVector[_currentPoint].erase(randomIter);
                _preComputedPaths.erase(pathIdx);
                return;
            }
        }
    }

    // 更新当前点和目标位置
    _currentPoint = newPoint;
    G3D::Vector3& finalPoint = finalPath[finalPath.size() - 1];
    _currDestPosition.Relocate(finalPoint.x, finalPoint.y, finalPoint.z);

    creature->AddUnitState(UNIT_STATE_ROAMING_MOVE);
    bool walk = true;

    // 根据随机移动类型设置行走/奔跑状态
    switch (creature->GetMovementTemplate().GetRandom())
    {
    case CreatureRandomMovementType::CanRun:
        walk = creature->IsWalking();
        break;
    case CreatureRandomMovementType::AlwaysRun:
        walk = false;
        break;
    default:
        break;
    }

    // 初始化路径移动
    Movement::MoveSplineInit init(creature);
    init.MovebyPath(finalPath);
    init.SetWalk(walk);
    init.Launch();

    // 更新移动次数并可能重置定时器
    ++_moveCount;
    if (roll_chance_i((int32) _moveCount * 25 + 10))
    {
        _moveCount = 0;
        _nextMoveTime.Reset(urand(4000, 8000));
    }

    // 如果配置不缓存路径，清除缓存
    if (sWorld->getBoolConfig(CONFIG_DONT_CACHE_RANDOM_MOVEMENT_PATHS))
        _preComputedPaths.erase(pathIdx);

    // 如果是编队领袖，通知编队成员移动
    if (creature->GetFormation() && creature->GetFormation()->GetLeader() == creature)
        creature->GetFormation()->LeaderMoveTo(finalPoint.x, finalPoint.y, finalPoint.z, 0);
}

template<>
void RandomMovementGenerator<Creature>::DoInitialize(Creature* creature)
{
    // 如果生物死亡，直接返回
    if (!creature->IsAlive())
        return;

    // 初始化游荡距离
    if (!_wanderDistance)
        _wanderDistance = creature->GetWanderDistance();

    // 设置下一次移动时间
    _nextMoveTime.Reset(creature->GetSpawnId() && creature->GetWanderDistance() == _wanderDistance ? urand(1, 5000) : 0);

    // 确保最小游荡距离
    _wanderDistance = std::max((creature->GetWanderDistance() == _wanderDistance && creature->GetInstanceId() == 0) ? (creature->CanFly() ? MIN_WANDER_DISTANCE_AIR : MIN_WANDER_DISTANCE_GROUND) : 0.0f, _wanderDistance);

    // 如果初始位置未设置，生成随机目标点
    if (G3D::fuzzyEq(_initialPosition.GetExactDist2d(0.0f, 0.0f), 0.0f))
    {
        _initialPosition.Relocate(creature);
        _destinationPoints.clear();
        for (uint8 i = 0; i < RANDOM_POINTS_NUMBER; ++i)
        {
            float angle = (M_PI * 2.0f / (float)RANDOM_POINTS_NUMBER) * i;
            float factor = 0.5f + rand_norm() * 0.5f;
            _destinationPoints.push_back(G3D::Vector3(_initialPosition.GetPositionX() + _wanderDistance * cos(angle)*factor, _initialPosition.GetPositionY() + _wanderDistance * std::sin(angle)*factor, _initialPosition.GetPositionZ()));
        }
    }

    // 添加单位移动状态
    creature->AddUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
}

template<>
void RandomMovementGenerator<Creature>::DoReset(Creature* creature)
{
    // 重置时调用初始化
    DoInitialize(creature);
}

template<>
void RandomMovementGenerator<Creature>::DoFinalize(Creature* creature)
{
    // 清除单位移动状态
    creature->ClearUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
    creature->SetWalk(false);
}

template<>
bool RandomMovementGenerator<Creature>::DoUpdate(Creature* creature, const uint32 diff)
{
    // 如果单位禁止移动或正在施法，停止移动并重置定时器
    if (creature->HasUnitState(UNIT_STATE_NOT_MOVE) || creature->IsMovementPreventedByCasting())
    {
        _nextMoveTime.Reset(0);  // 重置定时器
        creature->StopMoving();
        return true;
    }

    // 如果单位被标记为禁止移动，仅清除移动状态
    if (creature->HasUnitFlag(UNIT_FLAG_DISABLE_MOVE))
    {
        _nextMoveTime.Reset(0);  // 重置定时器
        creature->ClearUnitState(UNIT_STATE_ROAMING_MOVE);
        return true;
    }

    // 如果路径已完成，更新定时器并尝试随机移动
    if (creature->movespline->Finalized())
    {
        _nextMoveTime.Update(diff);
        if (_nextMoveTime.Passed())
            _setRandomLocation(creature);
    }
    return true;
}

template<>
bool RandomMovementGenerator<Creature>::GetResetPosition(float& x, float& y, float& z)
{
    // 获取当前位置或初始位置作为重置位置
    if (_currentPoint < RANDOM_POINTS_NUMBER)
        _currDestPosition.GetPosition(x, y, z);
    else if (G3D::fuzzyNe(_initialPosition.GetExactDist2d(0.0f, 0.0f), 0.0f)) // 如果初始位置不是原点
        _initialPosition.GetPosition(x, y, z);
    else
        return false;

    return true;
}
