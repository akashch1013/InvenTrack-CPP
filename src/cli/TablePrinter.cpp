#include "storeforge/cli/TablePrinter.hpp"
#include <iomanip>
#include <algorithm>
#include <regex>

namespace storeforge::cli {

std::string TablePrinter::colorize(const std::string& text, const std::string& ansiColor) {
    return "\033[" + ansiColor + "m" + text + "\033[0m";
}

std::string TablePrinter::green(const std::string& text) {
    return colorize(text, "1;32");
}

std::string TablePrinter::red(const std::string& text) {
    return colorize(text, "1;31");
}

std::string TablePrinter::yellow(const std::string& text) {
    return colorize(text, "1;33");
}

std::string TablePrinter::cyan(const std::string& text) {
    return colorize(text, "1;36");
}

std::string TablePrinter::bold(const std::string& text) {
    return colorize(text, "1");
}

std::string TablePrinter::dim(const std::string& text) {
    return colorize(text, "2");
}

size_t TablePrinter::visibleLength(const std::string& text) {
    // Strip ANSI escape codes when computing visual column width
    static const std::regex ansiRegex(R"(\x1B\[[0-9;]*[a-zA-Z])");
    std::string stripped = std::regex_replace(text, ansiRegex, "");
    return stripped.length();
}

std::string TablePrinter::padCell(const std::string& text, size_t width, Alignment align) {
    size_t visLen = visibleLength(text);
    if (visLen >= width) {
        return text;
    }
    size_t padTotal = width - visLen;
    if (align == Alignment::Right) {
        return std::string(padTotal, ' ') + text;
    } else if (align == Alignment::Center) {
        size_t leftPad = padTotal / 2;
        size_t rightPad = padTotal - leftPad;
        return std::string(leftPad, ' ') + text + std::string(rightPad, ' ');
    } else {
        return text + std::string(padTotal, ' ');
    }
}

void TablePrinter::addColumn(std::string header, size_t minWidth, Alignment align) {
    size_t w = std::max(header.length() + 2, minWidth);
    columns_.push_back({std::move(header), w, align});
}

void TablePrinter::addRow(std::vector<std::string> cells) {
    for (size_t i = 0; i < cells.size() && i < columns_.size(); ++i) {
        size_t len = visibleLength(cells[i]) + 2;
        if (len > columns_[i].width) {
            columns_[i].width = len;
        }
    }
    rows_.push_back(std::move(cells));
}

void TablePrinter::print(std::ostream& os) const {
    if (columns_.empty()) return;

    // Helper lambda for border lines
    auto printDivider = [&](char left, char mid, char right, char fill) {
        os << left;
        for (size_t i = 0; i < columns_.size(); ++i) {
            os << std::string(columns_[i].width, fill);
            if (i + 1 < columns_.size()) {
                os << mid;
            }
        }
        os << right << "\n";
    };

    // Top border
    printDivider('+', '+', '+', '-');

    // Header row
    os << "|";
    for (const auto& col : columns_) {
        os << padCell(" " + col.header + " ", col.width, Alignment::Center) << "|";
    }
    os << "\n";

    // Header separator
    printDivider('+', '+', '+', '=');

    // Rows
    for (const auto& row : rows_) {
        os << "|";
        for (size_t i = 0; i < columns_.size(); ++i) {
            std::string content = (i < row.size()) ? row[i] : "";
            os << padCell(" " + content + " ", columns_[i].width, columns_[i].align) << "|";
        }
        os << "\n";
    }

    // Bottom border
    printDivider('+', '+', '+', '-');
}

void TablePrinter::clear() {
    columns_.clear();
    rows_.clear();
}

} // namespace storeforge::cli
