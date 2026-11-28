#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>

using namespace std;
using namespace sf;

// Window
const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 700;
const int FPS_LIMIT = 60;

// Bricks
const int ROWS = 8;
const int COLS = 10;
const float BRICK_WIDTH = 60;
const float BRICK_HEIGHT = 25;
const float BRICK_START_Y = 100;
const float BRICK_GAP = 2;

// Paddle
const float PADDLE_WIDTH = 100;
const float PADDLE_HEIGHT = 14;
const float PADDLE_SPEED = 8;
const float PADDLE_START_X = 250;
const float PADDLE_Y = 650;

// Ball
const float BALL_RADIUS = 6;
const float BALL_SPEED_X = 3;
const float BALL_SPEED_Y = -5;
const int MAX_BALLS = 3;

// Fire power-up
const float POWERUP_RADIUS = 10;
const float POWERUP_SPEED = 3;
const int POWERUP_CHANCE = 5;                  // 1 in 5 destroyed bricks
const int FIRE_DURATION = 10 * FPS_LIMIT;      // 10 seconds, counted in frames
const int FIRE_DAMAGE = 2;                     // normal hit = 1
const int BRICK_SCORE = 10;
const int FIRE_BRICK_SCORE = 15;

SoundBuffer brickBuffer;
SoundBuffer gameOverBuffer;
SoundBuffer powerUpBuffer;
SoundBuffer winBuffer;

Sound brickSound(brickBuffer);
Sound gameOverSound(gameOverBuffer);
Sound powerUpSound(powerUpBuffer);
Sound winSound(winBuffer);

struct Ball
{
    float x = 0;
    float y = 0;
    float dx = 0;
    float dy = 0;
    bool active = false;
};

struct GameState
{
    int score = 0;
    int bricks[ROWS][COLS] = {};

    float paddleX = PADDLE_START_X;

    Ball balls[MAX_BALLS];

    // Fire power-up
    bool powerUpExists = false;
    float powerUpX = 0;
    float powerUpY = 0;
    int fireTimer = 0;                         // frames left, 0 = no fire

    bool ballLaunched = false;
    bool gameStarted = false;
    bool paused = false;

    int lives = 3;

    bool gameOver = false;
    bool gameWon = false;

    int menuSelection = 0;
};

// Load level
bool loadLevel(GameState& game)
{
    ifstream file("level.txt");

    if (!file.is_open())
    {
        cout << "Could not open level.txt" << endl;
        return false;
    }

    int fileRows, fileCols;

    if (!(file >> fileRows >> fileCols) ||
        fileRows != ROWS || fileCols != COLS)
    {
        cout << "Invalid level dimensions" << endl;
        return false;
    }

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            int val;

            if (!(file >> val) || val < 0 || val > 3)
            {
                cout << "Invalid brick value" << endl;
                return false;
            }

            game.bricks[r][c] = val;
        }
    }

    return true;
}

// Reset ball and paddle
void resetBall(GameState& game)
{
    game.powerUpExists = false;
    game.fireTimer = 0;
    game.ballLaunched = false;

    for (Ball& ball : game.balls)
        ball = Ball();

    game.paddleX = PADDLE_START_X;

    game.balls[0].active = true;
    game.balls[0].x = game.paddleX + 44;
    game.balls[0].y = 638;
}

// Start new game
void restartGame(GameState& game)
{
    game.score = 0;
    game.lives = 3;

    game.gameOver = false;
    game.gameWon = false;
    game.gameStarted = true;
    game.paused = false;

    game.menuSelection = 0;

    resetBall(game);
    loadLevel(game);
}

// Return to main menu
void returnToMenu(GameState& game)
{
    game.gameStarted = false;
    game.gameOver = false;
    game.gameWon = false;
    game.paused = false;

    game.menuSelection = 0;
}

// Save game
bool saveGame(GameState& game)
{
    ofstream file("save.txt");

    if (!file.is_open())
    {
        cout << "Save failed." << endl;
        return false;
    }

    file << game.score << endl;
    file << game.lives << endl;

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            file << game.bricks[r][c] << " ";
        }

        file << endl;
    }

    cout << "Game saved." << endl;
    return true;
}

