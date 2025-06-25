#include "tui_app.h"
#include "tui_terminal.h"
#include "../utils/tui_utils.h" // 新增：包含TUI工具函数
#include <iostream>
#include <string>
#include <algorithm> // For std::string::resize in drawStatusBar
#include <memory>    // For std::make_unique in updateUI

void TuiApp::drawHeader()
{
    Terminal::setCursor(0, 0);
    Terminal::setForeground(Color::CYAN);
    Terminal::setBackground(Color::BLUE);

    std::string title = "线性代数计算系统 v1.3";
    
    // 使用UTF-8视觉宽度计算来正确处理中文字符
    size_t titleVisualWidth = TuiUtils::calculateUtf8VisualWidth(title);
    int padding = (terminalCols - static_cast<int>(titleVisualWidth)) / 2;
    if (padding < 0) padding = 0; // 防止负数

    // 构建完整的header行
    std::string header;
    header.reserve(terminalCols * 3); // 预留足够空间处理UTF-8字符
    
    // 添加左侧填充
    header.append(padding, ' ');
    
    // 添加标题（如果放得下的话）
    if (padding + static_cast<int>(titleVisualWidth) <= terminalCols) {
        header += title;
    }
    
    // 计算已使用的视觉宽度
    size_t usedVisualWidth = TuiUtils::calculateUtf8VisualWidth(header);
    
    // 添加右侧填充以确保占满整行
    if (usedVisualWidth < static_cast<size_t>(terminalCols)) {
        header.append(terminalCols - usedVisualWidth, ' ');
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

    // 使用UTF-8视觉宽度计算来正确处理中文字符
    size_t statusVisualWidth = TuiUtils::calculateUtf8VisualWidth(status);

    // 如果状态栏信息超过终端宽度，则截断
    if (statusVisualWidth > static_cast<size_t>(terminalCols)) {
        status = TuiUtils::trimToUtf8VisualWidth(status, terminalCols);
        statusVisualWidth = TuiUtils::calculateUtf8VisualWidth(status); // 重新计算截断后的宽度
    }

    // 添加右侧填充以确保占满整行
    if (statusVisualWidth < static_cast<size_t>(terminalCols))
    {
        status.append(terminalCols - statusVisualWidth, ' ');
    }

    std::cout << status;
    Terminal::resetColor();
}

void TuiApp::initUI()
{
    Terminal::clear();
    drawHeader();
    // 如果编辑器或预览器激活，不绘制标准输入提示和结果区，由编辑器/预览器负责
    if (!matrixEditor && !variableViewer && !helpViewer)
    {
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

        // 新增：如果编辑器或预览器处于活动状态，则更新其尺寸
        if (matrixEditor) {
            matrixEditor->updateDimensions(terminalRows, terminalCols);
        }
        if (variableViewer) {
            variableViewer->updateDimensions(terminalRows, terminalCols);
        }
        if (helpViewer) {
            helpViewer->updateDimensions(terminalRows, terminalCols);
        }
        
        initUI();
    }

    // 更新输入提示行 (仅当编辑器和预览器未激活时)
    if (!matrixEditor && !variableViewer && !helpViewer)
    {
        drawInputPrompt(); // drawInputPrompt 内部会处理 suggestionBox 的绘制
    }
    // 状态栏总是更新 (或者由编辑器/预览器更新自己的状态消息)
    drawStatusBar(); // 已移至run循环末尾
}

void TuiApp::run()
{
    initUI(); // 初始化UI，绘制初始界面

    // 如果有初始命令，则在主循环开始前执行它
    if (!initialCommandToExecute.empty())
    {
        LOG_INFO("执行来自启动界面的初始命令: " + initialCommandToExecute);

        // 直接执行命令
        // executeCommand 会处理输出和状态更新
        executeCommand(initialCommandToExecute);
        initialCommandToExecute.clear(); // 清除初始命令，避免重复执行
        cursorPosition = 0;

        updateUI(); // 执行命令后刷新整个UI以显示结果
    }

    // 进入原始模式，以便直接读取按键
    Terminal::setRawMode(true);

    // 主循环
    while (running)
    {
        // 更新UI
        if (matrixEditor)
        { // 如果增强型编辑器激活
            updateUI(); // 确保编辑器绘制前更新UI
            matrixEditor->draw();
            // 状态栏由编辑器或TuiApp更新
        }
        else if (variableViewer)
        { // 如果变量预览器激活
            updateUI(); // 确保预览器绘制前更新UI
            variableViewer->draw();
            // 状态栏由预览器或TuiApp更新
        }else if (helpViewer)
        { // 如果帮助查看器激活
            updateUI(); // 确保帮助查看器绘制前更新UI
            helpViewer->draw();
            // 状态栏由帮助查看器或TuiApp更新
        }
        else
        {
            updateUI(); // updateUI 会调用 drawInputPrompt, drawInputPrompt 会调用 suggestionBox->draw
        }
        
        drawStatusBar(); // 总是绘制状态栏，确保它在最下面且最新

        // 确保UI更新立即显示
        std::cout.flush();

        // 处理输入
        handleInput();
    }

    // 清理工作
    Terminal::clear();
    Terminal::setRawMode(false);                  // 确保恢复终端的原始模式
    Terminal::resetColor();                       // 重置终端颜色
    Terminal::setCursor(0, 0);                    // 将光标移到左上角
    std::cout << "感谢使用！再见！" << std::endl; // 退出信息
    std::cout << std::flush;
}

void TuiApp::clearResultArea()
{
    if (matrixEditor)
        return; // 编辑器激活时，主结果区可能不需要清除或由编辑器管理
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
    if (matrixEditor)
        return;        // 编辑器激活时不绘制主结果区
    clearResultArea(); // 这会重置 resultRow
}
