
#include <SFML/Graphics.hpp>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace std;
using namespace sf;

const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 700;
const int FPS_LIMIT = 60;

const int ROWS = 8;
const int COLS = 10;

const float BRICK_WIDTH = 60;
const float BRICK_HEIGHT = 25;
const float BRICK_START_Y = 100;
const float BRICK_GAP = 2;

const float PADDLE_WIDTH = 100;
const float PADDLE_HEIGHT = 14;
const float PADDLE_SPEED = 8;
const float PADDLE_START_X = 250;
const float PADDLE_Y = 650;

const float BALL_RADIUS = 6;
const float BALL_SPEED_X = 3;
const float BALL_SPEED_Y = -5;

struct GameState
{
    int bricks[ROWS][COLS] = {};
    bool levelLoaded = false;

    float paddleX = PADDLE_START_X;

    float ballX = PADDLE_START_X + 44;
    float ballY = 638;

    float dx = 0;
    float dy = 0;

    bool ballLaunched = false;
    bool gameStarted = false;

    int score = 0;
    int lives = 3;

    bool gameOver = false;
    bool gameWon = false;
};

bool loadLevel(GameState& game)
{
    ifstream file("level.txt");

    if (!file.is_open())
    {
        cout << "Error: Could not open level.txt" << endl;
        game.levelLoaded = false;
        return false;
    }

    int fileRows, fileCols;

    if (!(file >> fileRows >> fileCols) ||
        fileRows != ROWS || fileCols != COLS)
    {
        cout << "Error: Invalid grid dimensions" << endl;
        game.levelLoaded = false;
        return false;
    }

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            int val;

            if (!(file >> val) || val < 0 || val > 3)
            {
                cout << "Error: Invalid brick value" << endl;
                game.levelLoaded = false;
                return false;
            }

            game.bricks[r][c] = val;
        }
    }

    game.levelLoaded = true;
    file.close();

    return true;
}

bool saveGame(GameState& game)
{
    ofstream file("save.txt");

    if (!file.is_open())
    {
        cout << "Could not save game." << endl;
        return false;
    }

    file << game.score << endl;
    file << game.lives << endl;
    file << game.paddleX << endl;
    file << game.ballX << endl;
    file << game.ballY << endl;
    file << game.dx << endl;
    file << game.dy << endl;
    file << game.ballLaunched << endl;

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            file << game.bricks[r][c] << " ";
        }
        file << endl;
    }

    file.close();

    cout << "Game saved!" << endl;
    return true;
}

bool loadSave(GameState& game)
{
    ifstream file("save.txt");

    if (!file.is_open())
    {
        cout << "No save file found." << endl;
        return false;
    }

    if (!(file >> game.score >> game.lives
        >> game.paddleX >> game.ballX >> game.ballY
        >> game.dx >> game.dy >> game.ballLaunched))
    {
        cout << "Invalid save file." << endl;
        return false;
    }

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            if (!(file >> game.bricks[r][c]))
            {
                cout << "Invalid brick data." << endl;
                return false;
            }
        }
    }

    game.gameStarted = true;
    game.gameOver = false;
    game.gameWon = false;

    file.close();

    cout << "Game loaded!" << endl;
    return true;
}

Color getBrickColor(int hp)
{
    if (hp == 1)
        return Color(60, 200, 90);

    if (hp == 2)
        return Color(235, 200, 50);

    if (hp == 3)
        return Color(225, 65, 65);

    return Color::Transparent;
}

void handleInput(RenderWindow& window, GameState& game)
{
    while (auto event = window.pollEvent())
    {
        if (event->is<Event::Closed>())
        {
            window.close();
        }

        else if (const auto* key = event->getIf<Event::KeyPressed>())
        {
            if (key->code == Keyboard::Key::Space)
            {
                if (!game.gameStarted &&
                    !game.gameOver &&
                    !game.gameWon)
                {
                    game.gameStarted = true;
                }

                else if (game.gameStarted &&
                    !game.ballLaunched &&
                    !game.gameOver &&
                    !game.gameWon)
                {
                    game.ballLaunched = true;
                    game.dy = BALL_SPEED_Y;

                    if (rand() % 2 == 0)
                        game.dx = BALL_SPEED_X;
                    else
                        game.dx = -BALL_SPEED_X;
                }
            }

            // Save game
            else if (key->code == Keyboard::Key::S)
            {
                if (game.gameStarted &&
                    !game.gameOver &&
                    !game.gameWon)
                {
                    saveGame(game);
                }
            }

            // Load game from menu
            else if (key->code == Keyboard::Key::L)
            {
                if (!game.gameStarted &&
                    !game.gameOver &&
                    !game.gameWon)
                {
                    loadSave(game);
                }
            }
        }
    }
}

