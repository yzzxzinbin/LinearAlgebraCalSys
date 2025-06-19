#include "startup_screen.h"
#include "../utils/logger.h"
#include <iostream>      // 用于 std::cout (TuiUtils间接使用)
#include <algorithm>     // 用于 std::min/max, std::sort
#include <filesystem>    // 确保包含filesystem头文件
#include <unordered_set> // 新增：用于文件扩展名白名单
#include <map>           // 新增：用于图标映射

// 定义命名空间别名
namespace fs = std::filesystem;

// 定义 NULL 选项的文本
const std::string StartupScreen::NULL_WORKSPACE_OPTION_TEXT = "╭────── NONE ───────"; // 使用边框样式";

// Tree drawing characters
const std::string TREE_BRANCH_MIDDLE = "├─"; // Branch for a middle child
const std::string TREE_BRANCH_LAST   = "╰─"; // Branch for the last child
const std::string TREE_STEM_VERTICAL = "│  "; // Vertical line for an ongoing stem (2 spaces after |)
const std::string TREE_STEM_EMPTY    = "   "; // Empty space for a stem that ended (3 spaces)

// 新增：定义用于树结构字符的淡白色
const RGBColor TREE_STRUCTURE_COLOR = {120, 120, 120}; // 淡灰色/淡白色

// 新增：获取文件图标颜色的辅助方法实现
RGBColor StartupScreen::getIconColorForFile(const std::string &filename) { // Return type changed
    std::string ext_str;
    try {
        fs::path p = filename;
        ext_str = p.extension().string();
    } catch (const std::exception &e) {
        LOG_WARNING("Could not parse filename for extension (color): " + filename + " Error: " + e.what());
    }

    std::transform(ext_str.begin(), ext_str.end(), ext_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Text file extensions
    static const std::unordered_set<std::string> textExtensions = {
        ".txt", ".md", ".cpp", ".h", ".hpp", ".c", ".py", ".js", ".json", ".html", ".css", ".xml", ".csv", ".log"
    };
    // Resource file extensions
    static const std::unordered_set<std::string> resourceExtensions = {
        ".jpg", ".jpeg", ".png", ".gif", ".svg", ".pdf", ".zip", ".tar", ".rar"
    };
    // Executable file extensions
    static const std::unordered_set<std::string> executableExtensions = {
        ".exe", ".dll", ".sh", ".ps1"
    };
    // Office file extensions (specific colors)
    if (ext_str == ".doc" || ext_str == ".docx") return {0, 82, 155}; // Darker Blue
    if (ext_str == ".xls" || ext_str == ".xlsx") return {16, 124, 64}; // Darker Green
    if (ext_str == ".ppt" || ext_str == ".pptx") return {211, 72, 47}; // Office Red/Orange

    if (textExtensions.count(ext_str)) return {170, 170, 170}; // Light Gray
    if (resourceExtensions.count(ext_str)) return {90, 130, 180}; // A more desaturated blue
    if (executableExtensions.count(ext_str)) return {220, 220, 220}; // Light gray/white for executables
    
    return {255, 255, 255}; // Default color for other file icons (White)
}


// 新增：获取文件图标的辅助方法实现
std::string StartupScreen::getIconForFile(const std::string &filename)
{
    std::string ext_str;
    try
    {
        fs::path p = filename;
        ext_str = p.extension().string();
    }
    catch (const std::exception &e)
    {
        // 如果文件名无效导致 fs::path 构造失败，则 ext_str 将为空
        LOG_WARNING("Could not parse filename for extension: " + filename + " Error: " + e.what());
    }

    // 将扩展名转为小写
    std::transform(ext_str.begin(), ext_str.end(), ext_str.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });

    // 图标映射表
    // 用户可以根据自己的喜好和字体支持情况修改这些 Unicode 字符
    static const std::map<std::string, std::string> iconMap = {
        // 文本文件
        {".txt", ""},   //
        {".md", ""},    //
        {".cpp", "󰙲"},  //
        {".h", "\uf0fd"},  //
        {".hpp", ""},   //
        {".c", "󰙱"},    //
        {".py", ""},    //
        {".js", ""},    //
        {".json", "󰘦"}, //
        {".html", ""},  //
        {".css", ""},   //
        {".xml", "󰗀"},  //
        {".csv", ""},   //
        {".log", "󱂅"},  //

        // 资源文件
        {".jpg", "󰈥"},  //
        {".jpeg", "󰈥"}, //
        {".png", "󰸭"},  //
        {".gif", "󰵸"},  //
        {".svg", "󰜡"},  //
        {".pdf", ""},   // PDF 文件
        {".zip", "󰛫"},  // 压缩文件
        {".tar", "󰛫"},  //
        {".rar", "󰛫"},  //

        // 可执行文件
        {".exe", ""}, // 可执行文件
        {".dll", ""}, // 动态链接库
        {".sh", ""},  // Shell 脚本
        {".ps1", ""}, // PowerShell 脚本

        // 办公文件
        {".doc", "󱎒"},       // Word 文档
        {".docx", "󱎒"},      // Word 文档（.docx）
        {".xls", "󱎏"},      // Excel 表格
        {".xlsx", "󱎏"},     // Excel 表格（.xlsx）
        {".ppt", "󱎐"}, // PowerPoint 演示文稿
        {".pptx", "󱎐"}, // PowerPoint 演示文稿（.pptx）
        // 添加更多文件类型和对应的图标
    };

    auto it = iconMap.find(ext_str);
    if (it != iconMap.end())
    {
        return it->second;
    }
    return ""; // 默认文件图标 (Page with Curl)
}

