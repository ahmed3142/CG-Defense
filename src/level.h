#pragma once
#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <inttypes.h>
#include <queue>
#include <vector>
#include <memory>
#include <bits/stdc++.h>

#include "textureloader.h"
#include "leveldata.h" 


using namespace std;
enum class TileType{
        empty,
        wall,
        enymyspawner,
};

class Level
{
private:
    static const unsigned char MAX_LEVELS = 255;

    struct Tile
    {
        TileType type = TileType::empty;
        int directionX = 0; // flow direction
        int directionY = 0;
        bool isWall = false;
        // bool isSpawn = false;
        unsigned char flowDistance = MAX_LEVELS;
    };

    // void drawTile(int x, int y, int tileSize);
    void calculateFlowDistances();
    void calculateFlowField();
    void CalculateFlowFieldForTile(); // flow directions

    TileType getTileType(int x, int y); 
    

    
    //vector<Tile> tiles48; 
    vector<Tile> tiles; 
    const int tileCountX;
    const int tileCountY;
    int targetTileX;
    int targetTileY;

    Texture2D textureTileWall;
    Texture2D textureTileTarget;
    Texture2D textureTileEmpty;
    Texture2D textureTileArrowUp;
    Texture2D textureTileArrowUpRight;
    Texture2D textureTileArrowRight;
    Texture2D textureTileArrowDownRight;
    Texture2D textureTileArrowDown;
    Texture2D textureTileArrowDownLeft;
    Texture2D textureTileArrowLeft;
    Texture2D textureTileArrowUpLeft;
    Texture2D textureTileEnemySpawner;
    Texture2D textureTileBackgroundx;


public:
    Level(int setTileCountX, int setTileCountY);
    ~Level();

    Vector2 getRandomEnemySpawnerPosition();

    void draw(int tileSize, int x);
    void drawTile(int x, int y, int tileSize);
    bool isTileWall(int x, int y);
    void setTileWall(int x, int y, bool isWall);
    void setTileType(int x,int y,TileType type);
    Vector2 getTargetPosition();
    Vector2 getFlowNormal(int x, int y);
    
    void printLevelInfo();
    void loadFromData(const LevelData &data);
    
    // bugged i think
    void resetLevel();
};