/*
 * f_apple_ptp_sim.c - USB peripheral apple_ptp_sim configuration driver
 *
 * Copyright (C) 2020 Arkmicro Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* #define VERBOSE_DEBUG */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/usb/composite.h>
#include <linux/usb/functionfs.h>
#include <net/sock.h>
#include <linux/netlink.h>


#define NETLINK_USB_DEV_READY  26

static struct sock *netlink_handle;

int usb_dev_ready_notify(int is_ready)
{
	struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;
	char *buf_on = "connected";
	char *buf_off = "disconnected";
	char *buf_hid_ready = "hidready";
	char *buf = NULL;
	int len = 0;
    int ret;

	if (!netlink_handle) {
		
	}

	if (is_ready == 1) {
		buf = buf_on;printk("@%s:%d %s\n", __func__, __LINE__, buf_on);
	} else if (is_ready == 2) {
		buf = buf_hid_ready;printk("%s:%d %s\n", __func__, __LINE__, buf);
	} else {
		buf = buf_off;printk("%s:%d %s\n", __func__, __LINE__, buf_off);
	}
	len = strlen(buf) + 1;

    nl_skb = nlmsg_new(len, GFP_ATOMIC);
    if(!nl_skb) {
        printk("netlink_alloc_skb error\n");
        return -1;
    }

    nlh = nlmsg_put(nl_skb, 0, 0, NETLINK_USB_DEV_READY, len, 0);
    if(nlh == NULL) {
        printk("nlmsg_put() error\n");
        nlmsg_free(nl_skb);
        return -1;
    }
	
    memcpy(nlmsg_data(nlh), buf, len);

	ret = netlink_unicast(netlink_handle, nl_skb, 53, MSG_DONTWAIT);
//exit:
    return ret;
}
EXPORT_SYMBOL(usb_dev_ready_notify);

struct f_apple_ptp_sim {
	struct usb_function	function;
};

struct f_apple_ptp_sim_opts {
	struct usb_function_instance func_inst;

	struct mutex			lock;
	int				refcnt;
	unsigned		iso_qlen;
};

static inline struct f_apple_ptp_sim *func_to_apple_ptp_sim(struct usb_function *f)
{
	return container_of(f, struct f_apple_ptp_sim, function);
}

static struct usb_interface_descriptor apple_ptp_sim_intf = {
	.bLength =		sizeof apple_ptp_sim_intf,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bNumEndpoints =	3,
	.bInterfaceClass =	USB_CLASS_STILL_IMAGE,
	.bInterfaceSubClass = 1,
	.bInterfaceProtocol = 1,
	/* .iInterface = DYNAMIC */
};

/* full speed support: */

static struct usb_endpoint_descriptor fs_apple_ptp_sim_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_apple_ptp_sim_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_apple_ptp_sim_intr_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.bInterval		= 10,
};

static struct usb_descriptor_header *fs_apple_ptp_sim_descs[] = {
	(struct usb_descriptor_header *) &apple_ptp_sim_intf,
	(struct usb_descriptor_header *) &fs_apple_ptp_sim_sink_desc,
	(struct usb_descriptor_header *) &fs_apple_ptp_sim_source_desc,
	(struct usb_descriptor_header *) &fs_apple_ptp_sim_intr_desc,
	NULL,
};

/* high speed support: */

static struct usb_endpoint_descriptor hs_apple_ptp_sim_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_apple_ptp_sim_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_apple_ptp_sim_intr_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(64),
	.bInterval		= 10,
};

static struct usb_descriptor_header *hs_apple_ptp_sim_descs[] = {
	(struct usb_descriptor_header *) &apple_ptp_sim_intf,
	(struct usb_descriptor_header *) &hs_apple_ptp_sim_sink_desc,
	(struct usb_descriptor_header *) &hs_apple_ptp_sim_source_desc,
	(struct usb_descriptor_header *) &hs_apple_ptp_sim_intr_desc,
	NULL,
};

/* super speed support: */

static struct usb_endpoint_descriptor ss_apple_ptp_sim_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor ss_apple_ptp_sim_source_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst =		0,
	.bmAttributes =		0,
	.wBytesPerInterval =	0,
};

static struct usb_endpoint_descriptor ss_apple_ptp_sim_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor ss_apple_ptp_sim_sink_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst =		0,
	.bmAttributes =		0,
	.wBytesPerInterval =	0,
};

static struct usb_descriptor_header *ss_apple_ptp_sim_descs[] = {
	(struct usb_descriptor_header *) &apple_ptp_sim_intf,
	(struct usb_descriptor_header *) &ss_apple_ptp_sim_sink_desc,
	(struct usb_descriptor_header *) &ss_apple_ptp_sim_sink_comp_desc,
	(struct usb_descriptor_header *) &ss_apple_ptp_sim_source_desc,
	(struct usb_descriptor_header *) &ss_apple_ptp_sim_source_comp_desc,
	
