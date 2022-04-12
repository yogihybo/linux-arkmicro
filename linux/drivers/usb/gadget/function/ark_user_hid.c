#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#endif
#include <linux/types.h>
#include <linux/file.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#include <linux/hid.h>
#include <linux/hiddev.h>

#if 1
#define HID_REGISTER	0x12703
#define HID_UNREGISTER	0x12704
#define HID_SET_DATA	0x12705
#else
#define HID_REGISTER   _IOWR('A', 1, char[8])
#define HID_UNREGISTER   _IOWR('A', 2, char[8])
#define HID_SET_DATA   _IOWR('A', 3, char[8])
#endif

#define HID_VENDOR  0x1573

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

struct ark_user_hid;
struct ark_hid_dev {
	struct list_head	list;
	struct hid_device *hid;
	struct ark_user_hid *dev;
	int id;
	u8 *report_desc;
	int report_desc_len;
	int report_desc_offset;
};

struct ark_user_hid {
	spinlock_t lock;
	atomic_t open_excl;
	struct delayed_work start_work;
	struct work_struct hid_work;
	struct list_head	hid_list;
	struct list_head	new_hid_list;
	struct list_head	dead_hid_list;
	bool driver_load;
};


struct hid_user_register_data {
	char __user *report_desc;
	__u32 report_desc_len;
	__u32 user_id;
};

struct hid_user_data {
	char __user *data;
	__u32 data_len;
	__u32 user_id;
};

struct hid_user_unregister_data {
	__u32 remove_all;
	__u32 user_id;
};

static struct miscdevice ark_user_hid_device;

static struct ark_user_hid *_ark_user_hid;

static int ark_hid_parse(struct hid_device *hid)
{
	struct ark_hid_dev *hdev = hid->driver_data;

	hid_parse_report(hid, hdev->report_desc, hdev->report_desc_len);
	return 0;
}

static int ark_hid_start(struct hid_device *hid)
{
	(void)hid;

	return 0;
}

static void ark_hid_stop(struct hid_device *hid)
{
	(void)hid;
}

static int ark_hid_open(struct hid_device *hid)
{
	(void)hid;
	return 0;
}

static void ark_hid_close(struct hid_device *hid)
{
	(void)hid;
}

static int ark_hid_raw_request(struct hid_device *hid, unsigned char reportnum,
	__u8 *buf, size_t len, unsigned char rtype, int reqtype)
{
	(void)hid;
	(void)reportnum;
	(void)buf;
	(void)len;
	(void)rtype;
	(void)reqtype;

	return 0;
}

static struct hid_ll_driver ark_hid_ll_driver = {
	.parse = ark_hid_parse,
	.start = ark_hid_start,
	.stop = ark_hid_stop,
	.open = ark_hid_open,
	.close = ark_hid_close,
	.raw_request = ark_hid_raw_request,
};

static struct ark_hid_dev *ark_hid_new(struct ark_user_hid *dev,
		int id, int desc_len)
{
	struct ark_hid_dev *hdev;

	hdev = kzalloc(sizeof(*hdev), GFP_ATOMIC);
	if (!hdev)
		return NULL;
	hdev->report_desc = kzalloc(desc_len, GFP_ATOMIC);
	if (!hdev->report_desc) {
		kfree(hdev);
		return NULL;
	}
	hdev->dev = dev;
	hdev->id = id;
	hdev->report_desc_len = desc_len;

	return hdev;
}

static struct ark_hid_dev *ark_hid_get(struct list_head *list, int id)
{
	struct ark_hid_dev *hid;

	list_for_each_entry(hid, list, list) {
		if (hid->id == id)
			return hid;
	}
	return NULL;
}

static int ark_register_hid(struct ark_user_hid *dev, int id, char *report_desc, int desc_length)
{
	struct ark_hid_dev *hid;
	unsigned long flags;

	if (desc_length <= 0)
		return -EINVAL;

	spin_lock_irqsave(&dev->lock, flags);

	hid = ark_hid_get(&dev->hid_list, id);
	if (!hid)
		hid = ark_hid_get(&dev->new_hid_list, id);
	if (hid)
		list_move(&hid->list, &dev->dead_hid_list);

	hid = ark_hid_new(dev, id, desc_length);
	if (!hid) {
		spin_unlock_irqrestore(&dev->lock, flags);
		return -ENOMEM;
	}
	memcpy(hid->report_desc, report_desc, desc_length);
	hid->report_desc_offset = desc_length;
	hid->report_desc_len	= desc_length;

	list_add(&hid->list, &dev->new_hid_list);
	spin_unlock_irqrestore(&dev->lock, flags);

	schedule_work(&dev->hid_work);
	return 0;
}

