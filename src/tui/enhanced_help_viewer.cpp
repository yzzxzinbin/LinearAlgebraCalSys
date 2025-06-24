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
    updateStatus("帮助查看器：↑/↓ 选择主题, ← /→ 翻页, ESC 退出");
}

EnhancedHelpViewer::~EnhancedHelpViewer() {
    std::iostream::sync_with_stdio(true);
}

void EnhancedHelpViewer::initializeHelpContent() {
    // 第1页：基础命令
    helpPages.push_back({
        "基础命令",
        {
            {"\033[1;36mhelp\033[22m", 
             "显示此帮助信息，进入增强型帮助查看器。\n\n"
             "\033[1m操作:\033[0m\n"
             "- \033[1;32m↑/↓\033[0m: 选择主题\n"
             "- \033[1;32m←/→\033[0m: 翻页\n"
             "- \033[1;32mESC\033[0m: 退出\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> help\033[0m\n"
             "\033[36m[效果: 打开帮助浏览器界面]\033[0m"
            },
            {"\033[1;36mclear\033[22m", 
             "清空屏幕、历史或变量。\n\n"
             "\033[1m用法:\033[0m\n"
             "- clear: 清空屏幕\n"
             "- clear -h: 清除命令历史\n"
             "- clear -v: 清除所有变量\n"
             "- clear -a: 清空屏幕+历史+变量\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> clear -v\033[0m\n"
             "\033[36m[效果: 所有变量已清除]\033[0m"
            },
            {"\033[1;36mvars\033[22m", 
             "显示所有已定义的变量。\n\n"
             "\033[1m用法:\033[0m\n"
             "- vars: 打开增强型变量预览器\n"
             "- vars -l: 简单列表模式，显示变量名和类型\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> vars -l\033[0m\n"
             "\033[36m  m1 : 矩阵 (2×2)\n  v1 : 向量 (3维)\n  f1 : 分数\n  ...\033[0m"
            },
            {"\033[1;36mexit\033[22m", 
             "退出程序。\n\n"
             "\033[1m用法:\033[0m\n"
             "- exit: 正常退出（自动保存）\n"
             "- exit --no-saving: 退出时不保存变量和历史\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> exit --no-saving\033[0m\n"
             "\033[36m[效果: 退出且不保存工作区]\033[0m"
            },
            {"\033[1;36msteps\033[22m", 
             "切换计算步骤显示开关。\n\n"
             "当开启时，计算过程会显示详细的中间步骤，便于理解和验证计算过程。\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> steps\033[0m\n"
             "\033[36m[效果: 计算步骤显示已开启]\033[0m"
            },
            {"\033[1;36mshow\033[22m", 
             "显示变量内容，支持格式化输出。\n\n"
             "\033[1m用法:\033[0m\n"
             "- show <var>: 显示原始格式\n"
             "- show <var> -f<精度>: 有效数字格式\n"
             "- show <var> -p<小数位>: 小数格式\n"
             "- show <var> -r <结果变量>: 保存格式化结果(需结合-p或-f选项)\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> show f1 -f4\033[0m\n"
             "\033[36m= 0.5\033[0m\n"
             "\033[1;33m> show m1 -p2\033[0m\n"
             "\033[36m| 1.00 2.00 3.00 |\n| 4.00 5.00 6.00 |\033[0m"
            },
            {"\033[1;36mdel\033[22m", 
             "删除指定的变量。\n\n"
             "\033[1m用法:\033[0m del <变量名>\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> del m1\033[0m\n"
             "\033[36m[效果: 变量 'm1' 已删除]\033[0m"
            },
            {"\033[1;36mrename\033[22m", 
             "重命名变量。\n\n"
             "\033[1m用法:\033[0m rename <旧名> <新名>\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> rename m1 matrix_a\033[0m\n"
             "\033[36m[效果: 变量 'm1' 已重命名为 'matrix_a']\033[0m"
            },
            {"\033[1;36mnew\033[22m", 
             "创建新的矩阵或向量并进入编辑器。\n\n"
             "\033[1m用法:\033[0m\n"
             "- new <维度>: 创建向量\n"
             "- new <行数> <列数>: 创建矩阵\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> new 3\033[0m\n"
             "\033[36m[效果: 创建3维向量并进入编辑器]\033[0m"
            },
            {"\033[1;36medit\033[22m", 
             "编辑已存在的矩阵或向量。\n\n"
             "\033[1m用法:\033[0m edit <变量名>\n"
             "\n\033[1m编辑器操作:\033[0m\n"
             "- ↑↓←→: 移动光标\n"
             "- 数字/分数: 输入值\n"
             "- CTRL+ENTER:选中单元格\n"
             "- CTRL+A:全选\n"
             "- CTRL+方向键:选中一行/列\n"
             "- ESC: 退出编辑器\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> edit m1\033[0m\n"
             "\033[36m[效果: 进入矩阵编辑器]\033[0m"
            }
        }
    });

    // 第2页：变量操作
    helpPages.push_back({
        "变量操作",
        {
            {"\033[1;36m矩阵定义\033[22m", 
             "使用方括号和分号定义矩阵。\n\n"
             "\033[1m格式:\033[0m 变量名 = [元素1,元素2;元素3,元素4]\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> m1 = [1,2,3;4,5,6]\n> identity = [1,0;0,1]\n> zeros = [0,0,0;0,0,0]\033[0m\n"
             "\033[36m[效果: 定义矩阵变量]\033[0m"
            },
            {"\033[1;36m向量定义\033[22m", 
             "使用方括号定义向量（一行矩阵）。\n\n"
             "\033[1m格式:\033[0m 变量名 = [元素1,元素2,元素3]\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> v1 = [1,2,3]\n> v2 = [0,1,0]\n> position = [3,4,5]\033[0m\n"
             "\033[36m[效果: 定义向量变量]\033[0m"
            },
            {"\033[1;36m分数定义\033[22m", 
             "使用斜杠定义分数。\n\n"
             "\033[1m格式:\033[0m 变量名 = 分子/分母\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> f1 = 1/2\n> f2 = 3/4\n> f3 = -5/7\033[0m\n"
             "\033[36m[效果: 定义分数变量，自动化简]\033[0m"
            },
            {"\033[1;36m复杂表达式\033[22m", 
             "可以使用表达式直接定义变量。\n\n"
             "\033[2m示例:\033[0m\n"
             "\033[1;33m> result = m1 * m2\n> sum_vec = v1 + v2\n> det_val = det(m1)\033[0m\n"
             "\033[36m[效果: 变量可由表达式赋值]\033[0m"
            },
            {"\033[1;36m矩阵运算\033[22m", 
             "矩阵支持加减乘运算。\n\n"
             "\033[1m运算符:\033[0m\n"
             "- +: 矩阵加法（同型矩阵）\n"
             "- -: 矩阵减法（同型矩阵）\n"
             "- *: 矩阵乘法（左矩阵列数=右矩阵行数）\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> sum = m1 + m2\n> diff = m1 - m2\n> product = m1 * m2\033[0m\n"
             "\033[36m[效果: 计算矩阵加减乘]\033[0m"
            },
            {"\033[1;36m向量运算\033[22m", 
             "向量支持加减、点积、叉积运算。\n\n"
             "\033[1m运算符:\033[0m\n"
             "- +: 向量加法\n"
             "- -: 向量减法\n"
             "- *: 向量点积（返回分数）\n"
             "- x: 向量叉积（仅限3维向量）\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> sum = v1 + v2\n> dot_product = v1 * v2\n> cross_product = v1 x v2\033[0m\n"
             "\033[36m[效果: 计算向量加减/点积/叉积]\033[0m"
            },
            {"\033[1;36m分数运算\033[22m", 
             "分数支持四则运算。\n\n"
             "\033[1m运算符:\033[0m +, -, *, /\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> sum = f1 + f2\n> product = f1 * f2\n> quotient = f1 / f2\033[0m\n"
             "\033[36m[效果: 分数自动化简]\033[0m"
            },
            {"\033[1;36m混合运算\033[22m", 
             "矩阵与标量、向量与标量的运算。\n\n"
             "\033[1m支持的运算:\033[0m\n"
             "- 矩阵 * 标量\n"
             "- 标量 * 矩阵\n"
             "- 向量 * 标量\n"
             "- 标量 * 向量\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> scaled = 2 * m1\n> doubled = v1 * 2\033[0m\n"
             "\033[36m[效果: 结果为放大后的矩阵/向量]\033[0m"
            }
        }
    });

    // 第3页：矩阵函数
    helpPages.push_back({
        "矩阵函数",
        {
            {"\033[1;36mtranspose()\033[22m", 
             "计算矩阵转置。\n\n"
             "\033[1m用法:\033[0m transpose(matrix)\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> mt = transpose(m1)\033[0m\n"
             "\033[36m[效果: mt 为 m1 的转置矩阵]\033[0m"
            },
            // 拆分 det
            {"\033[1;36mdet()\033[22m", 
             "计算方阵的行列式（高斯消元法）。\n\n"
             "\033[1m用法:\033[0m det(matrix)\n"
             "\033[1m说明:\033[0m 适用于任意阶方阵，内部采用高斯消元法，速度快，支持分数精度。\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> d = det(m1)\033[0m\n"
             "\033[36m[效果: 结果为分数型行列式值]\033[0m"
            },
            // 新增 det_expansion
            {"\033[1;36mdet_expansion()\033[22m", 
             "按行列展开法计算方阵的行列式。\n\n"
             "\033[1m用法:\033[0m det_expansion(matrix)\n"
             "\033[1m说明:\033[0m 适用于小型方阵（如3阶及以下），可显示详细展开步骤。\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> d2 = det_expansion(m1)\033[0m\n"
             "\033[36m[效果: 结果为分数型行列式值，支持详细步骤显示]\033[0m"
            },
            {"\033[1;36minverse()\033[22m", 
             "计算方阵的逆矩阵。\n\n"
             "\033[1m用法:\033[0m\n"
             "- inverse(matrix): 伴随矩阵法\n"
             "- inverse_gauss(matrix): 高斯-若尔当法\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> inv1 = inverse(m1)\n> inv2 = inverse_gauss(m1)\033[0m\n"
             "\033[36m[效果: 结果为逆矩阵]\033[0m"
            },
            {"\033[1;36mrank()\033[22m", 
             "计算矩阵的秩。\n\n"
             "\033[1m用法:\033[0m rank(matrix)\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> r = rank(m1)\033[0m\n"
             "\033[36m[效果: r 为秩的整数值]\033[0m"
            },
            // 拆分 ref
            {"\033[1;36mref()\033[22m", 
             "化简为行阶梯形（高斯消元法）。\n\n"
             "\033[1m用法:\033[0m ref(matrix)\n"
             "\033[1m说明:\033[0m 只进行前向消元，主元下方全为0，主元不一定为1。\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> ref_form = ref(m1)\033[0m\n"
             "\033[36m[效果: 结果为行阶梯形矩阵]\033[0m"
            },
            // 拆分 rref
            {"\033[1;36mrref()\033[22m", 
             "化简为最简行阶梯形（高斯-若尔当消元法）。\n\n"
             "\033[1m用法:\033[0m rref(matrix)\n"
             "\033[1m说明:\033[0m 主元为1，主元所在列其他元素全为0。\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> rref_form = rref(m1)\033[0m\n"
             "\033[36m[效果: 结果为最简行阶梯形矩阵]\033[0m"
            },
            {"\033[1;36mcofactor_matrix()\033[22m", 
             "计算代数余子式矩阵。\n\n"
             "\033[1m用法:\033[0m cofactor_matrix(matrix)\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> cof = cofactor_matrix(m1)\033[0m\n"
             "\033[36m[效果: 结果为代数余子式矩阵]\033[0m"
            },
            {"\033[1;36madjugate()\033[22m", 
             "计算伴随矩阵。\n\n"
             "\033[1m用法:\033[0m adjugate(matrix)\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> adj = adjugate(m1)\033[0m\n"
             "\033[36m[效果: 结果为伴随矩阵]\033[0m"
            }
        }
    });

    // 第4页：高级功能
    helpPages.push_back({
        "高级功能",
        {
            {"\033[1;36mdiag()\033[22m", 
             "使用向量创建对角矩阵。\n\n"
             "\033[1m用法:\033[0m diag(vector)\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> v = [1,2,3]\n> diagonal = diag(v)\033[0m\n"
             "\033[1;33m> diagonal2 = diag(1, 2, 3)\033[0m\n"
             "\033[36m[效果: diagonal 为3×3对角矩阵]\033[0m\n"
             "\ndiagnal = \n"
                "    \033[36m|\033[0m 1 0 0 \033[36m|\033[0m\n"
                "    \033[36m|\033[0m 0 2 0 \033[36m|\033[0m\n"
                "    \033[36m|\033[0m 0 0 3 \033[36m|\033[0m\n"
            },
            {"\033[1;36msolveq()\033[22m", 
             "求解线性方程组。\n\n"
             "\033[1m用法:\033[0m\n"
             "- solveq(A): 求解 Ax = 0\n"
             "- solveq(A, b): 求解 Ax = b\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> solution = solveq(m1, v1)\033[0m\n"
             "\033[36m[效果: solution 为解向量或解集]\033[0m"
            },
            {"\033[1;36munion_rref()\033[22m", 
             "向量组联合化行最简形。\n\n"
             "\033[1m用法:\033[0m union_rref(A, B)\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> combined = union_rref(m1, m2)\033[0m\n"
             "\033[36m[效果: 结果为合并并化简后的矩阵]\033[0m"
            },
            {"\033[1;36mRS_rep_vecset()\033[22m", 
             "计算向量组的线性表示。\n\n"
             "\033[1m用法:\033[0m RS_rep_vecset(A, B)\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> representation = RS_rep_vecset(m1, m2)\033[0m\n"
             "\033[36m[效果: 解析A B是否能互相线性表示]\033[0m"
            },
            {"\033[1;36mrep_vecsingle()\033[22m", 
             "判断向量组能否线性表示指定向量。\n\n"
             "\033[1m用法:\033[0m rep_vecsingle(A, v)\n"
             "\033[1m参数:\033[0m\n"
             "- A: 向量组（矩阵或向量）\n"
             "- v: 目标向量（不能全为0）\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> coeffs = rep_vecsingle(m1, v1)\033[0m\n"
             "\033[36m[效果: 若能线性表示，返回系数列矩阵；否则返回全0列矩阵]\033[0m"
            },
            {"\033[1;36mmax_independentset_col()\033[22m",
             "计算矩阵的极大线性无关列向量组。\n\n"
             "\033[1m用法:\033[0m max_independentset_col(A)\n"
             "\033[1m参数:\033[0m\n"
             "- A: 矩阵\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> basis = max_independentset_col(m1)\033[0m\n"
             "\033[36m[效果: 返回m1的极大线性无关列向量组（子矩阵）]\033[0m"
            },
            {"\033[1;36mmax_independentset_row()\033[22m",
             "计算矩阵的极大线性无关行向量组。\n\n"
             "\033[1m用法:\033[0m max_independentset_row(A)\n"
             "\033[1m参数:\033[0m\n"
             "- A: 矩阵\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> basis = max_independentset_row(m1)\033[0m\n"
             "\033[36m[效果: 返回m1的极大线性无关行向量组（子矩阵）]\033[0m"
            },
            {"\033[1;36m向量函数\033[22m", 
             "向量专用函数。\n\n"
             "\033[1m函数:\033[0m\n"
             "- dot(v1, v2): 点积\n"
             "- cross(v1, v2): 叉积\n"
             "- norm(v): 向量模长\n"
             "- normalize(v): 单位化\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> length = norm(v1)\n> unit = normalize(v1)\033[0m\n"
             "\033[36m[效果: 计算模长或单位向量]\033[0m"
            }
        }
    });

    // 第5页：文件操作
    helpPages.push_back({
        "文件操作",
        {
            {"\033[1;36mexport\033[22m", 
             "导出变量和历史到文件。\n\n"
             "\033[1m用法:\033[0m export <文件名>/<\"绝对路径\">\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> export session.dat\033[0m\n"
             "\033[36m[效果: 保存所有变量和命令历史]\033[0m"
            },
            {"\033[1;36mimport\033[22m", 
             "从文件导入变量和历史。\n\n"
             "\033[1m用法:\033[0m import <文件名>\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> import \"K:\\rebel.txt\"\n> import session.dat\033[0m\n"
             "\033[36m[效果: 加载变量和历史记录]\033[0m"
            },
            {"\033[1;36mcsv\033[22m", 
             "导出变量为CSV格式。\n\n"
             "\033[1m用法:\033[0m csv <变量名>\n"
             "\033[1m\033[38;5;196m注意:\033[0m 仅支持Matrix, Vector, 或 Result 类型导出为CSV格式\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> csv m1\n> csv result_matrix\033[0m\n"
             "\033[36m[效果: 生成 <变量名>.csv 文件]\033[0m"
            },
            {"\033[1;36m自动保存\033[22m", 
             "程序退出时自动保存。\n\n"
             "\033[1m特性:\033[0m\n"
             "- 正常退出时自动保存到启动时选择的工作文件\n"
             "- 意外退出时会执行基于系统平台的进程退出回调,不确保一定能成功保存\n"
             "- 使用 exit --no-saving 可跳过保存\n"
             "- 若启动时未选择工作文件，则不进行自动保存\n"
             "\n\033[1m文件位置:\033[0m 程序当前目录"
            },
        }
    });

    // 第6页：代数运算
    helpPages.push_back({
        "代数运算",
        {
            {"\033[1;36m代数表达式\033[22m", 
             "本系统支持对单变量多项式进行化简、因式分解和求解。\n\n"
             "\033[1m格式:\033[0m\n"
             "- 变量必须是单个字母 (例如 x, y, a)。\n"
             "- 系数可以是整数或分数 (例如 2, -5, 3/4)。\n"
             "- 支持 `+`, `-`, `*`, `^` 运算符。\n"
             "- 表达式中的空格会被忽略。\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> alg_simplify(x^2 - 4)\n> result = alg_factor(2*y^3 + 1/2*y - 5)\033[0m"
            },
            {"\033[1;36malg_simplify\033[22m", 
             "化简一个代数表达式。\n\n"
             "\033[1m用法:\033[0m alg_simplify(<表达式>)\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> alg_simplify(3*x + 2 - x + x^2)\033[0m\n"
             "\033[36m[结果: x^2 + 2*x + 2]\033[0m"
            },
            {"\033[1;36malg_factor\033[22m", 
             "对一个代数表达式进行因式分解。\n\n"
             "\033[1m用法:\033[0m alg_factor(<表达式>)\n"
             "\033[1m注意:\033[0m 目前主要支持二次多项式和简单公因式提取。\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> alg_factor(2*x^2 - 8)\033[0m\n"
             "\033[36m[结果: 2 * (x - 2) * (x + 2)]\033[0m"
            },
            {"\033[1;36malg_solve\033[22m", 
             "求解代数方程 (表达式 = 0)。\n\n"
             "\033[1m用法:\033[0m alg_solve(<表达式>)\n"
             "\n\033[2m示例:\033[0m\n"
             "\033[1;33m> alg_solve(x^2 - 5*x + 6)\033[0m\n"
             "\033[36m[结果: x = 2, x = 3]\033[0m"
            }
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

        // 显示文本处理
        std::string text = topic.title;
        size_t textWidth = TuiUtils::calculateUtf8VisualWidth(text);
        int availWidth = listWidth - 2; // 两侧空格
        if ((int)textWidth > availWidth) {
            text = TuiUtils::trimToUtf8VisualWidth(text, availWidth - 3) + "...";
            textWidth = TuiUtils::calculateUtf8VisualWidth(text);
        }
        int padding = availWidth - textWidth;
        if (padding < 0) padding = 0;
        std::string line = " " + text + std::string(padding, ' ') + " ";

        std::cout << line;

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
