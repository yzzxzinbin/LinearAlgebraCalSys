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

// Structure to hold information about each item in the list
struct ListItem {
    std::string name;          // Original name (e.g., "myfile", "mydir")
    std::string displayName;   // Formatted name for display (e.g., "  📁> mydir")
    std::filesystem::path fullPath; // 修改类型为 path
    enum class Type { FILE, DIRECTORY, SPECIAL } itemType;
    bool isExpanded;           // For directories: true if expanded, false otherwise
    int depth;                 // Indentation depth level
    
    // New members for detailed drawing
    std::string constructedIndentString; // Full prefix for display, e.g., "│  ├── "
    std::string iconGlyph;
    RGBColor iconColor; // Changed from Color to RGBColor
    RGBColor indentColor; // 新增：用于缩进/树结构字符的颜色
    bool isLastAmongSiblings; // True if this item is the last in its current sibling group
    std::string stemForMyChildren; // Stem prefix for children of this item, e.g., "│     "

    ListItem(std::string n, std::string dispN_unused, std::filesystem::path fp, Type t, int d, bool exp = false)
        : name(std::move(n)), fullPath(std::move(fp)),
          itemType(t), isExpanded(exp), depth(d), iconColor({255, 255, 255}), 
          indentColor({180, 180, 180}), // 初始化 indentColor，例如使用 TREE_STRUCTURE_COLOR
          isLastAmongSiblings(false) { // Default to white, isLastAmongSiblings defaults to false
        // constructedIndentString, stemForMyChildren, iconGlyph, iconColor, and displayName
        // will be set by StartupScreen::buildDisplayString.
    }
};

class StartupScreen {
public:
    StartupScreen(const std::string& bannerFilePath, const std::string& workDirPath);
    ~StartupScreen();
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
    void buildDisplayString(ListItem& item, const std::string& stemAtItemLevel); // Added stemAtItemLevel parameter
    void getChildrenOfPath(const std::filesystem::path& path, int childrenDepth, const std::string& stemForChildrenToUse, std::vector<ListItem>& childrenList); // 参数类型改为 path
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
