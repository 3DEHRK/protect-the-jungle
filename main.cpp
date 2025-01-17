#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <curl/curl.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <functional>
#include <sstream>
#include <time.h>
#include <regex>
#include <algorithm>
#include <random>
#include <filesystem>

// Callback function to write received data into a string
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* buffer) {
    size_t total_size = size * nmemb;
    buffer->append((char*)contents, total_size);
    return total_size;
}

int extractStatusCode(const std::string& jsonString) {
    std::regex statusCodePattern(R"("status":(\d+))");
    std::smatch matches;

    if (std::regex_search(jsonString, matches, statusCodePattern) && matches.size() > 1) {
        // The status code is in the second capture group
        return std::stoi(matches[1].str());
    }

    // Return a default value or handle the error as needed
    return -1; // Indicates that the status code was not found
}

// return false on unsuccessful request <3
bool saveScore(std::string name, int score) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    bool isRequestSuccessful;
    if(curl) {
        // Set the URL
        curl_easy_setopt(curl, CURLOPT_URL, "http://marco.jaros.ch/score/create.php");
        // Set the HTTP method to POST
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // Set the headers to indicate JSON data
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the POST data using std::format
        std::ostringstream postDataStream;
        postDataStream << R"({"name": ")" << name << R"(","score": ")" << score << "\"}";
        std::string postData = postDataStream.str();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());

        // Set the write callback function
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            isRequestSuccessful = false;
        } else {
            std::cout << "Response: " << readBuffer << std::endl;
            int statusCode = extractStatusCode(readBuffer);
            isRequestSuccessful = statusCode == 201;
        }

        // Clean up
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();
    return isRequestSuccessful;
}

std::pair<std::string, int> getNameAndScoreByRank(const std::string& data, int rank) {
    std::string name;
    int score = 0;

    // Find the "data" array
    size_t pos = data.find("\"data\"");
    if (pos == std::string::npos) {
        std::cerr << "Data array not found in JSON" << std::endl;
        return { name, score };
    }

    // Find the beginning of the array
    pos = data.find('[', pos);
    if (pos == std::string::npos) {
        std::cerr << "Data array not found in JSON" << std::endl;
        return { name, score };
    }

    // Iterate through each object in the array
    while (true) {
        // Find the beginning of an object
        pos = data.find('{', pos);
        if (pos == std::string::npos)
            break;

        // Find the end of the object
        size_t endPos = data.find('}', pos);
        if (endPos == std::string::npos)
            break;

        // Extract the object
        std::string object = data.substr(pos, endPos - pos + 1);

        // Find "rank"
        size_t rankPos = object.find("\"rank\":\"" + std::to_string(rank) + "\"");
        if (rankPos != std::string::npos) {
            // Find "name"
            size_t namePos = object.find("\"name\":\"");
            if (namePos != std::string::npos) {
                namePos += 8; // Length of "\"name\":\""
                size_t nameEnd = object.find('"', namePos);
                name = object.substr(namePos, nameEnd - namePos);
            }

            // Find "score"
            size_t scorePos = object.find("\"score\":\"");
            if (scorePos != std::string::npos) {
                scorePos += 9; // Length of "\"score\":\""
                size_t scoreEnd = object.find('"', scorePos);
                score = std::stoi(object.substr(scorePos, scoreEnd - scorePos));
            }

            break; // Stop after finding the specified rank
        }

        // Move to the next object
        pos = endPos + 1;
    }

    return { name, score };
}

std::vector<sf::Text*> curlGetScores() {
    //curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    std::vector<sf::Text*> texts;
    if (curl) {
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_URL, "http://marco.jaros.ch/score/read.php");
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res == CURLE_OK) {
            for (int i = 1; i < 30; i++)
            {
                auto result = getNameAndScoreByRank(readBuffer, i);

                // yeah this does leak memory but whatever..
                sf::Font* font = new sf::Font;
                font->loadFromFile("res/arial.ttf");

                sf::Text* text = new sf::Text();
                text->setFont(*font);
                text->setCharacterSize(15);
                text->setString(std::to_string(i) + ". " + std::to_string(result.second) + "   " + result.first);
                text->setPosition(250, 300 + i*14);
                texts.push_back(text);
            }
        }
    }
    return texts;
}