static int ark_unregister_hid(struct ark_user_hid *dev, int id)
{
	struct ark_hid_dev *hid;
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	hid = ark_hid_get(&dev->hid_list, id);
	if (!hid)
		hid = ark_hid_get(&dev->new_hid_list, id);
	if (!hid) {
		spin_unlock_irqrestore(&dev->lock, flags);
		return -EINVAL;
	}

	list_move(&hid->list, &dev->dead_hid_list);
	spin_unlock_irqrestore(&dev->lock, flags);

	schedule_work(&dev->hid_work);
	return 0;
}

static int ark_hid_probe(struct hid_device *hdev,
		const struct hid_device_id *id)
{
	int ret;
	//int i, j, k;
	int i, k;
	struct hid_report *report;
	//struct hid_usage *usage;
	unsigned  application = 0;

	ret = hid_parse(hdev);
	if (ret)
		return ret;

	for (k = HID_INPUT_REPORT; k <= HID_OUTPUT_REPORT; k++) {
		list_for_each_entry(report, &hdev->report_enum[k].report_list, list) {
			for (i = 0; i < report->maxfield; i++) {
				//for ( j = 0; j < report->field[i]->maxusage; j++) {
					//usage = report->field[i]->usage  j;
				application = report->field[i]->application;
				printk("application:%08x maxfield:%d\n", application, report->maxfield);
				if ((0x4 == (application & 0xffff)) && (HID_UP_DIGITIZER == (application & HID_USAGE_PAGE))) {
					hdev->product = 0x3896;
					printk("touch screen\n");break;
				} else if ((0x05 == (application & 0xffff)) && (HID_UP_DIGITIZER == (application & HID_USAGE_PAGE))) {
					hdev->product = 0x3897;
					printk("touch pad\n");break;
				} else if ((0x08 == (application & 0xffff)) && (HID_UP_GENDESK == (application & HID_USAGE_PAGE))) {
					hdev->product = 0x3898;
					printk("knob\n");break;
				} else if ((0x07 == (application & 0xffff) || 0x01 == (application & 0xffff)) && (HID_UP_TELEPHONY == (application & HID_USAGE_PAGE))) {
					hdev->product = 0x3899;
					printk("telephone key\n");break;
				} else if ((0x01 == (application & 0xffff)) && (HID_UP_CONSUMER == (application & HID_USAGE_PAGE))) {
					hdev->product = 0x389A;
					printk("media key\n");break;
				}
				//}
			}
		}
	}

	if (hdev->product == HID_ANY_ID)
		hdev->product = 0x38FF;

	ret = hid_hw_start(hdev, HID_CONNECT_HIDINPUT | HID_CONNECT_HIDINPUT_FORCE);

	return ret;
}

static const struct hid_device_id ark_hid_table[] = {
	{ HID_USB_DEVICE(HID_VENDOR, HID_ANY_ID) },
	{ }
};
MODULE_DEVICE_TABLE(hid, ark_hid_table);

static struct hid_driver ark_hid_driver = {
	.name = "Ark hid",
	.id_table = ark_hid_table,
	.probe = ark_hid_probe,
};