void updateGame(GameState& game)
{
    if (!game.gameStarted || game.gameOver || game.gameWon)
        return;

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

    // Paddle boundaries
    if (game.paddleX < 0)
        game.paddleX = 0;

    if (game.paddleX > WINDOW_WIDTH - PADDLE_WIDTH)
        game.paddleX = WINDOW_WIDTH - PADDLE_WIDTH;

    // Ball follows paddle before launch
    if (!game.ballLaunched)
    {
        game.ballX = game.paddleX + 44;
        game.ballY = 638;
    }

    // Ball movement
    if (game.ballLaunched)
    {
        game.ballX += game.dx;
        game.ballY += game.dy;
    }

    // Left wall
    if (game.ballX <= 0)
    {
        game.ballX = 0;
        game.dx = BALL_SPEED_X;
    }

    // Right wall
    if (game.ballX >= WINDOW_WIDTH - BALL_RADIUS * 2)
    {
        game.ballX = WINDOW_WIDTH - BALL_RADIUS * 2;
        game.dx = -BALL_SPEED_X;
    }

    // Top wall
    if (game.ballY <= 0)
    {
        game.ballY = 0;
        game.dy = -BALL_SPEED_Y;
    }

    // Paddle collision
    if (game.dy > 0 &&
        game.ballY + BALL_RADIUS * 2 >= PADDLE_Y &&
        game.ballX + BALL_RADIUS * 2 >= game.paddleX &&
        game.ballX <= game.paddleX + PADDLE_WIDTH)
    {
        game.ballY = PADDLE_Y - BALL_RADIUS * 2;
        game.dy = BALL_SPEED_Y;

        float hitPosition =
            (game.ballX + BALL_RADIUS - game.paddleX)
            / PADDLE_WIDTH;

        if (hitPosition < 0.33)
            game.dx = -BALL_SPEED_X;

        else if (hitPosition > 0.66)
            game.dx = BALL_SPEED_X;

        else
            game.dx = 0;
    }

    // Ball lost
    if (game.ballY > WINDOW_HEIGHT)
    {
        game.lives--;

        if (game.lives <= 0)
        {
            game.gameOver = true;
        }
        else
        {
            game.ballLaunched = false;
            game.dx = 0;
            game.dy = 0;
            game.ballX = game.paddleX + 44;
            game.ballY = 638;
        }
    }

    // Brick collision
    if (game.ballLaunched && !game.gameOver)
    {
        bool hitBrick = false;

        for (int r = 0; r < ROWS && !hitBrick; r++)
        {
            for (int c = 0; c < COLS; c++)
            {
                if (game.bricks[r][c] == 0)
                    continue;

                float brickX = c * BRICK_WIDTH + BRICK_GAP / 2;
                float brickY = BRICK_START_Y +
                    r * BRICK_HEIGHT + BRICK_GAP / 2;

                float ballLeft = game.ballX;
                float ballRight = game.ballX + BALL_RADIUS * 2;
                float ballTop = game.ballY;
                float ballBottom = game.ballY + BALL_RADIUS * 2;

                if (ballRight >= brickX &&
                    ballLeft <= brickX + BRICK_WIDTH &&
                    ballBottom >= brickY &&
                    ballTop <= brickY + BRICK_HEIGHT)
                {
                    float overlapX = ballRight - brickX;
                    float otherOverlapX =
                        brickX + BRICK_WIDTH - ballLeft;

                    if (otherOverlapX < overlapX)
                        overlapX = otherOverlapX;

                    float overlapY = ballBottom - brickY;
                    float otherOverlapY =
                        brickY + BRICK_HEIGHT - ballTop;

                    if (otherOverlapY < overlapY)
                        overlapY = otherOverlapY;

                    // Horizontal collision
                    if (overlapX < overlapY)
                    {
                        if (game.dx > 0)
                            game.ballX = brickX - BALL_RADIUS * 2;
                        else
                            game.ballX = brickX + BRICK_WIDTH;

                        game.dx = -game.dx;
                    }

                    // Vertical collision
                    else
                    {
                        if (game.dy > 0)
                            game.ballY = brickY - BALL_RADIUS * 2;
                        else
                            game.ballY = brickY + BRICK_HEIGHT;

                        game.dy = -game.dy;
                    }

                    game.bricks[r][c]--;
                    game.score += 10;

                    hitBrick = true;
                    break;
                }
            }
        }
    }

    // Check if all bricks destroyed
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
        game.gameWon = true;
}

