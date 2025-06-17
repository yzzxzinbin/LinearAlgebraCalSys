#include "tui_utils.h"
#include "../tui/tui_terminal.h" // For Terminal class
#include <fstream>
#include <iostream>       // For std::cout, used by Terminal output
#include <filesystem>     // C++17 for directory listing
#include <algorithm>      // For std::sort, std::max
#include <unordered_set>  // 新增：用于文件扩展名白名单

namespace fs = std::filesystem;

namespace TuiUtils {

std::vector<std::string> readFileLines(const std::string& filePath) {
    std::vector<std::string> lines;
    std::ifstream file(filePath);
    std::string line;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            // Windows换行符\r\n可能导致行尾有\r，需要移除
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            lines.push_back(line);
        }
        file.close();
    }
    return lines;
}

void drawText(int r, int c, const std::string& text, Color fg, Color bg) {
    Terminal::setCursor(r, c);
    Terminal::setForeground(fg);
    Terminal::setBackground(bg);
    std::cout << text;
    Terminal::resetColor(); // 通常在更高级别的绘制函数结束时重置
}

void fillRect(int r, int c, int h, int w, char fillChar, Color fg, Color bg) {
    if (w <= 0 || h <= 0) return;
    Terminal::setForeground(fg);
    Terminal::setBackground(bg);
    std::string line(w, fillChar);
    for (int i = 0; i < h; ++i) {
        Terminal::setCursor(r + i, c);
        std::cout << line;
    }
    Terminal::resetColor();
}

void drawBox(int r, int c, int h, int w, const std::string& title, Color borderColor, Color bgColor) {
    if (h < 2 || w < 2) return;

    Terminal::setForeground(borderColor);
    Terminal::setBackground(bgColor); // 使用传入的 bgColor

    // Corners
    Terminal::setCursor(r, c); std::cout << "╭";
    Terminal::setCursor(r, c + w - 1); std::cout << "╮";
    Terminal::setCursor(r + h - 1, c); std::cout << "╰";
    Terminal::setCursor(r + h - 1, c + w - 1); std::cout << "╯";

    // Horizontal lines
    std::string hLine_char = "─"; // UTF-8 character for horizontal line
    std::string hLine_str;
    int hLine_len = std::max(0, w - 2);
    for (int i = 0; i < hLine_len; ++i) {
        hLine_str += hLine_char;
    }
    Terminal::setCursor(r, c + 1); std::cout << hLine_str;
    Terminal::setCursor(r + h - 1, c + 1); std::cout << hLine_str;
    
    // Vertical lines
    for (int i = 1; i < h - 1; ++i) {
        Terminal::setCursor(r + i, c); std::cout << "│";
        Terminal::setCursor(r + i, c + w - 1); std::cout << "│";
    }

    // Title
    if (!title.empty() && w > 4) {
        int titleX = c + 2;
        int maxTitleLen = std::max(0, w - 4);
        std::string displayTitle = title;
        if (displayTitle.length() > static_cast<size_t>(maxTitleLen)) {
            displayTitle = displayTitle.substr(0, maxTitleLen);
        }
        Terminal::setCursor(r, titleX);
        Terminal::setBackground(bgColor); // 确保标题背景也是 bgColor
        // Ensure title doesn't overwrite box corners if too long
        if (titleX + displayTitle.length() < static_cast<size_t>(c + w -1) ) {
             std::cout << displayTitle;
        }
    }
    Terminal::resetColor();
}

