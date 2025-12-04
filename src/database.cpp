#include "../include/database.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <algorithm>

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
                    StoredFingerprint fp;
                    char delimiter;

                    if (iss >> hash >> delimiter >> fp.songId >> delimiter >> fp.timeOffset) {
                        fingerprintIndex[hash].push_back(fp);
                        songFingerprintCounts[fp.songId]++; // Mettre à jour le cache
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
        for (const auto& [hash, fingerprints] : fingerprintIndex) {
            for (const auto& fp : fingerprints) {
                file << hash << "|" << fp.songId << "|" << fp.timeOffset << "\n";
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

        for (const auto& fp : fingerprints) {
            StoredFingerprint stored;
            stored.songId = songId;
            stored.timeOffset = fp.timeOffset;
            fingerprintIndex[fp.hash].push_back(stored);
        }

        // Mettre à jour le cache
        songFingerprintCounts[songId] = fingerprints.size();

        std::cout << "Added song ID " << songId << ": " << song.title
                  << " with " << fingerprints.size() << " fingerprints" << std::endl;

        saveToFile();
        return songId;
    }

    std::vector<Match> Database::search(const std::vector<fingerprint::Fingerprint>& queryFingerprints, int minScore) {
        std::cout << "\n=== DEBUT RECHERCHE ===" << std::endl;
        std::cout << "Nombre de fingerprints de la query: " << queryFingerprints.size() << std::endl;
        std::cout << "Nombre de hashs uniques dans la BDD: " << fingerprintIndex.size() << std::endl;

        // Map pour compter les matches: [songId][offsetDelta] -> count
        std::map<int, std::map<int, int>> matchCounts;
        int totalMatches = 0;

        // Phase 1: Compter les matches (time-invariant)
        for (const auto& queryFp : queryFingerprints) {
            auto it = fingerprintIndex.find(queryFp.hash);
            if (it != fingerprintIndex.end()) {
                totalMatches++;
                for (const auto& storedFp : it->second) {
                    // Calcul de l'offset delta (différence de position)
                    // Cela permet de trouver la chanson même si l'extrait vient du milieu
                    int offsetDelta = storedFp.timeOffset - queryFp.timeOffset;
                    matchCounts[storedFp.songId][offsetDelta]++;
                }
            }
        }

        std::cout << "Nombre de fingerprints matches: " << totalMatches << " / " << queryFingerprints.size()
                  << " (" << (100.0f * totalMatches / queryFingerprints.size()) << "%)" << std::endl;

        // Phase 2: Trouver le meilleur offset pour chaque chanson
        std::vector<Match> candidateMatches;

        std::cout << "Chansons candidates: " << matchCounts.size() << std::endl;

        for (const auto& [songId, offsetMap] : matchCounts) {
            // Trouver l'offset avec le plus de matches
            int bestOffset = 0;
            int bestCount = 0;

            for (const auto& [offset, count] : offsetMap) {
                if (count > bestCount) {
                    bestCount = count;
                    bestOffset = offset;
                }
            }

            auto songIt = songs.find(songId);
            if (songIt != songs.end()) {
                std::cout << "  Chanson ID " << songId << " (" << songIt->second.title
                         << "): " << bestCount << " matches a l'offset " << bestOffset << std::endl;
            }

            if (bestCount >= minScore) {
                if (songIt != songs.end()) {
                    Match match;
                    match.song = songIt->second;
                    match.score = bestCount;
                    match.timeOffset = bestOffset;

                    // Calcul de confiance amélioré
                    int songFpCount = songFingerprintCounts[songId];
                    int denominator = std::min(static_cast<int>(queryFingerprints.size()), songFpCount);

                    // Confiance basée sur le ratio de matches
                    match.confidence = (bestCount * 100.0f) / denominator;

                    // Bonus si beaucoup de matches à un même offset (forte cohérence temporelle)
                    float coherenceBonus = 0.0f;
                    if (bestCount > 20) {
                        coherenceBonus = std::min(10.0f, (bestCount - 20) * 0.2f);
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