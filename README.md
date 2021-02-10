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
### FRC2

This was the original timer used in Espressif products.  It has 12.5 ns resolution and it is very simple to use, just read one 32 bit system register at 0x3ff47024.

### TG0_LAC

This is the new standard for ESP32.  It has 500 ns resolution, which is enough for esp_timer_get_time() function implementation.  Internally, it has quite convoluted code with busy waiting. For these reasons, the timer_u32 uses esp_timer_get_time as an internal implementation. **NOT RECOMMENDED.**

### SYSTIMER

This is the latest standard.  It is available for ESP32-S2 and perhaps for ESP32-S3.  It is not availale for ESP32.  It has 12.5 ns resolution and is very simple to use

 - First it writes a command that a new reading is required
 - Then it waits a confirmation that a new reading is available (very fast)
 - Finally it reads the 32 bit register value
 
 ## Demo Program
 
 The following program (timer_u32_main.c) demonstrates the time measurement resuts with these 3 different methods.
 
 	/* Demonstrations of timer_u32 on ESP32 and ESP32-S2
	    using the available timer options (FRC2, TG0_LAC, SYSTIMER).
	    These options are descried on page

	    https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html

	    Source code is available at

	    https://github.com/OliviliK/ESP32_timer_u32
	*/

	#include <stdio.h>
	#include "sdkconfig.h"
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
	
This code calculates the fibonacci numbers, measures the time required for the calculations, and finally checks that the calculated fibonacci(92) has correct value.  The fibonacci(92) is the largest fitting in a 64 bit uint64_t variable.  Fibonacci(93) would require uint128_t.

## FRC2 Results

	ESP32 processor, FRC2
	 8.113 us, F1 = 1
	 0.300 us, F2 = 1
	 4.338 us, F3 = 2
	 0.488 us, F4 = 3
	 0.575 us, F5 = 5
	 0.675 us, F6 = 8
	 0.762 us, F7 = 13
	 0.863 us, F8 = 21
	 0.950 us, F9 = 34
	 1.050 us, F10 = 55
	 1.138 us, F11 = 89
	 1.237 us, F12 = 144
	 1.325 us, F13 = 233
	 1.425 us, F14 = 377
	 1.513 us, F15 = 610
	 1.612 us, F16 = 987
	 1.700 us, F17 = 1597
	 1.800 us, F18 = 2584
	 1.888 us, F19 = 4181
	 1.987 us, F20 = 6765
	 2.075 us, F21 = 10946
	 2.175 us, F22 = 17711
	 2.263 us, F23 = 28657
	 2.362 us, F24 = 46368
	 2.450 us, F25 = 75025
	 2.550 us, F26 = 121393
	 2.638 us, F27 = 196418
	 2.737 us, F28 = 317811
	 2.825 us, F29 = 514229
	 2.925 us, F30 = 832040
	 3.013 us, F31 = 1346269
	 3.112 us, F32 = 2178309
	 3.200 us, F33 = 3524578
	 3.300 us, F34 = 5702887
	 3.388 us, F35 = 9227465
	 3.487 us, F36 = 14930352
	 3.575 us, F37 = 24157817
	 3.675 us, F38 = 39088169
	 3.763 us, F39 = 63245986
	 3.862 us, F40 = 102334155
	 3.950 us, F41 = 165580141
	 4.050 us, F42 = 267914296
	 4.137 us, F43 = 433494437
	 4.238 us, F44 = 701408733
	 4.325 us, F45 = 1134903170
	 4.425 us, F46 = 1836311903
	 4.512 us, F47 = 2971215073
	 4.625 us, F48 = 4807526976
	 4.713 us, F49 = 7778742049
	 4.812 us, F50 = 12586269025
	 4.912 us, F51 = 20365011074
	 5.025 us, F52 = 32951280099
	 5.125 us, F53 = 53316291173
	 5.238 us, F54 = 86267571272
	 5.325 us, F55 = 139583862445
	 5.425 us, F56 = 225851433717
	 5.525 us, F57 = 365435296162
	 5.625 us, F58 = 591286729879
	 5.713 us, F59 = 956722026041
	 5.825 us, F60 = 1548008755920
	 5.925 us, F61 = 2504730781961
	 6.025 us, F62 = 4052739537881
	 6.113 us, F63 = 6557470319842
	 6.225 us, F64 = 10610209857723
	 6.325 us, F65 = 17167680177565
	 6.425 us, F66 = 27777890035288
	 6.512 us, F67 = 44945570212853
	 6.625 us, F68 = 72723460248141
	 6.713 us, F69 = 117669030460994
	 6.825 us, F70 = 190392490709135
	 6.925 us, F71 = 308061521170129
	 7.025 us, F72 = 498454011879264
	 7.113 us, F73 = 806515533049393
	 7.213 us, F74 = 1304969544928657
	 7.312 us, F75 = 2111485077978050
	 7.425 us, F76 = 3416454622906707
	 7.512 us, F77 = 5527939700884757
	 7.625 us, F78 = 8944394323791464
	 7.713 us, F79 = 14472334024676221
	 7.812 us, F80 = 23416728348467685
	 7.912 us, F81 = 37889062373143906
	 8.025 us, F82 = 61305790721611591
	 8.125 us, F83 = 99194853094755497
	 8.238 us, F84 = 160500643816367088
	 8.325 us, F85 = 259695496911122585
	 8.425 us, F86 = 420196140727489673
	 8.525 us, F87 = 679891637638612258
	 8.637 us, F88 = 1100087778366101931
	 8.738 us, F89 = 1779979416004714189
	 8.837 us, F90 = 2880067194370816120
	 8.925 us, F91 = 4660046610375530309
	 9.038 us, F92 = 7540113804746346429
	F92 is OK

