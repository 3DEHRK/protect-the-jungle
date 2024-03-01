#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <cstdlib>

int WINDOW_WIDTH = 1200;
int WINDOW_HEIGHT = 800;
const float FRAME_RATE = 60.f;
const int GRID_SPACE = 50;

sf::Vector2i mousePos;
bool buildModeActive = true;

// forward declare existing classes
class Plant;
class Zombie;
// plants & zombies collections using unique pointers
// TODO: clean structure to generically store and delete (entity superclass?, identifiers?)
std::vector<std::unique_ptr<Plant>> plants;
std::vector<std::unique_ptr<Zombie>> zombies;


class Zombie {
protected:
    int gridRow;
    float velocity = 15.f;
    float progress = 0.f;
    int health = 100;
    float cooldown = 0.f;
    sf::CircleShape sprite;

public:
    Zombie(int initialGridRow) : gridRow(initialGridRow) {
        sprite.setRadius(20.f);
        sprite.setFillColor(sf::Color(100, 250, 50));
        sprite.setPosition(progress, gridRow * GRID_SPACE);
    }

    void damage(int amount, int cooldown = 15.f) {
        health -= amount;
        this->cooldown += cooldown;
        if (health <= 0) {
            // .. remove
            velocity = 0;
        }
    }

    virtual void tick(sf::RenderWindow &window) {
        // Base implementation for generic Zombie movement/logic
        if (cooldown < 0)
            progress += velocity * (1.f / FRAME_RATE);
        else
            cooldown -= velocity * (1.f / FRAME_RATE);

        sprite.setPosition(WINDOW_WIDTH - progress, gridRow * GRID_SPACE);
        window.draw(sprite);
    }

    int getGridRow() const { return gridRow; }
    float getProgressLocation() const { return WINDOW_WIDTH - progress; }
};

class ZombieFast : public Zombie {
public:
    ZombieFast(int initialGridRow) : Zombie(initialGridRow) {
        // Customize properties for ZombieFast
        velocity = 40.0f;
        sprite.setFillColor(sf::Color(250, 50, 50));
    }

    void tick(sf::RenderWindow &window) override {
        // Additional logic for ZombieFast

        // Call base class tick for common behavior
        Zombie::tick(window);
    }
};

class ZombieTank : public Zombie {
public:
    ZombieTank(int initialGridRow) : Zombie(initialGridRow) {
        // Customize properties for ZombieTank
        velocity = 10.f;
        health = 2.0f;
        sprite.setFillColor(sf::Color(100, 50, 250));
    }

    void tick(sf::RenderWindow &window) override {
        // Additional logic for ZombieTank

        // Call base class tick for common behavior
        Zombie::tick(window);
    }
};


class Projectile {
protected:
    float velocity = 500.f;
    int gridRow;
    float progress;
    float lifeSpan;
    float lifeTimer = 0.f;
    float height;
    int damage = 20;
    sf::CircleShape sprite;
    std::vector<std::unique_ptr<Projectile>>* projectileVector;

public:
    Projectile(int gridRow, int gridColumn, std::vector<std::unique_ptr<Projectile>>* projectileVector) : gridRow(gridRow), progress(gridColumn * GRID_SPACE), height(-10.f), lifeSpan(1.5f), projectileVector(projectileVector) {
        sprite.setRadius(10.f);
        sprite.setPointCount(6);
        sprite.setFillColor(sf::Color(250, 50, 50));
        sprite.setPosition(progress, gridRow * GRID_SPACE);

        velocity += velocity * ((rand() % 41) - 20) / 100.0f;
    }

    virtual void tick(sf::RenderWindow& window) {
        progress += velocity * (1.f / FRAME_RATE);
        height += 20.f * (1.f / FRAME_RATE);
        velocity -= 20.f * (1.f / FRAME_RATE);
        lifeTimer += (1.f / FRAME_RATE);

        if (lifeTimer >= lifeSpan) {
            // Find and remove this projectile from the vector
            auto it = std::find_if(projectileVector->begin(), projectileVector->end(),
                [this](const std::unique_ptr<Projectile>& proj) {
                    return proj.get() == this;
                });
            if (it != projectileVector->end()) {
                projectileVector->erase(it);
            }
        }

        // check for colliding zombies; damage & destroy self
        for (std::unique_ptr<Zombie>& zombie : zombies) {
            if (zombie->getGridRow() != gridRow)
                continue;
            if (std::abs(zombie->getProgressLocation() - progress) < 5.f) { // is nearly equal
                zombie->damage(damage);
                lifeSpan = 0.f;
            }
        }

        sprite.setPosition(progress, gridRow * GRID_SPACE + height);
        window.draw(sprite);
    }
};


