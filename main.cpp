#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>

int WINDOW_WIDTH = 1200;
int WINDOW_HEIGHT = 800;
const int GRID_SPACE = 50;

class Zombie {
protected:
    int gridIndex;
    float velocity;
    int health;
    float cooldown;
    sf::CircleShape sprite;
    float progress;

public:
    Zombie(int initialGridIndex) : gridIndex(initialGridIndex), progress(0.0f), velocity(1.0f), health(100) {
        sprite.setRadius(20.f);
        sprite.setFillColor(sf::Color(100, 250, 50));
        sprite.setPosition(progress, gridIndex * GRID_SPACE);
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

        sprite.setPosition(WINDOW_WIDTH - progress, gridIndex * GRID_SPACE);

        window.draw(sprite);
    }
};

class ZombieFast : public Zombie {
public:
    ZombieFast(int initialGridIndex) : Zombie(initialGridIndex) {
        // Customize properties for ZombieFast
        velocity = 2.0f;
        sprite.setFillColor(sf::Color(250, 50, 50));
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
        sprite.setFillColor(sf::Color(100, 50, 250));
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
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Gibb vs. Zombies");

    std::vector<Zombie> zombies;
    zombies.push_back(Zombie(1));
    zombies.push_back(ZombieFast(2));
    zombies.push_back(ZombieTank(4));

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        if (clock.getElapsedTime().asSeconds() >= 1.0 / 60.0) {
            // Update game logic at 60 FPS

            window.clear();
            
            for (Zombie& zombie : zombies) {
                zombie.tick(window);
            }

            window.display();
            
            clock.restart();
        }

    }

    return 0;
}
