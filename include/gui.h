#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include "database.h"

namespace gui {

    enum class Screen {
        MAIN_MENU,
        INDEXING,
        INDEXING_SINGLE,
        SEARCHING,
        VIEW_SONGS,
        RESULTS,
        CONFIRM_CLEAR
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
        std::vector<std::string> searchHistory;  // History of searches
        std::string statusMessage;

        sf::Text titleText;
        sf::Text statusText;
        sf::RectangleShape resultBox;

        // Multithreading
        std::atomic<bool> processingActive{false};
        std::atomic<float> processingProgress{0.0f};
        std::mutex statusMutex;
        std::string processingStatus;
        std::thread workerThread;

        void setupMainMenu();
        void setupIndexingScreen();
        void setupIndexingSingleScreen();
        void setupSearchingScreen();
        void setupViewSongsScreen();
        void setupResultsScreen();
        void setupConfirmClearScreen();
        void displayStats();
        void displayHistory();

        void handleEvents();
        void update();
        void render();

        std::string openFileDialog();
        void indexAllFiles();
        void indexSingleFile();
        void searchAudio();
        void displaySongs();
        void clearDatabase();

    public:
        GUI(database::Database& database);
        ~GUI();
        void run();
    };

} // namespace gui