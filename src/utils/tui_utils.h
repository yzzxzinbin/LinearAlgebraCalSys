#pragma once
#include <string>
#include <vector>
#include "../tui/tui_terminal.h" // For Color enum and Terminal functions

namespace TuiUtils {

// 从文件读取所有行
std::vector<std::string> readFileLines(const std::string& filePath);

// 列出指定目录中没有扩展名的文件
std::vector<std::string> listFilesNoExt(const std::string& directoryPath);

// 绘制一个带可选标题的边框
void drawBox(int r, int c, int h, int w, const std::string& title = "", Color borderColor = Color::WHITE);

// 在指定区域内绘制一个字符串列表，支持高亮选中项和滚动
void drawTextList(int r, int c, int h, int w, const std::vector<std::string>& items,
                  int selectedIndex, int scrollOffset,
                  Color itemColor = Color::DEFAULT, Color selectedColor = Color::YELLOW, 
                  Color selectedBgColor = Color::BLUE, Color defaultBgColor = Color::BLACK,
                  // 新增参数用于特殊项样式
                  const std::string& specialItemText = "", 
                  Color specialItemFgColor = Color::DEFAULT,
                  Color specialItemBgColor = Color::DEFAULT);

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

} // namespace TuiUtils
