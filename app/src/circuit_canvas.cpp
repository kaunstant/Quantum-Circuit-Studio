#include "circuit_canvas.hpp"

#include <QFile>
#include <QFileDialog>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>
#include <QSaveFile>
#include <QTextDocument>
#include <QTextOption>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace app {
namespace {

constexpr qreal kLeft = 150.0;
constexpr qreal kTop = 150.0;
constexpr qreal kWireSpacing = 68.0;
constexpr qreal kStepSpacing = 82.0;
constexpr qreal kGateHeight = 42.0;
constexpr int kSnapshotVersion = 1;
constexpr int kMaximumWireCount = 256;
constexpr int kMaximumOperationCount = 100000;

template <typename OperationT>
QString operationLabel(const OperationT& operation)
{
    QString label = operation.name;
    if (!operation.parameters.isEmpty() &&
        operation.type == static_cast<int>(quantum::GateType::Parameterized)) {
        label += QStringLiteral("\n%1").arg(operation.parameters.front(), 0, 'g', 3);
    }
    return label;
}

bool readInteger(const QJsonValue& value, int& output)
{
    if (!value.isDouble()) {
        return false;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) ||
        number < static_cast<double>(std::numeric_limits<int>::min()) ||
        number > static_cast<double>(std::numeric_limits<int>::max()) ||
        std::floor(number) != number) {
        return false;
    }
    output = static_cast<int>(number);
    return true;
}

void reportError(QString* errorMessage, const QString& message, QWidget* parent)
{
    if (errorMessage) {
        *errorMessage = message;
    } else {
        QMessageBox::warning(parent, QStringLiteral("Circuit Canvas"), message);
    }
}

QGraphicsPathItem* addRoundedRect(
    QGraphicsScene* scene,
    const QRectF& rectangle,
    qreal xRadius,
    qreal yRadius,
    const QPen& pen,
    const QBrush& brush)
{
    QPainterPath path;
    path.addRoundedRect(rectangle, xRadius, yRadius);
    return scene->addPath(path, pen, brush);
}

template <typename OperationT>
QString registerOperationLabel(const OperationT& operation)
{
    if (operation.name == QStringLiteral("WALSH_HADAMARD")) {
        return QStringLiteral("Walsh\nHadamard");
    }
    if (operation.name == QStringLiteral("GROVER_DIFFUSION")) {
        const int iterations = operation.parameters.size() > 1
            ? static_cast<int>(operation.parameters[1])
            : 0;
        return QStringLiteral("Grover\nDiffuser\nIterations =\n%1")
            .arg(iterations, 2, 10, QLatin1Char('0'));
    }
    if (operation.name == QStringLiteral("CustomRegisterU") ||
        operation.name == QStringLiteral("ControlledCustomRegisterU")) {
        const QString dimension = operation.matrix.isEmpty()
            ? QString()
            : QStringLiteral("\n%1×%1").arg(operation.matrix.size());
        return (operation.controls.isEmpty()
                    ? QStringLiteral("Register\nUnitary")
                    : QStringLiteral("Controlled\nRegister Unitary")) + dimension;
    }
    return operation.name;
}

template <typename OperationT>
QString operationTooltip(const OperationT& operation)
{
    QString description = QStringLiteral("%1\nGate type: %2")
        .arg(operation.name)
        .arg(operation.type);
    if (!operation.controls.isEmpty()) {
        QStringList controls;
        for (int control : operation.controls) {
            controls.push_back(QStringLiteral("q%1").arg(control));
        }
        description += QStringLiteral("\nControls: %1").arg(controls.join(QStringLiteral(", ")));
    }
    if (!operation.parameters.isEmpty()) {
        QStringList parameters;
        for (double parameter : operation.parameters) {
            parameters.push_back(QString::number(parameter, 'g', 8));
        }
        description += QStringLiteral("\nParameters: %1").arg(parameters.join(QStringLiteral(", ")));
    }
    if (!operation.matrix.isEmpty()) {
        description += QStringLiteral("\nExplicit matrix: %1×%1")
            .arg(operation.matrix.size());
    }
    if (operation.registerId >= 0) {
        description += QStringLiteral("\nRegister: %1").arg(operation.registerId);
    }
    return description;
}

