#include <QCS>
#include "VariationalExampleSupport.hpp"

int main()
{
    const Variational::QAOA qaoa(2, {{0, 1}});
    const std::vector<double> initialParameters = {0.8, 0.4};
    const Variational::OptimizationResult result = qaoa.minimize(initialParameters, Variational::CoordinateDescent(0.25, 80));
    printOptimizationResult("QAOA: maximum cut", initialParameters, result, "Negative cut cost");
    return 0;
}