// Helper to build the display string for a ListItem
void StartupScreen::buildDisplayString(ListItem& item, const std::string& stemAtItemLevel)
{
    item.iconColor = {255, 255, 255}; // Default icon color
    item.indentColor = TREE_STRUCTURE_COLOR; // 设置缩进/树结构字符的颜色

    if (item.itemType == ListItem::Type::DIRECTORY)
    {
        item.constructedIndentString = stemAtItemLevel + (item.isLastAmongSiblings ? TREE_BRANCH_LAST : TREE_BRANCH_MIDDLE);
        item.stemForMyChildren = stemAtItemLevel + (item.isLastAmongSiblings ? TREE_STEM_EMPTY : TREE_STEM_VERTICAL);
        item.iconGlyph = item.isExpanded ? ICON_DIR_OPEN : ICON_DIR_CLOSED;
        item.iconColor = {255, 165, 0}; // Orange using RGB
        item.displayName = item.constructedIndentString + item.iconGlyph + " " + item.name;
    }
    else if (item.itemType == ListItem::Type::FILE)
    {
        item.constructedIndentString = stemAtItemLevel + (item.isLastAmongSiblings ? TREE_BRANCH_LAST : TREE_BRANCH_MIDDLE);
        item.stemForMyChildren = stemAtItemLevel + (item.isLastAmongSiblings ? TREE_STEM_EMPTY : TREE_STEM_VERTICAL); // Though files don't have children, store consistently
        item.iconGlyph = getIconForFile(item.name);
        item.iconColor = getIconColorForFile(item.name);
        item.displayName = item.constructedIndentString + item.iconGlyph + " " + item.name;
    }
    else if (item.itemType == ListItem::Type::SPECIAL)
    {
        item.constructedIndentString = ""; // No tree prefix for special items
        item.stemForMyChildren = "";       // No children stem
        item.iconGlyph = "";               // No icon
        // item.iconColor remains default white, though not used
        // item.indentColor will be TREE_STRUCTURE_COLOR but constructedIndentString is empty
        item.displayName = item.name; // Special items might not have leading/trailing spaces around icon
    }
}

