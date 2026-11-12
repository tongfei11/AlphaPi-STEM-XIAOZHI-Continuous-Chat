# AlphaPi STEM 小智连续对话固件

基于 [shj4588/AlphaPi-STEM-XIAOZHI](https://github.com/shj4588/AlphaPi-STEM-XIAOZHI) 的 V2.3.2，适用于 AlphaPi STEM / Zero 的 ESP32-C3、4 MB Flash 和 5×5 点阵。本 fork 的固件标记为 `V2.3.2 local-0014`。

0014 将已验证的 0013 整理为独立固件仓库，录音、连续对话和阈值与 0013 相同。上游基线为 `2ae2d058831518468d68ce415d04cb1892d4ffe2`，原项目说明保存在 [README.upstream.md](README.upstream.md)。

可从 [Releases](https://github.com/tongfei11/AlphaPi-STEM-XIAOZHI-Continuous-Chat/releases) 下载已编译的主程序 `.bin` 和 SHA-256 校验文件。0014 的主程序写入地址为 `0x10000`。

## 按键

| 按键 | 短按 | 长按 |
| --- | --- | --- |
| A | 音量减 | 约 2 秒，打开或关闭配网 |
| B | 音量加 | 约 0.65 秒，开启连续对话 |
| C | 停止回复并退出会话 | 约 0.35 秒开始录音，松开发送一句 |

连续对话时，看到麦克风图标后说话。停顿约 0.9 秒自动提交，菱形表示等待回复，喇叭表示进入播放，播完后再次聆听。约 10 秒未检测到说话会退出。检测到说话且音频已上传时，每轮最长录音 15 秒。

连续模式的起音阈值至少 1600，持续语音阈值至少 1200，需要连续 3 帧（约 180 毫秒）达到起音阈值。更高底噪会继续抬高自适应阈值。这些是麦克风采样幅度单位，与收音测试页的 P90 同口径。

当前会依次录音、播放。小智播放期间可以短按 C 停止会话，再长按 B 开始连续对话，或长按 C 说一句。

## 点阵与休眠

连接路由器时显示旋转动画，连上后短显 WiFi 图案，再以笑脸待机。录音显示麦克风，等待连续模式回复显示菱形，普通回复播放时显示喇叭。图案代表设备所处的流程，实际音频以扬声器输出为准。

空闲约 60 秒后熄屏并深睡。B、C 可唤醒，按住直到重新联网，可以进入对应对话模式。A 长按的配网开关仍以 WiFi 图案或 X 提示。

## 配网与收音测试

长按 A 打开配网热点，手机连接 `AlphaPi-XZ-xxxx`，浏览器访问 `http://192.168.4.1/`，可设置 WiFi、官方服务器或本地服务器。

排查底噪误触发时，先短按 C 结束对话，在配网页进入「收音测试」，或访问 `http://192.168.4.1/mic`。先保持安静测底噪，再用平时的距离和音量测说话。每次准备约 1.5 秒，测量约 4 秒，页面显示 RMS、峰值、P90 和采样时长。测试只在板上统计电平，测量结果保留到重启。

RMS 使用相对数字满幅的 dBFS，越接近 0 越响。它和 P90 用于比较同一麦克风的底噪与人声。当前阈值参考一次实机测量，底噪为 −33.2 dBFS、P90 826，人声为 −17.6 dBFS、P90 4859。环境和说话距离变化可能影响分句效果。

## 编译

依赖 Arduino CLI 和以下固定版本。

| 依赖 | 版本 |
| --- | --- |
| ESP32 Arduino core | 2.0.14 |
| WebSockets（Links2004） | 2.7.2 |
| arduino-libopus（Phil Schatzmann） | 1.3.2 |

macOS 中运行以下脚本。它会在项目 `.build/arduino/` 准备隔离依赖，并将源码暂存为 Arduino 要求的同名草图目录。

```sh
./build.command
```

输出主程序为 `output/0014/AlphaPi-Xiaozhi-0014.bin`，写入地址为 `0x10000`。使用 Arduino IDE 时，将 `.ino`、两个 `.h` 和 `build_opt.h` 一起放进 `AlphaPi_Xiaozhi/` 目录，再打开 `.ino`。

目标板为 `ESP32C3 Dev Module`，Flash 4 MB，分区选择 `Huge APP (3MB No OTA)`。`build_opt.h` 中的 `-DUSE_ALLOCA` 必须参与 Opus 编译。库默认的 60 KB 堆工作区曾在录音时分配失败并触发重启，任务栈分配与可用的原版镜像一致。

升级已运行原版 V2.3.2 的设备时，只将主程序写入 `0x10000` 并保留 NVS，即可保留 WiFi、服务器和激活配置。烧录前请确认芯片、Flash 大小和分区与本项目匹配。

## 验证

使用 macOS 的 Clang 可运行按键、分句和麦克风电平测试。

```sh
mkdir -p .build/tests
clang++ -std=c++11 -Wall -Wextra -Werror tests/talk_controls_test.cpp -o .build/tests/talk-controls
.build/tests/talk-controls
clang++ -std=c++11 -Wall -Wextra -Werror tests/mic_levels_test.cpp -o .build/tests/mic-levels
.build/tests/mic-levels
```

0013 已在用户设备上验证语音回复、连续对话和按键体验。0014 的仓库整理版本完成编译与离线测试，仍需在目标设备上确认。离线分句测试使用测量电平构造的合成序列。
