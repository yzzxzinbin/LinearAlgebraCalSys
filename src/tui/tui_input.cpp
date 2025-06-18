#include "tui_app.h"
#include "tui_terminal.h"
#include <iostream>
#include <string>
#include <algorithm> // For std::string::erase, etc.
#include <cctype>    // For std::isspace
#include "../utils/logger.h" // For LOG_DEBUG in handleInput
#include "enhanced_matrix_editor.h" 
#include "tui_suggestion_box.h" 

void TuiApp::handleInput()
{
    // 读取一个字符
    int key = Terminal::readChar();
    
    // 将 suggestionBoxWasVisible 的声明移到函数开头
    bool suggestionBoxWasVisible = suggestionBox->isVisible();

    // 如果增强型编辑器激活，将输入传递给它
    if (matrixEditor) {
        EnhancedMatrixEditor::EditorResult result = matrixEditor->handleInput(key);
        statusMessage = matrixEditor->getStatusMessage(); // 获取编辑器的状态消息

        if (result == EnhancedMatrixEditor::EditorResult::EXIT_SAVE) {
            interpreter.getVariablesNonConst()[matrixEditor->getVariableName()] = matrixEditor->getEditedVariableCopy();
            statusMessage = "数据更改已生效于 " + matrixEditor->getVariableName() ;
            matrixEditor.reset(); 
            initUI(); // 重绘标准UI
        } else if (result == EnhancedMatrixEditor::EditorResult::EXIT_DISCARD) {
            // 当前 EXIT_DISCARD 未实现，ESC 默认为 EXIT_SAVE
            statusMessage = "已退出 " + matrixEditor->getVariableName() + " 的编辑器";
            matrixEditor.reset();
            initUI();
        }
        // 如果是 CONTINUE 或 UPDATE_STATUS，状态栏会在 run() 循环中重绘
        return;
    }
    
    // 如果变量预览器激活，将输入传递给它
    if (variableViewer) {
        EnhancedVariableViewer::ViewerResult result = variableViewer->handleInput(key);
        statusMessage = variableViewer->getStatusMessage(); // 获取预览器的状态消息

        if (result == EnhancedVariableViewer::ViewerResult::EXIT) {
            statusMessage = "已退出变量预览器";
            variableViewer.reset();
            initUI(); // 重绘标准UI
        }
        // 如果是 CONTINUE，状态栏会在 run() 循环中重绘
        return;
    }
    
    // 优先处理候选框输入
    if (suggestionBox->isVisible()) {
        SuggestionAction action = suggestionBox->handleKey(key);
        if (action == SuggestionAction::APPLY_SUGGESTION) {
            std::string selectedText = suggestionBox->getSelectedSuggestion().text;
            std::string prefixToReplace = suggestionBox->getCurrentInputPrefix();
            size_t wordStartPos = currentInput.rfind(prefixToReplace, cursorPosition - prefixToReplace.length());
            
            if (wordStartPos != std::string::npos && wordStartPos + prefixToReplace.length() == cursorPosition) {
                 currentInput.replace(wordStartPos, prefixToReplace.length(), selectedText);
                 cursorPosition = wordStartPos + selectedText.length();
            } else { 
                size_t originalCursor = cursorPosition;
                currentInput.insert(cursorPosition, selectedText);
                cursorPosition += selectedText.length();
                if (originalCursor >= prefixToReplace.length() && 
                    currentInput.substr(originalCursor - prefixToReplace.length(), prefixToReplace.length()) == prefixToReplace) {
                    currentInput.erase(originalCursor - prefixToReplace.length(), prefixToReplace.length());
                    cursorPosition -= prefixToReplace.length();
                }
            }
            suggestionBox->hide();
            clearSuggestionArea(); // 调用 clearSuggestionArea
            drawInputPrompt();     
            return; // 修改：移除对key的检查，Tab键和Enter键不再冲突
        } else if (action == SuggestionAction::CLOSE_BOX) {
            suggestionBox->hide();
            clearSuggestionArea(); // 调用 clearSuggestionArea
            drawInputPrompt();
            return;
        } else if (action == SuggestionAction::NAVIGATION) {
            drawInputPrompt(); 
            return;
        }
    }
    
    // 检查是否是退格键(包括Linux的127) - 仅当编辑器未激活
    if (key == KEY_BACKSPACE) {
        // 删除光标前一个字符
        if (cursorPosition > 0 && !currentInput.empty())
        {
            currentInput.erase(cursorPosition - 1, 1);
            cursorPosition--;
            // drawInputPrompt(); // Defer drawing until after suggestion update
        } else { // Backspace at start of empty line
             return;
        }
    } else if (inStepDisplayMode)
    {
        if (key == KEY_ESCAPE)
        {
            // ESC键退出步骤导航模式
            exitStepDisplayMode();
            return;
        }
        else if (key == KEY_LEFT || (key == 27 && Terminal::hasInput() && Terminal::readChar() == '[' && Terminal::readChar() == 'D'))
        {
            // 左箭头，显示上一步
            if (currentStep > 0)
            {
                currentStep--;
                displayCurrentStep();
                drawStepProgressBar();
            }
            return;
        }
        else if (key == KEY_RIGHT || (key == 27 && Terminal::hasInput() && Terminal::readChar() == '[' && Terminal::readChar() == 'C'))
        {
            // 右箭头，显示下一步
            if (currentStep < totalSteps - 1)
            {
                currentStep++;
                displayCurrentStep();
                drawStepProgressBar();
            }
            return;
        }
        // 在步骤显示模式下忽略其他键
        return;
    }

    // 处理特殊键
    if (key == KEY_ESCAPE)
    {
        // 检查是否是转义序列
        if (Terminal::hasInput())
        {
            int next = Terminal::readChar();
            if (next == '[')
            {
                // 这是一个ANSI转义序列
                int code = Terminal::readChar();
                switch (code)
                {
                case 'A': // 上箭头
                    handleSpecialKey(KEY_UP);
                    break; 
                case 'B': // 下箭头
                    handleSpecialKey(KEY_DOWN);
                    break; 
                case 'C': // 右箭头
                    handleSpecialKey(KEY_RIGHT);
                    break; 
                case 'D': // 左箭头
                    handleSpecialKey(KEY_LEFT);
                    break; 
                // 根据 tui_terminal.cpp 的 readChar 实现，Delete 键可能不会通过这里
                // 它可能直接返回一个特定的 KEY_DELETE 值（如果定义了）
                // 或者需要更复杂的序列检测
                }
            }
        }
        else
        {
            // ESC键
            if (suggestionBox->isVisible()) { // 如果候选框可见，ESC优先关闭候选框
                suggestionBox->hide();
                clearSuggestionArea(); // 调用 clearSuggestionArea
                // drawInputPrompt(); // Defer drawing
            } else if (currentInput.empty())
            {
                running = false; // 如果输入为空，退出程序
            }
            else
            {
                currentInput.clear(); // 否则清空当前输入
                cursorPosition = 0;
                // drawInputPrompt(); // Defer
            }
        }
    } else if (key == KEY_ENTER) {
        // Enter键现在只用于执行命令，不再用于选择候选词
        // 如果候选框可见，先隐藏它
        if (suggestionBox->isVisible()){ 
            suggestionBox->hide();
            clearSuggestionArea(); // 调用 clearSuggestionArea
        }
        if (!currentInput.empty())
        {
            executeCommand(currentInput);

            // 将命令添加到历史记录
            if (history.empty() || history.front() != currentInput)
            {
                history.push_front(currentInput);
                if (history.size() > MAX_HISTORY)
                {
                    history.pop_back();
                }
            }
            
            if (!tempInputBuffer.empty()) { // 清除历史导航时的临时缓冲
                tempInputBuffer.clear();
            }

            historyIndex = 0;
            currentInput.clear();
            cursorPosition = 0;
            // drawInputPrompt(); // Defer
        }
    } else if (key >= 32 && key <= 126) { // 可打印字符
        if (key == '(') {
            // 括号自动补全：当输入(时，自动添加一对括号并将光标放在中间
            currentInput.insert(cursorPosition, "()");
            cursorPosition++; // 只移动一位，使光标位于()之间
        } else {
            // 正常处理其他可打印字符
            currentInput.insert(cursorPosition, 1, static_cast<char>(key));
            cursorPosition++;
        }
        
        if (historyIndex != 0) {
            historyIndex = 0; 
        }
        // drawInputPrompt(); // Defer
    } else { // 处理其他特殊键，如箭头键
        // 如果候选框不可见，才允许历史导航等
        if (!suggestionBoxWasVisible) { // Use suggestionBoxWasVisible to avoid conflict if ESC closed the box
             handleSpecialKey(key); // handleSpecialKey calls drawInputPrompt
        } else {
            // If suggestion box was visible but didn't handle the key (e.g. left/right arrow)
            // and it wasn't a char key, it might be a command for TuiApp itself.
            // For now, if suggestion box was visible, assume it or the char handler takes precedence.
            // This 'else' branch for special keys might need refinement if specific non-char,
            // non-suggestion-box keys need to work while box is visible.
            // However, up/down/enter/esc are handled by suggestion box.
            // Left/right should affect cursorPosition, then suggestions update.
            if (key == KEY_LEFT || key == KEY_RIGHT) {
                 handleSpecialKey(key); // This will move cursor and call drawInputPrompt
            }
        }
    }

    // 更新候选词 (在输入或删除后)
    // 但不在Enter后，因为命令已执行，输入已清空
    if (key != KEY_ENTER) {
        size_t currentWordStartPos = 0; // Relative to currentInput string
        std::string word_prefix = getCurrentWordForSuggestion(currentWordStartPos);
        if (!word_prefix.empty()) {
            // 先清除旧的候选框区域，然后再更新显示新的候选词
            clearSuggestionArea(); 
            suggestionBox->updateSuggestions(word_prefix, getVariableNames(), KNOWN_FUNCTIONS, KNOWN_COMMANDS);
        } else {
            if (suggestionBox->isVisible()) { // If no prefix but box was visible, hide it
                suggestionBox->hide();
                clearSuggestionArea(); // 调用 clearSuggestionArea
            }
        }
    } else { // After Enter, ensure box is hidden
        if (suggestionBox->isVisible()) {
            suggestionBox->hide();
            clearSuggestionArea(); // 调用 clearSuggestionArea
        }
    }
    
    // 统一在末尾绘制输入提示，确保所有状态更新后UI正确显示
    drawInputPrompt();
    std::cout.flush();
}