// Load saved game
bool loadSave(GameState& game)
{
    ifstream file("save.txt");

    if (!file.is_open())
    {
        cout << "No save file found." << endl;
        return false;
    }

    int savedScore, savedLives;
    int savedBricks[ROWS][COLS];

    if (!(file >> savedScore >> savedLives))
        return false;

    if (savedLives <= 0 || savedLives > 3)
        return false;

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            if (!(file >> savedBricks[r][c]) ||
                savedBricks[r][c] < 0 ||
                savedBricks[r][c] > 3)
            {
                return false;
            }
        }
    }

    game.score = savedScore;
    game.lives = savedLives;

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            game.bricks[r][c] = savedBricks[r][c];
        }
    }

    resetBall(game);
    game.gameStarted = true;

    return true;
}

// Brick colors
Color getBrickColor(int hp)
{
    if (hp == 1)
        return Color(70, 210, 120);

    if (hp == 2)
        return Color(255, 190, 70);

    if (hp == 3)
        return Color(255, 85, 100);

    return Color::Transparent;
}

// Draw menu option with selection slider
void drawOption(RenderWindow& window, Font& font,
    string label, int index, int selected, float startY)
{
    float y = startY + index * 65;

    // Selection background
    if (index == selected)
    {
        RectangleShape highlight(Vector2f(330, 52));
        highlight.setPosition(Vector2f(135, y));
        highlight.setFillColor(Color(55, 75, 115));
        highlight.setOutlineColor(Color(100, 190, 255));
        highlight.setOutlineThickness(2);

        window.draw(highlight);

        // Selection pointer
        Text pointer(font, ">", 28);
        pointer.setFillColor(Color(100, 220, 255));
        pointer.setPosition(Vector2f(150, y + 8));
        window.draw(pointer);
    }

    // Option text
    Text optionText(font, label, 25);

    if (index == selected)
        optionText.setFillColor(Color::White);
    else
        optionText.setFillColor(Color(170, 180, 200));

    optionText.setPosition(Vector2f(195, y + 10));

    window.draw(optionText);
}

// Input
void handleInput(RenderWindow& window, GameState& game)
{
    while (auto event = window.pollEvent())
    {
        if (event->is<Event::Closed>())
        {
            window.close();
        }

        else if (const auto* key =
            event->getIf<Event::KeyPressed>())
        {
            // MAIN MENU
            if (!game.gameStarted &&
                !game.gameOver &&
                !game.gameWon &&
                !game.paused)
            {
                if (key->code == Keyboard::Key::Up)
                {
                    game.menuSelection--;

                    if (game.menuSelection < 0)
                        game.menuSelection = 2;
                }

                else if (key->code == Keyboard::Key::Down)
                {
                    game.menuSelection++;

                    if (game.menuSelection > 2)
                        game.menuSelection = 0;
                }

                else if (key->code == Keyboard::Key::Enter)
                {
                    if (game.menuSelection == 0)
                    {
                        restartGame(game);
                    }

                    else if (game.menuSelection == 1)
                    {
                        if (loadSave(game))
                        {
                            cout << "Save loaded." << endl;
                        }
                    }

                    else if (game.menuSelection == 2)
                    {
                        window.close();
                    }
                }
            }

            // PAUSE MENU
            else if (game.paused)
            {
                if (key->code == Keyboard::Key::Up)
                {
                    game.menuSelection--;

                    if (game.menuSelection < 0)
                        game.menuSelection = 2;
                }

                else if (key->code == Keyboard::Key::Down)
                {
                    game.menuSelection++;

                    if (game.menuSelection > 2)
                        game.menuSelection = 0;
                }

                else if (key->code == Keyboard::Key::Enter)
                {
                    if (game.menuSelection == 0)
                    {
                        game.paused = false;
                    }

                    else if (game.menuSelection == 1)
                    {
                        returnToMenu(game);
                    }

                    else if (game.menuSelection == 2)
                    {
                        window.close();
                    }
                }
            }

            // WIN / LOSE MENU
            else if (game.gameOver || game.gameWon)
            {
                if (key->code == Keyboard::Key::Up)
                {
                    game.menuSelection--;

                    if (game.menuSelection < 0)
                        game.menuSelection = 2;
                }

                else if (key->code == Keyboard::Key::Down)
                {
                    game.menuSelection++;

                    if (game.menuSelection > 2)
                        game.menuSelection = 0;
                }

                else if (key->code == Keyboard::Key::Enter)
                {
                    if (game.menuSelection == 0)
                    {
                        restartGame(game);
                    }

                    else if (game.menuSelection == 1)
                    {
                        returnToMenu(game);
                    }

                    else if (game.menuSelection == 2)
                    {
                        window.close();
                    }
                }
            }

            // GAMEPLAY
            else
            {
                if (key->code == Keyboard::Key::Space &&
                    !game.ballLaunched)
                {
                    game.ballLaunched = true;
                    game.balls[0].dy = BALL_SPEED_Y;

                    if (rand() % 2 == 0)
                        game.balls[0].dx = BALL_SPEED_X;
                    else
                        game.balls[0].dx = -BALL_SPEED_X;
                }

                else if (key->code == Keyboard::Key::P)
                {
                    game.paused = true;
                    game.menuSelection = 0;
                }

                else if (key->code == Keyboard::Key::S)
                {
                    saveGame(game);
                }
            }
        }
    }
}

