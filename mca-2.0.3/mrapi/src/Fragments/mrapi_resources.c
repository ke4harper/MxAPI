/************************************************************************
mrapi_resources_get

SYNOPSIS
mrapi_resource_t* mrapi_resources_get(
        MRAPI_IN mrapi_rsrc_filter_t subsystem_filter,
        MRAPI_OUT mrapi_status_t* status
);

DESCRIPTION
mrapi_resources_get() returns a tree of system resources available to the calling node, at the point in time when it is called (this is dynamic in nature).  mrapi_resource_get_attribute() can be used to make a specific query of an attribute of a specific system resource. subsystem_filter is an enumerated type that is used as a filter indicating the scope of the desired information MRAPI returns.  See Section 3.6.1 for a description of how to navigate the resource tree as well as section 6.1 for an example use case.

The valid subsystem filters are:
        MRAPI_RSRC_MEM, MRAPI_RSRC_CACHE, MRAPI_RSRC_CPU

RETURN VALUE
On success, returns a pointer to the root of a tree structure containing the available system resources, and *status is set to MRAPI_SUCCESS.  On error, MRAPI_NULL is returned and *status is set to the appropriate error defined below.  The memory associated with the data structures returned by this function is system managed and must be released via a call to mrapi_resource_tree_free().

ERRORS
MRAPI_ERR_RSRC_INVALID_SUBSYSTEM        Argument is not a valid subsystem enum value.

***********************************************************************/
mrapi_resource_t* mrapi_resources_get(
                                      MRAPI_IN mrapi_rsrc_filter_t subsystem_filter,
                                      MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
    return NULL;
  } else {
    return mrapi_impl_resources_get(subsystem_filter, status);
  }
}


/************************************************************************
mrapi_resource_get_attribute

SYNOPSIS
void mrapi_resource_get_attribute(
        MRAPI_IN mrapi_resource_t* resource,
        MRAPI_IN mrapi_uint_t attribute_num,
        MRAPI_OUT void* attribute,
        MRAPI_IN size_t attribute_size,
        MRAPI_OUT mrapi_status_t* status
);

DESCRIPTION
mrapi_resource_get_attribute() returns the attribute value at the point in time when this function is called (the value of an attribute may be dynamic in nature), given the input resource and attribute number.  resource is a pointer to the respective resource, attribute_num is the number of the attribute to query for that resource, and attribute_size is the size of the attribute.  Resource attributes are read-only.  Attribute numbers are assigned by the MRAPI implementation and are specific to the given resource type (see Section 3.6.1).

The tables below show the valid attribute_nums for each type of resource:

type of mrapi_resource_t = MRAPI_RSRC_MEM
attribute_num:  datatype:
MRAPI_RSRC_MEM_BASEADDR mrapi_addr_t
MRAPI_RSRC_MEM_WORDSIZE mrapi_uint_t
MRAPI_RSRC_MEM_NUMWORDS mrapi_uint_t

type of mrapi_resource_t = MRAPI_RSRC_CACHE
attribute_num:  datatype:
TO BE FILLED IN TO BE FILLED IN

type of mrapi_resource_t = MRAPI_RSRC_CPU
attribute_num:  datatype:
TO BE FILLED IN TO BE FILLED IN

RETURN VALUE
On success *status is set to MRAPI_SUCCESS and the attribute value is filled in.  On error, *status is set to the appropriate error defined below and the attribute value is undefined.  The attribute identified by the attribute_num is returned in the void* attribute parameter.

ERRORS
MRAPI_ERR_RSRC_INVALID  Invalid resource
MRAPI_ERR_ATTR_NUM Unknown attribute number
MRAPI_ERR_ATTR_SIZE Incorrect attribute size
MRAPI_ERR_PARAMETER  Invlid attribute parameter.

***********************************************************************/
void mrapi_resource_get_attribute(
 	MRAPI_IN mrapi_resource_t* resource,
 	MRAPI_IN mrapi_uint_t attribute_number,
 	MRAPI_OUT void* attribute_value,
 	MRAPI_IN size_t attr_size,
 	MRAPI_OUT mrapi_status_t* status)
{
  mrapi_boolean_t is_static;
  mrapi_boolean_t has_started;
  mrapi_boolean_t get_success;

  /* Check for error conditions */

   
   if  (!mrapi_impl_initialized()) {
     *status = MRAPI_ERR_NODE_NOTINIT;
   } else if (mrapi_impl_valid_attribute_number(resource, attribute_number) == MRAPI_FALSE) {
     *status = MRAPI_ERR_ATTR_NUM;
   } else {
     /* Check if this attribute is dynamic and if has been stated */
     has_started = *(resource->attribute_started[attribute_number]);
     is_static = mrapi_impl_is_static(resource, attribute_number);
     if (is_static == MRAPI_FALSE && has_started == MRAPI_FALSE) {
       *status = MRAPI_ERR_RSRC_NOTSTARTED;
       return;
     }
     
     get_success = mrapi_impl_resource_get_attribute(resource,
                                                     attribute_number,
                                                     attribute_value,
                                                     attr_size,
                                                     status);
     assert(get_success == MRAPI_TRUE);
   }
}
 
