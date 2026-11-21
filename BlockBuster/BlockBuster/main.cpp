#include <SFML/Graphics.hpp>

int main()
{
    sf::RenderWindow window(sf::VideoMode({ 600, 700 }), "Block Buster");
    window.setFramerateLimit(60);

    sf::CircleShape circle(40.f);
    circle.setFillColor(sf::Color::Green);
    circle.setPosition({ 260.f, 310.f });

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        window.clear(sf::Color(25, 25, 45));
        window.draw(circle);
        window.display();
    }

    return 0;
}