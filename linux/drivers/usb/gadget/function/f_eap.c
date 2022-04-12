/*
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/usb/composite.h>
#include <linux/usb/functionfs.h>
#include "u_f.h"

#define EAP_BULK_BUFFER_SIZE           16384

#define TX_REQ_MAX 4
#define NETLINK_USB_DEV_READY  27
#define DRIVER_NAME "eap"
#define MAX_INST_NAME_LEN          40
static const char eap_shortname[] = "eap";

struct eap_dev {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;

	struct usb_ep *ep_in;
	struct usb_ep *ep_out;

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

	struct sock *netlink_handle;
};

static struct usb_interface_assoc_descriptor eap_iad_desc = {
	.bLength =		sizeof eap_iad_desc,
	.bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
	.bInterfaceCount = 1,
	.bFunctionClass = 0xFF,
	.bFunctionSubClass = 0xF0,
	.bFunctionProtocol = 0x1,
};

static struct usb_interface_descriptor eap_interface_desc0 = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 1,
	.bAlternateSetting	= 0,
	.bNumEndpoints          = 0,
	.bInterfaceClass        = 0xFF,
	.bInterfaceSubClass     = 0xF0,
	.bInterfaceProtocol     = 0x1,
};

static struct usb_interface_descriptor eap_interface_desc1 = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 1,
	.bAlternateSetting	= 1,
	.bNumEndpoints          = 2,
	.bInterfaceClass        = 0xFF,
	.bInterfaceSubClass     = 0xF0,
	.bInterfaceProtocol     = 0x1,
};

static struct usb_endpoint_descriptor eap_superspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(1024),
};

static struct usb_endpoint_descriptor eap_superspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor eap_superspeed_bulk_comp_desc = {
	.bLength =              sizeof eap_superspeed_bulk_comp_desc,
	.bDescriptorType =      USB_DT_SS_ENDPOINT_COMP,

	/* the following 2 values can be tweaked if necessary */
	/* .bMaxBurst =         0, */
	/* .bmAttributes =      0, */
};

static struct usb_endpoint_descriptor eap_highspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor eap_highspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor eap_fullspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor eap_fullspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *fs_eap_descs[] = {
	//(struct usb_descriptor_header *) &eap_iad_desc,
	(struct usb_descriptor_header *) &eap_interface_desc0,
	(struct usb_descriptor_header *) &eap_interface_desc1,
	(struct usb_descriptor_header *) &eap_fullspeed_in_desc,
	(struct usb_descriptor_header *) &eap_fullspeed_out_desc,
	NULL,
};

static struct usb_descriptor_header *hs_eap_descs[] = {
	//(struct usb_descriptor_header *) &eap_iad_desc,
	(struct usb_descriptor_header *) &eap_interface_desc0,
	(struct usb_descriptor_header *) &eap_interface_desc1,
	(struct usb_descriptor_header *) &eap_highspeed_in_desc,
	(struct usb_descriptor_header *) &eap_highspeed_out_desc,
	NULL,
};

static struct usb_descriptor_header *ss_eap_descs[] = {
	//(struct usb_descriptor_header *) &eap_iad_desc,
	(struct usb_descriptor_header *) &eap_interface_desc0,
	(struct usb_descriptor_header *) &eap_interface_desc1,
	(struct usb_descriptor_header *) &eap_superspeed_in_desc,
	(struct usb_descriptor_header *) &eap_superspeed_bulk_comp_desc,
	(struct usb_descriptor_header *) &eap_superspeed_out_desc,
	(struct usb_descriptor_header *) &eap_superspeed_bulk_comp_desc,
	NULL,
};

#define STRING_CTRL_IDX	0

static struct usb_string eap_string_defs[] = {
	[STRING_CTRL_IDX].s = "com.baidu.CarLifeVehicleProtocol",
	//[STRING_CTRL_IDX1].s = "com.baidu.CarLifeVehicleProtocol",
	{  } /* end of list */
};

static struct usb_gadget_strings eap_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		eap_string_defs,
};

static struct usb_gadget_strings *eap_strings[] = {
	&eap_string_table,
	NULL,
};


static struct eap_dev *_eap_dev;


