# Espressif ESP32 High Resolution Timer

The timer_u32.h file allows an application to use a read only timer for timing measurements done at and below 1 microsecond level.

The **timer_u32()** is an alternative for the **esp_timer_get_time()** function as described in [Epressif Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html).

## Main Differences

1. Resolution
   - timer_u32 uses 80 MHz clock (in most cases)
   - esp_timer_get_time uses 1 MHz clock
1. Range
   - timer_u32 uses 32 bit allowing time measurements up to 53 seconds
   - esp_timer_get_time uses 64 bit allowing up to 585 years
1. Wraparound
   - the timer_u32 wraparound from a very large number to a small number is transparent to applications
   - esp_timer_get_time doesn't wrap around
   
## Usage Pattern


  