void addCenteredText(
    QGraphicsScene* scene,
    const QString& text,
    const QRectF& bounds,
    const QFont& font,
    const QColor& color)
{
    auto* label = scene->addText(text);
    label->setFont(font);
    label->setDefaultTextColor(color);
    QTextOption option = label->document()->defaultTextOption();
    option.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    label->document()->setDefaultTextOption(option);
    label->setTextWidth(bounds.width());
    const qreal labelHeight = label->document()->size().height();
    label->setPos(bounds.left(), bounds.center().y() - labelHeight / 2.0);
}

} // namespace

circuit_canvas::circuit_canvas(QWidget* parent)
    : QGraphicsView(parent),
      m_scene(new QGraphicsScene(this))
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setBackgroundBrush(QColor(250, 251, 253));
    setMinimumSize(640, 360);
}

circuit_canvas::circuit_canvas(quantum::circuit& circuit, QWidget* parent)
    : circuit_canvas(parent)
{
    m_circuit = &circuit;
}

circuit_canvas::circuit_canvas(
    quantum::circuit& circuit,
    const quantum::operators& operations,
    const QString& title,
    QWidget* parent)
    : circuit_canvas(circuit, parent)
{
    m_operations = &operations;
    m_snapshot.title = title;
}

void circuit_canvas::captureSource()
{
    if (!m_circuit) {
        return;
    }
    if (m_circuit->numQubits() > static_cast<std::size_t>(kMaximumWireCount)) {
        throw std::length_error("Circuit canvas supports at most 256 wires");
    }

    Snapshot snapshot;
    snapshot.title = m_snapshot.title;
    snapshot.wires.reserve(static_cast<int>(m_circuit->numQubits()));
    snapshot.registers.reserve(static_cast<int>(m_circuit->numRegisters()));

    for (std::size_t registerId = 0; registerId < m_circuit->numRegisters(); ++registerId) {
        const auto reg = (*m_circuit)(registerId);
        snapshot.registers.push_back({
            static_cast<int>(reg.startIndex()),
            static_cast<int>(reg.size())
        });
    }

    for (std::size_t wireIndex = 0; wireIndex < m_circuit->numQubits(); ++wireIndex) {
        const auto wire = (*m_circuit)[wireIndex];
        snapshot.wires.push_back({
            wire.inRegister() ? static_cast<int>(wire.registerId()) : -1,
            wire.inRegister() ? static_cast<int>(wire.localIdx()) : -1
        });
    }

    if (m_operations) {
        const auto& gates = m_operations->getGates();
        if (gates.size() > static_cast<std::size_t>(kMaximumOperationCount)) {
            throw std::length_error("Circuit canvas supports at most 100000 operations");
        }

        snapshot.operations.reserve(static_cast<int>(gates.size()));
        for (const quantum::gate& gate : gates) {
            Operation operation;
            operation.name = QString::fromStdString(gate.name);
            operation.type = static_cast<int>(gate.type);
            operation.registerId = gate.registerId;
            operation.classicalBit = gate.classicalBit;
            for (int target : gate.targets) {
                operation.targets.push_back(target);
            }
            for (int control : gate.controls) {
                operation.controls.push_back(control);
            }
            for (double parameter : gate.parameters) {
                operation.parameters.push_back(parameter);
            }
            for (const auto& row : gate.explicitMatrix) {
                QVector<QPair<double, double>> matrixRow;
                matrixRow.reserve(static_cast<int>(row.size()));
                for (const quantum::Complex& value : row) {
                    matrixRow.push_back(qMakePair(value.real(), value.imag()));
                }
                operation.matrix.push_back(std::move(matrixRow));
            }
            snapshot.operations.push_back(std::move(operation));
        }
    }

    m_snapshot = std::move(snapshot);
    m_hasSnapshot = true;
}

void circuit_canvas::draw()
{
    if (m_circuit) {
        captureSource();
    }
    if (!m_hasSnapshot) {
        m_snapshot = Snapshot{};
        m_hasSnapshot = true;
    }
    renderSnapshot();
}