static int usb_dev_ready_notify(struct eap_dev *dev, int is_ready)
{
	struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;
	char *buf_on = "connected";
	char *buf_off = "disconnected";
	char *buf = NULL;
	int len = 0;
    int ret;

	if (!dev || !dev->netlink_handle)
		return -1;

	if (is_ready) {
		buf = buf_on;printk("%s:%d %s\n", __func__, __LINE__, buf_on);
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

	ret = netlink_unicast(dev->netlink_handle, nl_skb, 53, MSG_DONTWAIT);

    return ret;
}

static inline struct eap_dev *func_to_eap(struct usb_function *f)
{
	return container_of(f, struct eap_dev, function);
}


static struct usb_request *eap_request_new(struct usb_ep *ep, int buffer_size)
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

static void eap_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

static inline int eap_lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1) {
		return 0;
	} else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void eap_unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

void eap_req_put(struct eap_dev *dev, struct list_head *head,
		struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* remove a request from the head of a list */
struct usb_request *eap_req_get(struct eap_dev *dev, struct list_head *head)
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

static void eap_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct eap_dev *dev = (struct eap_dev *)(req->context);

	if (req->status != 0){//printk("+++%s:%d+++req->status=%d\n", __func__, __LINE__, req->status);
		dev->error = 1;
	}
	eap_req_put(dev, &dev->tx_idle, req);

	wake_up(&dev->write_wq);
}

static void eap_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct eap_dev *dev = (struct eap_dev *)(req->context);
	//unsigned long flags;

	dev->rx_done = 1;
	if (req) {
		if (req->status != 0){//printk("+++%s:%d+++req->status=%d\n", __func__, __LINE__, req->status);
			eap_req_put(dev, &dev->rx_idle, req);
			dev->error = 1;
		} else {
			eap_req_put(dev, &dev->rx_used, req);
		}
	}
	wake_up(&dev->read_wq);
}

