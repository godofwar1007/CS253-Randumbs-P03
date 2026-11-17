#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using Count = std::uint64_t;
using Row = std::vector<std::string>;

const std::array<std::string, 4> strategies{
    "hibbard_succ", "hibbard_rand", "scapegoat_array", "scapegoat_inplace",
};
const std::array<std::string, 4> colors{"#a34425", "#7a428f", "#156692", "#167458"};

std::vector<Row> readCsv(const fs::path& file, const std::string& header, std::size_t fields) {
    std::ifstream input(file);
    std::string line;
    auto readLine = [&] {
        if (!std::getline(input, line)) return false;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        return true;
    };
    if (!readLine() || line != header) throw std::runtime_error("Invalid CSV header: " + file.string());
    std::vector<Row> rows;
    while (readLine()) {
        std::istringstream stream(line);
        Row row;
        for (std::string field; std::getline(stream, field, ',');) row.push_back(field);
        if (row.size() != fields) throw std::runtime_error("Invalid CSV row: " + file.string());
        rows.push_back(std::move(row));
    }
    if (input.bad() || rows.empty()) throw std::runtime_error("Unreadable or empty CSV: " + file.string());
    return rows;
}

Count count(const std::string& value) {
    if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos) {
        throw std::runtime_error("Invalid integer: " + value);
    }
    return std::stoull(value);
}

double number(const std::string& value) {
    std::size_t end = 0;
    const double result = std::stod(value, &end);
    if (end != value.size() || !std::isfinite(result) || result < 0) {
        throw std::runtime_error("Invalid measurement: " + value);
    }
    return result;
}

struct Run {
    std::vector<Count> pairs, rebuilds;
    std::vector<double> depths;
    double seconds;
};

using Runs = std::map<Count, Run>;
using Groups = std::map<Count, std::map<std::string, Runs>>;

Groups loadRuns(const fs::path& raw, std::string& schedule) {
    Groups groups;
    for (const auto& row : readCsv(raw / "runs.csv", "strategy,n,seed,schedule,pairs,seconds,samples", 7)) {
        const Count n = count(row[1]), seed = count(row[2]), budget = count(row[4]);
        const auto filename = fs::u8path(row[6]);
        if (schedule.empty()) schedule = row[3];
        if (row[3].empty() || row[3] != schedule || n == 0 || budget == 0 ||
            filename.filename() != filename || filename == ".." ||
            std::find(strategies.begin(), strategies.end(), row[0]) == strategies.end()) {
            throw std::runtime_error("Invalid run metadata: " + row[6]);
        }
        Run run{{}, {}, {}, number(row[5])};
        for (const auto& sample : readCsv(raw / filename,
                "strategy,n,seed,pair_index,avg_depth,rebuild_count", 6)) {
            const Count pair = count(sample[3]), rebuilds = count(sample[5]);
            if (sample[0] != row[0] || count(sample[1]) != n || count(sample[2]) != seed ||
                (!run.pairs.empty() && (pair <= run.pairs.back() || rebuilds < run.rebuilds.back()))) {
                throw std::runtime_error("Invalid sample metadata or order: " + row[6]);
            }
            run.pairs.push_back(pair);
            run.depths.push_back(number(sample[4]));
            run.rebuilds.push_back(rebuilds);
        }
        if (run.pairs.front() != 0 || run.pairs.back() != budget ||
            !groups[n][row[0]].emplace(seed, std::move(run)).second) {
            throw std::runtime_error("Invalid sample endpoints or duplicate seed: " + row[6]);
        }
    }
    return groups;
}

struct Statistics {
    double mean, sd;
};

Statistics stats(const std::vector<double>& values) {
    if (values.size() < 5) throw std::runtime_error("Expected at least five runs per condition");
    const double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double squares = 0;
    for (double value : values) squares += (value - mean) * (value - mean);
    return {mean, std::sqrt(squares / (values.size() - 1))};
}

