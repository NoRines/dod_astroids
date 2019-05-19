#include <iostream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <bitset>
#include <array>
#include <memory>
#include <cassert>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <unordered_map>



#include "renderer.hpp"

// Window Constants
constexpr int windowWidth = 640;
constexpr int windowHeight = 480;


// Define the keymap and related function
using KeyMap = std::unordered_map<unsigned int, bool>;

bool isKeyDown(const KeyMap& keymap, unsigned int key)
{
    auto itt = keymap.find(key);
    if(itt == keymap.end())
        return false;
    return itt->second;
}


template<typename resolution, int targetFps>
int limitFps()
{
    using ClockType = std::conditional<
        std::chrono::high_resolution_clock::is_steady,
        std::chrono::high_resolution_clock,
        std::chrono::steady_clock>::type;

    static auto startTime = ClockType::now();
    static auto endTime = ClockType::now();

    // Calculate the target frame time based on the target fps
    constexpr std::chrono::seconds sec(1);
    constexpr int targetFrameTime = resolution(sec).count() / targetFps;

    // Sample first time point
    startTime = ClockType::now();

    // Get the time spent not sleeping this frame
    int workTime = std::chrono::duration_cast<resolution>(startTime - endTime).count();

    // Sleep the process if needed
    if(workTime < targetFrameTime)
        std::this_thread::sleep_for(resolution(targetFrameTime - workTime));

    // Sample second time point
    endTime = ClockType::now();

    // Get the time spent sleeping this frame
    int sleepTime = std::chrono::duration_cast<resolution>(endTime - startTime).count();

    // The total number of ticks this frame is: sleep time + non sleep time
    int ticks = sleepTime + workTime;
    return ticks;
}

enum class ShapeDef
{
    NONE = 0,
    SHIP = 1,
    FLAME = 2,
    FIRST_ASTROID = 3,
    LAST_ASTROID = 7
        // TODO (FYLL MED ALLA SHAPE SAKER...
};

static const std::vector<std::vector<float>> shapeDefs = {
    {}, // NONE
    {6,0, -3,-3, -1,0, -3,3, 6,0}, // SHIP
    {-1,0, -4,1, -6,0, -4,-1, -1,0}, // FLAME
    {-4,-2,-2,-4,0,-2,2,-4,4,-2,3,0,4,2,1,4,-2,4,-4,2,-4,-2}, // ASTROIDS
    {1,4, 3,3, 1,1, 4,-1, 2,-4, -2,-4, -4,-1, -4,2, -1,3, 1,4},
    {-2,0,-4,-1,-1,-4,2,-4,4,-1,4,1,2,4,0,4,0,1,-2,4,-4,1,-2,0},
    {-1,-2,-2,-4,1,-4,4,-2,4,-1,1,0,4,2,2,4,1,3,-2,4,-4,1,-4,-2,-1,-2},
    {-4,-2,-2,-4,2,-4,4,-2,4,2,2,4,-2,4,-4,2,-4,-2}
};
//
//        LETTERS: [
//		[0,6,0,2,2,0,4,2,4,4,0,4,4,4,4,6],                 //A
//		[0,3,0,6,2,6,3,5,3,4,2,3,0,3,0,0,2,0,3,1,3,2,2,3], //B
//		[4,0,0,0,0,6,4,6],                                 //C
//		[0,0,0,6,2,6,4,4,4,2,2,0,0,0],                     //D
//		[4,0,0,0,0,3,3,3,0,3,0,6,4,6],                     //E
//		[4,0,0,0,0,3,3,3,0,3,0,6],                         //F
//		[4,2,4,0,0,0,0,6,4,6,4,4,2,4],                     //G
//		[0,0,0,6,0,3,4,3,4,0,4,6],                         //H
//		[0,0,4,0,2,0,2,6,4,6,0,6],                         //I
//		[4,0,4,6,2,6,0,4],                                 //J
//		[3,0,0,3,0,0,0,6,0,3,3,6],                         //K
//		[0,0,0,6,4,6],                                     //L
//		[0,6,0,0,2,2,4,0,4,6],                             //M
//		[0,6,0,0,4,6,4,0],                                 //N
//		[0,0,4,0,4,6,0,6,0,0],                             //O
//		[0,6,0,0,4,0,4,3,0,3],                             //P
//		[0,0,0,6,2,6,3,5,4,6,2,4,3,5,4,4,4,0,0,0],         //Q
//		[0,6,0,0,4,0,4,3,0,3,1,3,4,6],                     //R
//		[4,0,0,0,0,3,4,3,4,6,0,6],                         //S
//		[0,0,4,0,2,0,2,6],                                 //T
//		[0,0,0,6,4,6,4,0],                                 //U
//		[0,0,2,6,4,0],                                     //V
//		[0,0,0,6,2,4,4,6,4,0],                             //W
//		[0,0,4,6,2,3,4,0,0,6],                             //X
//		[0,0,2,2,4,0,2,2,2,6],                             //Y
//		[0,0,4,0,0,6,4,6]                                  //Z
//	],
//
//	NUMBERS: [
//		[0,0,0,6,4,6,4,0,0,0],                             //0
//		[2,0,2,6],                                         //1
//		[0,0,4,0,4,3,0,3,0,6,4,6],                         //2
//		[0,0,4,0,4,3,0,3,4,3,4,6,0,6],                     //3
//		[0,0,0,3,4,3,4,0,4,6],                             //4
//		[4,0,0,0,0,3,4,3,4,6,0,6],                         //5
//		[0,0,0,6,4,6,4,3,0,3],                             //6
//		[0,0,4,0,4,6],                                     //7
//		[0,3,4,3,4,6,0,6,0,0,4,0,4,3],                     //8
//		[4,3,0,3,0,0,4,0,4,6],                             //9
//        ]
//