/************************************************************************
mrapi_dynamic_attribute_start

DESCRIPTION
mrapi_dynamic_attribute_start() sets the system up to begin collection 
of the attribute's value over time.  resource is a pointer to 
the given resource, attribute_num is the number of the attribute 
to start monitoring for that resource.  Attribute numbers are 
specific to the given resource type.
The rollover_callback is an optional function pointer.  If supplied 
the implementation will call the function when the specified 
attribute value rolls over from its maximum value.  If this callback 
is not supplied the attribute will roll over silently.

If you call stop and then start again, the resource will start 
at it's previous value.  To reset it, call mrapi_dynamic_attribute_reset().


RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below.

ERRORS
MRAPI_ERR_RSRC_INVALID Invalid resource
MRAPI_ERR_ATTR_NUM Invalid attribute number
MRAPI_ERR_RSRC_NOTDYNAMIC
The input attribute is static and not dynamic, and therefore can't be started.
MRAPI_ERR_RSRC_STARTED
The attribute is dynamic and has already been startedMRAPI_ERR_RSRC_COUNTER_INUSE	
The counter is currently in use by another node.

NOTE

***********************************************************************/
void mrapi_dynamic_attribute_start(
 	MRAPI_IN mrapi_resource_t* resource,
 	MRAPI_IN mrapi_uint_t attribute_number,
	MRAPI_FUNCTION_PTR void (*rollover_callback) (void),
 	MRAPI_OUT mrapi_status_t* status)
{
  mrapi_boolean_t get_success;

  /* Error checking */

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_valid_attribute_number(resource, attribute_number) == MRAPI_FALSE) {
    *status = MRAPI_ERR_ATTR_NUM;
  } else if (mrapi_impl_is_static(resource, attribute_number) == MRAPI_TRUE) {
    *status = MRAPI_ERR_RSRC_NOTDYNAMIC;
  } else {
    
    get_success = mrapi_impl_dynamic_attribute_start(resource, 
                                                     attribute_number,
                                                     rollover_callback,
                                                     status);
    assert(get_success == MRAPI_TRUE);
  }
}

/************************************************************************
mrapi_dynamic_attribute_reset

DESCRIPTION
mrapi_dynamic_attribute_reset() resets the value of the collected 
dynamic attribute.  resource is the given resource, attribute_num 
is the number of the attribute to reset.  Attribute numbers are 
specific to a given resource type.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_RSRC_INVALID Invalid resource
MRAPI_ERR_ATTR_NUM Invalid attribute number
MRAPI_ERR_RSRC_NOTDYNAMIC
The input attribute is static and not dynamic, and therefore can't be reset.MRAPI_ERR_RSRC_NOTSTARTED
The attribute is not currently started by the calling node.


NOTE
Some dynamic attributes do not have a defined reset value.  In 
this case, calling mrapi_dynamic_attribute_reset() has no effect.


***********************************************************************/
void mrapi_dynamic_attribute_reset(
 	MRAPI_IN mrapi_resource_t *resource,
 	MRAPI_IN mrapi_uint_t attribute_number,
 	MRAPI_OUT mrapi_status_t* status)
{
  mrapi_boolean_t get_success;
  
  /* Error checking */

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_valid_attribute_number(resource, attribute_number) == MRAPI_FALSE) {
    *status = MRAPI_ERR_ATTR_NUM;
  } else if (mrapi_impl_is_static(resource, attribute_number) == MRAPI_TRUE) {
    *status = MRAPI_ERR_RSRC_NOTDYNAMIC;
  } else {
    get_success = mrapi_impl_dynamic_attribute_reset(resource, attribute_number, status);
    assert(get_success == MRAPI_TRUE);
  }
}


