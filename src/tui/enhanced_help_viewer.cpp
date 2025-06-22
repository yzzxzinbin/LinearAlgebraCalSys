#include "enhanced_help_viewer.h"
#include "../utils/tui_utils.h"
#include <iostream>
#include <algorithm> // For std::max

EnhancedHelpViewer::EnhancedHelpViewer(int termRows, int termCols)
    : currentPageIndex(0), currentTopicIndex(0), scrollOffset(0),
      terminalRows(termRows), terminalCols(termCols) {
    std::iostream::sync_with_stdio(false);
    initializeHelpContent();
    updateLayout();
    updateStatus("帮助查看器：↑/↓ 选择主题, ←/→ 翻页, ESC 退出");
}

EnhancedHelpViewer::~EnhancedHelpViewer() {
    std::iostream::sync_with_stdio(true);
}

void EnhancedHelpViewer::initializeHelpContent() {
    // 第1页：基础命令
    helpPages.push_back({
        "基础命令",
        {
            {"help", "显示此帮助信息，进入增强型帮助查看器。\n\n操作:\n- ↑/↓: 选择主题\n- ←/→: 翻页\n- ESC: 退出"},
            {"clear", "清空屏幕、历史或变量。\n\n用法:\n- clear: 清空屏幕\n- clear -h: 清除命令历史\n- clear -v: 清除所有变量\n- clear -a: 清空屏幕+历史+变量"},
            {"vars", "显示所有已定义的变量。\n\n用法:\n- vars: 打开增强型变量预览器\n- vars -l: 简单列表模式，显示变量名和类型"},
            {"exit", "退出程序。\n\n用法:\n- exit: 正常退出（自动保存）\n- exit --no-saving: 退出时不保存变量和历史"},
            {"steps", "切换计算步骤显示开关。\n\n当开启时，计算过程会显示详细的中间步骤，便于理解和验证计算过程。"}
        }
    });

    // 第2页：变量定义
    helpPages.push_back({
        "变量定义",
        {
            {"矩阵定义", "使用方括号和分号定义矩阵。\n\n格式: 变量名 = [元素1,元素2;元素3,元素4]\n\n示例:\n- m1 = [1,2,3;4,5,6]\n- identity = [1,0;0,1]\n- zeros = [0,0,0;0,0,0]"},
            {"向量定义", "使用方括号定义向量（一行矩阵）。\n\n格式: 变量名 = [元素1,元素2,元素3]\n\n示例:\n- v1 = [1,2,3]\n- v2 = [0,1,0]\n- position = [3,4,5]"},
            {"分数定义", "使用斜杠定义分数。\n\n格式: 变量名 = 分子/分母\n\n示例:\n- f1 = 1/2\n- f2 = 3/4\n- f3 = -5/7\n\n注意: 分数会自动化简"},
            {"复杂表达式", "可以使用表达式直接定义变量。\n\n示例:\n- result = m1 * m2\n- sum_vec = v1 + v2\n- det_val = det(m1)"}
        }
    });

    // 第3页：变量操作
    helpPages.push_back({
        "变量操作",
        {
            {"show", "显示变量内容，支持格式化输出。\n\n用法:\n- show <var>: 显示原始格式\n- show <var> -f<精度>: 有效数字格式\n- show <var> -p<小数位>: 小数格式\n- show <var> -r <结果变量>: 保存格式化结果"},
            {"del", "删除指定的变量。\n\n用法: del <变量名>\n\n示例:\n- del m1\n- del temp_result\n\n注意: 删除后无法恢复"},
            {"rename", "重命名变量。\n\n用法: rename <旧名> <新名>\n\n示例:\n- rename m1 matrix_a\n- rename temp final_result\n\n注意: 新名称不能与已有变量冲突"},
            {"new", "创建新的矩阵或向量并进入编辑器。\n\n用法:\n- new <维度>: 创建向量\n- new <行数> <列数>: 创建矩阵\n\n示例:\n- new 3 (创建3维向量)\n- new 2 3 (创建2×3矩阵)"},
            {"edit", "编辑已存在的矩阵或向量。\n\n用法: edit <变量名>\n\n编辑器操作:\n- ↑↓←→: 移动光标\n- 数字/分数: 输入值\n- Tab: 下一个元素\n- ESC: 退出编辑器"}
        }
    });

    // 第4页：基本运算
    helpPages.push_back({
        "基本运算",
        {
            {"矩阵运算", "矩阵支持加减乘运算。\n\n运算符:\n- +: 矩阵加法（同型矩阵）\n- -: 矩阵减法（同型矩阵）\n- *: 矩阵乘法（左矩阵列数=右矩阵行数）\n\n示例:\n- sum = m1 + m2\n- diff = m1 - m2\n- product = m1 * m2"},
            {"向量运算", "向量支持加减、点积、叉积运算。\n\n运算符:\n- +: 向量加法\n- -: 向量减法\n- *: 向量点积（返回分数）\n- x: 向量叉积（仅限3维向量）\n\n示例:\n- sum = v1 + v2\n- dot_product = v1 * v2\n- cross_product = v1 x v2"},
            {"分数运算", "分数支持四则运算。\n\n运算符:\n- +, -, *, /: 四则运算\n\n示例:\n- sum = f1 + f2\n- product = f1 * f2\n- quotient = f1 / f2\n\n注意: 结果自动化简"},
            {"混合运算", "矩阵与标量、向量与标量的运算。\n\n支持的运算:\n- 矩阵 * 标量\n- 标量 * 矩阵\n- 向量 * 标量\n- 标量 * 向量\n\n示例:\n- scaled = 2 * m1\n- doubled = v1 * 2"}
        }
    });

    // 第5页：矩阵函数
    helpPages.push_back({
        "矩阵函数",
        {
            {"transpose", "计算矩阵转置。\n\n用法: transpose(matrix)\n\n示例:\n- mt = transpose(m1)\n\n说明: 将矩阵的行列互换"},
            {"det", "计算方阵的行列式。\n\n用法:\n- det(matrix): 默认算法\n- det_expansion(matrix): 按行列展开\n\n示例:\n- d = det(m1)\n- d2 = det_expansion(m1)\n\n注意: 仅适用于方阵"},
            {"inverse", "计算方阵的逆矩阵。\n\n用法:\n- inverse(matrix): 伴随矩阵法\n- inverse_gauss(matrix): 高斯-若尔当法\n\n示例:\n- inv1 = inverse(m1)\n- inv2 = inverse_gauss(m1)\n\n注意: 矩阵必须可逆"},
            {"rank", "计算矩阵的秩。\n\n用法: rank(matrix)\n\n示例:\n- r = rank(m1)\n\n说明: 返回矩阵的线性无关行（列）数"},
            {"ref/rref", "行阶梯形和最简行阶梯形。\n\n用法:\n- ref(matrix): 行阶梯形\n- rref(matrix): 最简行阶梯形\n\n示例:\n- ref_form = ref(m1)\n- rref_form = rref(m1)"},
            {"cofactor_matrix", "计算代数余子式矩阵。\n\n用法: cofactor_matrix(matrix)\n\n示例:\n- cof = cofactor_matrix(m1)\n\n说明: 每个元素替换为对应的代数余子式"},
            {"adjugate", "计算伴随矩阵。\n\n用法: adjugate(matrix)\n\n示例:\n- adj = adjugate(m1)\n\n说明: 代数余子式矩阵的转置"}
        }
    });

    // 第6页：高级功能
    helpPages.push_back({
        "高级功能",
        {
            {"diag", "使用向量创建对角矩阵。\n\n用法: diag(vector)\n\n示例:\n- v = [1,2,3]\n- diagonal = diag(v)\n\n结果: 3×3对角矩阵"},
            {"solveq", "求解线性方程组。\n\n用法:\n- solveq(A): 求解 Ax = 0\n- solveq(A, b): 求解 Ax = b\n\n示例:\n- solution = solveq(m1, v1)\n\n说明: 返回方程组解的向量或解集"},
            {"union_rref", "向量组联合化行最简形。\n\n用法: union_rref(A, B)\n\n示例:\n- combined = union_rref(m1, m2)\n\n说明: 将两个矩阵按列合并后化简"},
            {"rep_vecset", "计算向量组的线性表示。\n\n用法: rep_vecset(A, B)\n\n示例:\n- representation = rep_vecset(m1, m2)\n\n说明: 计算B中向量能否由A中向量线性表示"},
            {"向量函数", "向量专用函数。\n\n函数:\n- dot(v1, v2): 点积\n- cross(v1, v2): 叉积\n- norm(v): 向量模长\n- normalize(v): 单位化\n\n示例:\n- length = norm(v1)\n- unit = normalize(v1)"}
        }
    });

    // 第7页：文件操作
    helpPages.push_back({
        "文件操作",
        {
            {"export", "导出变量和历史到文件。\n\n用法: export <文件名>\n\n示例:\n- export \"my_work.dat\"\n- export session.dat\n\n说明: 保存所有变量和命令历史"},
            {"import", "从文件导入变量和历史。\n\n用法: import <文件名>\n\n示例:\n- import \"my_work.dat\"\n- import session.dat\n\n说明: 加载变量和历史记录"},
            {"csv", "导出变量为CSV格式。\n\n用法: csv <变量名>\n\n示例:\n- csv m1\n- csv result_matrix\n\n说明: 仅支持Matrix、Vector和Result类型\n文件名为: <变量名>.csv"},
            {"自动保存", "程序退出时自动保存。\n\n特性:\n- 正常退出时自动保存到 workspace.dat\n- 使用 exit --no-saving 可跳过保存\n- 下次启动时自动加载工作环境\n\n文件位置: 程序当前目录"}
        }
    });
}

