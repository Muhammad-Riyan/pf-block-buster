
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
    int score = 0;
    int bricks[ROWS][COLS] = {};
    bool levelLoaded = false;

    float paddleX = PADDLE_START_X;

    float ballX = PADDLE_START_X + 44;
    float ballY = 638;

    float dx = 0;
    float dy = 0;

    bool ballLaunched = false;
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

    if (!(file >> fileRows >> fileCols) || fileRows != ROWS || fileCols != COLS)
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
            if (key->code == Keyboard::Key::Space && !game.ballLaunched)
            {
                game.ballLaunched = true;
                game.dy = BALL_SPEED_Y;

                if (rand() % 2 == 0)
                    game.dx = BALL_SPEED_X;
                else
                    game.dx = -BALL_SPEED_X;
            }
        }
    }
}

//checks if the ball hit the brick
bool hitBrick(GameState& game) {
    int col = game.ballX / BRICK_WIDTH;
    int row = (game.ballY - BRICK_START_Y) / BRICK_HEIGHT;

    if (game.ballY >= BRICK_START_Y &&
        row < ROWS &&
        col >= 0 &&
        col < COLS)
    {
        if(game.bricks[row][col] > 0)
        {
            game.bricks[row][col]--;
            return true;
        }
    }
    return false;
}

void updateGame(GameState& game)
{
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

    // Stop left wall
    if (game.paddleX < 0)
        game.paddleX = 0;

    // Stop rght wall
    if (game.paddleX > WINDOW_WIDTH - PADDLE_WIDTH)
        game.paddleX = WINDOW_WIDTH - PADDLE_WIDTH;

    if (game.ballY <= 0)
        game.dy = BALL_SPEED_Y * -1;

    if (game.dy > 0 &&
        game.ballY + BALL_RADIUS * 2 >= PADDLE_Y &&
        game.ballX + BALL_RADIUS * 2 >= game.paddleX &&
        game.ballX <= game.paddleX + PADDLE_WIDTH)
    {
        float hitPosition = (game.ballX + BALL_RADIUS - game.paddleX) / PADDLE_WIDTH;

        if (hitPosition < 0.33)
            game.dx = -BALL_SPEED_X;

        else if (hitPosition > 0.66)
            game.dx = BALL_SPEED_X;

        else
            game.dx = 0;

        game.dy = BALL_SPEED_Y;
    }

   //bal on padl
    if (!game.ballLaunched)
    {
        game.ballX = game.paddleX + 44;
        game.ballY = 638;
    }

    if (game.ballLaunched)
    {
        game.ballX += game.dx;
        if (hitBrick(game))
        {
            game.ballX -= game.dx;
            game.dx = -game.dx;
        }
        game.ballY += game.dy;
        if (hitBrick(game))
        {
            game.ballY -= game.dy;
            game.dy = -game.dy;
        }
    }

    if (game.ballX <= 0)
    {
        game.dx = BALL_SPEED_X;
    }

    if (game.ballX >= WINDOW_WIDTH - BALL_RADIUS * 2)
        game.dx = -BALL_SPEED_X;
}

void drawGame(RenderWindow& window, GameState& game)
{
    window.clear(Color(25, 25, 45));

    if (!game.levelLoaded)
    {
        RectangleShape errorBox(Vector2f(500, 60));
        errorBox.setPosition(Vector2f(50, 280));
        errorBox.setFillColor(Color(140, 30, 30));

        window.draw(errorBox);
        window.display();
        return;
    }

    // Draw bricks
    RectangleShape brickShape(Vector2f(BRICK_WIDTH - BRICK_GAP, BRICK_HEIGHT - BRICK_GAP));

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            int hp = game.bricks[r][c];

            if (hp == 0)
                continue;

            float x = c * BRICK_WIDTH + BRICK_GAP / 2;
            float y = BRICK_START_Y + r * BRICK_HEIGHT + BRICK_GAP / 2;

            brickShape.setPosition(Vector2f(x, y));
            brickShape.setFillColor(getBrickColor(hp));

            window.draw(brickShape);
        }
    }

    // Draw paddle
    RectangleShape paddle(Vector2f(PADDLE_WIDTH, PADDLE_HEIGHT));
    paddle.setPosition(Vector2f(game.paddleX, PADDLE_Y));
    paddle.setFillColor(Color(230, 230, 240));

    window.draw(paddle);

    // Draw ball
    CircleShape ball(BALL_RADIUS);
    ball.setPosition(Vector2f(game.ballX, game.ballY));
    ball.setFillColor(Color(255, 255, 255));

    window.draw(ball);

    window.display();
}

int main()
{
    srand(time(0));

    RenderWindow window(VideoMode({ 600, 700 }), "Block Buster");
    window.setFramerateLimit(FPS_LIMIT);

    GameState game;

    if (!loadLevel(game))
    {
        cout << "Level could not be loaded." << endl;
    }

    while (window.isOpen())
    {
        handleInput(window, game);
        updateGame(game);
        drawGame(window, game);
    }

    return 0;
}
