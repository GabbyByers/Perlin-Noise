#include <iostream>
#include <string>
#include <cmath>
#include <ctime>
#include <vector>
#include <SFML/Graphics.hpp>
#include <json/json.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <typeinfo>

class Game {
public:
    int screenWidth = 1000;
    int screenHeight = 1000;

    sf::Font CourierPrime_Regular;

    Game() {
        srand(time(nullptr)); //seed
        CourierPrime_Regular.loadFromFile("../Fonts/CourierPrime-Regular.ttf");
    }
};
Game game;

class DisplayFPS {
public:
    sf::Text text;
    int historySize = 100;
    std::vector <double> historicalFPS;

    DisplayFPS() {
        text.setFont(game.CourierPrime_Regular);
        text.setCharacterSize(16);
        text.setFillColor(sf::Color(255, 255, 255));
        text.setPosition(5, 0);
    }

    void drawFPS(sf::RenderWindow& window, sf::Clock& clock) {
        double fpsThisFrame = getFPS(clock);
        pushAndPop(fpsThisFrame);
        double fpsRollingAverage = returnRollingAverageFPS();
        text.setString("FPS: " + std::to_string(int(fpsRollingAverage)));
        window.draw(text);
    }

    void pushAndPop(double fps) {
        historicalFPS.push_back(fps);
        if (historicalFPS.size() > historySize) {
            historicalFPS.erase(historicalFPS.begin());
        }
    }

    double returnRollingAverageFPS() {
        double sum = 0;
        for (double fps : historicalFPS) {
            sum += fps;
        }
        return sum / historicalFPS.size();
    }

    double getFPS(sf::Clock& clock) {
        sf::Time timeSinceLastFrame = clock.getElapsedTime();
        clock.restart();
        return (1.0 / timeSinceLastFrame.asSeconds());
    }
};
DisplayFPS displayFPS;

class Mouse {
public:
    int x = 0;
    int y = 0;
    int prev_x = 0;
    int prev_y = 0;
    int rel_x = 0;
    int rel_y = 0;
    bool isOverScreen = false;

    void setMouseProperties(sf::Vector2i vect) {
        checkMouseOverScreen(vect);
        setMousePosition(vect);
        getRelativeMouseMovement(vect);
    }

    void checkMouseOverScreen(sf::Vector2i vect) {
        isOverScreen = false;
        if (0 < vect.x && vect.x < game.screenWidth) {
            if (0 < vect.y && vect.y < game.screenHeight) {
                isOverScreen = true;
            }
        }
    }

    void setMousePosition(sf::Vector2i vect) {
        if (0 < vect.x && vect.x < game.screenWidth) {
            x = vect.x;
        }
        if (0 < vect.y && vect.y < game.screenHeight) {
            y = vect.y;
        }
    }

    void getRelativeMouseMovement(sf::Vector2i vect) {
        rel_x = vect.x - prev_x;
        rel_y = vect.y - prev_y;
        prev_x = vect.x;
        prev_y = vect.y;
    }
};
Mouse mouse;

class PerlinVec3 {
public:
    double x = 0;
    double y = 0;

    PerlinVec3() {}

    PerlinVec3(double init_x, double init_y) {
        x = init_x;
        y = init_y;
    }

    void Randomize() { // set x and y to a random double bettween -1.0 and 1.0
        x = (2 * (static_cast<double>(rand()) / RAND_MAX)) - 1;
        y = (2 * (static_cast<double>(rand()) / RAND_MAX)) - 1;
    }

    void Normalize() {
        double magnitude = sqrt(x * x + y * y);
        x = x / magnitude;
        y = y / magnitude;
    }
};

class PerlinStruct {
public:
    int x = 0;
    int y = 0;
    int i = 0;
    int j = 0;
    int local_x = 0;
    int local_y = 0;

    PerlinVec3 distanceVector_A;
    PerlinVec3 distanceVector_B;
    PerlinVec3 distanceVector_C;
    PerlinVec3 distanceVector_D;

    PerlinVec3 gradientVector_A;
    PerlinVec3 gradientVector_B;
    PerlinVec3 gradientVector_C;
    PerlinVec3 gradientVector_D;

    double dot_A = 0;
    double dot_B = 0;
    double dot_C = 0;
    double dot_D = 0;

    double value = 0;

    char closest = 'x';

    PerlinStruct() {}

    PerlinStruct(int init_x, int init_y) {
        x = init_x;
        y = init_y;
    }
};