void drawGame(RenderWindow& window, GameState& game, Font& font)
{
    window.clear(Color(25, 25, 45));

    // Menu
    if (!game.gameStarted && !game.gameOver && !game.gameWon)
    {
        Text title(font, "BLOCK BUSTER", 50);
        title.setFillColor(Color(235, 200, 50));
        title.setPosition(Vector2f(90, 230));
        window.draw(title);

        Text instructions(font, "Press SPACE to start", 28);
        instructions.setFillColor(Color::White);
        instructions.setPosition(Vector2f(145, 350));
        window.draw(instructions);

        Text controls(font, "Move with A/D or arrow keys", 22);
        controls.setFillColor(Color(180, 180, 200));
        controls.setPosition(Vector2f(115, 420));
        window.draw(controls);

        Text saveInstructions(font, "Press L to load saved game", 22);
        saveInstructions.setFillColor(Color(180, 180, 200));
        saveInstructions.setPosition(Vector2f(135, 470));
        window.draw(saveInstructions);
        window.display();
        return;
    }

    // HUD
    RectangleShape hud(Vector2f(WINDOW_WIDTH, 50));
    hud.setFillColor(Color(15, 15, 30));
    window.draw(hud);

    // Score
    Text scoreText(font, "Score: " + to_string(game.score), 24);
    scoreText.setPosition(Vector2f(20, 10));
    scoreText.setFillColor(Color::White);
    window.draw(scoreText);

    // Lives
    Text livesText(font, "Lives: " + to_string(game.lives), 24);
    livesText.setPosition(Vector2f(440, 10));
    livesText.setFillColor(Color::White);
    window.draw(livesText);

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
    paddle.setFillColor(Color(230, 230, 240));

    window.draw(paddle);

    // Ball
    CircleShape ball(BALL_RADIUS);
    ball.setPosition(Vector2f(game.ballX, game.ballY));
    ball.setFillColor(Color::White);

    window.draw(ball);

    // End screen
    if (game.gameOver || game.gameWon)
    {
        RectangleShape overlay(Vector2f(600, 700));
        overlay.setFillColor(Color(0, 0, 0, 180));
        window.draw(overlay);

        if (game.gameWon)
        {
            Text resultText(font, "YOU WIN!", 50);
            resultText.setFillColor(Color::Green);
            resultText.setPosition(Vector2f(160, 280));
            window.draw(resultText);
        }
        else
        {
            Text resultText(font, "GAME OVER", 50);
            resultText.setFillColor(Color::Red);
            resultText.setPosition(Vector2f(130, 280));
            window.draw(resultText);
        }

        Text finalScore(font, "Final Score: " + to_string(game.score), 30);
        finalScore.setPosition(Vector2f(160, 370));
        window.draw(finalScore);
    }

    window.display();
}

int main()
{
    srand(time(0));

    RenderWindow window(
        VideoMode({ 600, 700 }), "Block Buster");

    window.setFramerateLimit(FPS_LIMIT);

    Font font;

    if (!font.openFromFile("arial.ttf"))
    {
        cout << "Error: Could not load arial.ttf" << endl;
        return 1;
    }

    GameState game;

    if (!loadLevel(game))
    {
        cout << "Level could not be loaded." << endl;
    }

    while (window.isOpen())
    {
        handleInput(window, game);
        updateGame(game);
        drawGame(window, game, font);
    }

    return 0;
}
