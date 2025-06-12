#include "tui_app.h"
#include "tui_terminal.h"
#include <iostream>
#include <string>
#include <algorithm> // For std::string::resize in drawStatusBar
#include <memory>    // For std::make_unique in updateUI

void TuiApp::drawHeader()
{
    Terminal::setCursor(0, 0);
    Terminal::setForeground(Color::CYAN);
    Terminal::setBackground(Color::BLUE);

    std::string title = " 线性代数计算系统 v1.0 ";
    int padding = (terminalCols - title.length()) / 2;

    std::string header(terminalCols, ' ');
    for (size_t i = 0; i < title.length(); i++)
    {
        header[padding + i] = title[i];
    }

    std::cout << header;
    Terminal::resetColor();
    std::cout << std::endl;
}

void TuiApp::drawStatusBar()
{
    Terminal::setCursor(terminalRows - 1, 0);
    Terminal::setForeground(Color::BLACK);
    Terminal::setBackground(Color::WHITE);

    // 状态栏信息
    std::string status = " " + statusMessage;
    status.resize(terminalCols, ' ');

    std::cout << status;
    Terminal::resetColor();
}

void TuiApp::initUI()
{
    Terminal::clear();
    drawHeader();
    // 如果编辑器激活，不绘制标准输入提示和结果区，由编辑器负责
    if (!matrixEditor) {
        drawInputPrompt(); // drawInputPrompt 内部会处理 suggestionBox 的绘制
        drawResultArea(); 
    }
    drawStatusBar();
}

void TuiApp::updateUI()
{
    // 检查终端大小是否变化
    auto [rows, cols] = Terminal::getSize();
    if (rows != terminalRows || cols != terminalCols)
    {
        terminalRows = rows;
        terminalCols = cols;
        inputRow = terminalRows - 2;
        // 如果终端大小改变，可能需要重新创建或更新 suggestionBox 的宽度
        suggestionBox = std::make_unique<SuggestionBox>(terminalCols);
        initUI();
    }

    // 更新输入提示行 (仅当编辑器未激活时)
    if (!matrixEditor) {
        drawInputPrompt(); // drawInputPrompt 内部会处理 suggestionBox 的绘制
    }
    // 状态栏总是更新 (或者由编辑器更新自己的状态消息)
    // drawStatusBar(); // 已移至run循环末尾
}

void TuiApp::run()
{
    // 初始化UI
    initUI();

    // 进入原始模式，以便直接读取按键
    Terminal::setRawMode(true);

    // 主循环
    while (running)
    {
        // 更新UI
        if (matrixEditor) { // 如果增强型编辑器激活
            matrixEditor->draw(); 
            // 状态栏由编辑器或TuiApp更新
            // drawStatusBar(); // 确保状态栏在编辑器绘制后更新，如果编辑器不自己画的话
        } else {
            updateUI(); // updateUI 会调用 drawInputPrompt, drawInputPrompt 会调用 suggestionBox->draw
        }
        drawStatusBar(); // 总是绘制状态栏，确保它在最下面且最新
        
        // 确保UI更新立即显示
        std::cout.flush();

        // 处理输入
        handleInput();
    }

    // 恢复终端状态
    Terminal::setRawMode(false);
    Terminal::resetColor();
    Terminal::clear();
    Terminal::setCursor(0, 0);
}

void TuiApp::clearResultArea()
{
    if (matrixEditor) return; // 编辑器激活时，主结果区可能不需要清除或由编辑器管理
    // 清空结果区域的实际内容部分
    for (int i = RESULT_AREA_CONTENT_START_ROW; i < inputRow; i++)
    {
        Terminal::setCursor(i, 0);
        std::string spaces(terminalCols, ' ');
        std::cout << spaces;
    }

    // 重新绘制结果区域标题
    Terminal::setCursor(RESULT_AREA_TITLE_ROW, 0);
    Terminal::setForeground(Color::YELLOW);
    std::cout << "输出区域:" << std::endl;
    Terminal::resetColor();

    // 重置 TuiApp::resultRow 成员变量，以便下一次打印从内容区域的顶部开始
    this->resultRow = RESULT_AREA_CONTENT_START_ROW;
}

void TuiApp::drawResultArea()
{
    if (matrixEditor) return; // 编辑器激活时不绘制主结果区
    clearResultArea(); // 这会重置 resultRow
}