// Helper to get children of a given path and populate a list
void StartupScreen::getChildrenOfPath(const fs::path& dirPath, int childrenDepth, const std::string& stemForChildrenToUse, std::vector<ListItem>& childrenList)
{
    std::vector<fs::directory_entry> entries;
    // fs::path dirPath(path); // 已经是 path 类型
    try
    {
        if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
        {
            return;
        }
        for (const auto &entry : fs::directory_iterator(dirPath))
        {
            entries.push_back(entry);
        }
    }
    catch (const fs::filesystem_error &e)
    {
        LOG_WARNING("Error accessing directory " + dirPath.string() + ": " + std::string(e.what()));
        return;
    }

    // Sort entries: directories first, then by name
    std::sort(entries.begin(), entries.end(), [](const fs::directory_entry &a, const fs::directory_entry &b)
              {
                  bool a_is_dir = fs::is_directory(a.status());
                  bool b_is_dir = fs::is_directory(b.status());
                  if (a_is_dir != b_is_dir)
                      return a_is_dir > b_is_dir;                                     // Directories first
                  return a.path().filename().string() < b.path().filename().string(); // Then by name
              });

    for (const auto &entry : entries)
    {
        std::string itemName = entry.path().filename().string();
        fs::path itemFullPath = fs::absolute(entry.path()); // 保持为 path 类型
        bool isLast = (&entry == &entries.back());
        if (fs::is_directory(entry.status()))
        {
            ListItem dirItem(itemName, "", itemFullPath, ListItem::Type::DIRECTORY, childrenDepth, false);
            dirItem.isLastAmongSiblings = isLast;
            buildDisplayString(dirItem, stemForChildrenToUse);
            childrenList.push_back(dirItem);
        }
        else if (fs::is_regular_file(entry.status()))
        {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c){ return std::tolower(c); });

            const static std::unordered_set<std::string> allowedExtensions = {
                "", ".txt", ".cpp", ".json", ".h", ".hpp", ".log", ".md", ".csv",
                ".py", ".js", ".xml", ".c", ".html", ".css", 
                ".jpg", ".jpeg", ".png", ".gif", ".svg", ".pdf", 
                ".zip", ".tar", ".rar", 
                ".exe", ".dll", ".sh", ".ps1", 
                ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx"
            };

            if (allowedExtensions.count(ext))
            {
                ListItem fileItem(itemName, "", itemFullPath, ListItem::Type::FILE, childrenDepth, false);
                fileItem.isLastAmongSiblings = isLast;
                buildDisplayString(fileItem, stemForChildrenToUse);
                childrenList.push_back(fileItem);
            }
        }
    }
}

void StartupScreen::toggleDirectoryExpansion(int listIndex)
{
    if (listIndex < 0 || listIndex >= static_cast<int>(fileList_.size()))
        return;

    ListItem &item = fileList_[listIndex];
    if (item.itemType != ListItem::Type::DIRECTORY)
        return;

    item.isExpanded = !item.isExpanded;
    // Re-build display string for the toggled item itself to update its icon and potentially prefix if logic changes
    // The stemAtItemLevel for this item doesn't change, so we need to find what it was.
    // This is tricky if buildDisplayString needs the original stem.
    // For now, assume its constructedIndentString and stemForMyChildren are correct and only icon changes.
    // A better way: find its parent's stemForMyChildren or reconstruct.
    // Simplification: its own stem parts (constructedIndentString, stemForMyChildren) are already correct from its creation.
    // We only need to update its icon and thus displayName.
    // Let's find the stem that *led* to this item.
    // The stem *at* item's level is item.constructedIndentString minus the "├── " or "└── " part.
    std::string stemLeadingToItem;
    size_t branchLen = TREE_BRANCH_MIDDLE.length(); // Assume middle and last have same length
    if (item.constructedIndentString.length() >= branchLen) {
        stemLeadingToItem = item.constructedIndentString.substr(0, item.constructedIndentString.length() - branchLen);
    }
    buildDisplayString(item, stemLeadingToItem); // Rebuild with correct icon after expansion toggle


    if (item.isExpanded)
    {
        std::vector<ListItem> children;
        getChildrenOfPath(item.fullPath, item.depth + 1, item.stemForMyChildren, children); // 直接传递 path
        if (!children.empty())
        {
            fileList_.insert(fileList_.begin() + listIndex + 1, children.begin(), children.end());
        }
    }
    else
    {
        // Collapse: remove children
        // Children are items directly following the parent with a greater depth
        auto it = fileList_.begin() + listIndex + 1;
        while (it != fileList_.end() && it->depth > item.depth)
        {
            it = fileList_.erase(it); // Erase returns iterator to the next element
        }
    }
    // Ensure selection is valid after list modification
    currentSelection_ = std::min(currentSelection_, static_cast<int>(fileList_.size()) - 1);
    if (currentSelection_ < 0 && !fileList_.empty())
    {
        currentSelection_ = 0;
    }
    // Adjust scrollOffset if needed (simplified: ensure selected is visible)
    int listH = (lastTermRows_ > 0) ? std::max(0, ((lastTermCols_ > lastTermRows_ && lastTermCols_ > 60) ? lastTermRows_ : lastTermRows_ / 2) - 2) : 10; // Estimate list height
    if (listH > 0)
    { // Avoid division by zero or negative if termRows not set
        if (currentSelection_ < scrollOffset_)
        {
            scrollOffset_ = currentSelection_;
        }
        else if (currentSelection_ >= scrollOffset_ + listH)
        {
            scrollOffset_ = currentSelection_ - listH + 1;
        }
        scrollOffset_ = std::max(0, scrollOffset_);
    }
}

