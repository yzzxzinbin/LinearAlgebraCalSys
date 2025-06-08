#pragma once
#include <string>
#include <vector>
#include <utility> // For std::pair

// 定义特殊键码
// 这些常量移到这里，以便 Terminal::readChar 和 TuiApp 都能使用
const int KEY_ENTER = 13;
const int KEY_ESCAPE = 27;
const int KEY_BACKSPACE = 8;
const int KEY_DELETE = 127; // Linux系统通常使用127作为退格键(Backspace)
// 保持原有的特殊键值定义
const int KEY_UP = 256;
const int KEY_DOWN = 257;
const int KEY_LEFT = 258;
const int KEY_RIGHT = 259;

// 新增：用于增强型编辑器模式的组合键 (这些值的实际检测依赖于 Terminal::readChar 实现)
const int KEY_CTRL_ENTER = 0x1000; 
const int KEY_CTRL_UP    = 0x1001; 
const int KEY_CTRL_DOWN  = 0x1002; 
const int KEY_CTRL_LEFT  = 0x1003; 
const int KEY_CTRL_RIGHT = 0x1004; 
const int KEY_CTRL_A     = 0x1005; // 新增：CTRL+A全选
// KEY_DELETE is already defined, ensure Terminal::readChar() can return it.


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
    // 初始化终端（启用虚拟终端处理）
    static bool init();
    
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
