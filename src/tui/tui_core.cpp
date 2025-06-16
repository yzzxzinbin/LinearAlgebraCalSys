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

    std::string title = " 线性代数计算系统 v1.1 ";
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
void TuiApp::run() {
    initUI(); // 初始化UI，绘制初始界面

    // 如果有初始命令，则在主循环开始前执行它
    if (!initialCommandToExecute.empty()) {
        LOG_INFO("执行来自启动界面的初始命令: " + initialCommandToExecute);
        
        // 将命令显示在结果区域（模拟用户输入）
        // 注意：如果命令本身会清屏 (如 clear)，这行输出会被清除
        // printToResultView("> " + initialCommandToExecute); 
        
        // 直接执行命令
        // executeCommand 会处理输出和状态更新
        currentInput = initialCommandToExecute; // 放入 currentInput 以便 executeCommand 使用
        executeCommand(currentInput);           // executeCommand 内部会清空 currentInput

        // executeCommand 内部通常不直接将会话历史记录，除非是有效命令
        // 这里我们确保它被记录，如果它是一个有效的命令字符串
        if (!initialCommandToExecute.empty()) { // 再次检查，因为 executeCommand 可能修改
            history.push_front(initialCommandToExecute);
            if (history.size() > MAX_HISTORY) {
                history.pop_back();
            }
            historyIndex = 0; // 重置历史记录导航索引
        }
        
        initialCommandToExecute.clear(); // 清除初始命令，避免重复执行
        // currentInput 已经被 executeCommand 清除
        cursorPosition = 0;
        
        updateUI(); // 执行命令后刷新整个UI以显示结果
    }


    while (running) {
        handleInput(); // 处理用户输入
        if (running) { // handleInput 可能会将 running 设置为 false (例如输入exit)
            updateUI();    // 根据当前状态更新并重绘UI
        }
        // 可以考虑加入短暂延时以降低CPU占用，但通常输入处理是阻塞的
        // std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
    }

    // 清理工作
    Terminal::clear();
    Terminal::setRawMode(false); // 确保恢复终端的原始模式
    Terminal::resetColor();      // 重置终端颜色
    Terminal::setCursor(0, 0);   // 将光标移到左上角
    std::cout << "感谢使用！再见！" << std::endl; // 退出信息
    std::cout << std::flush;
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