class Plant {
protected:
    int gridRow;
    int gridColumn;
    float attackSpeed = 2.f;
    int health = 100;
    float attackTimer = 0.f;
    sf::CircleShape sprite;
    std::vector<std::unique_ptr<Projectile>> projectiles;

public:
    Plant(int gridRow, int gridColumn) : gridRow(gridRow), gridColumn(gridColumn) {
        sprite.setRadius(20.f);
        sprite.setPointCount(4);
        sprite.setFillColor(sf::Color(200, 250, 200));
        sprite.setPosition(gridColumn * GRID_SPACE, gridRow * GRID_SPACE);
    }

    void damage(int amount) {
        health -= amount;
        if (health <= 0) {
            std::cout << "Plant defeated!" << std::endl;
        }
    }

    virtual void tick(sf::RenderWindow &window) {
        attackTimer += 1.f / FRAME_RATE;

        if (attackTimer >= attackSpeed) {
            projectiles.push_back(std::make_unique<Projectile>(gridRow, gridColumn, &projectiles));
            attackTimer = 0.f;
        }

        // Base implementation for generic Plant logic
        for (const std::unique_ptr<Projectile>& projectile : projectiles) {
            projectile->tick(window);
        }

        window.draw(sprite);
    }
};

class ProductionPlant : public Plant {
private:
    float productionRate;
    float productionTimer = 0.f;

public:
    ProductionPlant(int gridRow, int gridColumn) : Plant(gridRow, gridColumn), productionRate(10.f) {
        attackSpeed = 999999999.f;
        sprite.setFillColor(sf::Color(255, 255, 100));
    }

    void produce() {
        std::cout << "Banana produced!" << std::endl;
        buildModeActive = true;
        productionTimer = 0.f;
    }

    void tick(sf::RenderWindow &window) override {

        productionTimer += 1.f / FRAME_RATE;

        // Check if it's time to produce
        if (productionTimer >= productionRate) {
            produce();
        }

        Plant::tick(window);
    }
};


void buildMode(sf::RenderWindow& window, float time, std::vector<std::unique_ptr<Plant>>& plants) {
    // Draw preview/selected square
    sf::RectangleShape sprite;
    sprite.setSize(sf::Vector2f(GRID_SPACE, GRID_SPACE));
    sprite.setFillColor(sf::Color(150, 150, 255, sin(time * 5) * 100 + 150));
    sprite.setPosition(mousePos.x / GRID_SPACE * GRID_SPACE, mousePos.y / GRID_SPACE * GRID_SPACE);
    window.draw(sprite);

    // Place plant and leave build mode
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        //todo: check if slot is empty
        plants.push_back(std::make_unique<ProductionPlant>(mousePos.y / GRID_SPACE, mousePos.x / GRID_SPACE));
        buildModeActive = false;
    }
}


int main() {
    sf::Clock frameClock;
    sf::Clock gameClock;
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Protect The Jungle: monkeys fight back!");

    zombies.push_back(std::make_unique<Zombie>(1));
    zombies.push_back(std::make_unique<ZombieFast>(2));
    zombies.push_back(std::make_unique<ZombieTank>(4));

    plants.push_back(std::make_unique<Plant>(1,1));
    plants.push_back(std::make_unique<Plant>(2,2));
    plants.push_back(std::make_unique<Plant>(4,1));

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // Update game logic at specified FRAME_RATE
        if (frameClock.getElapsedTime().asSeconds() >= 1.0 / FRAME_RATE) {
            window.clear();
            mousePos = sf::Mouse::getPosition(window);
            
            if (buildModeActive)
                buildMode(window, gameClock.getElapsedTime().asSeconds(), plants);

            // plants tick
            for (const std::unique_ptr<Plant>& plant : plants) {
                plant->tick(window);
            }

            // zombies tick
            for (const std::unique_ptr<Zombie>& zombie : zombies) {
                zombie->tick(window);
            }

            window.display();
            frameClock.restart();
        }

    }
    return 0;
}
