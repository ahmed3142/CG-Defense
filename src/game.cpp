#include "game.h"
#include "textureloader.h"
#include "tower.h"
#include "level.h"
#include "projectile.h"

using namespace std;

#define rep(i, n) for (int i = 0; i < (n); ++i)
#define rrep(i, a, b) for (int i = (a); i < (b); ++i)

Game::Game(int windowWidth, int windowHeight, const LevelData &data)
    : level(windowWidth / tileSize, windowHeight / tileSize),
      PlacementModeCurrent(PlacementMode::tower),
      spawnTimer(2.0f), roundTimer(2.0f),
      money(baseMoney)
{
    // audio control
    InitAudioDevice();

    backgroundMusic = LoadMusicStream("E:/1-2 Courses/Structured Programming LAB/projectDemo-ProjectDefense/src/assets/Audios/backgroundMusic.mp3");
    if (backgroundMusic.stream.buffer == nullptr)
    {
        std::cerr << "Error: Failed to load background music!" << std::endl;
        CloseAudioDevice();
        exit(1); // Exit the game with error code
    }
   // cannonBlastSound = LoadSound("assets/audio/cannon_blast.wav");
    // enemyMoveSound = LoadSound("assets/audio/enemy_move.wav");
    buttonHoverSound = LoadSound("E:/1-2 Courses/Structured Programming LAB/projectDemo-ProjectDefense/src/assets/Audios/buttonHover.wav");
    towerShootSound = LoadSound("E:/1-2 Courses/Structured Programming LAB/projectDemo-ProjectDefense/src/assets/Audios/shootingTower.mp3");
    cannonExplosionSound = LoadSound("E:/1-2 Courses/Structured Programming LAB/projectDemo-ProjectDefense/src/assets/Audios/cannonBlast.mp3");

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

    const float deltaTime = 1.0f / 60.0f;
    float accumulator = 0.0f;

    // loading main menu background
    loadMainMenu(deltaTime);

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

void Game::processEvents(bool &running)
{ // for every frame
    bool mouseDownThisFrame = (mouseDownStatus == 0);
    bool mouseClick = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    Vector2 mouse = GetMousePosition();

    if (currentState == GameUIState::MainMenu)
    {
        if (mouseClick && CheckCollisionPointRec(mouse, startBtn))
        {
            selectLevelIndex = 3;
            level.loadFromData(allLevels[selectLevelIndex]);
            currentState = GameUIState::Playing;
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

    // CONTROLS or LEVELSELECT BACK BUTTON
    if ((currentState == GameUIState::Controls || currentState == GameUIState::LevelSelect) && mouseClick && CheckCollisionPointRec(mouse, backBtn))
    {
        currentState = GameUIState::MainMenu;
        return;
    }

    // PLAYING or PAUSED BACK BUTTON
    if ((currentState == GameUIState::Playing || currentState == GameUIState::Paused) && mouseClick && CheckCollisionPointRec(mouse, instantGameOverBtn))
    {
        gameOver = true;
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
                level.resetLevel();
                level.loadFromData(allLevels[i]);
                currentState = GameUIState::Playing;
                return;
            }
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
            currentState = GameUIState::GameOver;
        }
    }

    // GAMEOVER
    if (currentState == GameUIState::GameOver)
    {
        if (mouseClick && CheckCollisionPointRec(mouse, gameOverMainMenuBtn))
        {
            currentState = GameUIState::MainMenu;
            gameOver = false;
            gameWon = false;
            targetHealth = baseHealth;
            money = baseMoney;
            units.clear();
            towers.clear();
            selectedTower = nullptr;
            roundCount = 0;
        }
        else if (mouseClick && CheckCollisionPointRec(mouse, gameOverLevelSelectBtn))
        {
            currentState = GameUIState::LevelSelect;
            gameOver = false;
            gameWon = false;
            targetHealth = baseHealth;
            money = baseMoney;
            units.clear();
            towers.clear();
            selectedTower = nullptr;
            roundCount = 0;
        }
        else if (mouseClick && CheckCollisionPointRec(mouse, gameOverRestartBtn))
        {
            currentState = GameUIState::Playing;
            gameOver = false;
            gameWon = false;
            targetHealth = baseHealth;
            money = baseMoney;
            units.clear();
            towers.clear();
            selectedTower = nullptr;
            roundCount = 0;
            level.loadFromData(allLevels[selectLevelIndex]);
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

        cout << "Placement mode: wall" << endl;
        PlacementModeCurrent = PlacementMode::wall;
    }
    if (IsKeyPressed(KEY_TWO))
    {
        cout << "Placement mode: tower" << endl;
        PlacementModeCurrent = PlacementMode::tower;
    }
    if (IsKeyPressed(KEY_M))
    {
        cout << "Overlay toggled" << endl;
        overlayVisible = !overlayVisible;
    }
    if (IsKeyPressed(KEY_S))
    {
        // selling tower

        if (selectedTower != nullptr)
        {
            sellConfirm = true;
            sellConfirmTimer.resetToMax();
        }
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
    if (sellConfirm && selectedTower != nullptr && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
    {
        removeTower(selectedTower->getPosition());
        sellConfirm = false;
        selectedTower = nullptr;
    }

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

        // money and health
        char healthBuffer[50];
        std::snprintf(healthBuffer, sizeof(healthBuffer), "CG ~ %.2f", targetHealth / 100.0f);

        DrawText(("Credits: " + std::to_string(money)).c_str(), 30, 20, 30, NEON_BLUE);
        DrawText(healthBuffer, 30, 60, 30, NEON_BLUE);

        // round count and instant back button
        string roundText = "Round: " + to_string(roundCount) + " / 10";
        int textWidth = MeasureText(roundText.c_str(), 30);
        DrawText(roundText.c_str(), 1488 - textWidth - 30, 9, 30, NEON_BLUE);
        DrawNeonButton(instantGameOverBtn, "End Game", SKYBLUE, NEON_PINK, WHITE);

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
        roundTimer.countDown(deltaTime);
        roundCompleted = true;

        if (roundTimer.timeSIsZero())
        {
            roundStarted = false;

            if (roundCount >= maxRounds)
            {
                gameOver = true;
                currentState = GameUIState::GameOver;
                cout << "🎉 LEVEL COMPLETE! YOU WON 🎉" << endl;
            }
        }

        return;
    }

    // If it's time to spawn a unit
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

        spawnTimer.setTo(cooldown);
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

        bool towerAdded = false;

        if (money >= towerCost)
        {
            auto newTower = make_shared<Tower>(position, type);
            newTower->addSpentCost(towerCost);
            towers.push_back(newTower);
            money -= towerCost;
            towerAdded = true;
        }

        else
        {
            cout << "Not enough money" << endl;
        }

        // checking
        if (towerAdded)
        {
            if (type == TowerType::sniper)
            {
                cout << "Sniper tower added at position: (" << position.x << ", " << position.y << ")" << endl;
            }
            else
            {
                cout << "Basic tower added at position: (" << position.x << ", " << position.y << ")" << endl;
            }
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
            int refund = 0.6 * spent;
            money += refund;

            it = towers.erase(it);

            cout << "Tower sold. Refunded: " << refund << " | Money: " << money << endl;
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
                towerUpgraded = true;
                cout << "Tower Upgraded! Money Left: " << money << endl;
            }
            else
            {
                cout << "Not enough money to upgrade" << endl;
            }

            break;
        }
    }
}

void Game::newRound()
{
    if (roundCount >= maxRounds)
        return;

    roundStarted = true;
    roundTimer.resetToMax();
    roundCompleted = false;

    // income and round progression
    money += baseIncome;
    baseIncome += incomeIncrement;
    roundCount++;

    if (roundCount == 1)
    {
        money = baseMoney;
        baseIncome = 2000;
        targetHealth = baseHealth;
    }

    // setup enemy spawn
    spawnQueue.clear();

    int fastCount = 10 + roundCount * 200;
    int basicCount = 5 + roundCount * 150;
    int tankCount = 2 + roundCount * 10;
    int physicsCount = 10;
    int finalBossCount = 0;

    // fastCount = 0;
    // basicCount = 0;
    // tankCount = 0;

    if (roundCount == maxRounds)
    {
        fastCount = 6000;
        basicCount = 650;
        tankCount = 200;
        finalBossCount = 1;
    }

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

    std::random_shuffle(spawnQueue.begin(), spawnQueue.end());

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

                cout << "[DAMAGE] UnitID " << unit->unitId
                     << " | Addr: " << unit.get()
                     << " | Type: " << (int)unit->getEnemyType()
                     << " | Target HP: " << targetHealth << endl;

                if (targetHealth <= 0)
                {
                    targetHealth = 0;
                    gameOver = true;
                    currentState = GameUIState::GameOver;
                }
            }
            else
            {
                cout << "[SKIP] UnitID " << unit->unitId << " already damaged target\n";
            }

            it = units.erase(it);
            continue;
        }

        if (!unit->getIsAlive())
        {
            switch (unit->getEnemyType())
            {
            case EnemyType::basic:
                money += 10;
                break;
            case EnemyType::fast:
                money += 5;
                break;
            case EnemyType::tank:
                money += 50;
                break;
            }

            // cout << "[DEAD] UnitID " << unit->unitId << " died before reaching target\n";

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

    if (currentState == GameUIState::MainMenu || currentState == GameUIState::Controls || currentState == GameUIState::LevelSelect || currentState == GameUIState::GameOver || currentState == GameUIState::Paused || currentState == GameUIState::Settings)
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

    updateUnit(deltaTime); // update all units

    for (auto &tower : towers)
    { // tower update
        tower->update(deltaTime, units, projectiles, towerShootSound);
    }

    updateProjectiles(deltaTime, cannonExplosionSound); // update all projectiles
    updateRoundSpawn(deltaTime);

    // update sell confirm
    if (sellConfirm)
    {
        sellConfirmTimer.countDown(deltaTime);
        if (sellConfirmTimer.timeSIsZero())
        {
            sellConfirm = false;
            selectedTower = nullptr;
        }
    }

    // update click lock timer
    clickLockTimer.countDown(deltaTime);

    // update double click timer
    doubleClickTimer.countDown(deltaTime);
}