StartupScreen::StartupScreen(const std::string &bannerFilePath, const std::string &workDirPath)
    : currentSelection_(0), scrollOffset_(0), active_(true),
      lastTermRows_(0), lastTermCols_(0), // 初始化上次终端尺寸
      bannerFileConfigPath_(bannerFilePath), workDirConfigPath_(workDirPath)
{

    std::iostream::sync_with_stdio(false); // Disable sync with C stdio for performance
    try
    {
        // Ensure workDirectoryPath_ is absolute and valid.
        // If workDirPath is empty or invalid, fs::absolute might behave differently or throw.
        // It's good practice to check if workDirConfigPath_ is empty before calling fs::absolute,
        // or handle potential exceptions robustly.
        if (workDirConfigPath_.empty())
        {
            LOG_ERROR("Work directory path is empty. Using current directory as fallback.");
            workDirectoryPath_ = fs::current_path().string();
        }
        else
        {
            fs::path tempPath = workDirConfigPath_;
            if (!fs::exists(tempPath))
            {
                LOG_WARNING("Provided work directory does not exist: " + workDirConfigPath_ + ". Using current directory.");
                workDirectoryPath_ = fs::current_path().string();
            }
            else
            {
                workDirectoryPath_ = fs::absolute(tempPath).string();
            }
        }
    }
    catch (const fs::filesystem_error &e)
    {
        LOG_ERROR("Error processing work directory path '" + workDirConfigPath_ + "': " + std::string(e.what()) + ". Using current directory.");
        try
        {
            workDirectoryPath_ = fs::current_path().string();
        }
        catch (const fs::filesystem_error &currentPathError)
        {
            LOG_FATAL("Failed to get current path: " + std::string(currentPathError.what()) + ". Startup screen may not function correctly.");
            // workDirectoryPath_ might remain uninitialized or empty, handle downstream.
            // For now, we'll proceed, and getChildrenOfPath will likely find nothing.
        }
    }

    loadBanner(bannerFileConfigPath_);
    loadInitialFiles();

    if (fileList_.empty())
    {                           // Should at least have "None"
        currentSelection_ = -1; // No selectable items
    }
    else
    {
        currentSelection_ = 0; // Default to first item
    }
}

// 析构函数
StartupScreen::~StartupScreen()
{
    // std::iostream::sync_with_stdio(true); // Re-enable sync with C stdio
}

void StartupScreen::loadBanner(const std::string &filePath)
{
    bannerLines_ = TuiUtils::readFileLines(filePath);
    if (bannerLines_.empty())
    {
        LOG_WARNING("启动横幅文件未找到或为空: " + filePath);
        bannerLines_.push_back("Banner not found.");
        bannerLines_.push_back("Please check path: " + filePath);
    }
}

void StartupScreen::loadInitialFiles()
{
    fileList_.clear();
    std::vector<ListItem> topLevelItems;
    if (!workDirectoryPath_.empty())
    {
        getChildrenOfPath(fs::path(workDirectoryPath_), 0, "", topLevelItems); // 传递 path 类型
    }
    // Add "None" option first. It's at depth 0.
    // Its isLastAmongSiblings depends on whether topLevelItems exist.
    ListItem noneItem(NULL_WORKSPACE_OPTION_TEXT, "", "", ListItem::Type::SPECIAL, 0);
    noneItem.isLastAmongSiblings = topLevelItems.empty(); // "None" is last if no FS items follow.
    buildDisplayString(noneItem, ""); // Stem for depth 0 items is empty.
    fileList_.push_back(noneItem);

    // If "None" was added and FS items exist, "None" is not the last.
    // The isLastAmongSiblings for items in topLevelItems is relative to *their own group*.
    // This is generally fine as "None" has no tree lines.
    // If topLevelItems exist, and "None" was marked as not last, the first item in topLevelItems
    // might need its isLastAmongSiblings re-evaluated if it was the *only* item in topLevelItems.
    // However, getChildrenOfPath correctly sets isLastAmongSiblings for the items *it generates*.
    // If "None" is present, and topLevelItems has only one item, that one item is correctly marked as last *within its group*.
    // The visual effect should be:
    // -------- NONE --------
    // └── file1 (if only one file/dir at root)
    // or
    // -------- NONE --------
    // ├── dir1
    // └── file2

    if (!topLevelItems.empty()) {
        if (fileList_.size() == 1 && fileList_[0].itemType == ListItem::Type::SPECIAL) { // "None" is currently the only item
             // If topLevelItems are about to be added, "None" is no longer the last.
            fileList_[0].isLastAmongSiblings = false;
            buildDisplayString(fileList_[0], ""); // Rebuild "None" item's display string
        }
        fileList_.insert(fileList_.end(), topLevelItems.begin(), topLevelItems.end());
    }


    currentSelection_ = fileList_.empty() ? -1 : 0;
    scrollOffset_ = 0;
}

