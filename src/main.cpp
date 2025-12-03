import exo1;
#include "../include/audio.h"
#include "../include/fft.h"
#include "../include/fingerprint.h"
#include "../include/database.h"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

std::vector<fingerprint::Fingerprint> extractFingerprints(const std::string& audioPath, float& duration) {
    std::cout << "Chargement: " << audioPath << "\n";
    audio::WavData wav = audio::loadWav(audioPath);

    std::cout << "Sample Rate: " << wav.sampleRate << " Hz\n";
    std::cout << "Durée: " << (wav.samples.size() / (float)wav.sampleRate) << " secondes\n";

    duration = wav.samples.size() / (float)wav.sampleRate;

    int frameSize = 4096;
    int hopSize = 2048;

    auto frames = audio::makeFrames(wav.samples, frameSize, hopSize);
    auto window = audio::hannWindow(frameSize);

    std::cout << "" << frames.size() << " frames crees\n";

    std::vector<std::vector<fft::Peak>> allPeaks;
    for (size_t i = 0; i < frames.size(); ++i) {
        auto frame = frames[i];

        for (size_t j = 0; j < frame.size(); ++j) {
            frame[j] *= window[j];
        }

        auto fftResult = fft::computeFFT(frame);
        auto magnitude = fft::computeMagnitude(fftResult);

        float maxMag = 0.0f;
        for (float m : magnitude) {
            if (m > maxMag) maxMag = m;
        }

        float threshold = maxMag * 0.01f;
        auto peaks = fft::extractPeaks(magnitude, wav.sampleRate, threshold, 5);

        allPeaks.push_back(peaks);
    }

    auto fingerprints = fingerprint::generateFingerprints(allPeaks, 5, 3);
    std::cout << "" << fingerprints.size() << " fingerprints generes\n\n";

    return fingerprints;
}

// Phase d'indexation : charger tous les WAV du dossier /data/
void indexDatabase(database::Database& db) {
    std::cout << "\nPHASE D'INDEXATION\n\n";

    std::string dataDir = "../data/";

    if (!fs::exists(dataDir)) {
        std::cerr << "Erreur: le dossier " << dataDir << " n'existe pas!\n";
        return;
    }

    std::vector<std::string> wavFiles;
    for (const auto& entry : fs::directory_iterator(dataDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".wav") {
            wavFiles.push_back(entry.path().string());
        }
    }

    if (wavFiles.empty()) {
        std::cout << "Aucun fichier WAV trouvé dans " << dataDir << "\n";
        return;
    }

    std::cout << "Fichiers WAV trouvés: " << wavFiles.size() << "\n\n";

    for (const auto& wavPath : wavFiles) {
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

            std::cout << "Indexation: " << song.artist << " - " << song.title << "\n";

            float duration = 0.0f;
            auto fingerprints = extractFingerprints(wavPath, duration);
            song.duration = duration;

            int songId = db.addSong(song, fingerprints);
            std::cout << "✓ Enregistré avec ID: " << songId << "\n";

        } catch (const std::exception& e) {
            std::cerr << "✗ Erreur avec " << wavPath << ": " << e.what() << "\n\n";
        }
    }

    std::cout << "\n=== INDEXATION TERMINÉE ===\n";
}
void searchAudio(database::Database& db, const std::string& queryPath) {
    std::cout << "\n=== PHASE DE RECHERCHE ===\n\n";

    std::cout << "Fichier à identifier: " << queryPath << "\n";

    float duration = 0.0f;
    auto queryFingerprints = extractFingerprints(queryPath, duration);

    std::cout << "Recherche dans la base de données...\n";
    auto matches = db.search(queryFingerprints, 5);

    std::cout << "RÉSULTATS DE LA RECHERCHE\n";

    if (matches.empty()) {
        std::cout << "Aucune correspondance trouvée.\n";
        std::cout << "La chanson n'est pas dans la base de données.\n";
    } else {
        for (size_t i = 0; i < matches.size() && i < 5; ++i) {
            const auto& match = matches[i];
            std::cout << "Match #" << (i+1) << ":\n";
            std::cout << " Titre    : " << match.song.title << "\n";
            std::cout << " Artiste  : " << match.song.artist << "\n";
            std::cout << " Score    : " << match.score << " fingerprints matchés\n";
            std::cout << " Confiance: " << std::fixed << std::setprecision(1)
                      << match.confidence << "%\n";
            std::cout << " Offset   : " << match.timeOffset << " frames\n";

            if (match.confidence > 80.0f) {
                std::cout << " MATCH CONFIRMÉ!\n";
            } else if (match.confidence > 50.0f) {
                std::cout << " Match probable\n";
            } else {
                std::cout << " Match incertain\n";
            }
            std::cout << "\n";
        }
    }

}

void displayMenu() {
    std::cout << "1. Indexer tous les fichiers de /data/\n";
    std::cout << "2. Rechercher/Identifier un audio\n";
    std::cout << "3. Afficher les chansons en base\n";
    std::cout << "4. Vider la base de donnees\n";
    std::cout << "5. Quitter\n";
    std::cout << "\nChoix: ";
}

int main() {
    try {
        database::Database db("shazam.db");

        while (true) {
            displayMenu();

            int choice;
            std::cin >> choice;
            std::cin.ignore();

            switch (choice) {
                case 1:
                    indexDatabase(db);
                    break;

                case 2: {
                    std::cout << "\nChemin du fichier audio à identifier: ";
                    std::string queryPath;
                    std::getline(std::cin, queryPath);

                    if (!fs::exists(queryPath)) {
                        std::cerr << "Erreur: le fichier n'existe pas!\n";
                    } else {
                        searchAudio(db, queryPath);
                    }
                    break;
                }

                case 3: {
                    std::cout << "\nCHANSONS DANS LA BASE\n\n";
                    auto songs = db.getAllSongs();
                    if (songs.empty()) {
                        std::cout << "La base de données est vide.\n";
                    } else {
                        for (const auto& song : songs) {
                            std::cout << "[ID: " << song.id << "] "
                                      << song.artist << " - " << song.title
                                      << " (" << std::fixed << std::setprecision(1)
                                      << song.duration << "s)\n";
                        }
                    }
                    break;
                }

                case 4: {
                    std::cout << "\nÊtes-vous sûr de vouloir vider la base? (o/n): ";
                    char confirm;
                    std::cin >> confirm;
                    if (confirm == 'o' || confirm == 'O') {
                        db.clear();
                        std::cout << "Base de données vidée.\n";
                    }
                    break;
                }

                case 5:
                    std::cout << "\nAu revoir!\n";
                    return 0;

                default:
                    std::cout << "\nChoix invalide!\n";
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "\nErreur fatale: " << e.what() << "\n";
        return 1;
    }

    return 0;
}