class Button {
public:
    Button(sf::Vector2f position, sf::Vector2f size, const std::string& text, const sf::Color& color, std::function<void()> onClick, const std::string& imagePath = "")
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
        if (imagePath != "") {
            m_texture.loadFromFile(imagePath);
            m_sprite.setTexture(m_texture);
            m_text.setPosition(position.x + size.x / 2.f, position.y + size.y + 15.f);
        }
        sf::FloatRect spriteRect = m_sprite.getLocalBounds();
        m_sprite.setOrigin(spriteRect.left + spriteRect.width / 2.0f,
                           spriteRect.top + spriteRect.height / 2.0f);
        m_sprite.setPosition(position.x + size.x / 2.0f, position.y + size.y / 2.0f);
        m_sprite.setScale(size.y / spriteRect.height, size.y / spriteRect.height);
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
        window.draw(m_sprite);
        window.draw(m_text);
    }

private:
    sf::RectangleShape m_shape;
    sf::Sprite m_sprite;
    sf::Texture m_texture;
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
    float x = 0;
    float y = 0;
    float xVel = 0;
    float yVel = 0;
    float health = 100;
    float topHealth = 100;
    int id = -1;
    std::string group = "entity";
    std::string type = "entity";

    // animation
    std::string resDir = "entity";
    bool pauseAnimation = false;
    std::vector<sf::Texture> textures;
    sf::Sprite sprite;
    int currentFrame = 0;
    float frameDuration = 0.2f;
    float frameTimer = 0.f;
    
    class Game* game = nullptr;

    Entity() {}

    // use ready() instead of the constructor since class Game* game; isn't defined there yet
    virtual void ready() {
        int i = 0;
        bool loaded = true;
        while (loaded) {
            std::string filename = "res/" + resDir + "/" + std::to_string(i) + ".png";
            sf::Texture texture;
            if (std::filesystem::exists(filename)) {
                texture.loadFromFile(filename);
                textures.push_back(texture);
                i++;
            }
            else {
                loaded = false;
            }
        }
        sprite.setTexture(textures[0]);
    }

    virtual void updateAnimation(float dt) {
        if (pauseAnimation)
            return;
        frameTimer += dt;
        if (frameTimer >= frameDuration) {
            frameTimer = 0.0f;
            currentFrame = (currentFrame + 1) % textures.size();
            sprite.setTexture(textures[currentFrame]);
        }
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
    std::vector<Entity*> getGridCollisionsAround(const GridPos center, const std::string& groupFilter = "")
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
    
    int bananaCount = 50;
    int editMode = 0; // 0: sleep, 1: build, 2: destroy
    int selectedPlant = 0;
    int score = 0;

    bool isGameOver = false;

    int waveCount = -500;
    int zombieChance = 500;
    int passedWaves = 0;

    Game(sf::RenderWindow& window) : gameWindow(window) {
        WINDOW_WIDTH = gameWindow.getSize().x;
        WINDOW_HEIGHT = gameWindow.getSize().y;
        font.loadFromFile("res/arial.ttf");

        // do not ask, do not change; it fixes the crashes (keep it on the low)
        for (int i = 0; i < 9; i++) {
            entityCollection.push_back(nullptr);
        }
        entityCollection.clear();

        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
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

    std::vector<Entity*> getGridCollisionsAround(const GridPos center, const std::string& groupFilter = "") {
        std::vector<Entity*> around;

        std::vector<Entity*> inside = getGridCollisions(center, groupFilter);
        around.insert(around.end(), inside.begin(), inside.end());
        std::vector<Entity*> above = getGridCollisions(GridPos(center.x, center.y + 1), groupFilter);
        around.insert(around.end(), above.begin(), above.end());
        std::vector<Entity*> below = getGridCollisions(GridPos(center.x, center.y - 1), groupFilter);
        around.insert(around.end(), below.begin(), below.end());
        std::vector<Entity*> right = getGridCollisions(GridPos(center.x + 1, center.y), groupFilter);
        around.insert(around.end(), right.begin(), right.end());
        std::vector<Entity*> left = getGridCollisions(GridPos(center.x - 1, center.y), groupFilter);
        around.insert(around.end(), left.begin(), left.end());

        return around;
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

    void spawnZombie(int type);

    sf::Text generateText(int x, int y) {
//        sf::Font m_font;
//        m_font.loadFromFile("res/arial.ttf");

        sf::Text text;
        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);
        text.setPosition(x, y);

        return text;
    }

    bool startGame() {
        srand(time(NULL));
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

        Button destroyButton(sf::Vector2f(0.f, 720.f), sf::Vector2f(150.f, 80.f), "Demobilize", sf::Color(180, 50, 50), [this]{
                editMode = 2;
            });

        Button plantButton(sf::Vector2f(250.f, 720.f), sf::Vector2f(125.f, 80.f), "3$", sf::Color(50, 180, 50), [this]{
                editMode = 1;
                selectedPlant = 0;
            }, "res/monkey/0.png");

        Button plant2Button(sf::Vector2f(400.f, 720.f), sf::Vector2f(125.f, 80.f), "4$", sf::Color(50, 180, 50), [this]{
                editMode = 1;
                selectedPlant = 2;
            }, "res/tank_monkey/0.png");

        Button plant1Button(sf::Vector2f(550.f, 720.f), sf::Vector2f(125.f, 80.f), "5$", sf::Color(50, 180, 50), [this]{
                editMode = 1;
                selectedPlant = 1;
            }, "res/prod_monkey/0.png");

        Button plant4Button(sf::Vector2f(700.f, 720.f), sf::Vector2f(125.f, 80.f), "1$", sf::Color(50, 180, 50), [this] {
            editMode = 1;
            selectedPlant = 4;
            }, "res/bananaTreeShadowless.png");

        Button plant3Button(sf::Vector2f(850.f, 720.f), sf::Vector2f(125.f, 80.f), "10$", sf::Color(50, 180, 50), [this]{
                editMode = 1;
                selectedPlant = 3;
            }, "res/med_monkey/0.png");

        Button plant5Button(sf::Vector2f(1000.f, 720.f), sf::Vector2f(125.f, 80.f), "5$", sf::Color(50, 180, 50), [this]{
                editMode = 1;
                selectedPlant = 5;
            }, "res/bomb/0.png");

        Button plant6Button(sf::Vector2f(1150.f, 720.f), sf::Vector2f(125.f, 80.f), "20$", sf::Color(50, 180, 50), [this]{
                editMode = 1;
                selectedPlant = 6;
            }, "res/heavy_monkey/0.png");

        sf::Text bananasCountText = generateText(250, 680);
        sf::Text scoreText = generateText(550, 680);

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
                plant3Button.handleEvent(event, gameWindow);
                plant4Button.handleEvent(event, gameWindow);
                plant5Button.handleEvent(event, gameWindow);
                plant6Button.handleEvent(event, gameWindow);
            }
            mousePos.x = sf::Mouse::getPosition(gameWindow).x * ((float)WINDOW_WIDTH / gameWindow.getSize().x);
            mousePos.y = sf::Mouse::getPosition(gameWindow).y * ((float)WINDOW_HEIGHT / gameWindow.getSize().y);

            gameWindow.clear();

            // draw static elements
            gameWindow.draw(fieldSprite);
            gameWindow.draw(barSprite);

            if (isGameOver) {
                return false;
            }

            // make entities tick
            for (Entity* entity : entityCollection) {
                entity->tick();
            }

            // spöwns a sömbie every tick with 1 zu füfhundert chance.
            if ((rand() % zombieChance + 1) == zombieChance) {
                int whichZombieNumber = rand() % 100;
                // pushing a default skin 75%
                if (whichZombieNumber <= 75)
                   spawnZombie(0);
                else if (whichZombieNumber <= 86 && passedWaves > 0)
                    spawnZombie(1);
				else if (whichZombieNumber <= 99 && passedWaves > 0)
                    if (rand() % 2 == 0)
					    spawnZombie(2);
                    else
                        spawnZombie(3);
            }
            waveCount++;
            // noch einerhalb minute hört wave uf
            if(waveCount >= 5400) {
                zombieChance = 200 + passedWaves * 15;
                waveCount = 0;
                passedWaves++;
            // after 1 minute fangt wave aa
            }else if (waveCount >= 3600) {
                // double spawn rate
                zombieChance = 20 + passedWaves;
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

            bananasCountText.setString("Bananas: " + std::to_string(bananaCount) + "$");
            scoreText.setString("Score: " + std::to_string(score));

            // draw static elements
            gameWindow.draw(bananasCountText);
            gameWindow.draw(scoreText);

            plantButton.draw(gameWindow);
            destroyButton.draw(gameWindow);
            plant1Button.draw(gameWindow);
            plant2Button.draw(gameWindow);
            plant3Button.draw(gameWindow);
            plant4Button.draw(gameWindow);
            plant5Button.draw(gameWindow);
            plant6Button.draw(gameWindow);

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
    updateAnimation(game->deltaTime());
    game->gameWindow.draw(sprite);

    if (health < topHealth) {
        sf::Text healthText;
        healthText.setFont(game->font);
        healthText.setString(std::to_string((int)health));
        healthText.setCharacterSize(13);
        healthText.setPosition(x, y + 20.f);
        game->gameWindow.draw(healthText);
    }
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
    int scorePoints = 10;
    int walkingAnimationCounter = 0;
    int spawnChance = 200;

public:

    int getScorePoints() {
        return scorePoints;
    }

    Zombie(int startingGridRow) : startingGridRow(startingGridRow) {}

    void ready() override {
        resDir = "woodchopper";
        // Call Entity's ready after setting the ressources directory
        Entity::ready();

        group = "zombie";
        sprite.setScale(100/32, 100/32);
        sprite.setOrigin(sprite.getLocalBounds().width / 2.f, sprite.getLocalBounds().height / 2.f);

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
            // attack 2 targets max
            std::vector<Entity*> collisions = game->getGridCollisions(getGridPos(), "plant");
            collisions[0]->damage(damageDonePerSec * game->deltaTime());
            if (collisions.size() > 1)
                collisions[1]->damage(damageDonePerSec * game->deltaTime());
        } else {
            xVel = xVelNormal;
        }

        x += knockback;
        if (knockback > 0.f)
            knockback -= game->deltaTime() * 15.f;

        if(walkingAnimationCounter == 40) {
            sprite.setRotation(-10.f);
        
        }

        if(walkingAnimationCounter == 80) {
            sprite.setRotation(10.f);
            walkingAnimationCounter = 0;
        
        }
        walkingAnimationCounter++;
        if (getGridPos().x < 0) {
            game->isGameOver = true;
        }
        // Always call Entity's tick
        Entity::tick();
    }

    // Zombie specific functions
    int getGridRow() const { return game->freeToGrid(x); }
    float getProgressLocation() const { return game->WINDOW_WIDTH - x; }
};

class TankZombie : public Zombie {
public:
    TankZombie(int startingRow) : Zombie(startingRow) {}
    void ready() override {
        resDir = "tank_woodchopper";
        Entity::ready();

        topHealth = 500;
        health = topHealth;
        xVelNormal = -50.f;
        xVel = xVelNormal;
        group = "zombie";
        sprite.setScale(100 / 32, 100 / 32);
        sprite.setOrigin(sprite.getLocalBounds().width / 2.f, sprite.getLocalBounds().height / 2.f);
        y = game->gridToFree(startingGridRow);
        x = game->WINDOW_WIDTH;
    }

    bool damage(float d) override {
        knockback += d / 5;
        return Entity::damage(d);
    }
};

class ChainsawZombie : public Zombie {
public:
    ChainsawZombie(int startingRow) : Zombie(startingRow) {}
    void ready() override {
        resDir = "chainsaw_carrier";
        Entity::ready();

        topHealth = 100;
        health = topHealth;
        xVelNormal = -100.f;
        xVel = xVelNormal;
		damageDonePerSec = 175.f;
        group = "zombie";
        sprite.setScale(100 / 32, 100 / 32);
        sprite.setOrigin(sprite.getLocalBounds().width / 2.f, sprite.getLocalBounds().height / 2.f);
        y = game->gridToFree(startingGridRow);
        x = game->WINDOW_WIDTH;
    }
	void tick() override {
		if(walkingAnimationCounter % 20 == 0) {
			sprite.setTexture(textures[0]);
		}
		if(walkingAnimationCounter % 20 == 10) {
			sprite.setTexture(textures[1]);
		}
		Zombie::tick();
	}
    bool damage(float d) override {
        knockback += d / 5;
        return Entity::damage(d);
    }
};

class BulldozerZombie : public Zombie {
public:
    BulldozerZombie(int startingRow) : Zombie(startingRow) {}
    void ready() override {
        resDir = "bulldozer";
        Entity::ready();

        topHealth = 200;
        health = topHealth;
        xVelNormal = -50.f;
        xVel = xVelNormal;
		damageDonePerSec = 300.f;
        group = "zombie";
        sprite.setScale(100 / 32, 100 / 32);
        y = game->gridToFree(startingGridRow);
        x = game->WINDOW_WIDTH;
    }
	void tick() override {
        walkingAnimationCounter = 0;
		Zombie::tick();
	}
};

class Projectile : public Entity {
protected:
    float lifeSpan = 1.0f;
    int damageDone = 15;
    float baseVelocity = 1000.f;
    float gravityMultiplier = 200.f;

    GridPos initGridPos;
    float lifeTimer = 0.f;

public:
    Projectile(GridPos gridPos) : initGridPos(gridPos) {}

    void ready() override {
        resDir = "stone";
        sprite.setScale(100/32, 100/32);
        Entity::ready();
        group = "projectile";

        x = game->gridToFree(initGridPos.x);
        y = game->gridToFree(initGridPos.y) - 20.f;
        xVel = baseVelocity;
    }

    void tick() override {
        // imitate physics
        xVel -= game->deltaTime() * 200;
        yVel += game->deltaTime() * gravityMultiplier;

        lifeTimer += game->deltaTime();
        if (lifeTimer >= lifeSpan)
            game->destroyEntity(id);

        // check for colliding zombies; damage & destroy self
        std::vector<Entity*> hits = game->getCollisions(x, y, 25.f, "zombie");
        for (Entity* zombie : hits) {
            bool isZombieDead = zombie->damage(damageDone);
            if (isZombieDead) {
                Zombie* realZombie = dynamic_cast<Zombie*>(zombie);
                game->score += realZombie->getScorePoints();
            }
            game->destroyEntity(id);
        }

        Entity::tick();
    }
};

class ProjectileHeavy : public Projectile {
public:
    ProjectileHeavy(GridPos gridPos) : Projectile(gridPos) {}

    void ready() override {
        baseVelocity = 1000.f;
        yVel = -200.f;
        lifeSpan = 1.7f;
        gravityMultiplier = 300.f;
        damageDone = 30.f;

        resDir = "rock";
        sprite.setScale(100/32, 100/32);
        Entity::ready();
        group = "projectile";
        x = game->gridToFree(initGridPos.x);
        y = game->gridToFree(initGridPos.y) - 10.f;
        xVel = baseVelocity;
    }
};


class Plant : public Entity {
protected:
    float attackSpeed = 2.0f;
    float attackTimer = 0.f;

    GridPos initGridPos;

public:
    int price = 1;
    Plant(GridPos gridPos) : initGridPos(gridPos) {}

    void ready() override {
        resDir = "monkey";
        sprite.setScale(100/32, 100/32);
        Entity::ready();
        price = 3;
        group = "plant";
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
            makeNewProjectile();
            isReady = false;
        }

        Entity::tick();
    }

    virtual void makeNewProjectile() {
        Projectile* projectile = new Projectile(this->getGridPos());
        game->createEntity(projectile);
    }
};