// Shape drawing pipeline

struct ShapeDrawInfo
{
    float scaleFact;
    float dirValue;
    float x, y;
    uint32_t color;

    std::size_t fromI, toI;
};

void transformShapes(std::vector<float>& shapeData, const std::vector<ShapeDrawInfo>& drawInfo)
{
    for(auto& info : drawInfo)
    {
        float s = std::sin(info.dirValue);
        float c = std::cos(info.dirValue);

        for(int i = info.fromI; i < info.toI; i+=2)
        {
            // Scale
            shapeData[i] *= info.scaleFact;
            shapeData[i+1] *= info.scaleFact;

            // Rotate
            float x = shapeData[i] * c - shapeData[i+1] * s;
            float y = shapeData[i] * s + shapeData[i+1] * c;
            shapeData[i] = x;
            shapeData[i+1] = y;

            // Offset
            shapeData[i] += info.x;
            shapeData[i+1] += info.y;
        }
    }
}

void renderShapes(const std::vector<float>& shapeData, const std::vector<ShapeDrawInfo>& drawInfo)
{
    for(auto& info : drawInfo)
    {
        float lastX = shapeData[info.fromI];
        float lastY = shapeData[info.fromI+1];
        for(int i = info.fromI + 2; i < info.toI; i+=2)
        {
            renderer::drawLine(lastX, lastY, shapeData[i], shapeData[i+1], info.color);
            lastX = shapeData[i];
            lastY = shapeData[i+1];
        }
    }
}



// Entity things

using Entity = std::size_t;

constexpr std::size_t maxComponents = 32;

struct Group
{
    std::vector<Entity> entities;
    bool set;
};

using ComponentBitset = std::bitset<maxComponents>;
using GroupMap = std::unordered_map<ComponentBitset, Group>;

inline std::size_t makeNewComponentId()
{
    static std::size_t id = 0;
    return id++;
}

template<typename T>
inline std::size_t getUniqueComponentId()
{
    static std::size_t id = makeNewComponentId();
    return id;
}


// COMPONENTS
struct CPosition
{
    float x, y;
};

