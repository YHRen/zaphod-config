/* SPDX-License-Identifier: MIT */
#include "fake_zephyr.h"
#include <stdio.h>
#include PS2_UART_SOURCE

struct device gpio_device, uart_device;
static const pinctrl_soc_pin_t pins[] = {27, 5};
static const struct pinctrl_state states[] = {{pins,2},{pins,2}};
const struct pinctrl_dev_config mock_pcfg = {states,2};
static bool in_isr, early_response, inject_in_callback;
static unsigned int irq_depth;
static int ack_bit, rx_errors, restore_error, mode_error, gpio_error;
static int lock_calls, unlock_calls, response_waits, write_waits;
static int64_t now;
static gpio_flags_t pin_flags[32];
static uint8_t received[256];
static size_t received_count;
enum scenario {ACK, BAD_ACK, NO_CLOCK, NO_RESPONSE, RESTORE_ERROR};
static enum scenario scenario;

unsigned int irq_lock(void) {return irq_depth++;}
void irq_unlock(unsigned int key) {assert(irq_depth); irq_depth=key;}
bool k_is_in_isr(void) {return in_isr;}
int64_t k_uptime_ticks(void) {return now;}
int gpio_pin_configure_dt(const struct gpio_dt_spec *spec,gpio_flags_t flags) {
 flags |= spec->dt_flags;
 assert(!((flags&GPIO_INPUT)&&!(flags&GPIO_OUTPUT)&&(flags&GPIO_SINGLE_ENDED)));
 pin_flags[spec->pin]=flags;
 return gpio_error;
}
int gpio_pin_set_dt(const struct gpio_dt_spec *s,int value) {(void)s;(void)value;return 0;}
int gpio_pin_get_dt(const struct gpio_dt_spec *s) {(void)s;return ack_bit;}
int gpio_pin_interrupt_configure_dt(const struct gpio_dt_spec *s,int mode) {(void)s;(void)mode;return gpio_error;}
void gpio_init_callback(struct gpio_callback *c,void (*cb)(const struct device *,struct gpio_callback *,uint32_t),uint32_t pins_) {(void)c;(void)cb;(void)pins_;}
int gpio_add_callback(const struct device *d,struct gpio_callback *c) {(void)d;(void)c;return 0;}
int pinctrl_apply_state(const struct pinctrl_dev_config *c,int state) {(void)c;return state==PINCTRL_STATE_DEFAULT?restore_error:mode_error;}
int uart_irq_update(const struct device *d) {(void)d;return 1;}
int uart_irq_rx_ready(const struct device *d) {(void)d;return 0;}
int uart_fifo_read(const struct device *d,uint8_t *b,int n) {(void)d;(void)b;(void)n;return 0;}
int uart_err_check(const struct device *d) {(void)d;return rx_errors;}
void uart_irq_rx_enable(const struct device *d) {
 (void)d;
 if(early_response && ps2_uart_data.write_awaits_resp) {
   early_response=false;
   ps2_uart_read_process_received_byte(0xfa);
 }
}
void uart_irq_rx_disable(const struct device *d) {(void)d;}
void uart_irq_err_enable(const struct device *d) {(void)d;}
int uart_config_get(const struct device *d,struct uart_config *c) {(void)d;memset(c,0,sizeof(*c));return 0;}
int uart_configure(const struct device *d,const struct uart_config *c) {(void)d;(void)c;return 0;}
int uart_irq_callback_user_data_set(const struct device *d,void (*cb)(const struct device *,void *),void *u) {(void)d;(void)cb;(void)u;return 0;}
bool device_is_ready(const struct device *d) {(void)d;return true;}
int k_mutex_lock(struct k_mutex *m,k_timeout_t timeout) {(void)timeout;assert(!in_isr);assert(!m->count);m->count++;lock_calls++;return 0;}
int k_mutex_unlock(struct k_mutex *m) {assert(!in_isr);assert(m->count==1);m->count--;unlock_calls++;return 0;}
void k_sem_init(struct k_sem *s,unsigned int count,unsigned int limit) {s->count=count;s->limit=limit;}
void k_sem_reset(struct k_sem *s) {s->count=0;}
void k_sem_give(struct k_sem *s) {if(s->count<s->limit)s->count++;}
int k_sem_take(struct k_sem *s,k_timeout_t timeout) {
 if(s->count) {s->count--;return 0;}
 if(timeout==K_NO_WAIT)return -EBUSY;
 if(s==&ps2_uart_data.write_lock) {
   write_waits++;
   assert(ps2_uart_write_mutex.count==1);
   if(scenario==NO_CLOCK)return -EAGAIN;
   ack_bit=scenario==BAD_ACK;
   restore_error=scenario==RESTORE_ERROR?-EIO:0;
   early_response=scenario==ACK;
   in_isr=true;
   ps2_uart_data.cur_write_pos=PS2_UART_POS_ACK;
   ps2_uart_write_scl_interrupt_handler_async(&gpio_device,NULL,0);
   in_isr=false;
 } else if(s==&ps2_uart_data.write_awaits_resp_sem) {
   response_waits++;
 }
 if(s->count) {s->count--;return 0;}
 return -EAGAIN;
}
void k_msgq_init(struct k_msgq *q,char *buffer,size_t size,size_t cap) {*q=(struct k_msgq){.buffer=buffer,.size=size,.cap=cap};}
void k_msgq_purge(struct k_msgq *q) {q->count=q->read=q->write=0;}
int k_msgq_get(struct k_msgq *q,void *item,k_timeout_t t) {(void)t;if(!q->count)return -ENOMSG;memcpy(item,q->buffer+q->read*q->size,q->size);q->read=(q->read+1)%q->cap;q->count--;return 0;}
int k_msgq_put(struct k_msgq *q,const void *item,k_timeout_t t) {(void)t;if(q->count==q->cap)return -ENOMSG;memcpy(q->buffer+q->write*q->size,item,q->size);q->write=(q->write+1)%q->cap;q->count++;return 0;}
void k_work_init(struct k_work *w,void (*h)(struct k_work *)) {w->handler=h;w->pending=false;}
void k_work_init_delayable(struct k_work_delayable *w,void (*h)(struct k_work *)) {k_work_init(&w->work,h);w->pending=false;}
int k_work_cancel_delayable(struct k_work_delayable *w) {w->pending=false;return 0;}
bool k_work_cancel_delayable_sync(struct k_work_delayable *w,struct k_work_sync *sync) {(void)sync;assert(!in_isr);w->pending=false;return true;}
int k_work_schedule_for_queue(struct k_work_q *q,struct k_work_delayable *w,k_timeout_t t) {(void)q;(void)t;w->pending=true;return 0;}
int k_work_reschedule_for_queue(struct k_work_q *q,struct k_work_delayable *w,k_timeout_t t) {return k_work_schedule_for_queue(q,w,t);}
int k_work_submit_to_queue(struct k_work_q *q,struct k_work *w) {(void)q;int n=!w->pending;w->pending=true;return n;}
void k_work_queue_start(struct k_work_q *q,char *stack,size_t size,int priority,void *cfg) {(void)q;(void)stack;(void)size;(void)priority;(void)cfg;}
void k_busy_wait(unsigned int us) {now+=us;}
void k_sleep(k_timeout_t t) {now+=t;}

