#pragma once
#include <raylib.h>
#include <bits/stdc++.h>
#include "unit.h"
#include "timer.h"
#include <memory>
#include "textureloader.h"
#include <raymath.h>
// using namespace std;

enum class ProjectileType
{
    basic,
    sniper,
    cannon
};

class Projectile
{
    ProjectileType type;
    Vector2 position;
    Vector2 direction;
    float distanceTraveled = 0.0f;

    Texture2D bulletTex;
    
    // explosion animation
    int currentBlastFrame = 0;
    Timer blastAnimationTimer = Timer(1 / 82.0f);
    bool isExploding = false;
    bool explosionFinished = false;
    static const int totalBlastFrames = 41;
    static bool explosionTexturesLoaded;
    static Texture2D explosionAnimation[50];

    // Sound cannonExplosionSound;

    // cannon area damage
    float explosionRadius = 1.0f;
    bool areaDamageApplied = false;
    Vector2 explosionCenter;

    bool collided = false;

    float speed;
    float maxDistance;
    int damage;

    static const float size; // Size of the projectile

    void checkCollisionWithEnemy(vector<shared_ptr<Unit>> units, Sound& cannonExplosionSound);

public:
    // static const float speed;

    static void loadExplosionTextures();

    Projectile(Vector2 setPosition, Vector2 setDirection,
               float speed, float maxDistance, int damage, ProjectileType projtype,
               Vector2 setExplosionCenter = {0.0f, 0.0f});
    void update(float deltaTime, vector<shared_ptr<Unit>> units, Sound& cannonExplosionSound);
    void draw(int tileSize);
    bool checkCollision();
    float getProjectileSpeed();
};