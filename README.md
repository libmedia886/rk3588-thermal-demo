# RK3588 libmedia Thermal palette demo

这个示例生成一张 GRAY8 温度场，通过公开 `MEDIA_THERMAL` API 依次输出彩虹、
黑热、白热、铁红和蓝红五种 NV12 颜色映射。

```bash
cmake -S . -B build -DLIBMEDIA_LIBRARY=/absolute/path/to/libmedia.so
cmake --build build -j
LIBMEDIA_LICENSE_PATH=/root/licence.dat ./build/thermal_palette_demo outputs
```

仓库不包含 `libmedia.so`、`libmedia.a` 或其他预编译库。

测试图把最大输入限制为 254；当前版本在铁红模式的满量程 255 端点需要单独验证。
