/*
 * USB Midi driver 1.0
 *
 * Copyright (C) 2016 258633901@qq.com
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <linux/mutex.h>

#define err(format, arg...) printk(format, ##arg)

#define USB_INT_SUB_CLASS_MIDI_STREAM 0X03

#define CS_INTERFACE 0x24
#define MS_HEADER 0x01
#define MIDI_IN_JACK_DESC 0x02
#define MIDI_OUT_JACK_DESC 0x03
#define INTERNAL_JACK 0x01
#define EXTERNAL_JACK 0x02

#define ENDPOINT 0x05
#define CS_ENDPOINT 0x25

#define MS_GENERAL 0x01

#define BULK_ENDPOINT 0x02
#define INTERRUPT_ENDPOINT 0x03


/* ms descriptor */
struct cs_ms_descriptor 
{
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u16  bcdMSC;
	__u16  wTotalLength;
} __attribute__ ((packed));

struct generic_descriptor_header
{
	__u8  bLength;
	__u8  bDescriptorType;
}__attribute__ ((packed));


struct cs_midi_jack_descriptor_header
{
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bJackType;
	__u8  bJackID;
}__attribute__ ((packed));


struct endpoint_descriptor_header
{
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bEndpointAddress;
	__u8  bmAttributes;
	__u16  wMaxPacketSize;
	__u8  bInterval;
	__u8  bRefresh;
	__u8  bSyncAddress;	
}__attribute__ ((packed));

struct cs_endpoint_descriptor_header
{
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bNumEmbMIDIJack;
	
}__attribute__ ((packed));

struct cs_endpoint_descriptor_tail
{
	__u8  BaAssocJackID;
}__attribute__ ((packed));

struct cs_midi_in_jack_descriptor
{
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bJackType;
	__u8  bJackID;
	__u8  iJack;
}__attribute__ ((packed));

struct cs_midi_out_jack_descriptor_header
{
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bJackType;
	__u8  bJackID;
	__u8  bNrInputPin;
}__attribute__ ((packed));

struct cs_midi_out_jack_descriptor_input
{
	__u8  baSourceID;
	__u8  baSourcePin;
	
}__attribute__ ((packed));


struct cs_midi_out_jack_descriptor_tail
{
	__u8  iJack;
}__attribute__ ((packed));




