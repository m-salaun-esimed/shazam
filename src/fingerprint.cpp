#include "../include/fingerprint.h"
#include <algorithm>

namespace fingerprint {

    uint64_t generateHash(int freq1, int freq2, int deltaTime) {

        uint32_t f1 = static_cast<uint32_t>(freq1 / 10) * 10;
        uint32_t f2 = static_cast<uint32_t>(freq2 / 10) * 10;
        uint32_t dt = static_cast<uint32_t>(deltaTime);

        uint64_t hash = 0;
        hash |= (static_cast<uint64_t>(f1 & 0xFFFFF) << 44);
        hash |= (static_cast<uint64_t>(f2 & 0xFFFFF) << 24);
        hash |= (static_cast<uint64_t>(dt & 0x3FF) << 14);

        return hash;
    }

    std::vector<Fingerprint> generateFingerprints(
        const std::vector<std::vector<fft::Peak>>& peaksByFrame,
        int targetZone,
        int fanout
    ) {
        std::vector<Fingerprint> fingerprints;

        for (size_t anchorFrame = 0; anchorFrame < peaksByFrame.size(); ++anchorFrame) {
            const auto& anchorPeaks = peaksByFrame[anchorFrame];

            for (const auto& anchorPeak : anchorPeaks) {
                int pairsCreated = 0;

                for (int dt = 1; dt <= targetZone && (anchorFrame + dt) < peaksByFrame.size(); ++dt) {
                    const auto& targetPeaks = peaksByFrame[anchorFrame + dt];

                    for (const auto& targetPeak : targetPeaks) {
                        Fingerprint fp;
                        fp.hash = generateHash(
                            static_cast<int>(anchorPeak.frequency),
                            static_cast<int>(targetPeak.frequency),
                            dt
                        );
                        fp.timeOffset = static_cast<int>(anchorFrame);

                        fingerprints.push_back(fp);

                        pairsCreated++;
                        if (pairsCreated >= fanout) {
                            break;
                        }
                    }

                    if (pairsCreated >= fanout) {
                        break;
                    }
                }
            }
        }

        return fingerprints;
    }
}