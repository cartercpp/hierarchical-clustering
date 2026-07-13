#include <iostream>
#include <format>
#include <vector>
#include <tuple>
#include <utility>
#include <random>
#include <ranges>
#include <algorithm>
#include <stop_token>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstddef>

int main()
{
    constexpr int rows = 80,
                  columns = 120,
                  pointCount = 100;

    std::random_device rd;
    std::uniform_int_distribution<int> rowDist(0, rows - 1),
                                       columnDist(0, columns - 1),
                                       rgbDist(0, 255);

    auto moveCursor = [](int row, int column)
    {
        std::cout << std::format("\033[{};{}H", row + 1, column + 1);
    };

    auto changeColor = [](int red, int green, int blue)
    {
        std::cout << std::format("\033[38;2;{};{};{}m", red, green, blue);
    };

    //                      row  col
    using point = std::pair<int, int>;

    std::cout << "\033[?25l";

    std::jthread thr{[&](std::stop_token st)
    {
        while (!st.stop_requested())
        {
            std::cout << "\033[2J";

            std::vector<std::vector<point>> clusters(pointCount, std::vector<point>(1));
            std::vector<std::tuple<int, int, int>> clusterColors(pointCount);

            for (int clusterIndex = 0; clusterIndex < pointCount; ++clusterIndex)
            {
                auto& clusterRef{clusters[clusterIndex]};
                auto& clusterColorRef{clusterColors[clusterIndex]};

                for (auto& [rowRef, columnRef] : clusterRef)
                {
                    rowRef = rowDist(rd);
                    columnRef = columnDist(rd);
                }

                std::get<0>(clusterColorRef) = rgbDist(rd);
                std::get<1>(clusterColorRef) = rgbDist(rd);
                std::get<2>(clusterColorRef) = rgbDist(rd);
            }

            while (true)
            {
                for (std::size_t clusterIndex = 0; clusterIndex < clusters.size(); ++clusterIndex)
                {
                    const auto& cluster{clusters[clusterIndex]};
                    const auto& clusterColor{clusterColors[clusterIndex]};

                    const int red = std::get<0>(clusterColor),
                              green = std::get<1>(clusterColor),
                              blue = std::get<2>(clusterColor);

                    changeColor(red, green, blue);

                    for (const auto [row, column] : cluster)
                    {
                        moveCursor(row, column);
                        std::cout << 'o';
                    }
                }

                if (clusters.size() == 1)
                    break;

                std::vector<double> averageClusterDistances(clusters.size() - 1, 0);
                for (std::size_t clusterIndex = 1; clusterIndex < clusters.size(); ++clusterIndex)
                {
                    std::vector<double> pointDistances;
                    pointDistances.reserve(clusters[0].size() * clusters[clusterIndex].size());

                    for (std::size_t i = 0; i < clusters[0].size(); ++i)
                    {
                        const auto [row1, column1] = clusters[0][i];

                        for (std::size_t i2 = 0; i2 < clusters[clusterIndex].size(); ++i2)
                        {
                            const auto [row2, column2] = clusters[clusterIndex][i2];
                            pointDistances.emplace_back(
                                std::sqrt((row1 - row2) * (row1 - row2) + (column1 - column2) * (column1 - column2))
                                );
                        }
                    }

                    averageClusterDistances[clusterIndex - 1]
                        = std::ranges::fold_left(pointDistances, 0, std::plus<double>{}) / pointDistances.size();
                }

                const std::size_t mergeIndex
                = std::min_element(averageClusterDistances.begin(), averageClusterDistances.end())
                    - averageClusterDistances.begin() + 1;

                clusters[0].insert(clusters[0].end(), clusters[mergeIndex].begin(), clusters[mergeIndex].end());
                clusters.erase(clusters.begin() + mergeIndex);
                clusterColors.erase(clusterColors.begin() + mergeIndex);

                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }};
    std::cin.get();

    std::cout << "\033[?25h";
    std::cout << "\033[0m";
}