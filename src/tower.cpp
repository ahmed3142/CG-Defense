#include "tower.h"
#include <raymath.h>
#include <algorithm>
#include "projectile.h"
#include <cmath>
#include <algorithm>   

using namespace std;

const float Tower::angularSpeed = 180.0f * DEG2RAD * 5.0f; 
// const float Tower::attackRange = 5.0f; // attack range in tiles


Tower::Tower(Vector2 setPosition, TowerType setType) :
    type(setType), // setting tower type 
    position(setPosition),
    angle(0.0f), // angle in radians
    weaponTimer(1.0f) // seconds cooldown for shooting
{
    //towerShootSound = LoadSound("E:/1-2 Courses/Structured Programming LAB/projectDemo-ProjectDefense/src/assets/Audios/shootingTower.mp3");
    
    //SetSoundVolume(towerShootSound, 0.5f);

    switch (type) {
    case TowerType::basic:
        textureTileTower = *TextureLoader::LoadTextureFromFile("Basic Tower2.png");
        range = 4.0f;
        fireCooldown = 1.0f;
        projectileSpeed = 20.0f;
        projectileMaxDistance = 5.0f;
        projectileDamage = 2;
        break;

    case TowerType::sniper:
        textureTileTower = *TextureLoader::LoadTextureFromFile("Sniper Tower2.png");
        range = 7.0f;
        fireCooldown = 2.0f;
        projectileSpeed = 40.0f;
        projectileMaxDistance = 15.0f;
        projectileDamage = 3;
        break;
    case TowerType::cannon:
        textureTileTower = *TextureLoader::LoadTextureFromFile("Cannon Tower2.png");
        range = 3.0f;
        fireCooldown = 1.5f;
        projectileSpeed = 20.0f;
        projectileMaxDistance = 10.0f;
        projectileDamage = 2;
        break;

        weaponTimer.setTo(fireCooldown);
    }
}

void Tower::update(float deltaTime, vector<shared_ptr<Unit>>& units, vector<Projectile>& projectiles, Sound &towerShootSound)
{
    weaponTimer.countDown(deltaTime); // update weapon timer

    
    
    // picking target  
    if(auto e = targetEnemy.lock()){
        if(e-> getIsAlive() == false || Vector2Distance(position, e->getPosition()) > range){
            targetEnemy.reset(); 
        }
    }

    if(targetEnemy.expired())
    {
        targetEnemy = findEnemy(units); //finding new target
    }

    if(updateAngle(deltaTime)){
        shoot(projectiles,units,towerShootSound);
    }
}

void Tower::draw(int tileSize)
{
    // Origin for rotation: center of the destination rectangle (half tileSize)
    Vector2 origin = { tileSize / 2.0f, tileSize / 2.0f };

    // Destination rectangle centered at (position.x * tileSize, position.y * tileSize)
    Rectangle destRect = {
        position.x * tileSize + origin.x,
        position.y * tileSize + origin.y,
        (float)tileSize,
        (float)tileSize
    };
    float drawAngle = (angle * RAD2DEG) + 90; // angle in degrees 

    Rectangle sourceRect = { 0.0f, 0.0f, (float)textureTileTower.width, (float)textureTileTower.height };
    DrawTexturePro(textureTileTower, sourceRect, destRect, origin, drawAngle, WHITE);
    DrawText(TextFormat("Lv%d", towerLevel) , position.x * tileSize + 4, position.y * tileSize + 4, 10, BLACK);

    if (auto t = targetEnemy.lock()) {
        Vector2 pos = t->getPosition();
        DrawCircleLines((int)((pos.x) * tileSize), (int)((pos.y) * tileSize), tileSize * 0.01f, RED);
    }
}

bool Tower::checkIfOnTile(int x,int y){
    return ((int)position.x == x && (int)position.y == y);
}

weak_ptr<Unit> Tower::findEnemy(vector<shared_ptr<Unit>>& units)
{
    weak_ptr<Unit> bestTarget;
    float closestDistance = FLT_MAX;
    float closestHighHealth = FLT_MAX;

    const int dynamicThreshold = getDynamicThreshold();

    for (auto& unit : units) {
        if (!unit || !unit->getIsAlive()) continue;

        float dist = Vector2Distance(position, unit->getPosition());
        if (dist > range) continue;

        int health = unit->getCurrentHealth();

        if (type == TowerType::sniper && health > dynamicThreshold) {
            if (dist < closestHighHealth) {
                bestTarget = unit;
                closestHighHealth = dist;
            }
        } else if (bestTarget.expired() && dist < closestDistance) {
            bestTarget = unit;
            closestDistance = dist;
        }
    }

    return bestTarget;
}