We can observe that the calculations take from 0.3 to 9.0 microsecond.  In the beginning there is an anomaly that some calculations take long time.

### TG0_LAC Results

	ESP32 processor, TG0_LAC
	 9.000 us, F1 = 1
	 0.000 us, F2 = 1
	 1.000 us, F3 = 2
	 1.000 us, F4 = 3
	 2.000 us, F5 = 5
	 1.000 us, F6 = 8
	 1.000 us, F7 = 13
	 2.000 us, F8 = 21
	 2.000 us, F9 = 34
	 2.000 us, F10 = 55
	 2.000 us, F11 = 89
	 2.000 us, F12 = 144
	 2.000 us, F13 = 233
	 2.000 us, F14 = 377
	 2.000 us, F15 = 610
	 2.000 us, F16 = 987
	 2.000 us, F17 = 1597
	 2.000 us, F18 = 2584
	 2.000 us, F19 = 4181
	 3.000 us, F20 = 6765
	 2.000 us, F21 = 10946
	 3.000 us, F22 = 17711
	 3.000 us, F23 = 28657
	 2.000 us, F24 = 46368
	 3.000 us, F25 = 75025
	 3.000 us, F26 = 121393
	 4.000 us, F27 = 196418
	 4.000 us, F28 = 317811
	 3.000 us, F29 = 514229
	 3.000 us, F30 = 832040
	 3.000 us, F31 = 1346269
	 3.000 us, F32 = 2178309
	 3.000 us, F33 = 3524578
	 4.000 us, F34 = 5702887
	 4.000 us, F35 = 9227465
	 4.000 us, F36 = 14930352
	 4.000 us, F37 = 24157817
	 4.000 us, F38 = 39088169
	 4.000 us, F39 = 63245986
	 5.000 us, F40 = 102334155
	 4.000 us, F41 = 165580141
	 5.000 us, F42 = 267914296
	 5.000 us, F43 = 433494437
	 5.000 us, F44 = 701408733
	 5.000 us, F45 = 1134903170
	 5.000 us, F46 = 1836311903
	 5.000 us, F47 = 2971215073
	 5.000 us, F48 = 4807526976
	 6.000 us, F49 = 7778742049
	 5.000 us, F50 = 12586269025
	 5.000 us, F51 = 20365011074
	 5.000 us, F52 = 32951280099
	 6.000 us, F53 = 53316291173
	 6.000 us, F54 = 86267571272
	 6.000 us, F55 = 139583862445
	 6.000 us, F56 = 225851433717
	 6.000 us, F57 = 365435296162
	 6.000 us, F58 = 591286729879
	 6.000 us, F59 = 956722026041
	 7.000 us, F60 = 1548008755920
	 6.000 us, F61 = 2504730781961
	 6.000 us, F62 = 4052739537881
	 7.000 us, F63 = 6557470319842
	 7.000 us, F64 = 10610209857723
	 7.000 us, F65 = 17167680177565
	 7.000 us, F66 = 27777890035288
	 7.000 us, F67 = 44945570212853
	 7.000 us, F68 = 72723460248141
	 7.000 us, F69 = 117669030460994
	 8.000 us, F70 = 190392490709135
	 7.000 us, F71 = 308061521170129
	 8.000 us, F72 = 498454011879264
	 8.000 us, F73 = 806515533049393
	 7.000 us, F74 = 1304969544928657
	 8.000 us, F75 = 2111485077978050
	 8.000 us, F76 = 3416454622906707
	 8.000 us, F77 = 5527939700884757
	 8.000 us, F78 = 8944394323791464
	 9.000 us, F79 = 14472334024676221
	 9.000 us, F80 = 23416728348467685
	 9.000 us, F81 = 37889062373143906
	 9.000 us, F82 = 61305790721611591
	 9.000 us, F83 = 99194853094755497
	 9.000 us, F84 = 160500643816367088
	 9.000 us, F85 = 259695496911122585
	 9.000 us, F86 = 420196140727489673
	 9.000 us, F87 = 679891637638612258
	10.000 us, F88 = 1100087778366101931
	 9.000 us, F89 = 1779979416004714189
	10.000 us, F90 = 2880067194370816120
	10.000 us, F91 = 4660046610375530309
	 9.000 us, F92 = 7540113804746346429
	F92 is OK
	
