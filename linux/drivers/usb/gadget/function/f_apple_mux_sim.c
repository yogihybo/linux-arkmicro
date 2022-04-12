/*
 * f_apple_mux_sim.c - USB peripheral apple_mux_sim configuration driver
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
#include <linux/types.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include "f_apple_common.h"

#define MUX_BULK_BUFFER_SIZE           16384

#define MUX_TX_REQ_MAX 4

struct f_apple_mux_sim {
	struct usb_function	function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;

	struct usb_ep *ep_in;
	struct usb_ep *ep_out;
	uint8_t	ep_addr_in;
	uint8_t	ep_addr_out;

	int online;
	int error;

	atomic_t read_excl;
	atomic_t write_excl;
	atomic_t open_excl;

	struct list_head tx_idle;
	struct list_head rx_idle;
	struct list_head rx_used;

	wait_queue_head_t read_wq;
	wait_queue_head_t write_wq;

	int rx_done;
	int cur_read_pos;
	struct miscdevice device;
	struct list_head node;
	char *name;
};

struct f_apple_mux_sim_opts {
	struct usb_function_instance func_inst;

	struct mutex			lock;
	int				refcnt;
	unsigned		iso_qlen;
};

static DEFINE_MUTEX(apple_mux_mutex);
static LIST_HEAD(apple_mux_list);
static int apple_mux_setup(struct f_apple_mux_sim *dev, int intc, int intf);
static void apple_mux_cleanup(struct f_apple_mux_sim *dev);

int usb_dev_ready_notify(int is_ready);
static inline struct f_apple_mux_sim *func_to_apple_mux_sim(struct usb_function *f)
{
	return container_of(f, struct f_apple_mux_sim, function);
}


static struct usb_interface_descriptor apple_mux_sim_intf = {
	.bLength =		sizeof apple_mux_sim_intf,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = USB_CLASS_APP_SPEC,
	.bInterfaceProtocol = 2,
	/* .iInterface = DYNAMIC */
};

/* full speed support: */

static struct usb_endpoint_descriptor fs_apple_mux_sim_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_apple_mux_sim_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};


static struct usb_descriptor_header *fs_apple_mux_sim_descs[] = {
	(struct usb_descriptor_header *) &apple_mux_sim_intf,
	(struct usb_descriptor_header *) &fs_apple_mux_sim_sink_desc,
	(struct usb_descriptor_header *) &fs_apple_mux_sim_source_desc,
	NULL,
};

/* high speed support: */

static struct usb_endpoint_descriptor hs_apple_mux_sim_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_apple_mux_sim_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};


static struct usb_descriptor_header *hs_apple_mux_sim_descs[] = {
	(struct usb_descriptor_header *) &apple_mux_sim_intf,
	(struct usb_descriptor_header *) &hs_apple_mux_sim_sink_desc,
	(struct usb_descriptor_header *) &hs_apple_mux_sim_source_desc,
	NULL,
};

/* super speed support: */

static struct usb_endpoint_descriptor ss_apple_mux_sim_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor ss_apple_mux_sim_source_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst =		0,
	.bmAttributes =		0,
	.wBytesPerInterval =	0,
};

static struct usb_endpoint_descriptor ss_apple_mux_sim_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor ss_apple_mux_sim_sink_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst =		0,
	.bmAttributes =		0,
	.wBytesPerInterval =	0,
};

static struct usb_descriptor_header *ss_apple_mux_sim_descs[] = {
	(struct usb_descriptor_header *) &apple_mux_sim_intf,
	(struct usb_descriptor_header *) &ss_apple_mux_sim_sink_desc,
	(struct usb_descriptor_header *) &ss_apple_mux_sim_sink_comp_desc,
	(struct usb_descriptor_header *) &ss_apple_mux_sim_source_desc,
	(struct usb_descriptor_header *) &ss_apple_mux_sim_source_comp_desc,
	NULL,
};

#define FUNC_MUX_IDX	0

static struct usb_string apple_mux_sim_string_defs[] = {
	[FUNC_MUX_IDX].s	= "Apple USB Multiplexor",
	{},
};