bool Tower::updateAngle(float deltaTime)
{
    if (auto e = targetEnemy.lock())
    {
        Vector2 T = position;

        // 2) enemy’s current center‐ed position:
        Vector2 U = e->getPosition();
        U.x -= 0.5f;   
        U.y -= 0.5f;

        // 3) enemy’s velocity:
        Vector2 v_o = e->getVelocity();

        // 4) projectile speed:
        float v_p = projectileSpeed; // speed of the projectile

        // 5) build quadratic a·t² + b·t + c = 0
        Vector2 toTarget = Vector2Subtract(U, T);
        float a = Vector2DotProduct(v_o, v_o) - v_p*v_p;
        float b = 2.0f * Vector2DotProduct(toTarget, v_o);
        float c = Vector2DotProduct(toTarget, toTarget);

        // 6) discriminant
        float disc = b*b - 4.0f*a*c;
        Vector2 aimPoint;

        if (disc > 0.0f)
        {
            float sqrtD = sqrtf(disc);
            float t1 = (-b + sqrtD)/(2.0f*a);
            float t2 = (-b - sqrtD)/(2.0f*a);
            float t = FLT_MAX;
            if (t1>0) t = t1;
            if (t2>0 && t2<t) t = t2;

            if (t<FLT_MAX)
            {
                // intercept point
                aimPoint = Vector2Add(U, Vector2Scale(v_o, t));
            }
            else
            {
                aimPoint = U;  // fallback
            }
        }
        else
        {
            aimPoint = U;      // no real solution → fallback
        }

        // 7) compute desired angle
        Vector2 dir = Vector2Normalize(Vector2Subtract(aimPoint, T));
        float desired = atan2f(dir.y, dir.x);

        // 8) turn smoothly toward it
        float diff = desired - angle;
        if (diff >  PI) diff -= 2*PI;
        if (diff < -PI) diff += 2*PI;

        float maxTurn = angularSpeed * deltaTime;
        if (fabsf(diff) <= maxTurn)
        {
            angle = desired;
            return true;   // “locked on” → you can shoot this frame
        }
        angle += (diff>0 ? +maxTurn : -maxTurn);
        if (angle >  PI) angle -= 2*PI;
        if (angle < -PI) angle += 2*PI;
    }
    return false;
}

void Tower::shoot(vector<Projectile> &projectiles, vector<shared_ptr<Unit>>& units, Sound& towerShootSound){
    if(weaponTimer.timeSIsZero()){
        // Vector2 towerCenter = {position.x + 0.29167f, position.y + 0.29167f}; 
        Vector2 towerCenter = {position.x , position.y}; 
        Vector2 direction = {cosf(angle), sinf(angle)};
        
        switch (type){
        case TowerType::basic:
            projectiles.push_back(Projectile(towerCenter, direction, 
                projectileSpeed, projectileMaxDistance, projectileDamage, ProjectileType::basic));
                PlaySound(towerShootSound);
                weaponTimer.setTo(fireCooldown); // reset the timer for the next shot
                break;
        case TowerType::sniper:
            projectiles.push_back(Projectile(towerCenter, direction, 
                projectileSpeed, projectileMaxDistance, projectileDamage, ProjectileType::sniper));
                PlaySound(towerShootSound);
                weaponTimer.setTo(fireCooldown); // reset the timer for the next shot
                break;   
        case TowerType::cannon:
            Vector2 targetPosittion = position;
            if(auto t = targetEnemy.lock()){
                targetPosittion = t->getPosition();
            }
            targetPosittion.x += 0.5f; 
            targetPosittion.y += 0.5f; 
            projectiles.push_back(Projectile(towerCenter, direction, 
                projectileSpeed, projectileMaxDistance, projectileDamage, ProjectileType::cannon, targetPosittion));
            PlaySound(towerShootSound);
                // for (auto &unit : units) {
            //     if (unit && unit->getIsAlive() &&
            //         Vector2Distance(unit->getPosition(), position) <= range) {
            //         unit->damage(projectileDamage);
            //     }
            // }
            weaponTimer.setTo(fireCooldown); // reset the timer for the next shot
            break; 
        }

    }
}

void Tower::upgrade() {
    if (towerLevel >= maxTowerLevel) return;

    towerLevel++;

    switch (type) {
        case TowerType::basic:
            range += 0.5f;
            projectileDamage += 1;
            fireCooldown = max(0.1f, fireCooldown - 0.4f);
            projectileMaxDistance += 0.5f;
            break;

        case TowerType::sniper:
            range += 2.0f;
            projectileDamage += 1;
            projectileMaxDistance += 2.0f;
            fireCooldown = max(0.3f, fireCooldown - 0.5f);
            break;
        case TowerType::cannon:
            range += 0.5f;
            projectileDamage += 3;
            fireCooldown = max(0.3f, fireCooldown - 0.3f);
            break;
    }

    weaponTimer.setTo(fireCooldown);
}

Vector2 Tower::getPosition() const {
    return position;
}

float Tower::getRange() const {
    return range;
}

int Tower::getDynamicThreshold() const {
    switch (type) {
        case TowerType::basic:
            return 0; // Basic tower has no dynamic threshold
        case TowerType::sniper:
            if(towerLevel > 1) return 6 + towerLevel;
        default:
            return 0; // Default case, should not happen
    }
} 

TowerType Tower::getTowerType() const {
    return type;
}

int Tower::getTowerLevel() const {
    return towerLevel;
}

int Tower::getTotalSpent() const {
    return totalSpent;
}

void Tower::addSpentCost(int cost) {
    totalSpent += cost;
}

bool Tower::isMaxLevel() const {
    return towerLevel>=maxTowerLevel;
}