class ProductionPlant : public Plant {
private:
    float productionDelay = 5.f;
    float productionTimer = 0.f;
    int productionAmount = 1.f;

public:
    ProductionPlant(GridPos gridPos) : Plant(gridPos) {}

    void ready() override {
        resDir = "prod_monkey";
        sprite.setScale(100/32, 100/32);
        Entity::ready();
        frameDuration = 2.f;
        group = "plant";
        price = 5;
        setGridPos(initGridPos);
    }

    void tick() override {
        if (isTreeAround()) {
            productionTimer += game->deltaTime();
            pauseAnimation = false;
            if (productionTimer >= productionDelay) {
                game->bananaCount += productionAmount;
                productionTimer = 0.f;
            }
        } else {
            pauseAnimation = true;
        }
        Entity::tick();
    }

    bool isTreeAround() {
        bool isTreeAround = false;
        for (const Entity* test : game->getGridCollisionsAround(getGridPos(),"plant")) {
            if (test->type == "tree")
                isTreeAround = true;
        }
        return isTreeAround;
    }
};

class TreePlant : public Plant{
public:
    TreePlant(GridPos gridPos) : Plant(gridPos) {}
    void ready() override {
        resDir = "tree";
        sprite.setScale(100 / 32, 100 / 32);
        Entity::ready();
        type = "tree";
        price = 1;
        group = "plant";
        setGridPos(initGridPos);
    }
    void tick() override {
        Entity::tick();
    }
};