	NULL,
};

#define FUNC_PTP_IDX	0

static struct usb_string apple_ptp_sim_string_defs[] = {
	[FUNC_PTP_IDX].s	= "PTP",
	{},
};

static struct usb_gadget_strings apple_ptp_sim_string_table = {
	.language	= 0x0409,	/* en-US */
	.strings	= apple_ptp_sim_string_defs,
};

static struct usb_gadget_strings *apple_ptp_sim_strings[] = {
	&apple_ptp_sim_string_table,
	NULL,
};

static int apple_ptp_sim_bind(struct usb_configuration *c, struct usb_function *f)
{
	int			id;
	struct usb_string	*us;
printk("%s:%d\n", __func__, __LINE__);
	/* maybe allocate device-global string IDs, and patch descriptors */
	us = usb_gstrings_attach(c->cdev, apple_ptp_sim_strings,
				 ARRAY_SIZE(apple_ptp_sim_string_defs));
	if (IS_ERR(us))
		return PTR_ERR(us);

	apple_ptp_sim_intf.iInterface = us[FUNC_PTP_IDX].id;

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	apple_ptp_sim_intf.bInterfaceNumber = id;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		hs_apple_ptp_sim_source_desc.bEndpointAddress = 0x81;
		hs_apple_ptp_sim_sink_desc.bEndpointAddress = 0x02;
		hs_apple_ptp_sim_intr_desc.bEndpointAddress = 0x83;
		f->hs_descriptors = hs_apple_ptp_sim_descs;
	}

	/* support super speed hardware */
	if (gadget_is_superspeed(c->cdev->gadget)) {
		ss_apple_ptp_sim_source_desc.bEndpointAddress =
				fs_apple_ptp_sim_source_desc.bEndpointAddress;
		ss_apple_ptp_sim_sink_desc.bEndpointAddress =
				fs_apple_ptp_sim_sink_desc.bEndpointAddress;
		f->ss_descriptors = ss_apple_ptp_sim_descs;
	}

	return 0;
}

static void apple_ptp_sim_unbind(struct usb_configuration *c, struct usb_function *f)
{printk("%s:%d\n", __func__, __LINE__);
	(void)f;
	(void)c;
}

static int apple_ptp_sim_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{printk("%s:%d\n", __func__, __LINE__);
	(void)f;
	(void)intf;
	(void)alt;
	return 0;
}

static void apple_ptp_sim_disable(struct usb_function *f)
{printk("%s:%d\n", __func__, __LINE__);
	(void)f;
}

static bool apple_ptp_sim_req_match(struct usb_function *f,
					const struct usb_ctrlrequest *ctrl,
					bool config0)
{
	
#if 0
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);
printk("@%s:%d\n", __func__, __LINE__);

	if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE ||
	    (ctrl->bRequestType & USB_TYPE_MASK) != USB_TYPE_CLASS)
		return false;
printk("%s:%d\n", __func__, __LINE__);

	switch (ctrl->bRequest) {
	case 0x40:
		if (!w_value && w_length == 1 &&
		    (USB_DIR_IN & ctrl->bRequestType))
			break;
		return false;
	case 0xc0:
		if (!w_value && !w_length &&
		   !(USB_DIR_IN & ctrl->bRequestType))
			break;
		/* fall through */
	default:
		return false;
	}
#else
	(void)f;
	(void)ctrl;
	(void)config0;
#endif
	return true;
}

static int apple_ptp_sim_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);
	char result[4] = {0x01, 0x00, 0x00, 0x00};

	printk("%s-->bRequestType=%02x bRequest=%02x\n", __func__,  ctrl->bRequestType, ctrl->bRequest);
	switch (ctrl->bRequestType) {
	case 0x40 :
		printk("%s %x-->receive switch ctrl\n",  __func__, ctrl->bRequest);
		if (ctrl->bRequest != 0x51)
			break;
		value = 0;
		usb_dev_ready_notify(1);
		break;
	case 0xc0 :
		printk("%s %x-->receive query ctrl\n", __func__, ctrl->bRequest);
		if (ctrl->bRequest != 0x53)
			break;
		value = 4;
		memcpy(req->buf, result, value);
		break;
	default:
		value = 0;
		DBG(cdev, "invalid control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

	if (value >= 0) {
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(cdev, "ncm req %02x.%02x response err %d\n",
					ctrl->bRequestType, ctrl->bRequest,
					value);
	}

	return value;
}

static void apple_ptp_sim_free_func(struct usb_function *f)
{
	struct f_apple_ptp_sim_opts *opts;

	opts = container_of(f->fi, struct f_apple_ptp_sim_opts, func_inst);

	mutex_lock(&opts->lock);
	opts->refcnt--;
	mutex_unlock(&opts->lock);
printk("%s:%d\n", __func__, __LINE__);
	kfree(func_to_apple_ptp_sim(f));
}

static inline struct f_apple_ptp_sim_opts *to_f_apple_ptp_sim_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_apple_ptp_sim_opts,
			    func_inst.group);
}