This demonstrates, how the TGO_LAC is not a proper alternative for microsecond level measurements.

### SYSTIMER Results

	ESP32-S2 processor, SYSTIMER
	 6.200 us, F1 = 1
	 6.100 us, F2 = 1
	 4.075 us, F3 = 2
	 4.050 us, F4 = 3
	 4.050 us, F5 = 5
	 4.050 us, F6 = 8
	 4.050 us, F7 = 13
	 4.050 us, F8 = 21
	 4.050 us, F9 = 34
	 4.075 us, F10 = 55
	 4.050 us, F11 = 89
	 4.125 us, F12 = 144
	 4.250 us, F13 = 233
	 4.325 us, F14 = 377
	 4.425 us, F15 = 610
	 4.500 us, F16 = 987
	 4.600 us, F17 = 1597
	 4.700 us, F18 = 2584
	 4.800 us, F19 = 4181
	 4.875 us, F20 = 6765
	 4.975 us, F21 = 10946
	 5.075 us, F22 = 17711
	 5.175 us, F23 = 28657
	 5.275 us, F24 = 46368
	 5.350 us, F25 = 75025
	 5.450 us, F26 = 121393
	 5.550 us, F27 = 196418
	 5.650 us, F28 = 317811
	 5.750 us, F29 = 514229
	 5.825 us, F30 = 832040
	 5.925 us, F31 = 1346269
	 6.000 us, F32 = 2178309
	 6.100 us, F33 = 3524578
	 6.200 us, F34 = 5702887
	 6.300 us, F35 = 9227465
	 6.400 us, F36 = 14930352
	 6.500 us, F37 = 24157817
	 6.575 us, F38 = 39088169
	 6.675 us, F39 = 63245986
	 6.775 us, F40 = 102334155
	 6.850 us, F41 = 165580141
	 6.950 us, F42 = 267914296
	 7.050 us, F43 = 433494437
	 7.125 us, F44 = 701408733
	 7.250 us, F45 = 1134903170
	 7.325 us, F46 = 1836311903
	 7.425 us, F47 = 2971215073
	 7.525 us, F48 = 4807526976
	 7.625 us, F49 = 7778742049
	 7.700 us, F50 = 12586269025
	 7.825 us, F51 = 20365011074
	 7.925 us, F52 = 32951280099
	 8.025 us, F53 = 53316291173
	 8.125 us, F54 = 86267571272
	 8.225 us, F55 = 139583862445
	 8.325 us, F56 = 225851433717
	 8.450 us, F57 = 365435296162
	 8.525 us, F58 = 591286729879
	 8.625 us, F59 = 956722026041
	 8.725 us, F60 = 1548008755920
	 8.825 us, F61 = 2504730781961
	 8.925 us, F62 = 4052739537881
	 9.025 us, F63 = 6557470319842
	 9.125 us, F64 = 10610209857723
	 9.250 us, F65 = 17167680177565
	 9.325 us, F66 = 27777890035288
	 9.425 us, F67 = 44945570212853
	 9.525 us, F68 = 72723460248141
	 9.625 us, F69 = 117669030460994
	 9.725 us, F70 = 190392490709135
	 9.825 us, F71 = 308061521170129
	 9.925 us, F72 = 498454011879264
	10.025 us, F73 = 806515533049393
	 8.050 us, F74 = 1304969544928657
	10.225 us, F75 = 2111485077978050
	 8.325 us, F76 = 3416454622906707
	10.425 us, F77 = 5527939700884757
	 8.500 us, F78 = 8944394323791464
	10.625 us, F79 = 14472334024676221
	 8.700 us, F80 = 23416728348467685
	10.825 us, F81 = 37889062373143906
	 8.900 us, F82 = 61305790721611591
	11.050 us, F83 = 99194853094755497
	 9.125 us, F84 = 160500643816367088
	11.250 us, F85 = 259695496911122585
	 9.300 us, F86 = 420196140727489673
	11.425 us, F87 = 679891637638612258
	 9.525 us, F88 = 1100087778366101931
	11.650 us, F89 = 1779979416004714189
	 9.725 us, F90 = 2880067194370816120
	11.825 us, F91 = 4660046610375530309
	 9.925 us, F92 = 7540113804746346429
	F92 is OK
	
Here we have a similar anomaly in the beginning as we had with FRC2.  The other anomaly is in the cyclic variations where odd fibonaccies take much longer to calculate than the even fibonaccies.  This is not real, and the determination of these variations is outside of the scope of this description. It could be related to the background communication.
