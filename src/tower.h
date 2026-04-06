#pragma once
#include <raylib.h>
#include <vector>
#include "textureloader.h"
#include <raymath.h>
#include <bits/stdc++.h>
#include <memory>
#include "unit.h"
#include "projectile.h"
#include "timer.h"
#include "projectile.h"

using namespace std;

enum class TowerType {
    basic,
    sniper,
    cannon,
};

// Which enemy the tower prioritises when choosing a target
enum class TargetingMode {
    Closest,    // nearest enemy to the tower
    Strongest,  // enemy with highest remaining HP
    First,      // enemy closest to the castle (will arrive soonest)
    Last,       // enemy furthest from the castle (just spawned)
};

class Tower {
    TowerType type;

    // int tileX;
    // int tileY;
    // int tileSize;
    // int health;
    // int damage;
    // static const float attackRange;
    // float attackCooldown;
    // float attackCooldownCurrent;

    float range;
    float fireCooldown;
    float projectileSpeed;
    float projectileMaxDistance;
    int projectileDamage;

    int getDynamicThreshold() const;

    int towerLevel = 1;
    static const int maxTowerLevel = 3;

    TargetingMode targetingMode = TargetingMode::Closest;

    Vector2 position;
    float angle;
    static const float angularSpeed;

    Texture2D textureTileTower;

    weak_ptr<Unit> findEnemy(vector<shared_ptr<Unit>>& units, Vector2 targetPos);

    weak_ptr<Unit> targetEnemy;

    bool updateAngle(float deltaTime);
    void shoot(vector<Projectile>& projectiles, vector<shared_ptr<Unit>>& units, Sound& towerShootSound);

    Timer weaponTimer;

    int totalSpent = 0;

    public:
        Tower(Vector2 setPosition, TowerType setType);

        void update(float deltaTime, vector<shared_ptr<Unit>>& units, vector<Projectile>& projectiles, Sound& towerShootSound, Vector2 targetPos);
        void upgrade();
        void draw(int tileSize);

        Vector2 getPosition() const;
        float getRange() const;

        TowerType getTowerType() const;
        int getTowerLevel() const;
        bool isMaxLevel() const;

        bool checkIfOnTile(int x, int y);

        int getTotalSpent() const;
        void addSpentCost(int cost);

        void cycleTargetingMode();
        TargetingMode getTargetingMode() const;
        const char* getTargetingLabel() const;
};