std::string label(double value) {
    std::ostringstream text;
    text << std::setprecision(3) << value;
    return text.str();
}

std::string escapeXml(const std::string& value) {
    std::string escaped;
    for (char c : value) {
        if (c == '&') escaped += "&amp;";
        else if (c == '<') escaped += "&lt;";
        else if (c == '>') escaped += "&gt;";
        else escaped += c;
    }
    return escaped;
}

class Chart {
public:
    Chart(const fs::path& file, double low, double high, double top,
          const std::string& title, const std::string& xLabel, const std::string& yLabel,
          const std::vector<double>& ticks)
        : svg(file), low(low), high(high), top(top) {
        svg.exceptions(std::ios::badbit | std::ios::failbit);
        svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"930\" height=\"515\" "
               "viewBox=\"0 0 930 515\" role=\"img\"><title>" << escapeXml(title)
            << "</title><rect width=\"930\" height=\"515\" fill=\"white\"/>"
               "<g font-family=\"sans-serif\" font-size=\"12\" fill=\"#202b37\">\n";
        text(80, 28, title, "start", 20);
        for (int i = 0; i <= 5; ++i) {
            const double tick = top * i / 5;
            line(80, y(tick), 710, y(tick), "#dbe1e7");
            text(70, y(tick) + 4, label(tick), "end");
        }
        for (double tick : ticks) text(x(tick), 403, label(tick), "middle");
        line(80, 380, 710, 380, "#526170");
        line(80, 55, 80, 380, "#526170");
        text(395, 429, xLabel, "middle");
        svg << "<g transform=\"translate(19 218) rotate(-90)\">";
        text(0, 0, yLabel, "middle");
        svg << "</g>\n";
    }

    void series(const std::vector<double>& xs, const std::vector<Statistics>& values,
                const std::string& name, const std::string& color, bool dashed = false,
                bool band = false, bool markers = false) {
        if (band) {
            svg << "<polygon fill=\"" << color << "\" opacity=\"0.15\" points=\"";
            for (std::size_t i = 0; i < xs.size(); ++i) point(xs[i], values[i].mean + values[i].sd);
            for (std::size_t i = xs.size(); i-- > 0;) point(xs[i], values[i].mean - values[i].sd);
            svg << "\"/>\n";
        }
        svg << "<polyline fill=\"none\" stroke=\"" << color << "\" stroke-width=\"2\"";
        if (dashed) svg << " stroke-dasharray=\"6 4\"";
        svg << " points=\"";
        for (std::size_t i = 0; i < xs.size(); ++i) point(xs[i], values[i].mean);
        svg << "\"/>\n";
        if (markers) {
            for (std::size_t i = 0; i < xs.size(); ++i) {
                const double px = x(xs[i]), upper = y(values[i].mean + values[i].sd);
                const double lower = y(values[i].mean - values[i].sd);
                line(px, upper, px, lower, color);
                line(px - 4, upper, px + 4, upper, color);
                line(px - 4, lower, px + 4, lower, color);
                svg << "<circle cx=\"" << px << "\" cy=\"" << y(values[i].mean)
                    << "\" r=\"3\" fill=\"" << color << "\"/>\n";
            }
        }
        line(731, legendY - 4, 753, legendY - 4, color, dashed);
        text(760, legendY, name, "start");
        legendY += 22;
    }

    void finish(const std::string& caption, const fs::path& raw,
                const fs::path& comparison = {}, bool baseline = false) {
        text(80, 458, caption, "start");
        text(80, 477, "Data: " + (raw / "runs.csv").generic_u8string() + " and listed sample CSVs", "start", 10);
        const auto source = comparison.empty() ? "" : (baseline ? "Baseline: " : "Model: ")
                            + comparison.generic_u8string() + " | ";
        text(80, 494, source + "Generator: results/d6_figures.cpp", "start", 10);
        svg << "</g></svg>\n";
        svg.close();
    }

private:
    std::ofstream svg;
    double low, high, top;
    int legendY = 72;