static int eap_create_bulk_endpoints(struct eap_dev *dev,
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
	DBG(cdev, "usb_ep_autoconfig for EAP ep_out got %s\n", ep->name);
	ep->driver_data = dev;
	ep->desc = out_desc;
	dev->ep_out = ep;

	for (i = 0; i < TX_REQ_MAX; i++) {
		req = eap_request_new(dev->ep_out, EAP_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = eap_complete_out;
		req->context = (void *)dev;
		eap_req_put(dev, &dev->rx_idle, req);
	}

	for (i = 0; i < TX_REQ_MAX; i++) {
		req = eap_request_new(dev->ep_in, EAP_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = eap_complete_in;
		req->context = (void *)dev;
		eap_req_put(dev, &dev->tx_idle, req);
	}

	return 0;

fail:
	printk(KERN_ERR "eap_bind() could not allocate requests\n");
	return -1;
}

static int eap_rx_submit(struct eap_dev *dev, struct usb_request *req)
{
	int ret, r;

	dev->rx_done = 0;
	ret = usb_ep_queue(dev->ep_out, req, GFP_ATOMIC);
	if (ret < 0) {
		pr_debug("eap_read: failed to queue req %p (%d)\n", req, ret);
		r = -EIO;
		dev->error = 1;
		goto done;
	} else {
		pr_debug("rx %p queue\n", req);
	}
done:
	pr_debug("eap_read returning %d\n", r);
	return r;
}

static ssize_t eap_read(struct file *fp, char __user *buf,
				size_t count, loff_t *pos)
{
	struct eap_dev *dev = fp->private_data;
	struct usb_request *req = NULL;
	int r = 0, xfer, cur_actual = 0;
	int ret;
	//static int cur_pos = 0;
	unsigned long flags;

	pr_debug("eap_read(%d) n", count);
	if (!dev)
		return -ENODEV;

	//if (count > EAP_BULK_BUFFER_SIZE)
	//	return -EINVAL;
	if (count <= 0)
		return 0;

	if (eap_lock(&dev->read_excl))
		return -EBUSY;

	/*while (!(dev->online || dev->error)) {
		printk("eap_read: waiting for online state\n");
		ret = wait_event_interruptible(dev->read_wq,
				(dev->online || dev->error));
		if (ret < 0) {
			eap_unlock(&dev->read_excl);
			return ret;
		}
	}*/
	if (dev->error || dev->online == 0) {
		r = -EIO;
		goto done;
	}

	req = NULL;
	ret = wait_event_interruptible(dev->read_wq, (req = eap_req_get(dev, &dev->rx_used)) != NULL);
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
			eap_rx_submit(dev, req);//the req is read completely, return to the usb core
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
	eap_unlock(&dev->read_excl);
	pr_debug("eap_read returning %d\n", r);
	return r;
}

static ssize_t eap_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	struct eap_dev *dev = fp->private_data;
	struct usb_request *req = 0;
	int r = count, xfer;
	int ret;

	if (!dev)
		return -ENODEV;
	pr_debug("eap_write(%d)\n", count);

	if (eap_lock(&dev->write_excl))
		return -EBUSY;

	while (count > 0) {
		if (dev->error) {
			pr_debug("eap_write dev->error\n");
			r = -EIO;
			break;
		}
		if (dev->online == 0) {
			pr_debug("eap_write dev->online == 0\n");
			r = -ENODEV;
			break;
		}

		req = 0;
		ret = wait_event_interruptible_timeout(dev->write_wq,
			((req = eap_req_get(dev, &dev->tx_idle)) || (dev->error) || (dev->online == 0)), msecs_to_jiffies(1000));
		if (ret <= 0) {
			r = ret;
			break;
		}

		if (req != 0) {
			if (count > EAP_BULK_BUFFER_SIZE)
				xfer = EAP_BULK_BUFFER_SIZE;
			else
				xfer = count;
			if (copy_from_user(req->buf, buf, xfer)) {
				r = -EFAULT;
				break;
			}

			req->length = xfer;
			ret = usb_ep_queue(dev->ep_in, req, GFP_ATOMIC);
			if (ret < 0) {
				pr_debug("eap_write: xfer error %d\n", ret);
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
		eap_req_put(dev, &dev->tx_idle, req);

//done:
	eap_unlock(&dev->write_excl);
	pr_debug("eap_write returning %d\n", r);
	return r;
}

static int eap_open(struct inode *ip, struct file *fp)
{
	if (!_eap_dev)
		return -ENODEV;

	if (eap_lock(&_eap_dev->open_excl))
		return -EBUSY;

	fp->private_data = _eap_dev;
	_eap_dev->error = 0;
	_eap_dev->rx_done = 0;
	//printk(KERN_INFO "eap_open\n", _eap_dev->online);

	return 0;
}

static int eap_release(struct inode *ip, struct file *fp)
{
	//printk(KERN_INFO "eap_release\n");
	eap_unlock(&_eap_dev->open_excl);
	return 0;
}

static unsigned int eap_poll(struct file *fp, struct poll_table_struct *wait)
{
	struct eap_dev *dev = fp->private_data;
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

#define EAP_IOCTL_BASE    0xb7
#define EAP_GET_ONLINE_STATE  _IOW(EAP_IOCTL_BASE, 0, unsigned long)

static long eap_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	struct eap_dev *dev = fp->private_data;
	long res = 0;
	int online = 0;

	switch(cmd) {
	case EAP_GET_ONLINE_STATE:
		//printk("dev->online = %d\n", dev->online);
		online = dev->online;
		 if (copy_to_user((void*)arg, &online, sizeof(online))) {
			return -EFAULT;
		 }
		break;
	}

	return res;
}


/* file operations for EAP device /dev/EAP */
static struct file_operations eap_fops = {
	.owner = THIS_MODULE,
	.read = eap_read,
	.write = eap_write,
	.unlocked_ioctl = eap_ioctl,
	.open = eap_open,
	.poll = eap_poll,
	.release = eap_release,
};

static struct miscdevice eap_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = eap_shortname,
	.fops = &eap_fops,
};

static int
eap_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct eap_dev	*dev = func_to_eap(f);
	int			id;
	int			ret, status;

	dev->cdev = cdev;
	DBG(cdev, "eap_function_bind dev: %p\n", dev);

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	eap_interface_desc0.bInterfaceNumber = id;
	eap_interface_desc1.bInterfaceNumber = id;
	
	status = usb_string_id(cdev);
	if (status < 0)
		return status;
	eap_string_defs[0].id = status;
	eap_interface_desc0.bInterfaceNumber = id;
	eap_interface_desc1.bInterfaceNumber = id;
	dev->function.fs_descriptors = usb_copy_descriptors(fs_eap_descs);

	/* allocate endpoints */
	ret = eap_create_bulk_endpoints(dev, &eap_fullspeed_in_desc,
			&eap_fullspeed_out_desc);
	if (ret)
		return ret;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		eap_highspeed_in_desc.bEndpointAddress =
			eap_fullspeed_in_desc.bEndpointAddress;
		eap_highspeed_out_desc.bEndpointAddress =
			eap_fullspeed_out_desc.bEndpointAddress;
		dev->function.fs_descriptors = usb_copy_descriptors(hs_eap_descs);
	}

	/* support super speed hardware */
	if (gadget_is_superspeed(c->cdev->gadget)) {
		eap_superspeed_in_desc.bEndpointAddress =
			eap_fullspeed_in_desc.bEndpointAddress;
		eap_superspeed_out_desc.bEndpointAddress =
			eap_fullspeed_out_desc.bEndpointAddress;
		dev->function.fs_descriptors = usb_copy_descriptors(ss_eap_descs);
	}

	DBG(cdev, "%s speed %s: IN/%s, OUT/%s\n",
			gadget_is_superspeed(c->cdev->gadget) ? "super" :
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			f->name, dev->ep_in->name, dev->ep_out->name);
	return 0;
}

static void
eap_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct eap_dev	*dev = func_to_eap(f);
	struct usb_request *req = NULL;

	dev->online = 0;
	dev->error = 1;

	wake_up(&dev->read_wq);
	wake_up(&dev->write_wq);

	while ((req = eap_req_get(dev, &dev->rx_idle)))
		eap_request_free(req, dev->ep_out);
	req = NULL;
	while ((req = eap_req_get(dev, &dev->rx_used)))
		eap_request_free(req, dev->ep_out);
	req = NULL;
	while ((req = eap_req_get(dev, &dev->tx_idle)))
		eap_request_free(req, dev->ep_in);
	dev->cur_read_pos = 0;
}