// Fill up to MAX_BALLS balls, launched from the first active ball
void spawnExtraBalls(GameState& game)
{
    Ball source;

    for (const Ball& ball : game.balls)
    {
        if (ball.active)
        {
            source = ball;
            break;
        }
    }

    float spread = -3;

    for (Ball& ball : game.balls)
    {
        if (ball.active)
            continue;

        ball = source;
        ball.active = true;
        ball.dy = BALL_SPEED_Y;
        ball.dx = source.dx + spread;

        if (ball.dx > 6)
            ball.dx = 6;

        if (ball.dx < -6)
            ball.dx = -6;

        spread = -spread;
    }
}

// Move one ball and handle its collisions
void updateBall(GameState& game, Ball& ball)
{
    ball.x += ball.dx;
    ball.y += ball.dy;

    // Left wall
    if (ball.x <= 0)
    {
        ball.x = 0;
        ball.dx = -ball.dx;
    }

    // Right wall
    if (ball.x >= WINDOW_WIDTH - BALL_RADIUS * 2)
    {
        ball.x = WINDOW_WIDTH - BALL_RADIUS * 2;
        ball.dx = -ball.dx;
    }

    // Top wall
    if (ball.y <= 0)
    {
        ball.y = 0;
        ball.dy = -ball.dy;
    }

    // Paddle collision
    if (ball.dy > 0 &&
        ball.y + BALL_RADIUS * 2 >= PADDLE_Y &&
        ball.x + BALL_RADIUS * 2 >= game.paddleX &&
        ball.x <= game.paddleX + PADDLE_WIDTH)
    {
        ball.y = PADDLE_Y - BALL_RADIUS * 2;

        float ballCenter = ball.x + BALL_RADIUS;
        float paddleCenter = game.paddleX + PADDLE_WIDTH / 2;

        float hitOffset =
            (ballCenter - paddleCenter) /
            (PADDLE_WIDTH / 2);

        ball.dx = hitOffset * 5;
        ball.dy = BALL_SPEED_Y;
    }

    // Ball lost
    if (ball.y > WINDOW_HEIGHT)
    {
        ball.active = false;
        return;
    }

    // Brick collision
    bool onFire = game.fireTimer > 0;

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            if (game.bricks[r][c] == 0)
                continue;

            float brickX = c * BRICK_WIDTH + BRICK_GAP / 2;
            float brickY = BRICK_START_Y + r * BRICK_HEIGHT + BRICK_GAP / 2;

            float ballLeft = ball.x;
            float ballRight = ball.x + BALL_RADIUS * 2;
            float ballTop = ball.y;
            float ballBottom = ball.y + BALL_RADIUS * 2;

            if (ballRight < brickX ||
                ballLeft > brickX + BRICK_WIDTH ||
                ballBottom < brickY ||
                ballTop > brickY + BRICK_HEIGHT)
            {
                continue;
            }

            float overlapX = ballRight - brickX;
            float otherOverlapX = brickX + BRICK_WIDTH - ballLeft;

            if (otherOverlapX < overlapX)
                overlapX = otherOverlapX;

            float overlapY = ballBottom - brickY;
            float otherOverlapY = brickY + BRICK_HEIGHT - ballTop;

            if (otherOverlapY < overlapY)
                overlapY = otherOverlapY;

            // Horizontal collision
            if (overlapX < overlapY)
            {
                if (ball.dx > 0)
                    ball.x = brickX - BALL_RADIUS * 2;
                else
                    ball.x = brickX + BRICK_WIDTH;

                ball.dx = -ball.dx;
            }

            // Vertical collision
            else
            {
                if (ball.dy > 0)
                    ball.y = brickY - BALL_RADIUS * 2;
                else
                    ball.y = brickY + BRICK_HEIGHT;

                ball.dy = -ball.dy;
            }

            // Fire does more damage and scores more
            game.bricks[r][c] -= onFire ? FIRE_DAMAGE : 1;

            if (game.bricks[r][c] < 0)
                game.bricks[r][c] = 0;

            game.score += onFire ? FIRE_BRICK_SCORE : BRICK_SCORE;

            brickSound.play();

            // Power-up drop
            if (game.bricks[r][c] == 0 &&
                !game.powerUpExists &&
                rand() % POWERUP_CHANCE == 0)
            {
                game.powerUpExists = true;
                game.powerUpX = brickX + BRICK_WIDTH / 2;
                game.powerUpY = brickY + BRICK_HEIGHT / 2;
            }

            return;
        }
    }
}

