#pragma once
#include <string>
#include <vector>
#include <filesystem> // For path manipulation
#include "../utils/tui_utils.h" 
#include "tui_terminal.h"      

class StartupScreen {
public:
    StartupScreen(const std::string& bannerFilePath, const std::string& workDirPath);
    // 返回选中的文件完整路径，如果按ESC或无选择则返回空字符串
    std::string run(); 

private:
    void loadBanner(const std::string& filePath);
    void loadFiles(const std::string& directoryPath);
    void draw();
    void drawWideLayout(int termRows, int termCols);
    void drawTallLayout(int termRows, int termCols);
    // 返回值: 0=继续, 1=回车确认, 2=ESC退出
    int handleInput(int key); 

    std::vector<std::string> bannerLines_;
    std::vector<std::string> fileList_; // 存储文件名
    std::string workDirectoryPath_;     // 工作目录的完整路径
    int currentSelection_;              // 当前选中项的索引
    int scrollOffset_;                  // 列表的滚动偏移量
    bool active_;                       // 控制启动界面循环

    const std::string bannerFileConfigPath_; // 传入的banner文件路径 (可能相对)
    const std::string workDirConfigPath_;    // 传入的工作目录路径 (可能相对)
    static const std::string NULL_WORKSPACE_OPTION_TEXT; // 新增：NULL选项的文本
};
