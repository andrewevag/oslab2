/*
 * lunix-chrdev.c
 *
 * Implementation of character devices
 * for Lunix:TNG
 *
 * Andreas Evaggelatos
 * Panagiota-Nikoletta Barmpa
 */

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mmzone.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>

#include "lunix.h"
#include "lunix-chrdev.h"
#include "lunix-lookup.h"

/*
 * Global data
 */
struct cdev lunix_chrdev_cdev;

/*
 * Just a quick [unlocked] check to see if the cached
 * chrdev state needs to be updated from sensor measurements.
 */
static int lunix_chrdev_state_needs_refresh(struct lunix_chrdev_state_struct *state)
{
	struct lunix_sensor_struct *sensor;
	
	WARN_ON ( !(sensor = state->sensor));
	/* ? */
	//0elei lock
	if(state->buf_timestamp == sensor->msr_data[state->type]->last_update)
	{
		return 0;
	}
	

	/* The following return is bogus, just for the stub to compile */
	return 1; /* ? */
}

/*
 * Updates the cached state of a character device
 * based on sensor data. Must be called with the
 * character device state lock held.
 */
static int lunix_chrdev_state_update(struct lunix_chrdev_state_struct *state)
{
	struct lunix_sensor_struct *sensor;
	uint32_t temp_val;
	debug("entered\n");
	sensor = state->sensor;
	/*
	 * Grab the raw data quickly, hold the
	 * spinlock for as little as possible.
	 */
	/* ? */

	if(lunix_chrdev_state_needs_refresh(state) == 0)
	{
		debug("leaving at needs -EAGAIN\n"); 
		return -EAGAIN;
	}

	// an den exei nea dedomena dwse  -EAGAIN
	/* Why use spinlocks? See LDD3, p. 119 */
	spin_lock(&sensor->lock);
	//SPLIN LOCK TO GET THE DATA FROM THE LINE DISCIPLE SENSOR_STRUCT
	//kai valta ston lunix_chrdev_state_struct.
	//kanoume copy to timestampt, kai ta dedomena ston buf_data
	state->buf_timestamp = sensor->msr_data[state->type]->last_update;
	temp_val = sensor->msr_data[state->type]->values[0];
	/*
	 * Any new data available?
	 */
	/* ? */
	//SPIN UNLOCK
	spin_unlock(&sensor->lock);
	debug("YES THE LINE WORKS---------------------------\n\n\n\n");
	debug("%d\n", state->buf_timestamp);
	debug("temp_val read %u\n", temp_val);
	int dekadiko_meros, akeraio_meros;
	long whole;
	/*
	 * Now we can take our time to format them,
	 * holding only the private state semaphore
	 */
	switch (state->type)
	{
	case BATT:
		//edw mporei na einai mono 9etikes times.
		whole = lookup_voltage[temp_val];
		break;
	case TEMP:
		whole = lookup_temperature[temp_val];
		break;
	case LIGHT:
		whole = lookup_light[temp_val];
		break;
		
	default:
		whole = 0;
		break;
	}
	debug("WHOLE =  %ld\n", whole);
	akeraio_meros = whole / 1000;
	dekadiko_meros = whole % 1000;
	state->buf_lim = snprintf(state->buf_data, LUNIX_CHRDEV_BUFSZ, "%d.%d\n", akeraio_meros, dekadiko_meros);

	/* ? */
	debug("buf data = %s\n", state->buf_data);
	debug("leaving\n");
	return 0;
}

/*************************************
 * Implementation of file operations
 * for the Lunix character device
 *************************************/

static int lunix_chrdev_open(struct inode *inode, struct file *filp)
{
	/* Declarations */
	unsigned int mes_id, mes_type, sensor_id;
	/* ? */
	int ret;

	debug("entering\n");
	ret = -ENODEV;
	if ((ret = nonseekable_open(inode, filp)) < 0)
		goto out;

	/*
	 * Associate this open file with the relevant sensor based on
	 * the minor number of the device node [/dev/sensor<NO>-<TYPE>]
	 */
	
	mes_id = iminor(inode);
	sensor_id = mes_id >> 3;
	mes_type = mes_id & 0x7;
	
	/*
	 * Check that the device is of the ones we handle.
	*/
	if(sensor_id >= lunix_sensor_cnt || mes_type >= N_LUNIX_MSR){
		ret = -ENODEV;
		goto out;
	}
	//to lunix_sensor_struct * --> lunix_sensors[sensor_id];
	
	/* Allocate a new Lunix character device private state structure */
	struct lunix_chrdev_state_struct* new_chdev_state;
	//MHN 3EXASEIS STO CLOSE TO KFREE.
	new_chdev_state = (struct lunix_chrdev_state_struct* )kmalloc(sizeof(struct lunix_chrdev_state_struct), GFP_KERNEL);

	if(!new_chdev_state){
		debug("failed to allocate memory for chrdev_state\n");
		goto out;
	}
	//Set state parameters
	new_chdev_state->type = mes_type;
	new_chdev_state->sensor = &lunix_sensors[sensor_id];
	new_chdev_state->buf_lim = 1;
	new_chdev_state->buf_data[0] = '\0';
	new_chdev_state->buf_timestamp = 0;
	//buf lim 9a pros9esw toy filou mou to (?)
	//initialize semaphore to avoid race conditions.
	sema_init(&new_chdev_state->lock, 1);

	//pass it to private data
	filp->private_data = new_chdev_state;
	debug("successfuly opened\n");
	/* ? */
out:
	debug("leaving, with ret = %d\n", ret);
	return ret;
}