static struct usb_gadget_strings apple_mux_sim_string_table = {
	.language	= 0x0409,	/* en-US */
	.strings	= apple_mux_sim_string_defs,
};

static struct usb_gadget_strings *apple_mux_sim_strings[] = {
	&apple_mux_sim_string_table,
	NULL,
};

static struct usb_request *apple_mux_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req)
		return NULL;

	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void apple_mux_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

static inline int apple_mux_lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1) {
		return 0;
	} else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void apple_mux_unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

void apple_mux_req_put(struct f_apple_mux_sim *dev, struct list_head *head,
		struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* remove a request from the head of a list */
struct usb_request *apple_mux_req_get(struct f_apple_mux_sim *dev, struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}

static void apple_mux_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct f_apple_mux_sim *dev = (struct f_apple_mux_sim *)(req->context);

	if (req->status != 0){//printk("+++%s:%d+++req->status=%d\n", __func__, __LINE__, req->status);
		dev->error = 1;
	}
	apple_mux_req_put(dev, &dev->tx_idle, req);

	wake_up(&dev->write_wq);
}

static void apple_mux_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct f_apple_mux_sim *dev = (struct f_apple_mux_sim *)(req->context);
	//unsigned long flags;

	dev->rx_done = 1;
	if (req) {
		if (req->status != 0){//printk("+++%s:%d+++req->status=%d\n", __func__, __LINE__, req->status);
			apple_mux_req_put(dev, &dev->rx_idle, req);
			dev->error = 1;
		} else {
			apple_mux_req_put(dev, &dev->rx_used, req);
		}
	}
	wake_up(&dev->read_wq);
}

static int apple_mux_create_bulk_endpoints(struct f_apple_mux_sim *dev,
				struct usb_endpoint_descriptor *in_desc,
				struct usb_endpoint_descriptor *out_desc)
{
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req;
	struct usb_ep *ep;
	int i;

	DBG(cdev, "create_bulk_endpoints dev: %p\n", dev);

