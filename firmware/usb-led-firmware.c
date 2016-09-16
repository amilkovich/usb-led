#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "usbdrv.h"

#define USB_LED_SET_RED   0x0
#define USB_LED_GET_RED   0x1
#define USB_LED_SET_GREEN 0x2
#define USB_LED_GET_GREEN 0x3
#define USB_LED_SET_BLUE  0x4
#define USB_LED_GET_BLUE  0x5

static uchar reply_buffer[1];

/*
 * the following inline assembly is inspired and implemented based on an article
 * by Mike Silva
 * http://www.embeddedrelated.com/showarticle/528.php
 *
 * asm function to output 24-bit rgb value in (g, r, b) order, msb first
 * r18 = red byte to be output
 * r19 = green byte to be output
 * r20 = blue byte to be output
 * r26 = saved sreg
 * r27 = inner loop counter
 */
void ws2812(uint8_t red, uint8_t green, uint8_t blue) {
	wdt_disable();
	asm volatile (
		// initialize
		"1:  mov r18, %A[red]     ; red byte                         \n"
		"    mov r19, %A[green]   ; green byte                       \n"
		"    mov r20, %A[blue]    ; blue byte                        \n"
		"    ldi r27, 8           ; load inner loop counter          \n"
		"    in r26, __SREG__     ; timing-critical, so no interrupts\n"
		"    cli                                                     \n"
		// red byte
		"2:  sbi  %[port], %[pin] ; pin lo -> hi                     \n"
		"    sbrc r18, 7          ; test hi bit clear                \n"
		"    rjmp 3f              ; true, skip pin hi -> lo          \n"
		"    cbi  %[port], %[pin] ; false, pin hi -> lo              \n"
		"3:  sbrc r18, 7          ; equalise delay of both code paths\n"
		"    rjmp 4f                                                 \n"
		"4:  nop                  ; pulse timing delay               \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    lsl r18              ; shift to next bit                \n"
		"    dec r27              ; decrement loop counter           \n"
		"    cbi %[port], %[pin]  ; pin hi -> lo                     \n"
		"    brne 2b\n            ; loop if required                 \n"
		"    ldi r27, 7           ; reload inner loop counter        \n"
		// green 7 bits
		"5:  sbi %[port], %[pin]  ; pin lo -> hi                     \n"
		"    sbrc r19, 7          ; test hi bit clear                \n"
		"    rjmp 6f              ; true, skip pin hi -> lo          \n"
		"    cbi %[port], %[pin]  ; false, pin hi -> lo              \n"
		"6:  sbrc r19, 7          ; equalise delay of both code paths\n"
		"    rjmp 7f                                                 \n"
		"7:  nop                  ; pulse timing delay               \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    lsl r19              ; shift to next bit                \n"
		"    dec r27              ; decrement inner loop counter     \n"
		"    cbi %[port], %[pin]  ; pin hi -> lo                     \n"
		"    brne 5b              ; inner loop, if required          \n"
		"    nop                  ; equalise delay of both code paths\n"
		// green 8th bit, output & fetch next values
		"    sbi %[port], %[pin]  ; pin lo -> hi                     \n"
		"    sbrc r18, 7          ; test hi bit clear                \n"
		"    rjmp 8f              ; true, skip pin hi -> lo          \n"
		"    cbi %[port], %[pin]  ; false, pin hi -> lo              \n"
		"8:  sbrc r18, 7          ; equalise delay of both code paths\n"
		"    rjmp 9f                                                 \n"
		"9:  nop                  ; pulse timing delay               \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    ldi r27, 7           ; reload inner loop counter        \n"
		"    cbi %[port], %[pin]  ; pin hi -> lo                     \n"
		"    nop                  ; pulse timing delay               \n"
		"    nop                                                     \n"
		// blue - loop over first 7 bits
		"10:  sbi %[port], %[pin] ; pin lo -> hi                     \n"
		"    sbrc r20, 7          ; test hi bit clear                \n"
		"    rjmp 11f             ; true, skip pin hi -> lo          \n"
		"    cbi %[port], %[pin]  ; false, pin hi -> lo              \n"
		"11: sbrc r20, 7          ; equalise delay of both code paths\n"
		"    rjmp 12f                                                \n"
		"12: nop                  ; pulse timing delay               \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    nop                                                     \n"
		"    lsl r20              ; shift to next bit                \n"
		"    dec r27              ; decrement inner loop counter     \n"
		"    cbi %[port], %[pin]  ; pin hi -> lo                     \n"
		"    brne 10b             ; inner loop, if required          \n"
		"    nop                  ; equalise delay of both code paths\n"
		// blue, 8th bit
		"    sbi %[port], %[pin]  ; pin lo -> hi                     \n"
		"    sbrc r20, 7          ; test hi bit clear                \n"
		"    rjmp 13f             ; true, skip pin hi -> lo          \n"
		"    cbi %[port], %[pin]  ; false, pin hi -> lo              \n"
		"13: sbrc r20, 7          ; equalise delay of both code paths\n"
		"    rjmp 14f                                                \n"
		"14: nop                  ; pulse timing delay               \n"
		"    nop                                                     \n"
		"15: nop                  ; pulse timing delay               \n"
		"    cbi %[port], %[pin]  ; pin hi -> lo                     \n"
		"    nop                  ; pulse timing delay               \n"
		"    nop                                                     \n"
		"    out __SREG__, r26    ; reenable interrupts              \n"
		"16:                                                         \n"
		:
		: [red] "w" (red),
		  [green] "w" (green),
		  [blue] "w" (blue),
		  [port] "I" (_SFR_IO_ADDR(PORTB)),
		  [pin] "I" (PB0)
		: "r18", "r19", "r20", "r26", "r27", "cc", "memory"
	);
	wdt_enable(WDTO_1S);
}

USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
	usbRequest_t *rq = (void *)data;

	switch (rq->bRequest) {
		case USB_LED_SET_RED:
			eeprom_write_byte((uint8_t *)0x0, rq->wValue.bytes[0]);
			ws2812(eeprom_read_byte((uint8_t *)0x0),
			       eeprom_read_byte((uint8_t *)0x1),
			       eeprom_read_byte((uint8_t *)0x2));
		break;

		case USB_LED_GET_RED:
			reply_buffer[0] = eeprom_read_byte((uint8_t *)0x0);
			usbMsgPtr = reply_buffer;
			return sizeof(reply_buffer);
		break;

		case USB_LED_SET_GREEN:
			eeprom_write_byte((uint8_t *)0x1, rq->wValue.bytes[0]);
			ws2812(eeprom_read_byte((uint8_t *)0x0),
			       eeprom_read_byte((uint8_t *)0x1),
			       eeprom_read_byte((uint8_t *)0x2));
		break;

		case USB_LED_GET_GREEN:
			reply_buffer[0] = eeprom_read_byte((uint8_t *)0x1);
			usbMsgPtr = reply_buffer;
			return sizeof(reply_buffer);
		break;

		case USB_LED_SET_BLUE:
			eeprom_write_byte((uint8_t *)0x2, rq->wValue.bytes[0]);
			ws2812(eeprom_read_byte((uint8_t *)0x0),
			       eeprom_read_byte((uint8_t *)0x1),
			       eeprom_read_byte((uint8_t *)0x2));
		break;

		case USB_LED_GET_BLUE:
			reply_buffer[0] = eeprom_read_byte((uint8_t *)0x2);
			usbMsgPtr = reply_buffer;
			return sizeof(reply_buffer);
		break;
	}

	return 0;
}

void hadUsbReset() {
	#define abs(x) ((x) > 0 ? (x) : (-x))
	int frame_length;
	int target_length = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);
	int best_deviation = 9999;
	uchar trial_cal, best_cal = 0, step, region;

	for (region = 0; region <= 1; region++) {
		frame_length = 0;
		trial_cal = (region == 0) ? 0 : 128;

		for (step = 64; step > 0; step >>= 1) {
			if (frame_length < target_length)
				trial_cal += step;
			else
				trial_cal -= step;

			OSCCAL = trial_cal;
			frame_length = usbMeasureFrameLength();

			if (abs(frame_length - target_length)
			    < best_deviation) {
				best_cal = trial_cal;
				best_deviation =
					abs(frame_length - target_length);
			}
		}
	}

	OSCCAL = best_cal;
}

int main() {
	DDRB = (1 << PB0);

	ws2812(eeprom_read_byte((uint8_t *)0x0),
	       eeprom_read_byte((uint8_t *)0x1),
	       eeprom_read_byte((uint8_t *)0x2));

	wdt_enable(WDTO_1S);

	usbInit();
	usbDeviceDisconnect();
	for (uint8_t i = 0; i < 250; i++) {
		wdt_reset();
		_delay_ms(2);
	}
	usbDeviceConnect();

	sei();

	while (1) {
		wdt_reset();
		usbPoll();
	}

	return 0;
}