// Game logic
void updateGame(GameState& game)
{
    if (!game.gameStarted ||
        game.gameOver ||
        game.gameWon ||
        game.paused)
    {
        return;
    }

    // Paddle movement
    if (Keyboard::isKeyPressed(Keyboard::Key::Left) ||
        Keyboard::isKeyPressed(Keyboard::Key::A))
    {
        game.paddleX -= PADDLE_SPEED;
    }

    if (Keyboard::isKeyPressed(Keyboard::Key::Right) ||
        Keyboard::isKeyPressed(Keyboard::Key::D))
    {
        game.paddleX += PADDLE_SPEED;
    }

    // Boundaries
    if (game.paddleX < 0)
        game.paddleX = 0;

    if (game.paddleX > WINDOW_WIDTH - PADDLE_WIDTH)
        game.paddleX = WINDOW_WIDTH - PADDLE_WIDTH;

    // Ball follows paddle until launched
    if (!game.ballLaunched)
    {
        game.balls[0].x = game.paddleX + 44;
        game.balls[0].y = 638;
    }

    // Balls
    else
    {
        bool anyBallLeft = false;

        for (Ball& ball : game.balls)
        {
            if (ball.active)
                updateBall(game, ball);

            if (ball.active)
                anyBallLeft = true;
        }

        // Life is lost only when every ball is gone
        if (!anyBallLeft)
        {
            game.lives--;
            resetBall(game);

            if (game.lives <= 0)
            {
                game.gameOver = true;
                game.menuSelection = 0;
                gameOverSound.play();
            }
        }
    }

    // Power-up movement
    if (game.powerUpExists)
    {
        game.powerUpY += POWERUP_SPEED;

        // Caught by paddle
        if (game.powerUpY + POWERUP_RADIUS >= PADDLE_Y &&
            game.powerUpY - POWERUP_RADIUS <= PADDLE_Y + PADDLE_HEIGHT &&
            game.powerUpX + POWERUP_RADIUS >= game.paddleX &&
            game.powerUpX - POWERUP_RADIUS <= game.paddleX + PADDLE_WIDTH)
        {
            game.powerUpExists = false;
            game.fireTimer = FIRE_DURATION;
            spawnExtraBalls(game);
            powerUpSound.play();
        }

        // Missed
        else if (game.powerUpY > WINDOW_HEIGHT)
        {
            game.powerUpExists = false;
        }
    }

    // Fire burns out
    if (game.fireTimer > 0)
        game.fireTimer--;

    // Win check
    bool allBricksDestroyed = true;

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            if (game.bricks[r][c] > 0)
                allBricksDestroyed = false;
        }
    }

    if (allBricksDestroyed)
    {
        game.gameWon = true;
        game.menuSelection = 0;
        winSound.play();
    }
}

// Draw background decoration
void drawBackground(RenderWindow& window)
{
    window.clear(Color(15, 18, 35));

    // Decorative circles
    CircleShape circle1(100);
    circle1.setPosition(Vector2f(-50, -50));
    circle1.setFillColor(Color(25, 35, 65));
    window.draw(circle1);

    CircleShape circle2(150);
    circle2.setPosition(Vector2f(430, 520));
    circle2.setFillColor(Color(25, 35, 65));
    window.draw(circle2);
}