void circuit_canvas::renderSnapshot()
{
    m_scene->clear();
    const int wireCount = m_snapshot.wires.size();
    const int operationCount = m_snapshot.operations.size();

    QFont gateFont(QStringLiteral("Segoe UI"), 9, QFont::DemiBold);
    QFont registerFont(QStringLiteral("Segoe UI"), 10, QFont::DemiBold);
    QFontMetricsF gateMetrics(gateFont);
    QFontMetricsF registerMetrics(registerFont);
    QVector<qreal> columnWidths;
    QVector<qreal> columnCenters;
    columnWidths.reserve(operationCount);
    columnCenters.reserve(operationCount);
    qreal nextColumnLeft = kLeft;
    for (const Operation& operation : m_snapshot.operations) {
        const bool registerOperation =
            operation.type == static_cast<int>(quantum::GateType::RegisterTransform) ||
            operation.type == static_cast<int>(quantum::GateType::CustomRegisterMatrix);
        qreal width = kStepSpacing;
        if (registerOperation) {
            const QStringList lines = registerOperationLabel(operation).split(QLatin1Char('\n'));
            qreal longestLine = 0.0;
            for (const QString& line : lines) {
                longestLine = std::max(longestLine, registerMetrics.horizontalAdvance(line));
            }
            width = std::max<qreal>(132.0, longestLine + 34.0);
        } else if (operation.type == static_cast<int>(quantum::GateType::CustomMatrix) ||
                   operation.type == static_cast<int>(quantum::GateType::Controlled) ||
                   operation.type == static_cast<int>(quantum::GateType::MultiControlled)) {
            const QString label = operation.name;
            width = std::max<qreal>(kStepSpacing, gateMetrics.horizontalAdvance(label) + 36.0);
        }
        columnWidths.push_back(width);
        columnCenters.push_back(nextColumnLeft + width / 2.0);
        nextColumnLeft += width;
    }

    const qreal sceneWidth = std::max<qreal>(800.0, nextColumnLeft + 40.0);
    const qreal sceneHeight = std::max<qreal>(360.0, kTop + std::max(1, wireCount) * kWireSpacing + 65.0);
    m_scene->setSceneRect(0.0, 0.0, sceneWidth, sceneHeight);

    const QString title = m_snapshot.title.isEmpty()
        ? QStringLiteral("Quantum Circuit")
        : m_snapshot.title;
    auto* titleText = m_scene->addText(title);
    titleText->setFont(QFont(QStringLiteral("Segoe UI"), 15, QFont::DemiBold));
    titleText->setDefaultTextColor(QColor(31, 41, 55));
    titleText->setPos(24.0, 16.0);

    auto* details = m_scene->addText(
        QStringLiteral("%1 wires  |  %2 registers  |  %3 operations")
            .arg(wireCount)
            .arg(m_snapshot.registers.size())
            .arg(operationCount));
    details->setFont(QFont(QStringLiteral("Segoe UI"), 9));
    details->setDefaultTextColor(QColor(100, 116, 139));
    details->setPos(26.0, 45.0);

    const QPen wirePen(QColor(75, 85, 99), 1.7);
    const QPen guidePen(QColor(226, 232, 240), 1.0, Qt::DashLine);

    for (int wireIndex = 0; wireIndex < wireCount; ++wireIndex) {
        const qreal y = kTop + wireIndex * kWireSpacing;
        const Wire& wire = m_snapshot.wires[wireIndex];
        QString label = QStringLiteral("q%1").arg(wireIndex);
        if (wire.registerId >= 0) {
            label += QStringLiteral("  r%1[%2]").arg(wire.registerId).arg(wire.localIndex);
        }
        auto* wireLabel = m_scene->addText(label);
        wireLabel->setFont(QFont(QStringLiteral("Segoe UI"), 9, QFont::DemiBold));
        wireLabel->setDefaultTextColor(QColor(51, 65, 85));
        wireLabel->setPos(20.0, y - 12.0);
        m_scene->addLine(kLeft, y, sceneWidth - 30.0, y, wirePen);
    }

    for (int step = 0; step < operationCount; ++step) {
        const qreal x = columnCenters[step];
        auto* stepLabel = m_scene->addText(QStringLiteral("%1").arg(step));
        stepLabel->setFont(QFont(QStringLiteral("Segoe UI"), 8));
        stepLabel->setDefaultTextColor(QColor(148, 163, 184));
        stepLabel->setPos(x - stepLabel->boundingRect().width() / 2.0, kTop - 48.0);
        if (wireCount > 0) {
            m_scene->addLine(
                x,
                kTop - 12.0,
                x,
                kTop + (wireCount - 1) * kWireSpacing + 12.0,
                guidePen);
        }
    }

    for (int step = 0; step < operationCount; ++step) {
        const Operation& operation = m_snapshot.operations[step];
        const qreal x = columnCenters[step];
        const qreal columnWidth = columnWidths[step];
        QVector<int> targets = operation.targets;
        const bool registerOperation =
            operation.type == static_cast<int>(quantum::GateType::RegisterTransform) ||
            operation.type == static_cast<int>(quantum::GateType::CustomRegisterMatrix);
        if (registerOperation && operation.registerId >= 0) {
            if (operation.registerId >= m_snapshot.registers.size()) {
                throw std::runtime_error("Circuit canvas operation references an unknown register");
            }
            const Register& reg = m_snapshot.registers[operation.registerId];
            if (targets.isEmpty()) {
                for (int local = 0; local < reg.size; ++local) {
                    targets.push_back(reg.startIndex + local);
                }
            }
        }
        if (operation.type == static_cast<int>(quantum::GateType::Barrier) && targets.isEmpty()) {
            for (int wire = 0; wire < wireCount; ++wire) {
                targets.push_back(wire);
            }
        }

        for (int wire : targets) {
            if (wire < 0 || wire >= wireCount) {
                throw std::runtime_error("Circuit canvas operation contains an invalid target wire");
            }
        }
        for (int wire : operation.controls) {
            if (wire < 0 || wire >= wireCount) {
                throw std::runtime_error("Circuit canvas operation contains an invalid control wire");
            }
        }

        if (operation.type == static_cast<int>(quantum::GateType::Barrier)) {
            if (!targets.isEmpty()) {
                const auto bounds = std::minmax_element(targets.begin(), targets.end());
                const qreal firstY = kTop + *bounds.first * kWireSpacing;
                const qreal lastY = kTop + *bounds.second * kWireSpacing;
                m_scene->addLine(
                    x,
                    firstY - 18.0,
                    x,
                    lastY + 18.0,
                    QPen(QColor(100, 116, 139), 2.0, Qt::DashLine));
            }
            continue;
        }

        QVector<int> affected = targets;
        for (int control : operation.controls) {
            if (!affected.contains(control)) {
                affected.push_back(control);
            }
        }

        if (!affected.isEmpty()) {
            const auto bounds = std::minmax_element(affected.begin(), affected.end());
            const qreal firstY = kTop + *bounds.first * kWireSpacing;
            const qreal lastY = kTop + *bounds.second * kWireSpacing;
            if (firstY != lastY || !operation.controls.isEmpty()) {
                m_scene->addLine(x, firstY, x, lastY, QPen(QColor(51, 65, 85), 1.7));
            }
        }

        for (int control : operation.controls) {
            const qreal y = kTop + control * kWireSpacing;
            m_scene->addEllipse(
                x - 5.5,
                y - 5.5,
                11.0,
                11.0,
                QPen(QColor(30, 41, 59), 1.0),
                QBrush(QColor(30, 41, 59)));
        }

        const bool swapGate = operation.name.compare(QStringLiteral("SWAP"), Qt::CaseInsensitive) == 0;
        const bool controlledNot =
            operation.name.compare(QStringLiteral("CX"), Qt::CaseInsensitive) == 0 ||
            operation.name.compare(QStringLiteral("CCX"), Qt::CaseInsensitive) == 0;
        if (swapGate && targets.size() >= 2) {
            for (int target : targets) {
                const qreal y = kTop + target * kWireSpacing;
                m_scene->addLine(x - 7.0, y - 7.0, x + 7.0, y + 7.0, QPen(QColor(30, 41, 59), 2.0));
                m_scene->addLine(x - 7.0, y + 7.0, x + 7.0, y - 7.0, QPen(QColor(30, 41, 59), 2.0));
            }
            continue;
        }

        if (registerOperation && !targets.isEmpty()) {
            const auto bounds = std::minmax_element(targets.begin(), targets.end());
            const qreal firstY = kTop + *bounds.first * kWireSpacing;
            const qreal lastY = kTop + *bounds.second * kWireSpacing;
            const qreal centerY = (firstY + lastY) / 2.0;
            const QString registerLabel = registerOperationLabel(operation);
            auto* registerShape = addRoundedRect(
                m_scene,
                QRectF(x - columnWidth / 2.0, firstY - 24.0, columnWidth, lastY - firstY + 48.0),
                5.0,
                5.0,
                QPen(QColor(91, 33, 182), 1.7),
                QBrush(QColor(245, 243, 255)));
            registerShape->setToolTip(operationTooltip(operation));
            addCenteredText(
                m_scene,
                registerLabel,
                QRectF(x - columnWidth / 2.0 + 8.0, firstY - 16.0, columnWidth - 16.0, lastY - firstY + 32.0),
                registerFont,
                QColor(76, 29, 149));
            continue;
        }

        const bool measurement =
            operation.type == static_cast<int>(quantum::GateType::Measurement);
        const bool sampling =
            operation.type == static_cast<int>(quantum::GateType::Sampling);
        for (int target : targets) {
            const qreal y = kTop + target * kWireSpacing;
            if (measurement || sampling) {
                QPainterPath meter;
                meter.moveTo(x - 13.0, y + 5.0);
                meter.cubicTo(x - 13.0, y - 13.0, x + 13.0, y - 13.0, x + 13.0, y + 5.0);
                m_scene->addPath(meter, QPen(QColor(3, 105, 161), 2.0));
                m_scene->addLine(x - 15.0, y + 6.0, x + 15.0, y + 6.0, QPen(QColor(3, 105, 161), 2.0));
                m_scene->addLine(x, y + 5.0, x + 7.0, y - 5.0, QPen(QColor(3, 105, 161), 2.0));
                m_scene->addEllipse(
                    x - 2.0,
                    y + 3.0,
                    4.0,
                    4.0,
                    QPen(QColor(3, 105, 161), 1.0),
                    QBrush(QColor(3, 105, 161)));
                if (sampling && !operation.parameters.isEmpty()) {
                    auto* shots = m_scene->addText(
                        QString::number(static_cast<int>(operation.parameters.front())));
                    shots->setFont(QFont(QStringLiteral("Segoe UI"), 7, QFont::DemiBold));
                    shots->setDefaultTextColor(QColor(3, 105, 161));
                    shots->setPos(x + 9.0, y - 22.0);
                }
                if (measurement && operation.classicalBit >= 0) {
                    addCenteredText(
                        m_scene,
                        QStringLiteral("c%1").arg(operation.classicalBit),
                        QRectF(x - 18.0, y + 8.0, 36.0, 15.0),
                        QFont(QStringLiteral("Segoe UI"), 7),
                        QColor(3, 105, 161));
                }
                continue;
            }

            if (controlledNot && !operation.controls.isEmpty()) {
                const qreal radius = 13.0;
                m_scene->addEllipse(
                    x - radius,
                    y - radius,
                    radius * 2.0,
                    radius * 2.0,
                    QPen(QColor(30, 41, 59), 1.7),
                    QBrush(Qt::white));
                m_scene->addLine(x - radius, y, x + radius, y, QPen(QColor(30, 41, 59), 1.7));
                m_scene->addLine(x, y - radius, x, y + radius, QPen(QColor(30, 41, 59), 1.7));
                continue;
            }

            const QColor border = QColor(37, 99, 235);
            const QColor fill = QColor(239, 246, 255);
            const qreal gateWidth = std::max<qreal>(
                58.0,
                std::min<qreal>(columnWidth - 12.0, gateMetrics.horizontalAdvance(operation.name) + 24.0));
            auto* gateShape = addRoundedRect(
                m_scene,
                QRectF(x - gateWidth / 2.0, y - kGateHeight / 2.0, gateWidth, kGateHeight),
                5.0,
                5.0,
                QPen(border, 1.7),
                QBrush(fill));
            gateShape->setToolTip(operationTooltip(operation));
            QString labelText = operationLabel(operation);
            if (operation.type == static_cast<int>(quantum::GateType::CustomMatrix) ||
                (operation.type == static_cast<int>(quantum::GateType::Controlled) &&
                 !operation.matrix.isEmpty()) ||
                (operation.type == static_cast<int>(quantum::GateType::MultiControlled) &&
                 !operation.matrix.isEmpty())) {
                const QString controlPrefix =
                    operation.type == static_cast<int>(quantum::GateType::MultiControlled)
                        ? QStringLiteral("MCU")
                        : operation.type == static_cast<int>(quantum::GateType::Controlled)
                            ? QStringLiteral("CU")
                            : QStringLiteral("U");
                labelText = controlPrefix;
                if (!operation.matrix.isEmpty()) {
                    labelText += QStringLiteral("\n%1×%1").arg(operation.matrix.size());
                }
            }
            addCenteredText(
                m_scene,
                labelText,
                QRectF(x - gateWidth / 2.0 + 3.0, y - kGateHeight / 2.0 + 2.0, gateWidth - 6.0, kGateHeight - 4.0),
                QFont(QStringLiteral("Segoe UI"), 8, QFont::DemiBold),
                QColor(30, 64, 175));
        }
    }
}