/* table of devices that work with this driver */
static const struct usb_device_id midi_table[] = {
	{ .match_flags=USB_DEVICE_ID_MATCH_INT_CLASS|USB_DEVICE_ID_MATCH_INT_SUBCLASS,
	  .bInterfaceClass=USB_CLASS_AUDIO,
	  .bInterfaceSubClass=USB_INT_SUB_CLASS_MIDI_STREAM,
	 },
	{ }					/* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, midi_table);


/* Get a minor range for your devices from the usb maintainer */
#define USB_MIDI_MINOR_BASE	192

/* our private defines. if this grows any larger, use your own .h file */
#define MAX_TRANSFER		(PAGE_SIZE - 512)
/* MAX_TRANSFER is chosen so that the VM is not stressed by
   allocations > PAGE_SIZE and the number of packets in a page
   is an integer 512 is the largest possible packet on EHCI */
#define WRITES_IN_FLIGHT	8
/* arbitrarily chosen */

/* Structure to hold all of our device specific stuff */
struct usb_midi {
	struct usb_device	*udev;			/* the usb device for this device */
	struct usb_interface	*interface;		/* the interface for this device */
	struct semaphore	limit_sem;		/* limiting the number of writes in progress */
	struct usb_anchor	submitted;		/* in case we need to retract our submissions */
	struct urb		*bulk_in_urb;		/* the urb to read data with */
	unsigned char           *bulk_in_buffer;	/* the buffer to receive data */
	size_t			bulk_in_size;		/* the size of the receive buffer */
	size_t			bulk_in_filled;		/* number of bytes in the buffer */
	size_t			bulk_in_copied;		/* already copied to user space */
	__u8			bulk_in_endpointAddr;	/* the address of the bulk in endpoint */
	__u8			bulk_out_endpointAddr;	/* the address of the bulk out endpoint */
	int			errors;			/* the last request tanked */
	int			open_count;		/* count the number of openers */
	bool			ongoing_read;		/* a read is going on */
	spinlock_t		err_lock;		/* lock for errors */
	struct kref		kref;
	struct mutex		io_mutex;		/* synchronize I/O with disconnect */
	struct mutex		read_mutex;
	struct mutex		write_mutex;
	struct completion	bulk_in_completion;	/* to wait for an ongoing read */
};
#define to_midi_dev(d) container_of(d, struct usb_midi, kref)

static struct usb_driver midi_driver;
static void midi_draw_down(struct usb_midi *dev);

static void midi_delete(struct kref *kref)
{
	struct usb_midi *dev = to_midi_dev(kref);

	usb_free_urb(dev->bulk_in_urb);
	usb_put_dev(dev->udev);
	kfree(dev->bulk_in_buffer);
	kfree(dev);
}

static int midi_open(struct inode *inode, struct file *file)
{
	struct usb_midi *dev;
	struct usb_interface *interface;
	int subminor;
	int retval = 0;

	subminor = iminor(inode);

	interface = usb_find_interface(&midi_driver, subminor);
	if (!interface) {
		err("%s - error, can't find device for minor %d",
		     __func__, subminor);
		retval = -ENODEV;
		goto exit;
	}

	dev = usb_get_intfdata(interface);
	if (!dev) {
		retval = -ENODEV;
		goto exit;
	}

	/* increment our usage count for the device */
	kref_get(&dev->kref);

	/* lock the device to allow correctly handling errors
	 * in resumption */
	mutex_lock(&dev->io_mutex);

	if (!dev->open_count++) {
		retval = usb_autopm_get_interface(interface);
			if (retval) {
				dev->open_count--;
				mutex_unlock(&dev->io_mutex);
				kref_put(&dev->kref, midi_delete);
				goto exit;
			}
	} /* else { //uncomment this block if you want exclusive open
		retval = -EBUSY;
		dev->open_count--;
		mutex_unlock(&dev->io_mutex);
		kref_put(&dev->kref, midi_delete);
		goto exit;
	} */
	/* prevent the device from being autosuspended */

	/* save our object in the file's private structure */
	file->private_data = dev;
	mutex_unlock(&dev->io_mutex);

exit:
	return retval;
}

static int midi_release(struct inode *inode, struct file *file)
{
	struct usb_midi *dev;

	dev = file->private_data;
	if (dev == NULL)
		return -ENODEV;

	/* allow the device to be autosuspended */
	mutex_lock(&dev->io_mutex);
	if (!--dev->open_count && dev->interface)
		usb_autopm_put_interface(dev->interface);
	mutex_unlock(&dev->io_mutex);

	/* decrement the count on our device */
	kref_put(&dev->kref, midi_delete);
	return 0;
}

static int midi_flush(struct file *file, fl_owner_t id)
{
	struct usb_midi *dev;
	int res;

	dev = file->private_data;
	if (dev == NULL)
		return -ENODEV;
	/* wait for io to stop */
	mutex_lock(&dev->io_mutex);
	midi_draw_down(dev);

	/* read out errors, leave subsequent opens a clean slate */
	spin_lock_irq(&dev->err_lock);
	res = dev->errors ? (dev->errors == -EPIPE ? -EPIPE : -EIO) : 0;
	dev->errors = 0;
	spin_unlock_irq(&dev->err_lock);

	mutex_unlock(&dev->io_mutex);

	return res;
}

static void midi_read_bulk_callback(struct urb *urb)
{
	struct usb_midi *dev;

	dev = urb->context;

	spin_lock(&dev->err_lock);
	/* sync/async unlink faults aren't errors */
	if (urb->status) {
		if (!(urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN))
			err("%s - nonzero write bulk status received: %d",
			    __func__, urb->status);

		dev->errors = urb->status;
	} else {
		dev->bulk_in_filled = urb->actual_length;
	}
	dev->ongoing_read = 0;
	spin_unlock(&dev->err_lock);

	complete(&dev->bulk_in_completion);
}

static int midi_do_read_io(struct usb_midi *dev, size_t count)
{
	int rv;
	
	/* prepare a read */
	usb_fill_bulk_urb(dev->bulk_in_urb,
			dev->udev,
			usb_rcvbulkpipe(dev->udev,
				dev->bulk_in_endpointAddr),
			dev->bulk_in_buffer,
			min(dev->bulk_in_size, count),
			midi_read_bulk_callback,
			dev);

	

	/* do it */
	rv = usb_submit_urb(dev->bulk_in_urb, GFP_KERNEL);
	if (rv < 0) {
		err("%s - failed submitting read urb, error %d",
			__func__, rv);
		dev->bulk_in_filled = 0;
		rv = (rv == -ENOMEM) ? rv : -EIO;
		spin_lock_irq(&dev->err_lock);
		dev->ongoing_read = 0;
		spin_unlock_irq(&dev->err_lock);
	}
	
	return rv;
}

static ssize_t midi_read(struct file *file, char *buffer, size_t count,
			 loff_t *ppos)
{
	struct usb_midi *dev;
	int rv;
	dev = file->private_data;

	/* if we cannot read at all, return EOF */
	if (!dev->bulk_in_urb || !count)
		return 0;

	/* no concurrent readers */
	rv = mutex_lock_interruptible(&dev->read_mutex);
	if (rv < 0)
		return rv;

	if (!dev->interface) {		/* disconnect() was called */
		rv = -ENODEV;
		goto exit;
	}


	rv = midi_do_read_io(dev, count);
	if (rv < 0)
		goto exit;

	rv = wait_for_completion_interruptible(&dev->bulk_in_completion);
	if (rv < 0)
		goto exit;

	
	/* errors must be reported */
	rv = dev->errors;
	if (rv < 0) {
		/* any error is reported once */
		dev->errors = 0;
		/* to preserve notifications about reset */
		rv = (rv == -EPIPE) ? rv : -EIO;
		/* no data to deliver */
		dev->bulk_in_filled = 0;
		/* report it */
		goto exit;
	}


	if (dev->bulk_in_filled) {

		if (copy_to_user(buffer,dev->bulk_in_buffer,dev->bulk_in_filled))
		{
			rv = -EFAULT;
			goto exit;
		}
		rv = dev->bulk_in_filled;
					
	} 
	else
	{
		rv = 0;
	}

	
exit:
	mutex_unlock(&dev->read_mutex);
	return rv;
}

static void midi_write_bulk_callback(struct urb *urb)
{
	struct usb_midi *dev;

	dev = urb->context;

	/* sync/async unlink faults aren't errors */
	if (urb->status) {
		if (!(urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN))
			err("%s - nonzero write bulk status received: %d",
			    __func__, urb->status);

		spin_lock(&dev->err_lock);
		dev->errors = urb->status;
		spin_unlock(&dev->err_lock);
	}

	/* free up our allocated buffer */
	usb_free_coherent(urb->dev, urb->transfer_buffer_length,
			  urb->transfer_buffer, urb->transfer_dma);
	up(&dev->limit_sem);
}

static ssize_t midi_write(struct file *file, const char *user_buffer,
			  size_t count, loff_t *ppos)
{
	struct usb_midi *dev;
	int retval = 0;
	struct urb *urb = NULL;
	char *buf = NULL;
	size_t writesize = min(count, (size_t)MAX_TRANSFER);
	dev = file->private_data;

	mutex_lock(&dev->write_mutex);
	

	/* verify that we actually have some data to write */
	if (count == 0)
		goto exit;

	/*
	 * limit the number of URBs in flight to stop a user from using up all
	 * RAM
	 */
	if (!(file->f_flags & O_NONBLOCK)) {
		if (down_interruptible(&dev->limit_sem)) {
			retval = -ERESTARTSYS;
			goto exit;
		}
	} else {
		if (down_trylock(&dev->limit_sem)) {
			retval = -EAGAIN;
			goto exit;
		}
	}
	spin_lock_irq(&dev->err_lock);
	retval = dev->errors;
	if (retval < 0) {
		/* any error is reported once */
		dev->errors = 0;
		/* to preserve notifications about reset */
		retval = (retval == -EPIPE) ? retval : -EIO;
	}
	spin_unlock_irq(&dev->err_lock);
	if (retval < 0)
		goto error;
	/* create a urb, and a buffer for it, and copy the data to the urb */
	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		retval = -ENOMEM;
		goto error;
	}

	buf = usb_alloc_coherent(dev->udev, writesize, GFP_KERNEL,
				 &urb->transfer_dma);
	if (!buf) {
		retval = -ENOMEM;
		goto error;
	}
	if (copy_from_user(buf, user_buffer, writesize)) {
		retval = -EFAULT;
		goto error;
	}

	/* this lock makes sure we don't submit URBs to gone devices */
	
	
	mutex_lock(&dev->io_mutex);
	if (!dev->interface) {		/* disconnect() was called */
		mutex_unlock(&dev->io_mutex);
		retval = -ENODEV;
		goto error;
	}
	/* initialize the urb properly */
	usb_fill_bulk_urb(urb, dev->udev,
			  usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr),
			  buf, writesize, midi_write_bulk_callback, dev);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	usb_anchor_urb(urb, &dev->submitted);
	/* send the data out the bulk port */
	retval = usb_submit_urb(urb, GFP_KERNEL);
	mutex_unlock(&dev->io_mutex);
	if (retval) {
		err("%s - failed submitting write urb, error %d", __func__,
		    retval);
		goto error_unanchor;
	}

	/*
	 * release our reference to this urb, the USB core will eventually free
	 * it entirely
	 */
	usb_free_urb(urb);

	mutex_unlock(&dev->write_mutex);
	return writesize;

