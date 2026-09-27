#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* LED0 = green (LD1), LED1 = blue (LD2), LED2 = red (LD3) */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

/* --- Mutex demo: two threads sharing a counter, protected by a mutex --- */
K_MUTEX_DEFINE(counter_mutex);
static int shared_counter = 0;

void mutex_thread_a(void *a, void *b, void *c)
{
    while (1) {
        k_mutex_lock(&counter_mutex, K_FOREVER);

        shared_counter++;
        printk("[Mutex] Thread A incremented counter to %d\n", shared_counter);
        gpio_pin_toggle_dt(&led0);

        k_mutex_unlock(&counter_mutex);
        k_msleep(500);
    }
}

void mutex_thread_b(void *a, void *b, void *c)
{
    while (1) {
        k_mutex_lock(&counter_mutex, K_FOREVER);

        shared_counter++;
        printk("[Mutex] Thread B incremented counter to %d\n", shared_counter);
        gpio_pin_toggle_dt(&led0);

        k_mutex_unlock(&counter_mutex);
        k_msleep(700);
    }
}

/* --- Semaphore demo: producer signals, consumer waits and reacts --- */
K_SEM_DEFINE(data_ready_sem, 0, 1);

void producer_thread(void *a, void *b, void *c)
{
    while (1) {
        k_msleep(1000);
        printk("[Semaphore] Producer: data ready, signaling consumer\n");
        k_sem_give(&data_ready_sem);
    }
}

void consumer_thread(void *a, void *b, void *c)
{
    while (1) {
        k_sem_take(&data_ready_sem, K_FOREVER);
        printk("[Semaphore] Consumer: received signal, toggling LED1\n");
        gpio_pin_toggle_dt(&led1);
    }
}

/* Thread definitions */
K_THREAD_DEFINE(thread_a_id, 512, mutex_thread_a, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(thread_b_id, 512, mutex_thread_b, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(producer_id, 512, producer_thread, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(consumer_id, 512, consumer_thread, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
    if (!gpio_is_ready_dt(&led0) || !gpio_is_ready_dt(&led1) || !gpio_is_ready_dt(&led2)) {
        printk("Error: one or more LEDs not ready\n");
        return 0;
    }

    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);

    printk("Semaphore & Mutex demo started on nucleo_f767zi\n");

    /* Heartbeat LED in main thread, independent of the other two demos */
    while (1) {
        gpio_pin_toggle_dt(&led2);
        k_msleep(1500);
    }

    return 0;
}
