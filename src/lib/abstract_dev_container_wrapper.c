/*   
 * This file is part of cf4ocl (C Framework for OpenCL).
 * 
 * cf4ocl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation, either version 3 of the 
 * License, or (at your option) any later version.
 * 
 * cf4ocl is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with cf4ocl. If not, see 
 * <http://www.gnu.org/licenses/>.
 * */
 
/** 
 * @file
 * @brief Functions for obtaining information about OpenCL entities
 * such as platforms, devices, contexts, queues, kernels, etc.
 * 
 * @author Nuno Fachada
 * @date 2014
 * @copyright [GNU Lesser General Public License version 3 (LGPLv3)](http://www.gnu.org/licenses/lgpl.html)
 * */
 
#include "abstract_dev_container_wrapper.h"

static void ccl_dev_container_init_devices(CCLDevContainer* devcon, 
	ccl_dev_container_get_cldevices get_devices, GError **err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);
	
	/* Make sure devcon is not NULL. */
	g_return_val_if_fail(devcon != NULL, NULL);
	
	/* Make sure device list is not initialized. */
	g_return_val_if_fail(devcon->devices == NULL, NULL);

	CCLWrapperInfo* info_devs;
	GError* err_internal = NULL;

	/* Get device IDs. */
	info_devs = get_devices(devcon, &err_internal);
	gef_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine number of devices. */
	devcon->num_devices = info_devs->size / sizeof(cl_device_id);

	/* Allocate memory for array of device wrapper objects. */
	devcon->devices = g_slice_alloc(
		devcon->num_devices * sizeof(CCLDevice*));

	/* Wrap device IDs in device wrapper objects. */
	for (guint i = 0; i < devcon->num_devices; ++i) {

		/* Add device wrapper object to array of wrapper objects. */
		devcon->devices[i] = ccl_device_new_wrap(
			((cl_device_id*) info_devs->value)[i]);
	}

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	
finish:		
	
	/* Terminate function. */
	return;

}

/**
 * @brief Release the devices held by the given #CCLDevContainer object.
 * 
 * @param devcon A ::CCLDevContainer wrapper object.
 * */
void ccl_dev_container_release_devices(CCLDevContainer* devcon) {

	/* Make sure devcon wrapper object is not NULL. */
	g_return_if_fail(devcon != NULL);

	/* Check if any devices are associated with this device 
	 * container. */
	if (devcon->devices != NULL) {

		/* Release devices in device container. */
		for (guint i = 0; i < devcon->num_devices; ++i) {
			if (devcon->devices[i])
				ccl_device_unref(devcon->devices[i]);
		}
		
		/* Free device wrapper array. */
		g_slice_free1(devcon->num_devices * sizeof(CCLDevice*), 
			devcon->devices);
	}

}

/** 
 * @brief Get all ::CCLDevice wrappers in device container. 
 * 
 * @param devcon The device container object.
 * @param get_devices Function to get cl_device_id's from wrapped
 * object.
 * @param err Return location for a GError, or NULL if error reporting 
 * is to be ignored.
 * @return All ::CCLDevice wrappers in device container or NULL if an 
 * error occurs.
 * */
const CCLDevice** ccl_dev_container_get_all_devices(
	CCLDevContainer* devcon, 
	ccl_dev_container_get_cldevices get_devices, GError** err) {
		
	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, 0);
	
	/* Make sure devcon is not NULL. */
	g_return_val_if_fail(devcon != NULL, 0);
	
	/* Check if device list is already initialized. */
	if (devcon->devices == NULL) {
		
		/* Not initialized, initialize it. */
		ccl_dev_container_init_devices(devcon, get_devices, err);
		
	}
	
	/* Return all devices in platform. */
	return (const CCLDevice**) devcon->devices;

}


/** 
 * @brief Get ::CCLDevice wrapper at given index. 
 * 
 * @param devcon The device container object.
 * @param get_devices Function to get cl_device_id's from wrapped
 * object.
 * @param index Index of device in device container.
 * @param err Return location for a GError, or NULL if error reporting 
 * is to be ignored.
 * @return The ::CCLDevice wrapper at given index or NULL if an error 
 * occurs.
 * */
CCLDevice* ccl_dev_container_get_device(
	CCLDevContainer* devcon, 
	ccl_dev_container_get_cldevices get_devices, cl_uint index, 
	GError** err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);
	
	/* Make sure devcon is not NULL. */
	g_return_val_if_fail(devcon != NULL, NULL);
	
	/* The return value. */
	CCLDevice* device_ret;
	
	/* Internal error object. */
	GError* err_internal = NULL;
	
	/* Check if device list is already initialized. */
	if (devcon->devices == NULL) {
		
		/* Not initialized, initialize it. */
		ccl_dev_container_init_devices(
			devcon, get_devices, &err_internal);
		
		/* Check for errors. */
		gef_if_err_propagate_goto(err, err_internal, error_handler);
		
	}
	
	/* Make sure device index is less than the number of devices. */
	gef_if_err_create_goto(*err, CCL_ERROR, index >= devcon->num_devices, 
		CCL_ERROR_DEVICE_NOT_FOUND, error_handler, 
		"%s: device index (%d) out of bounds (%d devices in list).",
		 G_STRLOC, index, devcon->num_devices);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	device_ret = devcon->devices[index];
	goto finish;
	
error_handler:

	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	device_ret = NULL;
	
finish:		
	
	/* Return list of device wrappers. */
	return device_ret;

}

/**
 * @brief Return number of devices in device container.
 * 
 * @param devcon The device container object.
 * @param get_devices Function to get cl_device_id's from wrapped
 * object.
 * @param err Return location for a GError, or NULL if error reporting 
 * is to be ignored.
 * @return The number of devices in device container or 0 if an error 
 * occurs or is otherwise not possible to get any device.
 * */
cl_uint ccl_dev_container_get_num_devices(
	CCLDevContainer* devcon, 
	ccl_dev_container_get_cldevices get_devices, GError** err) {
	
	/* Make sure devcon is not NULL. */
	g_return_val_if_fail(devcon != NULL, 0);
	
	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, 0);

	/* Check if device list is already initialized. */
	if (devcon->devices == NULL) {
		
		/* Not initialized, initialize it. */
		ccl_dev_container_init_devices(devcon, get_devices, err);

	}
	
	/* Return the number of devices in context. */
	return devcon->num_devices;
	
}

