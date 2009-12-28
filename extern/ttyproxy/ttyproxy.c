/* -*- C -*-
 * ttyp.c -- the simple tty proxy device
 *
 * Copyright (C) 2009 Darek Marcinkiewicz
 *
 *
 * $Id: _main.c.in,v 1.21 2004/10/14 20:11:39 corbet Exp $
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/tty_driver.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/kfifo.h>
#include <asm/uaccess.h>

#include "ttyproxy.h"

#define BUFFER_SIZE 2048

struct ttyp_dev {
	struct kfifo *slave_buf;
	spinlock_t ttyp_mutex;
	spinlock_t fifo_mutex;
	char *tmp_buf;

	unsigned char modem_flags;
	int slave_ready;

	wait_queue_head_t slave_wait, tty_wait;

	struct tty_struct *tty;	/* the master tty */
	struct cdev slave_dev;	/* slave character device */
};

static struct tty_driver *tty_proxy_driver;

struct class *ttyp_class;

int ttyp_devs_count = 4;
int ttyps_major;
struct ttyp_dev *ttyp_devices;

static int ttyp_init(void);
static void ttyp_cleanup(void);

static int ttyps_open(struct inode *inode, struct file *filp);
static int ttyps_release(struct inode *inode, struct file *filp);
static unsigned int ttyps_poll(struct file *filp, poll_table * wait);
static int ttyps_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
		       unsigned long arg);
static int ttyps_iocready(struct ttyp_dev *dev);
static int ttyps_tiocmset(struct ttyp_dev *dev, unsigned char __user * arg);
static int ttyps_get_termios(struct ttyp_dev *dev, unsigned int cmd,
			     unsigned long arg);
static ssize_t ttyps_read(struct file *filp, char __user * buf, size_t count,
			  loff_t * f_pos);
static ssize_t ttyps_write(struct file *filp, const char __user * buf,
			   size_t count, loff_t * f_pos);

static int ttypm_open(struct tty_struct *tty, struct file *filp);
static void ttypm_release(struct tty_struct *tty, struct file *filp);
static ssize_t ttypm_write(struct tty_struct *tty, const unsigned char *buf,
			   int count);
static int ttypm_write_room(struct tty_struct *tty);
static void ttypm_set_termios(struct tty_struct *tty,
			      struct ktermios *old_termios);

static int ttyps_open(struct inode *inode, struct file *filp)
{
	struct ttyp_dev *dev;
	dev = container_of(inode->i_cdev, struct ttyp_dev, slave_dev);
	filp->private_data = dev;
	return 0;
}

ssize_t ttyps_read(struct file * filp, char __user * buf, size_t count,
		   loff_t * f_pos)
{
	struct ttyp_dev *dev = filp->private_data;
	while (kfifo_len(dev->slave_buf) == 0) {
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		if (wait_event_interruptible
		    (dev->slave_wait, (kfifo_len(dev->slave_buf) > 0)))
			return -ERESTARTSYS;

	}
	count = min(count, (size_t) BUFFER_SIZE);
	count = kfifo_get(dev->slave_buf, dev->tmp_buf, count);
	if (copy_to_user(buf, dev->tmp_buf, count))
		return -EFAULT;
	return count;
}

int slave_write_ready(struct ttyp_dev *dev)
{
	return dev->tty != NULL && test_bit(TTY_THROTTLED, &dev->tty->flags);
}

ssize_t ttyps_write(struct file * filp, const char __user * buf, size_t count,
		    loff_t * f_pos)
{
	struct ttyp_dev *dev = filp->private_data;
	count = min(count, (size_t) BUFFER_SIZE);
	if (copy_from_user(dev->tmp_buf, buf, count))
		return -EFAULT;
	spin_lock(&dev->ttyp_mutex);
	while (!slave_write_ready(dev)) {
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		spin_unlock(&dev->ttyp_mutex);
		if (!wait_event_interruptible(dev->tty_wait,
					      slave_write_ready(dev)))
			return -ERESTARTSYS;
		spin_lock(&dev->ttyp_mutex);
	}
	tty_insert_flip_string(dev->tty, dev->tmp_buf, count);
	tty_flip_buffer_push(dev->tty);
	spin_unlock(&dev->ttyp_mutex);
	return count;
}

