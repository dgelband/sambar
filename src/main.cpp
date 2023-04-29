#include <SFML/Graphics.hpp>
#include <random>

int main()
{

    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::normal_distribution<> d{0.0001, 0.00005}; 
    sf::RenderWindow window(sf::VideoMode(800,600), "Sambar Scamper");

    sf::Texture texture;
    if (!texture.loadFromFile("img/sambar-side.png", sf::IntRect(0,32,128,128))) return -1;
    
    sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setPosition(400.f, 300.f);


    while (window.isOpen())
    {
        window.clear(sf::Color(32,32,32));
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        sprite.move(sf::Vector2f(d(gen), d(gen)));
        window.draw(sprite);
        window.display();
    }

    return 0;
}