	ep = usb_ep_autoconfig(cdev->gadget, in_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_in failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for ep_in got %s\n", ep->name);
	ep->driver_data = dev;
	ep->desc = in_desc;
	dev->ep_in = ep;
	

	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_out failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for iap2 ep_out got %s\n", ep->name);
	ep->driver_data = dev;
	ep->desc = out_desc;
	dev->ep_out = ep;

	for (i = 0; i < MUX_TX_REQ_MAX; i++) {
		req = apple_mux_request_new(dev->ep_out, MUX_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = apple_mux_complete_out;
		req->context = (void *)dev;
		apple_mux_req_put(dev, &dev->rx_idle, req);
	}

	for (i = 0; i < MUX_TX_REQ_MAX; i++) {
		req = apple_mux_request_new(dev->ep_in, MUX_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = apple_mux_complete_in;
		req->context = (void *)dev;
		apple_mux_req_put(dev, &dev->tx_idle, req);
	}

	return 0;

fail:
	printk(KERN_ERR "apple_mux_bind() could not allocate requests\n");
	return -1;
}

static int apple_mux_rx_submit(struct f_apple_mux_sim *dev, struct usb_request *req)
{
	int ret, r;

	dev->rx_done = 0;
	ret = usb_ep_queue(dev->ep_out, req, GFP_ATOMIC);
	if (ret < 0) {
		pr_debug("apple_mux_read: failed to queue req %p (%d)\n", req, ret);
		r = -EIO;
		dev->error = 1;
		goto done;
	} else {
		pr_debug("rx %p queue\n", req);
	}
done:
	pr_debug("apple_mux_read returning %d\n", r);
	return r;
}

static ssize_t apple_mux_read(struct file *fp, char __user *buf,
				size_t count, loff_t *pos)
{
	struct f_apple_mux_sim *dev = fp->private_data;
	struct usb_request *req = NULL;
	int r = 0, xfer, cur_actual = 0;
	int ret;
	//static int cur_pos = 0;
	unsigned long flags;

	pr_debug("apple_mux_read(%d) n", count);
	if (!dev)
		return -ENODEV;

	if (count <= 0)
		return 0;

	if (apple_mux_lock(&dev->read_excl))
		return -EBUSY;

	if (dev->error || dev->online == 0) {
		r = -EIO;
		goto done;
	}

	req = NULL;
	ret = wait_event_interruptible(dev->read_wq, (req = apple_mux_req_get(dev, &dev->rx_used)) != NULL);
	if (ret < 0) {
		r = ret;
		usb_ep_dequeue(dev->ep_out, req);
		goto done;
	}

	if (!dev->error && req) {
		if (req->actual > dev->cur_read_pos) {
			cur_actual = req->actual - dev->cur_read_pos;
			xfer = (cur_actual <= count) ? cur_actual : count;
			r = xfer;
			if (copy_to_user(buf, (req->buf + dev->cur_read_pos), xfer))
					r = -EFAULT;

		}
		if (count >= cur_actual) {
			apple_mux_rx_submit(dev, req);//the req is read completely, return to the usb core
			dev->cur_read_pos = 0;
		} else {
			dev->cur_read_pos += count;
			spin_lock_irqsave(&dev->lock, flags);//the request buffer isn't completely read ,return the req to the head of the list of rx_used
			list_add(&req->list, &dev->rx_used);
			spin_unlock_irqrestore(&dev->lock, flags);
		}
	} else {// IO error
		if (dev->error)
			r = -EIO;
		if (dev->online == 0)
			r = -ENODEV;
		if (req) {
			spin_lock_irqsave(&dev->lock, flags);
			list_add(&req->list, &dev->rx_used);
			spin_unlock_irqrestore(&dev->lock, flags);
		}
	}

done:
	apple_mux_unlock(&dev->read_excl);
	pr_debug("apple_mux_read returning %d\n", r);
	return r;
}

static ssize_t apple_mux_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	struct f_apple_mux_sim *dev = fp->private_data;
	struct usb_request *req = 0;
	int r = count, xfer;
	int ret;

	if (!dev)
		return -ENODEV;
	pr_debug("apple_mux_write(%d)\n", count);

	if (apple_mux_lock(&dev->write_excl))
		return -EBUSY;

	while (count > 0) {
		if (dev->error) {
			pr_debug("apple_mux_write dev->error\n");
			r = -EIO;
			break;
		}
		if (dev->online == 0) {
			pr_debug("apple_mux_write dev->online == 0\n");
			r = -ENODEV;
			break;
		}

		req = 0;
		ret = wait_event_interruptible_timeout(dev->write_wq,
			((req = apple_mux_req_get(dev, &dev->tx_idle)) || (dev->error) || (dev->online == 0)), msecs_to_jiffies(1000));
		if (ret <= 0) {
			r = ret;
			break;
		}

		if (req != 0) {
			if (count > MUX_BULK_BUFFER_SIZE)
				xfer = MUX_BULK_BUFFER_SIZE;
			else
				xfer = count;
			if (copy_from_user(req->buf, buf, xfer)) {
				r = -EFAULT;
				break;
			}

			req->length = xfer;
			ret = usb_ep_queue(dev->ep_in, req, GFP_ATOMIC);
			if (ret < 0) {
				pr_debug("apple_mux_write: xfer error %d\n", ret);
				dev->error = 1;
				r = -EIO;
				break;
			}

			buf += xfer;
			count -= xfer;
			req = 0;
		}
	}

	if (req)
		apple_mux_req_put(dev, &dev->tx_idle, req);

//done:
	apple_mux_unlock(&dev->write_excl);
	pr_debug("apple_mux_write returning %d\n", r);
	return r;
}


static struct f_apple_mux_sim *apple_mux_locate(int minor)
{
	struct f_apple_mux_sim *pctx;

	list_for_each_entry(pctx, &apple_mux_list, node) {
		if (pctx->device.minor == minor)
			return pctx;
	}

	return NULL;
}

static int apple_mux_open(struct inode *ip, struct file *fp)
{
	int retval = -1;
	struct f_apple_mux_sim *pctx;

	retval = mutex_lock_interruptible(&apple_mux_mutex);
	if (retval)
		return retval;

	pctx = apple_mux_locate(iminor(ip));

	if (!pctx) {
		retval = -ENODEV;
		goto out;
	}
	
	if (apple_mux_lock(&pctx->open_excl))
		return -EBUSY;

	fp->private_data = pctx;
	pctx->error = 0;
	pctx->rx_done = 0;
	//printk(KERN_INFO "apple_mux_open\n", g_apple_mux_dev->online);
out:
	mutex_unlock(&apple_mux_mutex);
	return retval;
}

static int apple_mux_release(struct inode *ip, struct file *fp)
{
	struct f_apple_mux_sim *dev = fp->private_data;
	//printk(KERN_INFO "apple_mux_release\n");
	apple_mux_unlock(&dev->open_excl);
	return 0;
}

static unsigned int apple_mux_poll(struct file *fp, struct poll_table_struct *wait)
{
	struct f_apple_mux_sim *dev = fp->private_data;
	unsigned int mask = 0;
//printk("dev->online = %d, dev->error = %d dev->rx_done = %d \n", dev->online, dev->error, dev->rx_done);
	poll_wait(fp, &dev->write_wq, wait);
	poll_wait(fp, &dev->read_wq, wait);

	if (fp->f_mode & FMODE_READ && dev->online && (!dev->error) && (!list_empty_careful(&dev->rx_used)))
		mask |= POLLIN| POLLRDNORM;
	if (fp->f_mode & FMODE_WRITE && dev->online && (!dev->error) && (!list_empty_careful(&dev->tx_idle)))
		mask |= POLLOUT | POLLWRNORM;

	return mask;
}

#define MUX_IOCTL_BASE    0xb7
#define MUX_GET_ONLINE_STATE  _IOW(MUX_IOCTL_BASE, 0, unsigned long)

static long apple_mux_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	struct f_apple_mux_sim *dev = fp->private_data;
	long res = 0;
	int online = 0;

	switch(cmd) {
	case MUX_GET_ONLINE_STATE:
		//printk("dev->online = %d\n", dev->online);
		online = dev->online;
		 if (copy_to_user((void*)arg, &online, sizeof(online))) {
			return -EFAULT;
		 }
		break;
	}

	return res;
}


/* file operations for iap2 device /dev/iap2 */
static struct file_operations apple_mux_fops = {
	.owner = THIS_MODULE,
	.read = apple_mux_read,
	.write = apple_mux_write,
	.unlocked_ioctl = apple_mux_ioctl,
	.open = apple_mux_open,
	.poll = apple_mux_poll,
	.release = apple_mux_release,
};

static int apple_mux_setup(struct f_apple_mux_sim *dev, int intc, int intf)
{
	int ret = -1;

	if (dev == NULL)
		return -ENOMEM;

	spin_lock_init(&dev->lock);

	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->write_wq);