bool circuit_canvas::save(const QString& requestedPath, QString* errorMessage)
{
    QString filePath = requestedPath;
    if (filePath.isEmpty()) {
        filePath = QFileDialog::getSaveFileName(
            this,
            QStringLiteral("Save Circuit View"),
            QStringLiteral("circuit.qcsview.json"),
            QStringLiteral("Quantum Circuit View (*.qcsview.json);;JSON Files (*.json)"));
        if (filePath.isEmpty()) {
            return false;
        }
    }

    try {
        if (m_circuit) {
            captureSource();
        } else if (!m_hasSnapshot) {
            throw std::runtime_error("There is no circuit snapshot to save");
        }
    } catch (const std::exception& error) {
        reportError(errorMessage, QString::fromUtf8(error.what()), this);
        return false;
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), kSnapshotVersion);
    root.insert(QStringLiteral("title"), m_snapshot.title);

    QJsonArray wires;
    for (const Wire& wire : m_snapshot.wires) {
        QJsonObject item;
        item.insert(QStringLiteral("registerId"), wire.registerId);
        item.insert(QStringLiteral("localIndex"), wire.localIndex);
        wires.append(item);
    }
    root.insert(QStringLiteral("wires"), wires);

    QJsonArray registers;
    for (const Register& reg : m_snapshot.registers) {
        QJsonObject item;
        item.insert(QStringLiteral("startIndex"), reg.startIndex);
        item.insert(QStringLiteral("size"), reg.size);
        registers.append(item);
    }
    root.insert(QStringLiteral("registers"), registers);

    QJsonArray operations;
    for (const Operation& operation : m_snapshot.operations) {
        QJsonObject item;
        item.insert(QStringLiteral("name"), operation.name);
        item.insert(QStringLiteral("type"), operation.type);
        item.insert(QStringLiteral("registerId"), operation.registerId);
        item.insert(QStringLiteral("classicalBit"), operation.classicalBit);
        QJsonArray targets;
        for (int target : operation.targets) {
            targets.append(target);
        }
        item.insert(QStringLiteral("targets"), targets);
        QJsonArray controls;
        for (int control : operation.controls) {
            controls.append(control);
        }
        item.insert(QStringLiteral("controls"), controls);
        QJsonArray parameters;
        for (double parameter : operation.parameters) {
            parameters.append(parameter);
        }
        item.insert(QStringLiteral("parameters"), parameters);
        QJsonArray matrix;
        for (const auto& row : operation.matrix) {
            QJsonArray jsonRow;
            for (const auto& value : row) {
                QJsonArray complex;
                complex.append(value.first);
                complex.append(value.second);
                jsonRow.append(complex);
            }
            matrix.append(jsonRow);
        }
        item.insert(QStringLiteral("matrix"), matrix);
        operations.append(item);
    }
    root.insert(QStringLiteral("operations"), operations);

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        reportError(errorMessage, file.errorString(), this);
        return false;
    }
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size() || !file.commit()) {
        reportError(errorMessage, file.errorString(), this);
        return false;
    }
    return true;
}

