#include "../include/gui.h"
#include "../include/audio.h"
#include "../include/fft.h"
#include "../include/fingerprint.h"
#include <iostream>
#include <filesystem>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

namespace fs = std::filesystem;

namespace gui {

    // Button implementation
    Button::Button(const sf::Vector2f& position, const sf::Vector2f& size,
                   const std::string& label, const sf::Font& font) {
        shape.setPosition(position);
        shape.setSize(size);
        shape.setFillColor(sf::Color(70, 130, 180));
        shape.setOutlineThickness(2);
        shape.setOutlineColor(sf::Color::White);

        text.setFont(font);
        text.setString(label);
        text.setCharacterSize(20);
        text.setFillColor(sf::Color::White);

        sf::FloatRect textBounds = text.getLocalBounds();
        text.setOrigin(textBounds.left + textBounds.width / 2.0f,
                      textBounds.top + textBounds.height / 2.0f);
        text.setPosition(position.x + size.x / 2.0f, position.y + size.y / 2.0f);
    }

    bool Button::contains(const sf::Vector2f& point) const {
        return enabled && shape.getGlobalBounds().contains(point);
    }

    void Button::draw(sf::RenderWindow& window) {
        if (!enabled) {
            shape.setFillColor(sf::Color(50, 50, 50));
            text.setFillColor(sf::Color(100, 100, 100));
        } else {
            shape.setFillColor(sf::Color(70, 130, 180));
            text.setFillColor(sf::Color::White);
        }
        window.draw(shape);
        window.draw(text);
    }

    void Button::setEnabled(bool enable) {
        enabled = enable;
    }

    // ProgressBar implementation
    ProgressBar::ProgressBar(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Font& font) {
        background.setPosition(position);
        background.setSize(size);
        background.setFillColor(sf::Color(50, 50, 50));
        background.setOutlineThickness(2);
        background.setOutlineColor(sf::Color::White);

        fill.setPosition(position);
        fill.setSize(sf::Vector2f(0, size.y));
        fill.setFillColor(sf::Color(0, 200, 0));

        label.setFont(font);
        label.setCharacterSize(18);
        label.setFillColor(sf::Color::White);
        label.setPosition(position.x + 10, position.y + size.y + 10);
    }

    void ProgressBar::setProgress(float value) {
        progress = std::max(0.0f, std::min(1.0f, value));
        fill.setSize(sf::Vector2f(background.getSize().x * progress, background.getSize().y));
    }

    void ProgressBar::setText(const std::string& text) {
        label.setString(text);
    }

    void ProgressBar::draw(sf::RenderWindow& window) {
        window.draw(background);
        window.draw(fill);
        window.draw(label);
    }

    // GUI implementation
    GUI::GUI(database::Database& database)
        : window(sf::VideoMode(1000, 700), "Shazam Local - Audio Recognition"),
          db(database),
          currentScreen(Screen::MAIN_MENU),
          progressBar(sf::Vector2f(100, 500), sf::Vector2f(800, 40), font) {

        if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
            std::cerr << "Failed to load font!" << std::endl;
        }

        titleText.setFont(font);
        titleText.setCharacterSize(48);
        titleText.setFillColor(sf::Color(70, 130, 180));
        titleText.setString("Shazam Local");
        titleText.setPosition(300, 50);

        statusText.setFont(font);
        statusText.setCharacterSize(18);
        statusText.setFillColor(sf::Color::White);
        statusText.setPosition(50, 600);

        resultBox.setPosition(50, 200);
        resultBox.setSize(sf::Vector2f(900, 350));
        resultBox.setFillColor(sf::Color(30, 30, 30));
        resultBox.setOutlineThickness(2);
        resultBox.setOutlineColor(sf::Color::White);

        setupMainMenu();
    }

    std::string GUI::openFileDialog() {
#ifdef _WIN32
        OPENFILENAMEA ofn;
        char szFile[260] = {0};

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Audio Files\0*.WAV;*.wav\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameA(&ofn) == TRUE) {
            return std::string(ofn.lpstrFile);
        }
