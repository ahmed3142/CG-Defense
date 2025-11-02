#pragma once

#include <raylib.h>
#include <vector>
#include "level.h"
#include <bits/stdc++.h>
#include <memory>

using namespace std;

enum class EnemyType{
    basic,
    fast,
    tank,
    physics,
    finalBoss
};

class Unit {
    Vector2 position;
    Vector2 velocity; 
    // static const float speed;
    // static const float size;

    Texture2D texture;
    bool alive = true;
    bool reachedTarget = false;
    

    const int healthBasic=2;
    int currentHealth = healthBasic; 
    
    EnemyType type;
    float speed;
    float size;
    int maxHealth;


public: 
    Unit(Vector2 setPosition, EnemyType enemyType = EnemyType::basic);
    void update(float deltaTime, Level& level, vector<shared_ptr<Unit>>& units);
    void draw(int tileSize);
    bool checkOverlap(Vector2 positionOfOtherUnit, float otherUnitSize);
    bool getIsAlive();
    bool getIsReached();
    Vector2 getPosition();
    int getCurrentHealth();
    void damage(int damageAmount);
    Vector2 getVelocity() const;

    bool hasDamagedTarget = false;

    EnemyType getEnemyType() const;

    inline static int nextId = 1;
    int unitId;

};