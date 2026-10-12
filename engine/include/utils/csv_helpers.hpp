#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace quantum::utils {

struct DatasetSplit {
    std::vector<std::vector<double>> trainRows;
    std::vector<std::vector<double>> testRows;
    std::filesystem::path trainPath;
    std::filesystem::path testPath;
};

namespace detail {

inline std::string readCSVHeader(const std::filesystem::path& sourcePath)
{
    std::ifstream file(sourcePath);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open CSV file: " + sourcePath.string());
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.find_first_not_of(" \t\r\n") != std::string::npos) {
            return line;
        }
    }
    throw std::runtime_error("CSV file has no header row: " + sourcePath.string());
}

inline std::filesystem::path preparedPath(
    const std::string& originalFilepath,
    const std::string& prefix)
{
    const std::filesystem::path originalPath(originalFilepath);
    if (originalPath.filename().empty()) {
        throw std::invalid_argument("CSV source path must include a file name");
    }
    return originalPath.parent_path() /
        (prefix + originalPath.filename().string());
}

inline void writeCSV(
    const std::vector<std::vector<double>>& rows,
    const std::filesystem::path& destination,
    const std::string& header)
{
    if (rows.empty()) {
        throw std::invalid_argument("Cannot write an empty CSV partition");
    }
    const std::size_t columnCount = rows.front().size();
    if (columnCount == 0) {
        throw std::invalid_argument("CSV rows must contain at least one column");
    }

    std::ofstream file(destination, std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to create prepared CSV file: " + destination.string());
    }
    if (!header.empty()) {
        file << header << '\n';
    }
    file << std::setprecision(std::numeric_limits<double>::max_digits10);
    for (const auto& row : rows) {
        if (row.size() != columnCount) {
            throw std::invalid_argument("CSV rows have inconsistent column counts");
        }
        for (std::size_t column = 0; column < row.size(); ++column) {
            if (!std::isfinite(row[column])) {
                throw std::invalid_argument("CSV values must be finite");
            }
            if (column != 0) {
                file << ',';
            }
            file << row[column];
        }
        file << '\n';
    }
    file.close();
    if (!file) {
        throw std::runtime_error("Failed while writing prepared CSV file: " + destination.string());
    }
}

} // namespace detail

// Writes a training partition to train-<source file name>.csv without changing the source
inline std::filesystem::path prepareTrainData(
    const std::vector<std::vector<double>>& rows,
    const std::string& originalFilepath,
    const std::string& header = {})
{
    const auto destination = detail::preparedPath(originalFilepath, "train-");
    detail::writeCSV(rows, destination, header);
    return destination;
}

// Writes a test partition to test-<source file name>.csv without changing the source
inline std::filesystem::path prepareTestData(
    const std::vector<std::vector<double>>& rows,
    const std::string& originalFilepath,
    const std::string& header = {})
{
    const auto destination = detail::preparedPath(originalFilepath, "test-");
    detail::writeCSV(rows, destination, header);
    return destination;
}

// Loads a numeric CSV file into a matrix, optionally skipping its first non-empty header row
inline std::vector<std::vector<double>> loadCSV(
    const std::string& filepath,
    bool hasHeader = false)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open CSV file: " + filepath);
    }

    const auto trim = [](const std::string& value) {
        const std::size_t first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return std::string{};
        }
        const std::size_t last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    };

    std::vector<std::vector<double>> rows;
    std::string line;
    std::size_t lineNumber = 0;
    bool skippedHeader = !hasHeader;
    while (std::getline(file, line)) {
        ++lineNumber;
        if (trim(line).empty()) {
            continue;
        }
        if (!skippedHeader) {
            skippedHeader = true;
            continue;
        }

        std::vector<double> row;
        std::size_t cellStart = 0;
        while (true) {
            const std::size_t comma = line.find(',', cellStart);
            const std::string cell = trim(line.substr(
                cellStart,
                comma == std::string::npos ? std::string::npos : comma - cellStart));
            if (cell.empty()) {
                throw std::runtime_error(
                    "CSV contains an empty value on line " + std::to_string(lineNumber));
            }

            std::size_t parsedCharacters = 0;
            double value = 0.0;
            try {
                value = std::stod(cell, &parsedCharacters);
            } catch (const std::exception&) {
                throw std::runtime_error(
                    "CSV contains a non-numeric value on line " +
                    std::to_string(lineNumber));
            }
            if (parsedCharacters != cell.size() || !std::isfinite(value)) {
                throw std::runtime_error(
                    "CSV contains an invalid numeric value on line " +
                    std::to_string(lineNumber));
            }
            row.push_back(value);

            if (comma == std::string::npos) {
                break;
            }
            cellStart = comma + 1;
        }

        if (!rows.empty() && row.size() != rows.front().size()) {
            throw std::runtime_error(
                "CSV rows have inconsistent column counts at line " +
                std::to_string(lineNumber));
        }
        rows.push_back(std::move(row));
    }
    if (!file.eof()) {
        throw std::runtime_error("Failed while reading CSV file: " + filepath);
    }
    return rows;
}

// Shuffles a dataset deterministically, writes both partitions beside the source, and returns them
inline DatasetSplit splitAndSaveDataset(
    const std::vector<std::vector<double>>& sourceRows,
    int testNumerator,
    int denominator,
    unsigned int seed,
    const std::string& originalFilepath,
    bool sourceHasHeader = false)
{
    if (sourceRows.size() < 2) {
        throw std::invalid_argument("At least two CSV rows are required to split a dataset");
    }
    if (denominator <= 1 || testNumerator <= 0 || testNumerator >= denominator) {
        throw std::invalid_argument("Test split fraction must satisfy 0 < numerator < denominator");
    }
    const std::size_t columnCount = sourceRows.front().size();
    if (columnCount == 0) {
        throw std::invalid_argument("Dataset rows must contain at least one column");
    }
    for (const auto& row : sourceRows) {
        if (row.size() != columnCount) {
            throw std::invalid_argument("Dataset rows have inconsistent column counts");
        }
        for (const double value : row) {
            if (!std::isfinite(value)) {
                throw std::invalid_argument("Dataset values must be finite");
            }
        }
    }

    std::vector<std::vector<double>> shuffledRows = sourceRows;
    std::mt19937 randomEngine(seed);
    std::shuffle(shuffledRows.begin(), shuffledRows.end(), randomEngine);

    const std::size_t divisor = static_cast<std::size_t>(denominator);
    const std::size_t numerator = static_cast<std::size_t>(testNumerator);
    const std::size_t testSize =
        (shuffledRows.size() / divisor) * numerator +
        ((shuffledRows.size() % divisor) * numerator) / divisor;
    if (testSize == 0 || testSize >= shuffledRows.size()) {
        throw std::invalid_argument("The selected split fraction produces an empty partition");
    }
    const std::size_t trainSize = shuffledRows.size() - testSize;

    DatasetSplit split;
    split.trainRows.assign(
        shuffledRows.begin(),
        shuffledRows.begin() + static_cast<std::ptrdiff_t>(trainSize));
    split.testRows.assign(
        shuffledRows.begin() + static_cast<std::ptrdiff_t>(trainSize),
        shuffledRows.end());

    const std::string header = sourceHasHeader
        ? detail::readCSVHeader(std::filesystem::path(originalFilepath))
        : std::string{};
    split.trainPath = prepareTrainData(split.trainRows, originalFilepath, header);
    split.testPath = prepareTestData(split.testRows, originalFilepath, header);
    return split;
}

} // namespace quantum::utils
