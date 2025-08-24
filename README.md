# 操作系统实验环境与C程序运行说明

本项目在 **Linux 环境（WSL 或虚拟机）** 下进行操作系统实验

---

## 📌 环境准备
1. **Linux 环境**  
   - 你可以使用 **WSL（Windows Subsystem for Linux）** 或者安装 **Linux 虚拟机（如 Ubuntu）**（建议使用Ubuntu22.04，我在更高版本的运行会出问题原因布吉岛）
   - 确认已经安装**gcc,g++**和**qemu** 

2. **代码编辑器**  
   可使用 `emacs` 或 `vim` 进行代码编写，`vscode`也是极好用的工具

---

## ✏️ 编写 C 语言程序并执行
1. 打开代码编辑器并创建name的文件（name为你程序的名字，此下以reverse为例）
  emacs name.格式(e.g.reverse.c)
  ```bash
     emacs reverse.c
  ```
some operation: ctrl+x,ctrl+s:save
                    ctrl+x,ctrl+c:exit

2.运行该程序
  ```bash
     gcc -o reverse reverse.c -Wall
 ```
  这个命令是用于编译 C 语言源代码文件 reverse.c，并生成一个名为 reverse 的可执行文件。具体解释如下：
    gcc: GNU Compiler Collection 的缩写，是一个常用的 C 语言编译器。
    -o reverse: -o 选项用于指定生成的可执行文件的名称。在这里，生成的可执行文件将被命名为 reverse。
    reverse.c: 这是你要编译的 C 语言源代码文件。
    -Wall: 这个选项告诉编译器启用所有常见的警告信息。它可以帮助你发现代码中的潜在问题。
    
---
## ✏️ 执行测试脚本

1.修改文件 test-reverse.sh 的权限，使其对所有用户（所有者、组用户和其他用户）都具有读、写和执行的权限
  ```bash
     sudo chmod 777 test −reverse . sh
 ```

2.运行一个名为 test-reverse.sh 的脚本文件
  ```bash
    ./test-reverse.sh
 ```
PS.如果 test-reverse.sh 文件没有执行权限，运行时会报错：Permission denied
   你可以只为文件添加执行权限。可以使用以下命令：
 ```bash
   chmod +x test-reverse.sh
 ```