class TankPlant : public Plant {

public:
    TankPlant(GridPos gridPos) : Plant(gridPos) {}

    void ready() override {
        resDir = "tank_monkey";
        pauseAnimation = true;
        sprite.setScale(100/32, 100/32);
        Entity::ready();
        price = 4;
        topHealth = 1000;
        health = topHealth;
        group = "plant";
        setGridPos(initGridPos);
    }

    void tick() override {
        sprite.setTexture(textures[0]);
        if (health <= 666)
            sprite.setTexture(textures[1]);
        if (health <= 333)
            sprite.setTexture(textures[2]);
        Entity::tick();
    }
};

class MendingPlant : public Plant {
private:
    Entity* target = nullptr;
    float healingSpeed = 0.5f;
    float healthPerAppointment = 30.f;
    float healthOverload = 20.f;

    float healthDelt = 0;
    float movementSpeed = 100.f;
    float idleTimer = 2.f;

    Entity* findTarget() {
        std::vector<Entity*> entitiesShuffled = game->entityCollection;
        std::random_device rd;
        std::mt19937 randomEngine(rd());
        std::shuffle(entitiesShuffled.begin(), entitiesShuffled.end(), randomEngine);

        for (Entity* test : entitiesShuffled) {
            if (test->group == "plant" && test->health < test->topHealth && test != this) {
                idleTimer = 2.f + rand() / RAND_MAX;
                return test;
            }
        }
        return nullptr;
    }

