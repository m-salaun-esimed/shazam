#include "../include/fft.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace fft {

    static void fftRecursive(std::vector<std::complex<float>>& buffer) {
        const size_t N = buffer.size();
        if (N <= 1) return;

        if ((N & (N - 1)) != 0) {
            throw std::runtime_error("FFT size must be a power of 2");
        }

        std::vector<std::complex<float>> even(N / 2);
        std::vector<std::complex<float>> odd(N / 2);

        for (size_t i = 0; i < N / 2; ++i) {
            even[i] = buffer[i * 2];
            odd[i] = buffer[i * 2 + 1];
        }

        fftRecursive(even);
        fftRecursive(odd);

        constexpr float PI = 3.14159265358979323846f;
        for (size_t k = 0; k < N / 2; ++k) {
            float angle = -2.0f * PI * k / N;
            std::complex<float> t = std::polar(1.0f, angle) * odd[k];
            buffer[k] = even[k] + t;
            buffer[k + N / 2] = even[k] - t;
        }
    }

    std::vector<std::complex<float>> computeFFT(const std::vector<float>& input) {
        std::vector<std::complex<float>> buffer(input.size());

        for (size_t i = 0; i < input.size(); ++i) {
            buffer[i] = std::complex<float>(input[i], 0.0f);
        }

        fftRecursive(buffer);
        return buffer;
    }

    std::vector<float> computeMagnitude(const std::vector<std::complex<float>>& fftResult) {
        std::vector<float> magnitude(fftResult.size() / 2); // On ne garde que la moitié (symétrie)

        for (size_t i = 0; i < magnitude.size(); ++i) {
            magnitude[i] = std::abs(fftResult[i]);
        }

        return magnitude;
    }

    std::vector<Peak> extractPeaks(
        const std::vector<float>& magnitude,
        int sampleRate,
        float minMagnitude,
        int maxPeaks
    ) {
        std::vector<Peak> peaks;

        for (size_t i = 1; i < magnitude.size() - 1; ++i) {
            if (magnitude[i] > magnitude[i - 1] &&
                magnitude[i] > magnitude[i + 1] &&
                magnitude[i] > minMagnitude) {

                Peak p;
                p.bin = static_cast<int>(i);
                p.magnitude = magnitude[i];
                p.frequency = i * sampleRate / (2.0f * magnitude.size());

                peaks.push_back(p);
            }
        }

        std::sort(peaks.begin(), peaks.end(),
            [](const Peak& a, const Peak& b) {
                return a.magnitude > b.magnitude;
            });

        if (peaks.size() > static_cast<size_t>(maxPeaks)) {
            peaks.resize(maxPeaks);
        }

        std::sort(peaks.begin(), peaks.end(),
            [](const Peak& a, const Peak& b) {
                return a.frequency < b.frequency;
            });

        return peaks;
    }
}