static int ttyps_release(struct inode *inode, struct file *filp)
{
	struct ttyp_dev *dev = filp->private_data;
	spin_lock(&dev->ttyp_mutex);
	dev->slave_ready = 0;
	spin_unlock(&dev->ttyp_mutex);
	return 0;
}

static int ttyps_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
		       unsigned long arg)
{
	struct ttyp_dev *dev = filp->private_data;
	int ret = -ENXIO;

	switch (cmd) {
	case TTYPROXY_IOCSREADY:
		ret = ttyps_iocready(dev);
		break;
	case TIOCMSET:
		ret = ttyps_tiocmset(dev, (unsigned char __user *)arg);
		break;
	case TCGETS:
	case TCGETS2:
	case TCGETA:
		ret = ttyps_get_termios(dev, cmd, arg);
		break;
	default:
		break;
	}

	return ret;
}

static int ttyps_iocready(struct ttyp_dev *dev)
{
	spin_lock(&dev->ttyp_mutex);
	dev->slave_ready = 1;
	spin_unlock(&dev->ttyp_mutex);
	return 0;
}

static int ttyps_tiocmset(struct ttyp_dev *dev, unsigned char __user * arg)
{
	unsigned char flags;
	if (copy_from_user(&flags, arg, sizeof(flags)))
		return -EFAULT;
#define FFLAGS (TIOCM_CD | TIOCM_RNG | TIOCM_DSR | TIOCM_CTS)
	flags &= FFLAGS;
	spin_lock(&dev->ttyp_mutex);
	dev->modem_flags &= ~FFLAGS;
	dev->modem_flags |= flags;
	spin_unlock(&dev->ttyp_mutex);
#undef FFLAGS
	return 0;
}

static int ttyps_get_termios(struct ttyp_dev *dev, unsigned int cmd,
			     unsigned long arg)
{
	struct tty_struct *real_tty;
	int ret;

	spin_lock(&dev->ttyp_mutex);
	real_tty = dev->tty;
	if (real_tty == NULL) {
		spin_unlock(&dev->ttyp_mutex);
		return -EFAULT;
	}

	switch (cmd) {
#ifndef TCGETS2
	case TCGETS:
		mutex_lock(&real_tty->termios_mutex);
		if (kernel_termios_to_user_termios
		    ((struct termios __user *)arg, real_tty->termios))
			ret = -EFAULT;
		mutex_unlock(&real_tty->termios_mutex);
		break;
#else
	case TCGETS:
		mutex_lock(&real_tty->termios_mutex);
		if (kernel_termios_to_user_termios_1
		    ((struct termios __user *)arg, real_tty->termios))
			ret = -EFAULT;
		mutex_unlock(&real_tty->termios_mutex);
		break;
	case TCGETS2:
		mutex_lock(&real_tty->termios_mutex);
		if (kernel_termios_to_user_termios
		    ((struct termios2 __user *)arg, real_tty->termios))
			ret = -EFAULT;
		mutex_unlock(&real_tty->termios_mutex);
		break;
#endif
	case TCGETA:
		mutex_lock(&real_tty->termios_mutex);
		if (kernel_termios_to_user_termio
		    ((struct termio __user *)arg, real_tty->termios))
			ret = -EFAULT;
		mutex_unlock(&real_tty->termios_mutex);
		break;
	}
	spin_unlock(&dev->ttyp_mutex);

	return ret;
}

