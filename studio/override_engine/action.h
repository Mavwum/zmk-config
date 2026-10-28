#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct {uint8_t row,col;} keypos_t;
typedef struct {struct {keypos_t key;bool pressed;} event;} keyrecord_t;
