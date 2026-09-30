/* SPDX-License-Identifier: MIT */
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

/* These pads have no harness connection. Preserve all PS/2 and LED pins. */
static int release_display_pins(void) {
    const struct device *port = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    const gpio_pin_t pins[] = {8, 12, 21, 23};
    if (!device_is_ready(port)) {
        return -ENODEV;
    }
    for (size_t i = 0; i < ARRAY_SIZE(pins); i++) {
        int err = gpio_pin_configure(port, pins[i], GPIO_DISCONNECTED);
        if (err) {
            return err;
        }
    }
    return 0;
}
SYS_INIT(release_display_pins, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