    double x(double value) const { return 80 + 630 * (value - low) / (high - low); }
    double y(double value) const { return 380 - 325 * std::max(0.0, value) / top; }
    void point(double px, double py) { svg << x(px) << ',' << y(py) << ' '; }

    void text(double px, double py, const std::string& value, const char* anchor, int size = 12) {
        svg << "<text x=\"" << px << "\" y=\"" << py << "\" text-anchor=\"" << anchor
            << "\" font-size=\"" << size << "\">" << escapeXml(value) << "</text>\n";
    }

    void line(double x1, double y1, double x2, double y2, const std::string& color, bool dashed = false) {
        svg << "<line x1=\"" << x1 << "\" y1=\"" << y1 << "\" x2=\"" << x2
            << "\" y2=\"" << y2 << "\" stroke=\"" << color << '"';
        if (dashed) svg << " stroke-dasharray=\"6 4\"";
        svg << "/>\n";
    }
};

bool matched(const Run& a, const Run& b) {
    return a.pairs == b.pairs && a.depths == b.depths && a.rebuilds == b.rebuilds;
}

void plotDepth(const Groups& groups, const fs::path& raw, const fs::path& out,
               const std::string& schedule, std::ostream& timing) {
    for (const auto& [n, byStrategy] : groups) {
        const auto& pairs = byStrategy.at(strategies.front()).begin()->second.pairs;
        const std::vector<double> xs(pairs.begin(), pairs.end());
        std::array<std::vector<Statistics>, 4> depths;
        double top = std::sqrt(static_cast<double>(n));
        for (std::size_t i = 0; i < strategies.size(); ++i) {
            const auto& samples = byStrategy.at(strategies[i]);
            std::vector<double> seconds;
            for (const auto& [seed, run] : samples) {
                if (run.pairs != pairs) throw std::runtime_error("Inconsistent sample times");
                seconds.push_back(run.seconds);
            }
            const auto elapsed = stats(seconds);
            timing << n << ',' << strategies[i] << ',' << samples.size() << ','
                   << elapsed.mean << ',' << elapsed.sd << '\n';
            for (std::size_t k = 0; k < pairs.size(); ++k) {
                std::vector<double> values;
                for (const auto& record : samples) values.push_back(record.second.depths[k]);
                const auto depth = stats(values);
                depths[i].push_back(depth);
                top = std::max(top, depth.mean + depth.sd);
            }
        }
        std::vector<double> ticks;
        for (int i = 0; i <= 5; ++i) ticks.push_back(xs.back() * i / 5);
        Chart chart(out / ("depth_" + std::to_string(n) + ".svg"), 0, xs.back(), top * 1.08,
                    "Average depth during " + schedule + " churn (n = " + std::to_string(n) + ")",
                    "Churn pairs", "Average node depth (edges)", ticks);
        const std::vector<double> endpoints{0, xs.back()};
        const double ln = std::log(static_cast<double>(n)), root = std::sqrt(static_cast<double>(n));
        chart.series(endpoints, {{ln, 0}, {ln, 0}}, "ln(n)", "#596573", true);
        chart.series(endpoints, {{root, 0}, {root, 0}}, "sqrt(n)", "#85909b", true);
        for (std::size_t i = 0; i < strategies.size(); ++i) {
            chart.series(xs, depths[i], strategies[i], colors[i], i == 3, true);
        }
        chart.finish("Compare depth over time. Shading shows one sample SD.", raw);
    }
}

