#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <memory>

int WINDOW_WIDTH = 1200;
int WINDOW_HEIGHT = 800;
const float FRAME_RATE = 60.f;
const int GRID_SPACE = 50;

class Zombie {
protected:
    int gridRow;
    float velocity;
    float progress;
    int health;
    float cooldown;
    sf::CircleShape sprite;

public:
    Zombie(int initialGridRow) : gridRow(initialGridRow), progress(0.0f), velocity(1.0f), health(100) {
        sprite.setRadius(20.f);
        sprite.setFillColor(sf::Color(100, 250, 50));
        sprite.setPosition(progress, gridRow * GRID_SPACE);
    }

    void damage(int amount) {
        health -= amount;
        if (health <= 0) {
            std::cout << "Zombie defeated!" << std::endl;
        }
    }

    virtual void tick(sf::RenderWindow &window) {
        // Base implementation for generic Zombie movement/logic
        progress += velocity;

        sprite.setPosition(WINDOW_WIDTH - progress, gridRow * GRID_SPACE);
        window.draw(sprite);
    }
};

class ZombieFast : public Zombie {
public:
    ZombieFast(int initialGridRow) : Zombie(initialGridRow) {
        // Customize properties for ZombieFast
        velocity = 2.0f;
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
        velocity = 0.5f;
        health = 2.0f;
        sprite.setFillColor(sf::Color(100, 50, 250));
    }

    void tick(sf::RenderWindow &window) override {
        // Additional logic for ZombieTank

        // Call base class tick for common behavior
        Zombie::tick(window);
    }
};

class Plant {
protected:
    int gridRow;
    int gridColumn;
    float attackSpeed;
    int health;
    sf::CircleShape sprite;

public:
    Plant(int gridRow, int gridColumn) : gridRow(gridRow), gridColumn(gridColumn), health(100), attackSpeed(1.f) {
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
        // Base implementation for generic Plant logic

        window.draw(sprite);
    }
};

class ProductionPlant : public Plant {
private:
    float productionRate;
    float productionTimer;

public:
    ProductionPlant(int gridRow, int gridColumn) : Plant(gridRow, gridColumn), productionRate(1.f), productionTimer(0.f) {
        sprite.setFillColor(sf::Color(255, 255, 100));
    }

    void produce() {
        std::cout << "Banana produced!" << std::endl;
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


int main() {
    sf::Clock clock;
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Protect The Jungle: monkeys fight back!");

    // zombies collection using unique pointer
    std::vector<std::unique_ptr<Zombie>> zombies;
    zombies.push_back(std::make_unique<Zombie>(1));
    zombies.push_back(std::make_unique<ZombieFast>(2));
    zombies.push_back(std::make_unique<ZombieTank>(4));

    // plants collection using unique pointer
    std::vector<std::unique_ptr<Plant>> plants;
    plants.push_back(std::make_unique<Plant>(1,1));
    plants.push_back(std::make_unique<ProductionPlant>(1,2));
    plants.push_back(std::make_unique<Plant>(2,1));
    plants.push_back(std::make_unique<Plant>(4,1));

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // Update game logic at 60 FPS
        if (clock.getElapsedTime().asSeconds() >= 1.0 / FRAME_RATE) {
            window.clear();
            
            // zombies tick
            for (const std::unique_ptr<Zombie>& zombie : zombies) {
                zombie->tick(window);
            }

            // plants tick
            for (const std::unique_ptr<Plant>& plant : plants) {
                plant->tick(window);
            }

            window.display();
            clock.restart();
        }

    }
    return 0;
}