void EnhancedHelpViewer::updateLayout() {
    listStartRow = 2;
    listStartCol = 1;
    listHeight = terminalRows - 4;
    listWidth = std::max(20, terminalCols / 4);

    detailStartRow = 2;
    detailStartCol = listStartCol + listWidth + 2;
    detailHeight = terminalRows - 4;
    detailWidth = terminalCols - detailStartCol - 1;
}

void EnhancedHelpViewer::clearScreen() {
    for (int i = 1; i < terminalRows - 1; i++) {
        Terminal::setCursor(i, 0);
        std::cout << std::string(terminalCols, ' ');
    }
}

void EnhancedHelpViewer::updateStatus(const std::string& msg) {
    statusMessage = msg;
}

EnhancedHelpViewer::ViewerResult EnhancedHelpViewer::handleInput(int key) {
    switch (key) {
        case KEY_UP:
            if (currentTopicIndex > 0) {
                currentTopicIndex--;
                updateScrolling();
            }
            return ViewerResult::CONTINUE;
        case KEY_DOWN:
            if (currentTopicIndex < helpPages[currentPageIndex].topics.size() - 1) {
                currentTopicIndex++;
                updateScrolling();
            }
            return ViewerResult::CONTINUE;
        case KEY_LEFT:
            if (currentPageIndex > 0) {
                currentPageIndex--;
                currentTopicIndex = 0;
                scrollOffset = 0;
            }
            return ViewerResult::CONTINUE;
        case KEY_RIGHT:
            if (currentPageIndex < helpPages.size() - 1) {
                currentPageIndex++;
                currentTopicIndex = 0;
                scrollOffset = 0;
            }
            return ViewerResult::CONTINUE;
        case KEY_ESCAPE:
            return ViewerResult::EXIT;
        default:
            return ViewerResult::CONTINUE;
    }
}

