#pragma once

#include "spectrum.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

/// A pair of spectra displayed as a mirror plot.
///
/// The 'top' spectrum is rendered with positive intensities (upward).
/// The 'bottom' spectrum is rendered with negated intensities (downward).
/// Peaks whose m/z values coincide within a configurable tolerance are
/// connected by a vertical green line spanning from -bottomIntensity to
/// +topIntensity.
class SpectrumPair {
public:
    Spectrum top;    ///< displayed upward
    Spectrum bottom; ///< displayed downward

    SpectrumPair() = default;

    SpectrumPair(Spectrum top, Spectrum bottom)
        : top(std::move(top))
        , bottom(std::move(bottom))
    {}

    /// A peak that appears in both spectra within the given m/z tolerance.
    struct SharedPeak {
        double mz;
        double topIntensity;
        double bottomIntensity;
    };

    /// Return all peaks shared between top and bottom within @p tolerance Da.
    /// Uses a two-pointer scan over m/z-sorted indices – O(n log n).
    std::vector<SharedPeak> sharedPeaks(double tolerance = 0.01) const {
        // Build m/z-sorted index arrays without modifying the original data.
        std::vector<std::size_t> ti(top.size()), bi(bottom.size());
        std::iota(ti.begin(), ti.end(), 0);
        std::iota(bi.begin(), bi.end(), 0);
        std::sort(ti.begin(), ti.end(),
                  [&](std::size_t a, std::size_t b) { return top.mz[a] < top.mz[b]; });
        std::sort(bi.begin(), bi.end(),
                  [&](std::size_t a, std::size_t b) { return bottom.mz[a] < bottom.mz[b]; });

        std::vector<SharedPeak> result;
        std::size_t i = 0, j = 0;
        while (i < ti.size() && j < bi.size()) {
            double mzT = top.mz[ti[i]];
            double mzB = bottom.mz[bi[j]];
            double diff = mzT - mzB;
            if (std::abs(diff) <= tolerance) {
                result.push_back({mzT, top.intensity[ti[i]], bottom.intensity[bi[j]]});
                ++i;
            } else if (diff < 0) {
                ++i;
            } else {
                ++j;
            }
        }
        return result;
    }
};