struct CVelocity
{
    float xVel, yVel;
};

struct CScale
{
    float scale;
};

struct CRotation
{
    float rotationSpeed;
    float dir;
};

struct CShape
{
    std::size_t shape;
    uint32_t color;
    std::size_t fromI;
    std::size_t toI;
};

struct CControlMove
{
    float accelFactor;
    float rotationSpeed;
};

struct CControlInvisible
{
    bool isVisible;
};

struct CBullet
{
    float xLast;
    float yLast;
    uint32_t color;
};

struct CLifeTime
{
    float time;
};

struct CControlFire
{
    bool fired;
};


// ENTITY
struct EntityManager
{
    std::vector<CPosition> posList;
    std::vector<CVelocity> velocityList;
    std::vector<CScale> scaleList;
    std::vector<CRotation> rotationList;
    std::vector<CShape> shapeList;
    std::vector<CControlMove> moveList;
    std::vector<CControlInvisible> invisibleList;
    std::vector<CBullet> bulletList;
    std::vector<CLifeTime> lifeTimeList;
    std::vector<CControlFire> canFireList;

    std::vector<ComponentBitset> componentBitsets;
    std::vector<bool> markedForRemoval;

    GroupMap groupMap;
};

Entity addEntity(EntityManager& manager)
{
    manager.posList.push_back({});
    manager.velocityList.push_back({});
    manager.scaleList.push_back({});
    manager.rotationList.push_back({});
    manager.shapeList.push_back({});
    manager.moveList.push_back({});
    manager.invisibleList.push_back({});
    manager.bulletList.push_back({});
    manager.lifeTimeList.push_back({});
    manager.canFireList.push_back({});

    manager.componentBitsets.push_back({});
    manager.markedForRemoval.push_back(false);
    
    // For now clear all cached entites. Mayby TODO add entity to the right group when created
    for(auto& it : manager.groupMap)
    {
        it.second.entities.clear();
        it.second.set = false;
    }

    return manager.componentBitsets.size() - 1;
};

template<typename T>
void removeEntityFromVector(std::vector<T>& v, Entity e)
{
    std::swap(v[e], v.back());
    v.pop_back();
}

void delEntity(EntityManager& manager, Entity e)
{
    removeEntityFromVector(manager.posList, e);
    removeEntityFromVector(manager.velocityList, e);
    removeEntityFromVector(manager.scaleList, e);
    removeEntityFromVector(manager.rotationList, e);
    removeEntityFromVector(manager.shapeList, e);
    removeEntityFromVector(manager.moveList, e);
    removeEntityFromVector(manager.invisibleList, e);
    removeEntityFromVector(manager.bulletList, e);
    removeEntityFromVector(manager.lifeTimeList, e);
    removeEntityFromVector(manager.canFireList, e);

    removeEntityFromVector(manager.componentBitsets, e);
    removeEntityFromVector(manager.markedForRemoval, e);
}

void removeEntities(EntityManager& manager)
{
    bool oneEntityRemoved = false;
    for(Entity e = 0; e < manager.markedForRemoval.size(); e++)
    {
        if(manager.markedForRemoval[e])
        {
            oneEntityRemoved = true;
            delEntity(manager, e);
        }
    }


    // Clear all cached data since we cannot know if the data is correct
    if(oneEntityRemoved)
        for(auto& it : manager.groupMap)
        {
            it.second.entities.clear();
            it.second.set = false;
        }
}

const std::vector<Entity>& getEntitesForSystem(EntityManager& manager, ComponentBitset bitset)
{
    auto it = manager.groupMap.find(bitset);

    if(it != manager.groupMap.end())
    {
        if(it->second.set)
            return it->second.entities;

        for(Entity e = 0; e < manager.componentBitsets.size(); e++)
            if((manager.componentBitsets[e] & bitset) == bitset)
                it->second.entities.push_back(e);

        it->second.set = true;

        return it->second.entities;
    }

    std::vector<Entity> entites;
    entites.reserve(manager.componentBitsets.size());

    for(Entity e = 0; e < manager.componentBitsets.size(); e++)
        if((manager.componentBitsets[e] & bitset) == bitset)
            entites.push_back(e);

    manager.groupMap[bitset] = {entites, true};

    return manager.groupMap[bitset].entities;
}

