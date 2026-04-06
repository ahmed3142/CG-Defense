#include "game.h"
#include "textureloader.h"
#include "tower.h"
#include "level.h"
#include "projectile.h"
#include <fstream>

using namespace std;

#define rep(i, n) for (int i = 0; i < (n); ++i)
#define rrep(i, a, b) for (int i = (a); i < (b); ++i)

namespace
{
struct DifficultyTuning
{
    const char *label;
    int startingMoney;
    int startingHealth;
    int startingIncome;
    int incomeIncrement;
    float rewardMultiplier;
    float spawnCooldownMultiplier;
    float basicScale;
    float fastScale;
    float tankScale;
    int physicsPerRound;
    Color accentColor;
};

const DifficultyTuning &GetDifficultyTuning(DifficultyLevel level)
{
    static const DifficultyTuning easy{
        "Easy", 6200, 520, 2400, 220, 1.15f, 1.20f, 0.85f, 0.80f, 0.75f, 1, GREEN};
    static const DifficultyTuning medium{
        "Medium", 5000, 400, 2000, 160, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 2, ORANGE};
    static const DifficultyTuning hard{
        "Hard", 4200, 300, 1600, 100, 0.85f, 0.78f, 1.20f, 1.30f, 1.35f, 3, RED};

    switch (level)
    {
    case DifficultyLevel::Easy:
        return easy;
    case DifficultyLevel::Hard:
        return hard;
    case DifficultyLevel::Medium:
    default:
        return medium;
    }
}

int ScaleEnemyCount(int baseCount, float multiplier)
{
    return max(0, (int)lround(baseCount * multiplier));
}
} // namespace

Game::Game(int windowWidth, int windowHeight, const LevelData &data)
    : level(windowWidth / tileSize, windowHeight / tileSize),
      PlacementModeCurrent(PlacementMode::tower),
      spawnTimer(2.0f), roundTimer(2.0f),
      money(baseMoney)
{
    // audio control
    InitAudioDevice();

    backgroundMusic = LoadMusicStream("src/assets/Audios/backgroundMusic.mp3");
    if (backgroundMusic.stream.buffer == nullptr)
    {
        std::cerr << "Error: Failed to load background music!" << std::endl;
        CloseAudioDevice();
        exit(1);
    }
    buttonHoverSound = LoadSound("src/assets/Audios/buttonHover.wav");
    towerShootSound = LoadSound("src/assets/Audios/shootingTower.mp3");
    cannonExplosionSound = LoadSound("src/assets/Audios/cannonBlast.mp3");

    PlayMusicStream(backgroundMusic);
    SetMusicVolume(backgroundMusic, musicVolume);

    // tower icons
    textureOverlay = *TextureLoader::LoadTextureFromFile("Overlay.png"); // menu
    basicTowerIcon = *TextureLoader::LoadTextureFromFile("Basic Tower.png");
    sniperTowerIcon = *TextureLoader::LoadTextureFromFile("Sniper Tower.png");
    cannonTowerIcon = *TextureLoader::LoadTextureFromFile("Cannon Tower.png");

    // loading levels
    allLevels = loadAllLevelsFromFile("all_levels.json");
    if (allLevels.empty())
    {
        cout << "No levels found." << endl;
        exit(1);
    }

    applyDifficultySettings();

    const float deltaTime = 1.0f / 60.0f;
    float accumulator = 0.0f;

    loadHighScores();

    // Load only the small loading-animation strip; main menu frames are loaded lazily
    loadMainMenu();

    bool running = true;
    SetTargetFPS(60);

    // level.loadFromData(data); // external level

    while (running && !WindowShouldClose())
    {
        float delta = GetFrameTime();
        accumulator += delta;

        while (accumulator >= deltaTime)
        {
            processEvents(running);
            update(deltaTime);
            accumulator -= deltaTime;
        }

        draw();
    }
}

Game::~Game()
{
    TextureLoader::DeallocTexture(); // Deallocate all textures
    UnloadMusicStream(backgroundMusic);
    CloseAudioDevice();
}

int Game::getDefaultLevelIndex() const
{
    return allLevels.size() > 3 ? 3 : 0;
}

const char *Game::getDifficultyLabel() const
{
    return GetDifficultyTuning(selectedDifficulty).label;
}

Color Game::getDifficultyColor() const
{
    return GetDifficultyTuning(selectedDifficulty).accentColor;
}

void Game::applyDifficultySettings()
{
    const DifficultyTuning &tuning = GetDifficultyTuning(selectedDifficulty);

    baseMoney = tuning.startingMoney;
    baseHealth = tuning.startingHealth;
    baseIncome = tuning.startingIncome;
    incomeIncrement = tuning.incomeIncrement;

    money = baseMoney;
    targetHealth = baseHealth;
}

void Game::resetGameplaySession()
{
    gameOver = false;
    gameWon = false;
    roundStarted = false;
    roundCompleted = false;

    units.clear();
    towers.clear();
    projectiles.clear();
    spawnQueue.clear();

    hoveredTower = nullptr;
    selectedTower = nullptr;
    sellConfirm = false;
    overlayVisible = false;
    mouseDownStatus = 0;
    nextTowerType = TowerType::basic;

    spawnUnitCount = 0;
    spawnFastCount = 0;
    spawnBasicCount = 0;
    spawnTankCount = 0;
    spawnfinalBossCount = 0;
    roundCount = 0;

    lastClickedTile = {-1, -1};
    clickLockTimer.resetToZero();
    doubleClickTimer.resetToZero();
    sellConfirmTimer.resetToZero();
    spawnTimer.resetToZero();
    roundTimer.resetToMax();

    roundKills = 0;
    roundMoneyEarned = 0;
    showRoundStats = false;
    gameSpeed2x = false;
    scoreSaved = false;

    applyDifficultySettings();
}

void Game::startLevel(int levelIndex)
{
    if (allLevels.empty())
        return;

    if (levelIndex < 0 || levelIndex >= (int)allLevels.size())
        levelIndex = getDefaultLevelIndex();

    selectLevelIndex = levelIndex;

    resetGameplaySession();
    level.resetLevel();
    level.loadFromData(allLevels[selectLevelIndex]);
    currentState = GameUIState::Playing;
}

int Game::getEnemyReward(EnemyType type) const
{
    int baseReward = 0;

    switch (type)
    {
    case EnemyType::basic:
        baseReward = 10;
        break;
    case EnemyType::fast:
        baseReward = 6;
        break;
    case EnemyType::tank:
        baseReward = 45;
        break;
    case EnemyType::physics:
        baseReward = 20;
        break;
    case EnemyType::finalBoss:
        baseReward = 500;
        break;
    }

    return max(1, (int)lround(baseReward * GetDifficultyTuning(selectedDifficulty).rewardMultiplier));
}

