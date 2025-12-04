#include "../include/gui.h"
#include "../include/audio.h"
#include "../include/fft.h"
#include "../include/fingerprint.h"
#include <iostream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

namespace fs = std::filesystem;

namespace gui {

    // Button implementation - Design moderne
    Button::Button(const sf::Vector2f& position, const sf::Vector2f& size,
                   const std::string& label, const sf::Font& font)
        : text(font) {
        shape.setPosition(position);
        shape.setSize(size);
        shape.setFillColor(sf::Color(30, 215, 96));  // Spotify green
        shape.setOutlineThickness(0);

        text.setString(label);
        text.setCharacterSize(22);
        text.setFillColor(sf::Color::White);
        text.setStyle(sf::Text::Bold);

        sf::FloatRect textBounds = text.getLocalBounds();
        text.setOrigin({textBounds.position.x + textBounds.size.x / 2.0f,
                       textBounds.position.y + textBounds.size.y / 2.0f});
        text.setPosition({position.x + size.x / 2.0f, position.y + size.y / 2.0f});
    }

    bool Button::contains(const sf::Vector2f& point) const {
        return enabled && shape.getGlobalBounds().contains(point);
    }

    void Button::draw(sf::RenderWindow& window) {
        if (!enabled) {
            shape.setFillColor(sf::Color(60, 60, 60));
            text.setFillColor(sf::Color(120, 120, 120));
        } else {
            // Hover effect
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            if (shape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                shape.setFillColor(sf::Color(40, 235, 116));  // Lighter green on hover
            } else {
                shape.setFillColor(sf::Color(30, 215, 96));  // Normal green
            }
            text.setFillColor(sf::Color::White);
        }
        window.draw(shape);
        window.draw(text);
    }

    void Button::setEnabled(bool enable) {
        enabled = enable;
    }

    // ProgressBar implementation - Design moderne
    ProgressBar::ProgressBar(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Font& font)
        : label(font) {
        background.setPosition(position);
        background.setSize(size);
        background.setFillColor(sf::Color(40, 40, 40));
        background.setOutlineThickness(0);

        fill.setPosition(position);
        fill.setSize(sf::Vector2f(0, size.y));
        fill.setFillColor(sf::Color(30, 215, 96));  // Spotify green

        label.setCharacterSize(16);
        label.setFillColor(sf::Color(200, 200, 200));
        label.setPosition({position.x, position.y + size.y + 10});
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

    // GUI implementation - Design moderne sombre
    GUI::GUI(database::Database& database)
        : window(sf::VideoMode{{1000, 700}}, "Shazam Local - Reconnaissance Audio"),
          db(database),
          currentScreen(Screen::MAIN_MENU),
          titleText(font),
          statusText(font),
          progressBar(sf::Vector2f(100, 580), sf::Vector2f(800, 30), font) {

        if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
            std::cerr << "Failed to load font!" << std::endl;
        }

        titleText.setCharacterSize(56);
        titleText.setFillColor(sf::Color(30, 215, 96));  // Spotify green
        titleText.setString("Shazam Local");
        titleText.setStyle(sf::Text::Bold);

        sf::FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setOrigin({titleBounds.position.x + titleBounds.size.x / 2.0f,
                            titleBounds.position.y + titleBounds.size.y / 2.0f});
        titleText.setPosition({500, 60});

        statusText.setCharacterSize(15);
        statusText.setFillColor(sf::Color(180, 180, 180));
        statusText.setPosition({100, 655});

        resultBox.setPosition({50, 180});
        resultBox.setSize(sf::Vector2f(900, 370));
        resultBox.setFillColor(sf::Color(25, 25, 25));
        resultBox.setOutlineThickness(0);

        setupMainMenu();
    }

