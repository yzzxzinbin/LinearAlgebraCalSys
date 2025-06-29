# 线性代数计算系统 (Linear Algebra Calculation System)

## 项目简介

这是一个基于分数表示法的命令行交互式线性代数计算辅助程序，采用C++17开发，使用了部分boost库的特性。系统使用任意精度分数运算确保计算的绝对精确性，提供了丰富的线性代数功能和友好的终端用户界面(TUI)。

### 系统架构

系统采用模块化设计，主要包含以下层次：
```
┌───────────────────────────────────────┐
│         用户界面层 (TUI)               │
│  ┌─────────────┐ ┌─────────────────┐  │
│  │  应用主界面  │ │   矩阵编辑器     │  │
│  └─────────────┘ └─────────────────┘  │
└─────────────────┬─────────────────────┘
                  │
┌─────────────────┴─────────────────────┐
│       语法解析层                       │
│  ┌─────────────┐ ┌─────────────────┐  │
│  │  词法分析器  │ │   语法分析器     │  │
│  └─────────────┘ └─────────────────┘  │
│  ┌─────────────────────────────────┐  │
│  │        表达式解释器              │  │
│  └─────────────────────────────────┘  │
└─────────────────┬─────────────────────┘
                  │
┌─────────────────┴─────────────────────┐
│       计算核心层                       │
│  ┌─────────────┐ ┌─────────────────┐  │
│  │  矩阵操作类  │ │   历史记录系统   │  │
│  └─────────────┘ └─────────────────┘  │
└─────────────────┬─────────────────────┘
                  │
┌─────────────────┴─────────────────────┐
│       数据类型层                       │
│  ┌─────────────┐ ┌─────────────────┐  │
│  │   分数类     │ │     矩阵类      │  │
│  └─────────────┘ └─────────────────┘  │
│  ┌─────────────┐ ┌─────────────────┐  │
│  │   向量类     │ │     结果类      │  │
│  └─────────────┘ └─────────────────┘  │
└───────────────────────────────────────┘
```


### 核心特性

- **🎯 精确计算**: 使用Boost任意精度整形，避免浮点数误差
- **🔧 丰富功能**: 支持矩阵、向量的各种基础与高级运算
- **📖 步骤展示**: 详细记录并可视化展示计算过程
- **💻 交互界面**: TUI界面，支持命令提示、历史记录
- **📝 表达式解析**: 自然数学表达式输入，智能语法解析
- **💾 数据管理**: 变量存储、文件导入导出、CSV格式支持
- **🎨 视觉效果**: 彩色输出、矩阵可视化编辑器
- **🔄 模块设计**: 模块化，易于扩展和维护

## 使用指南

### 构建程序

如果git仓库中没有CMakeLists.txt和boost库的源代码则需要去Release中下载打包好的SourceCode.zip源代码文件
```bash
git clone https://github.com/yzzxzinbin/LinearAlgebraCalSys.git
cd ./LinearAlgebraCalSys
cmake ./
make
```

### 启动程序
 - 对于Windows平台,自行编译或者下载构建好的可执行文件并直接运行即可,不支持旧版控制台,可以在Win10新版控制台或者WindowsTerminal中运行
 - 对于Linux平台,需要先使用chmod +x 赋予执行权限
 - 对于Android平台,需要安装终端模拟器,在终端模拟器中执行ELF文件。Release中的Android版本二进制文件是在一台Android15移动设备的Termux环境上编译的,在Android14中也能运行

### 基本语法

#### 变量定义
```plaintext
a = 3/4          # 定义分数变量a
b = 2            # 定义整数变量b
m1 = [1,2,3;4,5,6]              # 矩阵定义语法
v1 = [1,2,3]                    # 向量定义语法
```

#### 基本运算表达式
```plaintext
a + b                  # 分数与整数相加 (3/4 + 2 = 11/4)
a - b                  # 分数与整数相减 (3/4 - 2 = -5/4)
a * b                  # 分数与整数相乘 (3/4 * 2 = 3/2)
b * m1                 # 数乘矩阵 (2 * [[1,2,3],[4,5,6]])
m1 + m1                # 矩阵加法 ([1,2,3],[4,5,6] + [1,2,3],[4,5,6])
m1 - m1                # 矩阵减法 ([1,2,3],[4,5,6] - [1,2,3],[4,5,6])
v1 + v1                # 向量加法 ([1,2,3] + [1,2,3])
v1 - v1                # 向量减法 ([1,2,3] - [1,2,3])
a * v1                 # 分数数乘向量 (3/4 * [1,2,3])
v1 * v1                # 向量点积 ([1,2,3]·[1,2,3] = 14)
v1 x v1                # 向量叉积 ([1,2,3]×[1,2,3] = [0,0,0])
```