/************************************************************************
mrapi_dynamic_attribute_stop

DESCRIPTION
mrapi_dynamic_attribute_stop() stops the system from collecting 
dynamic attribute values.  resource is the given resource, attribute_num 
is the number of the attribute to stop monitoring.  Attribute 
numbers are specific to a given resource type.  If you call stop 
and then start again, the resource will start at it's previous 
value.  To reset it, call mrapi_dynamic_attribute_reset().

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_RSRC_INVALID Invalid resource
MRAPI_ERR_ATTR_NUM Invalid attribute number
MRAPI_ERR_RSRC_NOTDYNAMIC
The input attribute is static and not dynamic, and therefore can't be stopped.MRAPI_ERR_RSRC_NOTSTARTED
The attribute is dynamic and has not been started by the calling node.


NOTE

***********************************************************************/
void mrapi_dynamic_attribute_stop(
 	MRAPI_IN mrapi_resource_t* resource,
 	MRAPI_IN mrapi_uint_t attribute_number,
 	MRAPI_OUT mrapi_status_t* status)
{
  mrapi_boolean_t get_success;

  /* Error checking */

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_valid_attribute_number(resource, attribute_number) == MRAPI_FALSE) {
    *status = MRAPI_ERR_ATTR_NUM;
  } else if (mrapi_impl_is_static(resource, attribute_number) == MRAPI_TRUE) {
    *status = MRAPI_ERR_RSRC_NOTDYNAMIC;
  } else {   
    get_success = mrapi_impl_dynamic_attribute_stop(resource, attribute_number, status);
    assert(get_success == MRAPI_TRUE);
  }
}

/************************************************************************
mrapi_resource_register_callback

DESCRIPTION
mrapi_register_callback() registers an  application-defined function 
to be called when a  specific system event occurs.  The set of 
available events is implementation defined.  Some implementations 
may choose not to define any events and thus not to support this 
functionality.  The frequency parameter is used to indicate the 
reporting frequency for which which an event should trigger the 
callback (frequency is specified in terms of number of event 
occurrences, e.g., callback on every nth occurrence where n=frequency). 
 An example usage of
mrapi_register_callback() could be for notification when the 
core experiences a power management event so that the application 
can determine the cause (manual or automatic) and/or the level 
(nap, sleep, or doze, etc.), and use this information to adjust 
resource usages. 

RETURN VALUE
On success, the callback_function() will be registered for the 
event, and *status is set to MRAPI_SUCCESS.  On error, *status 
is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_RSRC_INVALID_EVENT	Invalid event
MRAPI_ERR_RSRC_INVALID_CALLBACK	Invalid callback function

NOTE

***********************************************************************/
void mrapi_resource_register_callback(
        MRAPI_IN mrapi_event_t event,
        MRAPI_IN unsigned int frequency,
        MRAPI_FUNCTION_PTR void (*callback_function) (mrapi_event_t event),
        MRAPI_OUT mrapi_status_t* status)
{
  /* Cast away the const */
/*   void (*callback_function_nonconst) (mrapi_event_t event); */
/*   callback_function_nonconst = (void (*) (mrapi_event_t event)) callback_function; */

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else {
    mrapi_impl_resource_register_callback(event, frequency,
                                          callback_function, status);
  }
 }

/************************************************************************
mrapi_resource_tree_free

DESCRIPTION
mrapi_resource_tree_free() frees the memory in a resource tree. 
 root is the root of a resource tree originally obtained from 
a call to mrapi_resources_get().

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS and root will be 
set to MRAPI_NULL.  On error, *status is set to the appropriate 
error defined below.

ERRORS
MRAPI_ERR_RSRC_INVALID_TREE Invalid resource tree
MRAPI_ERR_RSRC_NOTOWNER	The calling node is not the same node that originally called mrapi_resources_get().


NOTE
Subsequent usage of root will give undefined results.

***********************************************************************/
void mrapi_resource_tree_free(
 	mrapi_resource_t* MRAPI_IN * root_ptr,
 	MRAPI_OUT mrapi_status_t* status)
{
#if !(__unix__||__MINGW32__)
  mrapi_boolean_t get_success;
#endif  /* !(__unix__||__MINGW32__) */

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else {
#if (__unix__||__MINGW32__)
    (void) mrapi_impl_resource_tree_free(root_ptr, status);
#else
    get_success = mrapi_impl_resource_tree_free(root_ptr, status);
#endif  /* !(__unix__||__MINGW32__) */
  }
}
