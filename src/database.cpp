#include "../include/database.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <unordered_map>

namespace database {

    Database::Database(const std::string& path) : dbPath(path) {
        loadFromFile();
    }

    Database::~Database() {
        saveToFile();
    }

    void Database::loadFromFile() {
        std::ifstream file(dbPath);
        if (!file.is_open()) {
            std::cout << "Creating new database at: " << dbPath << std::endl;
            return;
        }

        try {
            std::string line;
            std::string section;

            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue;

                if (line == "[SONGS]") {
                    section = "songs";
                    continue;
                }
                else if (line == "[FINGERPRINTS]") {
                    section = "fingerprints";
                    continue;
                }

                if (section == "songs") {
                    std::istringstream iss(line);
                    Song song;
                    char delimiter;

                    if (iss >> song.id >> delimiter &&
                        std::getline(iss, song.title, '|') &&
                        std::getline(iss, song.artist, '|') &&
                        iss >> song.duration) {

                        songs[song.id] = song;
                        if (song.id >= nextSongId) {
                            nextSongId = song.id + 1;
                        }
                    }
                }
                else if (section == "fingerprints") {
                    std::istringstream iss(line);
                    uint64_t hash;
                    int songId;
                    int timeOffset;
                    char delimiter;

                    if (iss >> hash >> delimiter >> songId >> delimiter >> timeOffset) {
                        fingerprintIndex[hash][songId].push_back(timeOffset);
                        songFingerprintCounts[songId]++; // Mettre à jour le cache
                    }
                }
            }

            std::cout << "Loaded " << songs.size() << " songs and "
                      << fingerprintIndex.size() << " unique fingerprint hashes" << std::endl;

        }
        catch (const std::exception& e) {
            std::cerr << "Error loading database: " << e.what() << std::endl;
        }

