#pragma once

#include <string>
#include <vector>
#include <iostream>

namespace storeforge::cli {

enum class Alignment {
    Left,
    Right,
    Center
};

class TablePrinter {
public:
    TablePrinter() = default;

    void addColumn(std::string header, size_t minWidth = 10, Alignment align = Alignment::Left);
    void addRow(std::vector<std::string> cells);
    void print(std::ostream& os = std::cout) const;
    void clear();

    // ANSI Color helpers
    static std::string colorize(const std::string& text, const std::string& ansiColor);
    static std::string green(const std::string& text);
    static std::string red(const std::string& text);
    static std::string yellow(const std::string& text);
    static std::string cyan(const std::string& text);
    static std::string bold(const std::string& text);
    static std::string dim(const std::string& text);

private:
    struct ColumnDef {
        std::string header;
        size_t width;
        Alignment align;
    };

    std::vector<ColumnDef> columns_;
    std::vector<std::vector<std::string>> rows_;

    static std::string padCell(const std::string& text, size_t width, Alignment align);
    static size_t visibleLength(const std::string& text);
};

} // namespace storeforge::cli
