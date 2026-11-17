#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    constexpr double alpha = 2.0 / 3.0;
    std::cout << "n,depth_limit,predicted_rate,expected_rebuilds\n";
    std::cout << std::setprecision(12);
    for (std::size_t n : {1000, 2000, 4000}) {
        const auto limit = static_cast<std::size_t>(
            std::floor(std::log(static_cast<double>(n)) / std::log(1.0 / alpha)));

        // Keep depths 0 through limit and one bucket for all deeper leaves.
        std::vector<double> probability(limit + 2, 0.0);
        probability[0] = 1.0;
        for (std::size_t k = 1; k < n; ++k) {
            const double increase = 2.0 / (k + 1);
            probability.back() += probability[limit] * increase;
            for (std::size_t depth = limit; depth > 0; --depth) {
                probability[depth] = probability[depth] * (1.0 - increase)
                    + probability[depth - 1] * increase;
            }
            probability[0] *= 1.0 - increase;
        }
        std::cout << n << ',' << limit << ',' << probability.back() << ','
                  << probability.back() * n * n << '\n';
    }
}
