#include "projectile.h"

const float Projectile::size = 0.4f;
bool Projectile::explosionTexturesLoaded = false;
Texture2D Projectile::explosionAnimation[50];

void Projectile::loadExplosionTextures() {
    if(!explosionTexturesLoaded) {
        for(int i = 0; i < totalBlastFrames; i++) { 
            string numstr = to_string(i + 1);
            // while (numstr.length() < 5) {
            //     numstr = "0" + numstr;
            // }
            string filename = "cannonExplosion/" + numstr + ".gif";
            explosionAnimation[i] = *TextureLoader::LoadTextureFromFile(filename);
        }
    }
}

Projectile::Projectile(Vector2 setPosition, Vector2 setDirection, float spd, float maxDist, int dmg, ProjectileType projtype, Vector2 setExplosionCenter)
    : position(setPosition),
      direction(Vector2Normalize(setDirection)),
      speed(spd),
      maxDistance(maxDist),
      damage(dmg),
      type(projtype),
      collided(false),
      isExploding(false),
      explosionFinished(false),
      currentBlastFrame(0),
      areaDamageApplied(false),
      explosionCenter(setExplosionCenter)
{
    bulletTex = *TextureLoader::LoadTextureFromFile("Bullet4.png");

    // cannonExplosionSound = LoadSound("E:/1-2 Courses/Structured Programming LAB/projectDemo-ProjectDefense/src/assets/Audios/cannonBlast.mp3");


    if(type == ProjectileType::cannon && !explosionTexturesLoaded) {
        loadExplosionTextures();
        explosionTexturesLoaded = true;
    }
}

void Projectile::update(float deltaTime, vector<shared_ptr<Unit>> units, Sound &cannonExplosionSound)
{
    if (!isExploding && !collided) { 
        float moveDistance = speed * deltaTime;
        position += direction * moveDistance;
        distanceTraveled += moveDistance;

        
        checkCollisionWithEnemy(units, cannonExplosionSound); 

        
        if (distanceTraveled >= maxDistance) {
            collided = true; 
            // if (type == ProjectileType::cannon) {
                // isExploding = true; 
                // explosionFinished = false;
            // }
        }
    }

    if (isExploding) { 
        if(type == ProjectileType::cannon && !areaDamageApplied) {
            areaDamageApplied = true;
            for(auto &unit : units) {
                if(unit && unit->getIsAlive()){
                    float dist = Vector2Distance(position, unit->getPosition());
                    if(dist <= explosionRadius) {
                        unit->damage(damage);
                    }
                }
            }
        }
        blastAnimationTimer.countDown(deltaTime);
        if (blastAnimationTimer.timeSIsZero()) {
            currentBlastFrame++;
            if (currentBlastFrame >= totalBlastFrames) {
                explosionFinished = true;
                
                currentBlastFrame = totalBlastFrames - 1; //keeps showing the last frame
            }
            blastAnimationTimer.resetToMax();
        }
    }
}

void Projectile::draw(int tileSize)
{
    if (isExploding) { 
        DrawTexture(explosionAnimation[currentBlastFrame],
                    (explosionCenter.x * tileSize) - 50, (explosionCenter.y * tileSize) - 50, WHITE);
    } else { 
        DrawTexture(bulletTex, (int)((position.x) * tileSize), (int)((position.y) * tileSize), WHITE);
    }
}

bool Projectile::checkCollision() {
    if (type == ProjectileType::cannon) {
        return explosionFinished || (collided && !isExploding);
    } else {
        return collided;
    }
}

void Projectile::checkCollisionWithEnemy(vector<shared_ptr<Unit>> units, Sound& cannonExplosionSound){
    if(!collided && !isExploding){ 
        for(int i=0; i<(int)units.size() && !collided; i++){
            auto &unit = units[i];
            Vector2 projectileCenter = {position.x, position.y};
            //if(type != ProjectileType::cannon) {
                projectileCenter.x += 0.5f;
                projectileCenter.y += 0.5f; 
            //}
            if(unit != nullptr && unit->getIsAlive() && unit->checkOverlap(projectileCenter, size)){
                unit->damage(damage);
                collided = true; 
                if(type == ProjectileType::cannon) {
                    isExploding = true; 
                    PlaySound(cannonExplosionSound);
                    explosionCenter = position;
                }
                break;
            }
        }
    }
}

float Projectile::getProjectileSpeed() {
    return speed;
}