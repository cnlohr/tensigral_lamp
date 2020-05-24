#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

#include <lib/usb_defs.h>
//#include <wchar.h>

// You can change these to give your code its own name.
#define STR_MANUFACTURER	{ 'm', 0, 'f', 0, 'g', 0 }
#define STR_PRODUCT		    { 'p', 0, 'r', 0, 'd', 0 }
#define STR_PRODUCT_LEN (6+1)
#define STR_MANUFACTURER_LEN (6+1)

#define VENDOR_ID		0xabcd
#define PRODUCT_ID		0xf410

#define HARDWARE_INTERFACE 0

#define ENDPOINT0_SIZE CONTROL_MAX_PACKET_SIZE
#define ENDPOINT1_SIZE 64
#define ENDPOINT2_SIZE 64


#define LSB(x) ((x)&0xff)
#define MSB(x) ((x)>>8)


const static uint8_t hid_report_desc[] = {
        0x06, 0xC9, 0xFF,                       // Usage Page 0xFFC9 (vendor defined)
        0x09, 0x04,                             // Usage 0x04
        0xA1, 0x5C,                             // Collection 0x5C
        0x75, 0x08,                             // report size = 8 bits (global)
        0x15, 0x00,                             // logical minimum = 0 (global)
        0x26, 0xFF, 0x00,                       // logical maximum = 255 (global)
        0x95, 64,                               // report count (global) 64 bytes.
        0x09, 0x75,                             // usage (local)
        0x81, 0x02,                             // Input
        0x95, 64,                               // report count (global) INPUT REPORT SIZE
        0x09, 0x76,                             // usage (local)
        0x91, 0x02,                             // Output
        0x95, 64,            	                // report count (global)  OUTPUT REPORT SIZE [64]  (0x96, 0x00, 0x02,=512)
        0x09, 0x76,                             // usage (local)
        0xB1, 0x02,                             // Feature
        0xC0                                    // end collection

};


const static uint8_t device_descriptor[] = {
	18,					// bLength
	1,					// bDescriptorType
	0x10, 0x01,			// bcdUSB
	0,					// bDeviceClass
	0,					// bDeviceSubClass
	0,					// bDeviceProtocol
	ENDPOINT0_SIZE,				// bMaxPacketSize0
	LSB(VENDOR_ID), MSB(VENDOR_ID),		// idVendor
	LSB(PRODUCT_ID), MSB(PRODUCT_ID),	// idProduct
	0x01, 0x01,				// bcdDevice
	1,					// iManufacturer
	2,					// iProduct
	3,					// iSerialNumber
	1					// bNumConfigurations
};

#define CONFIG_DESCRIPTOR_SIZE (9+9+9+7+7)
#define HID_DESC_OFFSET (9+9)

const static uint8_t config_descriptor[CONFIG_DESCRIPTOR_SIZE] = {
	// configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
	9, 					// bLength;
	2,					// bDescriptorType;
	LSB(CONFIG_DESCRIPTOR_SIZE),			// wTotalLength
	MSB(CONFIG_DESCRIPTOR_SIZE),
	1,					// bNumInterfaces
	1,					// bConfigurationValue
	0,					// iConfiguration
	0x80,				// bmAttributes (0xC0 is self-powered)
	20,					// bMaxPower

	9,					// bLength
	4,					// bDescriptorType
	HARDWARE_INTERFACE,	// bInterfaceNumber (unused, would normally be used for HID)
	0,					// bAlternateSetting
	2,					// bNumEndpoints
	3,					// bInterfaceClass 
	0xff,				// bInterfaceSubClass (Was 0xff) 1 = Boot device subclass.
	0xff,				// bInterfaceProtocol (Was 0xff) 1 = keyboard, 2 = mouse.
	0,					// iInterface


	9,					// bLength
	0x21,				// bDescriptorType [33]
	0x01, 0x01,			// bcdHID
	0,					// bCountryCode
	1,					// bNumDescriptors
	0x22,					// bDescriptorType (Normally 0x22)
	LSB(sizeof(hid_report_desc)),	// wDescriptorLength
	MSB(sizeof(hid_report_desc)),


	7,					// bLength
	5,					// bDescriptorType
	IN_ENDPOINT_ADDRESS,					// bEndpointAddress (IN, 1)
	0x03,				// bmAttributes
	LSB(ENDPOINT1_SIZE), MSB(ENDPOINT1_SIZE),			// wMaxPacketSize
	1,					// bInterval */


	7,					// bLength
	5,					// bDescriptorType
	OUT_ENDPOINT_ADDRESS,					// bEndpointAddress (OUT, 2)
	0x03,				// bmAttributes (Interrupt)
	LSB(ENDPOINT2_SIZE), MSB(ENDPOINT2_SIZE),			// wMaxPacketSize
	1,					// bInterval 
};



// If you're desperate for a little extra code memory, these strings
// can be completely removed if iManufacturer, iProduct, iSerialNumber
// in the device desciptor are changed to zeros.
struct usb_string_descriptor_struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t wString[];
};


static const struct usb_string_descriptor_struct string0 = {
	4,
	3,
	//L"\x0409"
	{ 0x09, 0x04, 0x00, 0x00 }
};
static const struct usb_string_descriptor_struct string1 = {
	STR_MANUFACTURER_LEN,
	3,
	STR_MANUFACTURER
};

static const struct usb_string_descriptor_struct string2 = {
	STR_PRODUCT_LEN,
	3,
	STR_PRODUCT
};
static struct usb_string_descriptor_struct string3 = {
	16,
	3,
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00 }  //This will be filled in with the UID on boot.
};


// This table defines which descriptor data is sent for each specific
// request from the host (in wValue and wIndex).
const static struct descriptor_list_struct {
	uint16_t	wValue;
	uint16_t	wIndex;
	const uint8_t	*addr;
	uint8_t		length;
} descriptor_list[] = {
	{0x0100, 0x0000, device_descriptor, sizeof(device_descriptor)},
	{0x0200, 0x0000, config_descriptor, sizeof(config_descriptor)},
	{0x2200, HARDWARE_INTERFACE, hid_report_desc, sizeof(hid_report_desc)},
	{0x2100, HARDWARE_INTERFACE, config_descriptor+HID_DESC_OFFSET, 9},
	{0x0300, 0x0000, (const uint8_t *)&string0, 4},
	{0x0301, 0x0409, (const uint8_t *)&string1, STR_MANUFACTURER_LEN},
	{0x0302, 0x0409, (const uint8_t *)&string2, STR_PRODUCT_LEN},	
	{0x0303, 0x0409, (const uint8_t *)&string3, 16}
};
#define NUM_DESC_LIST (sizeof(descriptor_list)/sizeof(struct descriptor_list_struct))


#endif

