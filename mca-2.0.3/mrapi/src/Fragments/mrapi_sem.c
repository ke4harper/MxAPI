/************************************************************************
mrapi_sem_create

DESCRIPTION
This function creates a semaphore.  Unless you want the defaults, 
attributes must be set before the call to mrapi_sem_create(). 
 Once a semaphore has been created, its attributes may not be 
changed.  If the attributes are NULL, then implementation defined 
default attributes will be used.  If sem_id is set to MRAPI_SEM_ID_ANY, 
then MRAPI will choose an internal id for you.  The shared_lock_limit 
parameter indicates the maximum number of available locks and 
it must be between 0 and MRAPI_MAX_SEM_SHAREDLOCKS.

RETURN VALUE
On success a semaphore handle is returned and *status is set 
to MRAPI_SUCCESS.  On error, *status is set to the appropriate 
error defined below.   In the case where the semaphore already 
exists, status will be set to MRAPI_EXISTS and the handle returned 
will not be a valid handle.

ERRORS
MRAPI_ERR_SEM_INVALID
The semaphore_id is not a valid semaphore id.
MRAPI_ERR_SEM_EXISTS	This semaphore is already created.
MRAPI_ERR_SEM_LIMIT
Exceeded maximum number of semaphores allowed.
MRAPI_ERR_SEM_LOCKLIMIT	The shared lock limit is out of bounds.
MRAPI_ERR_NODE_NOTINIT
The calling node is not initialized.
MRAPI_ERR_PARAMETER
Invalid attributes parameter.

NOTE

***********************************************************************/
mrapi_sem_hndl_t mrapi_sem_create(
 	MRAPI_IN mrapi_sem_id_t sem_id,
 	MRAPI_IN mrapi_sem_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t shared_lock_limit,
 	MRAPI_OUT mrapi_status_t* status)
{

  mrapi_sem_hndl_t sem_hndl = 0;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT; /* fixme: how to handle domain_notinit? */
  } else if (!mrapi_impl_sem_validID(sem_id)) {
    *status = MRAPI_ERR_SEM_ID_INVALID;
  }else if ((shared_lock_limit == 0) || (shared_lock_limit > MRAPI_MAX_SHARED_LOCKS)) {
    *status = MRAPI_ERR_SEM_LOCKLIMIT;
  } else if (mrapi_impl_sem_create(&sem_hndl,sem_id,attributes,shared_lock_limit,status)){
    *status = MRAPI_SUCCESS;
  }
  return sem_hndl;
}