void Game::processEvents(bool &running)
{ // for every frame
    bool mouseDownThisFrame = (mouseDownStatus == 0);
    bool mouseClick = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    Vector2 mouse = GetMousePosition();

    if (currentState == GameUIState::MainMenu)
    {
        if (mouseClick && CheckCollisionPointRec(mouse, startBtn))
        {
            selectLevelIndex = getDefaultLevelIndex();
            difficultyReturnState = GameUIState::MainMenu;
            currentState = GameUIState::DifficultySelect;
        }
        else if (mouseClick && CheckCollisionPointRec(mouse, levelSelectBtn))
        {
            currentState = GameUIState::LevelSelect;
        }
        else if (mouseClick && CheckCollisionPointRec(mouse, controlsBtn))
        {
            currentState = GameUIState::Controls;
        }
        else if (mouseClick && CheckCollisionPointRec(mouse, quitBtn))
        {
            running = false;
        }
        else if (mouseClick && CheckCollisionPointRec(mouse, levelEditorBtn))
        {
            currentState = GameUIState::LevelEditor;
            levelEditor = make_unique<LevelEditor>(1488, 912);
        }
        else if (CheckCollisionPointRec(GetMousePosition(), settingsBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            currentState = GameUIState::Settings;
        }

        return; // skip other input
    }

    if (currentState == GameUIState::Controls && mouseClick && CheckCollisionPointRec(mouse, backBtn))
    {
        currentState = GameUIState::MainMenu;
        return;
    }

    if (currentState == GameUIState::LevelSelect && mouseClick && CheckCollisionPointRec(mouse, backBtn))
    {
        currentState = GameUIState::MainMenu;
        return;
    }

    if (currentState == GameUIState::DifficultySelect && mouseClick && CheckCollisionPointRec(mouse, backBtn))
    {
        currentState = difficultyReturnState;
        return;
    }

    // PLAYING or PAUSED BACK BUTTON
    if ((currentState == GameUIState::Playing || currentState == GameUIState::Paused) && mouseClick && CheckCollisionPointRec(mouse, instantGameOverBtn))
    {
        gameOver = true;
        gameWon = false;
        currentState = GameUIState::GameOver;
        return;
    }

    if (currentState == GameUIState::LevelSelect)
    {
        for (int i = 0; i < (int)allLevels.size(); ++i)
        {
            Rectangle levelBtn = {600, 200 + i * 60, 300, 50};
            if (mouseClick && CheckCollisionPointRec(mouse, levelBtn))
            {
                selectLevelIndex = i;
                difficultyReturnState = GameUIState::LevelSelect;
                currentState = GameUIState::DifficultySelect;
                return;
            }
        }
    }

    if (currentState == GameUIState::DifficultySelect)
    {
        if (mouseClick && CheckCollisionPointRec(mouse, easyDifficultyBtn))
        {
            selectedDifficulty = DifficultyLevel::Easy;
            startLevel(selectLevelIndex);
            return;
        }
        if (mouseClick && CheckCollisionPointRec(mouse, mediumDifficultyBtn))
        {
            selectedDifficulty = DifficultyLevel::Medium;
            startLevel(selectLevelIndex);
            return;
        }
        if (mouseClick && CheckCollisionPointRec(mouse, hardDifficultyBtn))
        {
            selectedDifficulty = DifficultyLevel::Hard;
            startLevel(selectLevelIndex);
            return;
        }
    }

    // PAUSED
    if (currentState == GameUIState::Paused)
    {
        if ((mouseClick && CheckCollisionPointRec(mouse, resumeBtn)) || IsKeyPressed(KEY_P))
        {
            currentState = GameUIState::Playing;
        }
        if (mouseClick && CheckCollisionPointRec(mouse, instantGameOverBtn))
        {
            gameOver = true;
            gameWon = false;
            currentState = GameUIState::GameOver;
        }
    }

    // GAMEOVER
    if (currentState == GameUIState::GameOver)
    {
        // Save high score once on the first frame of GameOver
        if (!scoreSaved && selectLevelIndex >= 0 && selectLevelIndex < MAX_SCORE_LEVELS)
        {
            if (roundCount > highScores[selectLevelIndex])
            {
                highScores[selectLevelIndex] = roundCount;
                saveHighScores();
            }
            scoreSaved = true;
        }

        if (mouseClick && CheckCollisionPointRec(mouse, gameOverMainMenuBtn))
        {
            resetGameplaySession();
            currentState = GameUIState::MainMenu;
        }
        else if (mouseClick && CheckCollisionPointRec(mouse, gameOverLevelSelectBtn))
        {
            resetGameplaySession();
            currentState = GameUIState::LevelSelect;
        }
        else if (mouseClick && CheckCollisionPointRec(mouse, gameOverRestartBtn))
        {
            startLevel(selectLevelIndex);
        }
        return;
    }

    // PLAYING
    if (currentState == GameUIState::Playing && IsKeyPressed(KEY_P))
    {
        currentState = GameUIState::Paused;
    }

    if (currentState != GameUIState::Playing)
        return;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        mouseDownThisFrame = true;
        mouseDownStatus = 1;
        // cout << mouseDownStatus << endl;
        // cout << "Mouse left button pressed" << endl;
    }
    else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
    {
        mouseDownThisFrame = true;
        mouseDownStatus = 2;
        // cout << mouseDownStatus << endl;
        // cout << "Mouse right button pressed" << endl;
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_RIGHT_BUTTON))
    {
        mouseDownStatus = 0;
        // cout << "Mouse button released" << endl;
    }

    if (IsKeyPressed(KEY_ESCAPE))
        running = false;

    if (IsKeyPressed(KEY_ONE))
    {
        if (currentState != GameUIState::LevelEditor)
            return;

        PlacementModeCurrent = PlacementMode::wall;
    }
    if (IsKeyPressed(KEY_TWO))
    {
        PlacementModeCurrent = PlacementMode::tower;
    }
    if (IsKeyPressed(KEY_M))
    {
        overlayVisible = !overlayVisible;
    }
    if (IsKeyPressed(KEY_S))
    {
        if (selectedTower != nullptr)
        {
            sellConfirm = true;
            sellConfirmTimer.resetToMax();
        }
    }
    // T — cycle targeting mode of selected tower
    if (IsKeyPressed(KEY_T) && selectedTower != nullptr)
    {
        selectedTower->cycleTargetingMode();
    }
    if (IsKeyPressed(KEY_SPACE) && currentState == GameUIState::Playing && !roundStarted && !gameOver && roundCount < maxRounds)
    {
        newRound();
    }

    Vector2 mousePosition = {mouse.x / tileSize, mouse.y / tileSize};

    if (mouseDownStatus > 0)
    {
        if (mouseDownStatus == 1)
        { // left mouse button
            if (!clickLockTimer.timeSIsZero())
                return;

            if (PlacementModeCurrent == PlacementMode::wall)
            {
                level.setTileWall((int)mousePosition.x, (int)mousePosition.y, true);
                clickLockTimer.resetToMax();
            }
            else if (PlacementModeCurrent == PlacementMode::tower)
            {
                if (mouseDownThisFrame)
                    addTower(mousePosition), clickLockTimer.resetToMax();
            }

            // tower selection and double click update
            towerSelectionAndDoubleClickUpdate(mouse);
        }
        else if (mouseDownStatus == 2)
        { // right mouse button
            if (IsKeyDown(KEY_R))
                removeTower(mousePosition);
            else if (IsKeyPressed(KEY_U))
                upgradeTower(mousePosition);
        }
    }

    // hover tower
    hoveredTower = nullptr; // Reset hovered tower
    for (auto &tower : towers)
    {
        if (tower->checkIfOnTile((int)mousePosition.x, (int)mousePosition.y))
        {
            hoveredTower = tower; //
            break;
        }
    }
    // ── Selected-tower panel buttons ─────────────────────────────────────────
    if (selectedTower != nullptr && mouseClick)
    {
        // Upgrade button
        if (!selectedTower->isMaxLevel() && CheckCollisionPointRec(mouse, towerUpgradeBtnRect))
        {
            upgradeTower(selectedTower->getPosition());
        }
        // Sell button — first click arms confirm, second click sells
        if (CheckCollisionPointRec(mouse, towerSellBtnRect))
        {
            if (sellConfirm)
            {
                removeTower(selectedTower->getPosition());
                sellConfirm   = false;
                selectedTower = nullptr;
            }
            else
            {
                sellConfirm = true;
                sellConfirmTimer.resetToMax();
            }
        }
    }

    // Also keep the legacy right-click sell confirmation
    if (sellConfirm && selectedTower != nullptr && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
    {
        removeTower(selectedTower->getPosition());
        sellConfirm   = false;
        selectedTower = nullptr;
    }

    // Speed toggle button
    if (mouseClick && CheckCollisionPointRec(mouse, speedBtn))
        gameSpeed2x = !gameSpeed2x;

    // buy tower
    if (mouseClick && CheckCollisionPointRec(mouse, basicTowerBtnRect))
    {
        nextTowerType = TowerType::basic;
        PlacementModeCurrent = PlacementMode::tower;
    }
    if (mouseClick && CheckCollisionPointRec(mouse, sniperTowerBtnRect))
    {
        nextTowerType = TowerType::sniper;
        PlacementModeCurrent = PlacementMode::tower;
    }
    if (mouseClick && CheckCollisionPointRec(mouse, cannonTowerBtnRect))
    {
        nextTowerType = TowerType::cannon;
        PlacementModeCurrent = PlacementMode::tower;
    }
    // if (CheckCollisionPointRec(GetMousePosition(), settingsBtn2) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    // {
    //     currentState = GameUIState::Settings;
    // }
}