void TuiApp::handleSpecialKey(int key)
{
    if (matrixEditor) return; // 编辑器处理自己的特殊键
    switch (key)
    {
    case KEY_UP:
        navigateHistory(true);
        break;
    case KEY_DOWN:
        navigateHistory(false);
        break;
    case KEY_LEFT:
        if (cursorPosition > 0) {
            cursorPosition--;
            drawInputPrompt();
        }
        break;
    case KEY_RIGHT:
        if (cursorPosition < currentInput.length()) {
            cursorPosition++;
            drawInputPrompt();
        }
        break;
    // case KEY_DELETE: // 如果 readChar 返回 KEY_DELETE
    //     if (cursorPosition < currentInput.length()) {
    //         currentInput.erase(cursorPosition, 1);
    //         drawInputPrompt();
    //     }
    //     break;
    }
}

void TuiApp::navigateHistory(bool up)
{
    if (matrixEditor) return; // 不在编辑器模式下导航历史
    if (up) { // 向上浏览历史
        if (history.empty()) {
            return;
        }
        if (historyIndex == 0) { // 第一次按上箭头，或从最新输入向上
            tempInputBuffer = currentInput; // 保存当前输入行
        }
        if (historyIndex < history.size()) {
            historyIndex++;
            currentInput = history[historyIndex - 1];
            cursorPosition = currentInput.length(); // 光标移到末尾
        }
    } else { // 向下浏览历史
        if (historyIndex > 1) {
            historyIndex--;
            currentInput = history[historyIndex - 1];
            cursorPosition = currentInput.length(); // 光标移到末尾
        } else if (historyIndex == 1) { // 到达历史记录的“底部”，恢复之前暂存的输入
            historyIndex = 0;
            currentInput = tempInputBuffer;
            // tempInputBuffer.clear(); // 不立即清除，以便再次向上时能恢复
            cursorPosition = currentInput.length(); // 光标移到末尾
        }
    }
    drawInputPrompt();
}

