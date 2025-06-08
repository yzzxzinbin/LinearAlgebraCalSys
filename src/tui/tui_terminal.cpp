#include "tui_terminal.h"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <conio.h> // For _getch, _kbhit
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

// 初始化终端，启用虚拟终端处理功能
bool Terminal::init() {
#ifdef _WIN32
    // 在Windows上启用虚拟终端处理
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        return false;
    }
    
    // 启用ANSI转义序列处理
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        return false;
    }
    
    // 设置控制台输入模式
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetConsoleMode(hIn, &dwMode)) {
        return false;
    }
    
    // 启用快速编辑模式和插入模式
    dwMode |= ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS;
    if (!SetConsoleMode(hIn, dwMode)) {
        return false;
    }
#endif
    return true;
}

// 清屏
void Terminal::clear() {
    // 使用ANSI转义序列清屏并将光标移动到左上角
    std::cout << "\033[2J\033[H";
}

// 设置光标位置
void Terminal::setCursor(int row, int col) {
    // 使用ANSI转义序列设置光标位置
    std::cout << "\033[" << (row + 1) << ";" << (col + 1) << "H";
}

// 保存光标位置
void Terminal::saveCursor() {
    // 使用ANSI转义序列保存光标位置
    std::cout << "\033[s";
}

// 恢复光标位置
void Terminal::restoreCursor() {
    // 使用ANSI转义序列恢复光标位置
    std::cout << "\033[u";
}

// 设置前景色
void Terminal::setForeground(Color color) {
    int colorCode;
    
    switch (color) {
        case Color::BLACK:      colorCode = 30; break;
        case Color::RED:        colorCode = 31; break;
        case Color::GREEN:      colorCode = 32; break;
        case Color::YELLOW:     colorCode = 33; break;
        case Color::BLUE:       colorCode = 34; break;
        case Color::MAGENTA:    colorCode = 35; break;
        case Color::CYAN:       colorCode = 36; break;
        case Color::WHITE:      colorCode = 37; break;
        case Color::DEFAULT:    colorCode = 39; break;
    }
    
    std::cout << "\033[" << colorCode << "m";
}

// 设置背景色
void Terminal::setBackground(Color color) {
    int colorCode;
    
    switch (color) {
        case Color::BLACK:      colorCode = 40; break;
        case Color::RED:        colorCode = 41; break;
        case Color::GREEN:      colorCode = 42; break;
        case Color::YELLOW:     colorCode = 43; break;
        case Color::BLUE:       colorCode = 44; break;
        case Color::MAGENTA:    colorCode = 45; break;
        case Color::CYAN:       colorCode = 46; break;
        case Color::WHITE:      colorCode = 47; break;
        case Color::DEFAULT:    colorCode = 49; break;
    }
    
    std::cout << "\033[" << colorCode << "m";
}

// 重置颜色
void Terminal::resetColor() {
    // 使用ANSI转义序列重置所有属性
    std::cout << "\033[0m";
}

// 获取终端大小
std::pair<int, int> Terminal::getSize() {
    int rows = 24;  // 默认值
    int cols = 80;  // 默认值
    
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1) {
        rows = w.ws_row;
        cols = w.ws_col;
    }
#endif
    
    return std::make_pair(rows, cols);
}

// 启用/禁用原始模式
void Terminal::setRawMode(bool enable) {
#ifdef _WIN32
    // Windows使用原生API读取字符，不需要特殊设置
#else
    static struct termios oldTermios, newTermios;
    
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldTermios);
        newTermios = oldTermios;
        
        // 禁用标准输入处理
        newTermios.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
        
        // 禁用软件流控制
        newTermios.c_iflag &= ~(IXON | IXOFF | ICRNL | INLCR | IGNCR);
        
        // 设置输入模式
        newTermios.c_cc[VMIN] = 1;  // 最少读取一个字符
        newTermios.c_cc[VTIME] = 0; // 不设置超时
        
        tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
    }
#endif
}