void Game::addUnit(Vector2 spawnPos, EnemyType type)
{
    units.push_back(make_shared<Unit>(spawnPos, type));
}

// void Game::removeUnit(Vector2 mousePosition){
//     rep(i,units.size()){
//         if(units[i].checkOverlap(mousePosition, 0.0f)){
//             units.erase(units.begin()+i);
//             i--;
//         }
//     }
// }

void Game::draw()
{
    // frame
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (currentState == GameUIState::Playing)
    {
        // draw the map
        level.draw(tileSize, 0);

        // draw all units
        for (auto &unit : units)
        {
            if (unit)
                unit->draw(tileSize);
        }

        // draw all towers
        for (auto &tower : towers)
        {
            tower->draw(tileSize);
        }

        // draw all projectiles
        for (auto &projectile : projectiles)
        {
            projectile.draw(tileSize);
        }

        // optional overlay (e.g. “press M”)
        if (overlayVisible)
            DrawTexture(textureOverlay, 40, 40, WHITE);

        // round-completed message
        if (roundCompleted && units.size() == 0 && roundCount > 0)
        {
            int textX = 1488 / 2 - 200;
            int textY = 912 / 2 - 100;
            DrawText(
                "Round completed! Press SPACE to start a new round.",
                textX, textY, 20, NEON_GREEN);
        }

        // hovered-tower range circle
        if (hoveredTower != nullptr)
        {
            Vector2 center = {
                (hoveredTower->getPosition().x + 0.5f) * tileSize,
                (hoveredTower->getPosition().y + 0.5f) * tileSize};
            DrawCircleLines(
                center.x, center.y,
                hoveredTower->getRange() * tileSize,
                GRAY);
            DrawCircle(
                center.x, center.y,
                hoveredTower->getRange() * tileSize,
                Fade(BLUE, 0.3f));
        }

        // selected-tower UI overlay
        if (selectedTower != nullptr)
        {
            selectedTowerDisplay();
        }

        // ── Credits ──────────────────────────────────────────────────────────
        DrawText(("Credits: $" + std::to_string(money)).c_str(), 30, 16, 28, NEON_BLUE);

        if (showMoneyWarning)
            DrawText("Not enough credits!", 30, 48, 20, NEON_RED);

        // ── Castle health bar ─────────────────────────────────────────────────
        {
            DrawText("Castle HP", 30, 72, 18, NEON_BLUE);
            const int barX = 30, barY = 94, barW = 200, barH = 16;
            float pct = (float)targetHealth / (float)max(1, baseHealth);
            pct = Clamp(pct, 0.0f, 1.0f);
            Color fillCol = pct > 0.6f ? GREEN : (pct > 0.3f ? YELLOW : RED);
            DrawRectangle(barX, barY, barW, barH, Fade(BLACK, 0.5f));
            DrawRectangle(barX, barY, (int)(barW * pct), barH, fillCol);
            DrawRectangleLinesEx({(float)barX, (float)barY, (float)barW, (float)barH}, 1, NEON_BLUE);
            DrawText(TextFormat("%d / %d", targetHealth, baseHealth), barX + barW + 8, barY, 16, NEON_WHITE);
        }

        DrawText(TextFormat("Mode: %s", getDifficultyLabel()), 30, 116, 22, getDifficultyColor());

        // ── Round counter + End Game button ───────────────────────────────────
        string roundText = "Round: " + to_string(roundCount) + " / 10";
        int textWidth = MeasureText(roundText.c_str(), 30);
        DrawText(roundText.c_str(), 1488 - textWidth - 30, 9, 30, NEON_BLUE);
        DrawNeonButton(instantGameOverBtn, "End Game", SKYBLUE, NEON_PINK, WHITE);

        // ── 2x Speed button ───────────────────────────────────────────────────
        DrawNeonButton(speedBtn,
                       gameSpeed2x ? "Speed: 2x" : "Speed: 1x",
                       gameSpeed2x ? Fade(NEON_GREEN, 0.3f) : Fade(SKYBLUE, 0.2f),
                       NEON_GREEN, WHITE);

        // tower icons
        Color basicTint = (money >= 200) ? NEON_GREEN : NEON_PURPLE;
        DrawTextureEx(basicTowerIcon,
                      {basicTowerBtnRect.x, basicTowerBtnRect.y},
                      0.0f, 1.0f, basicTint);
        // if (nextTowerType == TowerType::basic)
        //     DrawRectangleLinesEx(basicTowerBtnRect, 2, NEON_BLUE);

        Color sniperTint = (money >= 400) ? NEON_GREEN : NEON_PURPLE;
        DrawTextureEx(sniperTowerIcon,
                      {sniperTowerBtnRect.x, sniperTowerBtnRect.y},
                      0.0f, 1.0f, sniperTint);
        // if (nextTowerType == TowerType::sniper)
        //     DrawRectangleLinesEx(sniperTowerBtnRect, 2, NEON_BLUE);

        Color cannonTint = (money >= 1000) ? NEON_GREEN : NEON_PURPLE;
        DrawTextureEx(cannonTowerIcon,
                      {cannonTowerBtnRect.x, cannonTowerBtnRect.y},
                      0.0f, 1.0f, cannonTint);
        // if (nextTowerType == TowerType::cannon)
        //     DrawRectangleLinesEx(cannonTowerBtnRect, 2, NEON_BLUE);

        DrawNeonButton(basicTowerBtnRect, "", Fade(SKYBLUE, 0.05f), Fade(NEON_PINK, 0.05f), WHITE);
        DrawNeonButton(sniperTowerBtnRect, "", Fade(SKYBLUE, 0.05f), Fade(NEON_PINK, 0.05f), WHITE);
        DrawNeonButton(cannonTowerBtnRect, "", Fade(SKYBLUE, 0.05f), Fade(NEON_PINK, 0.05f), WHITE);

        // ── Tower placement preview ───────────────────────────────────────────
        if (PlacementModeCurrent == PlacementMode::tower)
        {
            Vector2 mouse = GetMousePosition();
            int tileX = (int)(mouse.x / tileSize);
            int tileY = (int)(mouse.y / tileSize);
            if (level.isTileWall(tileX, tileY))
            {
                bool occupied = false;
                for (auto &t : towers)
                    if (t->checkIfOnTile(tileX, tileY)) { occupied = true; break; }

                Texture2D* previewTex = nullptr;
                if      (nextTowerType == TowerType::basic)  previewTex = &basicTowerIcon;
                else if (nextTowerType == TowerType::sniper) previewTex = &sniperTowerIcon;
                else                                          previewTex = &cannonTowerIcon;

                Color ghost = occupied ? Fade(RED, 0.45f) : Fade(WHITE, 0.5f);
                DrawTextureEx(*previewTex,
                    {(float)(tileX * tileSize), (float)(tileY * tileSize)},
                    0.0f, 1.0f, ghost);
            }
        }

        // ── Wave preview panel (when between rounds) ──────────────────────────
        if (!roundStarted && !gameOver && roundCount < maxRounds)
            drawWavePreview();

        // ── Round-complete stats overlay ──────────────────────────────────────
        if (roundCompleted && units.empty() && roundCount > 0)
        {
            int cx = 1488 / 2, cy = 912 / 2 + 100;
            DrawText("Round Complete!", cx - MeasureText("Round Complete!", 28) / 2, cy, 28, NEON_GREEN);
        }
        if (showRoundStats)
        {
            const int sw = 280, sh = 90;
            const int sx = 1488 / 2 - sw / 2, sy = 912 - sh - 20;
            DrawRectangleRounded({(float)sx,(float)sy,(float)sw,(float)sh}, 0.2f, 12, Fade(BLACK, 0.78f));
            DrawRectangleRoundedLines({(float)sx,(float)sy,(float)sw,(float)sh}, 0.2f, 12, Fade(NEON_GREEN,0.6f));
            DrawText(TextFormat("Round %d complete!", roundCount - 1), sx+12, sy+10, 18, NEON_YELLOW);
            DrawText(TextFormat("Kills  : %d",   lastRoundKills),       sx+12, sy+34, 18, WHITE);
            DrawText(TextFormat("Earned : $%d",  lastRoundMoneyEarned), sx+12, sy+58, 18, NEON_GREEN);
        }
    }

    // level editor draw
    else if (currentState == GameUIState::LevelEditor)
    {
        if (!levelEditor)
            levelEditor = make_unique<LevelEditor>(1488, 912);
        levelEditor->draw();
    }
    else
    {
        drawUI();
    }

    EndDrawing();
}

