#pragma once

#include <string>
#include <vector>

/// A single mass spectrum: an array of (m/z, intensity) pairs
/// augmented with a name, a unique id, and a weight.
struct Spectrum {
    std::string name;
    std::string id;
    double weight = 1.0;

    std::vector<double> mz;
    std::vector<double> intensity;

    Spectrum() = default;

    Spectrum(std::string name, std::string id, double weight,
             std::vector<double> mz, std::vector<double> intensity)
        : name(std::move(name))
        , id(std::move(id))
        , weight(weight)
        , mz(std::move(mz))
        , intensity(std::move(intensity))
    {}

    std::size_t size() const { return mz.size(); }
};
