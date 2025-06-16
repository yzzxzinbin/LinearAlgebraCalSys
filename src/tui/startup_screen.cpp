#include "startup_screen.h"
#include "../utils/logger.h" 
#include <iostream>          // 用于 std::cout (TuiUtils间接使用)
#include <algorithm>         // 用于 std::min/max
#include <filesystem>        // 新增：确保包含filesystem头文件

// 新增：定义命名空间别名
namespace fs = std::filesystem;

// 定义 NULL 选项的文本
const std::string StartupScreen::NULL_WORKSPACE_OPTION_TEXT = "<None (Start New)>";

StartupScreen::StartupScreen(const std::string& bannerFilePath, const std::string& workDirPath)
    : currentSelection_(0), scrollOffset_(0), active_(true),
      bannerFileConfigPath_(bannerFilePath), workDirConfigPath_(workDirPath) {
    
    // 将传入的工作目录路径转换为绝对路径，以确保一致性
    // 如果已经是绝对路径，fs::absolute 不会改变它
    // 如果是相对路径，它会相对于当前工作目录进行转换
    try {
        workDirectoryPath_ = fs::absolute(workDirConfigPath_).string();
    } catch (const fs::filesystem_error& e) { // 确保 catch 使用 std::filesystem::filesystem_error 或 fs::filesystem_error
        LOG_ERROR("无法获取工作目录的绝对路径: " + std::string(e.what()));
        workDirectoryPath_ = workDirConfigPath_; // Fallback to original path
    }
    
    // Banner路径也可能需要处理，但这里假设它相对于可执行文件或项目结构是固定的
    loadBanner(bannerFileConfigPath_);
    loadFiles(workDirectoryPath_); // loadFiles 会添加 NULL_WORKSPACE_OPTION_TEXT

    // fileList_ 至少会包含 NULL_WORKSPACE_OPTION_TEXT
    // 默认选中第一个条目 (即 NULL_WORKSPACE_OPTION_TEXT)
    currentSelection_ = 0; 
}

void StartupScreen::loadBanner(const std::string& filePath) {
    bannerLines_ = TuiUtils::readFileLines(filePath);
    if (bannerLines_.empty()) {
        LOG_WARNING("启动横幅文件未找到或为空: " + filePath);
        bannerLines_.push_back("Banner not found.");
        bannerLines_.push_back("Please check path: " + filePath);
    }
}

void StartupScreen::loadFiles(const std::string& directoryPath) {
    fileList_.clear(); // 清空现有列表
    fileList_.push_back(NULL_WORKSPACE_OPTION_TEXT); // 首先添加 NULL 选项

    std::vector<std::string> actualFiles = TuiUtils::listFilesNoExt(directoryPath);
    fileList_.insert(fileList_.end(), actualFiles.begin(), actualFiles.end()); // 追加实际文件

    // currentSelection_ 将在构造函数中设置
    scrollOffset_ = 0; // 重置滚动偏移
}

std::string StartupScreen::run() {
    Terminal::setRawMode(true);
    Terminal::clear();
    
    std::string selectedFileName = ""; // 用于存储选中的文件名

    while (active_) {
        draw(); // 绘制当前状态
        std::cout << std::flush; // 确保立即显示

        // 等待输入 (Terminal::readChar() 应该是阻塞的)
        int key = Terminal::readChar(); 
        int action = handleInput(key);

        if (action == 1) { // 回车
            if (currentSelection_ >= 0 && currentSelection_ < static_cast<int>(fileList_.size())) {
                const std::string& selectedItem = fileList_[currentSelection_];
                if (selectedItem == NULL_WORKSPACE_OPTION_TEXT) {
                    selectedFileName = ""; // NULL 选项被选中，不返回文件名
                } else {
                    selectedFileName = selectedItem; // 返回选中的文件名
                }
            }
            active_ = false;
        } else if (action == 2) { // Escape
            active_ = false;
            // selectedFileName 保持为空
        }
        // 如果 action == 0, 循环继续
    }

    Terminal::clear();
    Terminal::setRawMode(false); // 恢复终端模式
    Terminal::resetColor();
    Terminal::setCursor(0,0);    // 将光标移到左上角
    std::cout << std::flush;     // 确保清屏等操作生效
    return selectedFileName; // 修改：返回选中的文件名
}