void Game::updateRoundSpawn(float deltaTime)
{
    if (!roundStarted)
        return;

    spawnTimer.countDown(deltaTime);

    if (spawnQueue.empty())
    {
        if (!units.empty())
            return;

        roundTimer.countDown(deltaTime);
        roundCompleted = true;

        if (roundTimer.timeSIsZero())
        {
            roundStarted = false;

            if (roundCount >= maxRounds)
            {
                gameWon = true;
                gameOver = true;
                currentState = GameUIState::GameOver;
            }
        }

        return;
    }

    if (spawnTimer.timeSIsZero())
    {
        EnemyType type = spawnQueue.front();
        spawnQueue.pop_front();

        addUnit(level.getRandomEnemySpawnerPosition(), type);

        float cooldown = 1.0f;
        switch (type)
        {
        case EnemyType::fast:
            cooldown = 0.1f;
            break;
        case EnemyType::basic:
            cooldown = 0.2f;
            break;
        case EnemyType::tank:
            cooldown = 1.5f;
            break;
        case EnemyType::physics:
            cooldown = 0.1f;
            break;
        case EnemyType::finalBoss:
            cooldown = 5.0f;
        }

        cooldown *= GetDifficultyTuning(selectedDifficulty).spawnCooldownMultiplier;
        spawnTimer.setTo(max(0.05f, cooldown));
    }
}

void Game::addTower(Vector2 mousePosition)
{
    Vector2 position = {(int)mousePosition.x, (int)mousePosition.y};
    if (level.isTileWall((int)position.x, (int)position.y))
    {
        for (auto &tower : towers)
        {
            if (tower->checkIfOnTile((int)position.x, (int)position.y))
            {
                // cout << "Tower already exists at position: (" << position.x << ", " << position.y << ")" << endl;
                return;
            }
        }
        TowerType type = nextTowerType;

        int towerCost = 0;

        switch (type)
        {
        case TowerType::sniper:
            towerCost = 400;
            break;

        case TowerType::basic:
            towerCost = 200;
            break;
        case TowerType::cannon:
            towerCost = 1000;
            break;
        }

        if (money >= towerCost)
        {
            auto newTower = make_shared<Tower>(position, type);
            newTower->addSpentCost(towerCost);
            towers.push_back(newTower);
            money -= towerCost;
        }
        else
        {
            showMoneyWarning = true;
            moneyWarningTimer.resetToMax();
        }
    }
}