std::string StartupScreen::run()
{
    Terminal::setRawMode(true);
    Terminal::clear();
    std::cout << std::flush;

    std::string selectedPath = ""; // Changed from selectedFileName for clarity

    while (active_)
    {
        draw();
        std::cout << std::flush;

        int key = Terminal::readChar();
        int action = handleInput(key);

        if (action == 1) // Enter on a file or "None"
        {
            if (currentSelection_ >= 0 && currentSelection_ < static_cast<int>(fileList_.size()))
            {
                const auto &selectedItem = fileList_[currentSelection_];
                if (selectedItem.itemType == ListItem::Type::FILE)
                {
                    selectedPath = selectedItem.fullPath.string(); // 修正：加 .string()
                }
                else if (selectedItem.itemType == ListItem::Type::SPECIAL && selectedItem.name == NULL_WORKSPACE_OPTION_TEXT)
                {
                    selectedPath = ""; // "None" option returns empty string
                }
                // Directories are handled by toggleDirectoryExpansion, not resulting in action == 1
            }
            active_ = false;
        }
        else if (action == 2) // Escape
        {
            active_ = false;
            selectedPath = ""; // ESC also returns empty string
        }
    }

    Terminal::clear();
    Terminal::setRawMode(false);
    Terminal::resetColor();
    Terminal::setCursor(0, 0);
    std::cout << std::flush;
    return selectedPath;
}

int StartupScreen::handleInput(int key)
{
    if (fileList_.empty())
        return 0; // No items to interact with

    switch (key)
    {
    case KEY_UP:
        if (currentSelection_ > 0)
        {
            currentSelection_--;
        }
        break;
    case KEY_DOWN:
        if (currentSelection_ < static_cast<int>(fileList_.size()) - 1)
        {
            currentSelection_++;
        }
        break;
    case KEY_ENTER:
        if (currentSelection_ >= 0 && currentSelection_ < static_cast<int>(fileList_.size()))
        {
            ListItem &item = fileList_[currentSelection_];
            if (item.itemType == ListItem::Type::DIRECTORY)
            {
                toggleDirectoryExpansion(currentSelection_);
                return 0; // Stay in screen, list updated
            }
            else if (item.itemType == ListItem::Type::FILE || item.itemType == ListItem::Type::SPECIAL)
            {
                return 1; // Confirm selection (File or "None")
            }
        }
        break;
    case KEY_RIGHT: // Expand directory or do nothing
        if (currentSelection_ >= 0 && currentSelection_ < static_cast<int>(fileList_.size()))
        {
            ListItem &item = fileList_[currentSelection_];
            if (item.itemType == ListItem::Type::DIRECTORY && !item.isExpanded)
            {
                toggleDirectoryExpansion(currentSelection_);
            }
        }
        return 0;  // Stay in screen
    case KEY_LEFT: // Collapse directory or do nothing
        if (currentSelection_ >= 0 && currentSelection_ < static_cast<int>(fileList_.size()))
        {
            ListItem &item = fileList_[currentSelection_];
            if (item.itemType == ListItem::Type::DIRECTORY && item.isExpanded)
            {
                toggleDirectoryExpansion(currentSelection_);
            }
            // Future: could navigate to parent if not expanded and depth > 0
        }
        return 0; // Stay in screen
    case KEY_ESCAPE:
        return 2; // 退出
    default:
        break;
    }
    return 0; // 继续循环
}