static void
kill_all_hid_devices(struct ark_user_hid *dev)
{
	struct ark_hid_dev *hid;
	struct list_head *entry, *temp;
	unsigned long flags;

	if (!dev)
		return;

	spin_lock_irqsave(&dev->lock, flags);
	list_for_each_safe(entry, temp, &dev->hid_list) {
		hid = list_entry(entry, struct ark_hid_dev, list);
		list_del(&hid->list);
		list_add(&hid->list, &dev->dead_hid_list);
	}
	list_for_each_safe(entry, temp, &dev->new_hid_list) {
		hid = list_entry(entry, struct ark_hid_dev, list);
		list_del(&hid->list);
		list_add(&hid->list, &dev->dead_hid_list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);

	schedule_work(&dev->hid_work);
}

#if 0
static void
ark_hid_unbind(struct ark_user_hid *dev)
{
	printk("%s:%d\n", __func__, __LINE__);
	hid_unregister_driver(&ark_hid_driver);
	kill_all_hid_devices(dev);
	printk("%s:%d\n", __func__, __LINE__);
}
#endif

static int ark_hid_init(struct ark_hid_dev *hdev)
{
	struct hid_device *hid;
	int ret;

	hid = hid_allocate_device();
	if (IS_ERR(hid))
		return PTR_ERR(hid);

	hid->ll_driver = &ark_hid_ll_driver;
	hid->dev.parent = ark_user_hid_device.this_device;

	hid->bus = BUS_USB;
	hid->vendor = HID_VENDOR;
	hid->product = HID_ANY_ID;
	hid->driver_data = hdev;
	ret = hid_add_device(hid);
	if (ret) {
		pr_err("can't add hid device: %d\n", ret);
		hid_destroy_device(hid);
		return ret;
	}

	hdev->hid = hid;
	return 0;
}

static void ark_hid_delete(struct ark_hid_dev *hid)
{
	printk("%s:%d\n", __func__, __LINE__);
	kfree(hid->report_desc);
	kfree(hid);
	printk("%s:%d\n", __func__, __LINE__);
}

static void ark_hid_work(struct work_struct *data)
{
	struct ark_user_hid *dev = _ark_user_hid;
	struct list_head	*entry, *temp;
	struct ark_hid_dev *hid;
	struct list_head	new_list, dead_list;
	unsigned long flags;

	INIT_LIST_HEAD(&new_list);

	spin_lock_irqsave(&dev->lock, flags);

	/* copy hids that are ready for initialization to new_list */
	list_for_each_safe(entry, temp, &dev->new_hid_list) {
		hid = list_entry(entry, struct ark_hid_dev, list);
		if (hid->report_desc_offset == hid->report_desc_len)
			list_move(&hid->list, &new_list);
	}

	if (list_empty(&dev->dead_hid_list)) {
		INIT_LIST_HEAD(&dead_list);
	} else {
		/* move all of dev->dead_hid_list to dead_list */
		dead_list.prev = dev->dead_hid_list.prev;
		dead_list.next = dev->dead_hid_list.next;
		dead_list.next->prev = &dead_list;
		dead_list.prev->next = &dead_list;
		INIT_LIST_HEAD(&dev->dead_hid_list);
	}

	spin_unlock_irqrestore(&dev->lock, flags);

	/* register new HID devices */
	list_for_each_safe(entry, temp, &new_list) {
		hid = list_entry(entry, struct ark_hid_dev, list);
		if (ark_hid_init(hid)) {
			pr_err("can't add HID device %p\n", hid);
			ark_hid_delete(hid);
		} else {
			spin_lock_irqsave(&dev->lock, flags);
			list_move(&hid->list, &dev->hid_list);
			spin_unlock_irqrestore(&dev->lock, flags);
		}
	}

	/* remove dead HID devices */
	list_for_each_safe(entry, temp, &dead_list) {
		hid = list_entry(entry, struct ark_hid_dev, list);
		list_del(&hid->list);
		if (hid->hid)
			hid_destroy_device(hid->hid);
		ark_hid_delete(hid);
	}
}

#if 0
static ssize_t ark_misc_read(struct file *fp, char __user *buf,
	size_t count, loff_t *pos)
{
	ssize_t r = count;

	pr_debug("ark_misc_read(%zu)\n", count);

	return r;
}

static ssize_t ark_misc_write(struct file *fp, const char __user *buf,
	size_t count, loff_t *pos)
{
	ssize_t r = count;

	pr_debug("ark_misc_write(%zu)\n", count);

	pr_debug("ark_misc_write returning %zd\n", r);
	return r;
}
#endif

static long ark_misc_ioctl(struct file *fp, unsigned code, unsigned long arg)
{
	struct hid_user_register_data reg_arg;
	struct hid_user_data		 data_arg;
	struct hid_user_unregister_data unreg_arg;
	struct ark_user_hid *dev = fp->private_data;
	char *user_data = NULL;
	struct ark_hid_dev *hid;
	int ret = -EFAULT;
	char data[256] = {0};

	switch (code) {
	case HID_REGISTER:
		if (dev->driver_load == 0) {
			ret = hid_register_driver(&ark_hid_driver);
			if (ret == 0)
				dev->driver_load = 1;
		}

		if (copy_from_user(&reg_arg,
			   (struct hid_user_register_data __user *)arg,
			   sizeof(reg_arg)))
			return -EFAULT;
		if (reg_arg.report_desc_len <=0 && reg_arg.report_desc_len > 65536)
			return -EINVAL;
		user_data = (char *)memdup_user(reg_arg.report_desc, reg_arg.report_desc_len);
		
		ret = ark_register_hid(dev, reg_arg.user_id, user_data, reg_arg.report_desc_len);
		kfree(user_data);
		break;
	case HID_UNREGISTER:
		if (copy_from_user(&unreg_arg,
			   (struct hid_user_unregister_data __user *)arg,
			   sizeof(unreg_arg)))
			return -EFAULT;
		if (unreg_arg.remove_all == 0)
			ark_unregister_hid(dev, (int)unreg_arg.user_id);
		else
			kill_all_hid_devices(dev);
		ret = 0;
		break;
	case HID_SET_DATA:
		if (copy_from_user(&data_arg,
			   (struct hid_user_data __user *)arg,
			   sizeof(data_arg)))
			return -EFAULT;
		if (data_arg.data_len <= 0 && data_arg.data_len > 256)
			return -EINVAL;
		//user_data = (char *)memdup_user(data_arg.data, data_arg.data_len);
		if (copy_from_user((void *)data, (char __user *)data_arg.data, data_arg.data_len))
			return -EFAULT;	
		hid = ark_hid_get(&dev->hid_list, data_arg.user_id);
		/*
		int i;
		for (i = 0; i < data_arg.data_len; i++) {
			printk("%02x ", user_data[i]);
		}printk("\n");*/
		hid_input_report(hid->hid, HID_INPUT_REPORT, data, data_arg.data_len, 1);
		//kfree(user_data);
		ret = 0;
		break;
	default:
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long ark_misc_compat_ioctl(struct file *fp, unsigned code, unsigned long arg)
{
	void __user *arg64 = compat_ptr(arg);
	long ret;

	if (!file->f_op || !file->f_op->unlocked_ioctl) {
		printk("unlocked_ioctl is null\n");
		return -ENOTTY;
	}

	ret = file->f_op->unlocked_ioctl(file, code, (unsigned long)arg64);

	return ret;
}
#endif

static int ark_misc_open(struct inode *ip, struct file *fp)
{
	printk("ark_misc_open\n");
	if (atomic_xchg(&_ark_user_hid->open_excl, 1))
		return -EBUSY;

	fp->private_data = _ark_user_hid;
	return 0;
}

static int ark_misc_release(struct inode *ip, struct file *fp)
{
	printk(KERN_INFO "ark_misc_release\n");

	WARN_ON(!atomic_xchg(&_ark_user_hid->open_excl, 0));
	kill_all_hid_devices(_ark_user_hid);

	return 0;
}

static const struct file_operations ark_hid_misc_fops = {
	.owner = THIS_MODULE,
	//.read = ark_misc_read,
	//.write = ark_misc_write,
	.unlocked_ioctl = ark_misc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = ark_misc_compat_ioctl,
#endif
	.open = ark_misc_open,
	.release = ark_misc_release,
};

static int ark_user_hid_init(void)
{

	struct ark_user_hid *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	spin_lock_init(&dev->lock);
	atomic_set(&dev->open_excl, 0);
	INIT_LIST_HEAD(&dev->hid_list);
	INIT_LIST_HEAD(&dev->new_hid_list);
	INIT_LIST_HEAD(&dev->dead_hid_list);
	INIT_WORK(&dev->hid_work, ark_hid_work);
	
	ark_user_hid_device.minor 	= MISC_DYNAMIC_MINOR;
	ark_user_hid_device.name 	= "ark_user_hid";
	ark_user_hid_device.fops 	= &ark_hid_misc_fops;

	ret = misc_register(&ark_user_hid_device);
	if (ret)
		goto err;
	dev->driver_load = 0;
	
	_ark_user_hid = dev;
err:
	if (ret) {
		kfree(dev);
		dev = NULL;
	}
	return ret;
}

static void ark_user_hid_exit(void)
{
	if (_ark_user_hid->driver_load) {
		hid_unregister_driver(&ark_hid_driver);
		_ark_user_hid->driver_load = false;
	}
	misc_deregister(&ark_user_hid_device);
	kfree(_ark_user_hid);
	_ark_user_hid = NULL;
}

module_init(ark_user_hid_init);
module_exit(ark_user_hid_exit);

MODULE_AUTHOR("lucas");
MODULE_DESCRIPTION("user hid driver module");
MODULE_LICENSE("GPL");