int StartupScreen::handleInput(int key) {
    // fileList_ 至少包含 NULL_WORKSPACE_OPTION_TEXT，所以不为空
    // bool noItems = fileList_.empty(); // 此检查不再准确反映是否有“可选文件”

    switch (key) {
        case KEY_UP:
            if (currentSelection_ > 0) { // 只要不是第一个，就可以向上
                currentSelection_--;
            }
            break;
        case KEY_DOWN:
            if (currentSelection_ < static_cast<int>(fileList_.size()) - 1) { // 只要不是最后一个，就可以向下
                currentSelection_++;
            }
            break;
        case KEY_ENTER:
            return 1; // 确认选择
        case KEY_ESCAPE:
            return 2; // 退出
        default:
            break; // 其他键忽略
    }
    return 0; // 继续循环
}

void StartupScreen::draw() {
    // Terminal::clear(); // 在循环开始时清屏，或在每个绘制函数内部管理清除
    // 为了减少闪烁，最好只清除需要重绘的部分，但全屏清除更简单
    auto [termRows, termCols] = Terminal::getSize();

    // 用默认背景色填充整个屏幕，避免旧内容残留
    TuiUtils::fillRect(0, 0, termRows, termCols, ' ', Color::DEFAULT, Color::BLACK);


    if (termCols > termRows && termCols > 60) { // 宽屏 (增加一个最小宽度判断)
        drawWideLayout(termRows, termCols);
    } else { // 竖屏或方形
        drawTallLayout(termRows, termCols);
    }
    std::cout << std::flush;
}

void StartupScreen::drawWideLayout(int termRows, int termCols) {
    int dirWidth = termCols / 4;
    if (dirWidth < 20) dirWidth = std::min(termCols / 2, 20); // 最小目录宽度
    dirWidth = std::min(dirWidth, termCols - 40); // 确保banner至少有40列
    dirWidth = std::max(15, dirWidth); // 绝对最小宽度

    int bannerPanelWidth = termCols - dirWidth;
    int contentHeight = termRows; 

    // 目录面板 (左侧)
    TuiUtils::drawBox(0, 0, contentHeight, dirWidth, "SELECT WORKENV");
    int listR = 1, listC = 1;
    int listH = std::max(0, contentHeight - 2);
    int listW = std::max(0, dirWidth - 2);

    if (listH > 0 && listW > 0) {
        if (currentSelection_ < scrollOffset_) {
            scrollOffset_ = currentSelection_;
        } else if (currentSelection_ >= scrollOffset_ + listH) {
            scrollOffset_ = currentSelection_ - listH + 1;
        }
        scrollOffset_ = std::max(0, scrollOffset_); // 确保不为负
        TuiUtils::drawTextList(listR, listC, listH, listW, fileList_, currentSelection_, scrollOffset_,
                               Color::WHITE,  // itemColor
                               Color::BLACK,  // selectedColor (FG)
                               Color::CYAN,   // selectedBgColor
                               Color::BLACK,  // defaultBgColor
                               NULL_WORKSPACE_OPTION_TEXT, // specialItemText
                               Color::YELLOW, // specialItemFgColor
                               Color::BLACK   // specialItemBgColor (与 defaultBgColor 相同)
                               );
    }

    // 横幅面板 (右侧)
    int bannerPanelR = 0, bannerPanelC = dirWidth;
    int bannerPanelH = contentHeight;
    
    // 面板内文本区域的可用尺寸
    int textAvailableH = bannerPanelH;
    int textAvailableW = bannerPanelWidth;


    int bannerContentActualH = static_cast<int>(bannerLines_.size());
    int bannerContentActualW = 0;
    if (!bannerLines_.empty()) {
        for(const auto& line : bannerLines_) {
            size_t currentLineWidth = TuiUtils::countUtf8CodePoints(line);
            if (currentLineWidth > static_cast<size_t>(bannerContentActualW)) {
                bannerContentActualW = static_cast<int>(currentLineWidth);
            }
        }
    }
    
    // 确定实际绘制的尺寸 (不超过面板可用尺寸和内容实际尺寸)
    int drawH = std::min(bannerContentActualH, textAvailableH);
    int drawW = std::min(bannerContentActualW, textAvailableW);

    // 计算Banner块的起始Y坐标 (在可用区域内垂直居中)
    int bannerBlockStartY = bannerPanelR + std::max(0, (textAvailableH - drawH) / 2);
    // 计算Banner块的起始X坐标 (在可用区域内水平居中)
    int bannerBlockStartX = bannerPanelC + std::max(0, (textAvailableW - drawW) / 2);

    // 使用计算出的绘制尺寸和 Banner 内容进行绘制
    // drawTextLines 将负责处理 lines 向量中多余的行（如果 drawH < bannerContentActualH）
    // 以及行内内容的截断/填充（如果 drawW 与 bannerContentActualW 不同）
    TuiUtils::drawTextLines(bannerBlockStartY, bannerBlockStartX, 
                            drawH, drawW, 
                            bannerLines_, Color::CYAN, Color::BLACK);
}

