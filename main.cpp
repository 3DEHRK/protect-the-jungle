#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>

class Zombie {
protected:
    int gridIndex;
    float progress;
    float velocity;
    int health;
    float cooldown;

public:
    Zombie(int initialGridIndex) : gridIndex(initialGridIndex), progress(0.0f), velocity(1.0f), health(100) {}

    void damage(int amount) {
        health -= amount;
        if (health <= 0) {
            std::cout << "Zombie defeated!" << std::endl;
        }
    }

    virtual void tick(sf::RenderWindow &window) {
        // Base implementation for generic Zombie movement/logic
        progress += velocity;


        window.display();
    }
};

class ZombieFast : public Zombie {
public:
    ZombieFast(int initialGridIndex) : Zombie(initialGridIndex) {
        // Customize properties for ZombieFast if needed
        velocity = 2.0f;
    }

    void tick(sf::RenderWindow &window) override {
        // Additional logic for ZombieFast
        // ...

        // Call base class tick for common behavior
        Zombie::tick(window);
    }
};

class ZombieTank : public Zombie {
public:
    ZombieTank(int initialGridIndex) : Zombie(initialGridIndex) {
        // Customize properties for ZombieTank
        velocity = 0.5f;
        health = 2.0f;
    }

    void tick(sf::RenderWindow &window) override {
        // Additional logic for ZombieTank
        // ...

        // Call base class tick for common behavior
        Zombie::tick(window);
    }
};

int main() {
    sf::Clock clock;
    sf::RenderWindow window(sf::VideoMode(800, 600), "Gibb vs. Zombies");


    // Todo: make entity system collection (std::vector<Zombie>)
    std::vector<Zombie> zombies;
    zombies.push_back(ZombieFast(1));

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        window.clear();

        if (clock.getElapsedTime().asSeconds() >= 1.0 / 60.0) {
            // Update game logic at 60 FPS
            for (Zombie zombie : zombies) {
                zombie.tick(window);
            }

            clock.restart();
        }

        // Render
        window.clear();
        window.display();
    }

    return 0;
}