/************************************************************************
mrapi_sem_init_attributes

DESCRIPTION
Unless you want the defaults, this function should be called 
to initialize the values of an mrapi_sem_attributes_t structure 
prior to mrapi_sem_set_attribute().  You would then use mrapi_sem_set_attribute() 
to change any default values prior to calling mrapi_sem_create().


RETURN VALUE
On success *status is set to MRAPI_SUCCESS.  On error, *status is set to 
the appropriate error defined below.

ERRORS
MRAPI_ERR_PARAMETER Invalid attributes parameter.

NOTE

***********************************************************************/
void mrapi_sem_init_attributes(
                                 MRAPI_OUT mrapi_sem_attributes_t* attributes,
                                 MRAPI_OUT mrapi_status_t* status)
{
  *status = MRAPI_SUCCESS;
   if (attributes == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else {
    mrapi_impl_sem_init_attributes(attributes);
  }
}

/************************************************************************
mrapi_sem_set_attribute

DESCRIPTION
This function is used to change default values of an mrapi_sem_attributes_t 
data structure prior to calling mrapi_sem_create().  Calls to 
this function have no effect on semaphore attributes once the 
semaphore has been created.

MRAPI pre-defined semaphore attributes:
Attribute num:	Description:	Datatype:	Default:
MRAPI_ERROR_EXT	Indicates whether or not this semaphore has extended 
error checking enabled.  	mrapi_boolean_t	MRAPI_FALSE
MRAPI_DOMAIN_SHARED	Indicates whether or not this semaphore is shareable across domains.  	
  mrapi_boolean_t	MRAPI_TRUE


RETURN VALUE
On success *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below.

ERRORS
MRAPI_ERR_ATTR_READONLY Attribute can not be modified.
MRAPI_ERR_PARAMETER Invalid attribute parameter.
MRAPI_ERR_ATTR_NUM Unknown attribute number
MRAPI_ERR_ATTR_SIZE Incorrect attribute size


NOTE

***********************************************************************/
void mrapi_sem_set_attribute(
 	MRAPI_OUT mrapi_sem_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_IN void* attribute,
 	MRAPI_IN size_t attr_size,
 	MRAPI_OUT mrapi_status_t* status)
{
  if (attributes == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else {
    mrapi_impl_sem_set_attribute(attributes,attribute_num,attribute,attr_size,status);
  }
}

/************************************************************************
mrapi_sem_get_attribute

DESCRIPTION
Returns the attribute that corresponds to the given attribute_num 
for this semaphore.  The attribute may be viewed but may not 
be changed (for this semaphore).

RETURN VALUE
On success *status is set to MRAPI_SUCCESS and the attribute 
value is filled in.  On error, *status is set to the appropriate 
error defined below and the attribute value is undefined.  The 
attribute identified by the attribute_num is returned in the 
void* attribute parameter.  When extended error checking is enabled, 
if this function is called on a semaphore that no longer exists, 
an MRAPI_EDELETED error code will be returned.  When extended 
error checking is disabled, the MRAPI_ERR_SEM_INVALID error will 
be returned.

ERRORS
MRAPI_ERR_PARAMETER Invalid attribute parameter.
MRAPI_ERR_SEM_INVALID Argument is not a valid semaphore handle.
MRAPI_ERR_ATTR_NUM Unknown attribute number
MRAPI_ERR_ATTR_SIZE Incorrect attribute size


NOTE

***********************************************************************/
void mrapi_sem_get_attribute (
 	MRAPI_IN mrapi_sem_hndl_t sem,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_OUT void* attribute,
 	MRAPI_IN size_t attribute_size,
 	MRAPI_OUT mrapi_status_t* status)
{

  
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (attribute == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else if (  mrapi_impl_valid_sem_hndl(sem,status)) {
    mrapi_impl_sem_get_attribute(sem,attribute_num,attribute,attribute_size,status);
  }
}

/************************************************************************
mrapi_sem_get

DESCRIPTION
Given a sem_id, this function returns the MRAPI handle for referencing that semaphore.

RETURN VALUE
On success the semaphore handle is returned and *status is set 
to MRAPI_SUCCESS.  On error, *status is set to the appropriate 
error defined below.  When extended error checking is enabled, 
if this function is called on a semaphore that no longer exists, 
an MRAPI_EDELETED error code will be returned.  When extended 
error checking is disabled, the MRAPI_ERR_SEM_INVALID error will 
be returned.

ERRORS
MRAPI_ERR_SEM_ID_INVALID
The sem_id parameter does not refer to a valid semaphore or was called with sem_id set 
to MRAPI_SEM_ID_ANY.

MRAPI_ERR_NODE_NOTINIT The calling node is not initialized.

MRAPI_ERR_DOMAIN_NOTSHARED
This resource can not be shared by this domain. 

MRAPI_ERR_SEM_DELETED	If the semaphore has been deleted then 
if MRAPI_ERROR_EXT attribute is set, MRAPI will  return MRAPI_EDELETED 
otherwise MRAPI will just return MRAPI_ERR_SEM_INVALID. 



NOTE

***********************************************************************/
mrapi_sem_hndl_t mrapi_sem_get(
 	MRAPI_IN mrapi_sem_id_t sem_id,
 	MRAPI_OUT mrapi_status_t* status)
{ 
  mrapi_sem_hndl_t sem;

  *status = MRAPI_ERR_SEM_ID_INVALID;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_sem_get(&sem,sem_id)) {
    *status = MRAPI_SUCCESS;
  }
  return sem;
}

/************************************************************************
mrapi_sem_delete

DESCRIPTION
This function deletes the semaphore.  The semaphore will only 
be deleted if the semaphore is not locked.  If the semaphore 
attributes indicate extended error checking is enabled then all 
subsequent lock requests will be notified that the semaphore 
was deleted.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status 
is set to the appropriate error defined below. When extended 
error checking is enabled, if this function is called on a semaphore 
that no longer exists, an MRAPI_EDELETED error code will be returned. 
 When extended error checking is disabled, the MRAPI_ERR_SEM_INVALID 
error will be returned.

ERRORS
MRAPI_ERR_SEM_INVALID Argument is not a valid semaphore handle.

MRAPI_ERR_SEM_DELETED	If the semaphore has been deleted then 
if MRAPI_ERROR_EXT attribute is set, MRAPI will  return MRAPI_EDELETED 
otherwise MRAPI will just return MRAPI_ERR_SEM_INVALID.

MRAPI_ERR_SEM_LOCKED	The semaphore is locked and cannot be deleted.



NOTE

***********************************************************************/
void mrapi_sem_delete(
 	MRAPI_IN mrapi_sem_hndl_t sem,
 	MRAPI_OUT mrapi_status_t* status)
{

  
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( mrapi_impl_valid_sem_hndl(sem,status)) {
    if (mrapi_impl_sem_delete(sem)) {
      *status = MRAPI_SUCCESS;
    } else {
      *status = MRAPI_ERR_SEM_LOCKED;
    }
  }
}

/************************************************************************
mrapi_sem_lock

DESCRIPTION
This function attempts to obtain a single lock on the semaphore 
and will block until a lock is available or the timeout is reached 
(if timeout is non-zero).  If the request can't be satisfied 
for some other reason, this function will return the appropriate 
error code below.  An application may make this call as many 
times as needed to obtain multiple locks, up to the limit specified 
by the shared_lock_limit parameter used when the semaphore was 
created. 

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status 
is set to the appropriate error defined below. When extended 
error checking is enabled, if lock is called on semaphore that 
no longer exists, an MRAPI_EDELETED error code will be returned. 
 When extended error checking is disabled, the MRAPI_ERR_SEM_INVALID 
error will be returned and the lock will fail.

ERRORS
MRAPI_ERR_SEM_INVALID Argument is not a valid semaphore handle.

MRAPI_ERR_SEM_DELETED	If the semaphore has been deleted then 
if MRAPI_ERROR_EXT attribute is set, MRAPI will  return MRAPI_EDELETED 
otherwise MRAPI will just return MRAPI_ERR_SEM_INVALID. MRAPI_TIMEOUT	
Timeout was reached.

NOTE

***********************************************************************/
void mrapi_sem_lock(
 MRAPI_IN mrapi_sem_hndl_t sem,
 MRAPI_IN mrapi_timeout_t timeout,
 MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_SUCCESS;
  
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (  mrapi_impl_valid_sem_hndl(sem,status)) {
    mrapi_impl_sem_lock(sem,1,timeout,status);
  }  
}

/************************************************************************
mrapi_sem_trylock

DESCRIPTION
This function attempts to obtain a single lock on the semaphore. 
 If the lock can't be obtained because all the available locks 
are already locked (by this node and/or others) then the function 
will immediately return MRAPI_FALSE.  If the request can't be 
satisfied for any other reason, then this function will immediately 
return the appropriate error code below.

RETURN VALUE
Returns MRAPI_TRUE if the lock was acquired, returns MRAPI_FALSE 
otherwise.  If there was an error then *status will be set to 
indicate the error from the table below, otherwise *status will 
indicate MRAPI_SUCCESS.  If the lock could not be obtained then 
*status will be either MRAPI_ELOCKED or one of the error conditions 
in the table below. When extended error checking is enabled, 
if this function is called on a semaphore that no longer exists, 
an MRAPI_EDELETED error code will be returned.  When extended 
error checking is disabled, the MRAPI_ERR_SEM_INVALID error will 
be returned.

ERRORS
MRAPI_ERR_SEM_INVALID Argument is not a valid semaphore handle.

MRAPI_ERR_SEM_DELETED	If the semaphore has been deleted then 
if MRAPI_ERROR_EXT attribute is set, MRAPI will  return MRAPI_EDELETED 
otherwise MRAPI will just return MRAPI_ERR_SEM_INVALID.


NOTE

***********************************************************************/
mrapi_boolean_t mrapi_sem_trylock(MRAPI_IN mrapi_sem_hndl_t sem,
                                  MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (  mrapi_impl_valid_sem_hndl(sem,status)) {
    if ( mrapi_impl_sem_lock(sem,1,1,status)) {    
      return MRAPI_TRUE;
    } 
  }
  return MRAPI_FALSE;
 }

/************************************************************************
mrapi_sem_unlock

DESCRIPTION
This function releases a single lock.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status 
is set to the appropriate error defined below. When extended 
error checking is enabled, if this function is called on a semaphore 
that no longer exists, an MRAPI_EDELETED error code will be returned. 
 When extended error checking is disabled, the MRAPI_ERR_SEM_INVALID 
error will be returned.

ERRORS
MRAPI_ERR_SEM_INVALID 	Argument is not a valid semaphore handle.

MRAPI_ERR_SEM_NOTLOCKED This node does not have a lock on this semaphore

MRAPI_ERR_SEM_DELETED	If the semaphore has been deleted then 
if MRAPI_ERROR_EXT attribute is set, MRAPI will  return MRAPI_EDELETED 
otherwise MRAPI will just return MRAPI_ERR_SEM_INVALID. 



NOTE

***********************************************************************/
void mrapi_sem_unlock (
 MRAPI_IN mrapi_sem_hndl_t sem,
 MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (  mrapi_impl_valid_sem_hndl(sem,status)) {
    mrapi_impl_sem_unlock(sem,1,status);
  } 
}
