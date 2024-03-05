#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cmath>

struct GridPos {
    int x;
    int y;

    GridPos(int x, int y) : x(x), y(y) {}

    bool equals(GridPos gridPos) {
        return this->x == gridPos.x && this->y == gridPos.y;
    }
};

// Every other game object (Entity) must inherit from this class 🏛️
// It provides basic game object functionalities that are used a lot
// ... add functionality that all entities use here
class Entity {
public:
    int x = 0;
    int y = 0;
    float xVel = 0;
    float yVel = 0;
    float health = 100;

    sf::Texture texture;
    sf::Sprite sprite;

    int id = -1;
    std::string group = "entity";
    
    class Game* game = nullptr;

    Entity() {}

    // use ready() instead of the constructor since class Game* game; isn't defined there yet
    virtual void ready() {
        texture.loadFromFile("res/entity.png");
        sprite.setTexture(texture);
        sprite.setScale(sf::Vector2f(0.5f, 0.5f));
    }

    // defined below Game class because they use Game class functions
    virtual bool damage(float d);
    virtual void tick();
    GridPos getGridPos();
};


/*
The Game class holds all game objects as entities (std::vector<Entity*> entityCollection) and makes them tick() 🐒
It also offers commonly used functions and holds the game window (sf::RenderWindow& gameWindow)
To access it's members in an Entity use game->handyFunction(x,y);

Manage entities at runtime:
    int createEntity(Entity* entity)
    void destroyEntity(int entityId)

Collision checks:
    std::vector<Entity*> getCollisions(int x, int y, int hitRadius, const std::string& groupFilter = "")
    std::vector<Entity*> getGridCollisions(const GridPos collision, const std::string& groupFilter = "")
    bool hasGridCollision(const GridPos gridPos, const std::string& groupFilter = "")

Switching position units:
    float snapOnGrid(float v)   139 -> 150
    float gridToFree(int g)     2 -> 100
    int freeToGrid(float f)     110 -> 2

Misc:
    float deltaTime()   Time between frames, multiply this with velocity

... add commonly used functions to this class 🦅
*/
class Game {
public:
    sf::RenderWindow& gameWindow;
    std::vector<Entity*> entityCollection;

    float FRAME_RATE = 60.f;
    int GRID_SPACE = 50;
    int WINDOW_WIDTH;
    int WINDOW_HEIGHT;

    sf::Vector2i mousePos;
    sf::Clock frameClock;
    sf::Clock gameClock;
    sf::Font font;

    bool buildModeActive = true;
    int uniqueId = 0;
    
    bool testDestroy = false;

    Game(sf::RenderWindow& window) : gameWindow(window) {
        WINDOW_WIDTH = gameWindow.getSize().x;
        WINDOW_HEIGHT = gameWindow.getSize().y;
        font.loadFromFile("res/arial.ttf");
    }

    ~Game() {
        for (Entity* entity : entityCollection) {
            delete entity;
        }
    }

    float deltaTime() {
        return 1.f / FRAME_RATE;
    }

    float gridToFree(int g) {
        return g * GRID_SPACE;
    }

    int freeToGrid(float f) {
        return f / GRID_SPACE;
    }

    float snapOnGrid(float v) {
        return gridToFree(freeToGrid(v));
    }

    int createEntity(Entity* entity) {
        entity->id = uniqueId++;
        entity->game = this; // Set the game pointer

        entityCollection.push_back(entity);
        entity->ready();
        return entity->id;
    }

    void destroyEntity(int entityId) {
        entityCollection.erase(std::remove_if(entityCollection.begin(), entityCollection.end(),
                        [entityId](const Entity* entity) { return entity->id == entityId; }),
                        entityCollection.end());
    }

    std::vector<Entity*> getCollisions(int x, int y, int hitRadius, const std::string& groupFilter = "") {
        std::vector<Entity*> collisions;

        for (Entity* entity : entityCollection) {
            // Check if the entity matches the filter and is within the hit radius
            // collision damage should be deltaTime sensitive
            if ((groupFilter == "" || entity->group == groupFilter) &&
                std::hypot(entity->x - x, entity->y - y) <= hitRadius) {
                collisions.push_back(entity);
            }
        }

        return collisions;
    }