// 读取一个字符
int Terminal::readChar() {
#ifdef _WIN32
    int c = _getch();
    
    // 检测CTRL+ENTER
    if (c == 10 || (c == 13 && (GetAsyncKeyState(VK_CONTROL) & 0x8000))) {
        return KEY_CTRL_ENTER;
    }
    
    // 检测CTRL+A (新增)
    if ((c == 'a' || c == 'A' || c == 1) && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
        return KEY_CTRL_A;
    }
    
    // 处理特殊键前缀
    if (c == 0 || c == 224) {
        int scancode = _getch();
        
        // 根据您的调试结果，CTRL+方向键可能会产生特殊的扫描码
        // 因此直接根据扫描码返回对应的键值，而不检测CTRL状态
        switch (scancode) {
            // 普通方向键
            case 72: return KEY_UP;          // 上箭头
            case 80: return KEY_DOWN;        // 下箭头
            case 75: return KEY_LEFT;        // 左箭头
            case 77: return KEY_RIGHT;       // 右箭头
            
            // CTRL+方向键的特殊扫描码 - 这些值可能需要根据实际测试调整
            case 116: return KEY_CTRL_RIGHT;  // CTRL+右箭头 (根据您的调试)
            case 115: return KEY_CTRL_LEFT;   // CTRL+左箭头 (推测值，需要测试)
            case 141: return KEY_CTRL_UP;     // CTRL+上箭头 (推测值，需要测试)
            case 145: return KEY_CTRL_DOWN;   // CTRL+下箭头 (推测值，需要测试)
            
            case 83: return KEY_DELETE;      // Delete键
            default: 
                // 调试未知扫描码 - 您可以在此处添加代码，将未知扫描码打印到日志
                // 如果遇到未识别的扫描码，可以修改此函数以处理它们
                return -1;  // 未处理的特殊键
        }
    }
    
    return c; // 普通键
#else
    unsigned char c_val;
    if (read(STDIN_FILENO, &c_val, 1) == 1) {
        // CTRL+Enter检测
        if (c_val == 10 && (fcntl(STDIN_FILENO, F_GETFL) & O_NONBLOCK)) {
            return KEY_CTRL_ENTER;
        }
        
        // 处理ANSI转义序列
        if (c_val == 27) { // ESC字符
            // 检查是否是转义序列的一部分
            fd_set readfds;
            struct timeval timeout;
            
            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000; // 100ms超时
            
            // 检查是否有更多字符可读（转义序列的其余部分）
            if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) > 0) {
                unsigned char next;
                if (read(STDIN_FILENO, &next, 1) == 1) {
                    // 检测CTRL+方向键 (Linux中通常是 ESC [ 1 ; 5 A/B/C/D)
                    if (next == '[') { 
                        unsigned char code;
                        if (read(STDIN_FILENO, &code, 1) == 1) {
                            if (code == '1') {
                                char semicolon, five, direction;
                                if (read(STDIN_FILENO, &semicolon, 1) == 1 && semicolon == ';' &&
                                    read(STDIN_FILENO, &five, 1) == 1 && five == '5' &&
                                    read(STDIN_FILENO, &direction, 1) == 1) {
                                    switch (direction) {
                                        case 'A': return KEY_CTRL_UP;
                                        case 'B': return KEY_CTRL_DOWN;
                                        case 'C': return KEY_CTRL_RIGHT;
                                        case 'D': return KEY_CTRL_LEFT;
                                    }
                                }
                            }
                            
                            // 处理普通方向键
                            switch (code) {
                                case 'A': return KEY_UP;    // 上箭头
                                case 'B': return KEY_DOWN;  // 下箭头
                                case 'C': return KEY_RIGHT; // 右箭头
                                case 'D': return KEY_LEFT;  // 左箭头
                                case '3': // 可能是Delete键 (ESC [ 3 ~)
                                    if (read(STDIN_FILENO, &code, 1) == 1 && code == '~') {
                                        return KEY_DELETE;
                                    }
                                    break;
                            }
                        }
                    }
                }
                // 如果无法识别完整序列，返回ESC
                return KEY_ESCAPE;
            } else {
                // 如果没有后续字符，这只是一个单独的ESC键
                return KEY_ESCAPE;
            }
        }
        
        // 在Linux上，将DELETE键(127)作为退格键处理
        if (c_val == 127) {
            return KEY_BACKSPACE;
        }
        
        return (int)c_val;
    }
    return -1; // 读取错误
#endif
}

// 检查是否有输入可用
bool Terminal::hasInput() {
#ifdef _WIN32
    return _kbhit() != 0;
#else
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
#endif
}
