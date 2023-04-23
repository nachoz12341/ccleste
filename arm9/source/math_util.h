#include <nds/arm9/trig_lut.h>
#include <nds/arm9/math.h>
#include <stdio.h>

#define SIN_BITS 12
#define MATH_BITS 4

float sinf(float x){
    s16 a = floatToFixed(x,SIN_BITS);
    return fixedToFloat(sinLerp(a),SIN_BITS);
}

float floorf(float x){
    s16 a = floatToFixed(x,MATH_BITS);
    return fixedToFloat(floorFixed(a,MATH_BITS),MATH_BITS);
}

float fmodf(float x, float den)
{
    int32 int_x = f32toint(x);
    int32 int_den = f32toint(den);
    return inttof32(mod32(int_x,int_den));
}