    bool moveToTarget() {
        float tolerance = 20.f;
        float xDiff = target->x - x;
        float yDiff = target->y - y;

        if (std::abs(xDiff) <= tolerance && std::abs(yDiff) <= tolerance) {
            xVel = 0.f;
            yVel = 0.f;
            return true;
        } else {
            xVel = (std::abs(xDiff) <= tolerance) ? 0.f : (xDiff > 0 ? movementSpeed : -movementSpeed);
            yVel = (std::abs(yDiff) <= tolerance) ? 0.f : (yDiff > 0 ? movementSpeed : -movementSpeed);

            sprite.setTexture((xVel >= 0.f) ? textures[0] : textures[1]);
        }
        return false;
    }

    bool healTarget() {
        target->health += healingSpeed;
        healthDelt += healingSpeed;

        if (healthDelt >= healthPerAppointment || target->health >= target->topHealth + healthOverload) {
            healthDelt = 0;
            return true;
        }
        return false;
    }
public:
    MendingPlant(GridPos gridPos) : Plant(gridPos) {}

    void ready() override {
        resDir = "med_monkey";
        sprite.setScale(100 / 32, 100 / 32);
        Entity::ready();
        pauseAnimation = true;
        price = 10;
        group = "plant";
        setGridPos(initGridPos);
    }