void EnhancedHelpViewer::updateScrolling() {
    if (helpPages.empty() || helpPages[currentPageIndex].topics.empty()) return;

    if (currentTopicIndex < scrollOffset) {
        scrollOffset = currentTopicIndex;
    } else if (currentTopicIndex >= scrollOffset + listHeight) {
        scrollOffset = currentTopicIndex - listHeight + 1;
    }
}

void EnhancedHelpViewer::draw() {
    clearScreen();

    Terminal::setCursor(0, 0);
    Terminal::setForeground(Color::CYAN);
    Terminal::setBackground(Color::BLUE);
    std::string title = " 帮助查看器 ";
    int padding = (terminalCols - TuiUtils::calculateUtf8VisualWidth(title)) / 2;
    std::string header(terminalCols, ' ');
    header.replace(padding, TuiUtils::calculateUtf8VisualWidth(title), title);
    std::cout << header;
    Terminal::resetColor();

    drawLayout();
}

void EnhancedHelpViewer::drawLayout() {
    drawTopicList();
    drawDetailView();

    Terminal::setCursor(terminalRows - 1, 0);
    Terminal::setForeground(Color::BLACK);
    Terminal::setBackground(Color::WHITE);
    std::string status = " " + statusMessage;
    status.resize(terminalCols, ' ');
    std::cout << status;
    Terminal::resetColor();
}