static int eap_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct eap_dev	*dev = func_to_eap(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;
	struct usb_request *req = NULL;

	if (config_ep_by_speed(cdev->gadget, f, dev->ep_in) ||
		config_ep_by_speed(cdev->gadget, f, dev->ep_out)) {
		dev->ep_in->desc = NULL;
		dev->ep_out->desc = NULL;
		return -1;
	}

	DBG(cdev, "eap_function_set_alt intf: %d alt: %d\n", intf, alt);
	ret = usb_ep_enable(dev->ep_in);
	if (ret)
		return ret;
	ret = usb_ep_enable(dev->ep_out);
	if (ret) {
		usb_ep_disable(dev->ep_in);
		return ret;
	}
	dev->online = 1;
	usb_dev_ready_notify(dev, 1);
	dev->error = 0;

	while ((req = eap_req_get(dev, &dev->rx_used))) {
		eap_req_put(dev, &dev->rx_idle, req);
	}
	while ((req = eap_req_get(dev, &dev->rx_idle))) {
		req->length = EAP_BULK_BUFFER_SIZE;
		eap_rx_submit(dev, req);
	}
	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);
	wake_up(&dev->write_wq);
	return 0;
}

static void eap_function_disable(struct usb_function *f)
{
	struct eap_dev	*dev = func_to_eap(f);
	struct usb_composite_dev	*cdev = dev->cdev;

	printk("eap_function_disable cdev %p %d\n", cdev, __LINE__);
	usb_dev_ready_notify(dev, 0);
	dev->online = 0;
	dev->error = 1;
	usb_ep_disable(dev->ep_in);
	usb_ep_disable(dev->ep_out);

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);
	wake_up(&dev->write_wq);

	VDBG(cdev, "%s disabled\n", dev->function.name);
}

static void eap_function_suspend(struct usb_function *f)
{
	struct eap_dev	*dev = func_to_eap(f);
	struct usb_composite_dev	*cdev = dev->cdev;

	printk( "eap_function_suspend cdev %p\n", cdev);//DBG(cdev,
	usb_dev_ready_notify(dev, 0);
	dev->online = 0;
	dev->error = 0;
	usb_ep_disable(dev->ep_in);
	usb_ep_disable(dev->ep_out);

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);
	wake_up(&dev->write_wq);

	VDBG(cdev, "%s suspend\n", dev->function.name);
}
#if 0
static int eap_bind_config(struct usb_configuration *c)
{
	struct eap_dev *dev = _eap_dev;
	int status;

	if (eap_string_defs[0].id == 0) {
		/* control interface label */
		status = usb_string_id(c->cdev);
		if (status < 0)
			return status;
		eap_string_defs[STRING_CTRL_IDX].id = status;
		eap_interface_desc.iInterface = status;
	}

	dev->cdev = c->cdev;
	dev->function.name = "eap";
	dev->function.strings = eap_strings;
	dev->function.descriptors = fs_eap_descs;
	dev->function.hs_descriptors = hs_eap_descs;
	dev->function.ss_descriptors = ss_eap_descs;
	dev->function.bind = eap_function_bind;
	dev->function.unbind = eap_function_unbind;
	dev->function.set_alt = eap_function_set_alt;
	dev->function.disable = eap_function_disable;
	dev->function.suspend = eap_function_suspend;

	return usb_add_function(c, &dev->function);
}
#endif

