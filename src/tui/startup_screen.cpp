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
const std::string StartupScreen::NULL_WORKSPACE_OPTION_TEXT = " -------- NONE -------- ";

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
        {"word", "󱎒"},       // Word 文档
        {"excel", "󱎏"},      // Excel 表格
        {"powerpoint", "󱎐"}, // PowerPoint 演示文稿
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
void StartupScreen::buildDisplayString(ListItem &item)
{
    std::string indentPart;
    // Add indentation based on depth
    for (int i = 0; i < item.depth; ++i)
    {
        indentPart += INDENT_STRING;
    }

    std::string iconAndNamePart;
    if (item.itemType == ListItem::Type::DIRECTORY)
    {
        // 图标左侧加一个空格，图标本身，图标右侧一个空格，然后是名称
        iconAndNamePart = " " + std::string(item.isExpanded ? ICON_DIR_OPEN : ICON_DIR_CLOSED) + " " + item.name;
    }
    else if (item.itemType == ListItem::Type::FILE)
    {
        // 使用新的辅助函数获取文件类型特定的图标
        std::string fileIcon = getIconForFile(item.name);
        // 图标左侧加一个空格，图标本身，图标右侧一个空格，然后是名称
        iconAndNamePart = " " + fileIcon + " " + item.name;
    }
    else if (item.itemType == ListItem::Type::SPECIAL)
    {
        // 特殊项（如 "None"）前面不加图标的额外空格，只应用缩进（如果depth > 0）
        // 如果希望特殊项在 depth 0 时也与其他项的文本部分对齐，可以添加与 " icon " 等宽的空格
        // 例如: iconAndNamePart = "   " + item.name; // 假设 " icon " 视觉宽度为3
        // 为简单起见，目前仅使用名称
        iconAndNamePart = item.name;
    }
    item.displayName = indentPart + iconAndNamePart;
}

// Helper to get children of a given path and populate a list
void StartupScreen::getChildrenOfPath(const std::string &path, int depth, std::vector<ListItem> &childrenList)
{
    std::vector<fs::directory_entry> entries;
    try
    {
        if (!fs::exists(path) || !fs::is_directory(path))
        {
            // Log or handle cases where path is not a directory, e.g., when trying to get children of a file.
            // For initial load, workDirectoryPath_ is checked in constructor.
            // For recursive calls, 'path' comes from a ListItem known to be a directory.
            return;
        }
        for (const auto &entry : fs::directory_iterator(path))
        {
            entries.push_back(entry);
        }
    }
    catch (const fs::filesystem_error &e)
    {
        LOG_WARNING("Error accessing directory " + path + ": " + std::string(e.what()));
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
        // Store absolute paths to avoid issues with relative paths later
        std::string itemFullPath = fs::absolute(entry.path()).string();

        if (fs::is_directory(entry.status()))
        {
            ListItem dirItem(itemName, "", itemFullPath, ListItem::Type::DIRECTORY, depth, false);
            buildDisplayString(dirItem);
            childrenList.push_back(dirItem);
        }
        else if (fs::is_regular_file(entry.status()))
        {
            std::string ext = entry.path().extension().string();
            // 将扩展名转为小写以便比较
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });

            const static std::unordered_set<std::string> allowedExtensions = {
                "", ".txt", ".cpp", ".json", ".h", ".hpp", ".log", ".md", ".csv",
                ".py", ".js", ".xml", ".c", ".html", ".css", ".jpg", ".jpeg", ".png", ".gif", ".svg",
                ".pdf", ".zip", ".tar", ".rar", ".exe", ".dll", ".sh", ".ps1", "word", ".excel", ".powerpoint"
                // 根据需要添加更多常见的文本文件扩展名
            };

            if (allowedExtensions.count(ext))
            {
                ListItem fileItem(itemName, "", itemFullPath, ListItem::Type::FILE, depth, false);
                buildDisplayString(fileItem);
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
    buildDisplayString(item); // Update icon in displayName

    if (item.isExpanded)
    {
        // Expand: insert children
        std::vector<ListItem> children;
        getChildrenOfPath(item.fullPath, item.depth + 1, children);
        if (!children.empty())
        {
            // Insert children right after the parent directory item
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

    ListItem noneItem(NULL_WORKSPACE_OPTION_TEXT, "", "", ListItem::Type::SPECIAL, 0);
    buildDisplayString(noneItem);
    fileList_.push_back(noneItem);

    // workDirectoryPath_ is now guaranteed to be set (even if to current path as fallback)
    // or potentially empty if all fallbacks failed (highly unlikely but good to be aware).
    if (!workDirectoryPath_.empty())
    {
        std::vector<ListItem> topLevelItems;
        getChildrenOfPath(workDirectoryPath_, 0, topLevelItems);
        fileList_.insert(fileList_.end(), topLevelItems.begin(), topLevelItems.end());
    }

    // currentSelection_ and scrollOffset_ will be set/reset after this.
    // If fileList_ only contains "None", currentSelection_ will be 0.
    currentSelection_ = fileList_.empty() ? -1 : 0;
    scrollOffset_ = 0;
}

std::string StartupScreen::run()
{
    Terminal::setRawMode(true);
    Terminal::clear();

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
                    selectedPath = selectedItem.fullPath;
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
    dirWidth = std::min(dirWidth, termCols - 40);
    dirWidth = std::max(15, dirWidth);

    int bannerPanelWidth = termCols - dirWidth;
    int contentHeight = termRows;

    TuiUtils::drawBox(0, 0, contentHeight, dirWidth, " SELECT WORKENV ", Color::WHITE, Color::DEFAULT);
    int listR = 1, listC = 1;
    int listH = std::max(0, contentHeight - 2);
    int listW = std::max(0, dirWidth - 3);

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

        std::vector<std::string> displayNames;
        displayNames.reserve(fileList_.size());
        for (const auto &item : fileList_)
        {
            displayNames.push_back(item.displayName);
        }

        TuiUtils::drawTextList(listR, listC, listH, listW, displayNames, currentSelection_, scrollOffset_,
                               Color::WHITE,
                               Color::DEFAULT,
                               Color::CYAN,
                               Color::DEFAULT,
                               // 使用 "None" 选项的 displayName 进行特殊项匹配
                               (fileList_.empty() || fileList_[0].itemType != ListItem::Type::SPECIAL) ? "" : fileList_[0].displayName,
                               Color::YELLOW, // 特殊项的前景色 (例如 "None")
                               Color::DEFAULT // 特殊项的背景色 (例如 "None")
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

    TuiUtils::drawBox(0, 0, dirHeight, contentWidth, " SELECT WORKENV ", Color::WHITE, Color::DEFAULT);
    int listR = 1, listC = 1;
    int listH = std::max(0, dirHeight - 2);
    int listW = std::max(0, contentWidth - 2);

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

        std::vector<std::string> displayNames;
        displayNames.reserve(fileList_.size());
        for (const auto &item : fileList_)
        {
            displayNames.push_back(item.displayName);
        }

        TuiUtils::drawTextList(listR, listC, listH, listW, displayNames, currentSelection_, scrollOffset_,
                               Color::WHITE,
                               Color::DEFAULT,
                               Color::CYAN,
                               Color::DEFAULT,
                               (fileList_.empty() || fileList_[0].itemType != ListItem::Type::SPECIAL) ? "" : fileList_[0].displayName,
                               Color::YELLOW,
                               Color::DEFAULT);
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