void StartupScreen::drawTallLayout(int termRows, int termCols) {
    int dirHeight = termRows / 2;
    if (dirHeight < 5) dirHeight = std::min(termRows, 5); // 最小目录高度
    dirHeight = std::max(3, dirHeight); // 绝对最小高度

    int bannerPanelHeight = termRows - dirHeight;
    int contentWidth = termCols;

    // 目录面板 (顶部)
    TuiUtils::drawBox(0, 0, dirHeight, contentWidth, "SELECT WORKENV");
    int listR = 1, listC = 1;
    int listH = std::max(0, dirHeight - 2);
    int listW = std::max(0, contentWidth - 2);

    if (listH > 0 && listW > 0) {
        if (currentSelection_ < scrollOffset_) {
            scrollOffset_ = currentSelection_;
        } else if (currentSelection_ >= scrollOffset_ + listH) {
            scrollOffset_ = currentSelection_ - listH + 1;
        }
        scrollOffset_ = std::max(0, scrollOffset_);
        TuiUtils::drawTextList(listR, listC, listH, listW, fileList_, currentSelection_, scrollOffset_,
                               Color::WHITE,  // itemColor
                               Color::BLACK,  // selectedColor (FG)
                               Color::CYAN,   // selectedBgColor
                               Color::BLACK,  // defaultBgColor
                               NULL_WORKSPACE_OPTION_TEXT, // specialItemText
                               Color::YELLOW, // specialItemFgColor
                               Color::BLACK   // specialItemBgColor (与 defaultBgColor 相同)
                               );
    }
    
    // 横幅面板 (底部)
    int bannerPanelR = dirHeight, bannerPanelC = 0;
    // 面板内文本区域的可用尺寸
    int textAvailableH = bannerPanelHeight;
    int textAvailableW = contentWidth;

    int bannerContentActualH = static_cast<int>(bannerLines_.size());
    int bannerContentActualW = 0;
     if (!bannerLines_.empty()) {
        for(const auto& line : bannerLines_) {
            size_t currentLineWidth = TuiUtils::countUtf8CodePoints(line);
            if (currentLineWidth > static_cast<size_t>(bannerContentActualW)) {
                bannerContentActualW = static_cast<int>(currentLineWidth);
            }
        }
    }

    // 确定实际绘制的尺寸
    int drawH = std::min(bannerContentActualH, textAvailableH);
    int drawW = std::min(bannerContentActualW, textAvailableW);

    // 计算Banner块的起始Y坐标 (垂直居中)
    int bannerBlockStartY = bannerPanelR + std::max(0, (textAvailableH - drawH) / 2);
    // 计算Banner块的起始X坐标 (水平居中)
    int bannerBlockStartX = bannerPanelC + std::max(0, (textAvailableW - drawW) / 2);
    
    TuiUtils::drawTextLines(bannerBlockStartY, bannerBlockStartX,
                            drawH, drawW,
                            bannerLines_, Color::CYAN, Color::BLACK);
}