void StartupScreen::draw()
{
    auto [termRows, termCols] = Terminal::getSize();

    if (termRows != lastTermRows_ || termCols != lastTermCols_)
    {
        TuiUtils::fillRect(0, 0, termRows, termCols, ' ', Color::DEFAULT, Color::DEFAULT);
        lastTermRows_ = termRows;
        lastTermCols_ = termCols;
    }

    if (termCols > termRows && termCols > 60)
    {
        drawWideLayout(termRows, termCols);
    }
    else
    {
        drawTallLayout(termRows, termCols);
    }
    std::cout << std::flush;
}

void StartupScreen::drawWideLayout(int termRows, int termCols)
{
    int dirWidth = termCols / 4;
    if (dirWidth < 20)
        dirWidth = std::min(termCols / 2, 20);
    dirWidth = std::min(dirWidth, termCols - 40); // Ensure banner has some space
    dirWidth = std::max(15, dirWidth); // Minimum width for the directory panel

    int bannerPanelWidth = termCols - dirWidth;
    int contentHeight = termRows;

    // Box is drawn from (0,0) with width dirWidth.
    // Borders take 1 col on left and 1 col on right.
    // Inner content area is from col 1 to dirWidth-2.
    // So, inner width is (dirWidth-2) - 1 + 1 = dirWidth-2.
    TuiUtils::drawBox(0, 0, contentHeight, dirWidth, " SELECT WORKENV ", Color::WHITE, Color::DEFAULT);
    int listR = 1, listC = 1;
    int listH = std::max(0, contentHeight - 2); // -2 for top/bottom box borders
    int listW = std::max(0, dirWidth - 2);      // Corrected: -2 for left/right box borders

    if (listH > 0 && listW > 0)
    {
        if (currentSelection_ < scrollOffset_)
        {
            scrollOffset_ = currentSelection_;
        }
        else if (currentSelection_ >= scrollOffset_ + listH)
        {
            scrollOffset_ = currentSelection_ - listH + 1;
        }
        scrollOffset_ = std::max(0, scrollOffset_);

        std::vector<TuiUtils::PrintableListItem> printableItems;
        printableItems.reserve(fileList_.size());
        for (const auto &item : fileList_) {
            TuiUtils::PrintableListItem pli;
            pli.indentString = item.constructedIndentString; // Use the new tree-based indent
            pli.iconGlyph = item.iconGlyph;
            pli.iconColor = item.iconColor;
            pli.indentColor = item.indentColor; // 新增：传递缩进颜色S
            pli.textWithoutIcon = item.name; // Original name or special text
            pli.fullDisplayStringForMatching = item.displayName; // For special item matching
            printableItems.push_back(pli);
        }
        
        // Original colors used by StartupScreen:
        // defaultItemTextColor = Color::WHITE
        // selectedItemTextColor = Color::DEFAULT (text color for selected item)
        // selectedItemBgColor = Color::CYAN
        // defaultItemBgColor = Color::DEFAULT
        // specialItemTextColor = Color::YELLOW (for "None" option text)
        // specialItemBgColor = Color::DEFAULT (for "None" option background)

        TuiUtils::drawTextList(listR, listC, listH, listW, printableItems, currentSelection_, scrollOffset_,
                               Color::WHITE,    // defaultItemTextColor
                               Color::DEFAULT,  // selectedItemTextColor
                               Color::CYAN,     // selectedItemBgColor
                               Color::DEFAULT,  // defaultItemBgColor
                               (fileList_.empty() || fileList_[0].itemType != ListItem::Type::SPECIAL) ? "" : fileList_[0].displayName, // specialItemFullTextMatch
                               Color::YELLOW,   // specialItemTextColor
                               Color::DEFAULT   // specialItemBgColor
        );
    }

    int bannerPanelR = 0, bannerPanelC = dirWidth;
    int textAvailableH = contentHeight;
    int textAvailableW = bannerPanelWidth;

    int bannerContentActualH = static_cast<int>(bannerLines_.size());
    int bannerContentActualW = 0;
    if (!bannerLines_.empty())
    {
        for (const auto &line : bannerLines_)
        {
            size_t currentLineWidth = TuiUtils::countUtf8CodePoints(line);
            if (currentLineWidth > static_cast<size_t>(bannerContentActualW))
            {
                bannerContentActualW = static_cast<int>(currentLineWidth);
            }
        }
    }

    int drawH = std::min(bannerContentActualH, textAvailableH);
    int drawW = std::min(bannerContentActualW, textAvailableW);

    int bannerBlockStartY = bannerPanelR + std::max(0, (textAvailableH - drawH) / 2);
    int bannerBlockStartX = bannerPanelC + std::max(0, (textAvailableW - drawW) / 2);

    TuiUtils::drawTextLines(bannerBlockStartY, bannerBlockStartX,
                            drawH, drawW,
                            bannerLines_, Color::CYAN, Color::DEFAULT);
}

