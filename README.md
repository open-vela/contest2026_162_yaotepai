# SF23LB58 在 OpenVela 上的移植

## 一、作品简介
本项目实现了将 SF32LB58 移植到 OpenVela 上，以及使用 nxboot 作为 Bootloader，用以引导 OpenVela 的主程序。

## 二、选题方向
新硬件适配

原本计划基于 SF32LB58 开发一款 AI 智能眼镜项目，但是由于时间与精力问题，暂缩减为一个新平台移植项目。 SF32LB58 拥有 3个核心，双模蓝牙以及 2D GPU，可以使用非常小尺寸的带显示的 AI 眼镜产品。
 
## 三、目录结构
- `board/sf32lb58/sf32lb58-lcd_n16r32n1_qspi` —  用于构建主应用
- `board/sf32lb58/sf32lb58_n16r32n1-nxboot` —  用于构建 nxboot
- `logs/`           — AI Coding 日志
- `chips`    — sf32lb58 SoC 相关的驱动
- `scripts`  - 开发测试用的脚本，固件及二进制包。

## 四、运行方式

### 更新代码

```shell
repo init -u https://github.com/open-vela/contest2026_162_yaotepai \
  -b dev-ai-contest-2026 -m contest2026_162_yaotepai.xml
repo sync -c -j8
```

```shell
# 提前准备 python 环境，比如先建立 venv环境
pip install -r apps/boot/nxboot/tools/requirements.txt
```

### 编译

1. 编译 nxboot
```shell
./build.sh contest2026_162_yaotepai/board/sf32lb58/sf32lb58_n16r32n1-nxboot/configs/boot --cmake -j8
```
2. 编译 application
```shell
./build.sh contest2026_162_yaotepai/board/sf32lb58/sf32lb58-lcd_n16r32n1_qspi/configs/bt_nsh --cmake -j8
```

3. 为 nxboot 准备镜像
```shell
python apps/boot/nxboot/tools/nximage.py --primary -v cmake_out/sf32lb58-lcd_n16r32n1_qspi_bt_nsh/nuttx.bin nuttx-slot-primary-n16.bin
```

4. 烧路固件

创建一个 download.jlink 文件

```jlink
/* download.jlink */
device SF32LB58X
si SWD
speed 2000
r

loadbin nxboot-n16.bin 0x1C020000
loadbin nuttx-slot-primary-n16.bin 0x18000000
loadbin ftab_lb58_n16.bin 0x1C000000
exit
```

```shell
cp cmake_out/sf32lb58_n16r32n1-nxboot_boot/nuttx.bin nxboot-n16.bin
cp contest2026_162_yaotepai/scripts/ftab_lb58_n16.bin .
JLinkExe -CommandFile download_lb58_n16.jlink
```

## 五、AI Coding 使用说明

项目大部分的代码工作都由 AI 完成，并且通过一个 sf32lb58-flash-debug.skill 来实现 AI 调用 JLink 自动刷新固件，甚至可以调用 gdb 进行硬件上的调试操作。