class PerlinNoise {
public:
    int size = 0;
    int grid = 0;
    int corners = 0;

    std::vector<std::vector<PerlinVec3>> gradientVectors;
    std::vector<std::vector<PerlinStruct>> pixels;

    double pi = 3.1415926536897;

    PerlinNoise() {}

    std::vector<std::vector<double>> Generate(int init_size, int init_grid) {
        Initialize(init_size, init_grid);
        PopulateGradientVectors();
        PopulatePixels();
        PopulatePixelData();
        PixelsGetDistanceVectors();
        PixelsGetGradientVectors();
        CalculateDotProducts();
        Interpolate();
        _FindClosestGradient();
        std::vector<std::vector<double>> noiseMap;
        noiseMap = GetValues();
        return noiseMap;
    }

    void Initialize(int init_size, int init_grid) {
        size = init_size;
        grid = init_grid;
        corners = (size / grid) + 1;
    }

    void PopulateGradientVectors() {
        for (int i = 0; i < corners; i++) {
            std::vector<PerlinVec3> column;
            for (int j = 0; j < corners; j++) {
                PerlinVec3 gradientVector;
                gradientVector.Randomize();
                gradientVector.Normalize();
                column.push_back(gradientVector);
            }
            gradientVectors.push_back(column);
        }
    }

    void PopulatePixels() {
        for (int i = 0; i < size; i++) {
            std::vector<PerlinStruct> column;
            for (int j = 0; j < size; j++) {
                PerlinStruct pixel(i, j);
                column.push_back(pixel);
            }
            pixels.push_back(column);
        }
    }