    std::vector<Entity*> getGridCollisions(const GridPos collision, const std::string& groupFilter = "") {
        std::vector<Entity*> collisions;

        for (Entity* entity : entityCollection) {
            if ((groupFilter == "" || entity->group == groupFilter) && entity->getGridPos().equals(collision)) {
                collisions.push_back(entity);
            }
        }

        return collisions;
    }

    bool hasGridCollision(const GridPos gridPos, const std::string& groupFilter = "") {
        for (Entity* entity : entityCollection) {
            if (entity->getGridPos().equals(gridPos) && (groupFilter == "" || entity->group == groupFilter)) {
                return true;
            }
        }
        return false;
    }

    void buildMode(bool destroy = false) {
        sf::RectangleShape area;
        // Set selection square pulsating color
        sf::Color areaColor;
        if (destroy)
            areaColor = sf::Color(255, 150, 150, sin(gameClock.getElapsedTime().asSeconds() * 5) * 100 + 150);
        else
            areaColor = sf::Color(150, 150, 255, sin(gameClock.getElapsedTime().asSeconds() * 5) * 100 + 150);
        area.setFillColor(areaColor);
        // Draw selection square
        area.setSize(sf::Vector2f(GRID_SPACE, GRID_SPACE));
        area.setPosition(snapOnGrid(mousePos.x),snapOnGrid(mousePos.y));
        gameWindow.draw(area);

        // Do action on click & leave build mode
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            GridPos gridPos(freeToGrid(mousePos.x), freeToGrid(mousePos.y));

            if (destroy) {
                for (Entity* entity : getGridCollisions(gridPos, "plant")) {
                    destroyEntity(entity->id);

                    buildModeActive = false;
                }
            } else {
                if (!hasGridCollision(gridPos, "plant")) {
                    Entity* entity = new Entity();  //todo: how can i use plant here (maybe function declaration above game and implementation below plant)
                    entity->x = gridToFree(gridPos.x);
                    entity->y = gridToFree(gridPos.y);
                    createEntity(entity);

                    buildModeActive = false;
                    testDestroy = true;
                }
            }
        }
    }

    bool startGame() {
        while (gameWindow.isOpen()) {
            sf::Event event;

            while (gameWindow.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    gameWindow.close();
                }
            }

            // Update game logic at FRAME_RATE
            if (frameClock.getElapsedTime().asSeconds() >= deltaTime()) {
                // This happens every tick

                gameWindow.clear();
                mousePos = sf::Mouse::getPosition(gameWindow);

                if (buildModeActive)
                    buildMode(testDestroy);

                for (Entity* entity : entityCollection) {
                    entity->tick();
                }

                gameWindow.display();
                frameClock.restart();
                //todo: save ressources by leting the thwead sleep when waiting 🛌🥺💤
            }
        }
        return false;
    }
};


void Entity::tick() {
    x += xVel * game->deltaTime();
    y += yVel * game->deltaTime();
    sprite.setPosition(x, y);
    game->gameWindow.draw(sprite);

    //debugging info (to be removed)
    sf::Text groupText;
    groupText.setFont(game->font);
    groupText.setString(group);
    groupText.setCharacterSize(20);
    groupText.setPosition(x, y);
    game->gameWindow.draw(groupText);
    sf::Text healthText;
    healthText.setFont(game->font);
    healthText.setString(std::to_string((int)health));
    healthText.setCharacterSize(16);
    healthText.setPosition(x, y + 20.f);
    game->gameWindow.draw(healthText);
}

bool Entity::damage(float d) {
    health -= d;
    if (health <= 0) {
        game->destroyEntity(id);
        return true;
    }
    return false;
}

GridPos Entity::getGridPos() {
    return GridPos(game->freeToGrid(x), game->freeToGrid(y));
}

// ---------------------------- GAME ENTITIES ------------------------------


class Zombie : public Entity {
protected:
    float knockback = 0.f;
    int startingGridRow = 1;
    float xVelNormal = -100.f;
    float damageDonePerSec = 35.f;

public:
    Zombie(int startingGridRow) : startingGridRow(startingGridRow) {}

    void ready() override {
        group = "zombie";

        texture.loadFromFile("res/Zombie.png");
        sprite.setTexture(texture);
        sprite.setScale(sf::Vector2f(0.5f, 0.5f));

        y = game->gridToFree(startingGridRow);
        x = game->WINDOW_WIDTH;
        xVel = xVelNormal;
    }

