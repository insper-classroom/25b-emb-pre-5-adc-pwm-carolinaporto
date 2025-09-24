#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>

#include "data.h"
QueueHandle_t xQueueData;

// não mexer! Alimenta a fila com os dados do sinal
void data_task(void *p) {
    vTaskDelay(pdMS_TO_TICKS(400));

    int data_len = sizeof(sine_wave_four_cycles) / sizeof(sine_wave_four_cycles[0]);
    for (int i = 0; i < data_len; i++) {
        xQueueSend(xQueueData, &sine_wave_four_cycles[i], 1000000);
    }

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static int ma5_push(int x, int *y_out) {
    static int buf[5] = {0};
    static int idx = 0;
    static int count = 0;
    static int sum = 0;

    if (count < 5) {
        sum += x;
        buf[idx] = x;
        idx = (idx + 1) % 5;
        count++;
        if (count == 5) {
            *y_out = sum / 5;
            return 1;
        }
        return 0;
    } else {
        int x_old = buf[idx];
        sum -= x_old;
        sum += x;
        buf[idx] = x;
        idx = (idx + 1) % 5;

        *y_out = sum / 5;
        return 1;
    }
}

void process_task(void *p) {
    int data = 0;

    while (true) {
        if (xQueueReceive(xQueueData, &data, 100)) {

            int y;
            if (ma5_push(data, &y)) {
                printf("%d \n", y);
            }

            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

int main() {
    stdio_init_all();

    xQueueData = xQueueCreate(64, sizeof(int));

    xTaskCreate(data_task, "Data task ", 4096, NULL, 1, NULL);
    xTaskCreate(process_task, "Process task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