static unsigned int ttyps_poll(struct file *filp, poll_table * wait)
{
	unsigned int mask = 0;
	struct ttyp_dev *dev = filp->private_data;
	struct tty_struct *tty;
	poll_wait(filp, &dev->slave_wait, wait);
	poll_wait(filp, &dev->tty_wait, wait);
	spin_lock(&dev->ttyp_mutex);
	if (kfifo_len(dev->slave_buf) > 0)
		mask |= POLLIN | POLLRDNORM;
	tty = dev->tty;
	if (tty != NULL && test_bit(TTY_THROTTLED, &tty->flags) == 0)
		mask |= POLLOUT | POLLWRNORM;
	spin_unlock(&dev->ttyp_mutex);
	return mask;
}

static int ttypm_open(struct tty_struct *tty, struct file *filp)
{
	struct ttyp_dev *dev = ttyp_devices + tty->index;
	spin_lock(&dev->ttyp_mutex);
	tty->driver_data = dev;
	dev->tty = tty;
	spin_unlock(&dev->ttyp_mutex);
	wake_up_interruptible(&dev->tty_wait);
	return 0;
}

static void ttypm_release(struct tty_struct *tty, struct file *filp)
{
	struct ttyp_dev *dev = tty->driver_data;
	spin_lock(&dev->ttyp_mutex);
	dev->tty = NULL;
	spin_unlock(&dev->ttyp_mutex);
	wake_up_interruptible(&dev->tty_wait);
}

static int ttypm_write_room(struct tty_struct *tty)
{
	int room;
	struct ttyp_dev *dev = tty->driver_data;
	if (!dev)
		return -ENODEV;
	room = BUFFER_SIZE - kfifo_len(dev->slave_buf);
	room -= room / 256 + 1;
	return room;
}

static void ttypm_set_termios(struct tty_struct *tty,
			      struct ktermios *old_termios)
{
	unsigned char zero = 0;
	unsigned char command = TERMIOS_CHANGE;
	struct ttyp_dev *dev = tty->driver_data;
	spin_lock(&dev->ttyp_mutex);
	if (!dev->slave_ready) {
		spin_unlock(&dev->ttyp_mutex);
		return;
	}
	spin_lock(&dev->ttyp_mutex);
	if (kfifo_len(dev->slave_buf) >= BUFFER_SIZE - 2) {
		printk(KERN_ERR "Buffer overflow, termios change info lost!");
		return;
	}
	kfifo_put(dev->slave_buf, &zero, 1);
	kfifo_put(dev->slave_buf, &command, 1);
	wake_up_interruptible(&dev->slave_wait);
}

static void ttypm_unthrottle(struct tty_struct *tty)
{
	struct ttyp_dev *dev = tty->driver_data;
	wake_up_interruptible(&dev->tty_wait);
}

static ssize_t ttypm_write(struct tty_struct *tty, const unsigned char *buf,
			   int count)
{
	struct ttyp_dev *dev = tty->driver_data;
	int written = 0;
	spin_lock(&dev->ttyp_mutex);
	if (!dev || !dev->slave_ready) {
		spin_unlock(&dev->ttyp_mutex);
		return -ENODEV;
	}
	while (written < count) {
		unsigned char towrite;
		int avail = BUFFER_SIZE - kfifo_len(dev->slave_buf);
		if (avail < 2)
			break;

		towrite = min(255, min(avail, count - written));
		kfifo_put(dev->slave_buf, &towrite, 1);
		kfifo_put(dev->slave_buf, (unsigned char *)buf + written,
			  towrite);

		written += towrite;
	}
	spin_unlock(&dev->ttyp_mutex);
	wake_up_interruptible(&dev->slave_wait);
	return written;
}

