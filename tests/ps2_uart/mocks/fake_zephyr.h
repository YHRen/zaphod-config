/* SPDX-License-Identifier: MIT
 * Host contract model: GPIO modes, ISR mutex restrictions, semaphores and queues.
 * This does not simulate radio scheduling or PS/2 electrical timing.
 */
#pragma once
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define CONFIG_PS2_LOG_LEVEL 0
#ifndef CONFIG_PS2_UART_OPEN_DRAIN
#define CONFIG_PS2_UART_OPEN_DRAIN 1
#endif
#define CONFIG_PS2_UART_ENABLE_PS2_RESEND_CALLBACK 0
#define CONFIG_PS2_UART_WRITE_MODE_BLOCKING 0
#define CONFIG_SOC_SERIES_NRF52X 1
#define IS_ENABLED(x) (x)
#define BIT(n) (1UL << (n))
#define LOG_MODULE_REGISTER(...)
#define LOG_INF(...) ((void)0)
#define LOG_WRN(...) ((void)0)
#define LOG_ERR(...) ((void)0)
#define LOG_DBG(...) ((void)0)
#define DEVICE_DT_INST_DEFINE(...)
#define PINCTRL_DT_DEFINE(...)
#define DEVICE_DT_GET(...) (&uart_device)
#define GPIO_DT_SPEC_INST_GET(n, prop) {&gpio_device, MOCK_PIN_##prop, 0}
#define MOCK_PIN_scl_gpios 7
#define MOCK_PIN_sda_gpios 5
#define PINCTRL_DT_DEV_CONFIG_GET(...) (&mock_pcfg)
#define DT_PROP(...) 0
#define PINCTRL_STATE_DEFAULT 0
#define PINCTRL_STATE_SLEEP 1

typedef unsigned int gpio_flags_t;
struct device {void *data;};
extern struct device gpio_device, uart_device;
struct gpio_dt_spec {const struct device *port; unsigned int pin; gpio_flags_t dt_flags;};
struct gpio_callback {int unused;};
#define GPIO_INPUT BIT(0)
#define GPIO_OUTPUT BIT(1)
#define GPIO_OUTPUT_INIT_HIGH BIT(2)
#define GPIO_OUTPUT_HIGH (GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH)
#define GPIO_SINGLE_ENDED BIT(3)
#define GPIO_OPEN_DRAIN GPIO_SINGLE_ENDED
#define GPIO_INT_EDGE_FALLING 1
#define GPIO_INT_DISABLE 0
int gpio_pin_configure_dt(const struct gpio_dt_spec *, gpio_flags_t);
int gpio_pin_set_dt(const struct gpio_dt_spec *, int);
int gpio_pin_get_dt(const struct gpio_dt_spec *);
int gpio_pin_interrupt_configure_dt(const struct gpio_dt_spec *, int);
void gpio_init_callback(struct gpio_callback *, void (*)(const struct device *, struct gpio_callback *, uint32_t), uint32_t);
int gpio_add_callback(const struct device *, struct gpio_callback *);

typedef uint32_t pinctrl_soc_pin_t;
struct pinctrl_state {const pinctrl_soc_pin_t *pins; unsigned int pin_cnt;};
struct pinctrl_dev_config {const struct pinctrl_state *states; unsigned int state_cnt;};
extern const struct pinctrl_dev_config mock_pcfg;
int pinctrl_apply_state(const struct pinctrl_dev_config *, int);

