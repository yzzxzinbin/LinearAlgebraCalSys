#include "tui_terminal.h"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
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
        newTermios.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
    }
#endif
}

// 读取一个字符
int Terminal::readChar() {
#ifdef _WIN32
    return _getch();
#else
    char c;
    read(STDIN_FILENO, &c, 1);
    return (int)c;
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
