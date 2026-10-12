#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace quantum::ml {

struct RegressionScaler {
    std::vector<double> featureMinimum;
    std::vector<double> featureMaximum;
    std::size_t targetColumn = 0;
    double targetMinimum = 0.0;
    double targetMaximum = 0.0;
    double featureOutputMinimum = 0.0;
    double featureOutputMaximum = 3.14159265358979323846;
    double targetOutputMinimum = -1.0;
    double targetOutputMaximum = 1.0;
};

struct RegressionDataset {
    std::vector<std::vector<double>> rawFeatures;
    std::vector<std::vector<double>> features;
    std::vector<double> targets;
    std::vector<double> encodedTargets;
};

// Fits feature and target ranges using training rows only
inline RegressionScaler fitRegressionScaler(
    const std::vector<std::vector<double>>& rows,
    std::size_t targetColumn,
    double featureOutputMinimum = 0.0,
    double featureOutputMaximum = 3.14159265358979323846,
    double targetOutputMinimum = -1.0,
    double targetOutputMaximum = 1.0)
{
    if (rows.empty() || rows.front().empty()) {
        throw std::invalid_argument("Regression data must contain rows and columns");
    }
    if (targetColumn >= rows.front().size()) {
        throw std::invalid_argument("Regression target column is out of range");
    }
    if (!std::isfinite(featureOutputMinimum) ||
        !std::isfinite(featureOutputMaximum) ||
        featureOutputMinimum >= featureOutputMaximum ||
        !std::isfinite(targetOutputMinimum) ||
        !std::isfinite(targetOutputMaximum) ||
        targetOutputMinimum >= targetOutputMaximum) {
        throw std::invalid_argument("Regression scaling output ranges must be finite and increasing");
    }

    RegressionScaler scaler;
    scaler.targetColumn = targetColumn;
    scaler.featureOutputMinimum = featureOutputMinimum;
    scaler.featureOutputMaximum = featureOutputMaximum;
    scaler.targetOutputMinimum = targetOutputMinimum;
    scaler.targetOutputMaximum = targetOutputMaximum;

    const std::size_t columnCount = rows.front().size();
    scaler.featureMinimum.assign(columnCount - 1, std::numeric_limits<double>::infinity());
    scaler.featureMaximum.assign(columnCount - 1, -std::numeric_limits<double>::infinity());
    scaler.targetMinimum = std::numeric_limits<double>::infinity();
    scaler.targetMaximum = -std::numeric_limits<double>::infinity();

    for (const auto& row : rows) {
        if (row.size() != columnCount) {
            throw std::invalid_argument("Regression rows have inconsistent column counts");
        }
        for (std::size_t column = 0, feature = 0; column < columnCount; ++column) {
            if (!std::isfinite(row[column])) {
                throw std::invalid_argument("Regression values must be finite");
            }
            if (column == targetColumn) {
                scaler.targetMinimum = std::min(scaler.targetMinimum, row[column]);
                scaler.targetMaximum = std::max(scaler.targetMaximum, row[column]);
            } else {
                scaler.featureMinimum[feature] =
                    std::min(scaler.featureMinimum[feature], row[column]);
                scaler.featureMaximum[feature] =
                    std::max(scaler.featureMaximum[feature], row[column]);
                ++feature;
            }
        }
    }
    return scaler;
}

// Separates a target column and scales features and targets with a fitted scaler
inline RegressionDataset transformRegressionDataset(
    const std::vector<std::vector<double>>& rows,
    const RegressionScaler& scaler)
{
    if (rows.empty()) {
        throw std::invalid_argument("Regression data must contain at least one row");
    }
    const std::size_t expectedColumnCount = scaler.featureMinimum.size() + 1;
    if (scaler.targetColumn >= expectedColumnCount) {
        throw std::invalid_argument("Regression scaler has an invalid target column");
    }

    RegressionDataset transformed;
    transformed.rawFeatures.reserve(rows.size());
    transformed.features.reserve(rows.size());
    transformed.targets.reserve(rows.size());
    transformed.encodedTargets.reserve(rows.size());

    const double targetRange = scaler.targetMaximum - scaler.targetMinimum;
    for (const auto& row : rows) {
        if (row.size() != expectedColumnCount) {
            throw std::invalid_argument("Regression row does not match the fitted scaler");
        }

        std::vector<double> rawFeatures;
        std::vector<double> scaledFeatures;
        rawFeatures.reserve(expectedColumnCount - 1);
        scaledFeatures.reserve(expectedColumnCount - 1);
        for (std::size_t column = 0, feature = 0; column < row.size(); ++column) {
            if (!std::isfinite(row[column])) {
                throw std::invalid_argument("Regression values must be finite");
            }
            if (column == scaler.targetColumn) {
                continue;
            }
            const double range = scaler.featureMaximum[feature] - scaler.featureMinimum[feature];
            const double normalized = range == 0.0
                ? 0.0
                : (row[column] - scaler.featureMinimum[feature]) / range;
            rawFeatures.push_back(row[column]);
            scaledFeatures.push_back(
                scaler.featureOutputMinimum +
                normalized * (scaler.featureOutputMaximum - scaler.featureOutputMinimum));
            ++feature;
        }

        const double target = row[scaler.targetColumn];
        const double normalizedTarget = targetRange == 0.0
            ? 0.5
            : (target - scaler.targetMinimum) / targetRange;
        transformed.rawFeatures.push_back(std::move(rawFeatures));
        transformed.features.push_back(std::move(scaledFeatures));
        transformed.targets.push_back(target);
        transformed.encodedTargets.push_back(
            scaler.targetOutputMinimum +
            normalizedTarget * (scaler.targetOutputMaximum - scaler.targetOutputMinimum));
    }
    return transformed;
}

// Maps model outputs back to the original target scale
inline std::vector<double> inverseTransformTargets(
    const std::vector<double>& outputs,
    const RegressionScaler& scaler)
{
    std::vector<double> targets;
    targets.reserve(outputs.size());
    const double outputRange = scaler.targetOutputMaximum - scaler.targetOutputMinimum;
    const double targetRange = scaler.targetMaximum - scaler.targetMinimum;
    for (const double output : outputs) {
        if (!std::isfinite(output)) {
            throw std::invalid_argument("Regression model outputs must be finite");
        }
        const double boundedOutput = std::clamp(
            output,
            scaler.targetOutputMinimum,
            scaler.targetOutputMaximum);
        const double normalized =
            (boundedOutput - scaler.targetOutputMinimum) / outputRange;
        targets.push_back(scaler.targetMinimum + normalized * targetRange);
    }
    return targets;
}

} // namespace quantum::ml
