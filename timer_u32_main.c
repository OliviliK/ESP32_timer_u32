/* Demonstrations of timer_u32 on ESP32 and ESP32-S2
    using the available timer options (FRC2, TG0_LAC, SYSTIMER).
    These options are descried on page

    https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html

    Source code is available at

    https://github.com/OliviliK/ESP32_timer_u32
*/

#include <stdio.h>
#include "sdkconfig.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_timer.h"
#include "timer_u32.h"

uint64_t fibonacci(int n) {
    uint64_t a,b,f;
    a = 0;
    b = 1;
    while (--n > 0) {
        f = a + b;
        a = b;
        b = f;
    }
    return b;
}

void app_main(void) {

#if defined(CONFIG_IDF_TARGET_ESP32)
    printf("ESP32 processor, ");
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
    printf("ESP32-S2 processor, ");
#else
    printf("ESP32-??? processor, ");
#endif

#if defined(CONFIG_ESP_TIMER_IMPL_FRC2)
    printf("FRC2\n");
#elif defined(CONFIG_ESP_TIMER_IMPL_TG0_LAC)
    printf("TG0_LAC\n");
#else
    printf("SYSTIMER\n");
#endif

    uint32_t dt,t0;
    uint64_t f,F92 = 7540113804746346429;

    for (int i = 1; i<93; i++) {
        t0 = timer_u32();
        f = fibonacci(i);
        dt = timer_u32() - t0;
        printf("%6.3f us, F%d = %jd\n",timer_delta_us(dt),i,f);
    }
    if (fibonacci(92) == F92) printf("F92 is OK\n");
}