void addCPosition(EntityManager& manager, Entity e, const CPosition& pos)
{
    manager.posList[e] = pos;
    manager.componentBitsets[e][getUniqueComponentId<CPosition>()] = true;
}

void addCVelocity(EntityManager& manager, Entity e, const CVelocity& vel)
{
    manager.velocityList[e] = vel;
    manager.componentBitsets[e][getUniqueComponentId<CVelocity>()] = true;
}

void addCScale(EntityManager& manager, Entity e, const CScale& scale)
{
    manager.scaleList[e] = scale;
    manager.componentBitsets[e][getUniqueComponentId<CScale>()] = true;
}

void addCRotation(EntityManager& manager, Entity e, const CRotation& rot)
{
    manager.rotationList[e] = rot;
    manager.componentBitsets[e][getUniqueComponentId<CRotation>()] = true;
}

void addCShape(EntityManager& manager, Entity e, const CShape& shape)
{
    manager.shapeList[e] = shape;
    manager.componentBitsets[e][getUniqueComponentId<CShape>()] = true;
}

void addCControlMove(EntityManager& manager, Entity e, const CControlMove& controlMove)
{
    manager.moveList[e] = controlMove;
    manager.componentBitsets[e][getUniqueComponentId<CControlMove>()] = true;
}

void addCInvisible(EntityManager& manager, Entity e, const CControlInvisible& invisible)
{
    manager.invisibleList[e] = invisible;
    manager.componentBitsets[e][getUniqueComponentId<CControlInvisible>()] = true;
}

void addCBullet(EntityManager& manager, Entity e, const CBullet& bullet)
{
    manager.bulletList[e] = bullet;
    manager.componentBitsets[e][getUniqueComponentId<CBullet>()] = true;
}

void addCLifeTime(EntityManager& manager, Entity e, const CLifeTime& lifeTime)
{
    manager.lifeTimeList[e] = lifeTime;
    manager.componentBitsets[e][getUniqueComponentId<CLifeTime>()] = true;
}

void addCCanFire(EntityManager& manager, Entity e, const CControlFire& canfire)
{
    manager.canFireList[e] = canfire;
    manager.componentBitsets[e][getUniqueComponentId<CControlFire>()] = true;
}

// Entity behaviour
void createAstroid(EntityManager& manager, std::mt19937& randGen, float scale, float xPos = 0.0f, float yPos = 0.0f);
void createShip(EntityManager& manager);
void createBullet(EntityManager& manager, float xPos, float yPos, float dir);

ComponentBitset getSaveLastPosBitset()
{
    ComponentBitset saveLastPosBitset;
    saveLastPosBitset[getUniqueComponentId<CPosition>()] = true;
    saveLastPosBitset[getUniqueComponentId<CBullet>()] = true;
    return saveLastPosBitset;
}

void saveLastPos(const std::vector<Entity>& entities, EntityManager& manager)
{
    auto& positions = manager.posList;
    auto& bullets = manager.bulletList;

    for(auto& e : entities)
    {
        bullets[e].xLast = positions[e].x;
        bullets[e].yLast = positions[e].y;
    }
}

ComponentBitset getMoveEntitiesBitset()
{
    ComponentBitset moveBitset;
    moveBitset[getUniqueComponentId<CPosition>()] = true;
    moveBitset[getUniqueComponentId<CVelocity>()] = true;
    return moveBitset;
}

