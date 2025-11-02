#pragma once
#include "level.h"
#include "unit.h"
#include "timer.h"
#include "tower.h"
#include "projectile.h"
#include "leveldata.h"
#include "leveldataio.h"
#include "levelEditor.h"

#include <raylib.h>
#include <raymath.h>
#include <bits/stdc++.h>
#include <vector>
#include <memory>

using namespace std;

enum class GameUIState
{
    MainMenu,
    LevelSelect,
    Playing,
    Paused,
    Controls,
    GameOver,
    LevelEditor,
    Settings
};

class Game
{
    enum class PlacementMode
    {
        wall,
        tower,
    } PlacementModeCurrent; // variable

    GameUIState currentState = GameUIState::MainMenu;

    void processEvents(bool &running);
    //void processEventsLevelEditor();
    void update(float deltaTime);

    void updateRoundSpawn(float deltaTime); // update spawning of units
    bool roundStarted = false;

    void draw();
    void drawUI();

    void newRound(); // start a new round

    void addUnit(Vector2 mousePostion, EnemyType);
    // void removeUnit(Vector2 mousePosition);
    void updateUnit(float deltaTime);
    void updateProjectiles(float deltaTime, Sound& cannonExplosionSound);

    void addTower(Vector2 mousePosition);
    void removeTower(Vector2 mousePosition);
    void upgradeTower(Vector2 mousePosition);

    void updateMainMenu(float deltaTime);

    shared_ptr<Tower> hoveredTower = nullptr;

    Timer spawnTimer, // gap between spawning units
        roundTimer;   // gap between rounds
    int spawnUnitCount = 0,
        roundCount = 0;

    float enemySpawnCooldown = 2.0f;
    EnemyType currentEnemyType = EnemyType::basic;

    int mouseDownStatus = 0;

    const int tileSize = 48;
    Level level;

    vector<Projectile> projectiles; // projectiles fired by towers
    vector<shared_ptr<Unit>> units;
    vector<shared_ptr<Tower>> towers;

    Texture2D textureOverlay;
    bool overlayVisible = false;

    bool roundCompleted = false;

    int selectLevelIndex = -1;
    vector<LevelData> allLevels;

    // money
    int baseMoney = 5000;
    int money = 0;
    int baseIncome = 2000;
    int incomeIncrement = 100;

    int calculateUpgradeCost(shared_ptr<Tower> t);

    //buying towers
    TowerType nextTowerType = TowerType::basic;

    // selling tower
    bool sellConfirm = false;
    Timer sellConfirmTimer = Timer(2.0f);
    bool showMoneyWarning = false;
    Timer moneyWarningTimer = Timer(2.0f);

    shared_ptr<Tower> selectedTower = nullptr;

    // double click
    Timer doubleClickTimer = Timer(0.4f); // max time for double click
    Vector2 lastClickedTile = {-1, -1};
    void towerSelectionAndDoubleClickUpdate(Vector2 mouse);
    void selectedTowerDisplay();

    // single click lock
    Timer clickLockTimer = Timer(0.1f);

    // castle(target tile) health
    int targetHealth = 400;
    int baseHealth = 400;

    // game over
    bool gameOver = false;

    int spawnFastCount = 0;
    int spawnBasicCount = 0;
    int spawnTankCount = 0;
    int spawnfinalBossCount = 0;

    // randomize spawnQueue
    deque<EnemyType> spawnQueue;

    // round calc
    const int maxRounds = 10;
    bool gameWon = false;

    // level editor
    unique_ptr<LevelEditor> levelEditor;

    // icons and buttons
    Texture2D basicTowerIcon;
    Texture2D sniperTowerIcon;
    Texture2D cannonTowerIcon;

    void DrawNeonButton(Rectangle rect, const char* label, Color normalColor, Color hoverColor, Color clickColor);

    Rectangle startBtn = {600, 300, 300, 60};
    Rectangle levelSelectBtn = {600, 380, 300, 60};
    Rectangle controlsBtn = {600, 460, 300, 60};
    Rectangle quitBtn = {600, 540, 300, 60};
    Rectangle resumeBtn = {600, 600, 300, 60};
    Rectangle backBtn = {50, 50, 120, 52};
    Rectangle levelEditorBtn = {600, 620, 300, 60};
    Rectangle settingsBtn = {600, 700, 300, 60};

    Color btnColor = Fade(SKYBLUE, 0.2f);
    Color NEON_BLUE = {0, 255, 255, 255};     // Cyan
    Color NEON_PURPLE = {255, 0, 255, 255};   // Magenta
    Color NEON_GREEN = {0, 255, 100, 255};    // Bright green
    Color NEON_PINK = {255, 20, 147, 255};    // Hot pink
    Color NEON_YELLOW = {255, 255, 100, 255};
    Color CORNSILK = {255, 248, 220, 255}; 
    Color NEON_LAVENDER = {230, 230, 250, 255};
    Color soundbarColor = {235,132,204, 255};
    Color NEON_RED = {255, 49, 49, 200};
    Color NEON_WHITE = {251, 255, 255, 255};

    Rectangle gameOverMainMenuBtn = {600, 300, 300, 60};
    Rectangle gameOverLevelSelectBtn = {600, 380, 300, 60};
    Rectangle gameOverRestartBtn = {600, 460, 300, 60};
    Rectangle instantGameOverBtn   = {1280,  852, 180, 52};

    Rectangle basicTowerBtnRect   = { 1430 - 50, 48 + tileSize + 5, tileSize , tileSize };
    Rectangle sniperTowerBtnRect  = { 1430, 48 + tileSize + 5, tileSize,tileSize };
    Rectangle cannonTowerBtnRect = {1430, 48 + 2 * tileSize + 10, tileSize, tileSize};
    Rectangle settingsBtn2 = {1430 , 48 + 17 * tileSize + 10, tileSize, tileSize}; 


    // main menu animation
    Texture2D mainMenuBackground[1000];
    Texture2D loadingMainMenuAnimation[100];
    int mainMenuCurrentFrame = 0;
    Timer mainMenuAnimationTimer = Timer(1.0f/30.0f); // 30 FPS
    void loadMainMenu(float deltaTime);
    Timer loadingMainMenuAnimationTimer = Timer(1.0f/60.0f);
    int loadingMainMenuAnimationCurrentFrame = 0;

    unordered_map<string, bool> buttonHoverStates;

    // sound
    Music backgroundMusic;
    Sound cannonExplosionSound;
    Sound towerShootSound;
    Sound buttonHoverSound;

    float masterVolume = 0.5f;
    float musicVolume = 1.0f;
    float soundVolume = 0.5f;

public:
    Game(int windowWidth, int windowHeight, const LevelData &data);
    ~Game();

};