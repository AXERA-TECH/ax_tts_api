# ax_tts_api
C++ TTS API on Axera platforms

支持平台:  
 - AX650
 - AX630C
 - AX620Q
 - AX8850

支持模型:
 - Kokoro

## 文档目录  
- [快速开始](#快速开始)  
- [下载模型](#下载模型)  
- [编译](#编译)  
- [测试](#测试)  
- [性能表现](#性能表现)  
- [集成](#集成)  
- [讨论](#讨论)  

## 更新

## 快速开始

可从Release页面下载预编译库  

使用示例: [test_kokoro](tets/test_kokoro.cpp)

## 下载模型

安装huggingface_hub
```bash
pip3 install -U huggingface_hub
```

运行下载脚本:
```bash
bash download_models.sh
```

## 编译

### 依赖

#### 系统要求

目前在Ubuntu 22.04上编译成功,  
需要安装CMake >= 3.13  

```bash
sudo apt install cmake build-essential
```

#### 获取交叉编译器

 - AX650/AX630C(aarch64)
从[此处](https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz)获取aarch64交叉编译器  
将其添加到PATH:
```bash
export PATH=$PATH:path of gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin
```

 - AX620Q(arm-uclibc-linux)
从[此处](https://github.com/AXERA-TECH/ax620q_bsp_sdk/releases/download/v2.0.0/arm-AX620E-linux-uclibcgnueabihf_V3_20240320.tgz)获取
```bash
export PATH=$PATH:path of arm-AX620E-linux-uclibcgnueabihf/bin
```

 - AX620Q(arm-linux-gnueabihf, glibc)
```bash
export PATH=$PATH:path of arm-linux-gnueabihf/bin
```

### 获取BSP

```bash
bash download_bsp.sh
```

### 交叉编译

 - AX650
 ```bash
 bash build_ax650.sh
 ```
 编译完成后的产物在install/ax650下

 - AX630C
 ```bash
 bash build_ax630c.sh
 ```
  编译完成后的产物在install/ax630c下

 - AX620Q
 ```bash
 bash build_ax620q.sh
 ```
  编译完成后的产物在install/ax620q下

 - AX620Q (arm-linux-gnueabihf + glibc)
 ```bash
 bash build_620qp.sh
 ```
  编译完成后的产物在install/ax620qp下  
  如需自定义 MSP 路径可传环境变量：`BSP_MSP_DIR=/path/to/msp/out/arm_glibc bash build_620qp.sh`

 - AX8850
 ```bash
 bash build_ax8850_aarch64.sh.sh
 ```
  编译完成后的产物在install/ax8850_aarch64下

### 本地编译

暂不支持

### 其它编译选项

 - BUILD_TESTS 默认OFF  
 负责编译tests目录下的单元测试，可执行程序生成在install/ax650或install/ax630c下  
 ```bash
 bash build_ax650.sh -DBUILD_TESTS=ON
 ```

  - LOG_LEVEL_DEBUG 默认OFF  
  打印源码中的调试信息  
  ```bash
  bash build_ax650.sh -DLOG_LEVEL_DEBUG=ON
  ```    

  - BUILD_SERVER 默认ON   
  编译asr_server  
  ```bash
  bash build_ax650.sh -DBUILD_SERVER=ON
  ```    

## 测试

### 主程序  

```
./install/ax650/main -l en -t "Hello, World!" -o en.wav

```

Usage:  
```
usage: ./install/ax650/main --language=string --text=string --output=string [options] ...
options:
  -l, --language    Language, in ISO-639 format (string)
  -t, --text        Input text (string)
  -o, --output      Output wav path (string)
  -?, --help        print this message

```

### 服务端(asr_server)

```
./install/ax8850_aarch64/tts_server --port 8080

```

Usage:  
```
usage: ./install/ax650/tts_server [options] ...
options:
  -p, --port          On which port to run the server (int [=8080])
  -m, --model_path    model path which contains axmodel (string [=./models-ax650])
  -?, --help          print this message


```

### 客户端

#### Python

```
cd scripts
pip install openai
python test_tts_server.py --ip 10.126.33.140 --port 8080 -t "Hello, World" --output test_en
```
Check python test_tts_server.py --help for help.  

#### 目标设备冒烟测试（无需 ctest）

在目标设备启动服务后，可在任意机器执行：

```bash
cd scripts
./device_smoke_test.sh 10.126.33.188 8080 /tmp/tts_smoke
```

该脚本会验证：
- 英文可用
- 中文可用（需服务启动时带 `--jieba_dict_path`）
- 日语假名可用
- 日语片假名可用
- 日语长句（自动分块）可用
- 日语含汉字输入会被拒绝（HTTP 400）

#### 重要说明
- 日语仅支持假名（平假名/片假名）。含汉字输入会返回 400。
- Jieba 词典默认使用 mmap 加载（编译宏 `AX_TTS_JIEBA_USE_MMAP`，默认 ON）。
- 为提升日语长句稳定性，默认关闭日语短句 doubling（`AX_TTS_JA_ENABLE_SHORT_DOUBLE=OFF`）。
- 日语长句分块后会尝试合并过短片段，阈值由 `AX_TTS_JA_MIN_CHUNK_TOKENS` 控制（默认 `24`）。


### 单元测试

以下为tests下单元测试的使用示例和说明:

- test_kokoro: 加载kokoro模型，测试中英文的生成结果


## 性能表现

RTF(Real Time Factor)为推理时间除以音频时长，越小表示越快  
TTS的RTF表现与文本长度和前端实现有很强的关系，此处仅列出test_kokoro的结果

- AX650:
```bash
================================
test_en:
input text: Hello, World!
output duration: 1.40 seconds
output file: test_en.wav
RTF(0.78 / 1.40) = 0.5593

================================
test_zh:
input text: 一切有为法，如梦幻泡影，如露亦如电，应作如是观。
output duration: 4.22 seconds
output file: test_zh.wav
RTF(0.54 / 4.22) = 0.1280

```

- AX8850:
```bash
================================
test_en:
input text: Hello, World!
output duration: 1.40 seconds
output file: test_en.wav
RTF(0.52 / 1.40) = 0.3736

================================
test_zh:
input text: 一切有为法，如梦幻泡影，如露亦如电，应作如是观。
output duration: 4.22 seconds
output file: test_zh.wav
RTF(0.37 / 4.22) = 0.0880

```

## 集成

编译产物包含 include/ax_tts_api.h 和 lib/libax_tts_api.so

## 讨论

 - Github issues
 - QQ 群: 139953715

## 贡献

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
