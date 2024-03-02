#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cmath>


// Every other game object must inherit from this class 🏛️
// It provides basic game object functionalities that are used a lot
class Entity {
public:
    int x = 0;
    int y = 0;
    float xVel = 0;
    float yVel = 0;
    int health = 100;

    sf::Texture texture;
    sf::Sprite sprite;

    int id = -1;
    std::string group = "entity";
    
    class Game* game;

    Entity() {}

    virtual void ready() {
        texture.loadFromFile("res/entity.png");
        sprite.setTexture(texture);
        sprite.setScale(sf::Vector2f(0.5f, 0.5f));
    }

    // defined below Game class because they use Game class functions
    virtual bool damage(int d);
    virtual void tick();
};


/*
The Game class holds all game objects as entities and makes them tick()
It also offers commonly used functions
...and just about everything else... 🐒

It offers:
    createEntity(entity); -> entityId
    destroyEntity(entityId);

    getCollisions(x, y, hitRadius, ?groupFilter); -> std::vector<Entity*>

    deltaTime(); -> float (Time between frames, multiply this with velocity)

    snapToGrid(v); -> float  (139 -> 150)
    indexToGrid(i); -> float  (2 -> 100)
    posToIndex(v); -> int  (110 -> 2)

    ... add commonly used functions to this class 🦅

todo: fix 4+ entities memory leak
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

    bool buildModeActive = true;
    int uniqueId = 0;

    Game(sf::RenderWindow& window) : gameWindow(window) {
        WINDOW_WIDTH = gameWindow.getSize().x;
        WINDOW_HEIGHT = gameWindow.getSize().y;
    }

    ~Game() {
        for (Entity* entity : entityCollection) {
            delete entity;
        }
    }

    float deltaTime() {
        return 1.f / FRAME_RATE;
    }

    float indexToGrid(int i) {
        return i * GRID_SPACE;
    }

    float posToIndex(float v) {
        return v / GRID_SPACE;
    }

    float snapToGrid(float v) {
        return indexToGrid(posToIndex(v));
    }

    int createEntity(Entity* entity) {
        entity->id = uniqueId++;
        entity->game = this; // Set the game pointer

        entityCollection.push_back(entity);
        entity->ready();
        return entity->id;
    }

    void destroyEntity(int entityId) {
        auto it = std::remove_if(entityCollection.begin(), entityCollection.end(),
                                [entityId](const Entity* entity) { return entity->id == entityId; });

        if (it != entityCollection.end()) {
            delete *it;
            entityCollection.erase(it, entityCollection.end());
        }
    }

    std::vector<Entity*> getCollisions(int x, int y, int hitRadius, const std::string& groupFilter = "") {
        std::vector<Entity*> collisions;

        for (Entity* entity : entityCollection) {
            // Check if the entity matches the filter and is within the hit radius
            // could be made more accurate & collision damage should be deltaTime sensitive
            if ((entity->group == "" || entity->group == groupFilter) &&
                std::hypot(entity->x - x, entity->y - y) <= hitRadius) {
                collisions.push_back(entity);
            }
        }

        return collisions;
    }

    void buildMode() {
        // Draw preview/selected square
        sf::RectangleShape sprite;
        sprite.setSize(sf::Vector2f(GRID_SPACE, GRID_SPACE));
        sprite.setFillColor(sf::Color(150, 150, 255, sin(gameClock.getElapsedTime().asSeconds() * 5) * 100 + 150));
        sprite.setPosition(snapToGrid(mousePos.x),snapToGrid(mousePos.y));
        gameWindow.draw(sprite);

        // Place plant and leave build mode
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            //todo: check if slot is empty
            Entity* entity = new Entity();  //todo: how can i use plant here
            entity->x = snapToGrid(mousePos.x);
            entity->y = snapToGrid(mousePos.y);
            createEntity(entity);

            buildModeActive = false;
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
                    buildMode();

                for (Entity* entity : entityCollection) {
                    entity->tick();
                }

                gameWindow.display();
                frameClock.restart();
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
}

bool Entity::damage(int d) {
    health -= d;
    if (health <= 0) {
        game->destroyEntity(id);
        return true;
    }
    return false;
}


// ---------------------------- GAME ENTITIES ------------------------------


class Zombie : public Entity {
protected:
    float knockback = 0.f;
    int startingGridRow = 1;

public:
    Zombie(int startingGridRow) : startingGridRow(startingGridRow) {}

    void ready() override {
        group = "zombie";

        texture.loadFromFile("res/Zombie.png");
        sprite.setTexture(texture);
        sprite.setScale(sf::Vector2f(0.5f, 0.5f));

        y = game->indexToGrid(startingGridRow);
        x = game->WINDOW_WIDTH;
        xVel = -100.f;
    }

    bool damage(int d) override {
        knockback += d / 2;

        // Call Parent's (Entity's) base implementation
        return Entity::damage(d);
    }

    void tick() override {
        x += knockback;
        if (knockback > 0.f)
            knockback -= game->deltaTime() * 15.f;

        // Always call Entity's tick
        Entity::tick();
    }

    // Zombie specific functions
    int getGridRow() const { return game->posToIndex(x); }
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

        x = game->indexToGrid(gridColumn);
        y = game->indexToGrid(gridRow) - 20.f;
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
        std::vector<Entity*> hits = game->getCollisions(x, y, 20.f, "zombie");
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

        x = game->indexToGrid(gridColumn);
        y = game->indexToGrid(gridRow);
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
    ProductionPlant(int gridRow, int gridColumn) : Plant(gridRow, gridColumn) {
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

    // add sample entitys
    Plant* plant = new Plant(2,2);
    game->createEntity(plant);

    ProductionPlant* productionPlant = new ProductionPlant(4,1);
    game->createEntity(productionPlant);

    Zombie* zombie = new Zombie(2);
    game->createEntity(zombie);

    if (game->startGame()) {
        // victory
    } else {
        // game lost
    }

    delete game;
}