// Draw menus
void drawMenu(RenderWindow& window, Font& font,
    GameState& game)
{
    drawBackground(window);

    // Title
    Text title(font, "BLOCK BUSTER", 48);
    title.setFillColor(Color(100, 220, 255));
    title.setPosition(Vector2f(115, 160));
    window.draw(title);

    Text subtitle(font, "BREAK. BOUNCE. SURVIVE.", 17);
    subtitle.setFillColor(Color(150, 170, 200));
    subtitle.setPosition(Vector2f(165, 225));
    window.draw(subtitle);

    // Options
    drawOption(window, font, "NEW GAME", 0,
        game.menuSelection, 320);

    drawOption(window, font, "LOAD GAME", 1,
        game.menuSelection, 320);

    drawOption(window, font, "EXIT", 2,
        game.menuSelection, 320);

    // Controls footer
    Text footer(font, "UP / DOWN - SELECT", 16);
    footer.setFillColor(Color(150, 170, 200));
    footer.setPosition(Vector2f(160, 550));
    window.draw(footer);

    Text footer2(font, "ENTER - CONFIRM", 16);
    footer2.setFillColor(Color(150, 170, 200));
    footer2.setPosition(Vector2f(185, 585));
    window.draw(footer2);

    window.display();
}

// Draw a circle by its center
void drawCircle(RenderWindow& window, float cx, float cy,
    float radius, Color color)
{
    CircleShape shape(radius);
    shape.setPosition(Vector2f(cx - radius, cy - radius));
    shape.setFillColor(color);
    window.draw(shape);
}

// Falling fire power-up: a small glowing ember
void drawPowerUp(RenderWindow& window, GameState& game)
{
    float x = game.powerUpX;
    float y = game.powerUpY;
    float pulse = 1 + 0.12f * sin(y * 0.15f);

    // Faint trail above the ember
    for (int i = 1; i <= 4; i++)
    {
        drawCircle(window, x, y - i * 7,
            POWERUP_RADIUS * (1 - i * 0.18f),
            Color(255, 120, 30, 90 - i * 20));
    }

    drawCircle(window, x, y, POWERUP_RADIUS * 1.9f * pulse, Color(255, 120, 30, 55));
    drawCircle(window, x, y, POWERUP_RADIUS, Color(255, 130, 35));
    drawCircle(window, x, y, POWERUP_RADIUS * 0.55f * pulse, Color(255, 230, 120));
}

// Normal ball is white, a burning ball has a glowing flame tail
void drawBall(RenderWindow& window, GameState& game, Ball& ball)
{
    float cx = ball.x + BALL_RADIUS;
    float cy = ball.y + BALL_RADIUS;

    if (game.fireTimer <= 0)
    {
        drawCircle(window, cx, cy, BALL_RADIUS, Color::White);
        return;
    }

    float flicker = 1 + 0.15f * sin(game.fireTimer * 0.5f);

    for (int i = 1; i <= 8; i++)
    {
        drawCircle(window,
            cx - ball.dx * i * 0.8f,
            cy - ball.dy * i * 0.8f,
            BALL_RADIUS * (1 - i * 0.1f),
            Color(255, 150 - i * 14, 30, 150 - i * 17));
    }

    drawCircle(window, cx, cy, BALL_RADIUS * 2.2f * flicker, Color(255, 110, 20, 70));
    drawCircle(window, cx, cy, BALL_RADIUS * 1.35f, Color(255, 150, 40));
    drawCircle(window, cx, cy, BALL_RADIUS * 0.8f, Color(255, 235, 150));
}

