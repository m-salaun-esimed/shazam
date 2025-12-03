#pragma once

#include <vector>
#include <complex>

namespace fft {

    struct Peak {
        int bin;           // Indice de fréquence (bin FFT)
        float frequency;   // Fréquence en Hz
        float magnitude;   // Magnitude du peak
    };

    std::vector<std::complex<float>> computeFFT(const std::vector<float>& input);

    std::vector<float> computeMagnitude(const std::vector<std::complex<float>>& fftResult);

    std::vector<Peak> extractPeaks(
        const std::vector<float>& magnitude,
        int sampleRate,
        float minMagnitude = 10.0f,
        int maxPeaks = 5
    );
}