void Game::drawUI()
{
    // if (currentState == GameUIState::Playing || currentState == GameUIState::Paused)
    // {
    // }

    if (currentState == GameUIState::MainMenu)
    {
        DrawTexture(mainMenuBackground[mainMenuCurrentFrame], 0, 0, WHITE);

        DrawText("CG DEFENSE", 620, 180, 40, DARKGRAY);

        DrawNeonButton(startBtn, "Start Game", SKYBLUE, NEON_PINK, WHITE);
        DrawNeonButton(levelSelectBtn, "Select Level", SKYBLUE, NEON_PINK, WHITE);
        DrawNeonButton(controlsBtn, "Controls", SKYBLUE, NEON_PINK, WHITE);
        DrawNeonButton(quitBtn, "Quit", SKYBLUE, NEON_PINK, WHITE);
        DrawNeonButton(levelEditorBtn, "Level Editor", SKYBLUE, NEON_PINK, WHITE);
        DrawNeonButton(settingsBtn, "Settings", SKYBLUE, NEON_PINK, WHITE);
    }
    else if (currentState == GameUIState::Controls)
    {
        DrawTexture(mainMenuBackground[mainMenuCurrentFrame], 0, 0, WHITE);

        DrawText("Controls:", 100, 100, 50, NEON_YELLOW);

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
        DrawTexture(mainMenuBackground[mainMenuCurrentFrame], 0, 0, WHITE);

        DrawText("Select Level", GetScreenWidth() / 2 - MeasureText("Select Level", 40) / 2 + 5, 80, 40, DARKGRAY);

        for (int i = 0; i < (int)allLevels.size(); ++i)
        {

            Rectangle levelBtn = {600, 200 + i * 60, 300, 50};
            // DrawRectangleRec(levelBtn, LIGHTGRAY);
            string label = "Level " + to_string(i + 1);
            // DrawText(label.c_str(), levelBtn.x + 20, levelBtn.y + 15, 20, BLACK);
            DrawNeonButton(levelBtn, label.c_str(), SKYBLUE, NEON_PINK, WHITE);
        }

        // DrawRectangleRec(backBtn, LIGHTGRAY);
        // DrawText("Back", backBtn.x + 10, backBtn.y + 10, 20, BLACK);

        DrawNeonButton(backBtn, "Back", SKYBLUE, NEON_PINK, WHITE);
    }
    else if (currentState == GameUIState::GameOver)
    {
        DrawTexture(mainMenuBackground[mainMenuCurrentFrame], 0, 0, WHITE);

        if (gameWon)
            DrawText("🎉 YOU WON! 🎉", 600, 200, 50, DARKGREEN);
        else
            DrawText("Still waiting for Re-admission, huh!?", 300, 200, 50, NEON_RED);
            

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
                cout << "Tower upgraded via double-click!" << endl;
                doubleClickTimer.resetToZero(); // Reset to avoid multi-upgrade
            }
            else
            {
                // First click
                lastClickedTile = clickedTile;
                doubleClickTimer.resetToMax();
            }
            foundTower = true;
            cout << "Tower selected at (" << tilePos.x << ", " << tilePos.y << ")" << endl;
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
    // border mark of selected tower
    Vector2 pos = selectedTower->getPosition();
    DrawRectangleLinesEx({pos.x * tileSize, pos.y * tileSize,
                          (float)tileSize, (float)tileSize},
                         2, RED);
    int level = selectedTower->getTowerLevel();
    int cost = selectedTower->isMaxLevel() ? 0 : calculateUpgradeCost(selectedTower);
    int refund = selectedTower->getTotalSpent() * 0.6;

    std::string infoText = "Level: " + to_string(level) +
                           "  |  " + (cost == 0 ? "max level reached" : ("Upgrade: " + to_string(cost))) +
                           "  |  Refund: " + to_string(refund);

    // Screen position slightly above the hovered tower
    Vector2 center = {
        (selectedTower->getPosition().x + 0.5f) * tileSize,
        (selectedTower->getPosition().y - 0.3f) * tileSize};

    // text dimensions
    int textWidth = MeasureText(infoText.c_str(), 18);
    int textHeight = 20;
    Rectangle tooltipBox = {
        center.x - textWidth / 2.0f - 6,
        center.y - textHeight - 6,
        (float)textWidth + 12,
        (float)textHeight + 12};

    // background box and border
    DrawRectangleRec(tooltipBox, Fade(LIGHTGRAY, 0.9f));
    DrawRectangleLinesEx(tooltipBox, 1, DARKGRAY);

    // text inside the box
    DrawText(infoText.c_str(), tooltipBox.x + 6, tooltipBox.y + 6, 18, BLACK);

    // selling tooltip
    if (sellConfirm && selectedTower != nullptr)
    {
        DrawText("Right-click to SELL selected tower", 30, 90, 18, RED);
    }
}