static struct netlink_kernel_cfg cfg = {
	.groups	= 1,
	.flags	= NL_CFG_F_NONROOT_RECV,
};

static int eap_setup(struct eap_dev *dev)
{
	int ret = -1;

	if (!dev)
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

	dev->netlink_handle = netlink_kernel_create(&init_net, NETLINK_USB_DEV_READY, &cfg);
    if(!dev->netlink_handle) {
        printk(KERN_ERR "can not create a usb dev ready netlink socket!\n");
        goto err;
    }

	ret = misc_register(&eap_device);
	if (ret)
		goto err;

	_eap_dev = dev;

	return ret;

err:
	kfree(dev);
	printk(KERN_ERR "eap gadget driver failed to initialize\n");
	return ret;
}

static void eap_cleanup(struct eap_dev *dev)
{
	(void)dev;
	
	misc_deregister(&eap_device);

	if (NULL == _eap_dev)
		return;

	if(_eap_dev->netlink_handle) {
		netlink_kernel_release(_eap_dev->netlink_handle);
	}

	kfree(_eap_dev);
	_eap_dev = NULL;
}

static void eap_free(struct usb_function *f)
{
	(void)f;
}

struct f_eap_opts {
	struct usb_function_instance func_inst;
	struct eap_dev *dev;
	char *name;
	int refcnt;
	struct mutex			lock;
};

static struct f_eap_opts *to_eap_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_eap_opts,
		func_inst.group);
}

static void eap_attr_release(struct config_item *item)
{
	struct f_eap_opts *opts = to_eap_opts(item);

	usb_put_function_instance(&opts->func_inst);
}

static struct configfs_item_operations eap_item_ops = {
	.release        = eap_attr_release,
};

static struct config_item_type eap_func_type = {
	.ct_item_ops    = &eap_item_ops,
	.ct_owner       = THIS_MODULE,
};

static void eap_free_inst(struct usb_function_instance *fi)
{
	struct f_eap_opts *opts;

	opts = container_of(fi, struct f_eap_opts, func_inst);
	if (NULL != opts->name)
		kfree(opts->name);
	eap_cleanup(opts->dev);

	kfree(opts);
}

static struct usb_function_instance *eap_alloc_inst(void)
{
	struct f_eap_opts *opts = NULL;

	opts = kzalloc(sizeof(struct f_eap_opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);
	//opts->func_inst.set_inst_name = eap_set_inst_name;
	opts->func_inst.free_func_inst = eap_free_inst;

	mutex_init(&opts->lock);

	config_group_init_type_name(&opts->func_inst.group,
					"", &eap_func_type);

	return  &opts->func_inst;
}

struct usb_function *eap_alloc(struct usb_function_instance *fi)
{
	struct f_eap_opts *opts = container_of(fi, struct f_eap_opts, func_inst);
	struct eap_dev *dev;
	int ret = -1;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return ERR_PTR(-ENOMEM);

	ret = eap_setup(dev);
	if (ret) {
		kfree(dev);
		pr_err("Error setting EAP\n");
		return ERR_PTR(ret);
	}

	opts = container_of(fi, struct f_eap_opts, func_inst);
	mutex_lock(&opts->lock);
	opts->dev = dev;
	opts->refcnt++;
	mutex_unlock(&opts->lock);

	dev->function.name = DRIVER_NAME;
	dev->function.strings = eap_strings;

	dev->function.bind = eap_function_bind;
	dev->function.unbind = eap_function_unbind;
	dev->function.set_alt = eap_function_set_alt;
	dev->function.disable = eap_function_disable;
	dev->function.suspend = eap_function_suspend;
	dev->function.free_func = eap_free;

	return &dev->function;
}

DECLARE_USB_FUNCTION_INIT(eap, eap_alloc_inst, eap_alloc);
MODULE_LICENSE("GPL");
