#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <random>
#include <string>
//#include <iostream>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// Pixels per meter. Box2D uses metric units, so we need to define a conversion
#define PPM 30.0F
// SFML uses degrees for angles while Box2D uses radians
#define DEG_PER_RAD 57.2957795F

// Box2D world for physics simulation, gravity = 9.8 m/s^2
b2World world(b2Vec2(0, -9.8));

// SFML font for text
sf::Font font;

int total = 0;

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

struct Obstacle
{
    float x;
    float y;
};

struct Level
{
    sf::Texture texture;
    std::vector<Obstacle> trees;
    std::vector<Obstacle> mud;
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

    std::string banner{"SCORE: "};
    banner += std::to_string(total);
    sf::Text text(banner, font);
    text.setLetterSpacing(1.3);
    text.setCharacterSize(72);
    text.setOutlineThickness(2.0);
    w.draw(text);

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

    // Debug - tree view
    for (const auto &tree : level.trees) {
        sf::CircleShape circ(20.0);
        circ.setPosition(sf::Vector2f(tree.x, WINDOW_HEIGHT - tree.y));
        circ.setOrigin(20, 20);
        circ.setFillColor(sf::Color::Green);
//        w.draw(circ);
    }

    // Debug - mud view
    for (const auto &mud : level.mud) {
        sf::CircleShape circ(40.0);
        circ.setPosition(sf::Vector2f(mud.x, WINDOW_HEIGHT - mud.y));
        circ.setOrigin(40,40);
        circ.setFillColor(sf::Color::Yellow);
//        w.draw(circ);
    }

    w.display();

    return false;
}

bool reachedGoal(Sambar &sambar) {
    float x = sambar.x - WINDOW_WIDTH + 145;
    float y = sambar.y - 30;
    float dist = std::sqrt(x*x + y*y);  
//    std::cout << "x: " << sambar.x << " y:" << sambar.y << std::endl;
    return dist < 30.0;
}

bool struckTree(Sambar &sambar, Level &level) {
    for (auto & tree : level.trees) {
        float x = sambar.x - tree.x;
        float y = sambar.y - tree.y;
        float dist = std::sqrt(x*x + y*y);  
        if (dist < 20.0) return true;
    }
    return false;
}

bool struckMud(Sambar &sambar, Level &level) {
    for (auto & mud : level.mud) {
        float x = sambar.x - mud.x;
        float y = sambar.y - mud.y;
        float dist = std::sqrt(x*x + y*y);  
        if (dist < 40.0) return true;
    }
    return false;
}

void runLevel(sf::RenderWindow &window, sf::View &topview, sf::View &sideview, int n_boxes, Artwork &art, Level &level) {
    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::uniform_int_distribution<> d{0, 1000}; 
    float sambar_density = 800.f;

    // Container to hold all the boxes we create
    std::vector<Box> boxes;
    
    // Generate ground
    boxes.push_back(createGround(350, 80, 50000, 100, art.crate1));
    
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
    auto &&sambar = createBox(90, 200, 72, 30, sambar_density, 0.7f, art.sambar_side);
    sambar.height = 64;
    boxes.push_back(sambar);
    
    // Create a sambar from above
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
        if (struckTree(sambar_top, level)) {
            // instant rebound, timestep 1/60
            b2Vec2 rebound(-sambar_density * 60. * 2. * sambar.body->GetLinearVelocity());
            sambar.body->ApplyForceToCenter(rebound, true);
        }
        if (struckMud(sambar_top, level)) {
            // instant slowdown, timestep 1/60
            b2Vec2 rebound(-sambar_density * 60. * 0.25 * sambar.body->GetLinearVelocity());
            sambar.body->ApplyForceToCenter(rebound, true);
        }
        reached_goal = reachedGoal(sambar_top);
        struck_ground = render(window, sideview, topview, boxes, sambar_top, level);

        if (reached_goal) total += n_boxes;
        if (reached_goal || struck_ground) {
            for (auto & box : boxes) {
                destroyBox(box.body);
            }
        }
    }
}

