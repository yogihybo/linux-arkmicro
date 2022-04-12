#ifndef _F_APPLE_COMMON_H
#define _F_APPLE_COMMON_H

#define APPLE_BULK_BUFFER_SIZE           16384
#define APPLE_TX_REQ_MAX 4

#define EP_ASSIGN_AT_CONN   1

struct usb_ep *usb_ep_autoconfig_ex(
	struct usb_gadget		*gadget,
	struct usb_endpoint_descriptor	*desc,
	uint8_t pre_ep_num
);


#endif
