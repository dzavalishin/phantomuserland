/* ++++++++++
	driver.c
	A skeletal device driver
+++++ */

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>

/* ----------
	init_hardware - called once the first time the driver is loaded
----- */
status_t
init_hardware (void)
{
	return B_OK;
}


/* ----------
	init_driver - optional function - called every time the driver
	is loaded.
----- */
status_t
init_driver (void)
{
	return B_OK;
}


/* ----------
	uninit_driver - optional function - called every time the driver
	is unloaded
----- */
void
uninit_driver (void)
{
}

	
/* ----------
	my_device_open - handle open() calls
----- */

static status_t
my_device_open (const char *name, uint32 flags, void** cookie)
{
	return B_OK;
}


/* ----------
	my_device_read - handle read() calls
----- */

static status_t
my_device_read (void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was read */
	return B_IO_ERROR;
}


/* ----------
	my_device_write - handle write() calls
----- */

static status_t
my_device_write (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was written */
	return B_IO_ERROR;
}


/* ----------
	my_device_control - handle ioctl calls
----- */

static status_t
my_device_control (void* cookie, uint32 op, void* arg, size_t len)
{
	return B_BAD_VALUE;
}


/* ----------
	my_device_close - handle close() calls
----- */

static status_t
my_device_close (void* cookie)
{
	return B_OK;
}


/* -----
	my_device_free - called after the last device is closed, and after
	all i/o is complete.
----- */
static status_t
my_device_free (void* cookie)
{
	return B_OK;
}


/* -----
	null-terminated array of device names supported by this driver
----- */

static const char *my_device_name[] = {
	"bogus/my_device",
	NULL
};

/* -----
	function pointers for the device hooks entry points
----- */

device_hooks my_device_hooks = {
	my_device_open, 			/* -> open entry point */
	my_device_close, 			/* -> close entry point */
	my_device_free,			/* -> free cookie */
	my_device_control, 		/* -> control entry point */
	my_device_read,			/* -> read entry point */
	my_device_write			/* -> write entry point */
};

/* ----------
	publish_devices - return a null-terminated array of devices
	supported by this driver.
----- */

const char**
publish_devices()
{
	return my_device_name;
}

/* ----------
	find_device - return ptr to device hooks structure for a
	given device name
----- */

device_hooks*
find_device(const char* name)
{
	return &my_device_hooks;
}