error_unanchor:
	usb_unanchor_urb(urb);
error:
	if (urb) {
		usb_free_coherent(dev->udev, writesize, buf, urb->transfer_dma);
		usb_free_urb(urb);
	}
	up(&dev->limit_sem);
exit:
	mutex_unlock(&dev->write_mutex);
	return retval;
}

static const struct file_operations midi_fops = {
	.owner =	THIS_MODULE,
	.read =		midi_read,
	.write =	midi_write,
	.open =		midi_open,
	.release =	midi_release,
	.flush =	midi_flush,
	.llseek =	noop_llseek,
};

/*
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with the driver core
 */
static struct usb_class_driver midi_class = {
	.name =		"USBMidi%d",
	.fops =		&midi_fops,
	.minor_base =	USB_MIDI_MINOR_BASE,
};

static int midi_probe(struct usb_interface *interface,
		      const struct usb_device_id *id)
{
	struct usb_midi *dev;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	size_t buffer_size;
	int i;
	int retval = -ENOMEM;

	iface_desc = interface->cur_altsetting;

	printk("\n----------------midi interface desc--------------\n");
	printk("bLength:0x%02x\n",iface_desc->desc.bLength);
	printk("bDescriptorType:0x%02x\n",iface_desc->desc.bDescriptorType);
	printk("bInterfaceNumber:0x%02x\n",iface_desc->desc.bInterfaceNumber);
	printk("bAlternateSetting:0x%02x\n",iface_desc->desc.bAlternateSetting);
	printk("bNumEndpoints:0x%02x\n",iface_desc->desc.bNumEndpoints);
	printk("bInterfaceClass:0x%02x(%s)\n",iface_desc->desc.bInterfaceClass,(iface_desc->desc.bInterfaceClass==USB_CLASS_AUDIO)?"usb audio":"unkown");
	printk("bInterfaceSubClass:0x%02x(%s)\n",iface_desc->desc.bInterfaceSubClass,(iface_desc->desc.bInterfaceSubClass==USB_INT_SUB_CLASS_MIDI_STREAM)?"midi stream":"unkown");
	printk("bInterfaceProtocol:0x%02x\n",iface_desc->desc.bInterfaceProtocol);
	printk("iInterface:0x%02x\n",iface_desc->desc.iInterface);
	

	if(iface_desc->extralen)
	{
		struct cs_ms_descriptor ms_desc;
		struct generic_descriptor_header generic_desc;
		struct cs_midi_jack_descriptor_header jack_desc_header;
		unsigned char *p_extra=iface_desc->extra;
		memcpy(&ms_desc,p_extra,sizeof(struct cs_ms_descriptor));
		p_extra+=sizeof(struct cs_ms_descriptor);
		printk("----------------midi class specification stream desc--------------\n");
		printk("bLength:0x%02x\n",ms_desc.bLength);
		printk("bDescriptorType:0x%02x(%s)\n",ms_desc.bDescriptorType,(ms_desc.bDescriptorType==CS_INTERFACE)?"CS_INTERFACE":"unkown");
		printk("bDescriptorSubType:0x%02x(%s)\n",ms_desc.bDescriptorSubType,(ms_desc.bDescriptorSubType==MS_HEADER)?"MS_HEADER":"unkown");
		printk("bcdMSC:0x%02x\n",ms_desc.bcdMSC);
		printk("wTotalLength:0x%02x\n",ms_desc.wTotalLength);
		
		while(p_extra<iface_desc->extra+ms_desc.wTotalLength)
		{
			
			memcpy(&generic_desc,p_extra,sizeof(struct generic_descriptor_header));
			if(generic_desc.bDescriptorType==CS_INTERFACE)
			{
				memcpy(&jack_desc_header,p_extra,sizeof(struct cs_midi_jack_descriptor_header));
				if(jack_desc_header.bDescriptorSubType==MIDI_IN_JACK_DESC)
				{
					struct cs_midi_in_jack_descriptor midi_in_jack_desc;
					memcpy(&midi_in_jack_desc,p_extra,sizeof(struct cs_midi_in_jack_descriptor));
					p_extra+=sizeof(struct cs_midi_in_jack_descriptor);
					if(midi_in_jack_desc.bJackType==0x01)
						printk("----------------embbed midi in jack desc --------------\n");
					else if(midi_in_jack_desc.bJackType==0x02)
						printk("----------------external midi in jack desc --------------\n");
					else
						printk("----------------midi in jack desc --------------\n");
					printk("bLength:0x%02x\n",midi_in_jack_desc.bLength);
					printk("bDescriptorType:0x%02x(%s)\n",midi_in_jack_desc.bDescriptorType,(midi_in_jack_desc.bDescriptorType==CS_INTERFACE)?"CS_INTERFACE":"unkown");
					printk("bDescriptorSubType:0x%02x(%s)\n",midi_in_jack_desc.bDescriptorSubType,(midi_in_jack_desc.bDescriptorSubType==MIDI_IN_JACK_DESC)?"MIDI_IN_JACK_DESC":"unkown");
					printk("bJackType:0x%02x(%s)\n",midi_in_jack_desc.bJackType,(midi_in_jack_desc.bJackType==INTERNAL_JACK)?"EMBBED":"EXTERNAL");
					printk("bJackID:0x%02x\n",midi_in_jack_desc.bJackID);
					printk("bJackID:0x%02x\n",midi_in_jack_desc.iJack);
					
					
				}
				else if(jack_desc_header.bDescriptorSubType==MIDI_OUT_JACK_DESC)
				{
					struct cs_midi_out_jack_descriptor_header midi_out_jack_desc_header;
					struct cs_midi_out_jack_descriptor_tail midi_out_jack_desc_tail;
					memcpy(&midi_out_jack_desc_header,p_extra,sizeof(struct cs_midi_out_jack_descriptor_header));
					p_extra+=sizeof(struct cs_midi_out_jack_descriptor_header);
					if(midi_out_jack_desc_header.bJackType==0x01)
						printk("----------------embbed midi out jack desc --------------\n");
					else if(midi_out_jack_desc_header.bJackType==0x02)
						printk("----------------external midi out jack desc --------------\n");
					else
						printk("----------------midi out jack desc --------------\n");
					printk("bLength:0x%02x\n",midi_out_jack_desc_header.bLength);
					printk("bDescriptorType:0x%02x(%s)\n",midi_out_jack_desc_header.bDescriptorType,(midi_out_jack_desc_header.bDescriptorType==CS_INTERFACE)?"CS_INTERFACE":"unkown");
					printk("bDescriptorSubType:0x%02x(%s)\n",midi_out_jack_desc_header.bDescriptorSubType,(midi_out_jack_desc_header.bDescriptorSubType==MIDI_OUT_JACK_DESC)?"MIDI_OUT_JACK_DESC":"unkown");
					printk("bJackType:0x%02x(%s)\n",midi_out_jack_desc_header.bJackType,(midi_out_jack_desc_header.bJackType==INTERNAL_JACK)?"EMBBED":"EXTERNAL");
					printk("bJackID:0x%02x\n",midi_out_jack_desc_header.bJackID);
					printk("bNrInputPin:0x%02x\n",midi_out_jack_desc_header.bNrInputPin);
					for(i=0;i<midi_out_jack_desc_header.bNrInputPin;i++)
					{
						struct cs_midi_out_jack_descriptor_input midi_out_jack_desc_input;
						memcpy(&midi_out_jack_desc_input,p_extra,sizeof(struct cs_midi_out_jack_descriptor_input));
						p_extra+=sizeof(struct cs_midi_out_jack_descriptor_input);
						printk("baSourceID:0x%02x\n",midi_out_jack_desc_input.baSourceID);
						printk("baSourcePin:0x%02x\n",midi_out_jack_desc_input.baSourcePin);
						
					}

					memcpy(&midi_out_jack_desc_tail,p_extra,sizeof(struct cs_midi_out_jack_descriptor_tail));
					p_extra+=sizeof(struct cs_midi_out_jack_descriptor_tail);
					printk("iJack:0x%02x\n",midi_out_jack_desc_tail.iJack);
				}
				else
				{
					printk("unkown jack desc type\n");
					printk("-------------------begin dump------------------------\n");
					for(i=0;i<(iface_desc->extra+ms_desc.wTotalLength-p_extra);i++)
					{
						printk("0x%02x ",p_extra[i]);
					}
					printk("\n---------------------end dump------------------------\n");
					
					return retval;
				}
			}
			else if(generic_desc.bDescriptorType==ENDPOINT)
			{
				struct endpoint_descriptor_header ep_desc;
				char ep_dir[64]={0};
				char ep_attrib[64]={0};
				memcpy(&ep_desc,p_extra,sizeof(struct endpoint_descriptor_header));
				p_extra+=sizeof(struct endpoint_descriptor_header);
				if((ep_desc.bEndpointAddress&0x80)==0x80)
					strcpy(ep_dir,"input");
				else
					strcpy(ep_dir,"output");

				if(ep_desc.bmAttributes==BULK_ENDPOINT)
					strcpy(ep_attrib,"bulk");
				else if(ep_desc.bmAttributes==INTERRUPT_ENDPOINT)
					strcpy(ep_attrib,"interrupt");
				else
					strcpy(ep_attrib,"iso");
								
				printk("----------------%s %s endpoint descriptor --------------\n",ep_dir,ep_attrib);
				printk("bLength:0x%02x\n",ep_desc.bLength);
				printk("bDescriptorType:0x%02x(%s)\n",ep_desc.bDescriptorType,(ep_desc.bDescriptorType==ENDPOINT)?"ENDPOINT":"unknown");
				printk("bEndpointAddress:0x%02x(%s)\n",ep_desc.bEndpointAddress,ep_dir);
				printk("bmAttributes:0x%02x(%s)\n",ep_desc.bmAttributes,ep_attrib);
				printk("wMaxPacketSize:0x%02x\n",ep_desc.wMaxPacketSize);
				printk("bInterval:0x%02x\n",ep_desc.bInterval);
				printk("bRefresh:0x%02x\n",ep_desc.bRefresh);
				printk("bSyncAddress:0x%02x\n",ep_desc.bSyncAddress);
		
			}
			else if(generic_desc.bDescriptorType==CS_ENDPOINT)
			{
				struct cs_endpoint_descriptor_header cs_ep_desc;
				memcpy(&cs_ep_desc,p_extra,sizeof(struct cs_endpoint_descriptor_header));
				p_extra+=sizeof(struct cs_endpoint_descriptor_header);
				printk("----------------cs endpoint descriptor --------------\n");
				printk("bLength:0x%02x\n",cs_ep_desc.bLength);
				printk("bDescriptorType:0x%02x(%s)\n",cs_ep_desc.bDescriptorType,(cs_ep_desc.bDescriptorType==CS_ENDPOINT)?"CS_ENDPOINT":"unknown");
				printk("bDescriptorSubType:0x%02x(%s)\n",cs_ep_desc.bDescriptorSubType,(cs_ep_desc.bDescriptorSubType==MS_GENERAL)?"MS_GENERAL":"unknown");
				printk("bNumEmbMIDIJack:0x%02x\n",cs_ep_desc.bNumEmbMIDIJack);
				for(i=0;i<cs_ep_desc.bNumEmbMIDIJack;i++)
				{
					struct cs_endpoint_descriptor_tail cs_ep_desc_tail;
					memcpy(&cs_ep_desc_tail,p_extra,sizeof(struct cs_endpoint_descriptor_tail));
					p_extra+=sizeof(struct cs_endpoint_descriptor_tail);
					printk("BaAssocJackID:0x%02x\n",cs_ep_desc_tail.BaAssocJackID);
				}
							
			}
			else
			{
				printk("unknown descriptor:0x%02x\n",generic_desc.bDescriptorType);
			}
		}
		
		
		
	
		
		
	}
	else
	{
		return retval;
	}

	


	/* allocate memory for our device state and initialize it */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		err("Out of memory");
		goto error;
	}
	kref_init(&dev->kref);
	sema_init(&dev->limit_sem, WRITES_IN_FLIGHT);
	mutex_init(&dev->io_mutex);
	mutex_init(&dev->read_mutex);
	mutex_init(&dev->write_mutex);
	spin_lock_init(&dev->err_lock);
	init_usb_anchor(&dev->submitted);
	init_completion(&dev->bulk_in_completion);

	dev->udev = usb_get_dev(interface_to_usbdev(interface));
	dev->interface = interface;

	/* set up the endpoint information */
	/* use only the first bulk-in and bulk-out endpoints */

	
	
	

	
	

	
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		if (!dev->bulk_in_endpointAddr &&
		    usb_endpoint_is_bulk_in(endpoint)) {
			/* we found a bulk in endpoint */
			buffer_size = usb_endpoint_maxp(endpoint);
			dev->bulk_in_size = buffer_size;
			dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
			dev->bulk_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
			if (!dev->bulk_in_buffer) {
				err("Could not allocate bulk_in_buffer");
				goto error;
			}
			dev->bulk_in_urb = usb_alloc_urb(0, GFP_KERNEL);
			if (!dev->bulk_in_urb) {
				err("Could not allocate bulk_in_urb");
				goto error;
			}
		}

		if (!dev->bulk_out_endpointAddr &&
		    usb_endpoint_is_bulk_out(endpoint)) {
			/* we found a bulk out endpoint */
			dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
		}
	}
	if (!(dev->bulk_in_endpointAddr && dev->bulk_out_endpointAddr)) {
		err("Could not find both bulk-in and bulk-out endpoints");
		goto error;
	}

	/* save our data pointer in this interface device */
	usb_set_intfdata(interface, dev);

	/* we can register the device now, as it is ready */
	retval = usb_register_dev(interface, &midi_class);
	if (retval) {
		/* something prevented us from registering this driver */
		err("Not able to get a minor for this device.");
		usb_set_intfdata(interface, NULL);
		goto error;
	}

	/* let the user know what node this device is now attached to */
	dev_info(&interface->dev,
		 "USB midi device now attached to USBMidi%d",
		 interface->minor);
	return 0;

