#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <functional>


class Button {
public:
    Button(sf::Vector2f position, sf::Vector2f size, const std::string& text, const sf::Color& color, std::function<void()> onClick)
        : m_color(color), m_colorHover(color.r*1.3,color.g*1.3,color.b*1.3), m_onClick(onClick) {
        m_shape.setPosition(position);
        m_shape.setSize(size);
        m_shape.setFillColor(color);
        m_font.loadFromFile("res/arial.ttf");
        m_text.setFont(m_font);
        m_text.setString(text);
        m_text.setCharacterSize(22);
        m_text.setFillColor(sf::Color::White);
        sf::FloatRect textRect = m_text.getLocalBounds();
        m_text.setOrigin(textRect.left + textRect.width / 2.0f,
                         textRect.top + textRect.height / 2.0f);
        m_text.setPosition(position.x + size.x / 2.0f, position.y + size.y / 2.0f);
    }

    void handleEvent(sf::Event event, sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed) {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            if (m_shape.getGlobalBounds().contains(mousePos))
                m_onClick();
        }
        else if (event.type == sf::Event::MouseMoved) {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            if (m_shape.getGlobalBounds().contains(mousePos))
                m_shape.setFillColor(m_colorHover);
            else
                m_shape.setFillColor(m_color);
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(m_shape);
        window.draw(m_text);
    }

private:
    sf::RectangleShape m_shape;
    sf::Text m_text;
    sf::Font m_font;
    sf::Color m_color;
    sf::Color m_colorHover;
    std::function<void()> m_onClick;
};


struct GridPos {
    int x;
    int y;

    GridPos(int x, int y) : x(x), y(y) {}

    bool equals(GridPos gridPos) {
        return this->x == gridPos.x && this->y == gridPos.y;
    }

    bool sameYBiggerX (GridPos gridPos) {
        return this->x <= gridPos.x && this->y == gridPos.y;
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
    }

    // defined below Game class because they use Game class functions
    virtual bool damage(float d);
    virtual void tick();
    GridPos getGridPos();
    void setGridPos(GridPos gridPos);
};


