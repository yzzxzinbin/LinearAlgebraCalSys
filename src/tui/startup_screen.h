#pragma once
#include <string>
#include <vector>
#include <filesystem> // For path manipulation
#include <algorithm>  // For std::sort
#include "../utils/tui_utils.h" 
#include "tui_terminal.h"      

// Icon Definitions (Placeholders - User will replace with actual Nerd Font characters)
// #define ICON_FILE         "" // 旧的通用文件图标，将被移除
#define ICON_DIR_CLOSED   "" // 更新：文件夹图标 (U+1F4C1)
#define ICON_DIR_OPEN     "" // 更新：打开的文件夹图标 (U+1F4C2)
#define INDENT_STRING     "  "   // String used for one level of indentation

// Structure to hold information about each item in the list
struct ListItem {
    std::string name;          // Original name (e.g., "myfile", "mydir")
    std::string displayName;   // Formatted name for display (e.g., "  📁> mydir")
    std::string fullPath;      // Full path to the item
    enum class Type { FILE, DIRECTORY, SPECIAL } itemType;
    bool isExpanded;           // For directories: true if expanded, false otherwise
    int depth;                 // Indentation depth level
    
    // New members for detailed drawing
    std::string indentString;
    std::string iconGlyph;
    RGBColor iconColor; // Changed from Color to RGBColor

    ListItem(std::string n, std::string dispN, std::string fp, Type t, int d, bool exp = false)
        : name(std::move(n)), displayName(std::move(dispN)), fullPath(std::move(fp)),
          itemType(t), isExpanded(exp), depth(d), iconColor({255, 255, 255}) { // Default to white
        // indentString, iconGlyph, and potentially updated displayName will be set by buildDisplayString
    }
};

class StartupScreen {
public:
    StartupScreen(const std::string& bannerFilePath, const std::string& workDirPath);
    // 返回选中的文件完整路径，如果按ESC或选中 "None" 则返回空字符串
    std::string run(); 

private:
    void loadBanner(const std::string& filePath);
    void loadInitialFiles(); // Loads top-level files and the "None" option
    void draw();
    void drawWideLayout(int termRows, int termCols);
    void drawTallLayout(int termRows, int termCols);
    // 返回值: 0=继续, 1=回车确认, 2=ESC退出
    int handleInput(int key); 

    // Helper methods for list management
    void buildDisplayString(ListItem& item);
    void getChildrenOfPath(const std::string& path, int depth, std::vector<ListItem>& childrenList);
    void toggleDirectoryExpansion(int listIndex);
    static std::string getIconForFile(const std::string& filename);
    // New helper method for icon color
    static RGBColor getIconColorForFile(const std::string& filename); // Changed return type

    std::vector<std::string> bannerLines_;
    std::vector<ListItem> fileList_;    // List of items to display
    std::string workDirectoryPath_;     // 工作目录的完整路径
    int currentSelection_;              // 当前选中项的索引
    int scrollOffset_;                  // 列表的滚动偏移量
    bool active_;                       // 控制启动界面循环

    // 用于检测终端尺寸变化的成员变量
    int lastTermRows_;
    int lastTermCols_;

    const std::string bannerFileConfigPath_; // 传入的banner文件路径 (可能相对)
    const std::string workDirConfigPath_;    // 传入的工作目录路径 (可能相对)
    static const std::string NULL_WORKSPACE_OPTION_TEXT; // NULL选项的文本
};