void StartupScreen::drawTallLayout(int termRows, int termCols)
{
    int dirHeight = termRows / 2;
    if (dirHeight < 5)
        dirHeight = std::min(termRows, 5);
    dirHeight = std::max(3, dirHeight);

    int bannerPanelHeight = termRows - dirHeight;
    int contentWidth = termCols;

    // Box is drawn from (0,0) with width contentWidth.
    // Inner width is contentWidth - 2.
    TuiUtils::drawBox(0, 0, dirHeight, contentWidth, " SELECT WORKENV ", Color::WHITE, Color::DEFAULT);
    int listR = 1, listC = 1;
    int listH = std::max(0, dirHeight - 2);    // -2 for top/bottom box borders
    int listW = std::max(0, contentWidth - 2); // -2 for left/right box borders (this was already correct)

    if (listH > 0 && listW > 0)
    {
        if (currentSelection_ < scrollOffset_)
        {
            scrollOffset_ = currentSelection_;
        }
        else if (currentSelection_ >= scrollOffset_ + listH)
        {
            scrollOffset_ = currentSelection_ - listH + 1;
        }
        scrollOffset_ = std::max(0, scrollOffset_);

        std::vector<TuiUtils::PrintableListItem> printableItems;
        printableItems.reserve(fileList_.size());
        for (const auto &item : fileList_) {
            TuiUtils::PrintableListItem pli;
            pli.indentString = item.constructedIndentString; // Use the new tree-based indent
            pli.iconGlyph = item.iconGlyph;
            pli.iconColor = item.iconColor;
            pli.indentColor = item.indentColor; // 新增：传递缩进颜色
            pli.textWithoutIcon = item.name; // Original name or special text
            pli.fullDisplayStringForMatching = item.displayName; // For special item matching
            printableItems.push_back(pli);
        }
        
        TuiUtils::drawTextList(listR, listC, listH, listW, printableItems, currentSelection_, scrollOffset_,
                               Color::WHITE,    // defaultItemTextColor
                               Color::DEFAULT,  // selectedItemTextColor
                               Color::CYAN,     // selectedItemBgColor
                               Color::DEFAULT,  // defaultItemBgColor
                               (fileList_.empty() || fileList_[0].itemType != ListItem::Type::SPECIAL) ? "" : fileList_[0].displayName, // specialItemFullTextMatch
                               Color::YELLOW,   // specialItemTextColor
                               Color::DEFAULT   // specialItemBgColor
        );
    }

    int bannerPanelR = dirHeight, bannerPanelC = 0;
    int textAvailableH = bannerPanelHeight;
    int textAvailableW = contentWidth;

    int bannerContentActualH = static_cast<int>(bannerLines_.size());
    int bannerContentActualW = 0;
    if (!bannerLines_.empty())
    {
        for (const auto &line : bannerLines_)
        {
            size_t currentLineWidth = TuiUtils::countUtf8CodePoints(line);
            if (currentLineWidth > static_cast<size_t>(bannerContentActualW))
            {
                bannerContentActualW = static_cast<int>(currentLineWidth);
            }
        }
    }

    int drawH = std::min(bannerContentActualH, textAvailableH);
    int drawW = std::min(bannerContentActualW, textAvailableW);

    int bannerBlockStartY = bannerPanelR + std::max(0, (textAvailableH - drawH) / 2);
    int bannerBlockStartX = bannerPanelC + std::max(0, (textAvailableW - drawW) / 2);

    TuiUtils::drawTextLines(bannerBlockStartY, bannerBlockStartX,
                            drawH, drawW,
                            bannerLines_, Color::CYAN, Color::DEFAULT);
}