	atomic_set(&dev->open_excl, 0);
	atomic_set(&dev->read_excl, 0);
	atomic_set(&dev->write_excl, 0);

	INIT_LIST_HEAD(&dev->tx_idle);
	INIT_LIST_HEAD(&dev->rx_idle);
	INIT_LIST_HEAD(&dev->rx_used);

	mutex_lock(&apple_mux_mutex);
	list_add_tail(&dev->node, &apple_mux_list);
	mutex_unlock(&apple_mux_mutex);

	dev->name = (char *)kzalloc(32, GFP_KERNEL);
	if (dev->name == NULL) {
		ret = -ENOMEM;
		goto err;
	}
	sprintf(dev->name, "apple_mux_c%d_i%d", intc, intf);

	dev->device.name = dev->name;
	dev->device.parent = &dev->cdev->gadget->dev;
	dev->device.minor = MISC_DYNAMIC_MINOR;
	dev->device.fops = &apple_mux_fops;

	ret = misc_register(&dev->device);
	if (ret)
		goto err;
	return ret;

err:
	if (dev->name)
		kfree(dev->name);
	printk(KERN_ERR "iap2 gadget driver failed to initialize\n");
	return ret;
}

static void apple_mux_cleanup(struct f_apple_mux_sim *dev)
{
	if (dev->name) {
		kfree(dev->name);
		dev->name = NULL;
	}
	misc_deregister(&dev->device);
	mutex_lock(&apple_mux_mutex);
	list_del(&dev->node);
	mutex_unlock(&apple_mux_mutex);
}