#define UART_ERROR_PARITY BIT(0)
#define UART_ERROR_OVERRUN BIT(1)
#define UART_ERROR_FRAMING BIT(2)
#define UART_BREAK BIT(3)
#define UART_ERROR_COLLISION BIT(4)
#define NRF_UARTE_ERROR_PARITY_MASK UART_ERROR_PARITY
#define NRF_UARTE_ERROR_OVERRUN_MASK UART_ERROR_OVERRUN
#define NRF_UARTE_ERROR_FRAMING_MASK UART_ERROR_FRAMING
#define NRF_UARTE_ERROR_BREAK_MASK UART_BREAK
#define UART_CFG_DATA_BITS_8 8
#define UART_CFG_STOP_BITS_1 1
#define UART_CFG_FLOW_CTRL_NONE 0
#define UART_CFG_PARITY_EVEN 2
struct uart_config {int data_bits, stop_bits, flow_ctrl, parity;};
int uart_irq_update(const struct device *);
int uart_irq_rx_ready(const struct device *);
int uart_fifo_read(const struct device *, uint8_t *, int);
int uart_err_check(const struct device *);
void uart_irq_rx_enable(const struct device *);
void uart_irq_rx_disable(const struct device *);
void uart_irq_err_enable(const struct device *);
int uart_config_get(const struct device *, struct uart_config *);
int uart_configure(const struct device *, const struct uart_config *);
int uart_irq_callback_user_data_set(const struct device *, void (*)(const struct device *, void *), void *);
bool device_is_ready(const struct device *);

typedef int64_t k_timeout_t;
#define K_NO_WAIT 0
#define K_FOREVER -1
#define K_USEC(n) (n)
#define K_MSEC(n) ((n) * 1000)
#define K_SECONDS(n) ((n) * 1000000)
#define K_TICKS(n) (n)
#define K_THREAD_STACK_DEFINE(name,n) char name[n]
#define K_THREAD_STACK_SIZEOF(name) sizeof(name)
struct k_work {void (*handler)(struct k_work *); bool pending;};
struct k_work_delayable {struct k_work work; bool pending;};
struct k_work_sync {int unused;};
struct k_work_q {int unused;};
struct k_sem {unsigned int count, limit;};
struct k_mutex {int count;};
#define K_MUTEX_DEFINE(name) struct k_mutex name
struct k_msgq {char *buffer; size_t size, cap, read, write, count;};
typedef int atomic_t;
#define ATOMIC_INIT(n) (n)
static inline int atomic_get(const atomic_t *value) {return *value;}
static inline int atomic_set(atomic_t *value,int new_value) {int old=*value; *value=new_value; return old;}
unsigned int irq_lock(void);
void irq_unlock(unsigned int);
bool k_is_in_isr(void);
int64_t k_uptime_ticks(void);
static inline int64_t k_us_to_ticks_ceil64(int64_t us) {return us;}
int k_mutex_lock(struct k_mutex *, k_timeout_t);
int k_mutex_unlock(struct k_mutex *);
void k_sem_init(struct k_sem *, unsigned int, unsigned int);
void k_sem_reset(struct k_sem *);
int k_sem_take(struct k_sem *, k_timeout_t);
void k_sem_give(struct k_sem *);
void k_msgq_init(struct k_msgq *, char *, size_t, size_t);
void k_msgq_purge(struct k_msgq *);
int k_msgq_get(struct k_msgq *, void *, k_timeout_t);
int k_msgq_put(struct k_msgq *, const void *, k_timeout_t);
void k_work_init(struct k_work *, void (*)(struct k_work *));
void k_work_init_delayable(struct k_work_delayable *, void (*)(struct k_work *));
int k_work_cancel_delayable(struct k_work_delayable *);
bool k_work_cancel_delayable_sync(struct k_work_delayable *, struct k_work_sync *);
int k_work_schedule_for_queue(struct k_work_q *, struct k_work_delayable *, k_timeout_t);
int k_work_reschedule_for_queue(struct k_work_q *, struct k_work_delayable *, k_timeout_t);
int k_work_submit_to_queue(struct k_work_q *, struct k_work *);
void k_work_queue_start(struct k_work_q *, char *, size_t, int, void *);
void k_busy_wait(unsigned int);
void k_sleep(k_timeout_t);

typedef void (*ps2_callback_t)(const struct device *, uint8_t);
struct ps2_driver_api {
 int (*config)(const struct device *, ps2_callback_t);
 int (*read)(const struct device *, uint8_t *);
 int (*write)(const struct device *, uint8_t);
 int (*disable_callback)(const struct device *);
 int (*enable_callback)(const struct device *);
};
