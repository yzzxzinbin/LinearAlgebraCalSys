#include "tui_terminal.h"
#include <iostream>
#include <cstdio>
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

// 清屏
void Terminal::clear() {
#ifdef _WIN32
    // Windows 清屏
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    DWORD cellCount;
    COORD homeCoords = { 0, 0 };

    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        cellCount = csbi.dwSize.X * csbi.dwSize.Y;
        FillConsoleOutputCharacter(hConsole, ' ', cellCount, homeCoords, &count);
        FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count);
        SetConsoleCursorPosition(hConsole, homeCoords);
    }
#else
    // UNIX-like 系统使用 ANSI 转义序列
    std::cout << "\033[2J\033[H";
#endif
}

// 设置光标位置
void Terminal::setCursor(int row, int col) {
#ifdef _WIN32
    // Windows 设置光标位置
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord;
    coord.X = col;
    coord.Y = row;
    SetConsoleCursorPosition(hConsole, coord);
#else
    // UNIX-like 系统使用 ANSI 转义序列
    std::cout << "\033[" << row + 1 << ";" << col + 1 << "H";
#endif
}

// 保存光标位置
void Terminal::saveCursor() {
#ifdef _WIN32
    // Windows 不直接支持此功能，可以自行实现一个全局变量保存位置
    // 这里简化处理，仅在 UNIX-like 系统实现
#else
    // UNIX-like 系统使用 ANSI 转义序列
    std::cout << "\033[s";
#endif
}

// 恢复光标位置
void Terminal::restoreCursor() {
#ifdef _WIN32
    // Windows 不直接支持此功能，可以自行实现一个全局变量恢复位置
    // 这里简化处理，仅在 UNIX-like 系统实现
#else
    // UNIX-like 系统使用 ANSI 转义序列
    std::cout << "\033[u";
#endif
}

// 设置前景色
void Terminal::setForeground(Color color) {
#ifdef _WIN32
    // Windows 设置前景色
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD wAttributes;
    
    switch (color) {
        case Color::BLACK:      wAttributes = 0; break;
        case Color::RED:        wAttributes = FOREGROUND_RED; break;
        case Color::GREEN:      wAttributes = FOREGROUND_GREEN; break;
        case Color::YELLOW:     wAttributes = FOREGROUND_RED | FOREGROUND_GREEN; break;
        case Color::BLUE:       wAttributes = FOREGROUND_BLUE; break;
        case Color::MAGENTA:    wAttributes = FOREGROUND_RED | FOREGROUND_BLUE; break;
        case Color::CYAN:       wAttributes = FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        case Color::WHITE:      wAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        case Color::DEFAULT:    wAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
    }
    
    SetConsoleTextAttribute(hConsole, wAttributes);
#else
    // UNIX-like 系统使用 ANSI 转义序列
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
#endif
}

// 设置背景色
void Terminal::setBackground(Color color) {
#ifdef _WIN32
    // Windows 设置背景色
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD wAttributes;
    
    switch (color) {
        case Color::BLACK:      wAttributes = 0; break;
        case Color::RED:        wAttributes = BACKGROUND_RED; break;
        case Color::GREEN:      wAttributes = BACKGROUND_GREEN; break;
        case Color::YELLOW:     wAttributes = BACKGROUND_RED | BACKGROUND_GREEN; break;
        case Color::BLUE:       wAttributes = BACKGROUND_BLUE; break;
        case Color::MAGENTA:    wAttributes = BACKGROUND_RED | BACKGROUND_BLUE; break;
        case Color::CYAN:       wAttributes = BACKGROUND_GREEN | BACKGROUND_BLUE; break;
        case Color::WHITE:      wAttributes = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE; break;
        case Color::DEFAULT:    wAttributes = 0; break;
    }
    
    SetConsoleTextAttribute(hConsole, wAttributes);
#else
    // UNIX-like 系统使用 ANSI 转义序列
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
#endif
}

// 重置颜色
void Terminal::resetColor() {
#ifdef _WIN32
    // Windows 重置颜色
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    // UNIX-like 系统使用 ANSI 转义序列
    std::cout << "\033[0m";
#endif
}

// 获取终端大小
std::pair<int, int> Terminal::getSize() {
    int rows = 24;  // 默认值
    int cols = 80;  // 默认值
    
#ifdef _WIN32
    // Windows 获取终端大小
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#else
    // UNIX-like 系统获取终端大小
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
    // Windows 不需要特殊处理，使用 _getch() 就可以无需回车读取字符
#else
    // UNIX-like 系统设置终端模式
    static struct termios oldTermios, newTermios;
    
    if (enable) {
        // 获取当前终端设置
        tcgetattr(STDIN_FILENO, &oldTermios);
        newTermios = oldTermios;
        
        // 禁用规范模式和回显
        newTermios.c_lflag &= ~(ICANON | ECHO);
        
        // 设置新的终端设置
        tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
    } else {
        // 恢复原始设置
        tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
    }
#endif
}

// 读取一个字符
int Terminal::readChar() {
#ifdef _WIN32
    // Windows 使用 _getch() 读取字符
    return _getch();
#else
    // UNIX-like 系统读取字符
    char c;
    read(STDIN_FILENO, &c, 1);
    return (int)c;
#endif
}

// 检查是否有输入可用
bool Terminal::hasInput() {
#ifdef _WIN32
    // Windows 检查是否有键盘输入
    return _kbhit() != 0;
#else
    // UNIX-like 系统检查是否有输入可用
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
#endif
}