        file.close();
    }

    void Database::saveToFile() {
        std::ofstream file(dbPath);
        if (!file.is_open()) {
            std::cerr << "Cannot save database to: " << dbPath << std::endl;
            return;
        }

        file << "# Shazam Local Database\n";
        file << "# Format: Simple text-based storage\n\n";

        file << "[SONGS]\n";
        file << "# id|title|artist|duration\n";
        for (const auto& [id, song] : songs) {
            file << id << "|" << song.title << "|" << song.artist << "|" << song.duration << "\n";
        }

        file << "\n[FINGERPRINTS]\n";
        file << "# hash|songId|timeOffset\n";
        for (const auto& [hash, songMap] : fingerprintIndex) {
            for (const auto& [songId, offsets] : songMap) {
                for (int timeOffset : offsets) {
                    file << hash << "|" << songId << "|" << timeOffset << "\n";
                }
            }
        }

        file.close();
        std::cout << "Database saved to: " << dbPath << std::endl;
    }

    int Database::addSong(const Song& song, const std::vector<fingerprint::Fingerprint>& fingerprints) {
        int songId = nextSongId++;

        Song newSong = song;
        newSong.id = songId;
        songs[songId] = newSong;

        // Nouvelle structure multi-niveau : hash -> songId -> vector<timeOffset>
        for (const auto& fp : fingerprints) {
            fingerprintIndex[fp.hash][songId].push_back(fp.timeOffset);
        }

        // Mettre à jour le cache
        songFingerprintCounts[songId] = fingerprints.size();

        std::cout << "Added song ID " << songId << ": " << song.title
                  << " with " << fingerprints.size() << " fingerprints" << std::endl;

        saveToFile();
        return songId;
    }

    std::vector<Match> Database::search(const std::vector<fingerprint::Fingerprint>& queryFingerprints, int minScore) {
        auto searchStart = std::chrono::high_resolution_clock::now();

        std::cout << "\n=== DEBUT RECHERCHE ===" << std::endl;
        std::cout << "Nombre de fingerprints de la query: " << queryFingerprints.size() << std::endl;
        std::cout << "Nombre de hashs uniques dans la BDD: " << fingerprintIndex.size() << std::endl;

        // Structure optimisée : unordered_map pour O(1) + tracking du meilleur offset
        struct SongMatch {
            std::unordered_map<int, int> offsetCounts;  // offset -> count
            int bestOffset = 0;
            int bestCount = 0;
        };

        std::unordered_map<int, SongMatch> matchCounts;  // songId -> SongMatch
        int totalMatches = 0;

        // Phase 1: Compter les matches avec la nouvelle structure multi-niveau
        // Avantage : accès direct par songId, pas besoin de parcourir tous les fingerprints
        for (const auto& queryFp : queryFingerprints) {
            auto hashIt = fingerprintIndex.find(queryFp.hash);
            if (hashIt != fingerprintIndex.end()) {
                totalMatches++;
                // Parcourir uniquement les chansons qui ont ce hash (structure optimisée)
                const auto& songMap = hashIt->second;
                for (const auto& [songId, offsets] : songMap) {
                    auto& songMatch = matchCounts[songId];

                    // Parcourir tous les offsets de cette chanson pour ce hash
                    for (int storedOffset : offsets) {
                        int offsetDelta = storedOffset - queryFp.timeOffset;
                        int newCount = ++songMatch.offsetCounts[offsetDelta];

                        // Mettre à jour le meilleur offset en temps réel
                        if (newCount > songMatch.bestCount) {
                            songMatch.bestCount = newCount;
                            songMatch.bestOffset = offsetDelta;
                        }
                    }
                }
            }
        }

        auto phase1End = std::chrono::high_resolution_clock::now();
        auto phase1Ms = std::chrono::duration_cast<std::chrono::milliseconds>(phase1End - searchStart).count();

        std::cout << "Nombre de fingerprints matches: " << totalMatches << " / " << queryFingerprints.size()
                  << " (" << (100.0f * totalMatches / queryFingerprints.size()) << "%) [" << phase1Ms << "ms]" << std::endl;

        // Phase 2: Créer les résultats (le meilleur offset est déjà connu)
        std::vector<Match> candidateMatches;
        candidateMatches.reserve(matchCounts.size());  // Pré-allocation

        std::cout << "Chansons candidates: " << matchCounts.size() << std::endl;

        for (const auto& [songId, songMatch] : matchCounts) {
            if (songMatch.bestCount >= minScore) {
                auto songIt = songs.find(songId);
                if (songIt != songs.end()) {
                    std::cout << "  Chanson ID " << songId << " (" << songIt->second.title
                             << "): " << songMatch.bestCount << " matches a l'offset " << songMatch.bestOffset << std::endl;

                    Match match;
                    match.song = songIt->second;
                    match.score = songMatch.bestCount;
                    match.timeOffset = songMatch.bestOffset;

                    // Calcul de confiance amélioré
                    int songFpCount = songFingerprintCounts[songId];
                    int denominator = std::min(static_cast<int>(queryFingerprints.size()), songFpCount);

                    // Confiance basée sur le ratio de matches
                    match.confidence = (songMatch.bestCount * 100.0f) / denominator;

                    // Bonus si beaucoup de matches à un même offset (forte cohérence temporelle)
                    float coherenceBonus = 0.0f;
                    if (songMatch.bestCount > 20) {
                        coherenceBonus = std::min(10.0f, (songMatch.bestCount - 20) * 0.2f);
                    }
                    match.confidence += coherenceBonus;

                    // Cap at 100%
                    if (match.confidence > 100.0f) {
                        match.confidence = 100.0f;
                    }

                    candidateMatches.push_back(match);
                }
            }
        }

        // Trier par confiance décroissante
        std::sort(candidateMatches.begin(), candidateMatches.end(),
            [](const Match& a, const Match& b) {
                return a.confidence > b.confidence;
            });

        // Phase 3: Retourner les meilleurs résultats
        std::vector<Match> results;

        // Seuil de confiance abaissé à 70% pour mieux gérer les extraits
        const float CONFIDENCE_THRESHOLD = 70.0f;

        for (const auto& match : candidateMatches) {
            if (match.confidence >= CONFIDENCE_THRESHOLD) {
                results.push_back(match);
                std::cout << "Match trouvé: " << match.song.title
                         << " (Confiance: " << match.confidence << "%, Score: " << match.score
                         << ", Offset: " << match.timeOffset << " frames)" << std::endl;
            }
        }

        // Si aucun résultat avec seuil élevé, retourner les 3 meilleurs
        if (results.empty() && !candidateMatches.empty()) {
            std::cout << "Aucun match avec confiance >= " << CONFIDENCE_THRESHOLD
                     << "%. Retour des meilleurs candidats." << std::endl;
            for (size_t i = 0; i < std::min(size_t(3), candidateMatches.size()); ++i) {
                results.push_back(candidateMatches[i]);
            }
        }

        return results;
    }

    std::vector<Song> Database::getAllSongs() {
        std::vector<Song> allSongs;
        for (const auto& [id, song] : songs) {
            allSongs.push_back(song);
        }
        return allSongs;
    }

    void Database::clear() {
        songs.clear();
        fingerprintIndex.clear();
        songFingerprintCounts.clear();
        nextSongId = 1;
        saveToFile();
        std::cout << "Database cleared" << std::endl;
    }
}