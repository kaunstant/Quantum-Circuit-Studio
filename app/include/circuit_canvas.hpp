#pragma once

#include "circuit.hpp"
#include "operators.hpp"

#include <QGraphicsView>
#include <QString>
#include <QVector>

namespace app {

class circuit_canvas : public QGraphicsView {
public:
    explicit circuit_canvas(QWidget* parent = nullptr);
    explicit circuit_canvas(quantum::circuit& circuit, QWidget* parent = nullptr);
    circuit_canvas(
        quantum::circuit& circuit,
        const quantum::operators& operations,
        const QString& title = QString(),
        QWidget* parent = nullptr);

    void draw();
    bool save(const QString& filePath = QString(), QString* errorMessage = nullptr);
    bool load(const QString& filePath, QString* errorMessage = nullptr);

private:
    struct Wire {
        int registerId = -1;
        int localIndex = -1;
    };

    struct Register {
        int startIndex = 0;
        int size = 0;
    };

    struct Operation {
        QString name;
        int type = 0;
        QVector<int> targets;
        QVector<int> controls;
        QVector<double> parameters;
        QVector<QVector<QPair<double, double>>> matrix;
        int registerId = -1;
        int classicalBit = -1;
    };

    struct Snapshot {
        QString title;
        QVector<Wire> wires;
        QVector<Register> registers;
        QVector<Operation> operations;
    };

    quantum::circuit* m_circuit = nullptr;
    const quantum::operators* m_operations = nullptr;
    QGraphicsScene* m_scene = nullptr;
    Snapshot m_snapshot;
    bool m_hasSnapshot = false;

    void captureSource();
    void renderSnapshot();
};

} // namespace app
