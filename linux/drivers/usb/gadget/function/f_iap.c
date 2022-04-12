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

#define iAP2_BULK_BUFFER_SIZE           16384

#define TX_REQ_MAX 4
#define NETLINK_USB_DEV_READY  25
#define DRIVER_NAME "iap"
#define MAX_INST_NAME_LEN          40
static const char iap2_shortname[] = "iap";

struct iap2_dev {
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

static struct usb_interface_descriptor iap2_interface_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bNumEndpoints          = 2,
	.bInterfaceClass        = 0xFF,
	.bInterfaceSubClass     = 0xF0,
	.bInterfaceProtocol     = 0,
};

static struct usb_endpoint_descriptor iap2_superspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(1024),
};

static struct usb_endpoint_descriptor iap2_superspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor iap2_superspeed_bulk_comp_desc = {
	.bLength =              sizeof iap2_superspeed_bulk_comp_desc,
	.bDescriptorType =      USB_DT_SS_ENDPOINT_COMP,

	/* the following 2 values can be tweaked if necessary */
	/* .bMaxBurst =         0, */
	/* .bmAttributes =      0, */
};

static struct usb_endpoint_descriptor iap2_highspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor iap2_highspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor iap2_fullspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor iap2_fullspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *fs_iap2_descs[] = {
	(struct usb_descriptor_header *) &iap2_interface_desc,
	(struct usb_descriptor_header *) &iap2_fullspeed_in_desc,
	(struct usb_descriptor_header *) &iap2_fullspeed_out_desc,
	NULL,
};

static struct usb_descriptor_header *hs_iap2_descs[] = {
	(struct usb_descriptor_header *) &iap2_interface_desc,
	(struct usb_descriptor_header *) &iap2_highspeed_in_desc,
	(struct usb_descriptor_header *) &iap2_highspeed_out_desc,
	NULL,
};

static struct usb_descriptor_header *ss_iap2_descs[] = {
	(struct usb_descriptor_header *) &iap2_interface_desc,
	(struct usb_descriptor_header *) &iap2_superspeed_in_desc,
	(struct usb_descriptor_header *) &iap2_superspeed_bulk_comp_desc,
	(struct usb_descriptor_header *) &iap2_superspeed_out_desc,
	(struct usb_descriptor_header *) &iap2_superspeed_bulk_comp_desc,
	NULL,
};

#define STRING_CTRL_IDX	0

static struct usb_string iap_string_defs[] = {
	[STRING_CTRL_IDX].s = "iAP Interface",
	{  } /* end of list */
};

static struct usb_gadget_strings iap_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		iap_string_defs,
};

static struct usb_gadget_strings *iap_strings[] = {
	&iap_string_table,
	NULL,
};


static struct iap2_dev *_iap2_dev;


static int usb_dev_ready_notify(struct iap2_dev *dev, int is_ready)
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

	ret = netlink_unicast(dev->netlink_handle, nl_skb, 52, MSG_DONTWAIT);

    return ret;
}

static inline struct iap2_dev *func_to_iap2(struct usb_function *f)
{
	return container_of(f, struct iap2_dev, function);
}


static struct usb_request *iap2_request_new(struct usb_ep *ep, int buffer_size)
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

static void iap2_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

static inline int iap2_lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1) {
		return 0;
	} else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void iap2_unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

void iap2_req_put(struct iap2_dev *dev, struct list_head *head,
		struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* remove a request from the head of a list */
struct usb_request *iap2_req_get(struct iap2_dev *dev, struct list_head *head)
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

static void iap2_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct iap2_dev *dev = (struct iap2_dev *)(req->context);

	if (req->status != 0){//printk("+++%s:%d+++req->status=%d\n", __func__, __LINE__, req->status);
		dev->error = 1;
	}
	iap2_req_put(dev, &dev->tx_idle, req);

	wake_up(&dev->write_wq);
}

static void iap2_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct iap2_dev *dev = (struct iap2_dev *)(req->context);
	//unsigned long flags;

	dev->rx_done = 1;
	if (req) {
		if (req->status != 0){//printk("+++%s:%d+++req->status=%d\n", __func__, __LINE__, req->status);
			iap2_req_put(dev, &dev->rx_idle, req);
			dev->error = 1;
		} else {
			iap2_req_put(dev, &dev->rx_used, req);
		}
	}
	wake_up(&dev->read_wq);
}

