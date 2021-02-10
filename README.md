# Espressif ESP32 High Resolution Timer

The ***timer_u32.h*** file allows an application to use a read only timer for timing measurements done at and below 1 microsecond level.

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

This is a typical case

      uint32_t t0,t1,dt;
   
      t0 = timer_u32();
      <code to be measured>
      t1 = timer_u32();
      dt = t1 - t0;
   
The result is that dt contains the number of ticks used by the measured code.  It can be streamlined into following

      uint32_t t0,dt;
   
      t0 = timer_u32();
      <code to be measured>
      dt = timer_u32() - t0;
   
### Time Units

If instead of calculating the ticks, the result can *always* be converted into a floating point number.

      uint32_t t0,dt;
      float nanoSeconds, microSeconds, milliSeconds, seconds;

      t0 = timer_u32();
      <code to be measured>
      dt = timer_u32() - t0;                    // Ticks
      nanoSeconds = timer_u32_ns(dt);
      microSeconds = timer_u32_us(dt);
      milliSeconds = timer_u32_ms(dt);
      seconds = timer_u32_s(dt);

Note that the following code doesn't work during during the timer wraparound

      uint32_t t0;
      float ms0,ms1,deltaMs;

      t0 = timer_u32();
      ms0 = timer_u32_ms(t0);
      <code to be measured>
      ms1 = timer_u32_ms(timer_u32());
      deltaMs = ms1 - ms0;


  
