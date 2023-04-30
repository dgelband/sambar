#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <random>
#include <iostream>

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

struct Sambar
{
    float x;
    float y;
    float rotation;
    sf::Texture texture;
};

struct Level
{
    sf::Texture texture;
};

struct Artwork
{
    sf::Texture basket1;
    sf::Texture basket2;
    sf::Texture crate1;
    sf::Texture crate2;
    sf::Texture level;
    sf::Texture sambar_left;
    sf::Texture sambar_right;
    sf::Texture sambar_side;
    sf::Texture sambar_top;
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

void destroyBox(b2Body* box) {
    world.DestroyBody(box);
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

// return true if box struck the ground
bool render(sf::RenderWindow &w, sf::View &side, sf::View &top, std::vector<Box> &boxes, Sambar &sambar, Level &level)
{
    // Side view - first box is ground, last box is a sambar
    side.setCenter(sf::Vector2f(boxes.back().body->GetPosition().x * PPM, 0.5f * WINDOW_HEIGHT));
    w.setView(side);
    w.clear(sf::Color(64,64,64));
    sf::RectangleShape sky(sf::Vector2f(WINDOW_WIDTH*0.3, WINDOW_HEIGHT*0.8));
    sky.setPosition(boxes.back().body->GetPosition().x * PPM - WINDOW_WIDTH*0.15, 0);
    sky.setFillColor(sf::Color::Cyan);
    w.draw(sky);
    b2Body* ground = boxes.front().body;
    b2Body* truck = boxes.back().body;

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

        // Check if we dropped a box
        if (box.body == ground || box.body == truck) {
            continue;
        }
        b2ContactEdge* edge = box.body->GetContactList();
        while (edge != nullptr) {
            if (edge->other == ground) {
                return true;
            }
            edge = edge->next;
        }
    }

    // Level map
    w.setView(top);
    top.setCenter(sf::Vector2f(0.5f * WINDOW_WIDTH, 0.5f * WINDOW_HEIGHT));

    sf::Sprite map;
    map.setPosition(0.5f * WINDOW_WIDTH, 0.5f * WINDOW_HEIGHT);
    map.setScale(1.8f, 1.8f);
    map.setOrigin(160,160);
    map.setTexture(level.texture);
    w.draw(map);

    // Top view
    sf::Sprite samsprite;
    samsprite.setPosition(sambar.x, WINDOW_HEIGHT - sambar.y);
    samsprite.setOrigin(16, 16);
    samsprite.setRotation(sambar.rotation);
    samsprite.setTexture(sambar.texture);
    w.draw(samsprite);

    w.display();

    return false;
}

bool reachedGoal(Sambar &sambar) {
    float x = sambar.x - WINDOW_WIDTH + 145;
    float y = sambar.y - 30;
    float dist = std::sqrt(x*x + y*y);  
    return dist < 30.0;
}

void runLevel(sf::RenderWindow &window, sf::View &topview, sf::View &sideview, int n_boxes, Artwork &art) {
    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::uniform_int_distribution<> d{0, 1000}; 

    // Container to hold all the boxes we create
    std::vector<Box> boxes;
    
    // Generate ground
    boxes.push_back(createGround(350, 50, 50000, 100, art.crate1));
    
    // Generate a lot of boxes
    sf::Texture *box_textures[] {&art.crate1, &art.crate2, &art.basket1, &art.basket2};
    for (int i = 0; i < n_boxes; i++)
    {
        // Starting positions are randomly generated: x between 74 and 86, y between 270 and 55*n boxes
        auto &&box = createBox(80 + (d(gen) % 6),
                               270 + (d(gen) % (72*n_boxes - 270 + 1)),
                               32,
                               24,
                               80.f,
                               0.7f,
                               *box_textures[(d(gen)) % 4]);
        boxes.push_back(box);
    }
    
    // Create a sambar box
    auto &&sambar = createBox(90, 200, 64, 64, 500.f, 0.7f, art.sambar_side);
    boxes.push_back(sambar);
    
    // Create a sambar from above
    Level level {.texture = art.level};
    Sambar sambar_top {.x = 155.0,
                       .y = 520.0,
                       .rotation = 180.0,
                       .texture = art.sambar_top};
    

    float fast = 10000.0;
    float reckless = 13000.0;
    float heave = 370000.0;
    float squat = 500000.0;

    float force = 0.f;
    float angular_impulse = 0.f;
    float rotation = 0.f;
    bool struck_ground = false;
    bool reached_goal = false;
    while (window.isOpen() && !struck_ground && !reached_goal)
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed) {
                switch(event.key.code) {
                    case sf::Keyboard::H:
                        // Strong reverse
                        force = -reckless;
                        angular_impulse = -squat;
                        break;
                    case sf::Keyboard::J:
                        // Reverse
                        force = -fast;
                        angular_impulse = -heave;
                        break;
                    case sf::Keyboard::K:
                        // Forward
                        force = fast;
                        angular_impulse = heave;
                        break;
                    case sf::Keyboard::L:
                        // Strong forward
                        force = reckless;
                        angular_impulse = squat;
                        break;
                    case sf::Keyboard::A:
                        // Left turn
                        sambar_top.texture = art.sambar_left;
                        rotation = -4.f;
                        break;
                    case sf::Keyboard::D:
                        // Right turn
                        sambar_top.texture = art.sambar_right;
                        rotation = 4.f;
                        break;
                }
            } else if (event.type == sf::Event::KeyReleased) {
                switch(event.key.code) {
                    case sf::Keyboard::A:
                    case sf::Keyboard::D:
                        // No turn
                        sambar_top.texture = art.sambar_top;
                        rotation = 0.f;
                        break;
                    case sf::Keyboard::H:
                        // Strong reverse if it was most recently pressed
                        force = force == -reckless ? 0 : force;
                        angular_impulse = angular_impulse == -squat ? 0 : angular_impulse;
                        break;
                    case sf::Keyboard::J:
                        // Reverse if it was most recently pressed
                        force = force == -fast ? 0 : force;
                        angular_impulse = angular_impulse == -heave ? 0 : angular_impulse;
                        break;
                    case sf::Keyboard::K:
                        // Forward if it was most recently pressed
                        force = force == fast ? 0 : force;
                        angular_impulse = angular_impulse == heave ? 0 : angular_impulse;
                        break;
                    case sf::Keyboard::L:
                        // Strong forward if it was most recently pressed
                        force = force == reckless ? 0 : force;
                        angular_impulse = angular_impulse == squat ? 0 : angular_impulse;
                        break;
                }
            }
        }
        // Apply updates to sambar side
        sambar.body->ApplyForceToCenter(b2Vec2(force, 10), true);
        sambar.body->ApplyAngularImpulse(angular_impulse, true);
    
        // Apply updates to sambar top
        sambar_top.rotation += rotation;
        auto & v = sambar.body->GetLinearVelocity();
        // We will only use horizontal component, not vertical
        sambar_top.x += std::sin(sambar_top.rotation / DEG_PER_RAD) * v.x;
        sambar_top.y += std::cos(sambar_top.rotation / DEG_PER_RAD) * v.x;
    
        world.Step(1 / 60.f , 6, 3);
        reached_goal = reachedGoal(sambar_top);
        struck_ground = render(window, sideview, topview, boxes, sambar_top, level);

        if (reached_goal || struck_ground) {
            for (auto & box : boxes) {
                destroyBox(box.body);
            }
        }
    }
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH,WINDOW_HEIGHT), "Sambar Scamper");
    window.setFramerateLimit(60);

    sf::View sideview(sf::FloatRect(0.f,0.f,0.3*WINDOW_WIDTH-1,1.0*WINDOW_HEIGHT));
    sf::View topview(sf::FloatRect(0.3*WINDOW_WIDTH+1,0.0,0.7*WINDOW_WIDTH,1.0*WINDOW_HEIGHT));
    sideview.setViewport(sf::FloatRect(0.f, 0.f, 0.3f, 1.0f));
    topview.setViewport(sf::FloatRect(0.3f, 0.f, 0.7f, 1.0f));

    sf::Texture sambar_texture;
    if (!sambar_texture.loadFromFile("img/sambar-side.png", sf::IntRect(0,32,128,128))) return -1;

    sf::Texture sambar_left_texture;
    if (!sambar_left_texture.loadFromFile("img/sambar-left.png")) return -1;

    sf::Texture sambar_right_texture;
    if (!sambar_right_texture.loadFromFile("img/sambar-right.png")) return -1;

    sf::Texture sambar_top_texture;
    if (!sambar_top_texture.loadFromFile("img/sambar-top.png")) return -1;

    sf::Texture crate1_texture;
    if (!crate1_texture.loadFromFile("img/crate-1.png", sf::IntRect(0,32,128,128))) return -1;
    
    sf::Texture crate2_texture;
    if (!crate2_texture.loadFromFile("img/crate-2.png", sf::IntRect(0,32,128,128))) return -1;
    
    sf::Texture basket1_texture;
    if (!basket1_texture.loadFromFile("img/basket-1.png", sf::IntRect(0,32,128,128))) return -1;
    
    sf::Texture basket2_texture;
    if (!basket2_texture.loadFromFile("img/basket-2.png", sf::IntRect(0,32,128,128))) return -1;

    sf::Texture ground_texture;
    if (!ground_texture.loadFromFile("img/basket-1.png", sf::IntRect(0,0,128,128))) return -1;

    sf::Texture level1_texture;
    if (!level1_texture.loadFromFile("img/level-1.png")) return -1;

    sf::Texture level2_texture;
    if (!level2_texture.loadFromFile("img/level-2.png")) return -1;

    sf::Texture level3_texture;
    if (!level3_texture.loadFromFile("img/level-3.png")) return -1;

    Artwork art { .basket1 = basket1_texture,
                  .basket2 = basket2_texture,
                  .crate1 = crate1_texture,
                  .crate2 = crate2_texture,
                  .level = level1_texture,
                  .sambar_left = sambar_left_texture,
                  .sambar_right = sambar_right_texture,
                  .sambar_side = sambar_texture,
                  .sambar_top = sambar_top_texture }; 

    sf::Texture *level_textures[] {&level1_texture, &level2_texture, &level3_texture};
    for (int n_level = 0; n_level < 3; n_level++) {
        int n_boxes = 2;
        art.level = *level_textures[n_level];
        while (window.isOpen() && n_boxes < 8) {
            runLevel(window, topview, sideview, n_boxes++, art);
        }
    }

    return 0;
}