void compare(const Groups& groups, const fs::path& raw, const fs::path& model, const fs::path& out) {
    std::map<Count, double> predictions;
    for (const auto& row : readCsv(model, "n,depth_limit,predicted_rate,expected_rebuilds", 4)) {
        const double rate = number(row[2]);
        if (rate <= 0 || rate > 1 || !predictions.emplace(count(row[0]), rate).second) {
            throw std::runtime_error("Invalid or duplicate model rate");
        }
    }
    std::ofstream comparison(out / "rebuild_comparison.csv");
    comparison.exceptions(std::ios::badbit | std::ios::failbit);
    comparison << std::boolalpha << std::setprecision(12)
               << "n,seed,strategy,pairs,predicted_rate,observed_rate,observed_over_predicted,"
                  "within_model_range,variants_match\n";
    std::vector<double> sizes;
    std::vector<Statistics> predicted, band;
    std::array<std::vector<Statistics>, 2> observed;
    double top = 0;
    for (const auto& [n, byStrategy] : groups) {
        const double prediction = predictions.at(n);
        sizes.push_back(static_cast<double>(n));
        predicted.push_back({prediction, 0});
        band.push_back({0.55 * prediction, 0.45 * prediction});
        top = std::max(top, prediction);
        const auto& array = byStrategy.at(strategies[2]);
        const auto& inplace = byStrategy.at(strategies[3]);
        if (array.size() != inplace.size()) throw std::runtime_error("Scapegoat seed sets differ");
        std::array<std::vector<double>, 2> rates;
        for (const auto& [seed, a] : array) {
            const auto& b = inplace.at(seed);
            for (std::size_t i = 0; i < rates.size(); ++i) {
                const auto& run = i == 0 ? a : b;
                const double rate = static_cast<double>(run.rebuilds.back() - run.rebuilds.front())
                                  / run.pairs.back();
                const double ratio = rate / prediction;
                rates[i].push_back(rate);
                comparison << n << ',' << seed << ',' << strategies[i + 2] << ',' << run.pairs.back()
                           << ',' << prediction << ',' << rate << ',' << ratio << ','
                           << (ratio >= 0.1 && ratio <= 1) << ',' << matched(a, b) << '\n';
            }
        }
        for (std::size_t i = 0; i < rates.size(); ++i) {
            const auto value = stats(rates[i]);
            observed[i].push_back(value);
            top = std::max(top, value.mean + value.sd);
        }
    }
    const double padding = std::max(1.0, (sizes.back() - sizes.front()) * 0.06);
    Chart chart(out / "rebuild_rate.svg", sizes.front() - padding, sizes.back() + padding,
                top * 1.08, "Predicted and measured rebuild rates", "Tree size (keys)",
                "Rebuilds per churn pair", sizes);
    chart.series(sizes, band, "Predicted range (shaded)", "#a1a9b2", true, true);
    chart.series(sizes, predicted, "Random BST model", "#394655", true);
    for (std::size_t i = 0; i < observed.size(); ++i) {
        chart.series(sizes, observed[i], strategies[i + 2], colors[i + 2], i == 1, false, true);
    }
    chart.finish("Compare observed rates with the predicted band. Error bars show one sample SD.", raw, model);
    comparison.close();
}