void Game::removeTower(Vector2 mousePostion)
{
    // level.setTileWall((int)mousePostion.x, (int)mousePostion.y, false);
    for (auto it = towers.begin(); it != towers.end();)
    {
        if ((*it)->checkIfOnTile((int)mousePostion.x, (int)mousePostion.y))
        {
            // it = towers.erase(it);
            // cout << "Tower removed at position: (" << mousePostion.x << ", " << mousePostion.y << ")" << endl;

            int spent = (*it)->getTotalSpent();
            int refund = (int)(0.6f * spent);
            money += refund;

            it = towers.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Game::upgradeTower(Vector2 mousePosition)
{
    for (auto &tower : towers)
    {
        if (tower->checkIfOnTile((int)mousePosition.x, (int)mousePosition.y))
        {
            // tower->upgrade();
            // cout << "Tower upgraded!" << endl;
            // break;
            if (tower->isMaxLevel())
            {
                return; // returns if max level (3)
            }

            int upgradeCost = calculateUpgradeCost(tower);

            bool towerUpgraded = false;

            if (money >= upgradeCost)
            {
                tower->upgrade();
                money -= upgradeCost;
                tower->addSpentCost(upgradeCost);
            }
            else
            {
                showMoneyWarning = true;
                moneyWarningTimer.resetToMax();
            }

            break;
        }
    }
}

void Game::newRound()
{
    if (roundCount >= maxRounds)
        return;

    const DifficultyTuning &tuning = GetDifficultyTuning(selectedDifficulty);

    // Save last round's stats before starting the new round
    if (roundCount > 0) {
        lastRoundKills       = roundKills;
        lastRoundMoneyEarned = roundMoneyEarned;
        showRoundStats       = true;
        roundStatsTimer.resetToMax();
    }
    roundKills       = 0;
    roundMoneyEarned = 0;

    roundStarted = true;
    roundTimer.resetToMax();
    roundCompleted = false;

    roundCount++;

    if (roundCount == 1)
    {
        // First round: set everything to its base values, no income added yet
        money = baseMoney;
        baseIncome = tuning.startingIncome;
        targetHealth = baseHealth;
    }
    else
    {
        // Subsequent rounds: grant income and grow it for the next round
        money += baseIncome;
        roundMoneyEarned += baseIncome; // count income toward this round's earnings
        baseIncome += incomeIncrement;
    }

    // setup enemy spawn
    spawnQueue.clear();

    int fastCount = 2 + roundCount * 3;
    int basicCount = 6 + roundCount * 4;
    int tankCount = max(0, roundCount / 2);
    int physicsCount = tuning.physicsPerRound + roundCount / 3;
    int finalBossCount = 0;

    if (roundCount >= 4)
    {
        fastCount += 4;
        basicCount += 5;
    }

    if (roundCount >= 7)
    {
        fastCount += 6;
        basicCount += 8;
        tankCount += 2;
    }

    if (roundCount % 3 == 0)
        fastCount += 3;
    if (roundCount % 4 == 0)
        tankCount += 2;

    fastCount = ScaleEnemyCount(fastCount, tuning.fastScale);
    basicCount = ScaleEnemyCount(basicCount, tuning.basicScale);
    tankCount = ScaleEnemyCount(tankCount, tuning.tankScale);

    if (roundCount == maxRounds)
    {
        fastCount += ScaleEnemyCount(18, tuning.fastScale);
        basicCount += ScaleEnemyCount(16, tuning.basicScale);
        tankCount += ScaleEnemyCount(6, tuning.tankScale);
        physicsCount += tuning.physicsPerRound + 2;
        finalBossCount = 1;
    }

    spawnFastCount = fastCount;
    spawnBasicCount = basicCount;
    spawnTankCount = tankCount;
    spawnfinalBossCount = finalBossCount;
    spawnUnitCount = fastCount + basicCount + tankCount + physicsCount + finalBossCount;

    for (int i = 0; i < fastCount; ++i)
        spawnQueue.push_back(EnemyType::fast);
    for (int i = 0; i < basicCount; ++i)
        spawnQueue.push_back(EnemyType::basic);
    for (int i = 0; i < tankCount; ++i)
        spawnQueue.push_back(EnemyType::tank);
    for (int i = 0; i < physicsCount; i++)
    {
        spawnQueue.push_back(EnemyType::physics);
    }
    for (int i = 0; i < finalBossCount; i++)
    {
        spawnQueue.push_back(EnemyType::finalBoss);
    }

    {
        static std::mt19937 rng(std::random_device{}());
        std::shuffle(spawnQueue.begin(), spawnQueue.end(), rng);
    }

    spawnTimer.setTo(0);
}

// void Game::updateUnit(float deltaTime)
// {
//     auto it = units.begin();
//     while (it != units.end())
//     {
//         if ((*it) != nullptr)
//         {
//             (*it)->update(deltaTime, level, units);
//             if ((*it)->getIsAlive() == false)
//             {
//                 Vector2 unitPosition = (*it)->getPosition();
//                 Vector2 targetPosition = level.getTargetPosition();

//                 if ((*it)->getIsReached() && !(*it)->hasDamagedTarget)
//                 {

//                     (*it)->hasDamagedTarget = true;

//                     if((*it)->getEnemyType() == EnemyType::fast) cout << "fast";
//                     if((*it)->getEnemyType() == EnemyType::basic) cout << "basic";
//                     if((*it)->getEnemyType() == EnemyType::tank) cout << "tank";
//                     cout << " pre: " << targetHealth << " aft: ";

//                     targetHealth -= 1; // reduce health
//                     cout << targetHealth << endl;

//                     if (targetHealth <= 0)
//                     {
//                         targetHealth = 0;
//                         gameOver = true;
//                         currentState = GameUIState::GameOver;
//                         cout << "GAME OVER! Target health reached 0." << endl;
//                     }
//                 }
//                 else
//                 {
//                     switch ((*it)->getEnemyType())
//                     {
//                     case EnemyType::basic:
//                         money += 30;
//                         break;
//                     case EnemyType::fast:
//                         money += 20;
//                         break;
//                     case EnemyType::tank:
//                         money += 80;
//                         break;
//                     }
//                 }

//                 it = units.erase(it);
//                 continue;
//             }
//         }
//         ++it;
//     }
// }

void Game::updateUnit(float deltaTime)
{
    if (gameOver)
        return;

    auto it = units.begin();
    while (it != units.end())
    {
        auto &unit = *it;
        if (!unit)
        {
            ++it;
            continue;
        }

        unit->update(deltaTime, level, units);

        float dist = Vector2Distance(unit->getPosition(), level.getTargetPosition());

        if (dist < 1.5f) // Reached target tile
        {
            if (!unit->hasDamagedTarget)
            {
                unit->hasDamagedTarget = true;
                targetHealth -= unit->getCurrentHealth();

                if (targetHealth <= 0)
                {
                    targetHealth = 0;
                    gameOver = true;
                    currentState = GameUIState::GameOver;
                }
            }

            it = units.erase(it);
            continue;
        }

        if (!unit->getIsAlive())
        {
            int reward = getEnemyReward(unit->getEnemyType());
            money += reward;
            roundKills++;
            roundMoneyEarned += reward;

            it = units.erase(it);
            continue;
        }

        ++it;
    }
}

void Game::updateProjectiles(float deltaTime, Sound& cannonExplosionSound)
{
    auto it = projectiles.begin();
    while (it != projectiles.end())
    {
        (*it).update(deltaTime, units, cannonExplosionSound);
        if ((*it).checkCollision())
        {
            it = projectiles.erase(it); // Remove projectile if it collided
        }
        else
            it++;
    }
}

void Game::update(float deltaTime)
{
    UpdateMusicStream(backgroundMusic);

    if (currentState == GameUIState::MainMenu || currentState == GameUIState::Controls || currentState == GameUIState::LevelSelect || currentState == GameUIState::DifficultySelect || currentState == GameUIState::GameOver || currentState == GameUIState::Paused || currentState == GameUIState::Settings)
    {
        updateMainMenu(deltaTime);
        return;
    }

    if (currentState == GameUIState::LevelEditor)
    {
        if (!levelEditor)
            levelEditor = make_unique<LevelEditor>(1488, 912);

        bool stillEditing = true;
        levelEditor->processInput(stillEditing);

        if (!stillEditing)
        {
            levelEditor->reset(); // Delete and fully reset the editor
            currentState = GameUIState::MainMenu;
        }
        return;
    }

    // Apply speed multiplier to all gameplay-logic timings
    float dt = deltaTime * (gameSpeed2x ? 2.0f : 1.0f);

    updateUnit(dt);

    for (auto &tower : towers)
    {
        tower->update(dt, units, projectiles, towerShootSound, level.getTargetPosition());
    }

    updateProjectiles(dt, cannonExplosionSound);
    updateRoundSpawn(dt);

    // update sell confirm
    if (sellConfirm)
    {
        sellConfirmTimer.countDown(dt);
        if (sellConfirmTimer.timeSIsZero())
        {
            sellConfirm = false;
            selectedTower = nullptr;
        }
    }

    // update money warning
    if (showMoneyWarning)
    {
        moneyWarningTimer.countDown(dt);
        if (moneyWarningTimer.timeSIsZero())
            showMoneyWarning = false;
    }

    // update round stats display timer
    if (showRoundStats)
    {
        roundStatsTimer.countDown(dt);
        if (roundStatsTimer.timeSIsZero())
            showRoundStats = false;
    }

    // update click lock / double-click timers (real time feels better)
    clickLockTimer.countDown(deltaTime);
    doubleClickTimer.countDown(deltaTime);
}

void Game::drawUI()
{
    // if (currentState == GameUIState::Playing || currentState == GameUIState::Paused)
    // {
    // }

    if (currentState == GameUIState::MainMenu)
    {
        if (mainMenuBackground[mainMenuCurrentFrame].id != 0)
            if (mainMenuBackground[mainMenuCurrentFrame].id != 0)
            DrawTexture(mainMenuBackground[mainMenuCurrentFrame], 0, 0, WHITE);

        DrawText("CG DEFENSE", 620, 180, 40, DARKGRAY);
        DrawText(TextFormat("Current difficulty: %s", getDifficultyLabel()), 560, 240, 28, getDifficultyColor());

        DrawNeonButton(startBtn, "Start Game", SKYBLUE, NEON_PINK, WHITE);
        DrawNeonButton(levelSelectBtn, "Select Level", SKYBLUE, NEON_PINK, WHITE);
        DrawNeonButton(controlsBtn, "Controls", SKYBLUE, NEON_PINK, WHITE);
        DrawNeonButton(quitBtn, "Quit", SKYBLUE, NEON_PINK, WHITE);
        DrawNeonButton(levelEditorBtn, "Level Editor", SKYBLUE, NEON_PINK, WHITE);
        DrawNeonButton(settingsBtn, "Settings", SKYBLUE, NEON_PINK, WHITE);
    }
    else if (currentState == GameUIState::Controls)
    {
        if (mainMenuBackground[mainMenuCurrentFrame].id != 0)
            DrawTexture(mainMenuBackground[mainMenuCurrentFrame], 0, 0, WHITE);

        DrawText("Controls:", 100, 100, 50, NEON_YELLOW);
        DrawText("Difficulty changes starting credits, base HP, rewards, and wave pressure.", 120, 145, 24, NEON_WHITE);

        DrawText("Base Tower Price: 200", 120, 180, 30, NEON_WHITE);
        DrawText("Upgrade to Level 2: 100", 120, 210, 30, NEON_WHITE);
        DrawText("Upgrade to Level 3: 150", 120, 240, 30, NEON_WHITE);
        DrawText("Special Ability -> Targets closest enemy and High ", 125, 270, 30, NEON_WHITE);

        DrawText("Snipper Tower Price: 500", 120, 310, 30, NEON_WHITE);
        DrawText("Upgrade to Level 2: 1000", 120, 340, 30, NEON_WHITE);
        DrawText("Upgrade to Level 3: 1500", 120, 370, 30, NEON_WHITE);
        DrawText("Special Ability -> High Damage, Dynamic Threshold(targets enemy with higher health if level >= 2)", 125, 400, 30, NEON_WHITE);

        DrawText("Cannon Tower Price: 1000", 120, 440, 30, NEON_WHITE);
        DrawText("Upgrade to Level 2: 3000", 120, 470, 30, NEON_WHITE);
        DrawText("Upgrade to Level 3: 5000", 120, 500, 30, NEON_WHITE);
        DrawText("Special Ability -> AOE Effect(deals area damage of a specific range)", 125, 530, 30, NEON_WHITE);

        // DrawRectangleRec(backBtn, LIGHTGRAY);
        // DrawText("Back", backBtn.x + 10, backBtn.y + 10, 20, BLACK);

        DrawNeonButton(backBtn, "Back", SKYBLUE, NEON_PINK, WHITE);
    }
    else if (currentState == GameUIState::LevelSelect)
    {
        if (mainMenuBackground[mainMenuCurrentFrame].id != 0)
            DrawTexture(mainMenuBackground[mainMenuCurrentFrame], 0, 0, WHITE);

        DrawText("Select Level", GetScreenWidth() / 2 - MeasureText("Select Level", 40) / 2 + 5, 80, 40, DARKGRAY);
        DrawText(TextFormat("Difficulty: %s", getDifficultyLabel()), 630, 135, 28, getDifficultyColor());
        DrawText("Choose a map, then pick Easy, Medium, or Hard.", 470, 170, 24, NEON_WHITE);

        for (int i = 0; i < (int)allLevels.size(); ++i)
        {
            Rectangle levelBtn = {600, 200 + i * 60, 300, 50};
            string label = "Level " + to_string(i + 1);
            DrawNeonButton(levelBtn, label.c_str(), SKYBLUE, NEON_PINK, WHITE);

            // Show best round reached for this level
            int best = (i < MAX_SCORE_LEVELS) ? highScores[i] : 0;
            if (best > 0)
                DrawText(TextFormat("Best: %d / %d", best, maxRounds),
                         (int)(levelBtn.x + levelBtn.width + 12),
                         (int)(levelBtn.y + 14), 18,
                         best >= maxRounds ? NEON_GREEN : NEON_YELLOW);
        }

        // DrawRectangleRec(backBtn, LIGHTGRAY);
        // DrawText("Back", backBtn.x + 10, backBtn.y + 10, 20, BLACK);

        DrawNeonButton(backBtn, "Back", SKYBLUE, NEON_PINK, WHITE);
    }
    else if (currentState == GameUIState::DifficultySelect)
    {
        if (mainMenuBackground[mainMenuCurrentFrame].id != 0)
            DrawTexture(mainMenuBackground[mainMenuCurrentFrame], 0, 0, WHITE);

        DrawText("Select Difficulty", 555, 140, 42, NEON_YELLOW);
        DrawText(TextFormat("Level %d", selectLevelIndex >= 0 ? selectLevelIndex + 1 : getDefaultLevelIndex() + 1), 690, 200, 28, NEON_WHITE);
        DrawText("Easy gives more room to build. Hard tightens resources and speeds up waves.", 330, 235, 24, NEON_WHITE);

        DrawNeonButton(easyDifficultyBtn, "Easy", GREEN, NEON_GREEN, WHITE);
        DrawText("More credits, more base HP, slower enemy pressure", 560, 330, 22, NEON_WHITE);

        DrawNeonButton(mediumDifficultyBtn, "Medium", ORANGE, NEON_YELLOW, WHITE);
        DrawText("Balanced progression with standard payouts and pacing", 560, 430, 22, NEON_WHITE);

        DrawNeonButton(hardDifficultyBtn, "Hard", RED, NEON_RED, WHITE);
        DrawText("Fewer credits, faster spawns, denser mixed waves", 560, 530, 22, NEON_WHITE);

        DrawNeonButton(backBtn, "Back", SKYBLUE, NEON_PINK, WHITE);
    }
    else if (currentState == GameUIState::GameOver)
    {
        if (mainMenuBackground[mainMenuCurrentFrame].id != 0)
            DrawTexture(mainMenuBackground[mainMenuCurrentFrame], 0, 0, WHITE);

        if (gameWon)
            DrawText("🎉 YOU WON! 🎉", 600, 200, 50, DARKGREEN);
        else
            DrawText("Still waiting for Re-admission, huh!?", 300, 200, 50, NEON_RED);
            

        DrawText(TextFormat("Difficulty: %s", getDifficultyLabel()), 600, 250, 28, getDifficultyColor());

        // DrawRectangleRec(gameOverMainMenuBtn, LIGHTGRAY);
        // DrawText("Main Menu", gameOverMainMenuBtn.x + 20, gameOverMainMenuBtn.y + 15, 20, BLACK);

        // DrawRectangleRec(gameOverLevelSelectBtn, LIGHTGRAY);
        // DrawText("Level Select", gameOverLevelSelectBtn.x + 20, gameOverLevelSelectBtn.y + 15, 20, BLACK);

        // DrawRectangleRec(gameOverRestartBtn, LIGHTGRAY);
        // DrawText("Restart Level", gameOverRestartBtn.x + 20, gameOverRestartBtn.y + 15, 20, BLACK);

        DrawNeonButton(gameOverMainMenuBtn, "Main Menu", SKYBLUE, NEON_PINK, WHITE);
        DrawNeonButton(gameOverLevelSelectBtn, "Level Select", SKYBLUE, NEON_PINK, WHITE);
        DrawNeonButton(gameOverRestartBtn, "Restart Level", SKYBLUE, NEON_PINK, WHITE);
    }
    else if (currentState == GameUIState::Paused)
    {
        if (mainMenuBackground[mainMenuCurrentFrame].id != 0)
            DrawTexture(mainMenuBackground[mainMenuCurrentFrame], 0, 0, WHITE);

        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.2f));
        DrawText("Game Paused", 650, 150, 48, NEON_YELLOW);
        DrawNeonButton(resumeBtn, "Resume", SKYBLUE, NEON_PINK, WHITE);

        DrawNeonButton(instantGameOverBtn, "Game Over", SKYBLUE, NEON_PINK, WHITE);

        // DrawText("Audio Settings", 640, 180, 36, NEON_PURPLE);

        // Music volume bar
        DrawText("Music Volume", 550, 280, 24, NEON_PURPLE);
        Rectangle musicBar = {550, 310, 400, 15};
        DrawRectangleRec(musicBar, Fade(NEON_LAVENDER, 0.2f));
        DrawRectangleLinesEx(musicBar, 2, NEON_PURPLE);
        DrawRectangle(musicBar.x, musicBar.y, musicVolume * musicBar.width, musicBar.height, soundbarColor);

        // Click to set volume
        if (CheckCollisionPointRec(GetMousePosition(), musicBar) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            float mouseX = GetMouseX();
            musicVolume = (mouseX - musicBar.x) / musicBar.width;
            musicVolume = Clamp(musicVolume, 0.0f, 1.0f);
            SetMusicVolume(backgroundMusic, musicVolume);
        }

        // Sound volume bar
        DrawText("Sound Volume", 550, 360, 24, NEON_PURPLE);
        Rectangle soundBar = {550, 390, 400, 15};
        DrawRectangleLinesEx(soundBar, 2, NEON_PURPLE);
        DrawRectangleRec(soundBar, Fade(NEON_LAVENDER, 0.2f));
        DrawRectangle(soundBar.x, soundBar.y, soundVolume * soundBar.width, soundBar.height, soundbarColor);

        if (CheckCollisionPointRec(GetMousePosition(), soundBar) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            float mouseX = GetMouseX();
            soundVolume = (mouseX - soundBar.x) / soundBar.width;
            soundVolume = Clamp(soundVolume, 0.0f, 1.0f);
            SetSoundVolume(towerShootSound, soundVolume);
            SetSoundVolume(cannonExplosionSound, soundVolume);
            SetSoundVolume(buttonHoverSound, soundVolume);
            // SetSoundVolume(enemyMoveSound, soundVolume);
        }
    }
    else if (currentState == GameUIState::Settings)
    {
        if (mainMenuBackground[mainMenuCurrentFrame].id != 0)
            DrawTexture(mainMenuBackground[mainMenuCurrentFrame], 0, 0, WHITE);

        DrawText("Audio Settings", 640, 180, 36, NEON_YELLOW);

        // Music volume bar
        DrawText("Music Volume", 550, 280, 24, CORNSILK);
        Rectangle musicBar = {550, 310, 400, 20};
        DrawRectangleLinesEx(musicBar, 2, NEON_PURPLE);
        DrawRectangleRec(musicBar, Fade(NEON_LAVENDER, 0.2f));
        DrawRectangle(musicBar.x, musicBar.y, musicVolume * musicBar.width, musicBar.height, soundbarColor);

        // Click to set volume
        if (CheckCollisionPointRec(GetMousePosition(), musicBar) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            float mouseX = GetMouseX();
            musicVolume = (mouseX - musicBar.x) / musicBar.width;
            musicVolume = Clamp(musicVolume, 0.0f, 1.0f);
            SetMusicVolume(backgroundMusic, musicVolume);
        }

        // Sound volume bar
        DrawText("Sound Volume", 550, 360, 24, CORNSILK);
        Rectangle soundBar = {550, 390, 400, 15};
        DrawRectangleRec(soundBar, Fade(NEON_LAVENDER, 0.2f));
        DrawRectangleLinesEx(soundBar, 2, NEON_PURPLE);
        DrawRectangle(soundBar.x, soundBar.y, soundVolume * soundBar.width, soundBar.height, soundbarColor);

        if (CheckCollisionPointRec(GetMousePosition(), soundBar) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            float mouseX = GetMouseX();
            soundVolume = (mouseX - soundBar.x) / soundBar.width;
            soundVolume = Clamp(soundVolume, 0.0f, 1.0f);
            SetSoundVolume(towerShootSound, soundVolume);
            SetSoundVolume(cannonExplosionSound, soundVolume);
            SetSoundVolume(buttonHoverSound, soundVolume);
            // SetSoundVolume(enemyMoveSound, soundVolume);
        }

        // Back button
        Rectangle backBtnRect = {600, 480, 300, 50};
        DrawNeonButton(backBtnRect, "Back", SKYBLUE, NEON_PINK, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), backBtnRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            currentState = GameUIState::MainMenu;
        }
    }
}

