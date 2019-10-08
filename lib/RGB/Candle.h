#ifndef CANDLE_INCLUDE_h
#define CANDLE_INCLUDE_h

#include <Arduino.h>
#include <Color/Color.h>

struct Candle
{

  Candle(uint8_t rand_add = 250, uint16_t vary = 0, uint8_t rand_color = 0, uint16_t timer = 100) : vary(vary), rand_color(rand_color), timer(timer) {}

  uint16_t vary;
  uint8_t rand_color;
  uint16_t timer;
  static const uint8_t s = 7;
  Color color_list[s] = {{255, 235, 50}, {145, 64, 30}, {225, 105, 40}, {235, 205, 12}, {128, 95, 50}, {64, 50, 20}, {255, 127, 0}};

public:
  uint8_t RandColor(void)
  {
    uint8_t r;
    while ((r = ((uint8_t)rand() % s)) == rand_color)
      ;
    rand_color = r;
    return rand_color;
  } // make sure next random number is different than last
  uint16_t RandTimer(void)
  {
    timer = (rand() % (vary * 2 + 50)) + vary;
    return timer;
  } // randomness
  void Blend(RGBLEDBlender &blender)
  {
    Reset();
    blender.Blend(blender.GetColor(), color_list[RandColor()], RandTimer());
  } // blend colors
  void Reset(void)
  {
    vary = rand() % (timer); // random number variance
    if (vary > 32718)
      vary = 32717; // prevents overflow on next random number;
  }
};

#endif
