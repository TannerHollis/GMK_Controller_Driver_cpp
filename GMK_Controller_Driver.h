#ifndef GMK_CONTROLLER_DRIVER_H
#define GMK_CONTROLLER_DRIVER_H

#include <windows.h>
#include <XInput.h>
#include <ViGEm/Client.h>
#include <libusb.h>
#include <iostream>
#include <inttypes.h>
#include <conio.h>
#pragma comment (lib, "setupapi.lib")

typedef struct {
    uint8_t report_id;
    uint8_t buttons[2];
    uint8_t joysticks[8];
    uint8_t triggers[2];
} HID_Report_In_TypeDef;

typedef struct {
	union {
		struct {
			uint8_t up : 1;
			uint8_t down : 1;
			uint8_t left : 1;
			uint8_t right : 1;
			uint8_t start : 1;
			uint8_t back : 1;
			uint8_t lth : 1;
			uint8_t rth : 1;
			uint8_t lb : 1;
			uint8_t rb : 1;
			uint8_t xbox : 1;
			uint8_t _reserved : 1;
			uint8_t a : 1;
			uint8_t b : 1;
			uint8_t x : 1;
			uint8_t y : 1;
		};
		uint16_t _bits;
	} buttons;
	union {
		struct {
			struct {
				int16_t x;
				int16_t y;
			} left;
			struct {
				int16_t x;
				int16_t y;
			} right;
		};
		int16_t _bits[4];
	}joysticks;
	union {
		struct {
			uint8_t left;
			uint8_t right;
		};
		uint8_t _bits[2];
	} triggers;
} Controller_Data_TypeDef;

#endif