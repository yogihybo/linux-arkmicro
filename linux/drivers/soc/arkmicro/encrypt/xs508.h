#ifndef _XS508_H_
#define _XS508_H_

#define XS508_I2C_NAME				"dvr_xs508"
#define XS508_DEVICE_NAME			"xs508"
#define XS508_I2C_RETRY_COUNT		5

#define XS508_IOCTL						0x60
#define XS508_IOCTL_CMD_HANDSHAKE_KEY	_IO(XS508_IOCTL, 0x11)

struct xs508_private_data{
    //spinlock_t spin_lock;
	struct i2c_client *client;
	struct i2c_device_id *id;
	struct mutex lock;
};

#endif
