#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "database.h"

namespace gui {

    enum class Screen {
        MAIN_MENU,
        INDEXING,
        SEARCHING,
        VIEW_SONGS,
        RESULTS
    };

    class Button {
    public:
        sf::RectangleShape shape;
        sf::Text text;
        std::function<void()> onClick;
        bool enabled = true;

        Button(const sf::Vector2f& position, const sf::Vector2f& size,
               const std::string& label, const sf::Font& font);

        bool contains(const sf::Vector2f& point) const;
        void draw(sf::RenderWindow& window);
        void setEnabled(bool enable);
    };

    class ProgressBar {
    public:
        sf::RectangleShape background;
        sf::RectangleShape fill;
        sf::Text label;
        float progress = 0.0f;

        ProgressBar(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Font& font);

        void setProgress(float value);
        void setText(const std::string& text);
        void draw(sf::RenderWindow& window);
    };

    class GUI {
    private:
        sf::RenderWindow window;
        sf::Font font;
        database::Database& db;

        Screen currentScreen;
        std::vector<Button> buttons;
        ProgressBar progressBar;

        std::string selectedAudioPath;
        std::vector<std::string> searchResults;
        std::string statusMessage;

        sf::Text titleText;
        sf::Text statusText;
        sf::RectangleShape resultBox;

        void setupMainMenu();
        void setupIndexingScreen();
        void setupSearchingScreen();
        void setupViewSongsScreen();
        void setupResultsScreen();

        void handleEvents();
        void update();
        void render();

        std::string openFileDialog();
        void indexAllFiles();
        void searchAudio();
        void displaySongs();
        void clearDatabase();

    public:
        GUI(database::Database& database);
        void run();
    };

} // namespace gui