    void PopulatePixelData() {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                PerlinStruct& pixel = pixels[i][j];
                pixel.i = pixel.x / grid;
                pixel.j = pixel.y / grid;
                pixel.local_x = pixel.x - (grid * pixel.i);
                pixel.local_y = pixel.y - (grid * pixel.j);
            }
        }
    }

    void PixelsGetDistanceVectors() {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                PerlinStruct& pixel = pixels[i][j];
                pixel.distanceVector_A.x = (static_cast<double>(pixel.local_x) + 0.5) / grid;
                pixel.distanceVector_A.y = (static_cast<double>(pixel.local_y) + 0.5) / grid;
                pixel.distanceVector_C.x = -1 * (1.0 - pixel.distanceVector_A.x);
                pixel.distanceVector_C.y = -1 * (1.0 - pixel.distanceVector_A.y);
                pixel.distanceVector_D.x = pixel.distanceVector_A.x;
                pixel.distanceVector_D.y = pixel.distanceVector_C.y;
                pixel.distanceVector_B.x = pixel.distanceVector_C.x;
                pixel.distanceVector_B.y = pixel.distanceVector_A.y;
            }
        }
    }

    void PixelsGetGradientVectors() {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                PerlinStruct& pixel = pixels[i][j];
                pixel.gradientVector_A = gradientVectors[pixel.i    ][pixel.j];
                pixel.gradientVector_B = gradientVectors[pixel.i + 1][pixel.j];
                pixel.gradientVector_C = gradientVectors[pixel.i + 1][pixel.j + 1];
                pixel.gradientVector_D = gradientVectors[pixel.i    ][pixel.j + 1];
            }
        }
    }

    void CalculateDotProducts() {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                PerlinStruct& pixel = pixels[i][j];
                pixel.dot_A = (pixel.distanceVector_A.x * pixel.gradientVector_A.x) + (pixel.distanceVector_A.y * pixel.gradientVector_A.y);
                pixel.dot_B = (pixel.distanceVector_B.x * pixel.gradientVector_B.x) + (pixel.distanceVector_B.y * pixel.gradientVector_B.y);
                pixel.dot_C = (pixel.distanceVector_C.x * pixel.gradientVector_C.x) + (pixel.distanceVector_C.y * pixel.gradientVector_C.y);
                pixel.dot_D = (pixel.distanceVector_D.x * pixel.gradientVector_D.x) + (pixel.distanceVector_D.y * pixel.gradientVector_D.y);
            }
        }
    }

    void Interpolate() {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                PerlinStruct& pixel = pixels[i][j];
                double weight_right = (static_cast<double>(pixel.local_x) + 0.5) / grid;
                double weight_down = (static_cast<double>(pixel.local_y) + 0.5) / grid;
                double weight_left = 1.0 - weight_right;
                double weight_up = 1.0 - weight_down;

                weight_right = FadeFunction(weight_right);
                weight_down  = FadeFunction(weight_down);
                weight_left  = FadeFunction(weight_left);
                weight_up    = FadeFunction(weight_up);

                double top = pixel.dot_A * weight_left + pixel.dot_B * weight_right;
                double bottom = pixel.dot_D * weight_left + pixel.dot_C * weight_right;

                pixel.value = top * weight_up + bottom * weight_down;
            }
        }
    }

    double FadeFunction(double value) {
        return ((sin((value * pi) - (pi / 2))) / 2) + 0.5;
    }

    void _FindClosestGradient() {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                PerlinStruct& pixel = pixels[i][j];
                int half = grid / 2;
                if ((pixel.local_x <  half) && (pixel.local_y <  half)) { pixel.closest = 'a'; }
                if ((pixel.local_x >= half) && (pixel.local_y <  half)) { pixel.closest = 'b'; }
                if ((pixel.local_x >= half) && (pixel.local_y >= half)) { pixel.closest = 'c'; }
                if ((pixel.local_x <  half) && (pixel.local_y >= half)) { pixel.closest = 'd'; }
            }
        }
    }

    std::vector<std::vector<double>> GetValues() {
        std::vector<std::vector<double>> noiseMap;
        for (int i = 0; i < size; i++) {
            std::vector<double> column;
            for (int j = 0; j < size; j++) {
                PerlinStruct& pixel = pixels[i][j];
                double value = pixel.value;
                if (value > 1.0) { value = 1.0; }
                if (value < -1.0) { value = -1.0; }
                column.push_back(value);
            }
            noiseMap.push_back(column);
        }
        return noiseMap;
    }
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(game.screenWidth, game.screenHeight), "Hello SFML", sf::Style::Close);
    sf::Clock clock;

    int size = 800;
    int grid = 100;
    std::vector<std::vector<double>> noiseMap;
    PerlinNoise noise;
    noiseMap = noise.Generate(size, grid);

    // draw grid
    std::vector<sf::Vertex> gridVertices;
    for (int i = 0; i < (size / grid) + 1; i++) {
        gridVertices.push_back(sf::Vertex(sf::Vector2f(100 + i * grid, 100), sf::Color(255, 255, 255)));
        gridVertices.push_back(sf::Vertex(sf::Vector2f(100 + i * grid, 100 + size), sf::Color(255, 255, 255)));
    }
    for (int i = 0; i < (size / grid) + 1; i++) {
        gridVertices.push_back(sf::Vertex(sf::Vector2f(100, 100 + i * grid), sf::Color(255, 255, 255)));
        gridVertices.push_back(sf::Vertex(sf::Vector2f(100 + size, 100 + i * grid), sf::Color(255, 255, 255)));
    }

    // draw gradients
    const std::vector<std::vector<PerlinVec3>>& gradientVectors = noise.gradientVectors;
    std::vector<sf::Vertex> gradientVertices;
    for (int i = 0; i < (size / grid) + 1; i++) {
        for (int j = 0; j < (size / grid) + 1; j++) {
            const PerlinVec3& gradient = gradientVectors[i][j];
            gradientVertices.push_back(sf::Vertex(sf::Vector2f(100 + i * grid, 100 + j * grid), sf::Color(0, 255, 0)));
            gradientVertices.push_back(sf::Vertex(sf::Vector2f(100 + i * grid + gradient.x * grid, 100 + j * grid + gradient.y * grid), sf::Color(0, 255, 0)));
        }
    }

    // draw pixels
    std::vector<sf::Vertex> pixelVertices;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            double value = noiseMap[i][j];
            int color = 128 + value * 250;
            if (color < 0) { color = 0; }
            if (color > 255) { color = 255; }
            uint8_t rgb = color;
            pixelVertices.push_back(sf::Vertex(sf::Vector2f(100 + i, 100 + j), sf::Color(rgb, rgb, rgb)));
        }
    }

    while (window.isOpen()) {
        mouse.setMouseProperties(sf::Mouse::getPosition(window));

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        window.clear(sf::Color(0, 0, 0));
        window.draw(&pixelVertices[0], pixelVertices.size(), sf::Points);
        window.draw(&gridVertices[0], gridVertices.size(), sf::Lines);
        window.draw(&gradientVertices[0], gradientVertices.size(), sf::Lines);
        displayFPS.drawFPS(window, clock);
        window.display();
    }
    return 0;
}


//std::cerr << "\rGenerating Image: " << v << '/' << (canvas_height - 1) << std::flush;
//std::this_thread::sleep_for(std::chrono::milliseconds(10));