    void tick() override {
        if (idleTimer < 0.f) {
            if (target == nullptr) {
                target = findTarget();
            } else {
                if (moveToTarget())
                    if(healTarget())
                        target = findTarget();
            }
        } else {
            idleTimer -= game->deltaTime();
        }

        Entity::tick();
    }
};

class BombPlant : public Plant{
public:
    bool detonated = false;
    BombPlant(GridPos gridPos) : Plant(gridPos) {}
    void ready() override {
        resDir = "bomb";
        pauseAnimation = true;
        frameDuration = 0.1f;
        sprite.setOrigin(32.f, 32.f);
        sprite.setScale(100 / 32, 100 / 32);
        Entity::ready();
        price = 5;
        group = "plant";
        setGridPos(initGridPos);
    }
    void tick() override {
        Entity::tick();
    }
    bool damage(float d) override {
        pauseAnimation = false;
        return Entity::damage(d);
    }
    void updateAnimation(float dt) override {
        Entity::updateAnimation(dt);
        if (currentFrame == 2) {
            for (Entity* victim : game->getGridCollisionsAround(getGridPos(), "zombie"))
                victim->damage(200.f);
            detonated = true;
        }
        if (currentFrame == 0 && detonated) {
            game->destroyEntity(id);
        }
    }
};