static int apple_mux_sim_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_apple_mux_sim	*dev = func_to_apple_mux_sim(f);
	int							id;
	struct usb_string	*us;

	/* maybe allocate device-global string IDs, and patch descriptors */
	us = usb_gstrings_attach(c->cdev, apple_mux_sim_strings,
				 ARRAY_SIZE(apple_mux_sim_string_defs));
	if (IS_ERR(us))
		return PTR_ERR(us);

	apple_mux_sim_intf.iInterface = us[FUNC_MUX_IDX].id;

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	apple_mux_sim_intf.bInterfaceNumber = id;

printk("##%s:%d\n", __func__, __LINE__);
	//pctx->ep_in = usb_ep_autoconfig(cdev->gadget, &fs_apple_mux_sim_source_desc);
	//pctx->ep_out = usb_ep_autoconfig(cdev->gadget, &fs_apple_mux_sim_sink_desc);

	fs_apple_mux_sim_source_desc.bEndpointAddress = 0x85;
	fs_apple_mux_sim_sink_desc.bEndpointAddress = 0x04;

	dev->cdev = c->cdev;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		hs_apple_mux_sim_source_desc.bEndpointAddress = fs_apple_mux_sim_source_desc.bEndpointAddress;
		hs_apple_mux_sim_sink_desc.bEndpointAddress = fs_apple_mux_sim_sink_desc.bEndpointAddress;
		f->hs_descriptors = hs_apple_mux_sim_descs;
	}

	/* support super speed hardware */
	if (gadget_is_superspeed(c->cdev->gadget)) {
		ss_apple_mux_sim_source_desc.bEndpointAddress =
				fs_apple_mux_sim_source_desc.bEndpointAddress;
		ss_apple_mux_sim_sink_desc.bEndpointAddress =
				fs_apple_mux_sim_sink_desc.bEndpointAddress;
		f->ss_descriptors = ss_apple_mux_sim_descs;
	}

	dev->cdev = cdev;

	if (apple_mux_setup(dev, c->bConfigurationValue, id)) {
		return -1;
	}

	return 0;
}

static void apple_mux_sim_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_apple_mux_sim *dev = func_to_apple_mux_sim(f);
	struct usb_request *req = NULL;
	(void)c;
printk("%s:%d\n", __func__, __LINE__);
	dev->online = 0;
	dev->error = 1;

	wake_up(&dev->read_wq);
	wake_up(&dev->write_wq);

	while ((req = apple_mux_req_get(dev, &dev->rx_idle)))
		apple_mux_request_free(req, dev->ep_out);
	req = NULL;
	while ((req = apple_mux_req_get(dev, &dev->rx_used)))
		apple_mux_request_free(req, dev->ep_out);
	req = NULL;
	while ((req = apple_mux_req_get(dev, &dev->tx_idle)))
		apple_mux_request_free(req, dev->ep_in);
	dev->cur_read_pos = 0;

	apple_mux_cleanup(dev);
}

static int apple_mux_sim_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{printk("%s:%d\n", __func__, __LINE__);
	(void)f;
	(void)intf;
	(void)alt;
	return 0;
}

static void apple_mux_sim_disable(struct usb_function *f)
{
	(void)f;
}

static int apple_mux_sim_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
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

static void apple_mux_sim_free_func(struct usb_function *f)
{
	struct f_apple_mux_sim *dev;
	struct f_apple_mux_sim_opts *opts;
printk("%s:%d\n", __func__, __LINE__);
	dev = func_to_apple_mux_sim(f);
	opts = container_of(f->fi, struct f_apple_mux_sim_opts, func_inst);
	//apple_mux_cleanup(dev);
	kfree(dev);
	mutex_lock(&opts->lock);
	opts->refcnt--;
	mutex_unlock(&opts->lock);
}

