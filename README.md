该工程为CMake工程，工具链为arm-none-eabi，如果你配置的环境是MDK-ARM，可以用STM32CubeMX打开stm32-uwb.ioc，重新生成MDK工程，然后将Core/Src文件夹下的非cubemx生成的文件导入工程中。

编译工程（如果使用MDK，添加完文件后直接点击UI界面的编译）：
```sh
cd ./stm32-uwb/
mkdir build
cd build
cmake ..
make
```

在编译时可以加入一些宏定义以修改参数：

| 选项         | 宏                          | 类型 | 可取值                                           |
| ------------ | --------------------------- | ---- | ------------------------------------------------ |
| 标签扫描时间 | UWB_SCAN_INTERVAL           | int  | 100~1000（单位：ms）（默认500ms）                |
| 报警距离阈值 | UWB_DIST_THRESHOLD          | int  | 0~65535（单位：cm）（默认100cm）                 |
| 报警方式     | ALERT_MODE                  | int  | 1：使用蜂鸣器，0：使用led灯（默认0）             |
| 高性能模式   | UWB_ENABLE_HIGH_PERFORMANCE | int  | 1：启用高性能模式，0：关闭高性能模式（默认开启） |
| UWB扫描频道  | UWB_SCAN_CHANNEL            | int  | 5：使用频道5，9：使用频道9（默认5）              |

配置方式

1. cmake：

   ```sh
   cd ./stm32-uwb/
   mkdir build
   cd build
   cmake -D CMAKE_C_FLAGS="-DMACRO1=xxx -DMACRO2=xxx" ..
   make
   ```

   将`MACRO`替换为表格中的**宏**一栏中对应的一个，`xxx`为你想配置的值，例如设置扫描时间为300ms，报警方式为使用led：

   ```sh
   cmake -D CMAKE_C_FLAGS="-DUWB_SCAN_INTERVAL=300 -DALERT_MODE=0" ..
   ```

2. MDK：

   在魔术棒界面中，C/C++选项卡下的Define一栏中填入`MACRO=xxx`，多个宏以空格分开。

编译后生成的.elf文件在build文件夹下，windows可以使用st programmer进行烧录，linux可以使用openocd进行烧录，烧录接口为SWD。