void moveEntities(const std::vector<Entity>& entities, EntityManager& manager, float ft)
{
    auto& positions = manager.posList;
    auto& velocities = manager.velocityList;

    for(auto& e : entities)
    {
        positions[e].x += velocities[e].xVel * ft;
        positions[e].y += velocities[e].yVel * ft;

        if(positions[e].x > windowWidth)
            positions[e].x -= windowWidth;
        else if(positions[e].x < 0.0f)
            positions[e].x += windowWidth;
        
        if(positions[e].y > windowHeight)
            positions[e].y -= windowHeight;
        else if(positions[e].y < 0.0f)
            positions[e].y += windowHeight;
    }
}

ComponentBitset getRotateEntitiesBitset()
{
    ComponentBitset rotateBitset;
    rotateBitset[getUniqueComponentId<CRotation>()] = true;
    return rotateBitset;
}

void rotateEntites(const std::vector<Entity>& entities, EntityManager& manager, float ft)
{
    auto& rotations = manager.rotationList;

    for(auto& e : entities)
    {
        rotations[e].dir += rotations[e].rotationSpeed * ft;

        while(rotations[e].dir > M_PI)
            rotations[e].dir -= 2.0f * M_PI;
        while(rotations[e].dir < -M_PI)
            rotations[e].dir += 2.0f * M_PI;
    }
}

ComponentBitset getControllMoveEntitiesBitset()
{
    ComponentBitset controllerBitset;
    controllerBitset[getUniqueComponentId<CPosition>()] = true;
    controllerBitset[getUniqueComponentId<CVelocity>()] = true;
    controllerBitset[getUniqueComponentId<CRotation>()] = true;
    controllerBitset[getUniqueComponentId<CControlMove>()] = true;
    return controllerBitset;
}

void controllEnities(const std::vector<Entity>& entities, EntityManager& manager, const KeyMap& keymap, float ft)
{
    auto& positions = manager.posList;
    auto& velocities = manager.velocityList;
    auto& rotations = manager.rotationList;
    auto& controlMoves = manager.moveList;

    for(auto& e : entities)
    {
        rotations[e].rotationSpeed = 0.0f;

        if(isKeyDown(keymap, SDLK_LEFT))
            rotations[e].rotationSpeed -= controlMoves[e].rotationSpeed;
        else if(isKeyDown(keymap, SDLK_RIGHT))
            rotations[e].rotationSpeed += controlMoves[e].rotationSpeed;

        if(isKeyDown(keymap, SDLK_UP))
        {
            velocities[e].xVel += std::cos(rotations[e].dir) * ft * controlMoves[e].accelFactor;
            velocities[e].yVel += std::sin(rotations[e].dir) * ft * controlMoves[e].accelFactor;
        }

        velocities[e].xVel *= 0.99f;
        velocities[e].yVel *= 0.99f;
    }
}

ComponentBitset getShowInvisibleEntitiesBitset()
{
    ComponentBitset invisibleControllBitset;
    invisibleControllBitset[getUniqueComponentId<CControlInvisible>()] = true;
    invisibleControllBitset[getUniqueComponentId<CShape>()] = true;
    return invisibleControllBitset;
}

void showInvisibleEntities(const std::vector<Entity>& entities, EntityManager& manager, const KeyMap& keymap)
{
    auto& invisibles = manager.invisibleList;
    auto& shapes = manager.shapeList;

    auto& componentBitsets = manager.componentBitsets;

    for(auto& e : entities)
    {
        if(isKeyDown(keymap, SDLK_UP))
            invisibles[e].isVisible = true;
        else
            invisibles[e].isVisible = false;

        if(!invisibles[e].isVisible)
            shapes[e].shape = (std::size_t)ShapeDef::NONE;
        else
            shapes[e].shape = (std::size_t)ShapeDef::FLAME;
    }
}

ComponentBitset getLifeTimeEntitiesBitset()
{
    ComponentBitset lifeTimeBitset;
    lifeTimeBitset[getUniqueComponentId<CLifeTime>()] = true;
    return lifeTimeBitset;
}

void lifeTimeEntities(const std::vector<Entity>& entities, EntityManager& manager, float ft)
{
    auto& lifeTimes = manager.lifeTimeList;
    auto& markedForRemoval = manager.markedForRemoval;

    for(auto& e : entities)
    {
        lifeTimes[e].time -= ft;

        if(lifeTimes[e].time < 0.0f)
            markedForRemoval[e] = true;
    }
}

