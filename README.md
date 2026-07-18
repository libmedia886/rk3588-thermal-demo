# RK3588 libmedia Thermal palette demo

这个示例读取调用方提供的精确 `640x480 GRAY8` 热强度图，通过公开
`MEDIA_THERMAL` API 输出彩虹、黑热、白热、铁红和蓝红五种 NV12 映射。

```bash
cmake -S . -B build -DLIBMEDIA_LIBRARY=/absolute/path/to/libmedia.so
cmake --build build -j

mkdir -p outputs
LIBMEDIA_LICENSE_PATH=/root/licence.dat \
  ./build/thermal_palette_demo outputs thermal.gray sample01
```

输入灰度值应由客户的温度范围归一化得到。切换色表只改变显示颜色，不会修改
原始温度值；测温、报警阈值和定量分析仍应使用原始温度数据。

仓库不包含 `libmedia.so`、`libmedia.a`、许可证、数据集图片、构建目录、音频或视频。