static int ttypm_tiocmset(struct tty_struct *tty, struct file *filp,
			  unsigned int set, unsigned int clear)
{
	struct ttyp_dev *dev = tty->driver_data;

	unsigned char zero = 0;
	unsigned char command = 0;

	spin_lock(&dev->ttyp_mutex);

	if (set & TIOCM_DTR) {
		command |= SET_DTR;
		dev->modem_flags |= TIOCM_DTR;
	}

	if (set & TIOCM_RTS) {
		command |= SET_RTS;
		dev->modem_flags |= TIOCM_RTS;
	}

	if (clear & TIOCM_DTR) {
		command |= CLEAR_DTR;
		dev->modem_flags &= ~TIOCM_DTR;
	}

	if (clear & TIOCM_RTS) {
		command |= CLEAR_RTS;
		dev->modem_flags &= ~TIOCM_RTS;
	}

	if (command == 0) {
		spin_unlock(&dev->ttyp_mutex);
		return 0;
	}

	spin_unlock(&dev->ttyp_mutex);

	if (kfifo_len(dev->slave_buf) >= BUFFER_SIZE - 2) {
		printk(KERN_ERR "Buffer overflow, terminal change info lost!");
		return -ENOBUFS;
	}

	kfifo_put(dev->slave_buf, &zero, 1);
	kfifo_put(dev->slave_buf, &command, 1);

	wake_up_interruptible(&dev->slave_wait);

	return 0;
}

static int ttypm_tiocmget(struct tty_struct *tty, struct file *filp)
{
	struct ttyp_dev *dev = tty->driver_data;
	int modem_flags;

	spin_lock(&dev->ttyp_mutex);

	modem_flags = dev->modem_flags;

	spin_unlock(&dev->ttyp_mutex);

	return modem_flags;
}

static struct file_operations ttyps_fops = {
	.owner = THIS_MODULE,
	.read = ttyps_read,
	.write = ttyps_write,
	.open = ttyps_open,
	.poll = ttyps_poll,
	.ioctl = ttyps_ioctl,
	.release = ttyps_release,
};

static struct tty_operations serial_ops = {
	.open = ttypm_open,
	.close = ttypm_release,
	.write = ttypm_write,
	.write_room = ttypm_write_room,
	.set_termios = ttypm_set_termios,
	.unthrottle = ttypm_unthrottle,
	.tiocmget = ttypm_tiocmget,
	.tiocmset = ttypm_tiocmset,
};

static int ttyp_device_init(struct ttyp_dev *dev, int index)
{
	int err;
	int devno;
	struct device *c_dev, *t_dev;

	cdev_init(&dev->slave_dev, &ttyps_fops);
	init_waitqueue_head(&dev->slave_wait);
	init_waitqueue_head(&dev->tty_wait);

	spin_lock_init(&dev->ttyp_mutex);
	spin_lock_init(&dev->fifo_mutex);

	dev->slave_buf = kfifo_alloc(BUFFER_SIZE, GFP_KERNEL, &dev->fifo_mutex);
	if (dev->slave_buf == 0) {
		printk(KERN_NOTICE "Error in creating fifo");
		err = -ENOMEM;
		goto fifo_err;
	}

	dev->tmp_buf = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	if (dev->tmp_buf == NULL) {
		printk(KERN_NOTICE "Failed to alloc temporary buffer");
		err = -ENOMEM;
		goto kmalloc_err;
	}

	devno = MKDEV(ttyps_major, index);
	err = cdev_add(&dev->slave_dev, devno, 1);
	if (err) {
		printk(KERN_NOTICE "Error %d adding device %d", err, index);
		goto cdev_add_err;
	}

	c_dev = device_create(ttyp_class, NULL, devno, NULL, "pttys%d", index);
	if (IS_ERR(c_dev)) {
		err = PTR_ERR(c_dev);
		printk(KERN_NOTICE "Error %d cerating device %d", err, index);
		goto dev_create_err;
	}

	t_dev = tty_register_device(tty_proxy_driver, index, NULL);
	if (IS_ERR(t_dev)) {
		err = PTR_ERR(c_dev);
		printk(KERN_NOTICE "Error %d registering tty device %d", err,
		       index);
		goto tty_register_err;
	}

	return 0;

      tty_register_err:
	device_destroy(ttyp_class, MKDEV(ttyps_major, index));
      dev_create_err:
	cdev_del(&dev->slave_dev);
      cdev_add_err:
	kfree(dev->tmp_buf);
      kmalloc_err:
	kfifo_free(dev->slave_buf);
      fifo_err:
	return err;
}