    bool damage(float d) override {
        knockback += d / 2;

        // Call Parent's (Entity's) base implementation
        return Entity::damage(d);
    }

    void tick() override {
        // mhhh yummieyum.. let me see if theres a plant i can take a bite off 🧟
        if (game->hasGridCollision(getGridPos(), "plant")) {
            xVel = 0.f;
            std::vector<Entity*> collisions = game->getGridCollisions(getGridPos(), "plant");
            for (Entity* hit : collisions) {
                hit->damage(damageDonePerSec * game->deltaTime());
            }
        } else {
            xVel = xVelNormal;
        }

        x += knockback;
        if (knockback > 0.f)
            knockback -= game->deltaTime() * 15.f;

        // Always call Entity's tick
        Entity::tick();
    }

    // Zombie specific functions
    int getGridRow() const { return game->freeToGrid(x); }
    float getProgressLocation() const { return game->WINDOW_WIDTH - x; }
};


class Projectile : public Entity {
protected:
    float lifeSpan = 1.0f;
    int damageDone = 15;
    float baseVelocity = 1000.f;

    int gridRow;
    int gridColumn;
    float lifeTimer = 0.f;

public:
    Projectile(int gridRow, int gridColumn) : gridRow(gridRow), gridColumn(gridColumn) {}

    void ready() override {
        Entity::ready();
        group = "projectile";

        x = game->gridToFree(gridColumn);
        y = game->gridToFree(gridRow) - 20.f;
        xVel = baseVelocity;
    }

    void tick() override {
        Entity::tick();

        // imitate physics
        xVel -= game->deltaTime() * 200;
        yVel += game->deltaTime() * 200;

        lifeTimer += game->deltaTime();
        if (lifeTimer >= lifeSpan)
            game->destroyEntity(id);

        // check for colliding zombies; damage & destroy self
        std::vector<Entity*> hits = game->getCollisions(x, y, 25.f, "zombie");
        for (Entity* zombie : hits) {
            zombie->damage(damageDone);
            game->destroyEntity(id);
        }
    }
};


class Plant : public Entity {
protected:
    int gridRow;
    int gridColumn;
    float attackSpeed = 2.f;
    float attackTimer = 0.f;

public:
    Plant(int gridRow, int gridColumn) : gridRow(gridRow), gridColumn(gridColumn) {}

    void ready() override {
        group = "plant";

        texture.loadFromFile("res/Plant.png");
        sprite.setTexture(texture);
        sprite.setScale(sf::Vector2f(0.5f, 0.5f));

        x = game->gridToFree(gridColumn);
        y = game->gridToFree(gridRow);
    }

    void tick() override {
        attackTimer += game->deltaTime();

        if (attackTimer >= attackSpeed) {
            Projectile* projectile = new Projectile(gridRow, gridColumn);
            game->createEntity(projectile);
            attackTimer = 0.f;
        }

        Entity::tick();
    }
};

class ProductionPlant : public Plant {
private:
    float productionRate = 10.f;
    float productionTimer = 0.f;

public:
    ProductionPlant(int gridRow, int gridColumn) : Plant(gridRow, gridColumn) {}

    void ready() override {
        Plant::ready();
        attackSpeed = 999999999.f;
    }

    void tick() override {
        productionTimer += game->deltaTime();
        if (productionTimer >= productionRate)
            produce();

        Plant::tick();
    }

    void produce() {
        std::cout << "Banana produced!" << std::endl;
        game->buildModeActive = true;
        productionTimer = 0.f;
    }
};



// Entry point function
int main() {
    
    sf::RenderWindow window(sf::VideoMode(1200, 800), "Protect The Jungle: monkeys fight back!");
    Game* game = new Game(window);

    // display menu

    // add demo entitys
    Plant* plant = new Plant(2,2);
    game->createEntity(plant);

    Plant* plant1 = new Plant(4, 1);
    game->createEntity(plant1);

    ProductionPlant* productionPlant = new ProductionPlant(3,1);
    game->createEntity(productionPlant);

    Zombie* zombie = new Zombie(2);
    game->createEntity(zombie);

    Zombie* zombie1 = new Zombie(2);
    game->createEntity(zombie1);
    zombie1->x += 100.f;

    Zombie* zombie2 = new Zombie(4);
    game->createEntity(zombie2);
    zombie2->x += 200.f;

    if (game->startGame()) {
        // victory
    } else {
        // game lost
    }

    delete game;
}
