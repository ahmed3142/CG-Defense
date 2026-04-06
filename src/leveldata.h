#pragma once

#include <raylib.h>
#include <bits/stdc++.h>

using namespace std;

struct LevelData {
    vector<Vector2> wallPositions;
    vector<Vector2> spawnerPositions;
    Vector2 targetTile = {15,9}; //default target 
};