static ssize_t f_apple_ptp_sim_opts_iso_qlen_show(struct config_item *item, char *page)
{
	struct f_apple_ptp_sim_opts *opts = to_f_apple_ptp_sim_opts(item);
	int result;

	mutex_lock(&opts->lock);
	result = sprintf(page, "%u\n", opts->iso_qlen);
	mutex_unlock(&opts->lock);

	return result;
}

static ssize_t f_apple_ptp_sim_opts_iso_qlen_store(struct config_item *item,
					   const char *page, size_t len)
{
	struct f_apple_ptp_sim_opts *opts = to_f_apple_ptp_sim_opts(item);
	int ret;
	u32 num;

	mutex_lock(&opts->lock);
	if (opts->refcnt) {
		ret = -EBUSY;
		goto end;
	}

	ret = kstrtou32(page, 0, &num);
	if (ret)
		goto end;

	opts->iso_qlen = num;
	ret = len;
end:
	mutex_unlock(&opts->lock);
	return ret;
}

static void apple_ptp_sim_attr_release(struct config_item *item)
{
	struct f_apple_ptp_sim_opts *ss_opts = to_f_apple_ptp_sim_opts(item);

	usb_put_function_instance(&ss_opts->func_inst);
}

static struct configfs_item_operations apple_ptp_sim_item_ops = {
	.release		= apple_ptp_sim_attr_release,
};


CONFIGFS_ATTR(f_apple_ptp_sim_opts_, iso_qlen);

static struct configfs_attribute *apple_ptp_sim_attrs[] = {
	&f_apple_ptp_sim_opts_attr_iso_qlen,
	NULL,
};

static struct config_item_type apple_ptp_sim_func_type = {
	.ct_item_ops    = &apple_ptp_sim_item_ops,
	.ct_attrs	= apple_ptp_sim_attrs,
	.ct_owner       = THIS_MODULE,
};

static void apple_ptp_sim_free_inst(struct usb_function_instance *f)
{
	struct f_apple_ptp_sim_opts *opts;
printk("%s:%d\n", __func__, __LINE__);
	opts = container_of(f, struct f_apple_ptp_sim_opts, func_inst);

	kfree(opts);

}

static struct usb_function_instance *apple_ptp_sim_alloc_inst(void)
{
	struct f_apple_ptp_sim_opts *opts = NULL;
printk("%s:%d\n", __func__, __LINE__);
	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);

	mutex_init(&opts->lock);
	opts->func_inst.free_func_inst = apple_ptp_sim_free_inst;

	config_group_init_type_name(&opts->func_inst.group, "", &apple_ptp_sim_func_type);
printk("%s:%d\n", __func__, __LINE__);
	return &opts->func_inst;
}
static struct netlink_kernel_cfg cfg = {
	.groups	= 1,
	.flags	= NL_CFG_F_NONROOT_RECV,
};

static struct usb_function *apple_ptp_sim_alloc(struct usb_function_instance *fi)
{
	struct f_apple_ptp_sim	*pctx = NULL;
	struct f_apple_ptp_sim_opts *opts = NULL;

	pctx = kzalloc(sizeof *pctx, GFP_KERNEL);
	if (!pctx)
		return NULL;
printk("%s:%d\n", __func__, __LINE__);
	opts =  container_of(fi, struct f_apple_ptp_sim_opts, func_inst);

	mutex_lock(&opts->lock);
	opts->refcnt++;
	mutex_unlock(&opts->lock);

	pctx->function.name = "ptp";
	pctx->function.fs_descriptors = fs_apple_ptp_sim_descs;
	pctx->function.bind = apple_ptp_sim_bind;
	pctx->function.unbind = apple_ptp_sim_unbind;
	pctx->function.set_alt = apple_ptp_sim_set_alt;
	pctx->function.disable = apple_ptp_sim_disable;
	pctx->function.setup = apple_ptp_sim_setup;
	pctx->function.req_match = apple_ptp_sim_req_match;
	pctx->function.free_func = apple_ptp_sim_free_func;

	if (NULL == netlink_handle) {
		printk("##alloc usb netlink sock\n");
		netlink_handle = netlink_kernel_create(&init_net, NETLINK_USB_DEV_READY, &cfg);
	    if(!netlink_handle) {
	        printk(KERN_ERR "can not create a usb dev ready netlink socket!\n");
	        kfree(pctx);
			return NULL;
	    }
	}
printk("%s:%d\n", __func__, __LINE__);
	return &pctx->function;
}


DECLARE_USB_FUNCTION_INIT(ptp, apple_ptp_sim_alloc_inst, apple_ptp_sim_alloc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("art");