void EnhancedHelpViewer::drawTopicList() {
    const auto& currentPage = helpPages[currentPageIndex];
    
    // Draw page title
    std::string pageTitle = " " + std::to_string(currentPageIndex + 1) + "/" + std::to_string(helpPages.size()) + ": " + currentPage.pageTitle + " ";
    int titlePadding = (listWidth - TuiUtils::calculateUtf8VisualWidth(pageTitle)) / 2;
    Terminal::setCursor(listStartRow -1, listStartCol + titlePadding);
    Terminal::setForeground(Color::YELLOW);
    std::cout << TuiUtils::trimToUtf8VisualWidth(pageTitle, listWidth);
    Terminal::resetColor();

    if (currentPage.topics.empty()) {
        Terminal::setCursor(listStartRow + 1, listStartCol);
        std::cout << "此页无内容";
        return;
    }

    for (int i = 0; i < listHeight && (scrollOffset + i) < currentPage.topics.size(); i++) {
        size_t itemIndex = scrollOffset + i;
        const auto& topic = currentPage.topics[itemIndex];

        Terminal::setCursor(listStartRow + i, listStartCol);

        bool isSelected = (itemIndex == currentTopicIndex);
        if (isSelected) {
            Terminal::setBackground(Color::CYAN);
            Terminal::setForeground(Color::BLACK);
        }

        std::string line = " " + topic.title;
        line.resize(listWidth, ' ');
        
        std::cout << TuiUtils::trimToUtf8VisualWidth(line, listWidth);

        if (isSelected) {
            Terminal::resetColor();
        }
    }
}

void EnhancedHelpViewer::drawDetailView() {
    TuiUtils::drawBox(detailStartRow - 1, detailStartCol - 1, 
                      detailHeight + 2, detailWidth + 2, 
                      " 详细信息 ", Color::WHITE, Color::DEFAULT);

    if (helpPages.empty() || helpPages[currentPageIndex].topics.empty()) {
        return;
    }

    const auto& topic = helpPages[currentPageIndex].topics[currentTopicIndex];
    
    // Simple text wrapping
    std::vector<std::string> lines = TuiUtils::wordWrap(topic.content, detailWidth - 2);

    for (size_t i = 0; i < lines.size() && i < (size_t)detailHeight; ++i) {
        Terminal::setCursor(detailStartRow + i, detailStartCol);
        std::cout << lines[i];
    }
}

void EnhancedHelpViewer::updateDimensions(int termRows, int termCols) {
    this->terminalRows = termRows;
    this->terminalCols = termCols;
    updateLayout();
}

std::string EnhancedHelpViewer::getStatusMessage() const {
    return statusMessage;
}
