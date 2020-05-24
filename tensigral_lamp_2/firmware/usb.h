#ifndef _USB_H
#define _USB_H

#include <stdint.h>

extern volatile int usbDataOkToSend;
extern volatile int usbDataOkToRead;

void init_usb();
void usb_data( uint8_t * data, int len );

uint8_t * usb_get_out_ep_buffer();
void usb_release_out_ep_buffer();

//'length' is the size of the payload.
//'code' is the HID wValue.  This is usually the first byte in the HIDAPI stuff.
//It is STRONGLY recommended you use 0 as the first byte.  Experimentally, other values are NOT platform-safe.
void CBHIDSetup( uint16_t length, uint8_t code );
void CBHIDData( uint8_t paksize, uint8_t * data );
void CBHIDInterruptIn( uint8_t paklen, uint8_t * data );

#endif