int Game::calculateUpgradeCost(shared_ptr<Tower> t)
{

    int upgradeCost = 0;

    TowerType type = t->getTowerType();

    switch (type)
    {
    case TowerType::basic:
        upgradeCost = 100 + 50 * (t->getTowerLevel() - 1);
        break;
    case TowerType::sniper:
    {
        int towerLevel = t->getTowerLevel();
        switch (towerLevel)
        {
        case 1:
            upgradeCost = 1000;
            break;
        case 2:
            upgradeCost = 500;
            break;
        }
        break;
    }
    case TowerType::cannon:
    {
        int towerLevelCannon = t->getTowerLevel();
        switch (towerLevelCannon)
        {
        case 1:
            upgradeCost = 1000;
            break;
        case 2:
            upgradeCost = 5000;
            break;
        }
        break;
    }
    }

    return upgradeCost;
}

void Game::towerSelectionAndDoubleClickUpdate(Vector2 mouse)
{
    // tower selection
    Vector2 tilePos = {mouse.x / tileSize, mouse.y / tileSize};
    bool foundTower = false;

    for (auto &tower : towers)
    {
        if (tower->checkIfOnTile((int)tilePos.x, (int)tilePos.y))
        {
            selectedTower = tower;
            clickLockTimer.resetToMax();
            Vector2 clickedTile = {(int)tilePos.x, (int)tilePos.y};
            if (clickedTile.x == lastClickedTile.x && clickedTile.y == lastClickedTile.y &&
                !doubleClickTimer.timeSIsZero())
            {

                upgradeTower(clickedTile);
                doubleClickTimer.resetToZero();
            }
            else
            {
                // First click
                lastClickedTile = clickedTile;
                doubleClickTimer.resetToMax();
            }
            foundTower = true;
            break;
        }
    }

    if (!foundTower)
    {
        selectedTower = nullptr;
        sellConfirm = false;
        clickLockTimer.resetToMax();
    }
}