// Draw gameplay
void drawGameplay(RenderWindow& window, Font& font,
    GameState& game)
{
    drawBackground(window);

    // HUD
    RectangleShape hud(Vector2f(WINDOW_WIDTH, 50));
    hud.setFillColor(Color(20, 25, 45));
    window.draw(hud);

    Text scoreText(font, "Score: " + to_string(game.score), 22);
    scoreText.setPosition(Vector2f(20, 12));
    window.draw(scoreText);

    Text livesText(font, "Lives: " + to_string(game.lives), 22);
    livesText.setPosition(Vector2f(450, 12));
    window.draw(livesText);

    // Fire time left
    if (game.fireTimer > 0)
    {
        RectangleShape bar(
            Vector2f(560.0f * game.fireTimer / FIRE_DURATION, 4));
        bar.setPosition(Vector2f(20, 44));
        bar.setFillColor(Color(255, 140, 40));
        window.draw(bar);
    }

    // Bricks
    RectangleShape brickShape(
        Vector2f(BRICK_WIDTH - BRICK_GAP,
            BRICK_HEIGHT - BRICK_GAP));

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            int hp = game.bricks[r][c];

            if (hp == 0)
                continue;

            float x = c * BRICK_WIDTH + BRICK_GAP / 2;
            float y = BRICK_START_Y +
                r * BRICK_HEIGHT + BRICK_GAP / 2;

            brickShape.setPosition(Vector2f(x, y));
            brickShape.setFillColor(getBrickColor(hp));

            window.draw(brickShape);
        }
    }

    // Paddle
    RectangleShape paddle(
        Vector2f(PADDLE_WIDTH, PADDLE_HEIGHT));

    paddle.setPosition(Vector2f(game.paddleX, PADDLE_Y));
    paddle.setFillColor(Color(100, 220, 255));
    window.draw(paddle);

    // Power-up
    if (game.powerUpExists)
        drawPowerUp(window, game);

    // Balls
    for (Ball& ball : game.balls)
    {
        if (ball.active)
            drawBall(window, game, ball);
    }

    // Pause menu
    if (game.paused)
    {
        RectangleShape overlay(Vector2f(600, 700));
        overlay.setFillColor(Color(0, 0, 0, 180));
        window.draw(overlay);

        Text title(font, "PAUSED", 45);
        title.setFillColor(Color(100, 220, 255));
        title.setPosition(Vector2f(200, 200));
        window.draw(title);

        drawOption(window, font, "RESUME", 0,
            game.menuSelection, 310);

        drawOption(window, font, "MAIN MENU", 1,
            game.menuSelection, 310);

        drawOption(window, font, "EXIT", 2,
            game.menuSelection, 310);
    }

    // Win or Game Over menu
    if (game.gameOver || game.gameWon)
    {
        RectangleShape overlay(Vector2f(600, 700));
        overlay.setFillColor(Color(0, 0, 0, 200));
        window.draw(overlay);

        if (game.gameWon)
        {
            Text title(font, "YOU WIN!", 45);
            title.setFillColor(Color(100, 255, 140));
            title.setPosition(Vector2f(180, 180));
            window.draw(title);
        }
        else
        {
            Text title(font, "GAME OVER", 45);
            title.setFillColor(Color(255, 90, 100));
            title.setPosition(Vector2f(150, 180));
            window.draw(title);
        }

        Text scoreText(font,
            "FINAL SCORE: " + to_string(game.score), 24);

        scoreText.setFillColor(Color::White);
        scoreText.setPosition(Vector2f(175, 270));
        window.draw(scoreText);

        drawOption(window, font, "PLAY AGAIN", 0,
            game.menuSelection, 340);

        drawOption(window, font, "MAIN MENU", 1,
            game.menuSelection, 340);

        drawOption(window, font, "EXIT", 2,
            game.menuSelection, 340);
    }

    window.display();
}

// Main
int main()
{
    srand(time(0));

    RenderWindow window(
        VideoMode({ 600, 700 }), "Block Buster");

    window.setFramerateLimit(FPS_LIMIT);

    if (!brickBuffer.loadFromFile("brick.wav"))
        cout << "Could not load brick.wav" << endl;
    if (!gameOverBuffer.loadFromFile("gameover.wav"))
        cout << "Could not load gameover.wav" << endl;
    if (!powerUpBuffer.loadFromFile("powerup.wav"))
        cout << "Could not load powerup.wav" << endl;
    if (!winBuffer.loadFromFile("win.wav"))
        cout << "Could not load win.wav" << endl;

    Font font;

    if (!font.openFromFile("arial.ttf"))
    {
        cout << "Font loading failed." << endl;
        return 1;
    }

    GameState game;

    loadLevel(game);

    while (window.isOpen())
    {
        handleInput(window, game);
        updateGame(game);

        if (!game.gameStarted &&
            !game.gameOver &&
            !game.gameWon &&
            !game.paused)
        {
            drawMenu(window, font, game);
        }
        else
        {
            drawGameplay(window, font, game);
        }
    }

    return 0;
}
