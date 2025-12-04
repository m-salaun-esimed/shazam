#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "fingerprint.h"

namespace database {

    // Métadonnées d'une chanson
    struct Song {
        int id = -1;
        std::string title;
        std::string artist;
        float duration = 0.0f;
    };

    // Résultat d'une recherche
    struct Match {
        Song song;
        int score;              // Nombre de fingerprints matchés
        int timeOffset;         // Offset temporel le plus probable
        float confidence;       // Confiance du match (0-100%)
    };

    // Stockage d'un fingerprint avec sa chanson et son temps
    struct StoredFingerprint {
        int songId;
        int timeOffset;
    };

    // Classe de gestion de la base de données locale
    class Database {
    private:
        std::string dbPath;
        int nextSongId = 1;

        // Stockage en mémoire
        std::map<int, Song> songs;  // songId -> Song
        std::unordered_map<uint32_t, std::vector<StoredFingerprint>> fingerprintIndex;  // hash -> fingerprints
        std::map<int, int> songFingerprintCounts;  // songId -> nombre de fingerprints (CACHE)

        void loadFromFile();
        void saveToFile();

    public:
        Database(const std::string& path);
        ~Database();

        // Empêcher la copie
        Database(const Database&) = delete;
        Database& operator=(const Database&) = delete;

        // Ajouter une chanson et ses fingerprints
        int addSong(const Song& song, const std::vector<fingerprint::Fingerprint>& fingerprints);

        // Rechercher une chanson à partir de fingerprints
        std::vector<Match> search(const std::vector<fingerprint::Fingerprint>& queryFingerprints, int minScore = 5);

        // Utilitaires
        std::vector<Song> getAllSongs();
        void clear();  // Vider la base de données
    };
}