#pragma once

#include <vector>
#include <cstdint>
#include "fft.h"

namespace fingerprint {

    // Constantes pour l'optimisation des queries
    constexpr int QUERY_MAX_DURATION_SECONDS = 5;  // Durée max pour recherche
    constexpr int SAMPLE_RATE = 44100;
    constexpr int HOP_SIZE = 2048;
    constexpr int QUERY_MAX_FRAMES = (QUERY_MAX_DURATION_SECONDS * SAMPLE_RATE) / HOP_SIZE;

    // Structure représentant un fingerprint (empreinte)
    struct Fingerprint {
        uint64_t hash;      // Hash unique généré à partir des peaks
        int timeOffset;     // Position temporelle dans la chanson (en frames)
    };

    // Génère un hash à partir de deux peaks et leur différence de temps
    // Utilise la méthode Shazam : combine freq1, freq2, deltaTime
    uint64_t generateHash(int freq1, int freq2, int deltaTime);

    // Génère tous les fingerprints d'une liste de peaks par frame
    // peaksByFrame[i] = liste des peaks de la frame i
    // targetZone: nombre de frames suivantes à considérer pour les paires
    // fanout: nombre maximum de peaks à apparier par frame
    std::vector<Fingerprint> generateFingerprints(
        const std::vector<std::vector<fft::Peak>>& peaksByFrame,
        int targetZone = 5,
        int fanout = 3
    );

    // Version optimisée pour la recherche : limite aux N premières secondes
    // Beaucoup plus rapide car traite moins de frames
    std::vector<Fingerprint> generateFingerprintsForQuery(
        const std::vector<std::vector<fft::Peak>>& peaksByFrame,
        int targetZone = 5,
        int fanout = 3,
        int maxFrames = QUERY_MAX_FRAMES
    );
}