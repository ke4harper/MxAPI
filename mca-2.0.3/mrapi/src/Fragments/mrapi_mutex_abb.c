/*
Copyright (c) 2012, ABB, Inc
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of ABB, Inc nor the names of its contributors may be
	  used to endorse or promote products derived from this software without
	  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/************************************************************************
mrapi_mutex_create

DESCRIPTION
This function creates a mutex.  For non-default behavior, attributes 
must be set before the call to mrapi_mutex_create().  Once a 
mutex has been created, its attributes may not be changed.  If 
the attributes are NULL, then default attributes will be used. 
 The recursive attribute is disabled by default.  If you want 
to enable recursive locking/unlocking then you need to set that 
attribute before the call to create.  If mutex_id is set to MRAPI_MUTEX_ID_ANY, 
then MRAPI will choose an internal id for you.  

RETURN VALUE
On success a mutex handle is returned and *status is set to MRAPI_SUCCESS. 
 On error, *status is set to the appropriate error defined below. 
 In the case where the mutex already exists, status will be set 
to MRAPI_EXISTS and the handle returned will not be a valid handle. 
 

ERRORS
MRAPI_ERR_MUTEX_ID_INVALID	The mutex_id is not a valid mutex id.
MRAPI_ERR_MUTEX_EXISTS	This mutex is already created.
MRAPI_ERR_MUTEX_LIMIT Exceeded maximum number of mutexes allowed.
MRAPI_ERR_NODE_NOTINIT The calling node is not initialized.
MRAPI_ERR_PARAMETER Invalid attributes parameter.

NOTE

***********************************************************************/
mrapi_mutex_hndl_t mrapi_mutex_create(
 	MRAPI_IN mrapi_mutex_id_t mutex_id,
 	MRAPI_IN mrapi_mutex_attributes_t* attributes,
 	MRAPI_OUT mrapi_status_t* status)
{
  mrapi_mutex_hndl_t mutex_hndl;


  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (!mrapi_impl_mutex_validID(mutex_id)) {
    *status = MRAPI_ERR_MUTEX_ID_INVALID;
  } else if (mrapi_impl_mutex_create(&mutex_hndl,mutex_id,attributes,status)){
    *status = MRAPI_SUCCESS;
    return mutex_hndl;
  } else {
    /* assume the mutex already exists */
    *status = MRAPI_ERR_MUTEX_EXISTS;
  }
  return 0 ;
}

