#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include <stdbool.h>
#include <stdint.h>

const int LED_PIN = 4;
const int BTN_PIN = 15; 
const int ADC_GPIO = 28;

// 0.0–1.0 V: desligado
// 1.0–2.0 V: 300 ms
// 2.0–3.3 V: 500 ms
#define T_ZONE2_MS 300
#define T_ZONE3_MS 500
#define RECHECK_MS 50

volatile bool timer_fired = false;
volatile bool blinking_enabled = true;

int timer_ms(float v) {
    if (v < 1.0f) return 0;
    if (v < 2.0f) return T_ZONE2_MS;
    return T_ZONE3_MS;
}

void btn_callback(uint gpio, uint32_t events) {
    if (gpio == BTN_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        blinking_enabled = !blinking_enabled;
    }
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    timer_fired = true;
    return 0;
}

void arm_next_alarm(int next_period_ms, bool enabled) {
    int delay = (enabled && next_period_ms > 0) ? next_period_ms : RECHECK_MS;
    add_alarm_in_ms(delay, alarm_callback, NULL, false);
}

int main() {
    stdio_init_all();

    gpio_init(LED_PIN); gpio_set_dir(LED_PIN, GPIO_OUT); gpio_put(LED_PIN, 0);
    bool led_on = false;

    gpio_init(BTN_PIN); gpio_set_dir(BTN_PIN, GPIO_IN); gpio_pull_up(BTN_PIN);
    gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_FALL, true, &btn_callback);

    adc_init(); adc_gpio_init(ADC_GPIO); adc_select_input(2);

    const float conv = 3.3f / 4096.0f;

    uint16_t raw = adc_read();
    int next_period_ms = timer_ms(raw * conv);

    arm_next_alarm(next_period_ms, blinking_enabled);

    while (true) {
        if (timer_fired) {
            timer_fired = false;

            raw = adc_read();
            next_period_ms = timer_ms(raw * conv);

            if (!blinking_enabled || next_period_ms == 0) {
                led_on = false;
                gpio_put(LED_PIN, 0);
            } else {
                led_on = !led_on;
                gpio_put(LED_PIN, led_on);
            }

            arm_next_alarm(next_period_ms, blinking_enabled);
        }
    }
    return 0;
}