ComponentBitset getFireingEntitiesBitset()
{
    ComponentBitset canFireBitset;
    canFireBitset[getUniqueComponentId<CControlFire>()] = true;
    canFireBitset[getUniqueComponentId<CPosition>()] = true;
    canFireBitset[getUniqueComponentId<CRotation>()] = true;
    canFireBitset[getUniqueComponentId<CScale>()] = true;
    return canFireBitset;
}

void fireingEntities(const std::vector<Entity>& entities, EntityManager& manager, const KeyMap& keymap)
{
    auto& canFires = manager.canFireList;
    auto& positions = manager.posList;
    auto& rotations = manager.rotationList;
    auto& scales = manager.scaleList;

    for(auto& e : entities)
    {
        if(isKeyDown(keymap, SDLK_SPACE) && !canFires[e].fired)
        {
            canFires[e].fired = true;
            float startX = positions[e].x + std::cos(rotations[e].dir) * scales[e].scale * 6.0f;
            float startY = positions[e].y + std::sin(rotations[e].dir) * scales[e].scale * 6.0f;
            createBullet(manager, startX, startY, rotations[e].dir);
        }
        else if(!isKeyDown(keymap, SDLK_SPACE) && canFires[e].fired)
            canFires[e].fired = false;
    }
}

ComponentBitset getMakeShapeDataFromEntitiesBitset()
{
    ComponentBitset makeDataFromEntitiesBitset;
    makeDataFromEntitiesBitset[getUniqueComponentId<CPosition>()] = true;
    makeDataFromEntitiesBitset[getUniqueComponentId<CScale>()] = true;
    makeDataFromEntitiesBitset[getUniqueComponentId<CRotation>()] = true;
    makeDataFromEntitiesBitset[getUniqueComponentId<CShape>()] = true;
    return makeDataFromEntitiesBitset;
}

void makeShapeDataFromEntities(const std::vector<Entity>& entities, EntityManager& manager, std::vector<float>& shapeData, std::vector<ShapeDrawInfo>& drawInfo)
{
    auto& positions = manager.posList;
    auto& scales = manager.scaleList;
    auto& rotations = manager.rotationList;
    auto& shapes = manager.shapeList;

    for(auto& e : entities)
    {
        std::size_t shapeIndex = (std::size_t)shapes[e].shape;
        auto& shapeDef = shapeDefs[shapeIndex];

        shapes[e].fromI = shapeData.size();
        shapes[e].toI = shapeData.size() + shapeDef.size();

        drawInfo.push_back({
                scales[e].scale, rotations[e].dir, positions[e].x, positions[e].y,
                shapes[e].color, shapes[e].fromI, shapes[e].toI});

        for(auto& v : shapeDefs[shapeIndex])
            shapeData.emplace_back(v);
    }
}

ComponentBitset getAddBulletsToShapeDataBitset()
{
    ComponentBitset addBulletToShapeDataBitset;
    addBulletToShapeDataBitset[getUniqueComponentId<CPosition>()] = true;
    addBulletToShapeDataBitset[getUniqueComponentId<CBullet>()] = true;
    return addBulletToShapeDataBitset;
}

void addBulletsToShapeData(const std::vector<Entity>& entities, EntityManager& manager, std::vector<float>& shapeData, std::vector<ShapeDrawInfo>& drawInfo)
{
    auto& positions = manager.posList;
    auto& bullets = manager.bulletList;

    for(auto& e : entities)
    {
        drawInfo.push_back({
                1.0f, 0.0f, positions[e].x, positions[e].y,
                bullets[e].color, shapeData.size(), shapeData.size() + 4});

        if(std::abs(bullets[e].xLast - positions[e].x) < windowWidth / 2.0f &&
                std::abs(bullets[e].yLast - positions[e].y) < windowHeight / 2.0f )
        {
            shapeData.emplace_back(bullets[e].xLast);
            shapeData.emplace_back(bullets[e].yLast);
            shapeData.emplace_back(positions[e].x);
            shapeData.emplace_back(positions[e].y);
        }
    }
}

