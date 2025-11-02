#include <raylib.h>
#include <iostream>
#include "game.h"
#include <cstdlib>
#include <ctime>
#include "textureloader.h"
#include "levelEditor.h"
#include "leveldata.h"
#include "leveldataio.h"

// vector<LevelData> createSampleLevels()
// {
//     vector<LevelData> levels;

//     // Level 1
//     LevelData level1;
//     level1.wallPositions = {
//         {3, 3}, {4, 3}, {5, 3}, {6, 3}, {6, 4}, {6, 5}, {6, 6}};
//     level1.spawnerPositions = {
//         {1, 1}, {1, 10}};
//     level1.targetTile = {12, 6};
//     levels.push_back(level1);

//     // Level 2
//     LevelData level2;
//     level2.wallPositions = {
//         {10, 2}, {10, 3}, {10, 4}, {11, 4}, {12, 4}};
//     level2.spawnerPositions = {
//         {3, 2}, {3, 8}};
//     level2.targetTile = {13, 10};
//     levels.push_back(level2);

//     // Level 3
//     LevelData level3;
//     level3.wallPositions = {
//         {7, 7}, {8, 7}, {9, 7}, {10, 7}, {10, 6}, {10, 5}};
//     level3.spawnerPositions = {
//         {2, 6}, {2, 9}};
//     level3.targetTile = {12, 3};
//     levels.push_back(level3);

//     return levels;
// }

using namespace std;

int main()
{
    srand((unsigned)time(NULL));
    // const Color darkGreen = {20, 160, 133, 255};

    int screenWidth = 1488;
    int screenHeight = 912;

    InitWindow(screenWidth, screenHeight, "CG~~Defense");
    SetTargetFPS(60);

    bool isEditorMode = false; // manual toggle

    if (isEditorMode)
    {
        //LevelEditor editor(screenWidth, screenHeight);
        //editor.run();
       // LevelData createdLevel = editor.getLevelData();
    }
    else
    {
        Game game(screenWidth, screenHeight, LevelData());
    }

    CloseWindow();
}