#pragma once
#include <string>
#include <vector>

// 终端颜色
enum class Color {
    BLACK,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
    DEFAULT
};

// 终端控制类
class Terminal {
public:
    // 清屏
    static void clear();
    
    // 设置光标位置
    static void setCursor(int row, int col);
    
    // 保存光标位置
    static void saveCursor();
    
    // 恢复光标位置
    static void restoreCursor();
    
    // 设置前景色
    static void setForeground(Color color);
    
    // 设置背景色
    static void setBackground(Color color);
    
    // 重置颜色
    static void resetColor();
    
    // 获取终端大小
    static std::pair<int, int> getSize();
    
    // 启用/禁用原始模式（不需要回车确认输入）
    static void setRawMode(bool enable);
    
    // 读取一个字符（在原始模式下使用）
    static int readChar();
    
    // 检查是否有输入可用
    static bool hasInput();
};