void drawTextList(int r, int c, int h, int w, const std::vector<std::string>& items,
                  int selectedIndex, int scrollOffset,
                  Color itemColor, Color selectedColor,
                  Color selectedBgColor, Color defaultBgColor,
                  const std::string& specialItemText, 
                  Color specialItemFgColor, Color specialItemBgColor) {
    if (h <= 0 || w <= 0) return;

    for (int i = 0; i < h; ++i) {
        Terminal::setCursor(r + i, c);
        int itemIdx = scrollOffset + i;
        if (itemIdx >= 0 && itemIdx < static_cast<int>(items.size())) {
            const std::string& originalItemText = items[itemIdx]; 
            std::string itemTextToDisplay = originalItemText;    

            // 使用视觉宽度进行处理
            size_t visualLen = TuiUtils::countUtf8CodePoints(itemTextToDisplay);

            if (visualLen > static_cast<size_t>(w)) {
                itemTextToDisplay = TuiUtils::trimToVisualWidth(itemTextToDisplay, w);
                // trimToVisualWidth 应该返回一个视觉宽度为 w 的字符串（或尽可能接近）
                // 为确保准确，可以重新计算修剪后的视觉长度，但通常假设trim函数是准确的
                visualLen = TuiUtils::countUtf8CodePoints(itemTextToDisplay); 
            }

            // 基于视觉宽度进行填充
            std::string padding;
            if (visualLen < static_cast<size_t>(w)) {
                padding.assign(w - visualLen, ' ');
            }
            itemTextToDisplay += padding;


            bool isSpecialItem = (!specialItemText.empty() && originalItemText == specialItemText);

            if (itemIdx == selectedIndex) {
                Terminal::setForeground(selectedColor);
                Terminal::setBackground(selectedBgColor);
            } else if (isSpecialItem) {
                Color fg = (specialItemFgColor == Color::DEFAULT) ? itemColor : specialItemFgColor;
                Color bg = (specialItemBgColor == Color::DEFAULT) ? defaultBgColor : specialItemBgColor;
                Terminal::setForeground(fg);
                Terminal::setBackground(bg);
            } else {
                Terminal::setForeground(itemColor);
                Terminal::setBackground(defaultBgColor);
            }
            std::cout << itemTextToDisplay;
        } else {
            // 清除没有项目的行
            Terminal::setBackground(defaultBgColor); // 使用默认背景色清除
            std::cout << std::string(w, ' ');
        }
    }
    Terminal::resetColor();
}

void drawTextLines(int r, int c, int h, int w, const std::vector<std::string>& lines, 
                   Color textColor, Color bgColor) {
    if (h <= 0 || w <= 0) return;
    
    Terminal::setForeground(textColor);
    Terminal::setBackground(bgColor); // 确保背景色被正确设置

    for (int i = 0; i < h; ++i) {
        Terminal::setCursor(r + i, c);
        if (i < static_cast<int>(lines.size())) {
            std::string lineToDraw = lines[i];
            
            size_t currentVisualWidth = TuiUtils::countUtf8CodePoints(lineToDraw);

            if (currentVisualWidth > static_cast<size_t>(w)) {
                lineToDraw = TuiUtils::trimToVisualWidth(lineToDraw, w);
            } else if (currentVisualWidth < static_cast<size_t>(w)) {
                std::string padding(w - currentVisualWidth, ' ');
                lineToDraw += padding;
            }
            // 如果 currentVisualWidth == w, 无需操作

            std::cout << lineToDraw;
        } else {
            // 如果行数少于高度，用背景色清除剩余行
            Terminal::setBackground(bgColor); // 确保清除时使用正确的背景色
            std::cout << std::string(w, ' ');
        }
    }
    Terminal::resetColor();
}

// 修改：改进 countUtf8CodePoints 的健壮性
size_t countUtf8CodePoints(const std::string& s) {
    size_t count = 0;
    for (size_t i = 0; i < s.length(); ) {
        unsigned char c = s[i];
        size_t char_len = 1; // 默认为1字节字符或无效字符

        if (c < 0x80) { // ASCII (1 byte)
            // char_len is already 1
        } else if ((c & 0xE0) == 0xC0) { // 2-byte sequence
            char_len = 2;
        } else if ((c & 0xF0) == 0xE0) { // 3-byte sequence
            char_len = 3;
        } else if ((c & 0xF8) == 0xF0) { // 4-byte sequence
            char_len = 4;
        }
        // else: Invalid UTF-8 start byte or isolated continuation byte. Treat as 1 byte.

        if (i + char_len > s.length()) { // Check if the full character sequence fits
            // Malformed UTF-8 at the end of the string.
            // Count this problematic segment as one visual unit if it has any bytes.
            count++;
            break; 
        }
        count++;
        i += char_len;
    }
    return count;
}

// 新增：实现按视觉宽度截断字符串的函数
std::string trimToVisualWidth(const std::string& s, size_t visualWidth) {
    if (visualWidth == 0) return "";

    std::string result;
    result.reserve(s.length()); // 预分配以提高效率
    size_t current_visual_width = 0;
    
    for (size_t i = 0; i < s.length(); ) {
        if (current_visual_width >= visualWidth) {
            break;
        }

        unsigned char c = s[i];
        size_t char_len = 1; 

        if (c < 0x80) {
            // char_len is 1
        } else if ((c & 0xE0) == 0xC0) {
            char_len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_len = 4;
        }
        // else: Invalid UTF-8 start byte, treat as 1 byte.

        if (i + char_len > s.length()) { // Incomplete char at the end
            break; 
        }
        
        result.append(s, i, char_len);
        current_visual_width++;
        i += char_len;
    }
    return result;
}

} // namespace TuiUtils
