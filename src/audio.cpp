//
// Created by matth on 03/12/2025.
//

#include "../include/audio.h"

#include <fstream>
#include <stdexcept>
#include <cstring>
#include <cmath>

namespace audio {
    static int16_t readLE16(const unsigned char* b) {
        return static_cast<int16_t>(b[0] | (b[1] << 8));
    }


    WavData loadWav(const std::string& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) throw std::runtime_error("Unable to open WAV: " + path);


        // Read RIFF header
        char riff[4]; ifs.read(riff, 4);
        if (std::strncmp(riff, "RIFF", 4) != 0) throw std::runtime_error("Not a RIFF file");
        ifs.seekg(4, std::ios::cur); // skip chunk size
        char wave[4]; ifs.read(wave, 4);
        if (std::strncmp(wave, "WAVE", 4) != 0) throw std::runtime_error("Not a WAVE file");


        int16_t audioFormat = 0;
        int16_t numChannels = 0;
        int32_t sampleRate = 0;
        int16_t bitsPerSample = 0;
        std::vector<int16_t> rawSamples;


        while (ifs) {
            char chunkId[4]; ifs.read(chunkId, 4);
            if (!ifs) break;
            uint32_t chunkSize = 0; ifs.read(reinterpret_cast<char*>(&chunkSize), 4);


            if (std::strncmp(chunkId, "fmt ", 4) == 0) {
                ifs.read(reinterpret_cast<char*>(&audioFormat), 2);
                ifs.read(reinterpret_cast<char*>(&numChannels), 2);
                ifs.read(reinterpret_cast<char*>(&sampleRate), 4);
                ifs.seekg(6, std::ios::cur); // byte rate + block align
                ifs.read(reinterpret_cast<char*>(&bitsPerSample), 2);
                if (chunkSize > 16) ifs.seekg(chunkSize - 16, std::ios::cur);
            } else if (std::strncmp(chunkId, "data", 4) == 0) {
                if (bitsPerSample != 16) throw std::runtime_error("Only 16-bit PCM supported");
                size_t numSamples = chunkSize / (bitsPerSample/8);
                rawSamples.resize(numSamples);
                ifs.read(reinterpret_cast<char*>(rawSamples.data()), chunkSize);
                break;
            } else {
                ifs.seekg(chunkSize, std::ios::cur);
            }
        }


        if (rawSamples.empty()) throw std::runtime_error("No audio data found");

        WavData out;
        out.sampleRate = sampleRate;
        out.numChannels = numChannels;

        if (numChannels == 1) {
            out.samples.reserve(rawSamples.size());
            for (int16_t s : rawSamples) {
                out.samples.push_back(s / 32768.0f);
            }
        } else {
            size_t numFrames = rawSamples.size() / numChannels;
            out.samples.reserve(numFrames);
            for (size_t i = 0; i < numFrames; ++i) {
                float sum = 0.0f;
                for (int ch = 0; ch < numChannels; ++ch) {
                    sum += rawSamples[i * numChannels + ch];
                }
                out.samples.push_back(sum / (numChannels * 32768.0f));
            }
        }
        return out;
    }

    std::vector<std::vector<float>> makeFrames(const std::vector<float>& samples, int frameSize, int hopSize) {
        std::vector<std::vector<float>> frames;

        for (size_t i = 0; i + frameSize <= samples.size(); i += hopSize) {
            std::vector<float> frame(samples.begin() + i, samples.begin() + i + frameSize);
            frames.push_back(frame);
        }

        return frames;
    }

    std::vector<float> hannWindow(int size) {
        std::vector<float> window(size);
        constexpr float PI = 3.14159265358979323846f;

        for (int i = 0; i < size; ++i) {
            window[i] = 0.5f * (1.0f - std::cos(2.0f * PI * i / (size - 1)));
        }

        return window;
    }
}