static inline struct f_apple_mux_sim_opts *to_f_apple_mux_sim_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_apple_mux_sim_opts,
			    func_inst.group);
}

static ssize_t f_apple_mux_sim_opts_iso_qlen_show(struct config_item *item, char *page)
{
	struct f_apple_mux_sim_opts *opts = to_f_apple_mux_sim_opts(item);
	int result;

	mutex_lock(&opts->lock);
	result = sprintf(page, "%u\n", opts->iso_qlen);
	mutex_unlock(&opts->lock);

	return result;
}

static ssize_t f_apple_mux_sim_opts_iso_qlen_store(struct config_item *item,
					   const char *page, size_t len)
{
	struct f_apple_mux_sim_opts *opts = to_f_apple_mux_sim_opts(item);
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

static void apple_mux_sim_attr_release(struct config_item *item)
{
	struct f_apple_mux_sim_opts *ss_opts = to_f_apple_mux_sim_opts(item);

	usb_put_function_instance(&ss_opts->func_inst);
}

static struct configfs_item_operations apple_mux_sim_item_ops = {
	.release		= apple_mux_sim_attr_release,
};


CONFIGFS_ATTR(f_apple_mux_sim_opts_, iso_qlen);

static struct configfs_attribute *apple_mux_sim_attrs[] = {
	&f_apple_mux_sim_opts_attr_iso_qlen,
	NULL,
};

static struct config_item_type apple_mux_sim_func_type = {
	.ct_item_ops    = &apple_mux_sim_item_ops,
	.ct_attrs	= apple_mux_sim_attrs,
	.ct_owner       = THIS_MODULE,
};


static void apple_mux_sim_free_inst(struct usb_function_instance *f)
{
	struct f_apple_mux_sim_opts *opts;

	opts = container_of(f, struct f_apple_mux_sim_opts, func_inst);
printk("%s:%d\n", __func__, __LINE__);
	kfree(opts);
}

static struct usb_function_instance *apple_mux_sim_alloc_inst(void)
{
	struct f_apple_mux_sim_opts *opts = NULL;
printk("%s:%d\n", __func__, __LINE__);
	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);
	mutex_init(&opts->lock);
	opts->func_inst.free_func_inst = apple_mux_sim_free_inst;

	config_group_init_type_name(&opts->func_inst.group, "", &apple_mux_sim_func_type);
printk("%s:%d\n", __func__, __LINE__);
	return &opts->func_inst;
}

static struct usb_function *apple_mux_sim_alloc(struct usb_function_instance *fi)
{
	struct f_apple_mux_sim	*pctx = NULL;
	struct f_apple_mux_sim_opts *opts = NULL;
	int ret = -1;
printk("%s:%d\n", __func__, __LINE__);
	pctx = kzalloc(sizeof *pctx, GFP_KERNEL);
	if (!pctx)
		return NULL;

	opts =  container_of(fi, struct f_apple_mux_sim_opts, func_inst);

	mutex_lock(&opts->lock);
	opts->refcnt++;
	mutex_unlock(&opts->lock);

	pctx->function.name = "multiplexor";
	pctx->function.fs_descriptors = fs_apple_mux_sim_descs;
	pctx->function.bind = apple_mux_sim_bind;
	pctx->function.unbind = apple_mux_sim_unbind;
	pctx->function.set_alt = apple_mux_sim_set_alt;
	pctx->function.disable = apple_mux_sim_disable;
	pctx->function.setup = apple_mux_sim_setup;
	pctx->function.free_func = apple_mux_sim_free_func;

	/*ret = apple_mux_setup(pctx);
	if (ret) {
		kfree(pctx);
		return NULL;
	}*/
printk("%s:%d\n", __func__, __LINE__);
	return &pctx->function;
}


DECLARE_USB_FUNCTION_INIT(mux, apple_mux_sim_alloc_inst, apple_mux_sim_alloc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("art");

