#include "levelEditor.h"
#include "textureloader.h"
#include <bits/stdc++.h>
#include <iostream>
#include <raylib.h>
#include "leveldataio.h"
#include <fstream>

using namespace std;

LevelEditor::LevelEditor(int screenWidth, int screenHeight)
    : levelData(LevelData()),
      screenHeight(screenHeight),
      screenWidth(screenWidth),
      level(make_unique<Level>(31, 19))
{
    buttonHoverSound = LoadSound("E:/1-2 Courses/Structured Programming LAB/projectDemo-ProjectDefense/src/assets/Audios/buttonHover.wav");
}

// void LevelEditor::run()
// {
//     bool running = true;
//     while (!WindowShouldClose() && running)
//     {
//         processInput(running);
//         draw();
//     }
// }

void LevelEditor::processInput(bool &running)
{
    Vector2 mouse = GetMousePosition();
    Vector2 gridPosition = {mouse.x / tileSize, mouse.y / tileSize};

    // Back to menu button click
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, backBtn))
    {
        //editorDone = true;
        reset();
        running = false;
        return;
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        if (IsKeyDown(KEY_ONE))
        {
            level->setTileWall((int)gridPosition.x, (int)gridPosition.y, true);
            levelData.wallPositions.push_back(gridPosition);
            cout << "Wall added at " << gridPosition.x << " " << gridPosition.y << endl;
        }
        else if (IsKeyDown(KEY_TWO))
        {
            level->setTileWall((int)gridPosition.x, (int)gridPosition.y, false);
            level->setTileType((int)gridPosition.x, (int)gridPosition.y, TileType::enymyspawner);
            levelData.spawnerPositions.push_back(gridPosition);
            cout << "Spawner added at " << gridPosition.x << " " << gridPosition.y << endl;
        }
    }

    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
    {
        int gx = (int)gridPosition.x;
        int gy = (int)gridPosition.y;

        
        auto wallIt = find_if(levelData.wallPositions.begin(), levelData.wallPositions.end(),
                                   [gx, gy](Vector2 pos)
                                   {
                                       return (int)pos.x == gx && (int)pos.y == gy;
                                   });

        if (wallIt != levelData.wallPositions.end())
        {
            levelData.wallPositions.erase(wallIt);
            level->setTileWall(gx, gy, false);
            //cout << "Wall removed at " << gx << " " << gy << endl;
        }

        
        auto spawnerIt = find_if(levelData.spawnerPositions.begin(), levelData.spawnerPositions.end(),
                                      [gx, gy](Vector2 pos)
                                      {
                                          return (int)pos.x == gx && (int)pos.y == gy;
                                      });

        if (spawnerIt != levelData.spawnerPositions.end())
        {
            levelData.spawnerPositions.erase(spawnerIt);
            level->setTileType(gx, gy, TileType::empty); // Or however you represent non-spawner
            //cout << "Spawner removed at " << gx << " " << gy << endl;
        }
    }

    if (IsKeyPressed(KEY_S))
    {
        vector<LevelData> allLevels;
        ifstream infile("all_levels.json");
        if (infile.good())
        {
            allLevels = loadAllLevelsFromFile("all_levels.json");
        }
        allLevels.push_back(levelData);
        saveAllLevelsToFile(allLevels, "all_levels.json");
        cout << "Appended new level to all_levels.json" << endl;
    }

    // if (IsKeyPressed(KEY_E))
    // {
    //     saveAllLevelsToFile(allLevels, "all_levels.json");
    //     cout << "All levels saved to all_levels.json" << endl;
    // }
}

void LevelEditor::draw()
{
    cout << "entering level editor" << endl;

    level->draw(tileSize, 1);

    // Back button
    // DrawRectangleRec(backBtn, LIGHTGRAY);
    // DrawText("Back to Main Menu", backBtn.x + 10, backBtn.y + 15, 20, BLACK);

    DrawNeonButton(backBtn, "Back", SKYBLUE, NEON_PINK, WHITE);
}

LevelData LevelEditor::getLevelData() const
{
    return levelData;
}

void LevelEditor::reset()
{
    levelData = LevelData(); // Clear all stored tile positions
    level = make_unique<Level>(31, 19); // Recreate empty level
}

void LevelEditor::DrawNeonButton(Rectangle rect, const char *label, Color normalColor, Color hoverColor, Color clickColor)
{
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);
    bool clicked = hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    Color colorToUse = normalColor;

    std::string id = std::string(label) + "_" +
                     std::to_string((int)rect.x) + "_" +
                     std::to_string((int)rect.y);

    if (hovered)
    {
        if (!buttonHoverStates[id])
        {
            PlaySound(buttonHoverSound);
            buttonHoverStates[id] = true;
        }
        colorToUse = clicked ? clickColor : hoverColor;
    }
    else
    {
        buttonHoverStates[id] = false;
    }

    DrawRectangleRounded(rect, 0.35f, 48, Fade(colorToUse, 0.2f));
    DrawRectangleRoundedLines(rect, 0.35f, 48, Fade(colorToUse, 0.4f));
    DrawText(label, rect.x + 20, rect.y + 15, 22, WHITE);
}