bool circuit_canvas::load(const QString& filePath, QString* errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        reportError(errorMessage, file.errorString(), this);
        return false;
    }
    if (file.size() > 64 * 1024 * 1024) {
        reportError(errorMessage, QStringLiteral("Circuit view file exceeds the 64 MiB limit"), this);
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        reportError(
            errorMessage,
            QStringLiteral("Invalid circuit view JSON: %1").arg(parseError.errorString()),
            this);
        return false;
    }

    const QJsonObject root = document.object();
    int version = 0;
    if (!readInteger(root.value(QStringLiteral("version")), version) ||
        version != kSnapshotVersion ||
        !root.value(QStringLiteral("title")).isString() ||
        !root.value(QStringLiteral("wires")).isArray() ||
        !root.value(QStringLiteral("registers")).isArray() ||
        !root.value(QStringLiteral("operations")).isArray()) {
        reportError(errorMessage, QStringLiteral("Circuit view file has an unsupported or invalid structure"), this);
        return false;
    }

    Snapshot snapshot;
    snapshot.title = root.value(QStringLiteral("title")).toString();
    const QJsonArray jsonWires = root.value(QStringLiteral("wires")).toArray();
    const QJsonArray jsonRegisters = root.value(QStringLiteral("registers")).toArray();
    const QJsonArray jsonOperations = root.value(QStringLiteral("operations")).toArray();
    if (jsonWires.size() > kMaximumWireCount || jsonOperations.size() > kMaximumOperationCount) {
        reportError(errorMessage, QStringLiteral("Circuit view file exceeds supported dimensions"), this);
        return false;
    }

    for (const QJsonValue& value : jsonRegisters) {
        if (!value.isObject()) {
            reportError(errorMessage, QStringLiteral("Circuit view contains an invalid register"), this);
            return false;
        }
        const QJsonObject item = value.toObject();
        Register reg;
        if (!readInteger(item.value(QStringLiteral("startIndex")), reg.startIndex) ||
            !readInteger(item.value(QStringLiteral("size")), reg.size) ||
            reg.startIndex < 0 || reg.size <= 0 || reg.size > jsonWires.size() ||
            reg.startIndex > jsonWires.size() - reg.size) {
            reportError(errorMessage, QStringLiteral("Circuit view contains invalid register dimensions"), this);
            return false;
        }
        snapshot.registers.push_back(reg);
    }

    for (int wireIndex = 0; wireIndex < jsonWires.size(); ++wireIndex) {
        if (!jsonWires[wireIndex].isObject()) {
            reportError(errorMessage, QStringLiteral("Circuit view contains an invalid wire"), this);
            return false;
        }
        const QJsonObject item = jsonWires[wireIndex].toObject();
        Wire wire;
        if (!readInteger(item.value(QStringLiteral("registerId")), wire.registerId) ||
            !readInteger(item.value(QStringLiteral("localIndex")), wire.localIndex) ||
            wire.registerId < -1 || wire.localIndex < -1 ||
            (wire.registerId >= 0 &&
             (wire.registerId >= snapshot.registers.size() ||
              wire.localIndex >= snapshot.registers[wire.registerId].size))) {
            reportError(errorMessage, QStringLiteral("Circuit view contains invalid wire metadata"), this);
            return false;
        }
        snapshot.wires.push_back(wire);
    }

    QVector<int> registerOwners(snapshot.wires.size(), -1);
    for (int registerId = 0; registerId < snapshot.registers.size(); ++registerId) {
        const Register& reg = snapshot.registers[registerId];
        for (int localIndex = 0; localIndex < reg.size; ++localIndex) {
            const int wireIndex = reg.startIndex + localIndex;
            if (registerOwners[wireIndex] != -1) {
                reportError(errorMessage, QStringLiteral("Circuit view registers overlap"), this);
                return false;
            }
            registerOwners[wireIndex] = registerId;
        }
    }
    for (int wireIndex = 0; wireIndex < snapshot.wires.size(); ++wireIndex) {
        const Wire& wire = snapshot.wires[wireIndex];
        const int owner = registerOwners[wireIndex];
        if ((owner == -1 && (wire.registerId != -1 || wire.localIndex != -1)) ||
            (owner >= 0 &&
             (wire.registerId != owner ||
              wire.localIndex != wireIndex - snapshot.registers[owner].startIndex))) {
            reportError(errorMessage, QStringLiteral("Circuit view register metadata is inconsistent"), this);
            return false;
        }
    }

    for (const QJsonValue& value : jsonOperations) {
        if (!value.isObject()) {
            reportError(errorMessage, QStringLiteral("Circuit view contains an invalid operation"), this);
            return false;
        }
        const QJsonObject item = value.toObject();
        Operation operation;
        if (!item.value(QStringLiteral("name")).isString() ||
            !readInteger(item.value(QStringLiteral("type")), operation.type) ||
            !readInteger(item.value(QStringLiteral("registerId")), operation.registerId) ||
            !readInteger(item.value(QStringLiteral("classicalBit")), operation.classicalBit) ||
            !item.value(QStringLiteral("targets")).isArray() ||
            !item.value(QStringLiteral("controls")).isArray() ||
            !item.value(QStringLiteral("parameters")).isArray() ||
            !item.value(QStringLiteral("matrix")).isArray()) {
            reportError(errorMessage, QStringLiteral("Circuit view contains an invalid operation description"), this);
            return false;
        }
        operation.name = item.value(QStringLiteral("name")).toString();
        if (operation.name.isEmpty() ||
            operation.type < static_cast<int>(quantum::GateType::Standard) ||
            operation.type > static_cast<int>(quantum::GateType::RegisterTransform) ||
            operation.classicalBit < -1) {
            reportError(errorMessage, QStringLiteral("Circuit view contains invalid gate metadata"), this);
            return false;
        }
        for (const QJsonValue& target : item.value(QStringLiteral("targets")).toArray()) {
            int wire = -1;
            if (!readInteger(target, wire) || wire < 0 || wire >= jsonWires.size()) {
                reportError(errorMessage, QStringLiteral("Circuit view contains an invalid target wire"), this);
                return false;
            }
            operation.targets.push_back(wire);
        }
        for (const QJsonValue& control : item.value(QStringLiteral("controls")).toArray()) {
            int wire = -1;
            if (!readInteger(control, wire) || wire < 0 || wire >= jsonWires.size()) {
                reportError(errorMessage, QStringLiteral("Circuit view contains an invalid control wire"), this);
                return false;
            }
            operation.controls.push_back(wire);
        }
        for (const QJsonValue& parameter : item.value(QStringLiteral("parameters")).toArray()) {
            if (!parameter.isDouble() || !std::isfinite(parameter.toDouble())) {
                reportError(errorMessage, QStringLiteral("Circuit view contains an invalid gate parameter"), this);
                return false;
            }
            operation.parameters.push_back(parameter.toDouble());
        }
        const bool customMatrixOperation =
            operation.type == static_cast<int>(quantum::GateType::CustomMatrix) ||
            operation.type == static_cast<int>(quantum::GateType::Controlled) ||
            operation.type == static_cast<int>(quantum::GateType::MultiControlled) ||
            operation.type == static_cast<int>(quantum::GateType::CustomRegisterMatrix);
        if (customMatrixOperation && !operation.parameters.isEmpty()) {
            reportError(
                errorMessage,
                QStringLiteral("Custom matrix operations cannot contain a parameter vector"),
                this);
            return false;
        }
        for (const QJsonValue& rowValue : item.value(QStringLiteral("matrix")).toArray()) {
            if (!rowValue.isArray()) {
                reportError(errorMessage, QStringLiteral("Circuit view contains an invalid matrix row"), this);
                return false;
            }
            QVector<QPair<double, double>> row;
            for (const QJsonValue& complexValue : rowValue.toArray()) {
                if (!complexValue.isArray() || complexValue.toArray().size() != 2 ||
                    !complexValue.toArray()[0].isDouble() ||
                    !complexValue.toArray()[1].isDouble() ||
                    !std::isfinite(complexValue.toArray()[0].toDouble()) ||
                    !std::isfinite(complexValue.toArray()[1].toDouble())) {
                    reportError(errorMessage, QStringLiteral("Circuit view contains an invalid matrix value"), this);
                    return false;
                }
                const QJsonArray complex = complexValue.toArray();
                row.push_back(qMakePair(complex[0].toDouble(), complex[1].toDouble()));
            }
            operation.matrix.push_back(std::move(row));
        }
        if (operation.registerId < -1 ||
            (operation.registerId >= 0 && operation.registerId >= snapshot.registers.size())) {
            reportError(errorMessage, QStringLiteral("Circuit view operation references an unknown register"), this);
            return false;
        }
        snapshot.operations.push_back(std::move(operation));
    }

    m_snapshot = std::move(snapshot);
    m_hasSnapshot = true;
    m_circuit = nullptr;
    m_operations = nullptr;
    return true;
}

} // namespace app