void ttyp_device_destroy(struct ttyp_dev *device, int minor)
{
	kfifo_free(device->slave_buf);
	kfree(device->tmp_buf);
	cdev_del(&device->slave_dev);
	device_destroy(ttyp_class, MKDEV(ttyps_major, minor));
	tty_unregister_device(tty_proxy_driver, minor);
}

int ttyp_init(void)
{
	int result, i = 0, j;
	dev_t slave_dev = MKDEV(0, 0);

	tty_proxy_driver = alloc_tty_driver(ttyp_devs_count);
	if (!tty_proxy_driver)
		return -ENOMEM;

	tty_proxy_driver->owner = THIS_MODULE;
	tty_proxy_driver->driver_name = "tty_proxy";
	tty_proxy_driver->name = "pttym";
	tty_proxy_driver->major = 0;
	tty_proxy_driver->type = TTY_DRIVER_TYPE_SERIAL;
	tty_proxy_driver->subtype = SERIAL_TYPE_NORMAL;
	tty_proxy_driver->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
	tty_proxy_driver->init_termios = tty_std_termios;
	tty_proxy_driver->init_termios.c_cflag =
	    B0 | CS8 | CREAD | HUPCL | CLOCAL;

	tty_set_operations(tty_proxy_driver, &serial_ops);
	result = tty_register_driver(tty_proxy_driver);
	if (result) {
		printk(KERN_ERR "failed to register proxy tty driver");
		put_tty_driver(tty_proxy_driver);
		return result;
	}

	ttyp_devices =
	    kmalloc(ttyp_devs_count * sizeof(struct ttyp_dev), GFP_KERNEL);
	if (!ttyp_devices) {
		result = -ENOMEM;
		goto no_mem;
	}

	result = alloc_chrdev_region(&slave_dev, 0, ttyp_devs_count, "pttyps");
	if (result < 0)
		goto slave_err;
	ttyps_major = MAJOR(slave_dev);

	ttyp_class = class_create(THIS_MODULE, "ttypslave");
	if (IS_ERR(ttyp_class)) {
		result = PTR_ERR(ttyp_class);
		goto class_err;
	}

	memset(ttyp_devices, 0, ttyp_devs_count * sizeof(struct ttyp_dev));
	for (i = 0; i < ttyp_devs_count; i++)
		if (ttyp_device_init(ttyp_devices + i, i))
			goto dev_init_err;

	printk(KERN_NOTICE "ttyp module initialized, created %d devices\n", i);
	return 0;

      dev_init_err:
	for (j = 0; j < i; j++)
		ttyp_device_destroy(ttyp_devices + j, j);

	class_destroy(ttyp_class);
      class_err:
	unregister_chrdev_region(slave_dev, ttyp_devs_count);
      slave_err:
	kfree(ttyp_devices);
      no_mem:
	printk(KERN_NOTICE "Error initializing ttyp module\n");

	return result;
}

void ttyp_cleanup(void)
{
	int i;
	for (i = 0; i < ttyp_devs_count; i++)
		ttyp_device_destroy(&ttyp_devices[i], i);
	class_destroy(ttyp_class);
	unregister_chrdev_region(MKDEV(ttyps_major, 0), ttyp_devs_count);
	tty_unregister_driver(tty_proxy_driver);
	kfree(ttyp_devices);
}

module_param(ttyp_devs_count, int, 0);

module_init(ttyp_init);
module_exit(ttyp_cleanup);

MODULE_AUTHOR("Darek Marcinkiewicz (reksio@praterm.com.pl)");
MODULE_LICENSE("GPL");
