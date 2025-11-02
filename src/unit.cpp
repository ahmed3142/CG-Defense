#include "unit.h"
#include "game.h"
#include <raymath.h>
#include "textureloader.h"
#include <bits/stdc++.h>
#include <memory>
#include <cmath>

// const float Unit::speed = 1.0f;
// const float Unit::size = 0.4f;

Unit::Unit(Vector2 setPosition, EnemyType enemyType) : 
    position(setPosition),
    type(enemyType),
    unitId(nextId++)
{
    switch (type) {
        case EnemyType::basic:
            texture = *TextureLoader::LoadTextureFromFile("Unit2.png");
            speed = 1.0f;
            size = 0.4f;
            maxHealth = 5;
            break;

        case EnemyType::fast:
            texture = *TextureLoader::LoadTextureFromFile("Unit2.png");
            speed = 2.0f;
            size = 0.3f;
            maxHealth = 3;
            break;

        case EnemyType::tank:
            texture = *TextureLoader::LoadTextureFromFile("Unit2.png");
            speed = 0.5f;
            size = 0.6f;
            maxHealth = 70;
            break;
        case EnemyType::physics:
            texture = *TextureLoader::LoadTextureFromFile("Unit2.png");
            speed = 1.0f;
            size = 0.3f;
            maxHealth = 1;
            break;
        case EnemyType::finalBoss:
            texture = *TextureLoader::LoadTextureFromFile("Unit2.png");
            speed = 0.2f;
            size = 1.5f;
            maxHealth = 10000;
            break;
    }

    currentHealth = maxHealth;
}

void Unit::draw(int tileSize)
{
    DrawTexturePro(texture, {0.0f, 0.0f, (float)texture.width, (float)texture.height},
                   {(position.x - size / 2) * tileSize, (position.y - size / 2) * tileSize, size * tileSize, size * tileSize},
                   {0.0f, 0.0f}, 0.0f, WHITE);
    // cout << "Unit at position: (" << position.x << ", " << position.y << ")" << endl;
}

bool Unit::checkOverlap(Vector2 positionOfOtherUnit, float otherUnitSize) {
    float totalSize = (otherUnitSize + size) * 0.5f;
    return Vector2DistanceSqr(positionOfOtherUnit, position) <= totalSize * totalSize;
}

void Unit::update(float deltaTime, Level &level, vector<shared_ptr<Unit>> &units)
{
    Vector2 oldPosition = position;

    float targetDistance = Vector2Distance(level.getTargetPosition(), position);
    float moveDistance = (this->speed) * deltaTime;
    // cout << "Unit at position: (" << position.x << ", " << position.y << ") with target distance: " << targetDistance << endl;

    // if(targetDistance < 0.7f)
    // {
    //     alive=false;
    //     reachedTarget=true;
    // }

    if (moveDistance > targetDistance)
        moveDistance = targetDistance;

    Vector2 normalDirection(level.getFlowNormal((int)position.x, (int)position.y));
    if((int) position.x == (int)level.getTargetPosition().x && (int) position.y == (int)level.getTargetPosition().y)
    {
        normalDirection= Vector2Subtract(level.getTargetPosition(), position);
        normalDirection = Vector2Normalize(normalDirection);
    }
    Vector2 moveForward = normalDirection * moveDistance;

    // moving forward without bumping other unit
    bool ok = true;
    for (int count = 0; count < (int)units.size() && ok; count++)
    {
        auto &unitSelected = units[count];
        if(unitSelected != nullptr && unitSelected->checkOverlap(position, this->size) && unitSelected.get() != this)
        {
            // They overlap so check and see if this unit is moving towards or away from the unit it overlaps.
            Vector2 directionToOther = Vector2Subtract(unitSelected->position, position);
            // Ensure that they're not directly on top of each other.
            if (Vector2Length(directionToOther) > 0.35f) // spacing between enemies
            {
                // Check the angle between the units positions and the direction that this unit is traveling.
                // Ensure that this unit isn't moving directly towards the other unit (by checking the angle between).
                Vector2 normalToOther = Vector2Normalize(directionToOther);
                float angleBtw = fabs(Vector2Angle(normalToOther, normalDirection));
                if (angleBtw < 3.14159265359f / 4.0f) // tan45
                    // Don't allow the move.
                    ok = false;
            }
        }
    }
    // unit can move without walking through wall;
        if (ok) {
			//Check if it needs to move in the x direction.  If so then check if the new x position, plus an amount of spacing 
			//(to keep from moving too close to the wall) is within a wall or not and update the position as required.
			const float spacing = 0.47f;
			int x = (int)(position.x + moveForward.x + copysign(spacing, moveForward.x));
			int y = (int)(position.y);
			if (moveForward.x != 0.0f && level.isTileWall(x, y) == false){
				position.x += moveForward.x;
                if(level.isTileWall(position.x,position.y+spacing)) position.y-=(0.03f);
                if(level.isTileWall(position.x,position.y-spacing)) position.y+=0.03f;
            }
			//Do the same for the y direction.
			x = (int)(position.x);
			y = (int)(position.y + moveForward.y + copysign(spacing, moveForward.y));
			if (moveForward.y != 0.0f && level.isTileWall(x, y) == false){
				position.y += moveForward.y;
                if(level.isTileWall(position.x+spacing,position.y)) position.x-=(0.03f);
                if(level.isTileWall(position.x-spacing,position.y)) position.x+=0.03f;
            }
		}
    velocity = Vector2Scale(Vector2Subtract(position, oldPosition), 1.0f / deltaTime);
}

bool Unit::getIsAlive()
{
    return alive;
}

Vector2 Unit::getPosition()
{
    return position;
}

int Unit::getCurrentHealth	()
{
    return currentHealth;
}

void Unit::damage(int damageAmount){
    if(damageAmount>0){
        cout << "Unit " << unitId << " damaged by " << damageAmount << endl;
        currentHealth = max(0, currentHealth - damageAmount);
        cout << "cuurent health: " << currentHealth << endl; 
        if(currentHealth <= 0){
            currentHealth=0;
            alive = false;
        }
    }
}

Vector2 Unit::getVelocity() const
{
    return velocity;
}

EnemyType Unit::getEnemyType() const {
    return type;
}

bool Unit::getIsReached() {
    return reachedTarget;
}
