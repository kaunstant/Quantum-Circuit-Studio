#include "circuit_canvas.hpp"
#include "qnn.hpp"

#include <QApplication>
#include <QString>

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

constexpr std::size_t featureCount = 11;
constexpr std::size_t qubitCount = 6;
constexpr std::size_t ansatzLayers = 3;

quantum::ml::qnn makeVineQualityModel()
{
    const std::size_t totalWeights = qubitCount * 2 * ansatzLayers;
    quantum::variational::ansatz ansatz(
        qubitCount,
        totalWeights,
        [](quantum::operators& operations, const std::vector<double>& weights) {
            for (std::size_t layer = 0; layer < ansatzLayers; ++layer) {
                const std::size_t layerOffset = layer * qubitCount * 2;
                for (std::size_t wire = 0; wire < qubitCount; ++wire) {
                    operations.ry(
                        static_cast<int>(wire),
                        weights.at(layerOffset + wire * 2));
                    operations.rz(
                        static_cast<int>(wire),
                        weights.at(layerOffset + wire * 2 + 1));
                }
                for (std::size_t wire = 0; wire + 1 < qubitCount; ++wire) {
                    operations.cx(
                        static_cast<int>(wire + 1),
                        static_cast<int>(wire));
                }
                operations.cx(0, static_cast<int>(qubitCount - 1));
            }
        });

    return quantum::ml::qnn(
        qubitCount,
        [](quantum::operators& operations, const std::vector<double>& features) {
            if (features.size() != featureCount) {
                throw std::invalid_argument(
                    "Vine quality model expects 11 input features");
            }
            for (std::size_t index = 0; index < features.size(); ++index) {
                const int wire = static_cast<int>(index % qubitCount);
                if (index < qubitCount) {
                    operations.rx(wire, features[index]);
                } else {
                    operations.ry(wire, features[index]);
                }
            }
        },
        ansatz);
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    try {
        auto model = makeVineQualityModel();
        const std::filesystem::path modelPath =
            std::filesystem::path("outputs") / "vine.qnn";
        if (!model.load(modelPath.string())) {
            throw std::runtime_error(
                "Unable to load the trained QNN model: " + modelPath.string());
        }

        const std::vector<double> sampleInput(featureCount, 0.0);
        auto snapshot = model.getAssembledCircuit(sampleInput);
        app::circuit_canvas canvas(
            snapshot.layout,
            snapshot.operations,
            QStringLiteral("Trained Vine Quality QNN"));
        canvas.setWindowTitle(QStringLiteral("Vine Quality QNN"));
        canvas.resize(1400, 700);
        canvas.draw();
        canvas.show();
        return application.exec();
    } catch (const std::exception& error) {
        std::cerr << "Unable to display trained vine model: "
                  << error.what() << '\n';
        return 1;
    }
}
