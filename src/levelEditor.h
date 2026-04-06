#pragma once
#include "level.h"
#include "leveldata.h"
#include "leveldataio.h"
#include <bits/stdc++.h>
#include <memory>
#include <vector>

using namespace std;

class LevelEditor {
    unique_ptr<Level> level;
    LevelData levelData;
    vector<LevelData> allLevels;

    Color NEON_PINK = {255, 20, 147, 255};    // Hot pink

    int tileSize = 48;
    int screenWidth;
    int screenHeight;

    Rectangle backBtn = {20, 20, 220, 50};
    bool editorDone = false;

    void DrawNeonButton(Rectangle rect, const char* label, Color normalColor, Color hoverColor, Color clickColor);

    unordered_map<string, bool> buttonHoverStates;

    Sound buttonHoverSound;
public:
    LevelEditor(int screenWidth, int screenHeight);
    // void run();
    void processInput(bool &running);
    void draw();
    LevelData getLevelData() const;
    bool isDone() const { return editorDone; }
    void reset();
};
