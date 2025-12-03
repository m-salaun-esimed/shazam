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
                    uint32_t hash;
                    StoredFingerprint fp;
                    char delimiter;

                    if (iss >> hash >> delimiter >> fp.songId >> delimiter >> fp.timeOffset) {
                        fingerprintIndex[hash].push_back(fp);
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

        std::cout << "Added song ID " << songId << ": " << song.title
                  << " with " << fingerprints.size() << " fingerprints" << std::endl;

        saveToFile();
        return songId;
    }

    std::vector<Match> Database::search(const std::vector<fingerprint::Fingerprint>& queryFingerprints, int minScore) {
        std::map<std::pair<int, int>, int> matchCounts;

        for (const auto& queryFp : queryFingerprints) {
            auto it = fingerprintIndex.find(queryFp.hash);
            if (it != fingerprintIndex.end()) {
                for (const auto& storedFp : it->second) {
                    int offsetDelta = storedFp.timeOffset - queryFp.timeOffset;
                    matchCounts[{storedFp.songId, offsetDelta}]++;
                }
            }
        }

        std::vector<Match> results;
        for (const auto& [key, count] : matchCounts) {
            if (count >= minScore) {
                auto songIt = songs.find(key.first);
                if (songIt != songs.end()) {
                    Match match;
                    match.song = songIt->second;
                    match.score = count;
                    match.timeOffset = key.second;

                    // Count total fingerprints for this song
                    int songFingerprintCount = 0;
                    for (const auto& [hash, fps] : fingerprintIndex) {
                        for (const auto& fp : fps) {
                            if (fp.songId == key.first) {
                                songFingerprintCount++;
                            }
                        }
                    }

                    // Use the smaller of the two sets to ensure confidence never exceeds 100%
                    int denominator = std::min(static_cast<int>(queryFingerprints.size()), songFingerprintCount);
                    match.confidence = (count * 100.0f) / denominator;

                    // Cap at 100% just in case
                    if (match.confidence > 100.0f) {
                        match.confidence = 100.0f;
                    }

                    results.push_back(match);
                }
            }
        }

        std::sort(results.begin(), results.end(),
            [](const Match& a, const Match& b) {
                return a.score > b.score;
            });

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
        nextSongId = 1;
        saveToFile();
        std::cout << "Database cleared" << std::endl;
    }
}