/*
The Game class holds all game objects as entities (std::vector<Entity*> entityCollection) and makes them tick() 🐒
It also offers commonly used functions and holds a reference to the game window (sf::RenderWindow& gameWindow) ✈️
To access it's members in an Entity use the game pointer: game->handyFunction(x,y);

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
    int GRID_SPACE = 84;
    int GRID_ROWS = 8;
    int WINDOW_WIDTH;
    int WINDOW_HEIGHT;

    sf::Vector2i mousePos;
    sf::Clock frameClock;
    sf::Clock gameClock;
    sf::Font font;
    sf::Time delta;
    int uniqueId = 0;

    int bananaCount = 0;
    int editMode = 0; // 0: sleep, 1: build, 2: destroy
    int selectedPlant = 0;

    Game(sf::RenderWindow& window) : gameWindow(window) {
        WINDOW_WIDTH = gameWindow.getSize().x;
        WINDOW_HEIGHT = gameWindow.getSize().y;
        font.loadFromFile("res/arial.ttf");

        // do not ask, do not change; it fixes the crashes (keep it on the low)
        for (int i = 0; i < 9; i++) {
            entityCollection.push_back(nullptr);
        }
        entityCollection.clear();

        // fix thread sleep stutters on windows:
        // SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    }

    ~Game() {
        for (Entity* entity : entityCollection) {
            destroyEntity(entity->id);
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

    bool hasZombieOnRowBefore(GridPos gridPos) {
        for (Entity* entity : entityCollection) {
            GridPos entityGridPos = entity->getGridPos();
            if(gridPos.sameYBiggerX(entityGridPos) && entity->group == "zombie") {
                return true;
            }
        }
        return false;
    }

    void renderMouseSelection(){
        if (editMode == 0)
            return;
        sf::RectangleShape area;

        // Set selection square pulsating color
        sf::Color areaColor;
        if (editMode == 2)
            areaColor = sf::Color(255, 100, 100, sin(gameClock.getElapsedTime().asSeconds() * 5) * 100 + 150);
        else if(editMode == 1)
            areaColor = sf::Color(120, 150, 255, sin(gameClock.getElapsedTime().asSeconds() * 5) * 100 + 150);
        area.setFillColor(areaColor);

        // Draw selection square
        area.setSize(sf::Vector2f(GRID_SPACE, GRID_SPACE));
        area.setPosition(snapOnGrid(mousePos.x),snapOnGrid(mousePos.y));
        gameWindow.draw(area);
    }

    int placePlant();

    void removePlant() {
        GridPos gridPos(freeToGrid(mousePos.x), freeToGrid(mousePos.y));

        for (Entity* entity : getGridCollisions(gridPos, "plant")) {
            destroyEntity(entity->id);
        }
    }
    
    bool startGame() {
        sf::Clock sleepClock;

        sf::Texture fieldTexture;
        if (!fieldTexture.loadFromFile("./res/bgGameField.png")) {
            return EXIT_FAILURE;
        }
        sf::Sprite fieldSprite(fieldTexture);
        fieldSprite.setPosition(sf::Vector2f(0.f, 0.f));
        fieldSprite.setScale(5.27f, 5.27f);

        sf::Texture barTexture;
        if (!barTexture.loadFromFile("./res/bgActionBar.png")) {
            return EXIT_FAILURE;
        }
        sf::Sprite barSprite(barTexture);
        barSprite.setPosition(sf::Vector2f(0.f, 675.f));
        barSprite.setScale(0.84,0.84);

        Button destroyButton(sf::Vector2f(0.f, 720.f), sf::Vector2f(150.f, 80.f), "Remove", sf::Color(180, 50, 50), [this]{
                editMode = 2;
            });

        Button plantButton(sf::Vector2f(250.f, 720.f), sf::Vector2f(125.f, 80.f), "Plant", sf::Color(50, 180, 50), [this]{
                editMode = 1;
                selectedPlant = 0;
            });

        Button plant1Button(sf::Vector2f(400.f, 720.f), sf::Vector2f(125.f, 80.f), "Production\nPlant", sf::Color(50, 180, 50), [this]{
                editMode = 1;
                selectedPlant = 1;
            });

        Button plant2Button(sf::Vector2f(550.f, 720.f), sf::Vector2f(125.f, 80.f), "Plant", sf::Color(50, 180, 50), [this]{
                editMode = 1;
                selectedPlant = 2;
            });

        sf::Text bananasCountText;
        bananasCountText.setFont(font);
        bananasCountText.setCharacterSize(24);
        bananasCountText.setFillColor(sf::Color::White);
        bananasCountText.setPosition(10, 10);

        // Update game logic at FRAME_RATE
        while (gameWindow.isOpen()) {
            sleepClock.restart();

            // poll input
            sf::Event event;
            while (gameWindow.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    gameWindow.close();
                destroyButton.handleEvent(event, gameWindow);
                plantButton.handleEvent(event, gameWindow);
                plant1Button.handleEvent(event, gameWindow);
                plant2Button.handleEvent(event, gameWindow);
            }
            mousePos.x = sf::Mouse::getPosition(gameWindow).x * ((float)WINDOW_WIDTH / gameWindow.getSize().x);
            mousePos.y = sf::Mouse::getPosition(gameWindow).y * ((float)WINDOW_HEIGHT / gameWindow.getSize().y);

            gameWindow.clear();

            // draw static elements
            gameWindow.draw(fieldSprite);
            gameWindow.draw(barSprite);

            // make entities tick
            for (Entity* entity : entityCollection) {
                entity->tick();
            }

            // editing
            if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
                editMode = 0;
            if (mousePos.y < gridToFree(GRID_ROWS)) {
                renderMouseSelection();
                if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                    if (editMode == 1) {
                        int price = placePlant();
                        if (bananaCount < price) {
                            removePlant();
                        } else {
                            bananaCount -= price;
                        }
                    }
                    else if (editMode == 2) {
                        removePlant();
                    }
                }
            }

            bananasCountText.setString("Bananas to your name: " + std::to_string(bananaCount));

            // draw static elements
            plantButton.draw(gameWindow);
            destroyButton.draw(gameWindow);
            plant1Button.draw(gameWindow);
            plant2Button.draw(gameWindow);

            gameWindow.draw(bananasCountText);

            gameWindow.display();

            // let our thread sleep until dawn of new frame
            sf::Time remainingTime = sf::Time(sf::seconds(deltaTime())) - sleepClock.getElapsedTime();
            if (remainingTime > sf::Time::Zero)
                sf::sleep(remainingTime);
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

void Entity::setGridPos(GridPos gridPos) {
    x = game->gridToFree(gridPos.x);
    y = game->gridToFree(gridPos.y);
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

    GridPos initGridPos;
    float lifeTimer = 0.f;

public:
    Projectile(GridPos gridPos) : initGridPos(gridPos) {}

    void ready() override {
        Entity::ready();
        group = "projectile";
        sprite.setScale(0.7f, 0.7f);

        x = game->gridToFree(initGridPos.x);
        y = game->gridToFree(initGridPos.y) - 20.f;
        xVel = baseVelocity;
    }

    void tick() override {
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

        Entity::tick();
    }
};


class Plant : public Entity {
protected:
    float attackSpeed = 2.f;
    float attackTimer = 0.f;

    GridPos initGridPos;

public:
    int price = 1;
    Plant(GridPos gridPos) : initGridPos(gridPos) {}

    void ready() override {
        group = "plant";
        price = 2;

        texture.loadFromFile("res/Plant.png");
        sprite.setTexture(texture);

        setGridPos(initGridPos);
    }

    void tick() override {
        attackTimer += game->deltaTime();
        bool isReady;

        if (attackTimer >= attackSpeed) {
            isReady = true;
            attackTimer = 0.f;
        }

        if (isReady && game->hasZombieOnRowBefore(this->getGridPos())){
            Projectile* projectile = new Projectile(this->getGridPos());
            game->createEntity(projectile);
            isReady = false;
        }

        Entity::tick();
    }
};

class ProductionPlant : public Plant {
private:
    float productionDelay = 5.f;
    float productionTimer = 0.f;
    int productionAmount = 2;

public:
    ProductionPlant(GridPos gridPos) : Plant(gridPos) {}

    void ready() override {
        Plant::ready();
        price = 4;
    }

    void tick() override {
        productionTimer += game->deltaTime();
        if (productionTimer >= productionDelay)
            produce();

        Entity::tick();
    }

    void produce() {
        game->bananaCount += productionAmount;
        productionTimer = 0.f;
    }
};


int Game::placePlant() {
  GridPos gridPos(freeToGrid(mousePos.x), freeToGrid(mousePos.y));
  if (!hasGridCollision(gridPos, "plant")) {
      Plant* plant;
      switch (selectedPlant) {
        case 0:
            plant = new Plant(gridPos);
            break;
        case 1:
            plant = new ProductionPlant(gridPos);
            break;
        default:
            plant = new Plant(gridPos);
            break;
      }
      createEntity(plant);
      return plant->price;
  }
  return 0;
}


// Entry point function
int main() {
    sf::RenderWindow window(sf::VideoMode(1600, 837), "Protect The Jungle: monkeys fight back!");

    // sample menu
    bool gameStartRequested = true;
    Button startGameButton(sf::Vector2f(650.f, 450.f), sf::Vector2f(300.f, 100.f), "Enter the Jungle", sf::Color(180, 100, 180), [&gameStartRequested]{
        gameStartRequested = true;
    });
    while (window.isOpen() && !gameStartRequested) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            startGameButton.handleEvent(event, window);
        }
        window.clear();
        startGameButton.draw(window);
        window.display();
        sf::sleep(sf::milliseconds(16));
    }

    Game* game = new Game(window);

    // add demo entitys
    ProductionPlant* productionPlant = new ProductionPlant(GridPos(1,3));
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