// CREATE ENTITES
void createAstroid(EntityManager& manager, std::mt19937& randGen, float scale, float xPos, float yPos)
{
    std::uniform_real_distribution<float> speedDist(50.0f, 150.0f);
    std::uniform_real_distribution<float> dirDist(0.0f, M_PI * 2.0f);
    std::uniform_real_distribution<float> rotDist(-3.0f, 3.0f);
    std::uniform_int_distribution<int> shapeDist((int)ShapeDef::FIRST_ASTROID, (int)ShapeDef::LAST_ASTROID);
    std::uniform_real_distribution<float> posDist(-1.0f, 1.0f);

    float velocity = speedDist(randGen);
    float dir = dirDist(randGen);
    float rotSpeed = rotDist(randGen);
    int astroidId = shapeDist(randGen);
    float posFactor = posDist(randGen);

    if(xPos == 0.0f && yPos == 0.0f)
    {
        xPos = posFactor >= 0.0f ? posFactor * windowWidth : 0.0f;
        yPos = posFactor < 0.0f ? -posFactor * windowHeight : 0.0f;
    }

    Entity astroid = addEntity(manager);

    addCPosition(manager, astroid, {xPos, yPos});
    addCVelocity(manager, astroid, {std::cos(dir) * velocity, std::sin(dir) * velocity});
    addCScale(manager, astroid, {scale});
    addCRotation(manager, astroid, {rotSpeed, dir});
    addCShape(manager, astroid, {(std::size_t)astroidId, 0xFFFFFFFF});
}

void createShip(EntityManager& manager)
{
    constexpr float xStart = windowWidth / 2.0f, yStart = windowHeight / 2.0f;
    constexpr float accelFactor = 600.0f, rotateFactor = 5.0f;
    constexpr float scaleFactor = 3.0f;

    Entity flame = addEntity(manager);

    addCPosition(manager, flame, {xStart, yStart});
    addCVelocity(manager, flame, {0.0f, 0.0f});
    addCScale(manager, flame, {scaleFactor});
    addCRotation(manager, flame, {0.0f, -M_PI/2.0f});
    addCShape(manager, flame, {(std::size_t)ShapeDef::FLAME, 0xFF0000FF});
    addCControlMove(manager, flame, {accelFactor, rotateFactor});
    addCInvisible(manager, flame, {false});

    Entity ship = addEntity(manager);

    addCPosition(manager, ship, {xStart, yStart});
    addCVelocity(manager, ship, {0.0f, 0.0f});
    addCScale(manager, ship, {scaleFactor});
    addCRotation(manager, ship, {0.0f, -M_PI/2.0f});
    addCShape(manager, ship, {(std::size_t)ShapeDef::SHIP, 0x00FF00FF});
    addCControlMove(manager, ship, {accelFactor, rotateFactor});
    addCCanFire(manager, ship, {false});
}

void createBullet(EntityManager& manager, float xPos, float yPos, float dir)
{
    constexpr float bulletSpeed = 1000.0f;

    const float xVel = std::cos(dir) * bulletSpeed;
    const float yVel = std::sin(dir) * bulletSpeed;

    Entity bullet = addEntity(manager);

    addCPosition(manager, bullet, {xPos, yPos});
    addCVelocity(manager, bullet, {xVel, yVel});
    addCBullet(manager, bullet, {xPos, yPos, 0xFFFF00FF});
    addCLifeTime(manager, bullet, {0.5f});
}