void TuiApp::drawInputPrompt()
{
    if (matrixEditor) return; 

    // 先绘制候选框 (如果可见)
    // 候选框的绘制位置需要基于当前光标正在编辑的词的起始位置
    if (suggestionBox->isVisible()) {
        size_t currentWordStartPosInString = 0;
        getCurrentWordForSuggestion(currentWordStartPosInString); // Get the start index of the word in currentInput
        // The column for suggestionBox->draw should be where this word starts on screen.
        // currentWordStartPosInInput is the offset from "> "
        suggestionBox->draw(inputRow, 2, currentWordStartPosInString);
    }


    Terminal::setCursor(inputRow, 0);
    Terminal::setForeground(Color::GREEN);
    std::cout << "> ";

    // 清除旧的输入行内容
    std::string spaces(terminalCols - 2, ' '); // -2 for "> "
    std::cout << spaces;

    // 重新定位光标以打印输入和模拟光标
    Terminal::setCursor(inputRow, 2);

    // 打印光标前的部分
    std::cout << currentInput.substr(0, cursorPosition);

    // 模拟光标：反转颜色打印光标下的字符，或者打印特殊字符
    // 保存当前颜色状态
    // (如果 Terminal 类支持获取当前颜色会更好，这里假设默认是绿前景白背景)
    
    Terminal::setBackground(Color::WHITE); // 设置光标背景色
    Terminal::setForeground(Color::BLACK); // 设置光标前景色

    if (cursorPosition < currentInput.length()) {
        std::cout << currentInput[cursorPosition];
                } else {
        std::cout << " "; // 如果光标在末尾，显示一个空格作为光标块
    }
    
    Terminal::resetColor(); // 重置颜色到终端默认
    Terminal::setForeground(Color::GREEN); // 重新应用绿色前景以打印光标后的文本

    // 打印光标后的部分
    if (cursorPosition < currentInput.length()) {
        std::cout << currentInput.substr(cursorPosition + 1);
    }

    // 将真实的终端光标定位到我们模拟光标的逻辑位置之后
    // 这样，如果终端本身也显示光标，它不会与我们的模拟光标重叠或错位
    Terminal::setCursor(inputRow, 2 + cursorPosition +1);
}

