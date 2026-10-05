// main.cpp — console front end of the BS Constructor core.
//
//   bs_constructor [ACT_FILE|-] [--operator CODE] [--switch CODE] [--csv FILE] [--dwg] [--help]
//
// Without ACT_FILE a built-in demo act is used; "-" reads the act from stdin.
// Exit code: 0 — project is valid, 2 — project has conflicts, 1 — usage / IO error.
#include "BSConstructor.h"
#include "Report.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view kDemoAct =
    "АКТ ПРЕДПРОЕКТНОГО ОБСЛЕДОВАНИЯ\n"
    "Оператор: ПАО «МТС»\n"
    "Номер БС: 78-04546 (Орово)\n"
    "Тип площадки: Наземная АМС (опора ОДН Русмаркет)\n"
    "Высота подвеса АФУ: +36,100 м\n"
    "Количество секторов: 3\n"
    "Кабельная трасса: оптика внутри ствола ОДН\n"
    "Аппаратная: ТКШ Интеркросс Тип 5 на подвесной раме\n";

struct Options {
    std::string actPath;
    std::optional<bsc::Operator> op;
    std::optional<bsc::Operator> switchTo;
    std::string csvPath;
    bool dwg = false;
    bool help = false;
};

void printUsage() {
    std::cerr << "Usage: bs_constructor [ACT_FILE|-] [--operator MTS|T2|VIMPELCOM|MEGAFON]\n"
                 "                      [--switch MTS|T2|VIMPELCOM|MEGAFON] [--csv FILE] [--dwg]\n";
}

std::optional<bsc::Operator> parseOperator(std::string_view code) {
    const auto op = bsc::operatorFromCode(code);
    if (!op) {
        std::cerr << "Unknown operator code: " << code << "\n";
    }
    return op;
}

std::optional<Options> parseOptions(const std::vector<std::string_view>& args) {
    Options options;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string_view arg = args[i];
        const bool hasValue = i + 1 < args.size();
        if (arg == "--operator" && hasValue) {
            options.op = parseOperator(args[++i]);
            if (!options.op) return std::nullopt;
        } else if (arg == "--switch" && hasValue) {
            options.switchTo = parseOperator(args[++i]);
            if (!options.switchTo) return std::nullopt;
        } else if (arg == "--csv" && hasValue) {
            options.csvPath = std::string(args[++i]);
        } else if (arg == "--dwg") {
            options.dwg = true;
        } else if (arg == "--help" || arg == "-h") {
            options.help = true;
        } else if (!arg.empty() && (arg.front() != '-' || arg == "-") && options.actPath.empty()) {
            options.actPath = std::string(arg);
        } else {
            return std::nullopt;
        }
    }
    return options;
}

std::optional<std::string> readAct(const std::string& path) {
    if (path.empty()) {
        return std::string(kDemoAct);
    }
    if (path == "-") {
        return std::string(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open act file: " << path << "\n";
        return std::nullopt;
    }
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

void printProject(const bsc::Project& project, bool withDwg) {
    std::cout << bsc::formatReport(project);
    if (withDwg) {
        std::cout << "\nАтрибуты штампа dwg:\n";
        for (const auto& [name, value] : bsc::dwgAttributes(project)) {
            std::cout << "  " << name << " = " << value << "\n";
        }
    }
}

int run(const Options& options) {
    const auto act = readAct(options.actPath);
    if (!act) {
        return 1;
    }
    bsc::BSConstructor constructor;
    const bsc::Project* project = &constructor.loadAct(*act);
    if (options.op) {
        project = &constructor.switchOperator(*options.op);
    }
    printProject(*project, options.dwg);

    if (options.switchTo) {
        std::cout << "\n=== Переключение оператора: " << bsc::nameOf(*options.switchTo) << " ===\n\n";
        project = &constructor.switchOperator(*options.switchTo);
        printProject(*project, options.dwg);
    }
    if (!options.csvPath.empty()) {
        std::ofstream csv(options.csvPath, std::ios::binary);
        csv << bsc::specificationCsv(*project);
        if (!csv) {
            std::cerr << "Cannot write CSV file: " << options.csvPath << "\n";
            return 1;
        }
    }
    return project->isValid() ? 0 : 2;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const std::vector<std::string_view> args(argv + 1, argv + argc);
        const auto options = parseOptions(args);
        if (!options || options->help) {
            printUsage();
            return options ? 0 : 1;
        }
        return run(*options);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