int main(int argc, char** argv)
{
    renderer::init("dod_test", windowWidth, windowHeight);

    std::random_device rd;
    std::mt19937 generator(rd());

    using TimeRes = std::chrono::microseconds;

    KeyMap keymap;

    EntityManager manager;

    createShip(manager);

    // Create entities
    createAstroid(manager, generator, 10.0f);
    createAstroid(manager, generator, 10.0f);
    createAstroid(manager, generator, 10.0f);
    createAstroid(manager, generator, 10.0f);
    
    createAstroid(manager, generator, 5.0f);
    createAstroid(manager, generator, 5.0f);
    createAstroid(manager, generator, 5.0f);
    createAstroid(manager, generator, 5.0f);
    createAstroid(manager, generator, 5.0f);
    createAstroid(manager, generator, 5.0f);
    createAstroid(manager, generator, 5.0f);
    createAstroid(manager, generator, 5.0f);

    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);
    createAstroid(manager, generator, 2.5f);

    
    // Set up bitsets
    
    auto lifeTimeBitset = getLifeTimeEntitiesBitset();
    auto saveLastPosBitset = getSaveLastPosBitset();
    auto moveBitset = getMoveEntitiesBitset();
    auto rotateBitset = getRotateEntitiesBitset();
    auto controlMoveBitset = getControllMoveEntitiesBitset();
    auto invisibleControllBitset = getShowInvisibleEntitiesBitset();
    auto canFireBitset = getFireingEntitiesBitset();
    auto makeDataFromEntitiesBitset = getMakeShapeDataFromEntitiesBitset();
    auto addBulletToShapeDataBitset = getAddBulletsToShapeDataBitset();

    // To use in rendering
    std::vector<float> shapeData;
    std::vector<ShapeDrawInfo> drawInfo;

    bool windowOpen = true;
    while(windowOpen)
    {
        // Timing
        auto ticks = limitFps<TimeRes, 60>();
        float frameTime = ticks / 1000000.0f;

        // Input
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            switch(e.type)
            {
                case SDL_QUIT:
                    windowOpen = false;
                    break;
                case SDL_KEYDOWN:
                    keymap[e.key.keysym.sym] = true;
                    break;
                case SDL_KEYUP:
                    keymap[e.key.keysym.sym] = false;
                    break;
            }
        }

        // Update
        removeEntities(manager);

        auto lifeTimeSysEntities = getEntitesForSystem(manager, lifeTimeBitset);
        lifeTimeEntities(lifeTimeSysEntities, manager, frameTime);
        
        auto saveLastPosSysEntities = getEntitesForSystem(manager, saveLastPosBitset);
        saveLastPos(saveLastPosSysEntities, manager);

        auto moveSysEntities = getEntitesForSystem(manager, moveBitset);
        moveEntities(moveSysEntities, manager, frameTime);

        auto rotateSysEntities = getEntitesForSystem(manager, rotateBitset);
        rotateEntites(rotateSysEntities, manager, frameTime);

        auto controllerSysEntites = getEntitesForSystem(manager, controlMoveBitset);
        controllEnities(controllerSysEntites, manager, keymap, frameTime);

        auto invisibleControllSysEntities = getEntitesForSystem(manager, invisibleControllBitset);
        showInvisibleEntities(invisibleControllSysEntities, manager, keymap);

        auto canFireSysEntities = getEntitesForSystem(manager, canFireBitset);
        fireingEntities(canFireSysEntities, manager, keymap);

        // Transform data
        shapeData.clear();
        drawInfo.clear();

        auto makeDataFromEntitesSysEntities = getEntitesForSystem(manager, makeDataFromEntitiesBitset);
        makeShapeDataFromEntities(makeDataFromEntitesSysEntities, manager, shapeData, drawInfo);

        transformShapes(shapeData, drawInfo);

        auto addBulletToShapeDataSysEntities = getEntitesForSystem(manager, addBulletToShapeDataBitset);
        addBulletsToShapeData(addBulletToShapeDataSysEntities, manager, shapeData, drawInfo);

        // Collision handling

        //Rendering
        renderer::clear();

        renderShapes(shapeData, drawInfo);

        renderer::show();
    }


    renderer::quit();

    return 0;
}
