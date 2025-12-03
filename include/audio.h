#pragma once


#include <string>
#include <vector>

namespace audio {

    struct WavData {
        int sampleRate = 0;
        int numChannels = 0;
        std::vector<float> samples;
    };

    WavData loadWav(const std::string& path);

    std::vector<std::vector<float>> makeFrames(const std::vector<float>& samples, int frameSize, int hopSize);
    std::vector<float> hannWindow(int size);
}