error:
	if (dev)
		/* this frees allocated memory */
		kref_put(&dev->kref, midi_delete);
	return retval;
}

static void midi_disconnect(struct usb_interface *interface)
{
	struct usb_midi *dev;
	int minor = interface->minor;

	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);

	/* give back our minor */
	usb_deregister_dev(interface, &midi_class);

	/* prevent more I/O from starting */
	mutex_lock(&dev->io_mutex);
	dev->interface = NULL;
	mutex_unlock(&dev->io_mutex);

	usb_kill_anchored_urbs(&dev->submitted);

	/* decrement our usage count */
	kref_put(&dev->kref, midi_delete);

	dev_info(&interface->dev, "USB midi #%d now disconnected", minor);
}

static void midi_draw_down(struct usb_midi *dev)
{
	int time;

	time = usb_wait_anchor_empty_timeout(&dev->submitted, 1000);
	if (!time)
		usb_kill_anchored_urbs(&dev->submitted);
	usb_kill_urb(dev->bulk_in_urb);
}

static int midi_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct usb_midi *dev = usb_get_intfdata(intf);

	if (!dev)
		return 0;
	midi_draw_down(dev);
	return 0;
}

static int midi_resume(struct usb_interface *intf)
{
	return 0;
}

static int midi_pre_reset(struct usb_interface *intf)
{
	struct usb_midi *dev = usb_get_intfdata(intf);

	mutex_lock(&dev->io_mutex);
	midi_draw_down(dev);

	return 0;
}

static int midi_post_reset(struct usb_interface *intf)
{
	struct usb_midi *dev = usb_get_intfdata(intf);

	/* we are sure no URBs are active - no locking needed */
	dev->errors = -EPIPE;
	mutex_unlock(&dev->io_mutex);

	return 0;
}

static struct usb_driver midi_driver = {
	.name =		"usb_midi",
	.probe =	midi_probe,
	.disconnect =	midi_disconnect,
	.suspend =	midi_suspend,
	.resume =	midi_resume,
	.pre_reset =	midi_pre_reset,
	.post_reset =	midi_post_reset,
	.id_table =	midi_table,
	.supports_autosuspend = 1,
};

static int __init usb_midi_init(void)
{
	int result;

	/* register this driver with the USB subsystem */
	result = usb_register(&midi_driver);
	if (result)
		err("usb_register failed. Error number %d", result);

	return result;
}

static void __exit usb_midi_exit(void)
{
	/* deregister this driver with the USB subsystem */
	usb_deregister(&midi_driver);
}

module_init(usb_midi_init);
module_exit(usb_midi_exit);

MODULE_LICENSE("GPL");