#### 矩阵函数调用
```plaintext
transpose(A)                    # 矩阵转置
inverse(A)                      # 矩阵求逆(伴随矩阵法)
inverse_gauss(A)                # 矩阵求逆(高斯-若尔当消元法)
det(A)                          # 行列式计算(默认高斯消元法)
det_expansion(A)                # 行列式计算(代数余子式按行列展开)
rank(A)                         # 矩阵秩计算
ref(A)                          # 行阶梯形变换
rref(A)                         # 最简行阶梯形变换
cofactor_matrix(A)              # 代数余子式矩阵
adjugate(A)                     # 伴随矩阵
diag(v1)                        # 使用向量v1的元素创建对角矩阵
diag(f1, f2, f3)                # 使用分数f1, f2, f3创建对角矩阵
solveq(A, b)                    # 求解线性方程组 Ax = b
solveq(A)                       # 求解齐次线性方程组 Ax = 0
```

#### 线性方程组求解示例
```plaintext
# 定义系数矩阵和常数向量
A = [2,3;1,-1]                  # 2x + 3y = 7, x - y = 1
b = [7;1]                       # 常数向量

# 求解非齐次方程组
sol = solveq(A, b)              # 求解 Ax = b

# 求解齐次方程组  
sol_homo = solveq(A)            # 求解 Ax = 0

# 系统会自动判断解的性质：
# - 唯一解：rank(A) = rank([A|b]) = n
# - 无穷多解：rank(A) = rank([A|b]) < n  
# - 无解：rank(A) < rank([A|b])
```

#### 向量函数调用
```plaintext
dot(v, u)                       # 向量点积
cross(v, u)                     # 向量叉积(限三维向量)
norm(v)                         # 向量模长(的平方)
```

#### 系统命令
```plaintext
help                            # 显示帮助信息
clear                           # 清屏
clear -h                        # 清除命令历史
clear -v                        # 清除所有变量
clear -a                        # 清屏+清除历史+清除变量
vars                            # 启动变量预览器(带可视化预览窗口)
vars -l                         # 显示变量列表(仅名称和类型,包含变量总数)
show <变量名>                    # 显示指定变量
steps                           # 切换计算步骤显示模式
exit                            # 退出程序
```

#### 变量预览器功能
```plaintext
vars                            # 启动增强型变量预览器
```

变量预览器具有以下特性：
- 左侧显示变量列表，右侧显示选中变量的详细预览
- 支持上下箭头键选择变量
- 自动根据变量类型调整预览内容格式
- ESC键退出预览器返回主界面
- 对大型矩阵和长向量进行智能截断显示
- 实时响应终端大小变化

#### 变量管理命令
```plaintext
del <变量名>                     # 删除指定变量
rename <旧变量名> <新变量名>      # 重命名变量
m2 = m1                         # 复制m1到m2
```

#### 文件操作命令
```plaintext
export <文件名>                  # 导出所有变量和历史到文件
import <文件名>                  # 从文件导入变量和历史
csv <变量名>                     # 将Matrix/Vector/Result类型变量导出为CSV文件
```

#### 矩阵/向量编辑器命令
```plaintext
new <行数> <列数>                # 创建新矩阵并进入编辑器
new <维数>                      # 创建新向量并进入编辑器
edit <变量名>                   # 编辑已有矩阵或向量
```

矩阵编辑器对于Windows平台有完整的支持,对于Linux系统,部分特性可能会缺失
- 方向键控制单元格光标
- CTRL+左/右方向键:选中当前行
- CTRL+上/下方向键:选中当前列
- CTRL+ENTER:选中当前单元格
- 输入数字为选中或光标所在的单元格赋值
- CTRL+A:全选所有单元格

#### 语句结束符
```plaintext
# 大多数命令和表达式可以选择性地以分号结尾
a = 3/4;                        # 分号可选
help;                           # 分号可选
det(A);                         # 分号可选
```

#### 交互功能
- 支持**Tab键自动补全**函数名和命令
- 使用**上下箭头键**浏览历史命令
- 支持**智能语法提示**，显示可用函数和命令建议
- **步骤显示模式**: 使用`steps`命令开启/关闭详细计算过程展示


### 源代码目录

```plaintext
LACS
│  determinant_expansion.cpp
│  determinant_expansion.h
│  equationset.cpp
│  equationset.h
│  fraction.cpp
│  fraction.h
│  main.cpp
│  matrix.cpp
│  matrix.h
│  matrix_operations.cpp
│  matrix_operations.h
│  operation_step.cpp
│  operation_step.h
│  result.cpp
│  result.h
│  similar_matrix_operations.cpp
│  similar_matrix_operations.h
│  vector.cpp
│  vector.h
│
├─.vscode
│      settings.json
│
├─grammar
│      grammar_interpreter.cpp
│      grammar_interpreter.h
│      grammar_interpreter_file.cpp
│      grammar_parser.cpp
│      grammar_parser.h
│      grammar_token.h
│      grammar_tokenizer.cpp
│      grammar_tokenizer.h
│
├─tui
│      enhanced_matrix_editor.cpp
│      enhanced_matrix_editor.h
│      tui_app.cpp
│      tui_app.h
│      tui_commands.cpp
│      tui_core.cpp
│      tui_input.cpp
│      tui_step_mode.cpp
│      tui_suggestion_box.cpp
│      tui_suggestion_box.h
│      tui_terminal.cpp
│      tui_terminal.h
│      tui_utils.cpp
│
└─utils
        logger.cpp
        logger.h
```