void Game::selectedTowerDisplay()
{
    // Highlight selected tile
    Vector2 pos = selectedTower->getPosition();
    DrawRectangleLinesEx({pos.x * tileSize, pos.y * tileSize, (float)tileSize, (float)tileSize},
                         2, NEON_PINK);

    // Panel — anchored to bottom-right, above the tower buy buttons
    const float panelX = 1295.0f, panelY = 440.0f;
    const float panelW = 188.0f,  panelH = 240.0f;

    DrawRectangleRounded({panelX, panelY, panelW, panelH}, 0.12f, 12, Fade(BLACK, 0.78f));
    DrawRectangleRoundedLines({panelX, panelY, panelW, panelH}, 0.12f, 12, Fade(NEON_BLUE, 0.6f));

    // Tower name
    const char* tname = (selectedTower->getTowerType() == TowerType::basic)   ? "BASIC TOWER"
                      : (selectedTower->getTowerType() == TowerType::sniper)  ? "SNIPER TOWER"
                                                                               : "CANNON TOWER";
    DrawText(tname, (int)(panelX + 10), (int)(panelY + 10), 18, NEON_YELLOW);

    int lv     = selectedTower->getTowerLevel();
    int cost   = selectedTower->isMaxLevel() ? 0 : calculateUpgradeCost(selectedTower);
    int refund = (int)(selectedTower->getTotalSpent() * 0.6f);

    DrawText(TextFormat("Level : %d / 3",     lv),          (int)(panelX+10), (int)(panelY+36),  18, WHITE);
    DrawText(TextFormat("Mode  : %s", selectedTower->getTargetingLabel()),
                                                             (int)(panelX+10), (int)(panelY+58),  18, NEON_BLUE);
    DrawText(TextFormat("Refund: $%d",         refund),      (int)(panelX+10), (int)(panelY+80),  18, NEON_GREEN);

    if (cost > 0)
        DrawText(TextFormat("Upgrade: $%d",   cost),         (int)(panelX+10), (int)(panelY+102), 18, NEON_YELLOW);
    else
        DrawText("MAX LEVEL",                                 (int)(panelX+10), (int)(panelY+102), 18, NEON_GREEN);

    DrawText("[T] cycle target mode",                         (int)(panelX+10), (int)(panelY+124), 14, GRAY);

    // Upgrade button (greyed out at max level)
    towerUpgradeBtnRect = {panelX + 8, panelY + 146, panelW - 16, 36};
    if (!selectedTower->isMaxLevel()) {
        bool canAfford = money >= cost;
        DrawNeonButton(towerUpgradeBtnRect, TextFormat("Upgrade  $%d", cost),
                       canAfford ? Fade(NEON_GREEN, 0.3f) : Fade(GRAY, 0.2f),
                       canAfford ? NEON_GREEN : GRAY, WHITE);
    } else {
        DrawRectangleRounded(towerUpgradeBtnRect, 0.35f, 8, Fade(GRAY, 0.15f));
        DrawText("Max Level", (int)(panelX+40), (int)(panelY+156), 18, GRAY);
    }

    // Sell button
    towerSellBtnRect = {panelX + 8, panelY + 190, panelW - 16, 36};
    if (sellConfirm)
        DrawNeonButton(towerSellBtnRect, TextFormat("Confirm Sell $%d", refund), RED, NEON_RED, WHITE);
    else
        DrawNeonButton(towerSellBtnRect, TextFormat("Sell  $%d", refund), Fade(ORANGE, 0.3f), ORANGE, WHITE);
}

