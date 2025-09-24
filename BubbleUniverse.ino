#define private public
#include <Adafruit_dvhstx.h>
#undef private

#define CURVECOUNT 256
#define CURVESTEP 4
#define ITERATIONS 256
#define SCREENWIDTH 640
#define SCREENHEIGHT 480
#define PI 3.1415926535897932384626433832795
#define SINTABLEPOWER 14
#define SINTABLEENTRIES (1 << SINTABLEPOWER)
#define ANG1INC (int32_t)((CURVESTEP * SINTABLEENTRIES) / 235)
#define ANG2INC (int32_t)((CURVESTEP * SINTABLEENTRIES) / (2 * PI))
#define SCALEMUL (int32_t)(SCREENHEIGHT * PI / 2)

float speed = 0.2f;
int32_t animationTime = 0;
int32_t oldAnimationTime = 0;
uint8_t flag = 0;

int32_t SinTable[SINTABLEENTRIES];

#if defined(ADAFRUIT_FEATHER_RP2350_HSTX)
DVHSTXPinout pinConfig = ADAFRUIT_FEATHER_RP2350_CFG;
#elif defined(ADAFRUIT_METRO_RP2350)
DVHSTXPinout pinConfig = ADAFRUIT_METRO_RP2350_CFG;
#elif defined(ARDUINO_ADAFRUIT_FRUITJAM_RP2350)
DVHSTXPinout pinConfig = ADAFRUIT_FRUIT_JAM_CFG;
#elif (defined(ARDUINO_RASPBERRY_PI_PICO_2) || defined(ARDUINO_RASPBERRY_PI_PICO_2W))
DVHSTXPinout pinConfig = ADAFRUIT_HSTXDVIBELL_CFG;
#else
DVHSTXPinout pinConfig = DVHSTX_PINOUT_DEFAULT;
#endif

DVHSTX8 display(pinConfig, DVHSTX_RESOLUTION_640x480, false);

void CreateSinTable()
{
    for (int i = 0; i < SINTABLEENTRIES; ++i)
    {
        SinTable[i] = sin(i * 2 * PI / SINTABLEENTRIES) * SINTABLEENTRIES / (2 * PI);
    }
    for (int i = 0; i < SINTABLEENTRIES; ++i)
    {
        *((int16_t *)(SinTable + i) + 1) = *(int16_t *)(SinTable + ((i + SINTABLEENTRIES / 4) % SINTABLEENTRIES));
    }
}

void InitPalette()
{
    for (int i = 0; i < 8; ++i)
    {
        const uint32_t red = (255 * i) / 7;
        for (int j = 0; j < 16; ++j)
        {
            int index = i*16+j;
            if (index > 0)
            {
              const uint32_t green = (255 * j) / 15;
              const uint32_t blue = (510 - (red + green)) >> 1;
              uint32_t val = (red << 16) | (green << 8) | (blue << 0);
              display.setColor(index, val);
              display.setColor(index|0x80, val);
            }
            else
            {
              display.setColor(index, 0);
              display.setColor(index|0x80, 0);
            }
        }
    }
}

void Render(uint8_t *ptr)
{
    uint8_t *screenCentre = ptr + ((SCREENWIDTH + SCREENWIDTH * SCREENHEIGHT) >> 1);

    int32_t ang1Start = animationTime;
    int32_t ang2Start = animationTime;
    for (int i = 0; i < CURVECOUNT; i += CURVESTEP)
    {
        int32_t x = 0;
        int32_t y = 0;
        for (int j = 0; j < ITERATIONS; ++j)
        {
            int32_t values1 = SinTable[(ang1Start + x) & (SINTABLEENTRIES - 1)];
            int32_t values2 = SinTable[(ang2Start + y) & (SINTABLEENTRIES - 1)];
            x = (int32_t)(int16_t)values1 + (int32_t)(int16_t)values2;
            y = (values1 >> 16) + (values2 >> 16);
            int32_t pX = (x * SCALEMUL) >> SINTABLEPOWER;
            int32_t pY = (y * SCALEMUL) >> SINTABLEPOWER;
            screenCentre[pY * SCREENWIDTH + pX] = ((i>>1)&0x70) + (j>>4) + flag;
        }
        ang1Start += ANG1INC;
        ang2Start += ANG2INC;
    }

    ang1Start = oldAnimationTime;
    ang2Start = oldAnimationTime;
    for (int i = 0; i < CURVECOUNT; i += CURVESTEP)
    {
        int32_t x = 0;
        int32_t y = 0;
        for (int j = 0; j < ITERATIONS; ++j)
        {
            int32_t values1 = SinTable[(ang1Start + x) & (SINTABLEENTRIES - 1)];
            int32_t values2 = SinTable[(ang2Start + y) & (SINTABLEENTRIES - 1)];
            x = (int32_t)(int16_t)values1 + (int32_t)(int16_t)values2;
            y = (values1 >> 16) + (values2 >> 16);
            int32_t pX = (x * SCALEMUL) >> SINTABLEPOWER;
            int32_t pY = (y * SCALEMUL) >> SINTABLEPOWER;
            if ((screenCentre[pY * SCREENWIDTH + pX] & 0x80) != flag)
            {
                screenCentre[pY * SCREENWIDTH + pX] = 0;
            }
        }
        ang1Start += ANG1INC;
        ang2Start += ANG2INC;
    }
}

void setup()
{
  Serial.begin(115200);
  display.begin();
  CreateSinTable();
  InitPalette();
}

uint64_t oldTime = millis();
void loop() {
    uint64_t time = millis();
    uint32_t deltaTime = time - oldTime;
    oldTime = time;

    flag ^= 0x80;

    oldAnimationTime = animationTime;
    animationTime = time * speed;
    Render(display.getBuffer());

    Serial.println(deltaTime);
      
    display.hstx.wait_for_vsync();
}