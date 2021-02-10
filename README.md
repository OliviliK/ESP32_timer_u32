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
      nanoSeconds = timer_delta_ns(dt);
      microSeconds = timer_delta_us(dt);
      milliSeconds = timer_delta_ms(dt);
      seconds = timer_delta_s(dt);

Note that the following code doesn't work during during the timer wraparound

      uint32_t t0;
      float ms0,ms1,deltaMs;

      t0 = timer_u32();
      ms0 = timer_delta_ms(t0);
      <code to be measured>
      ms1 = timer_delta_ms(timer_u32());
      deltaMs = ms1 - ms0;

## Espressif esp_timer

Internally, esp_timer uses a 64-bit hardware timer that has 3 different implementations

1. LAC timer as a default for ESP32
2. FRC2 timer as an alternative for ESP32
3. SYSTIMER as the only option for ESP32-S2

When using ESP32, the default LAC can be changed to the legacy FRC2 by the ESP-IDF GUI Configuration tool (the gear icon on lower left corner).  Enter timer as search parameter, under the High resolution timer (esp_timer) header select the FRC2 from the pulldown menu.

These three alternatives can be seen in the timer_u32.h code

	#include "soc/frc_timer_reg.h"

	#if defined(CONFIG_ESP_TIMER_IMPL_FRC2)
	#define timer_u32() (REG_READ(0x3ff47024))      // FRC2
	#define _TICKS_PER_US (80)

	#elif defined(CONFIG_ESP_TIMER_IMPL_TG0_LAC)
	#include "esp_timer.h"
	#define timer_u32() (esp_timer_get_time())      // TG0_LAC
	#define _TICKS_PER_US (1)

	#else
							// SYSTIMER
	__attribute__((always_inline)) static inline uint32_t timer_u32(void) {
	   REG_WRITE(0x3f423038,1UL << 31);             // Write SYSTIMER_TIMER_UPDATE bit 
	   while (!REG_GET_BIT(0x3f423038,1UL << 30));  // Check SYSTIMER_TIMER_VALUE_VALID
	   return REG_READ(0x3f423040);                 // Read SYSTIMER_VALUE_LO_REG
	}
	#define _TICKS_PER_US (80)
	#endif

	__attribute__((always_inline)) static inline float timer_delta_ns(uint32_t tics) {
	   return tics * (1000.0 / _TICKS_PER_US);
	}

	__attribute__((always_inline)) static inline float timer_delta_us(uint32_t tics) {
	   return tics * (1.0 / _TICKS_PER_US);
	}

	__attribute__((always_inline)) static inline float timer_delta_ms(uint32_t tics) {
	   return tics * (0.001 / _TICKS_PER_US);
	}

	__attribute__((always_inline)) static inline float timer_delta_s(uint32_t tics) {
	   return tics * (0.000001 / _TICKS_PER_US);
	}