    GUI::~GUI() {
        if (workerThread.joinable()) {
            workerThread.join();
        }
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

        auto songs = db.getAllSongs();
        statusMessage = "Base de donnees: " + std::to_string(songs.size()) + " chansons | " +
                       std::to_string(searchHistory.size()) + " recherches effectuees";

        buttons.emplace_back(sf::Vector2f(150, 200), sf::Vector2f(280, 55),
                            "Indexer /data/", font);
        buttons.back().onClick = [this]() { setupIndexingScreen(); };

        buttons.emplace_back(sf::Vector2f(150, 275), sf::Vector2f(280, 55),
                            "Identifier fichier", font);
        buttons.back().onClick = [this]() { setupSearchingScreen(); };

        buttons.emplace_back(sf::Vector2f(150, 350), sf::Vector2f(280, 55),
                            "Voir chansons", font);
        buttons.back().onClick = [this]() { displaySongs(); };

        buttons.emplace_back(sf::Vector2f(570, 200), sf::Vector2f(280, 55),
                            "Statistiques BDD", font);
        buttons.back().onClick = [this]() { displayStats(); };

        buttons.emplace_back(sf::Vector2f(570, 275), sf::Vector2f(280, 55),
                            "Historique", font);
        buttons.back().onClick = [this]() { displayHistory(); };

        buttons.emplace_back(sf::Vector2f(570, 350), sf::Vector2f(280, 55),
                            "Vider BDD", font);
        buttons.back().onClick = [this]() { clearDatabase(); };

        buttons.emplace_back(sf::Vector2f(360, 450), sf::Vector2f(280, 55),
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

        buttons.emplace_back(sf::Vector2f(300, 300), sf::Vector2f(400, 60),
                            "Choisir un fichier audio", font);
        buttons.back().onClick = [this]() {
            selectedAudioPath = openFileDialog();
            if (!selectedAudioPath.empty()) {
                statusMessage = "Fichier selectionne: " + fs::path(selectedAudioPath).filename().string();
            }
        };

        buttons.emplace_back(sf::Vector2f(300, 380), sf::Vector2f(400, 60),
                            "Lancer la recherche", font);
        buttons.back().onClick = [this]() { searchAudio(); };

        buttons.emplace_back(sf::Vector2f(300, 460), sf::Vector2f(400, 60),
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

            handleEvents();
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
                for (size_t frameIdx = 0; frameIdx < frames.size(); ++frameIdx) {
                    auto& frame = frames[frameIdx];

                    // Update UI every 100 frames
                    if (frameIdx % 100 == 0) {
                        handleEvents();
                    }

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
        handleEvents();
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
            handleEvents();
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
            handleEvents();
            render();

            // Utiliser la version optimisée pour la recherche (limite aux premières secondes)
            auto queryFingerprints = fingerprint::generateFingerprintsForQuery(allPeaks, 5, 3);

            progressBar.setProgress(0.9f);
            progressBar.setText("Recherche dans la base...");
            handleEvents();
            render();

            auto matches = db.search(queryFingerprints, 5);

            // Filtrer pour ne garder que les matches avec confiance >= 90%
            std::vector<database::Match> goodMatches;
            for (const auto& match : matches) {
                if (match.confidence >= 90.0f) {
                    goodMatches.push_back(match);
                }
            }

            searchResults.clear();
            if (goodMatches.empty()) {
                searchResults.push_back("Aucun son trouve.");
                searchResults.push_back("La chanson n'est pas dans la base de donnees");
                searchResults.push_back("ou la qualite audio est insuffisante.");

                // Add to history
                auto now = std::time(nullptr);
                auto tm = std::localtime(&now);
                std::ostringstream historyEntry;
                historyEntry << "[" << std::put_time(tm, "%H:%M:%S") << "] Fichier: "
                            << fs::path(selectedAudioPath).filename().string()
                            << " - Aucun match";
                searchHistory.push_back(historyEntry.str());
            } else {
                // Afficher uniquement les meilleurs matches (confiance >= 90%)
                for (size_t i = 0; i < goodMatches.size() && i < 3; ++i) {
                    const auto& match = goodMatches[i];
                    std::ostringstream oss;
                    oss << "Match #" << (i + 1) << ": " << match.song.artist << " - " << match.song.title;
                    searchResults.push_back(oss.str());

                    oss.str("");
                    oss << "  Score: " << match.score << " | Confiance: "
                        << std::fixed << std::setprecision(1) << match.confidence << "%";
                    searchResults.push_back(oss.str());

                    // Calculate time in song
                    int timeInSeconds = (match.timeOffset * hopSize) / wav.sampleRate;
                    int minutes = timeInSeconds / 60;
                    int seconds = timeInSeconds % 60;
                    oss.str("");
                    oss << "  Trouve a " << minutes << "m " << seconds << "s dans la chanson";
                    searchResults.push_back(oss.str());

                    searchResults.push_back("  MATCH CONFIRME!");
                    searchResults.push_back("");

                    // Add to history (first match only)
                    if (i == 0) {
                        auto now = std::time(nullptr);
                        auto tm = std::localtime(&now);
                        std::ostringstream historyEntry;
                        historyEntry << "[" << std::put_time(tm, "%H:%M:%S") << "] Fichier: "
                                    << fs::path(selectedAudioPath).filename().string()
                                    << " -> " << match.song.artist << " - " << match.song.title
                                    << " (" << std::fixed << std::setprecision(0) << match.confidence << "%)";
                        searchHistory.push_back(historyEntry.str());
                    }
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

    void GUI::displayStats() {
        searchResults.clear();
        auto songs = db.getAllSongs();

        // Count total fingerprints
        int totalFingerprints = 0;
        for (const auto& song : songs) {
            // We need to access the database internals for this
            // For now, estimate based on song duration (approx 100 fingerprints per second)
            totalFingerprints += static_cast<int>(song.duration * 100);
        }

        searchResults.push_back("=== STATISTIQUES DE LA BASE DE DONNEES ===");
        searchResults.push_back("");
        searchResults.push_back("Nombre total de chansons: " + std::to_string(songs.size()));
        searchResults.push_back("");
        searchResults.push_back("Fingerprints estimes: ~" + std::to_string(totalFingerprints));
        searchResults.push_back("");

        // Calculate total duration
        float totalDuration = 0.0f;
        for (const auto& song : songs) {
            totalDuration += song.duration;
        }
        int totalMinutes = static_cast<int>(totalDuration / 60);
        int totalSeconds = static_cast<int>(totalDuration) % 60;

        searchResults.push_back("Duree totale indexee: " + std::to_string(totalMinutes) + "m " +
                               std::to_string(totalSeconds) + "s");
        searchResults.push_back("");

        // Database file size
        if (fs::exists("shazam.db")) {
            auto fileSize = fs::file_size("shazam.db");
            float fileSizeMB = fileSize / (1024.0f * 1024.0f);
            std::ostringstream oss;
            oss << "Taille du fichier BDD: " << std::fixed << std::setprecision(2) << fileSizeMB << " MB";
            searchResults.push_back(oss.str());
        }
        searchResults.push_back("");

        searchResults.push_back("Nombre de recherches effectuees: " + std::to_string(searchHistory.size()));
        searchResults.push_back("");
        searchResults.push_back("Seuil de confiance: >= 90%");
        searchResults.push_back("Duree d'analyse par query: ~5 secondes");

        statusMessage = std::to_string(songs.size()) + " chansons dans la base";
        setupResultsScreen();
    }

    void GUI::displayHistory() {
        searchResults.clear();

        searchResults.push_back("=== HISTORIQUE DES RECHERCHES ===");
        searchResults.push_back("");

        if (searchHistory.empty()) {
            searchResults.push_back("Aucune recherche effectuee pour le moment.");
        } else {
            // Show last 15 searches (most recent first)
            int startIdx = std::max(0, static_cast<int>(searchHistory.size()) - 15);
            for (int i = searchHistory.size() - 1; i >= startIdx; --i) {
                searchResults.push_back(searchHistory[i]);
            }

            searchResults.push_back("");
            searchResults.push_back("Total: " + std::to_string(searchHistory.size()) + " recherche(s)");
        }

        statusMessage = "Affichage des " + std::to_string(std::min(15, static_cast<int>(searchHistory.size()))) +
                       " dernieres recherches";
        setupResultsScreen();
    }


    void GUI::handleEvents() {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mousePos(static_cast<float>(mousePressed->position.x),
                                         static_cast<float>(mousePressed->position.y));
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
        window.clear(sf::Color(18, 18, 18));  // Dark background

        // Sous-titre discret
        sf::Text subtitle(font);
        subtitle.setCharacterSize(18);
        subtitle.setFillColor(sf::Color(150, 150, 150));
        subtitle.setString("Reconnaissance audio locale");
        sf::FloatRect subBounds = subtitle.getLocalBounds();
        subtitle.setOrigin({subBounds.position.x + subBounds.size.x / 2.0f,
                           subBounds.position.y + subBounds.size.y / 2.0f});
        subtitle.setPosition({500, 110});

        window.draw(titleText);
        window.draw(subtitle);

        for (auto& button : buttons) {
            button.draw(window);
        }

        if (currentScreen == Screen::INDEXING || currentScreen == Screen::SEARCHING) {
            progressBar.draw(window);
        }

        if (currentScreen == Screen::RESULTS || currentScreen == Screen::VIEW_SONGS) {
            window.draw(resultBox);

            sf::Text resultText(font);
            resultText.setCharacterSize(17);
            resultText.setFillColor(sf::Color(220, 220, 220));

            float yPos = 200;
            for (const auto& line : searchResults) {
                // Colorer differemment selon le type de ligne
                if (line.find("Match #") != std::string::npos ||
                    line.find("ID:") != std::string::npos) {
                    resultText.setFillColor(sf::Color(30, 215, 96));  // Green for titles
                    resultText.setStyle(sf::Text::Bold);
                } else if (line.find("MATCH CONFIRME") != std::string::npos) {
                    resultText.setFillColor(sf::Color(30, 215, 96));
                    resultText.setStyle(sf::Text::Bold);
                } else if (line.find("Confiance") != std::string::npos) {
                    resultText.setFillColor(sf::Color(180, 180, 180));
                    resultText.setStyle(sf::Text::Regular);
                } else {
                    resultText.setFillColor(sf::Color(200, 200, 200));
                    resultText.setStyle(sf::Text::Regular);
                }

                resultText.setString(line);
                resultText.setPosition({70, yPos});
                window.draw(resultText);
                yPos += 28;
                if (yPos > 530) break;
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