static int lunix_chrdev_release(struct inode *inode, struct file *filp)
{
	/* ? */
	kfree((filp->private_data));
	return 0;
}

static long lunix_chrdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	/* Why? */
	return -EINVAL;
}

static ssize_t lunix_chrdev_read(struct file *filp, char __user *usrbuf, size_t cnt, loff_t *f_pos)
{
	
	
	ssize_t ret = 0;

	struct lunix_sensor_struct *sensor;
	struct lunix_chrdev_state_struct *state;

	state = filp->private_data;
	WARN_ON(!state);

	sensor = state->sensor;
	WARN_ON(!sensor);
	
	/* Lock? */

	//DIKO MAS
	if (down_interruptible(&state->lock))
 		return -ERESTARTSYS;



	/*
	 * If the cached character device state needs to be
	 * updated by actual sensor data (i.e. we need to report
	 * on a "fresh" measurement, do so
	 */
	if (*f_pos == 0) {
		//if we enter here it means all data are consumed
		while (lunix_chrdev_state_update(state) == -EAGAIN) {
			/* ? */
			up(&state->lock); //release the lock.
			/* The process needs to sleep */
			/* See LDD3, page 153 for a hint */

			//if no data.
			//mpes sthn oura tou sensor kai 9a se 3upnhsei to line_disciple
			if(wait_event_interruptible(sensor->wq, lunix_chrdev_state_needs_refresh(state)))
				return -ERESTARTSYS;

			//reacquire lock
			if (down_interruptible(&state->lock))
 				return -ERESTARTSYS;
		}
	}

	/* End of file */
	/* ? */
	
	/* Determine the number of cached bytes to copy to userspace */
	/* ? */
	if(*f_pos >= state->buf_lim){
 		*f_pos = 0;
		goto out;

	}
	
 	// if(*f_pos + cnt > state->buf_lim)
 	// 	cnt = state->buf_lim - *f_pos;
	cnt = min(cnt, state->buf_lim - *f_pos);

	debug("before copy_to_user (newchange)   cnt = %d, f_pos = %d\n", cnt, *f_pos);
	//the following checks if the pointer is faulty.
	if(copy_to_user(usrbuf, state->buf_data + *f_pos, cnt)){
		debug("returned from copy_to_user with fault\n");
		ret = -EFAULT;
		goto out;
	}
	debug("returned from copy_to_user without problem\n");
	*f_pos += cnt;
	ret = cnt;
	/* Auto-rewind on EOF mode? */
	if(*f_pos >= state->buf_lim)
		*f_pos = 0; //so that on next read we will block till new data arrive.
	

	/* ? */
out:
	/* Unlock? */
	up(&state->lock); //release the lock.
	return ret;
}

static int lunix_chrdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return -EINVAL;
}

static struct file_operations lunix_chrdev_fops = 
{
        .owner          = THIS_MODULE,
	.open           = lunix_chrdev_open,
	.release        = lunix_chrdev_release,
	.read           = lunix_chrdev_read,
	.unlocked_ioctl = lunix_chrdev_ioctl,
	.mmap           = lunix_chrdev_mmap
};

int lunix_chrdev_init(void)
{
	/*
	 * Register the character device with the kernel, asking for
	 * a range of minor numbers (number of sensors * 8 measurements / sensor)
	 * beginning with LINUX_CHRDEV_MAJOR:0
	 */
	int ret;
	dev_t dev_no;
	unsigned int lunix_minor_cnt = lunix_sensor_cnt << 3;
	
	debug("initializing character device\n");
	cdev_init(&lunix_chrdev_cdev, &lunix_chrdev_fops);
	lunix_chrdev_cdev.owner = THIS_MODULE;
	
	dev_no = MKDEV(LUNIX_CHRDEV_MAJOR, 0);
	/* ? */
	/* register_chrdev_region? */
	ret = register_chrdev_region(dev_no, lunix_minor_cnt, "lunix");
	if (ret < 0) {
		debug("failed to register region, ret = %d\n", ret);
		goto out;
	}	
	/* ? */
	/* cdev_add? */


	ret = cdev_add(&lunix_chrdev_cdev, dev_no, lunix_minor_cnt);
	
	
	if (ret < 0) {
		debug("failed to add character device\n");
		goto out_with_chrdev_region;
	}
	debug("completed successfully\n");
	return 0;

out_with_chrdev_region:
	unregister_chrdev_region(dev_no, lunix_minor_cnt);
out:
	return ret;
}

void lunix_chrdev_destroy(void)
{
	dev_t dev_no;
	unsigned int lunix_minor_cnt = lunix_sensor_cnt << 3;
		
	debug("entering\n");
	dev_no = MKDEV(LUNIX_CHRDEV_MAJOR, 0);
	cdev_del(&lunix_chrdev_cdev);
	unregister_chrdev_region(dev_no, lunix_minor_cnt);
	debug("leaving\n");
}
