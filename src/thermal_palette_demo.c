#include "media_api.h"

#include <linux/dma-buf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define OUT_POOL 0
#define IN_POOL 6
#define GRP 0
#define WIDTH 640
#define HEIGHT 480
#define IN_SIZE ((size_t)WIDTH * HEIGHT)
#define OUT_SIZE ((size_t)WIDTH * HEIGHT * 3u / 2u)

typedef struct {
    const char *name;
    int mode;
} palette_t;

static void fill_temperature(uint8_t *data) {
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            int dx = x - 420;
            int dy = y - 190;
            int hotspot = 255 - (dx * dx + dy * dy) / 180;
            int value = 25 + x * 150 / WIDTH + y * 35 / HEIGHT;
            if (hotspot > value) value = hotspot;
            if (x > 80 && x < 250 && y > 280 && y < 410) value += 38;
            if (value < 0) value = 0;
            if (value > 254) value = 254;
            data[(size_t)y * WIDTH + x] = (uint8_t)value;
        }
    }
}

static int dump_buffer(const char *path, MEDIA_BUFFER buffer, size_t size) {
    if (MEDIA_POOL_BeginCpuAccess(buffer, DMA_BUF_SYNC_READ) != 0) return -1;
    const void *data = MEDIA_POOL_GetVaddr(buffer);
    if (!data || MEDIA_POOL_GetSize(buffer) < size) {
        MEDIA_POOL_EndCpuAccess(buffer, DMA_BUF_SYNC_READ);
        return -1;
    }
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        MEDIA_POOL_EndCpuAccess(buffer, DMA_BUF_SYNC_READ);
        return -1;
    }
    size_t written = fwrite(data, 1, size, fp);
    fclose(fp);
    MEDIA_POOL_EndCpuAccess(buffer, DMA_BUF_SYNC_READ);
    return written == size ? 0 : -1;
}

int main(int argc, char **argv) {
    static const palette_t palettes[] = {
        {"rainbow", MEDIA_THERMAL_COLOR_RAINBOW},
        {"black_hot", MEDIA_THERMAL_COLOR_BLACK_HOT},
        {"white_hot", MEDIA_THERMAL_COLOR_WHITE_HOT},
        {"iron", MEDIA_THERMAL_COLOR_IRON},
        {"blue_red", MEDIA_THERMAL_COLOR_BLUE_RED},
    };
    const char *outdir = argc > 1 ? argv[1] : "outputs";
    const char *license = getenv("LIBMEDIA_LICENSE_PATH");
    MEDIA_THERMAL_ATTR attr = {0};
    MEDIA_BUFFER input = {.pool_id = -1, .index = -1};
    char path[512];
    int rc = 1;

    mkdir(outdir, 0755);
    if (MEDIA_SYS_Init() != 0) return 1;
    (void)MEDIA_SYS_SetLicense(license ? license : "/root/licence.dat");
    if (MEDIA_POOL_Create(OUT_POOL, OUT_SIZE, 4) != 0 ||
        MEDIA_POOL_Create(IN_POOL, IN_SIZE, 2) != 0) goto cleanup;
    attr.width = WIDTH;
    attr.height = HEIGHT;
    attr.format = MEDIA_FORMAT_GRAY8;
    attr.color_mode = palettes[0].mode;
    attr.input_depth = 2;
    attr.output_depth = 4;
    if (MEDIA_THERMAL_CreateGrp(GRP, &attr) != 0 || MEDIA_THERMAL_Start(GRP) != 0) goto cleanup;

    if (MEDIA_POOL_GetBuffer(IN_POOL, &input) != 0 ||
        MEDIA_POOL_BeginCpuAccess(input, DMA_BUF_SYNC_WRITE) != 0) goto cleanup_group;
    uint8_t *data = MEDIA_POOL_GetVaddr(input);
    if (!data || MEDIA_POOL_GetSize(input) < IN_SIZE) {
        MEDIA_POOL_EndCpuAccess(input, DMA_BUF_SYNC_WRITE);
        goto cleanup_input;
    }
    fill_temperature(data);
    MEDIA_POOL_EndCpuAccess(input, DMA_BUF_SYNC_WRITE);
    snprintf(path, sizeof(path), "%s/temperature_gray8_%dx%d.raw", outdir, WIDTH, HEIGHT);
    if (dump_buffer(path, input, IN_SIZE) != 0) goto cleanup_input;

    for (size_t i = 0; i < sizeof(palettes) / sizeof(palettes[0]); ++i) {
        MEDIA_BUFFER output = {.pool_id = -1, .index = -1};
        if (MEDIA_THERMAL_SetColorMode(GRP, palettes[i].mode) != 0 ||
            MEDIA_THERMAL_Process(GRP, input, &output) != 0) goto cleanup_input;
        snprintf(path, sizeof(path), "%s/%s_nv12_%dx%d.raw", outdir, palettes[i].name, WIDTH, HEIGHT);
        if (dump_buffer(path, output, OUT_SIZE) != 0) {
            MEDIA_POOL_PutBuffer(output);
            goto cleanup_input;
        }
        MEDIA_POOL_PutBuffer(output);
        printf("PASS mode=%s output=%s\n", palettes[i].name, path);
    }
    rc = 0;

cleanup_input:
    if (input.pool_id >= 0) MEDIA_POOL_PutBuffer(input);
cleanup_group:
    MEDIA_THERMAL_Stop(GRP);
    MEDIA_THERMAL_DestroyGrp(GRP);
cleanup:
    MEDIA_POOL_Destroy(IN_POOL);
    MEDIA_POOL_Destroy(OUT_POOL);
    MEDIA_SYS_Exit();
    return rc;
}