void TuiApp::clearSuggestionArea() {
    // 确保此函数定义存在且在 TuiApp 类的作用域内
    // 如果 suggestionBox 本身不可见，或者它自己管理清除，这里可能不需要做太多
    // 但为了安全，在它隐藏后，我们确保它占用的区域被重绘为背景
    
    // 假设候选框最多5行，绘制在 inputRow 之上
    // 这个函数主要在 suggestionBox->hide() 之后，确保屏幕干净
    int maxSuggestionLines = 5; // 与 SuggestionBox 的 maxDisplayItems 对应
    int startClearRow = inputRow - maxSuggestionLines;
    if (startClearRow < 0) startClearRow = 0;

    // 只清除到输入行之前，避免清除输入行本身或状态栏
    for (int i = startClearRow; i < inputRow; ++i) {
        if (i >= 0) { // 确保行号有效
            Terminal::setCursor(i, 0);
            std::string spaces(terminalCols, ' ');
            std::cout << spaces;
        }
    }
}

std::string TuiApp::getCurrentWordForSuggestion(size_t& wordStartPosInInput) const {
    wordStartPosInInput = 0; // Default to 0
    if (currentInput.empty() || cursorPosition == 0) {
        return "";
    }

    size_t endPos = cursorPosition;
        // 如果光标前的字符是空格，则不认为正在输入一个可建议的单词的中间或末尾
        // (除非这是为了开始一个新词，但那时 prefix 为空，由 updateSuggestions 处理)
        // 如果光标前的字符是空格或左括号，则不认为正在输入一个可建议的单词的中间或末尾
    if (std::isspace(currentInput[endPos - 1]) || currentInput[endPos - 1] == '(') { 
        return "";
    }

    size_t startPos = endPos;
    while (startPos > 0 && !std::isspace(currentInput[startPos - 1]) && currentInput[startPos - 1] != '(') {
        startPos--;
    }
    
    // If startPos is still == endPos here, it means cursor was at pos 0 or after a space.
    // But we checked for currentInput[endPos-1] not being a space.
    // So, startPos should be < endPos.
    
    wordStartPosInInput = startPos; // Store the start position of the word in the input string
    return currentInput.substr(startPos, endPos - startPos);
}
