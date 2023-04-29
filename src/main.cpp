#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <random>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// Pixels per meter. Box2D uses metric units, so we need to define a conversion
#define PPM 30.0F
// SFML uses degrees for angles while Box2D uses radians
#define DEG_PER_RAD 57.2957795F

// Box2D world for physics simulation, gravity = 9.8 m/s^2
b2World world(b2Vec2(0, -9.8));

// A structure with all we need to render a box
struct Box
{
	float width;
	float height;
	sf::Texture texture;
	b2Body *body;
};

Box createBox(float x, float y, float width, float height, float density, float friction, sf::Texture &texture)
{
	// Body definition
	b2BodyDef boxBodyDef;
	boxBodyDef.position.Set(x / PPM, y / PPM);
	boxBodyDef.type = b2_dynamicBody;
    boxBodyDef.angularDamping = 100000.0f;

	// Shape definition
	b2PolygonShape boxShape;
	boxShape.SetAsBox(width / 2 / PPM, height / 2 / PPM);

	// Fixture definition
	b2FixtureDef fixtureDef;
	fixtureDef.density = density;
	fixtureDef.friction = friction;
	fixtureDef.shape = &boxShape;

	// Now we have a body for our Box object
	b2Body *boxBody = world.CreateBody(&boxBodyDef);
	// Lastly, assign the fixture
	boxBody->CreateFixture(&fixtureDef);

	return Box { width, height, texture, boxBody };
}

Box createGround(float x, float y, float width, float height, sf::Texture &texture)
{
	// Static body definition
	b2BodyDef groundBodyDef;
	groundBodyDef.position.Set(x / PPM, y / PPM);

	// Shape definition
	b2PolygonShape groundBox;
	groundBox.SetAsBox(width / 2 / PPM, height / 2 / PPM);

	// Now we have a body for our Box object
	b2Body *groundBody = world.CreateBody(&groundBodyDef);
	// For a static body, we don't need a custom fixture definition, this will do:
	groundBody->CreateFixture(&groundBox, 0.0f);

	return Box{ width, height, texture, groundBody };
}

void render(sf::RenderWindow &w, std::vector<Box> &boxes)
{
	w.clear();
	for (const auto &box : boxes)
	{
        sf::Sprite rect;

		// For the correct Y coordinate of our drawable rect, we must substract from WINDOW_HEIGHT
		// because SFML uses OpenGL coordinate system where X is right, Y is down
		// while Box2D uses traditional X is right, Y is up
		rect.setPosition(box.body->GetPosition().x * PPM, WINDOW_HEIGHT - (box.body->GetPosition().y * PPM));

		// We also need to set our drawable's origin to its center
		// because in SFML, "position" refers to the upper left corner
		// while in Box2D, "position" refers to the body's center
		rect.setOrigin(box.width / 2, box.height / 2);

		// For the rect to be rotated in the crrect direction, we have to multiply by -1
		rect.setRotation(-1 * box.body->GetAngle() * DEG_PER_RAD);

        rect.setTexture(box.texture);
		w.draw(rect);
	}
	w.display();
}

int main()
{

    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::uniform_int_distribution<> d{0, 1000}; 
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH,WINDOW_HEIGHT), "Sambar Scamper");
    window.setFramerateLimit(60);

    sf::Texture sambar_texture;
    if (!sambar_texture.loadFromFile("img/sambar-side.png", sf::IntRect(0,32,128,128))) return -1;

    sf::Texture crate1_texture;
    if (!crate1_texture.loadFromFile("img/crate-1.png", sf::IntRect(0,32,128,128))) return -1;
    
    sf::Texture crate2_texture;
    if (!crate2_texture.loadFromFile("img/crate-2.png", sf::IntRect(0,32,128,128))) return -1;
    
    sf::Texture basket1_texture;
    if (!basket1_texture.loadFromFile("img/basket-1.png", sf::IntRect(0,32,128,128))) return -1;
    
    sf::Texture basket2_texture;
    if (!basket2_texture.loadFromFile("img/basket-2.png", sf::IntRect(0,32,128,128))) return -1;

    sf::Texture *textures[] {&crate1_texture, &crate2_texture, &basket1_texture, &basket2_texture};
    
	// Container to hold all the boxes we create
	std::vector<Box> boxes;

	// Generate ground
	boxes.push_back(createGround(350, 50, 80000, 100, basket2_texture));

	// Generate a lot of boxes
	for (int i = 0; i < 4; i++)
	{
		// Starting positions are randomly generated: x between 72 and 88, y between 270 and 550
		auto &&box = createBox(80 + (d(gen) % 8),
                               270 + (d(gen) % (550 - 270 + 1)),
                               32,
                               24,
                               80.f,
                               0.7f,
                               *textures[(d(gen)) % 4]);
		boxes.push_back(box);
	}

	// Create a sambar
	auto &&sambar = createBox(90, 200, 64, 64, 150.f, 0.7f, sambar_texture);
	boxes.push_back(sambar);

    while (window.isOpen())
    {
        //window.clear(sf::Color(32,32,32));
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::H))
	            sambar.body->ApplyForceToCenter(b2Vec2(-15000, 0), false);
            if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::J))
	            sambar.body->ApplyForceToCenter(b2Vec2(-10000, 0), false);
            if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::K))
	            sambar.body->ApplyForceToCenter(b2Vec2(10000, 0), false);
            if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::L))
	            sambar.body->ApplyForceToCenter(b2Vec2(15000, 0), false);
        }
        world.Step(1 / 60.f , 6, 3);
        render(window, boxes);
    }

    return 0;
}
