# 打砖块 (Breakout)

经典打砖块小游戏，C++ + SDL2 实现。

## 玩法

- **← → / A D**：移动挡板
- **空格键**：发射球
- **P**：暂停/继续
- **R**：重新开始
- **N**：通关后进入下一关
- **鼠标左键**：拖动挡板

## 特性

- 7 种颜色砖块，每行分值不同
- 自动递增难度和球速
- 粒子特效
- 多关卡支持
- 中文界面

## 编译运行

### Windows (MSYS2 / MinGW)

```bash
# 安装依赖
pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-cmake

# 编译
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
make
./breakout.exe
```

### Linux

```bash
sudo apt install libsdl2-dev libsdl2-ttf-dev cmake

mkdir build && cd build
cmake ..
make
./breakout
```