#pragma once
#include <string>
#include <vector>
#include "../tui/tui_terminal.h" // For Color enum and Terminal functions

namespace TuiUtils {

// New struct to pass detailed item information to drawTextList
struct PrintableListItem {
    std::string indentString;
    std::string iconGlyph;
    RGBColor iconColor; // Changed from Color to RGBColor
    RGBColor indentColor; // 新增：用于缩进/树结构字符的颜色
    std::string textWithoutIcon;
    std::string fullDisplayStringForMatching; // Used for specialItemText matching against the original full display string
};

// 从文件读取所有行
std::vector<std::string> readFileLines(const std::string& filePath);

// 绘制一个带可选标题的边框
void drawBox(int r, int c, int h, int w, const std::string& title = "", Color borderColor = Color::WHITE, Color bgColor = Color::DEFAULT);

// 在指定区域内绘制一个字符串列表，支持高亮选中项和滚动
void drawTextList(int r, int c, int h, int w, 
                  const std::vector<PrintableListItem>& itemsToPrint, // Changed to use PrintableListItem
                  int selectedIndex, int scrollOffset,
                  Color defaultItemTextColor = Color::DEFAULT,     // Renamed from itemColor
                  Color selectedItemTextColor = Color::YELLOW,   // Renamed from selectedColor
                  Color selectedItemBgColor = Color::BLUE, 
                  Color defaultItemBgColor = Color::BLACK,       // Renamed from defaultBgColor
                  const std::string& specialItemFullTextMatch = "", // To match against PrintableListItem.fullDisplayStringForMatching
                  Color specialItemTextColor = Color::DEFAULT,   // Renamed from specialItemFgColor, applies to text
                  Color specialItemBgColor = Color::DEFAULT);    // Renamed from specialItemBgColor

void drawTextLines(int r, int c, int h, int w, const std::vector<std::string>& lines, 
                   Color textColor = Color::DEFAULT, Color bgColor = Color::BLACK);

// 在指定位置绘制单行文本
void drawText(int r, int c, const std::string& text, Color fg = Color::DEFAULT, Color bg = Color::DEFAULT);

// 用指定字符填充矩形区域
void fillRect(int r, int c, int h, int w, char fillChar = ' ', Color fg = Color::DEFAULT, Color bg = Color::DEFAULT);

// 新增：计算UTF-8字符串的码点数量（作为视觉宽度的近似）
size_t countUtf8CodePoints(const std::string& s);

// 新增：按视觉宽度截断字符串
std::string trimToVisualWidth(const std::string& s, size_t visualWidth);

// 新增：计算UTF-8字符串的实际视觉宽度（例如，中文字符计为2）
size_t calculateUtf8VisualWidth(const std::string& s);

// 新增：按实际视觉宽度截断UTF-8字符串
std::string trimToUtf8VisualWidth(const std::string& s, size_t targetVisualWidth);

} // namespace TuiUtils