class HeavyPlant : public Plant {
public:
    HeavyPlant(GridPos gridPos) : Plant(gridPos) {}
    void ready() override {
        resDir = "heavy_monkey";
        sprite.setScale(100/32, 100/32);
        Entity::ready();
        price = 20;
        group = "plant";
        setGridPos(initGridPos);
    }
    void makeNewProjectile() override {
        ProjectileHeavy* projectile = new ProjectileHeavy(this->getGridPos());
        game->createEntity(projectile);
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
        case 2:
            plant = new TankPlant(gridPos);
            break;
        case 3:
            plant = new MendingPlant(gridPos);
            break;
        case 4:
            plant = new TreePlant(gridPos);
            break;
        case 5:
            plant = new BombPlant(gridPos);
            break;
        case 6:
            plant = new HeavyPlant(gridPos);
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

void Game::spawnZombie(int type) {
    Zombie* zombie;

    switch (type) {
    case 0:
        zombie = new Zombie(rand() % 8);
        break;
    case 1:
        zombie = new TankZombie(rand() % 8);
        break;
    case 2:
        zombie = new ChainsawZombie(rand() % 8);
        break;
    case 3:
        zombie = new BulldozerZombie(rand() % 8);
    default:
        break;
    }

    createEntity(zombie);  
}

// Entry point function
int main() {
    FreeConsole();

    sf::RenderWindow window(sf::VideoMode(1600, 837), "Protect The Jungle: monkeys fight back!");

    sf::Music music;
    music.openFromFile("res/mainMenu.ogg");

    std::vector<sf::Text*> scoreTexts = curlGetScores();

    // sample menu
    bool gameStartRequested = false;
    bool exitRequested = false;
    Button startGameButton(sf::Vector2f(650.f, 450.f), sf::Vector2f(300.f, 100.f), "Enter the Jungle", sf::Color(180, 100, 180), [&gameStartRequested, &music]{
        gameStartRequested = true;
        music.stop();
    });
    Button endGameButton(sf::Vector2f(650.f, 550.f), sf::Vector2f(300.f, 50.f), "Dont enter it", sf::Color(200, 80, 120), [&exitRequested] {
        exitRequested = true;
    });
    Button eminemButton(sf::Vector2f(650.f, 100.f), sf::Vector2f(100.f, 100.f), "Eminem Button", sf::Color(0, 80, 120), [&music] {
        music.play();
    }, "res/eminem.jpg");
    sf::Texture texture;
    sf::Sprite sprite;
    texture.loadFromFile("res/monkey/0.png");
    sprite.setTexture(texture);
    sprite.setScale(20.f, 20.f);
    sf::Texture texture1;
    sf::Sprite sprite1;
    texture1.loadFromFile("res/woodchopper/0.png");
    sprite1.setTexture(texture1);
    sprite1.setScale(20.f, 20.f);
    sprite1.setPosition(1000.f,0.f);
    sf::Texture texture2;
    sf::Sprite sprite2;
    texture2.loadFromFile("res/tree/0.png");
    sprite2.setTexture(texture2);
    sprite2.setScale(20.f, 20.f);
    sprite2.setPosition(500.f,250.f);
    while (window.isOpen() && !gameStartRequested) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed || exitRequested)
                window.close();
            startGameButton.handleEvent(event, window);
            endGameButton.handleEvent(event, window);
            eminemButton.handleEvent(event, window);
        }
        window.clear();
        window.draw(sprite);
        window.draw(sprite1);
        window.draw(sprite2);
        startGameButton.draw(window);
        endGameButton.draw(window);
        eminemButton.draw(window);
        for (sf::Text* text : scoreTexts)
            window.draw(*text);
        window.display();
        sf::sleep(sf::milliseconds(16));
    }

    Game* game = new Game(window);

    while (game->gameWindow.isOpen()) {
        bool isWon = game->startGame();
        if (isWon) {
            // victory wirds das über haupt gä?!?!?!?! (I <3 GIBB)
        } else {
            bool isSaved = false;
            sf::Texture texture;
            sf::Sprite sprite;
            texture.loadFromFile("res/bgMenu.png");
            sprite.setTexture(texture);

            sf::Vector2u windowSize = game->gameWindow.getSize();
            sf::Vector2u spriteSize = texture.getSize();
            double centeredX = (windowSize.x / 2) - (spriteSize.x / 2);
            double centeredY = (windowSize.y / 2) - (spriteSize.y / 2);
            sprite.setPosition(centeredX, centeredY);


            bool gameRestartRequested = false;
            Button restartGameButton(sf::Vector2f(650.f, 450.f), sf::Vector2f(300.f, 100.f), "Restart", sf::Color(180, 100, 180), [&gameRestartRequested]{
                gameRestartRequested = true;
            });

            bool saveScoreRequested = false;
            Button saveScoreButton(sf::Vector2f(1100.f, 450.f), sf::Vector2f(150.f, 100.f), "Save Score", sf::Color(180, 100, 180), [&saveScoreRequested, &isSaved]{
                if (!isSaved) {
                    saveScoreRequested = true;
                }
            });

            sf::Text scoreText = game->generateText(250, 350);
            scoreText.setString("Your score: " + std::to_string(game->score));
            sf::Text playerText = game->generateText(650, 200);

            sf::String lastPlayerInput;
            sf::String playerInput;
            int keyDebounceCounter = 0;
            while (window.isOpen() && !gameRestartRequested) {
                sf::Event event;
                while (window.pollEvent(event)) {
                    if (event.type == sf::Event::Closed)
                        window.close();
                    restartGameButton.handleEvent(event, window);
                    saveScoreButton.handleEvent(event, window);
                }
                window.clear();

                if (event.type == sf::Event::TextEntered && playerInput.getSize() < 11)
                {
                    if ((event.text.unicode != lastPlayerInput || keyDebounceCounter >= 8) && playerInput.toAnsiString().length() <= 32) {
                        playerInput += event.text.unicode;
                        lastPlayerInput = event.text.unicode;
                        playerText.setString(playerInput);
                        keyDebounceCounter = 0;
                    }
                }

                if (saveScoreRequested) {
                    isSaved = saveScore(playerInput.toAnsiString(), game->score);
                    saveScoreRequested = false;
                }

                window.draw(sprite);
                window.draw(scoreText);
                window.draw(playerText);
                restartGameButton.draw(window);
                saveScoreButton.draw(window);
                window.display();
                keyDebounceCounter++;
                sf::sleep(sf::milliseconds(16));
            }
            game = new Game(window);
        }
    }
}