void Game::updateMainMenu(float deltaTime)
{
    mainMenuAnimationTimer.countDown(deltaTime);
    if (mainMenuAnimationTimer.timeSIsZero())
    {
        mainMenuAnimationTimer.resetToMax();
        mainMenuCurrentFrame = (mainMenuCurrentFrame + 1) % 900; // Loop through backgrounds
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

void Game::loadMainMenu(float deltaTime)
{
    for (int i = 0; i < 22; i++)
    {
        string numstr = to_string(i + 1);
        while (numstr.length() < 5)
        {
            numstr = "0" + numstr;
        }
        string filename = "loadingAnimation2/" + numstr + ".png";
        loadingMainMenuAnimation[i] = *TextureLoader::LoadTextureFromFile(filename);
    }

    for (int i = 0; i < 900; ++i)
    {
        string numstr = to_string(i + 1);
        while (numstr.length() < 5)
        {
            numstr = "0" + numstr;
        }
        string filename = "mainMenuAnimation2/" + numstr + ".png";
        mainMenuBackground[i] = *TextureLoader::LoadTextureFromFile(filename);

        loadingMainMenuAnimationTimer.countDown(deltaTime);
        if (loadingMainMenuAnimationTimer.timeSIsZero())
        {
            loadingMainMenuAnimationTimer.resetToMax();
            loadingMainMenuAnimationCurrentFrame = (loadingMainMenuAnimationCurrentFrame + 1) % 22;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        // progress bar
        float progress = (float)(i + 1) / 900.0f;
        int barWidth = 400;
        int barHeight = 16;
        int progressBarWidth = (int)(barWidth * progress);

        DrawTexture(loadingMainMenuAnimation[loadingMainMenuAnimationCurrentFrame], 670, 240, WHITE);

        Rectangle progressBarRect = {550, 400, (float)barWidth, (float)barHeight};
        Rectangle progressFillRect = {progressBarRect.x, progressBarRect.y, (float)progressBarWidth, progressBarRect.height};

        DrawRectangleRounded(progressBarRect, 0.5f, 30, Fade(SKYBLUE, 0.2f));
        DrawRectangleRounded(progressFillRect, 0.5f, 30, NEON_BLUE);
        DrawText("Loading... Please wait", 600, 360, 30, DARKGRAY);

        EndDrawing();
    }
}