static int iap2_create_bulk_endpoints(struct iap2_dev *dev,
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

	for (i = 0; i < TX_REQ_MAX; i++) {
		req = iap2_request_new(dev->ep_out, iAP2_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = iap2_complete_out;
		req->context = (void *)dev;
		iap2_req_put(dev, &dev->rx_idle, req);
	}

	for (i = 0; i < TX_REQ_MAX; i++) {
		req = iap2_request_new(dev->ep_in, iAP2_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = iap2_complete_in;
		req->context = (void *)dev;
		iap2_req_put(dev, &dev->tx_idle, req);
	}

	return 0;

fail:
	printk(KERN_ERR "iap2_bind() could not allocate requests\n");
	return -1;
}

static int iap2_rx_submit(struct iap2_dev *dev, struct usb_request *req)
{
	int ret, r;

	dev->rx_done = 0;
	ret = usb_ep_queue(dev->ep_out, req, GFP_ATOMIC);
	if (ret < 0) {
		pr_debug("iap2_read: failed to queue req %p (%d)\n", req, ret);
		r = -EIO;
		dev->error = 1;
		goto done;
	} else {
		pr_debug("rx %p queue\n", req);
	}
done:
	pr_debug("iap2_read returning %d\n", r);
	return r;
}

static ssize_t iap2_read(struct file *fp, char __user *buf,
				size_t count, loff_t *pos)
{
	struct iap2_dev *dev = fp->private_data;
	struct usb_request *req = NULL;
	int r = 0, xfer, cur_actual = 0;
	int ret;
	//static int cur_pos = 0;
	unsigned long flags;

	pr_debug("iap2_read(%d) n", count);
	if (!dev)
		return -ENODEV;

	//if (count > iAP2_BULK_BUFFER_SIZE)
	//	return -EINVAL;
	if (count <= 0)
		return 0;

	if (iap2_lock(&dev->read_excl))
		return -EBUSY;

	/*while (!(dev->online || dev->error)) {
		printk("iap2_read: waiting for online state\n");
		ret = wait_event_interruptible(dev->read_wq,
				(dev->online || dev->error));
		if (ret < 0) {
			iap2_unlock(&dev->read_excl);
			return ret;
		}
	}*/
	if (dev->error || dev->online == 0) {
		r = -EIO;
		goto done;
	}

	req = NULL;
	ret = wait_event_interruptible(dev->read_wq, (req = iap2_req_get(dev, &dev->rx_used)) != NULL);
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
			iap2_rx_submit(dev, req);//the req is read completely, return to the usb core
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
	iap2_unlock(&dev->read_excl);
	pr_debug("iap2_read returning %d\n", r);
	return r;
}

static ssize_t iap2_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	struct iap2_dev *dev = fp->private_data;
	struct usb_request *req = 0;
	int r = count, xfer;
	int ret;

	if (!dev)
		return -ENODEV;
	pr_debug("iap2_write(%d)\n", count);

	if (iap2_lock(&dev->write_excl))
		return -EBUSY;

	while (count > 0) {
		if (dev->error) {
			pr_debug("iap2_write dev->error\n");
			r = -EIO;
			break;
		}
		if (dev->online == 0) {
			pr_debug("iap2_write dev->online == 0\n");
			r = -ENODEV;
			break;
		}

		req = 0;
		ret = wait_event_interruptible_timeout(dev->write_wq,
			((req = iap2_req_get(dev, &dev->tx_idle)) || (dev->error) || (dev->online == 0)), msecs_to_jiffies(1000));
		if (ret <= 0) {
			r = ret;
			break;
		}

		if (req != 0) {
			if (count > iAP2_BULK_BUFFER_SIZE)
				xfer = iAP2_BULK_BUFFER_SIZE;
			else
				xfer = count;
			if (copy_from_user(req->buf, buf, xfer)) {
				r = -EFAULT;
				break;
			}

			req->length = xfer;
			ret = usb_ep_queue(dev->ep_in, req, GFP_ATOMIC);
			if (ret < 0) {
				pr_debug("iap2_write: xfer error %d\n", ret);
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
		iap2_req_put(dev, &dev->tx_idle, req);

//done:
	iap2_unlock(&dev->write_excl);
	pr_debug("iap2_write returning %d\n", r);
	return r;
}

static int iap2_open(struct inode *ip, struct file *fp)
{
	if (!_iap2_dev)
		return -ENODEV;

	if (iap2_lock(&_iap2_dev->open_excl))
		return -EBUSY;

	fp->private_data = _iap2_dev;
	_iap2_dev->error = 0;
	_iap2_dev->rx_done = 0;
	//printk(KERN_INFO "iap2_open\n", _iap2_dev->online);

	return 0;
}

static int iap2_release(struct inode *ip, struct file *fp)
{
	//printk(KERN_INFO "iap2_release\n");
	iap2_unlock(&_iap2_dev->open_excl);
	return 0;
}

static unsigned int iap2_poll(struct file *fp, struct poll_table_struct *wait)
{
	struct iap2_dev *dev = fp->private_data;
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

#define iAP2_IOCTL_BASE    0xb7
#define iAP2_GET_ONLINE_STATE  _IOW(iAP2_IOCTL_BASE, 0, unsigned long)

static long iap2_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	struct iap2_dev *dev = fp->private_data;
	long res = 0;
	int online = 0;

	switch(cmd) {
	case iAP2_GET_ONLINE_STATE:
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
static struct file_operations iap2_fops = {
	.owner = THIS_MODULE,
	.read = iap2_read,
	.write = iap2_write,
	.unlocked_ioctl = iap2_ioctl,
	.open = iap2_open,
	.poll = iap2_poll,
	.release = iap2_release,
};

static struct miscdevice iap2_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = iap2_shortname,
	.fops = &iap2_fops,
};

static int
iap2_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct iap2_dev	*dev = func_to_iap2(f);
	int			id;
	int			ret, status;

	dev->cdev = cdev;
	DBG(cdev, "iap2_function_bind dev: %p\n", dev);

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	iap2_interface_desc.bInterfaceNumber = id;
	
	status = usb_string_id(cdev);
	if (status < 0)
		return status;
	iap_string_defs[0].id = status;
	iap2_interface_desc.iInterface = status;

	dev->function.fs_descriptors = usb_copy_descriptors(fs_iap2_descs);

	/* allocate endpoints */
	ret = iap2_create_bulk_endpoints(dev, &iap2_fullspeed_in_desc,
			&iap2_fullspeed_out_desc);
	if (ret)
		return ret;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		iap2_highspeed_in_desc.bEndpointAddress =
			iap2_fullspeed_in_desc.bEndpointAddress;
		iap2_highspeed_out_desc.bEndpointAddress =
			iap2_fullspeed_out_desc.bEndpointAddress;
		dev->function.hs_descriptors = usb_copy_descriptors(hs_iap2_descs);
	}

	/* support super speed hardware */
	if (gadget_is_superspeed(c->cdev->gadget)) {
		iap2_superspeed_in_desc.bEndpointAddress =
			iap2_fullspeed_in_desc.bEndpointAddress;
		iap2_superspeed_out_desc.bEndpointAddress =
			iap2_fullspeed_out_desc.bEndpointAddress;
		dev->function.ss_descriptors = usb_copy_descriptors(ss_iap2_descs);
	}

	DBG(cdev, "%s speed %s: IN/%s, OUT/%s\n",
			gadget_is_superspeed(c->cdev->gadget) ? "super" :
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			f->name, dev->ep_in->name, dev->ep_out->name);
	return 0;
}

static void
iap2_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct iap2_dev	*dev = func_to_iap2(f);
	struct usb_request *req = NULL;

	dev->online = 0;
	dev->error = 1;

	wake_up(&dev->read_wq);
	wake_up(&dev->write_wq);

	while ((req = iap2_req_get(dev, &dev->rx_idle)))
		iap2_request_free(req, dev->ep_out);
	req = NULL;
	while ((req = iap2_req_get(dev, &dev->rx_used)))
		iap2_request_free(req, dev->ep_out);
	req = NULL;
	while ((req = iap2_req_get(dev, &dev->tx_idle)))
		iap2_request_free(req, dev->ep_in);
	dev->cur_read_pos = 0;
}

static int iap2_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct iap2_dev	*dev = func_to_iap2(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;
	struct usb_request *req = NULL;

	if (config_ep_by_speed(cdev->gadget, f, dev->ep_in) ||
		config_ep_by_speed(cdev->gadget, f, dev->ep_out)) {
		dev->ep_in->desc = NULL;
		dev->ep_out->desc = NULL;
		return -1;
	}

	DBG(cdev, "iap2_function_set_alt intf: %d alt: %d\n", intf, alt);
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

	while ((req = iap2_req_get(dev, &dev->rx_used))) {
		iap2_req_put(dev, &dev->rx_idle, req);
	}
	while ((req = iap2_req_get(dev, &dev->rx_idle))) {
		req->length = iAP2_BULK_BUFFER_SIZE;
		iap2_rx_submit(dev, req);
	}
	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);
	wake_up(&dev->write_wq);
	return 0;
}

static void iap2_function_disable(struct usb_function *f)
{
	struct iap2_dev	*dev = func_to_iap2(f);
	struct usb_composite_dev	*cdev = dev->cdev;

	printk("iap2_function_disable cdev %p %d\n", cdev, __LINE__);
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

static void iap2_function_suspend(struct usb_function *f)
{
	struct iap2_dev	*dev = func_to_iap2(f);
	struct usb_composite_dev	*cdev = dev->cdev;

	printk( "iap2_function_suspend cdev %p\n", cdev);//DBG(cdev,
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
static int iap2_bind_config(struct usb_configuration *c)
{
	struct iap2_dev *dev = _iap2_dev;
	int status;

	if (iap_string_defs[0].id == 0) {
		/* control interface label */
		status = usb_string_id(c->cdev);
		if (status < 0)
			return status;
		iap_string_defs[STRING_CTRL_IDX].id = status;
		iap2_interface_desc.iInterface = status;
	}

	dev->cdev = c->cdev;
	dev->function.name = "iap2";
	dev->function.strings = iap_strings;
	dev->function.descriptors = fs_iap2_descs;
	dev->function.hs_descriptors = hs_iap2_descs;
	dev->function.ss_descriptors = ss_iap2_descs;
	dev->function.bind = iap2_function_bind;
	dev->function.unbind = iap2_function_unbind;
	dev->function.set_alt = iap2_function_set_alt;
	dev->function.disable = iap2_function_disable;
	dev->function.suspend = iap2_function_suspend;

	return usb_add_function(c, &dev->function);
}
#endif

static struct netlink_kernel_cfg cfg = {
	.groups	= 1,
	.flags	= NL_CFG_F_NONROOT_RECV,
};

static int iap2_setup(struct iap2_dev *dev)
{
	int ret = -1;

	//dev = kzalloc(sizeof(*dev), GFP_KERNEL);
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

	ret = misc_register(&iap2_device);
	if (ret)
		goto err;

	_iap2_dev = dev;
	return ret;

err:
	kfree(dev);
	printk(KERN_ERR "iap2 gadget driver failed to initialize\n");
	return ret;
}

static void iap2_cleanup(struct iap2_dev *dev)
{
	misc_deregister(&iap2_device);

	if (NULL == _iap2_dev)
		return;

	if(_iap2_dev->netlink_handle) {
		netlink_kernel_release(_iap2_dev->netlink_handle);
	}

	kfree(_iap2_dev);
	_iap2_dev = NULL;
}

static void iap2_free(struct usb_function *f)
{
	(void)f;
}

struct f_iap2_opts {
	struct usb_function_instance func_inst;
	struct iap2_dev *dev;
	char *name;
	int refcnt;
	struct mutex			lock;
};

static struct f_iap2_opts *to_iap2_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_iap2_opts,
		func_inst.group);
}

static void iap2_attr_release(struct config_item *item)
{
	struct f_iap2_opts *opts = to_iap2_opts(item);

	usb_put_function_instance(&opts->func_inst);
}

static struct configfs_item_operations iap2_item_ops = {
	.release        = iap2_attr_release,
};

static struct config_item_type iap2_func_type = {
	.ct_item_ops    = &iap2_item_ops,
	.ct_owner       = THIS_MODULE,
};

static void iap_free_inst(struct usb_function_instance *fi)
{
	struct f_iap2_opts *opts;

	opts = container_of(fi, struct f_iap2_opts, func_inst);
	if (NULL != opts->name)
		kfree(opts->name);
	iap2_cleanup(opts->dev);

	kfree(opts);
}

static struct usb_function_instance *iap_alloc_inst(void)
{
	struct f_iap2_opts *opts = NULL;

	opts = kzalloc(sizeof(struct f_iap2_opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);
	//fi_iap->func_inst.set_inst_name = iap_set_inst_name;
	opts->func_inst.free_func_inst = iap_free_inst;

	mutex_init(&opts->lock);

	config_group_init_type_name(&opts->func_inst.group,
					"", &iap2_func_type);

	return  &opts->func_inst;
}

struct usb_function *iap_alloc(struct usb_function_instance *fi)
{
	struct f_iap2_opts *opts = container_of(fi, struct f_iap2_opts, func_inst);
	struct iap2_dev *dev;
	int ret = -1;

	dev = kzalloc(sizeof *dev, GFP_KERNEL);
	if (!dev)
		return ERR_PTR(-ENOMEM);
	ret = iap2_setup(dev);
	if (ret) {
		pr_err("Error setting IAP\n");
		return ERR_PTR(ret);
	}
	opts = container_of(fi, struct f_iap2_opts, func_inst);

	mutex_lock(&opts->lock);
	opts->dev = dev;
	opts->refcnt++;
	mutex_unlock(&opts->lock);

	dev = opts->dev;
	dev->function.name = DRIVER_NAME;
	dev->function.strings = iap_strings;

	dev->function.bind = iap2_function_bind;
	dev->function.unbind = iap2_function_unbind;
	dev->function.set_alt = iap2_function_set_alt;
	dev->function.disable = iap2_function_disable;
	dev->function.suspend = iap2_function_suspend;
	dev->function.free_func = iap2_free;

	return &dev->function;
}

DECLARE_USB_FUNCTION_INIT(iap, iap_alloc_inst, iap_alloc);
MODULE_LICENSE("GPL");