void plotAdversary(const Groups& groups, const Groups& baseline, const fs::path& raw,
                   const fs::path& uniform, const fs::path& out, const std::string& schedule,
                   std::ostream& timing) {
    const bool useRebuilds = schedule == "scapegoat_stress";
    const std::size_t start = useRebuilds ? 2 : 0;
    for (const auto& [n, byStrategy] : groups) {
        struct Series {
            std::vector<double> xs;
            std::vector<Statistics> points;
        };
        std::array<Series, 4> series;
        const Count last = byStrategy.at(strategies[start]).begin()->second.pairs.back();
        double top = 1;
        for (std::size_t i = 0; i < 2; ++i) {
            const auto& name = strategies[start + i];
            const auto& stressed = byStrategy.at(name);
            const auto& control = baseline.at(n).at(name);
            if (control.size() != stressed.size()) throw std::runtime_error("Seed counts differ");
            std::vector<double> seconds;
            for (const auto& [seed, run] : stressed) {
                if (control.count(seed) == 0) throw std::runtime_error("Baseline seed missing");
                seconds.push_back(run.seconds);
            }
            const auto elapsed = stats(seconds);
            timing << n << ',' << name << ',' << stressed.size() << ','
                   << elapsed.mean << ',' << elapsed.sd << '\n';
            for (std::size_t k = 0; k < 2; ++k) {
                const auto& runs = k == 0 ? control : stressed;
                auto& line = series[2 * i + k];
                const auto& pairs = runs.begin()->second.pairs;
                if (!std::binary_search(pairs.begin(), pairs.end(), last)) {
                    throw std::runtime_error("Missing matched-budget sample");
                }
                for (const auto& entry : runs) {
                    if (entry.second.pairs != pairs) throw std::runtime_error("Inconsistent sample times");
                }
                for (std::size_t j = 0; j < pairs.size() && pairs[j] <= last; ++j) {
                    std::vector<double> values;
                    for (const auto& entry : runs) {
                        const auto& run = entry.second;
                        values.push_back(useRebuilds
                            ? static_cast<double>(run.rebuilds[j] - run.rebuilds.front())
                            : run.depths[j]);
                    }
                    const auto value = stats(values);
                    line.xs.push_back(static_cast<double>(pairs[j]));
                    line.points.push_back(value);
                    top = std::max(top, value.mean + value.sd);
                }
            }
        }
        std::vector<double> ticks;
        for (int i = 0; i <= 5; ++i) ticks.push_back(static_cast<double>(last) * i / 5);
        Chart chart(out / ("adversarial_" + std::to_string(n) + ".svg"), 0, static_cast<double>(last),
                    top * 1.08, schedule + " versus uniform (n = " + std::to_string(n) + ")",
                    "Churn pairs", useRebuilds ? "Rebuilds since initial build" : "Average node depth (edges)",
                    ticks);
        for (std::size_t i = 0; i < series.size(); ++i) {
            const auto& line = series[i];
            const bool isUniform = i % 2 == 0;
            const std::array<std::string, 4> shortNames{"successor", "random delete", "array", "in-place"};
            const auto name = shortNames[start + i / 2] + (isUniform ? " / uniform" : " / adversarial");
            chart.series(line.xs, line.points, name, colors[start + i / 2], isUniform, true);
        }
        chart.finish("Compare workloads at the same pair budget. Shading shows one sample SD.",
                     raw, uniform / "runs.csv", true);
    }
}

int main(int argc, char** argv) try {
    if (argc != 4 && argc != 5) {
        throw std::runtime_error("Usage: d6_figures RAW PREDICTIONS_CSV OUTPUT [UNIFORM_RAW]");
    }
    const auto raw = fs::u8path(argv[1]), model = fs::u8path(argv[2]), out = fs::u8path(argv[3]);
    std::string schedule;
    const auto groups = loadRuns(raw, schedule);
    fs::create_directories(out);
    std::ofstream timing(out / "timing_summary.csv");
    timing.exceptions(std::ios::badbit | std::ios::failbit);
    timing << std::setprecision(12) << "n,strategy,runs,mean_seconds,sd_seconds\n";
    if (schedule == "uniform") {
        plotDepth(groups, raw, out, schedule, timing);
        compare(groups, raw, model, out);
    } else {
        if (argc != 5) throw std::runtime_error("Adversarial figures require UNIFORM_RAW");
        const auto uniform = fs::u8path(argv[4]);
        std::string baselineSchedule;
        const auto baseline = loadRuns(uniform, baselineSchedule);
        if (baselineSchedule != "uniform") throw std::runtime_error("Expected uniform baseline");
        plotAdversary(groups, baseline, raw, uniform, out, schedule, timing);
    }
    timing.close();
} catch (const std::exception& error) {
    std::cerr << "d6_figures: " << error.what() << '\n';
    return EXIT_FAILURE;
}
