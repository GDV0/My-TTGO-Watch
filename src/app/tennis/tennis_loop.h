
#ifdef NATIVE_64BIT
    #include <string>
    #include "utils/logging.h"
    #include "utils/millis.h"

    using namespace std;
    #define String string
#else
    #include <Arduino.h>
#endif

uint8_t tennis_loop( uint32_t tile_num, uint8_t init);
void tennis_time_display( uint32_t tile_num);