/************************************************************************
mrapi_mutex_init_attributes

DESCRIPTION
This function initializes the values of an mrapi_mutex_attributes_t 
structure.  For non-default behavior this function should be 
called prior to calling mrapi_mutex_set_attribute().  You would 
then use mrapi_mutex_set_attribute() to change any default values 
prior to calling mrapi_mutex_create().

RETURN VALUE
On success *status is set to MRAPI_SUCCESS.  On error, *status is set to 
the appropriate error defined below.

ERRORS
MRAPI_ERR_PARAMETER Invalid attributes parameter.

NOTE

***********************************************************************/
void mrapi_mutex_init_attributes(
                                 MRAPI_OUT mrapi_mutex_attributes_t* attributes,
                                 MRAPI_OUT mrapi_status_t* status)
{
 *status = MRAPI_SUCCESS;
  if (attributes == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else {
    mrapi_impl_mutex_init_attributes(attributes);
  }
}

/************************************************************************
mrapi_mutex_set_attribute

DESCRIPTION
This function is used to change default values of an mrapi_mutex_attributes_t 
data structure prior to calling mrapi_mutex_create().  Calls 
to this function have no effect on mutex attributes once the 
mutex has been created.

MRAPI pre-defined mutex attributes:
Attribute num:	Description:	Datatype:	Default:
MRAPI_MUTEX_RECURSIVE	Indicates whether or not this is a recursive mutex. 	mrapi_boolean_t	MRAPI_FALSE
MRAPI_ERROR_EXT	Indicates whether or not this mutex has extended error checking enabled.  	mrapi_boolean_t	MRAPI_FALSE
MRAPI_DOMAIN_SHARED	Indicates whether or not the mutex is shareable across domains.	mrapi_boolean_t	MRAPI_TRUE

RETURN VALUE
On success *status is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_ATTR_READONLY Attribute can not be modified.
MRAPI_ERR_PARAMETER Invalid attribute parameter.
MRAPI_ERR_ATTR_NUM Unknown attribute number
MRAPI_ERR_ATTR_SIZE Incorrect attribute size

NOTE

***********************************************************************/
void mrapi_mutex_set_attribute (
 	MRAPI_OUT mrapi_mutex_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_IN void* attribute,
 	MRAPI_IN size_t attr_size,
 	MRAPI_OUT mrapi_status_t* status)
{
  if (attributes == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else {
    mrapi_impl_mutex_set_attribute(attributes,attribute_num,attribute,attr_size,status);
  }
}

/************************************************************************
mrapi_mutex_get_attribute

DESCRIPTION
Returns the attribute that corresponds to the given attribute_num 
for this mutex.  The attributes may be viewed but may not be 
changed (for this mutex).

RETURN VALUE
On success *status is set to MRAPI_SUCCESS and the attribute 
value is filled in.  On error, *status is set to the appropriate 
error defined below and the attribute value is undefined.  The 
attribute identified by the attribute_num is returned in the 
void* attribute parameter.  When extended error checking is enabled, 
if this function is called on a mutex that no longer exists, 
an MRAPI_EDELETED error code will be returned.  When extended 
error checking is disabled, the MRAPI_ERR_MUTEX_INVALID error 
will be returned.

ERRORS
MRAPI_ERR_PARAMETER Invalid attribute parameter.
MRAPI_ERR_MUTEX_INVALID Argument is not a valid mutex handle.
MRAPI_ERR_ATTR_NUM Unknown attribute number
MRAPI_ERR_ATTR_SIZE Incorrect attribute size

NOTE

***********************************************************************/
void mrapi_mutex_get_attribute (
 	MRAPI_IN mrapi_mutex_hndl_t mutex,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_OUT void* attribute,
 	MRAPI_IN size_t attribute_size,
 	MRAPI_OUT mrapi_status_t* status)
{
  
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (attribute == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else if ( mrapi_impl_valid_mutex_hndl(mutex,status)) {
    mrapi_impl_mutex_get_attribute(mutex,attribute_num,attribute,attribute_size,status);
  }
}

/************************************************************************
mrapi_mutex_get

DESCRIPTION
Given a mutex_id, this function returns the MRAPI handle for referencing that mutex.

RETURN VALUE
On success the mutex handle is returned and *status is set to 
MRAPI_SUCCESS.  On error, *status is set to the appropriate error 
defined below.  When extended error checking is enabled, if this 
function is called on a mutex that no longer exists, an MRAPI_EDELETED 
error code will be returned.  When extended error checking is 
disabled, the MRAPI_ERR_MUTEX_INVALID error will be returned.


ERRORS
MRAPI_ERR_MUTEX_ID_INVALID
The mutex_id parameter does not refer to a valid mutex or it is set to MRAPI_MUTEX_ID_ANY.

MRAPI_ERR_NODE_NOTINIT The node/domain is not initialized.

MRAPI_ERR_DOMAIN_NOTSHARED This resource can not be shared by this domain.

MRAPI_ERR_MUTEX_DELETED	If the mutex has been deleted then if 
MRAPI_ERROR_EXT attribute is set, MRAPI will  return MRAPI_EDELETED 
otherwise MRAPI will just return MRAPI_ERR_MUTEX_INVALID. 


NOTE

***********************************************************************/
mrapi_mutex_hndl_t mrapi_mutex_get(
 	MRAPI_IN mrapi_mutex_id_t mutex_id,
 	MRAPI_OUT mrapi_status_t* status)
{
  mrapi_mutex_hndl_t mutex;

  *status = MRAPI_ERR_MUTEX_ID_INVALID;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else  if (mrapi_impl_mutex_get(&mutex,mutex_id)) {
    *status = MRAPI_SUCCESS;
  }
  return mutex;
}

/************************************************************************
mrapi_mutex_delete

DESCRIPTION
This function deletes the mutex.  The mutex may only be deleted 
if it is unlocked.  If the mutex attributes indicate extended 
error checking is enabled then all subsequent lock requests will 
be notified that the mutex was deleted.  When extended error 
checking is enabled, if this function is called on a mutex that 
no longer exists, an MRAPI_EDELETED error code will be returned. 
 When extended error checking is disabled, the MRAPI_ERR_MUTEX_INVALID 
error will be returned.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_MUTEX_INVALID Argument is not a valid mutex handle.

MRAPI_ERR_MUTEX_LOCKED	The mutex is locked and cannot be deleted.

MRAPI_ERR_MUTEX_DELETED	If the mutex has been deleted then if 
MRAPI_ERROR_EXT attribute is set, MRAPI will  return MRAPI_EDELETED 
otherwise MRAPI will just return MRAPI_ERR_MUTEX_INVALID. 


NOTE

***********************************************************************/
void mrapi_mutex_delete(
 	MRAPI_IN mrapi_mutex_hndl_t mutex,
 	MRAPI_OUT mrapi_status_t* status)
{

  
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( mrapi_impl_valid_mutex_hndl(mutex,status)) {
	  /* All the locks must be owned by caller */
	  mrapi_impl_sem_lock(mutex, MRAPI_LOCKS_ALL, MRAPI_TIMEOUT_IMMEDIATE, status);
	if (mrapi_impl_mutex_delete(mutex)) {
      *status = MRAPI_SUCCESS;
    } else {
      *status = MRAPI_ERR_MUTEX_LOCKED;
    }
  }
}

/************************************************************************
mrapi_mutex_lock

DESCRIPTION
This function attempts to lock a mutex and will block if another 
node has a lock on the mutex.  When it obtains the lock, it sets 
up a unique key for that lock and that key is to be passed back 
on the call to unlock.  This key allows us to support recursive 
locking.  The lock_key is only valid if status indicates success. 
 Whether or not a mutex can be locked recursively is controlled 
via the MRAPI_MUTEX_RECURSIVE attribute, and the default is MRAPI_FALSE. 


RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status 
is set to the appropriate error defined below.  When extended 
error checking is enabled, if this function is called on a mutex 
that no longer exists, an MRAPI_EDELETED error code will be returned. 
 When extended error checking is disabled, the MRAPI_ERR_MUTEX_INVALID 
error will be returned.

ERRORS
MRAPI_ERR_MUTEX_INVALID
Argument is not a valid mutex handle.

MRAPI_ERR_MUTEX_LOCKED 		Mutex is already locked by another node 
or mutex is already locked by this node and is not a recursive 
mutex.MRAPI_ERR_MUTEX_DELETED	If the mutex has been deleted then 
if MRAPI_ERROR_EXT attribute is set, MRAPI will  return MRAPI_EDELETED 
otherwise MRAPI will just return MRAPI_ERR_MUTEX_INVALID. 

MRAPI_TIMEOUT	Timeout was reached.

MRAPI_ERR_PARAMETER Invalid lock_key or timeout parameter.

NOTE

***********************************************************************/
void mrapi_mutex_lock (
 MRAPI_IN mrapi_mutex_hndl_t mutex,
 MRAPI_OUT mrapi_key_t* lock_key,
 MRAPI_IN mrapi_timeout_t timeout,
 MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( mrapi_impl_valid_mutex_hndl(mutex,status)) {
    mrapi_impl_mutex_lock(mutex,lock_key,timeout,status);
  }
}

/************************************************************************
mrapi_mutex_trylock

DESCRIPTION
This function attempts to obtain a lock on the mutex.  If the 
lock can't be obtained then the function will immediately return 
MRAPI_FALSE.  If the request can't be satisfied for any other 
reason, then this function will immediately return the appropriate 
error code below.  If it is successful in obtaining the lock, 
it sets up a unique key for that lock and that key is to be passed 
back on the call to unlock.  The lock_key is only valid if status 
indicates success and the function returns MRAPI_TRUE.  This 
key allows us to support recursive locking.  Whether or not a 
mutex can be locked recursively is controlled via the MRAPI_MUTEX_RECURSIVE 
attribute, and the default is MRAPI_FALSE. 

RETURN VALUE
Returns MRAPI_TRUE if the lock was acquired, returns MRAPI_FALSE 
otherwise.  If there was an error then *status will be set to 
indicate the error from the table below, otherwise *status will 
indicate MRAPI_SUCCESS.  If the lock could not be obtained then 
*status will be either MRAPI_ELOCKED or one of the error conditions 
in the table below.  When extended error checking is enabled, 
if lock is called on a mutex that no longer exists, an MRAPI_EDELETED 
error code will be returned.  When extended error checking is 
disabled, the MRAPI_ERR_MUTEX_INVALID error will be returned 
and the lock will fail.

ERRORS

MRAPI_ERR_MUTEX_INVALID Argument is not a valid mutex handle.

MRAPI_ERR_MUTEX_DELETED	If the mutex has been deleted then if 
MRAPI_ERROR_EXT attribute is set, MRAPI will  return MRAPI_EDELETED 
otherwise MRAPI will just return MRAPI_ERR_MUTEX_INVALID. 

MRAPI_ERR_MUTEX_LOCKED 
Mutex is already locked by another node or mutex is already 
locked by this node and is not a recursive mutex.

MRAPI_ERR_PARAMETER Invalid lock_key parameter.


NOTE

***********************************************************************/
mrapi_boolean_t mrapi_mutex_trylock(
 MRAPI_IN mrapi_mutex_hndl_t mutex,
 MRAPI_OUT mrapi_key_t* lock_key,
 MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( mrapi_impl_valid_mutex_hndl(mutex,status)) {
    if ( mrapi_impl_mutex_lock(mutex,lock_key,1,status)) {
      return MRAPI_TRUE;
    } 
  }
  return MRAPI_FALSE;
}

/************************************************************************
mrapi_mutex_unlock

DESCRIPTION
This function unlocks a mutex.  If the mutex is recursive, then 
the lock_key parameter passed in must match the lock_key that 
was returned by the corresponding call to lock the mutex, and 
the set of recursive locks must be released using lock_keys in 
the reverse order that they were obtained.  When extended error 
checking is enabled, if this function is called on a mutex that 
no longer exists, an MRAPI_EDELETED error code will be returned. 
 When extended error checking is disabled, the MRAPI_ERR_MUTEX_INVALID 
error will be returned.


RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_MUTEX_INVALID
Argument is not a valid mutex handle.

MRAPI_ERR_MUTEX_NOTLOCKED
Mutex is not locked.

MRAPI_ERR_MUTEX_KEY
lock_key is invalid for this mutex.

MRAPI_ERR_MUTEX_LOCKORDER
The unlock call does not match the lock order for this recursive mutex.

MRAPI_ERR_PARAMETER Invalid lock_key parameter.

MRAPI_ERR_MUTEX_DELETED	If the mutex 
has been deleted then if MRAPI_ERROR_EXT attribute is set, MRAPI 
will  return MRAPI_EDELETED otherwise MRAPI will just return 
MRAPI_ERR_MUTEX_INVALID. 


NOTE

***********************************************************************/
void mrapi_mutex_unlock(
 MRAPI_IN mrapi_mutex_hndl_t mutex,
 MRAPI_IN mrapi_key_t* lock_key,
 MRAPI_OUT mrapi_status_t* status)
{
  

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( mrapi_impl_valid_mutex_hndl(mutex,status)) {
    mrapi_impl_mutex_unlock(mutex,lock_key,status); 
  } 
}