static void byte_callback(const struct device *dev,uint8_t byte) {
 (void)dev;received[received_count++]=byte;
 if(inject_in_callback) {inject_in_callback=false;ps2_uart_read_process_received_byte(0x33);}
}
static void reset_test(void) {
 memset(&ps2_uart_data,0,sizeof(ps2_uart_data));
 memset(&ps2_uart_write_mutex,0,sizeof(ps2_uart_write_mutex));
 memset(pin_flags,0,sizeof(pin_flags));
 in_isr=early_response=inject_in_callback=false;irq_depth=0;
 ack_bit=restore_error=mode_error=gpio_error=0;
 rx_errors=NRF_UARTE_ERROR_PARITY_MASK;
 received_count=0;lock_calls=unlock_calls=response_waits=write_waits=0;now=0;
 scenario=ACK;
 ps2_uart_data.callback_isr=byte_callback;
 k_msgq_init(&ps2_uart_data.data_queue,ps2_uart_data.data_queue_buffer,sizeof(struct ps2_uart_data_queue_item),PS2_UART_DATA_QUEUE_SIZE);
 k_msgq_init(&ps2_uart_data.callback_queue,ps2_uart_data.callback_queue_buffer,1,PS2_UART_DATA_QUEUE_SIZE);
 k_sem_init(&ps2_uart_data.write_lock,0,1);
 k_sem_init(&ps2_uart_data.write_awaits_resp_sem,0,1);
}
int main(void) {
 reset_test();
 assert(ps2_uart_configure_pin_scl_output()==0);
 assert(ps2_uart_configure_pin_sda_output()==0);
 assert(!!(pin_flags[7]&GPIO_OPEN_DRAIN)==CONFIG_PS2_UART_OPEN_DRAIN);
 assert(!!(pin_flags[5]&GPIO_OPEN_DRAIN)==CONFIG_PS2_UART_OPEN_DRAIN);
 assert(ps2_uart_configure_pin_scl_input()==0);
 assert(ps2_uart_configure_pin_sda_input()==0);
 assert(!(pin_flags[7]&GPIO_OPEN_DRAIN)&&!(pin_flags[5]&GPIO_OPEN_DRAIN));

 reset_test();assert(ps2_uart_write_byte(0xf4)==0);
 assert(write_waits==1&&response_waits==0); /* ACK arrived before caller waited. */
 assert(lock_calls==1&&unlock_calls==1&&!ps2_uart_write_mutex.count);
 assert(!ps2_uart_data.data_queue.count); /* ACK was consumed, not motion. */
 ps2_uart_write_finish(false,"late completion");
 assert(!ps2_uart_data.write_lock.count); /* Duplicate completion ignored. */

 for(enum scenario sc=BAD_ACK;sc<=RESTORE_ERROR;sc++) {
   reset_test();scenario=sc;
   assert(ps2_uart_write_byte(0xf4)!=0);
   assert(lock_calls==1&&unlock_calls==1&&!ps2_uart_write_mutex.count);
   assert(!ps2_uart_data.write_awaits_resp);
   assert(ps2_uart_data.cur_write_status!=PS2_UART_WRITE_STATUS_ACTIVE);
 }
 reset_test();mode_error=-EIO;assert(ps2_uart_write_byte(0xf4)!=0);assert(unlock_calls==1);
 reset_test();gpio_error=-EIO;assert(ps2_uart_write_byte(0xf4)!=0);assert(unlock_calls==1);

 reset_test();ps2_uart_data.cur_write_status=PS2_UART_WRITE_STATUS_ACTIVE;
 ps2_uart_data.write_deadline=100;now=50;
 ps2_uart_write_scl_timeout(NULL);assert(ps2_uart_data.cur_write_status==PS2_UART_WRITE_STATUS_ACTIVE);
 now=101;ps2_uart_write_scl_timeout(NULL);assert(ps2_uart_data.cur_write_status==PS2_UART_WRITE_STATUS_FAILURE);
 assert(ps2_uart_data.write_lock.count==1);
 ps2_uart_write_scl_timeout(NULL);assert(ps2_uart_data.write_lock.count==1);

 reset_test();ps2_uart_data.callback_enabled=true;
 ps2_uart_read_process_received_byte(0x08);ps2_uart_read_process_received_byte(0x11);ps2_uart_read_process_received_byte(0x22);
 inject_in_callback=true;ps2_uart_read_callback_work_handler(NULL);
 assert(received_count==4&&memcmp(received,(uint8_t[]){0x08,0x11,0x22,0x33},4)==0);

 reset_test();ps2_uart_data.callback_enabled=true;
 for(int i=0;i<=PS2_UART_DATA_QUEUE_SIZE;i++)ps2_uart_read_process_received_byte((uint8_t)i);
 assert(atomic_get(&ps2_uart_data.callback_fault));
 ps2_uart_read_callback_work_handler(NULL);assert(!received_count);
 struct device mouse={.data=&ps2_uart_data};assert(ps2_uart_enable_callback(&mouse)==-EIO);

 reset_test();ps2_uart_data.callback_enabled=true;
 rx_errors=0;ps2_uart_read_process_received_byte(0x08);assert(atomic_get(&ps2_uart_data.callback_fault));
 rx_errors=NRF_UARTE_ERROR_PARITY_MASK;ps2_uart_read_process_received_byte(0x11);
 ps2_uart_read_callback_work_handler(NULL);assert(!received_count);

 reset_test();ps2_uart_read_process_received_byte(0xaa);uint8_t byte=0;
 assert(ps2_uart_read(NULL,&byte)==0&&byte==0xaa);
 assert(ps2_uart_data_queue_get_next(&byte,K_NO_WAIT)==-ETIMEDOUT);
 printf("PASS: GPIO contracts, command/ACK/timeout failures, early response, completion ownership, FIFO bursts, RX fault latch (open-drain=%d)\n",CONFIG_PS2_UART_OPEN_DRAIN);
 return 0;
}
