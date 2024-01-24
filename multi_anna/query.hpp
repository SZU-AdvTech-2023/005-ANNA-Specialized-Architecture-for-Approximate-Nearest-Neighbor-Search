#pragma once

#include <config.hpp>

class Query {
public:
    std::vector<uint64_t> clusters; // clusters index
    std::vector<uint64_t> centroids;

    Query(const std::vector<uint64_t>& clusters)
        : clusters(clusters)
    {
        for (auto index : clusters) {
            centroids.push_back(cluster_centroid[index]);
        }
    }

private:
};