void Game::updateMainMenu(float deltaTime)
{
    mainMenuAnimationTimer.countDown(deltaTime);
    if (mainMenuAnimationTimer.timeSIsZero())
    {
        mainMenuAnimationTimer.resetToMax();
        mainMenuCurrentFrame = (mainMenuCurrentFrame + 1) % MENU_FRAME_COUNT;
    }

    // Lazy-load: ensure the current frame and the next 2 are ready
    for (int offset = 0; offset <= 2; ++offset)
    {
        int idx = (mainMenuCurrentFrame + offset) % MENU_FRAME_COUNT;
        if (mainMenuBackground[idx].id == 0)
        {
            string numstr = to_string(idx + 1);
            while (numstr.length() < 5) numstr = "0" + numstr;
            string filename = "mainMenuAnimation2/" + numstr + ".png";
            Texture2D *tex = TextureLoader::LoadTextureFromFile(filename);
            if (tex) mainMenuBackground[idx] = *tex;
        }
    }
}

void Game::DrawNeonButton(Rectangle rect, const char *label, Color normalColor, Color hoverColor, Color clickColor)
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

// ── High Score ───────────────────────────────────────────────────────────────

void Game::loadHighScores()
{
    ifstream f("highscores.txt");
    if (!f.is_open()) return;
    for (int i = 0; i < MAX_SCORE_LEVELS; i++) {
        if (!(f >> highScores[i])) break;
    }
}

void Game::saveHighScores()
{
    ofstream f("highscores.txt");
    for (int i = 0; i < MAX_SCORE_LEVELS; i++)
        f << highScores[i] << "\n";
}

// ── Wave preview ─────────────────────────────────────────────────────────────

Game::WaveComposition Game::computeWaveComposition(int roundNum) const
{
    const DifficultyTuning &tuning = GetDifficultyTuning(selectedDifficulty);

    int fastCount    = 2 + roundNum * 3;
    int basicCount   = 6 + roundNum * 4;
    int tankCount    = max(0, roundNum / 2);
    int physicsCount = tuning.physicsPerRound + roundNum / 3;
    int bossCount    = 0;

    if (roundNum >= 4) { fastCount += 4; basicCount += 5; }
    if (roundNum >= 7) { fastCount += 6; basicCount += 8; tankCount += 2; }
    if (roundNum % 3 == 0) fastCount += 3;
    if (roundNum % 4 == 0) tankCount += 2;

    fastCount  = ScaleEnemyCount(fastCount,  tuning.fastScale);
    basicCount = ScaleEnemyCount(basicCount, tuning.basicScale);
    tankCount  = ScaleEnemyCount(tankCount,  tuning.tankScale);

    if (roundNum == maxRounds) {
        fastCount    += ScaleEnemyCount(18, tuning.fastScale);
        basicCount   += ScaleEnemyCount(16, tuning.basicScale);
        tankCount    += ScaleEnemyCount(6,  tuning.tankScale);
        physicsCount += tuning.physicsPerRound + 2;
        bossCount     = 1;
    }
    return {basicCount, fastCount, tankCount, physicsCount, bossCount};
}

void Game::drawWavePreview()
{
    int nextRound = roundCount + 1;
    if (nextRound > maxRounds || roundStarted) return;

    WaveComposition wc = computeWaveComposition(nextRound);

    const int panelW = 260, panelH = 185;
    const int panelX = 1488 / 2 - panelW / 2;
    const int panelY = 912 / 2 - 60;

    DrawRectangleRounded({(float)panelX, (float)panelY, (float)panelW, (float)panelH},
                         0.15f, 16, Fade(BLACK, 0.72f));
    DrawRectangleRoundedLines({(float)panelX, (float)panelY, (float)panelW, (float)panelH},
                              0.15f, 16, Fade(NEON_BLUE, 0.7f));

    DrawText(TextFormat("Next Wave — Round %d / %d", nextRound, maxRounds),
             panelX + 12, panelY + 10, 18, NEON_YELLOW);

    int lineY = panelY + 38;
    const int lineH = 26;
    auto row = [&](const char* label, int count, Color col) {
        if (count <= 0) return;
        DrawText(label,                       panelX + 16, lineY, 20, col);
        DrawText(TextFormat("x%d", count),    panelX + 180, lineY, 20, WHITE);
        lineY += lineH;
    };

    row("Basic",        wc.basic,   WHITE);
    row("Fast",         wc.fast,    {255, 240, 80, 255});
    row("Tank",         wc.tank,    {220, 80,  80, 255});
    row("Physics",      wc.physics, {80,  220, 220, 255});
    if (wc.boss > 0)
        DrawText("*** FINAL BOSS ***", panelX + 16, lineY, 20, {200, 50, 255, 255});

    DrawText("[ SPACE ] to start",
             panelX + 50, panelY + panelH - 26, 18, NEON_GREEN);
}

void Game::loadMainMenu()
{
    // Load only the short loading-animation strip (22 frames) upfront — fast
    for (int i = 0; i < 22; i++)
    {
        string numstr = to_string(i + 1);
        while (numstr.length() < 5) numstr = "0" + numstr;
        string filename = "loadingAnimation2/" + numstr + ".png";
        Texture2D *tex = TextureLoader::LoadTextureFromFile(filename);
        if (tex) loadingMainMenuAnimation[i] = *tex;
    }
    // mainMenuBackground frames are loaded on-demand in updateMainMenu()
}