int main()
{
    font.loadFromFile("img/FreeMonoBold.ttf");
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH,WINDOW_HEIGHT), "Sambar Scamper");
    window.setFramerateLimit(60);

    sf::View sideview(sf::FloatRect(0.f,0.f,0.3*WINDOW_WIDTH-1,1.0*WINDOW_HEIGHT));
    sf::View topview(sf::FloatRect(0.3*WINDOW_WIDTH+1,0.0,0.7*WINDOW_WIDTH,1.0*WINDOW_HEIGHT));
    sideview.setViewport(sf::FloatRect(0.f, 0.f, 0.3f, 1.0f));
    topview.setViewport(sf::FloatRect(0.3f, 0.f, 0.7f, 1.0f));

    sf::Texture splash_texture;
    if (!splash_texture.loadFromFile("img/splash.png")) return -1;

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
                  .sambar_left = sambar_left_texture,
                  .sambar_right = sambar_right_texture,
                  .sambar_side = sambar_texture,
                  .sambar_top = sambar_top_texture }; 

    // TODO: store this appropriately
    Level levels[3];
    levels[0].texture = level1_texture;
    levels[0].trees.push_back(Obstacle{199.f, 531.f});
    levels[0].trees.push_back(Obstacle{372.f, 556.f});
    levels[0].trees.push_back(Obstacle{515.f, 556.f});
    levels[0].trees.push_back(Obstacle{659.f, 528.f});
    levels[0].trees.push_back(Obstacle{313.f, 441.f});
    levels[0].trees.push_back(Obstacle{480.f, 441.f});
    levels[0].trees.push_back(Obstacle{629.f, 382.f});
    levels[0].trees.push_back(Obstacle{199.f, 382.f});
    levels[0].trees.push_back(Obstacle{400.f, 382.f});
    levels[0].trees.push_back(Obstacle{198.f, 236.f});
    levels[0].trees.push_back(Obstacle{313.f, 294.f});
    levels[0].trees.push_back(Obstacle{514.f, 332.f});
    levels[0].trees.push_back(Obstacle{459.f, 272.f});
    levels[0].trees.push_back(Obstacle{256.f, 152.f});
    levels[0].trees.push_back(Obstacle{430.f, 152.f});
    levels[0].trees.push_back(Obstacle{511.f, 183.f});
    levels[0].trees.push_back(Obstacle{660.f, 183.f});
    levels[0].trees.push_back(Obstacle{545.f, 67.f});
    levels[0].trees.push_back(Obstacle{426.f, 37.f});
    levels[0].trees.push_back(Obstacle{285.f, 37.f});
    levels[0].trees.push_back(Obstacle{142.f, 37.f});
    levels[0].mud.push_back(Obstacle{210.f, 460.f});
    levels[0].mud.push_back(Obstacle{210.f, 302.f});
    levels[0].mud.push_back(Obstacle{210.f, 150.f});
    levels[0].mud.push_back(Obstacle{305.f, 210.f});
    levels[0].mud.push_back(Obstacle{305.f, 365.f});
    levels[0].mud.push_back(Obstacle{415.f, 216.f});
    levels[0].mud.push_back(Obstacle{230.f, 255.f});
    levels[0].mud.push_back(Obstacle{642.f, 200.f});
    levels[0].mud.push_back(Obstacle{210.f, 37.f});
    levels[0].mud.push_back(Obstacle{428.f, 84.f});
    levels[0].mud.push_back(Obstacle{519.f, 120.f});
    levels[0].mud.push_back(Obstacle{289.f, 527.f});
    levels[0].mud.push_back(Obstacle{521.f, 247.f});
    levels[0].mud.push_back(Obstacle{644.f, 304.f});
    levels[0].mud.push_back(Obstacle{522.f, 398.f});
    levels[0].mud.push_back(Obstacle{599.f, 524.f});
    levels[1].texture = level2_texture;
    levels[1].trees.push_back(Obstacle{247.f, 560.f});
    levels[1].trees.push_back(Obstacle{515.f, 560.f});
    levels[1].trees.push_back(Obstacle{625.f, 530.f});
    levels[1].trees.push_back(Obstacle{346.f, 530.f});
    levels[1].trees.push_back(Obstacle{227.f, 475.f});
    levels[1].trees.push_back(Obstacle{570.f, 445.f});
    levels[1].trees.push_back(Obstacle{169.f, 244.f});
    levels[1].trees.push_back(Obstacle{341.f, 210.f});
    levels[1].trees.push_back(Obstacle{485.f, 180.f});
    levels[1].trees.push_back(Obstacle{631.f, 154.f});
    levels[1].trees.push_back(Obstacle{424.f, 70.f});
    levels[1].trees.push_back(Obstacle{279.f, 124.f});
    levels[1].trees.push_back(Obstacle{143.f, 37.f});
    levels[1].mud.push_back(Obstacle{202.f, 391.f});
    levels[1].mud.push_back(Obstacle{243.f, 277.f});
    levels[1].mud.push_back(Obstacle{332.f, 359.f});
    levels[1].mud.push_back(Obstacle{414.f, 444.f});
    levels[1].mud.push_back(Obstacle{477.f, 368.f});
    levels[1].mud.push_back(Obstacle{406.f, 203.f});
    levels[1].mud.push_back(Obstacle{548.f, 300.f});
    levels[1].mud.push_back(Obstacle{617.f, 235.f});
    levels[1].mud.push_back(Obstacle{512.f, 57.f});
    levels[1].mud.push_back(Obstacle{491.f, 239.f});
    levels[1].mud.push_back(Obstacle{404.f, 296.f});
    levels[1].mud.push_back(Obstacle{470.f, 113.f});
    levels[1].mud.push_back(Obstacle{589.f, 171.f});
    levels[2].texture = level3_texture;
    levels[2].trees.push_back(Obstacle{227.f, 556.f});
    levels[2].trees.push_back(Obstacle{227.f, 500.f});
    levels[2].trees.push_back(Obstacle{201.f, 443.f});
    levels[2].trees.push_back(Obstacle{201.f, 382.f});
    levels[2].trees.push_back(Obstacle{255.f, 414.f});
    levels[2].trees.push_back(Obstacle{255.f, 354.f});
    levels[2].trees.push_back(Obstacle{343.f, 500.f});
    levels[2].trees.push_back(Obstacle{343.f, 442.f});
    levels[2].trees.push_back(Obstacle{400.f, 413.f});
    levels[2].trees.push_back(Obstacle{427.f, 471.f});
    levels[2].trees.push_back(Obstacle{513.f, 500.f});
    levels[2].trees.push_back(Obstacle{489.f, 411.f});
    levels[2].trees.push_back(Obstacle{456.f, 353.f});
    levels[2].trees.push_back(Obstacle{490.f, 300.f});
    levels[2].trees.push_back(Obstacle{395.f, 300.f});
    levels[2].trees.push_back(Obstacle{343.f, 270.f});
    levels[2].trees.push_back(Obstacle{230.f, 155.f});
    levels[2].trees.push_back(Obstacle{287.f, 99.f});
    levels[2].trees.push_back(Obstacle{319.f, 154.f});
    levels[2].trees.push_back(Obstacle{370.f, 184.f});
    levels[2].trees.push_back(Obstacle{460.f, 90.f});
    levels[2].trees.push_back(Obstacle{543.f, 185.f});
    levels[2].trees.push_back(Obstacle{164.f, 584.f});
    levels[2].trees.push_back(Obstacle{282.f, 584.f});
    levels[2].trees.push_back(Obstacle{340.f, 584.f});
    levels[2].trees.push_back(Obstacle{398.f, 584.f});
    levels[2].trees.push_back(Obstacle{456.f, 584.f});
    levels[2].trees.push_back(Obstacle{510.f, 584.f});
    levels[2].trees.push_back(Obstacle{568.f, 584.f});
    levels[2].trees.push_back(Obstacle{141.f, 267.f});
    levels[2].trees.push_back(Obstacle{141.f, 40.f});
    levels[2].trees.push_back(Obstacle{433.f, 40.f});
    levels[2].trees.push_back(Obstacle{485.f, 40.f});
    levels[2].trees.push_back(Obstacle{545.f, 12.f});
    levels[2].trees.push_back(Obstacle{373.f, 12.f});
    levels[2].trees.push_back(Obstacle{312.f, 12.f});
    levels[2].trees.push_back(Obstacle{257.f, 12.f});
    levels[2].trees.push_back(Obstacle{199.f, 12.f});
    levels[2].trees.push_back(Obstacle{121.f, 90.f});
    levels[2].trees.push_back(Obstacle{121.f, 180.f});
    levels[2].trees.push_back(Obstacle{121.f, 324.f});
    levels[2].trees.push_back(Obstacle{121.f, 383.f});
    levels[2].trees.push_back(Obstacle{121.f, 440.f});
    levels[2].trees.push_back(Obstacle{121.f, 498.f});
    levels[2].trees.push_back(Obstacle{121.f, 553.f});
    levels[2].trees.push_back(Obstacle{599.f, 381.f});
    levels[2].trees.push_back(Obstacle{599.f, 152.f});
    levels[2].trees.push_back(Obstacle{630.f, 95.f});
    levels[2].trees.push_back(Obstacle{630.f, 213.f});
    levels[2].trees.push_back(Obstacle{630.f, 330.f});
    levels[2].trees.push_back(Obstacle{630.f, 442.f});
    levels[2].trees.push_back(Obstacle{630.f, 556.f});
    levels[2].trees.push_back(Obstacle{658.f, 500.f});
    levels[2].trees.push_back(Obstacle{658.f, 386.f});
    levels[2].trees.push_back(Obstacle{658.f, 270.f});
    levels[2].trees.push_back(Obstacle{658.f, 154.f});
    levels[2].mud.push_back(Obstacle{233.f,253.f});
    levels[2].mud.push_back(Obstacle{455.f,198.f});



    // Display splash
    sf::Sprite splash;
    splash.setPosition(0.5*WINDOW_WIDTH, 0.5*WINDOW_HEIGHT);
    splash.setOrigin(0.5*WINDOW_WIDTH, 0.5*WINDOW_HEIGHT);
    splash.setTexture(splash_texture);
    bool key_pressed = false;
    while (window.isOpen() && !key_pressed)
    {
        window.clear();
        window.draw(splash);
        window.display();
        sf::Event event;
        while (window.pollEvent(event)){
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                key_pressed = true;
                break;
            }
        }
    }

    // Execute levels
    for (int n_level = 0; n_level < 3; n_level++) {
        int n_boxes = 2;
        while (window.isOpen() && n_boxes < 12) {
            runLevel(window, topview, sideview, n_boxes++, art, levels[n_level]);
        }
    }

    return 0;
}
