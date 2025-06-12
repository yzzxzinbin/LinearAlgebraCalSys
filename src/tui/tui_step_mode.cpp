#include "tui_app.h"
#include "tui_terminal.h"
#include <iostream>
#include <string>
#include <sstream> // For std::stringstream in displayCurrentStep
#include "../utils/logger.h" // For LOG_WARNING

// 进入步骤展示模式 - 操作历史版本
void TuiApp::enterStepDisplayMode(const OperationHistory& history_param) { // Renamed parameter
    if (matrixEditor) {
        LOG_WARNING("Attempted to enter step display mode while editor is active. Exiting editor first.");
        // Potentially save/discard editor changes here or prompt user
        matrixEditor.reset(); // Forcibly exit editor
        initUI();
    }
    if (history_param.size() == 0) return;
    
    inStepDisplayMode = true;
    currentStep = 0;
    totalSteps = history_param.size();
    currentHistory = history_param; // Use the renamed parameter
    isExpansionHistory = false;
    
    // resultRow 此时指向命令和最终结果（如果有）之后的下一可用行
    this->stepDisplayStartRow = this->resultRow;
    
    // displayCurrentStep 会从 stepDisplayStartRow 开始绘制
    displayCurrentStep();
    drawStepProgressBar();
    
    statusMessage = "步骤导航模式: 使用←→箭头浏览步骤, ESC退出";
    drawStatusBar();
}

// 进入步骤展示模式 - 行列式展开版本
void TuiApp::enterStepDisplayMode(const ExpansionHistory& history_param) { // Renamed parameter
     if (matrixEditor) {
        LOG_WARNING("Attempted to enter step display mode while editor is active. Exiting editor first.");
        matrixEditor.reset(); 
        initUI();
    }

    if (history_param.size() == 0) return;
    
    inStepDisplayMode = true;
    currentStep = 0;
    totalSteps = history_param.size();
    currentExpHistory = history_param; // Use the renamed parameter
    isExpansionHistory = true;

    this->stepDisplayStartRow = this->resultRow;
        
    displayCurrentStep();
    drawStepProgressBar();
    
    statusMessage = "步骤导航模式: 使用←→箭头浏览步骤, ESC退出";
    drawStatusBar();
}

// 退出步骤展示模式
void TuiApp::exitStepDisplayMode() {
    inStepDisplayMode = false;
    
    // 清除结果区域
    clearResultArea();
    
    // 更新状态消息
    statusMessage = "已退出步骤导航模式";
    drawStatusBar();
}

// 显示当前步骤
void TuiApp::displayCurrentStep() {
    if (matrixEditor) return; // 不在编辑器模式下显示步骤
    // 清除旧的步骤内容区域 (从 stepDisplayStartRow 开始，直到进度条之前)
    // 进度条在 inputRow - 2, 进度条上的数字在 inputRow - 3
    // 所以步骤内容区域最多到 inputRow - 4
    int endClearRow = inputRow - 1; // 默认清除到输入行前一行
    if (totalSteps > 0) { // 如果有步骤，则有进度条
        endClearRow = inputRow - 3; // 清除到进度条数字的上一行
    }
    if (endClearRow < stepDisplayStartRow) endClearRow = stepDisplayStartRow; // 防止负范围

    for (int i = stepDisplayStartRow; i < endClearRow; i++) {
        if (i >= 0) { // Ensure row index is valid
            Terminal::setCursor(i, 0);
            std::string spaces(terminalCols, ' ');
            std::cout << spaces;
        }
    }
    
    // 在 stepDisplayStartRow 显示当前步骤信息 ("步骤 X / Y:")
    if (stepDisplayStartRow >=0) {
        Terminal::setCursor(stepDisplayStartRow, 0);
        Terminal::setForeground(Color::YELLOW);
        std::cout << "步骤 " << (currentStep + 1) << " / " << totalSteps << ":" << std::endl;
        Terminal::resetColor();
    }
        
    // 在下一行 (stepDisplayStartRow + 1) 开始显示步骤内容
    // 确保有足够的空间显示步骤内容
    if (stepDisplayStartRow + 1 < endClearRow && stepDisplayStartRow + 1 >= 0) {
        Terminal::setCursor(stepDisplayStartRow + 1, 0); 
        Terminal::setForeground(Color::CYAN);
        // 捕获步骤打印的输出，以便正确更新光标和处理多行
        std::stringstream step_ss;
        if (isExpansionHistory) {
            currentExpHistory.getStep(currentStep).print(step_ss);
        } else {
            currentHistory.getStep(currentStep).print(step_ss);
        }
        
        std::string step_line;
        int current_print_row = stepDisplayStartRow + 1;
        std::istringstream step_iss(step_ss.str());
        while(std::getline(step_iss, step_line) && current_print_row < endClearRow && current_print_row >= 0) {
            Terminal::setCursor(current_print_row++, 0);
            std::cout << step_line << std::endl;
        }
        Terminal::resetColor();
    }
}

// 绘制步骤进度条
void TuiApp::drawStepProgressBar() {
    if (matrixEditor) return; // 不在编辑器模式下绘制进度条
    // 计算进度条位置
    int barRow = inputRow - 2;
    if (barRow < 0 || barRow -1 < 0) return; // Not enough space

    int barWidth = terminalCols - 10; // 留出左右边距
    if (barWidth < 1) barWidth = 1; // Minimum bar width
    int barStart = 5;
    
    // 清除进度条行
    Terminal::setCursor(barRow, 0);
    std::string spaces(terminalCols, ' ');
    std::cout << spaces;
    
    // 绘制进度条轨道
    Terminal::setCursor(barRow, barStart);
    Terminal::setForeground(Color::WHITE);
    std::cout << "[";
    for (int i = 0; i < barWidth; i++) {
        std::cout << "-";
    }
    std::cout << "]";

    // 计算当前步骤指示器位置
    int indicatorPos = barStart + 1; // Default to start if barWidth is 0 or totalSteps <=1
    if (totalSteps > 1 && barWidth > 0) { // Ensure barWidth is positive before division
        indicatorPos += static_cast<int>((static_cast<double>(currentStep) / (totalSteps - 1)) * (barWidth -1)); // barWidth-1 because indicator takes 1 char
    }
    if(indicatorPos >= barStart + 1 + barWidth) indicatorPos = barStart + barWidth; // Cap at end
    
    // 绘制当前步骤指示器
    Terminal::setCursor(barRow, indicatorPos);
    Terminal::setForeground(Color::GREEN);
    std::cout << "◆";
    
    // 在进度条上方显示步骤号
    Terminal::setCursor(barRow - 1, indicatorPos > 0 ? indicatorPos - 1 : 0); // Ensure cursor pos is valid
    std::cout << currentStep + 1;
    
    Terminal::resetColor();
}
