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

void drawTextList(int r, int c, int h, int w, 
                  const std::vector<PrintableListItem>& itemsToPrint,
                  int selectedIndex, int scrollOffset,
                  Color defaultItemTextColor, Color selectedItemTextColor, 
                  Color selectedItemBgColor, Color defaultItemBgColor,
                  const std::string& specialItemFullTextMatch, 
                  Color specialItemTextColor, Color specialItemBgColor) {
    if (h <= 0 || w <= 0) return;

    for (int i = 0; i < h; ++i) {
        Terminal::setCursor(r + i, c);
        int itemIdx = scrollOffset + i;

        if (itemIdx >= 0 && itemIdx < static_cast<int>(itemsToPrint.size())) {
            const auto& pItem = itemsToPrint[itemIdx];
            
            Color currentItemFgColor = defaultItemTextColor;
            Color currentItemBgColor = defaultItemBgColor;
            bool isSpecialMatched = false;

            if (!specialItemFullTextMatch.empty() && pItem.fullDisplayStringForMatching == specialItemFullTextMatch) {
                isSpecialMatched = true;
                currentItemFgColor = specialItemTextColor; // Text color for special item
                currentItemBgColor = specialItemBgColor;   // Background for special item
            }

            if (itemIdx == selectedIndex) {
                currentItemFgColor = selectedItemTextColor; // Override with selected text color
                currentItemBgColor = selectedItemBgColor;   // Override with selected background
            }
            
            Terminal::setBackground(currentItemBgColor); // Set background for the entire line

            size_t currentVisualCol = 0;
            // auto printSegment = [&](const std::string& text, Color fgColor, size_t& visualCol, int maxWidth) { // This helper is not used for indent anymore
            //     if (visualCol >= static_cast<size_t>(maxWidth)) return;

            //     std::string segmentToPrint = text;
            //     size_t segmentVisualLen = TuiUtils::calculateUtf8VisualWidth(segmentToPrint); // MODIFIED
                
            //     if (visualCol + segmentVisualLen > static_cast<size_t>(maxWidth)) {
            //         segmentToPrint = TuiUtils::trimToUtf8VisualWidth(segmentToPrint, maxWidth - visualCol); // MODIFIED
            //         segmentVisualLen = TuiUtils::calculateUtf8VisualWidth(segmentToPrint); // Re-evaluate after trim // MODIFIED
            //     }

            //     Terminal::setForeground(fgColor);
            //     std::cout << segmentToPrint;
            //     visualCol += segmentVisualLen;
            // };
            
            // Print Indent using its specific RGB color
            if (!pItem.indentString.empty() && currentVisualCol < static_cast<size_t>(w)) {
                std::string indentToPrint = pItem.indentString;
                size_t indentActualVisualLen = TuiUtils::calculateUtf8VisualWidth(indentToPrint); // MODIFIED

                if (currentVisualCol + indentActualVisualLen > static_cast<size_t>(w)) {
                    indentToPrint = TuiUtils::trimToUtf8VisualWidth(indentToPrint, w - currentVisualCol); // MODIFIED
                    indentActualVisualLen = TuiUtils::calculateUtf8VisualWidth(indentToPrint); // Re-evaluate // MODIFIED
                }
                if (indentActualVisualLen > 0) {
                    Terminal::setForegroundRGB(pItem.indentColor.r, pItem.indentColor.g, pItem.indentColor.b);
                    std::cout << indentToPrint;
                    currentVisualCol += indentActualVisualLen;
                }
            }
            
            // Print Icon (if any)
            if (!pItem.iconGlyph.empty()) {
                // Space before icon
                if (currentVisualCol < static_cast<size_t>(w)) {
                    Terminal::setForeground(currentItemFgColor); // Use current item's text color for space
                    std::cout << " ";
                    currentVisualCol++;
                }
                
                // Icon itself (with its own RGB color)
                if (currentVisualCol < static_cast<size_t>(w)) {
                    Terminal::setForegroundRGB(pItem.iconColor.r, pItem.iconColor.g, pItem.iconColor.b);
                    
                    std::string iconToPrint = pItem.iconGlyph;
                    size_t iconActualVisualLen = TuiUtils::calculateUtf8VisualWidth(iconToPrint); // MODIFIED
                    bool iconPrinted = false;

                    if (currentVisualCol + iconActualVisualLen > static_cast<size_t>(w)) {
                        iconToPrint = TuiUtils::trimToUtf8VisualWidth(iconToPrint, w - currentVisualCol); // MODIFIED
                        iconActualVisualLen = TuiUtils::calculateUtf8VisualWidth(iconToPrint);  // MODIFIED
                    }
                    if (iconActualVisualLen > 0) {
                        std::cout << iconToPrint;
                        currentVisualCol += iconActualVisualLen;
                        iconPrinted = true;
                    }
                    
                    // Space after icon (if icon was printed)
                    if (iconPrinted && currentVisualCol < static_cast<size_t>(w)) {
                        Terminal::setForeground(currentItemFgColor); // Use current item's text color for space
                        std::cout << " ";
                        currentVisualCol++;
                    }
                }
            }
            
            // Print Text using currentItemFgColor
            if (currentVisualCol < static_cast<size_t>(w)) {
                Terminal::setForeground(currentItemFgColor);
                std::string textToPrint = pItem.textWithoutIcon;
                
                if (currentVisualCol < static_cast<size_t>(w)) { // Ensure there's space left before trying to print text
                    size_t availableTextWidth = w - currentVisualCol;
                    size_t textActualVisualLen = TuiUtils::calculateUtf8VisualWidth(textToPrint); // MODIFIED

                    if (textActualVisualLen > availableTextWidth) {
                        textToPrint = TuiUtils::trimToUtf8VisualWidth(textToPrint, availableTextWidth); // MODIFIED
                        textActualVisualLen = TuiUtils::calculateUtf8VisualWidth(textToPrint); // Re-evaluate actual printed width // MODIFIED
                    }
                    std::cout << textToPrint;
                    currentVisualCol += textActualVisualLen; // Use actual printed visual width
                }
            }


            // Fill remaining space on the line
            if (currentVisualCol < static_cast<size_t>(w)) {
                Terminal::setForeground(currentItemFgColor); // Not strictly necessary for spaces
                std::cout << std::string(w - currentVisualCol, ' ');
            }

        } else {
            // Clear empty lines below the list items
            Terminal::setBackground(defaultItemBgColor);
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

//countUtf8CodePoints 非通用性宽度计算函数 专用于启动bannar输出控制
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

// 基于 countUtf8CodePoints 实现按视觉宽度截断字符串的函数
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

// 计算UTF-8字符串实际视觉宽度的通用函数方案 兼容PUA字符
size_t calculateUtf8VisualWidth(const std::string& s) {
    std::string str = stripAnsiEscape(s);
    size_t visual_width = 0;
    for (size_t i = 0; i < str.length(); ) {
        unsigned char byte = static_cast<unsigned char>(str[i]);
        size_t char_len = 1;
        size_t char_visual_width = 1;
        uint32_t codepoint = 0; // 用于存储解码后的码点

        if (byte < 0x80) { // ASCII (0xxxxxxx)
            codepoint = byte;
            // char_len = 1, char_visual_width = 1 (already set)
        } else if ((byte & 0xE0) == 0xC0) { // 2-byte sequence (110xxxxx 10xxxxxx)
            char_len = 2;
            if (i + 1 < str.length() && (static_cast<unsigned char>(str[i+1]) & 0xC0) == 0x80) {
                codepoint = ((byte & 0x1F) << 6) | (static_cast<unsigned char>(str[i+1]) & 0x3F);
                char_visual_width = 2; 
            } else { // Malformed
                char_len = 1; char_visual_width = 1; codepoint = byte;
            }
        } else if ((byte & 0xF0) == 0xE0) { // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
            char_len = 3;
            if (i + 2 < str.length() && (static_cast<unsigned char>(str[i+1]) & 0xC0) == 0x80 && (static_cast<unsigned char>(str[i+2]) & 0xC0) == 0x80) {
                codepoint = ((byte & 0x0F) << 12) | ((static_cast<unsigned char>(str[i+1]) & 0x3F) << 6) | (static_cast<unsigned char>(str[i+2]) & 0x3F);
                if (codepoint >= 0x2500 && codepoint <= 0x257F) { // Box Drawing characters
                    char_visual_width = 1;
                } else {
                    char_visual_width = 2; // Default for 3-byte (e.g., CJK)
                }
            } else { // Malformed
                char_len = 1; char_visual_width = 1; codepoint = byte;
            }
        } else if ((byte & 0xF8) == 0xF0) { // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
            char_len = 4;
            if (i + 3 < str.length() && (static_cast<unsigned char>(str[i+1]) & 0xC0) == 0x80 && (static_cast<unsigned char>(str[i+2]) & 0xC0) == 0x80 && (static_cast<unsigned char>(str[i+3]) & 0xC0) == 0x80) {
                codepoint = ((byte & 0x07) << 18) | ((static_cast<unsigned char>(str[i+1]) & 0x3F) << 12) | ((static_cast<unsigned char>(str[i+2]) & 0x3F) << 6) | (static_cast<unsigned char>(str[i+3]) & 0x3F);
                char_visual_width = 2; 
            } else { // Malformed
                char_len = 1; char_visual_width = 1; codepoint = byte;
            }
        } else {
            // Invalid UTF-8 start byte or isolated continuation byte.
            char_len = 1; char_visual_width = 1; codepoint = byte;
        }

        // --- 新增：Nerd Font PUA 区域特判 ---
        // BMP PUA: U+E000 ~ U+F8FF
        // SMP PUA: U+F0000 ~ U+FFFFD
        // SIP PUA: U+100000 ~ U+10FFFD
        if (
            (codepoint >= 0xE000 && codepoint <= 0xF8FF) ||
            (codepoint >= 0xF0000 && codepoint <= 0xFFFFD) ||
            (codepoint >= 0x100000 && codepoint <= 0x10FFFD)
        ) {
            char_visual_width = 1;
        }

        // --- 新增:希腊字母长度修正 ---
        if (codepoint >= 0x0391 && codepoint <= 0x03A1) { // 希腊大写字母
            char_visual_width = 1;
        } else if (codepoint >= 0x03B1 && codepoint <= 0x03C1) { // 希腊小写字母
            char_visual_width = 1;
        }

        // --- 新增:拉丁及拉丁扩展块长度修正 ---
        if (codepoint >= 0x0041 && codepoint <= 0x005A) { // 拉丁大写字母
            char_visual_width = 1;
        } else if (codepoint >= 0x0061 && codepoint <= 0x007A) { // 拉丁小写字母
            char_visual_width = 1;
        } else if (codepoint >= 0x0080 && codepoint <= 0x00FF) { // 拉丁-1补充
            char_visual_width = 1;
        } else if (codepoint >= 0x0100 && codepoint <= 0x02AF) { // 扩展
            char_visual_width = 1;
        }

        if (codepoint >= 0x2190 && codepoint <= 0x21FF)
        { // 箭头区块
            char_visual_width = 1;
        }
        else if (codepoint >= 0x27F0 && codepoint <= 0x27FF)
        { // 补充箭头
            char_visual_width = 1;
        }
        else if (codepoint >= 0x2B00 && codepoint <= 0x2BFF)
        { // 杂项箭头
            char_visual_width = 1;
        }
        // --- end 新增 ---

        // Boundary check: ensure the full character sequence is within the string
        if (i + char_len > s.length()) {
            visual_width += (s.length() - i);
            break; 
        }
        
        visual_width += char_visual_width;
        i += char_len;
    }
    return visual_width;
}

// 新增：实现按实际视觉宽度截断UTF-8字符串的函数
std::string trimToUtf8VisualWidth(const std::string& s, size_t targetVisualWidth) {
    if (targetVisualWidth == 0) return "";

    std::string result;
    result.reserve(s.length()); // Pre-allocate for efficiency
    size_t current_visual_width = 0;
    
    for (size_t i = 0; i < s.length(); ) {
        unsigned char byte = static_cast<unsigned char>(s[i]);
        size_t char_len = 1;
        size_t char_visual_width = 1;
        uint32_t codepoint = 0; // 用于存储解码后的码点

        if (byte < 0x80) { // ASCII
            codepoint = byte;
            // char_len = 1, char_visual_width = 1
        } else if ((byte & 0xE0) == 0xC0) { // 2-byte
            char_len = 2;
            if (i + 1 < s.length() && (static_cast<unsigned char>(s[i+1]) & 0xC0) == 0x80) {
                codepoint = ((byte & 0x1F) << 6) | (static_cast<unsigned char>(s[i+1]) & 0x3F);
                char_visual_width = 2;
            } else { // Malformed
                char_len = 1; char_visual_width = 1; codepoint = byte;
            }
        } else if ((byte & 0xF0) == 0xE0) { // 3-byte
            char_len = 3;
            if (i + 2 < s.length() && (static_cast<unsigned char>(s[i+1]) & 0xC0) == 0x80 && (static_cast<unsigned char>(s[i+2]) & 0xC0) == 0x80) {
                codepoint = ((byte & 0x0F) << 12) | ((static_cast<unsigned char>(s[i+1]) & 0x3F) << 6) | (static_cast<unsigned char>(s[i+2]) & 0x3F);
                if (codepoint >= 0x2500 && codepoint <= 0x257F) { // Box Drawing characters
                    char_visual_width = 1;
                } else {
                    char_visual_width = 2;
                }
            } else { // Malformed
                char_len = 1; char_visual_width = 1; codepoint = byte;
            }
        } else if ((byte & 0xF8) == 0xF0) { // 4-byte
            char_len = 4;
            if (i + 3 < s.length() && (static_cast<unsigned char>(s[i+1]) & 0xC0) == 0x80 && (static_cast<unsigned char>(s[i+2]) & 0xC0) == 0x80 && (static_cast<unsigned char>(s[i+3]) & 0xC0) == 0x80) {
                codepoint = ((byte & 0x07) << 18) | ((static_cast<unsigned char>(s[i+1]) & 0x3F) << 12) | ((static_cast<unsigned char>(s[i+2]) & 0x3F) << 6) | (static_cast<unsigned char>(s[i+3]) & 0x3F);
                char_visual_width = 2;
            } else { // Malformed
                char_len = 1; char_visual_width = 1; codepoint = byte;
            }
        } else {
            // Invalid start byte, treat as 1-byte, 1-width char
            // char_len = 1, char_visual_width = 1
            codepoint = byte;
        }

        if (i + char_len > s.length()) { // Incomplete char at the end of the string
            break; 
        }
        
        // Check if adding this character would exceed the target visual width
        if (current_visual_width + char_visual_width > targetVisualWidth) {
            break; 
        }
        
        result.append(s, i, char_len);
        current_visual_width += char_visual_width;
        i += char_len;
    }
    return result;
}

std::vector<std::string> wordWrap(const std::string& text, size_t maxWidth) {
    std::vector<std::string> lines;
    if (maxWidth == 0) return lines;
    size_t pos = 0;
    size_t len = text.length();
    while (pos < len) {
        // 跳过前导换行
        if (text[pos] == '\n') {
            lines.push_back("");
            ++pos;
            continue;
        }
        // 找到本段的结尾（遇到\n或到末尾）
        size_t lineEnd = text.find('\n', pos);
        if (lineEnd == std::string::npos) lineEnd = len;
        size_t segStart = pos;
        while (segStart < lineEnd) {
            size_t segLen = 0;
            size_t visualWidth = 0;
            // 按视觉宽度截断
            while (segStart + segLen < lineEnd && visualWidth < maxWidth) {
                unsigned char c = text[segStart + segLen];
                size_t charLen = 1;
                if (c < 0x80) {
                } else if ((c & 0xE0) == 0xC0) {
                    charLen = 2;
                } else if ((c & 0xF0) == 0xE0) {
                    charLen = 3;
                } else if ((c & 0xF8) == 0xF0) {
                    charLen = 4;
                }
                if (segStart + segLen + charLen > lineEnd) break;
                std::string ch = text.substr(segStart + segLen, charLen);
                size_t chWidth = TuiUtils::calculateUtf8VisualWidth(ch);
                if (visualWidth + chWidth > maxWidth) break;
                segLen += charLen;
                visualWidth += chWidth;
            }
            lines.push_back(text.substr(segStart, segLen));
            segStart += segLen;
        }
        pos = lineEnd + 1;
    }
    return lines;
}

// 过滤 ANSI 转义序列
std::string stripAnsiEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size()); // 预分配内存优化
    
    for (size_t i = 0; i < s.size(); ) {
        if (s[i] == '\x1B') { // 遇到转义字符
            if (i + 1 >= s.size()) {
                break; // 不完整的转义序列，直接结束
            }
            
            // 处理 CSI 序列 (ESC [)
            if (s[i+1] == '[') {
                i += 2; // 跳过 ESC [
                
                // 跳过参数部分，直到命令字符
                while (i < s.size() && (isdigit(s[i]) || s[i] == ';' || s[i] == '?')) {
                    i++;
                }
                
                // 跳过命令字符 (在 0x40-0x7E 之间)
                if (i < s.size() && s[i] >= '@' && s[i] <= '~') {
                    i++;
                }
                continue;
            }
            // 处理 OSC 序列 (ESC ])
            else if (s[i+1] == ']') {
                i += 2; // 跳过 ESC ]
                
                // 跳过直到 BEL (\a) 或 ST (\x1B\\)
                while (i < s.size()) {
                    if (s[i] == '\x07') { // BEL 终止符
                        i++;
                        break;
                    }
                    // ST 终止符 (ESC \)
                    if (s[i] == '\x1B' && i+1 < s.size() && s[i+1] == '\\') {
                        i += 2;
                        break;
                    }
                    i++;
                }
                continue;
            }
            // 处理单字符控制序列
            else if (isalpha(s[i+1]) || s[i+1] == '7' || s[i+1] == '8' || s[i+1] == '=' || s[i+1] == '>') {
                i += 2; // 跳过 ESC + 单字符
                continue;
            }
            // 其他未知转义序列，跳过 ESC 字符
            else {
                i++;
                continue;
            }
        }
        
        // 普通字符，添加到输出
        out.push_back(s[i]);
        i++;
    }
    
    return out;
}

} // namespace TuiUtils