#endif
        return "";
    }

    void GUI::setupMainMenu() {
        buttons.clear();
        currentScreen = Screen::MAIN_MENU;
        statusMessage = "Bienvenue! Choisissez une option ci-dessous.";

        buttons.emplace_back(sf::Vector2f(300, 200), sf::Vector2f(400, 60),
                            "Indexer tous les fichiers /data/", font);
        buttons.back().onClick = [this]() { setupIndexingScreen(); };

        buttons.emplace_back(sf::Vector2f(300, 280), sf::Vector2f(400, 60),
                            "Rechercher / Identifier un audio", font);
        buttons.back().onClick = [this]() { setupSearchingScreen(); };

        buttons.emplace_back(sf::Vector2f(300, 360), sf::Vector2f(400, 60),
                            "Afficher les chansons en base", font);
        buttons.back().onClick = [this]() { displaySongs(); };

        buttons.emplace_back(sf::Vector2f(300, 440), sf::Vector2f(400, 60),
                            "Vider la base de donnees", font);
        buttons.back().onClick = [this]() { clearDatabase(); };

        buttons.emplace_back(sf::Vector2f(300, 520), sf::Vector2f(400, 60),
                            "Quitter", font);
        buttons.back().onClick = [this]() { window.close(); };
    }

    void GUI::setupIndexingScreen() {
        buttons.clear();
        currentScreen = Screen::INDEXING;
        statusMessage = "Pret a indexer les fichiers du dossier /data/";

        buttons.emplace_back(sf::Vector2f(300, 400), sf::Vector2f(400, 60),
                            "Lancer l'indexation", font);
        buttons.back().onClick = [this]() { indexAllFiles(); };

        buttons.emplace_back(sf::Vector2f(300, 480), sf::Vector2f(400, 60),
                            "Retour au menu", font);
        buttons.back().onClick = [this]() { setupMainMenu(); };
    }

    void GUI::setupSearchingScreen() {
        buttons.clear();
        currentScreen = Screen::SEARCHING;
        statusMessage = "Selectionnez un fichier audio a identifier";

        buttons.emplace_back(sf::Vector2f(300, 400), sf::Vector2f(400, 60),
                            "Choisir un fichier audio", font);
        buttons.back().onClick = [this]() {
            selectedAudioPath = openFileDialog();
            if (!selectedAudioPath.empty()) {
                statusMessage = "Fichier selectionne: " + fs::path(selectedAudioPath).filename().string();
            }
        };

        buttons.emplace_back(sf::Vector2f(300, 480), sf::Vector2f(400, 60),
                            "Lancer la recherche", font);
        buttons.back().onClick = [this]() { searchAudio(); };

        buttons.emplace_back(sf::Vector2f(300, 560), sf::Vector2f(400, 60),
                            "Retour au menu", font);
        buttons.back().onClick = [this]() { setupMainMenu(); };
    }

    void GUI::setupResultsScreen() {
        buttons.clear();
        currentScreen = Screen::RESULTS;

        buttons.emplace_back(sf::Vector2f(400, 600), sf::Vector2f(200, 50),
                            "Retour au menu", font);
        buttons.back().onClick = [this]() { setupMainMenu(); };
    }

    void GUI::indexAllFiles() {
        statusMessage = "Indexation en cours...";
        progressBar.setProgress(0.0f);
        progressBar.setText("Recherche des fichiers...");

        std::string dataDir = "../data/";
        if (!fs::exists(dataDir)) {
            statusMessage = "Erreur: le dossier " + dataDir + " n'existe pas!";
            return;
        }

        std::vector<std::string> wavFiles;
        for (const auto& entry : fs::directory_iterator(dataDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".wav") {
                wavFiles.push_back(entry.path().string());
            }
        }

        if (wavFiles.empty()) {
            statusMessage = "Aucun fichier WAV trouve dans " + dataDir;
            return;
        }

        for (size_t i = 0; i < wavFiles.size(); ++i) {
            const auto& wavPath = wavFiles[i];
            float progress = (float)(i + 1) / wavFiles.size();
            progressBar.setProgress(progress);
            progressBar.setText("Traitement: " + fs::path(wavPath).filename().string() +
                              " (" + std::to_string(i + 1) + "/" + std::to_string(wavFiles.size()) + ")");

            render();

            try {
                std::string filename = fs::path(wavPath).filename().string();
                std::string nameWithoutExt = filename.substr(0, filename.find_last_of('.'));
                size_t underscorePos = nameWithoutExt.find('_');

                database::Song song;
                if (underscorePos != std::string::npos) {
                    song.artist = nameWithoutExt.substr(0, underscorePos);
                    song.title = nameWithoutExt.substr(underscorePos + 1);
                    for (char& c : song.artist) if (c == '_') c = ' ';
                    for (char& c : song.title) if (c == '_') c = ' ';
                } else {
                    song.artist = "Unknown";
                    song.title = nameWithoutExt;
                }

                audio::WavData wav = audio::loadWav(wavPath);
                song.duration = wav.samples.size() / (float)wav.sampleRate;

                int frameSize = 4096;
                int hopSize = 2048;
                auto frames = audio::makeFrames(wav.samples, frameSize, hopSize);
                auto window = audio::hannWindow(frameSize);

                std::vector<std::vector<fft::Peak>> allPeaks;
                for (auto& frame : frames) {
                    for (size_t j = 0; j < frame.size(); ++j) {
                        frame[j] *= window[j];
                    }
                    auto fftResult = fft::computeFFT(frame);
                    auto magnitude = fft::computeMagnitude(fftResult);
                    float maxMag = *std::max_element(magnitude.begin(), magnitude.end());
                    float threshold = maxMag * 0.01f;
                    auto peaks = fft::extractPeaks(magnitude, wav.sampleRate, threshold, 5);
                    allPeaks.push_back(peaks);
                }

                auto fingerprints = fingerprint::generateFingerprints(allPeaks, 5, 3);
                db.addSong(song, fingerprints);

            } catch (const std::exception& e) {
                std::cerr << "Erreur avec " << wavPath << ": " << e.what() << "\n";
            }
        }

        statusMessage = "Indexation terminee! " + std::to_string(wavFiles.size()) + " fichiers traites.";
        progressBar.setProgress(1.0f);
        progressBar.setText("Termine!");
    }

    void GUI::searchAudio() {
        if (selectedAudioPath.empty()) {
            statusMessage = "Veuillez d'abord selectionner un fichier audio!";
            return;
        }

        if (!fs::exists(selectedAudioPath)) {
            statusMessage = "Erreur: le fichier n'existe pas!";
            return;
        }

        statusMessage = "Analyse en cours...";
        progressBar.setProgress(0.3f);
        progressBar.setText("Extraction des fingerprints...");
        render();

        try {
            audio::WavData wav = audio::loadWav(selectedAudioPath);
            float duration = wav.samples.size() / (float)wav.sampleRate;

            int frameSize = 4096;
            int hopSize = 2048;
            auto frames = audio::makeFrames(wav.samples, frameSize, hopSize);
            auto window = audio::hannWindow(frameSize);

            progressBar.setProgress(0.5f);
            progressBar.setText("Calcul FFT...");
            render();

            std::vector<std::vector<fft::Peak>> allPeaks;
            for (auto& frame : frames) {
                for (size_t j = 0; j < frame.size(); ++j) {
                    frame[j] *= window[j];
                }
                auto fftResult = fft::computeFFT(frame);
                auto magnitude = fft::computeMagnitude(fftResult);
                float maxMag = *std::max_element(magnitude.begin(), magnitude.end());
                float threshold = maxMag * 0.01f;
                auto peaks = fft::extractPeaks(magnitude, wav.sampleRate, threshold, 5);
                allPeaks.push_back(peaks);
            }

            progressBar.setProgress(0.7f);
            progressBar.setText("Generation des fingerprints...");
            render();

            auto queryFingerprints = fingerprint::generateFingerprints(allPeaks, 5, 3);

            progressBar.setProgress(0.9f);
            progressBar.setText("Recherche dans la base...");
            render();

            auto matches = db.search(queryFingerprints, 5);

            searchResults.clear();
            if (matches.empty()) {
                searchResults.push_back("Aucune correspondance trouvee.");
                searchResults.push_back("La chanson n'est pas dans la base de donnees.");
            } else {
                for (size_t i = 0; i < matches.size() && i < 5; ++i) {
                    const auto& match = matches[i];
                    std::ostringstream oss;
                    oss << "Match #" << (i + 1) << ": " << match.song.artist << " - " << match.song.title;
                    searchResults.push_back(oss.str());

                    oss.str("");
                    oss << "  Score: " << match.score << " | Confiance: "
                        << std::fixed << std::setprecision(1) << match.confidence << "%";
                    searchResults.push_back(oss.str());

                    if (match.confidence > 80.0f) {
                        searchResults.push_back("  MATCH CONFIRME!");
                    } else if (match.confidence > 50.0f) {
                        searchResults.push_back("  Match probable");
                    } else {
                        searchResults.push_back("  Match incertain");
                    }
                    searchResults.push_back("");
                }
            }

            statusMessage = "Recherche terminee!";
            progressBar.setProgress(1.0f);
            progressBar.setText("Termine!");
            setupResultsScreen();

        } catch (const std::exception& e) {
            statusMessage = "Erreur: " + std::string(e.what());
            progressBar.setProgress(0.0f);
        }
    }

    void GUI::displaySongs() {
        searchResults.clear();
        auto songs = db.getAllSongs();

        if (songs.empty()) {
            searchResults.push_back("La base de donnees est vide.");
        } else {
            searchResults.push_back("=== CHANSONS DANS LA BASE ===");
            searchResults.push_back("");
            for (const auto& song : songs) {
                std::ostringstream oss;
                oss << "[ID: " << song.id << "] " << song.artist << " - " << song.title
                    << " (" << std::fixed << std::setprecision(1) << song.duration << "s)";
                searchResults.push_back(oss.str());
            }
        }

        statusMessage = std::to_string(songs.size()) + " chanson(s) dans la base";
        setupResultsScreen();
    }

    void GUI::clearDatabase() {
        db.clear();
        statusMessage = "Base de donnees videe avec succes!";
    }

    void GUI::handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
                    for (auto& button : buttons) {
                        if (button.contains(mousePos) && button.onClick) {
                            button.onClick();
                        }
                    }
                }
            }
        }
    }

    void GUI::update() {
        // Future: animation updates
    }

    void GUI::render() {
        window.clear(sf::Color(20, 20, 30));

        window.draw(titleText);

        for (auto& button : buttons) {
            button.draw(window);
        }

        if (currentScreen == Screen::INDEXING || currentScreen == Screen::SEARCHING) {
            progressBar.draw(window);
        }

        if (currentScreen == Screen::RESULTS) {
            window.draw(resultBox);

            sf::Text resultText;
            resultText.setFont(font);
            resultText.setCharacterSize(16);
            resultText.setFillColor(sf::Color::White);

            float yPos = 220;
            for (const auto& line : searchResults) {
                resultText.setString(line);
                resultText.setPosition(70, yPos);
                window.draw(resultText);
                yPos += 25;
                if (yPos > 520) break;
            }
        }

        statusText.setString(statusMessage);
        window.draw(statusText);

        window.display();
    }

    void GUI::run() {
        while (window.isOpen()) {
            handleEvents();
            update();
            render();
        }
    }

} // namespace gui