#include <SFML/Graphics.hpp>
#include <fstream>
#include <iostream>
#include <string>

constexpr unsigned int WINDOW_WIDTH = 600;
constexpr unsigned int WINDOW_HEIGHT = 700;
constexpr unsigned int FPS_LIMIT = 60;

constexpr int ROWS = 8;
constexpr int COLS = 10;
constexpr float BRICK_WIDTH = 60.f;
constexpr float BRICK_HEIGHT = 25.f;
constexpr float BRICK_START_Y = 100.f;
constexpr float BRICK_GAP = 2.f;

struct GameState
{
    int bricks[ROWS][COLS]{};
    bool levelLoaded = false;
    std::string levelError;
};

bool loadLevel(const std::string& filename, GameState& game)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        game.levelLoaded = false;
        game.levelError = "Error: Could not open " + filename;
        return false;
    }

    int fileRows = 0;
    int fileCols = 0;
    if (!(file >> fileRows >> fileCols) || fileRows != ROWS || fileCols != COLS)
    {
        game.levelLoaded = false;
        game.levelError = "Error: Invalid grid dimensions in " + filename;
        return false;
    }

    for (int r = 0; r < ROWS; ++r)
    {
        for (int c = 0; c < COLS; ++c)
        {
            int val = 0;
            if (!(file >> val) || val < 0 || val > 3)
            {
                game.levelLoaded = false;
                game.levelError = "Error: Invalid brick value in " + filename;
                return false;
            }
            game.bricks[r][c] = val;
        }
    }

    game.levelLoaded = true;
    game.levelError.clear();
    return true;
}

sf::Color getBrickColor(int hp)
{
    switch (hp)
    {
    case 1:  return sf::Color(60, 200, 90);   // Green (1 hit)
    case 2:  return sf::Color(235, 200, 50);  // Yellow (2 hits)
    case 3:  return sf::Color(225, 65, 65);   // Red (3 hits)
    default: return sf::Color::Transparent;
    }
}

void handleInput(sf::RenderWindow& window)
{
    while (const std::optional event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window.close();
        }
    }
}

void updateGame(GameState& /*game*/)
{
    // Paddle, ball, collision, and HUD logic will go here in later features
}

void drawGame(sf::RenderWindow& window, const GameState& game)
{
    window.clear(sf::Color(25, 25, 45));

    if (!game.levelLoaded)
    {
        sf::RectangleShape errorBanner({ WINDOW_WIDTH - 40.f, 60.f });
        errorBanner.setPosition({ 20.f, 280.f });
        errorBanner.setFillColor(sf::Color(140, 30, 30));
        window.draw(errorBanner);
        window.display();
        return;
    }

    sf::RectangleShape brickShape({ BRICK_WIDTH - BRICK_GAP, BRICK_HEIGHT - BRICK_GAP });

    for (int r = 0; r < ROWS; ++r)
    {
        for (int c = 0; c < COLS; ++c)
        {
            int hp = game.bricks[r][c];
            if (hp <= 0)
                continue;

            float x = static_cast<float>(c) * BRICK_WIDTH + (BRICK_GAP * 0.5f);
            float y = BRICK_START_Y + static_cast<float>(r) * BRICK_HEIGHT + (BRICK_GAP * 0.5f);

            brickShape.setPosition({ x, y });
            brickShape.setFillColor(getBrickColor(hp));
            window.draw(brickShape);
        }
    }

    window.display();
}

int main()
{
    sf::RenderWindow window(sf::VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }), "Block Buster");
    window.setFramerateLimit(FPS_LIMIT);

    GameState game;
    if (!loadLevel("level.txt", game))
    {
        std::cerr << game.levelError << '\n';
    }

    while (window.isOpen())
    {
        handleInput(window);
        updateGame(game);
        drawGame(window, game);
    }

    return 0;
}