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
mrapi_rwl_create

DESCRIPTION
This function creates a reader/writer lock.  Unless you want 
the defaults, attributes must be set before the call to mrapi_rwl_create(). 
 Once a reader/writer lock has been created, its attributes may 
not be changed.  If the attributes are NULL, then implementation 
defined default attributes will be used.  If rwl_id is set to 
MRAPI_RWL_ID_ANY, then MRAPI will choose an internal id for you. 
 

RETURN VALUE
On success a reader/writer lock handle is returned and *status 
is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate 
error defined below.  In the case where the reader/writer lock 
already exists, status will be set to MRAPI_EXISTS and the handle 
returned will not be a valid handle.  

ERRORS
MRAPI_ERR_RWL_INVALID The rwl_id is not a valid reader/writer lock id.
MRAPI_ERR_RWL_EXISTS	This reader/writer lock is already created.
MRAPI_ERR_RWL_LIMIT Exceeded maximum number of reader/writer locks allowed.
MRAPI_ERR_NODE_NOTINIT The calling node is not initialized.
MRAPI_ERR_PARAMETER Invalid attributes parameter.

NOTE

***********************************************************************/

mrapi_rwl_hndl_t mrapi_rwl_create(
 	MRAPI_IN mrapi_rwl_id_t rwl_id,
 	MRAPI_IN mrapi_rwl_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t reader_lock_limit,
 	MRAPI_OUT mrapi_status_t* status)
{

  mrapi_sem_hndl_t rwl_hndl = 0;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT; /* fixme: how to handle domain_notinit? */
  } else if (!mrapi_impl_rwl_validID(rwl_id)) {
    *status = MRAPI_ERR_RWL_ID_INVALID;
  } else if (mrapi_impl_rwl_create(&rwl_hndl,rwl_id,attributes,reader_lock_limit,status)){
    *status = MRAPI_SUCCESS;
  }
  return rwl_hndl;
}

/************************************************************************
mrapi_rwl_init_attributes

DESCRIPTION
Unless you want the defaults, this call must be used to initialize 
the values of an mrapi_rwl_attributes_t structure prior to mrapi_rwl_set_attribute(). 
 Use mrapi_rwl_set_attribute() to change any default values prior 
to calling mrapi_rwl_create().

RETURN VALUE
On success *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below.

ERRORS
MRAPI_ERR_PARAMETER Invalid attributes parameter.

NOTE

***********************************************************************/
void mrapi_rwl_init_attributes(
                                 MRAPI_OUT mrapi_rwl_attributes_t* attributes,
                                 MRAPI_OUT mrapi_status_t* status)
{
  *status = MRAPI_SUCCESS;
   if (attributes == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else {
    mrapi_impl_rwl_init_attributes(attributes);
  }
}

/************************************************************************
mrapi_rwl_set_attribute

DESCRIPTION
This function is used to change default values of an mrapi_rwl_attributes_t 
data structure prior to calling mrapi_rwl_create().  Calls to 
this function have no effect on mutex attributes once the mutex 
has been created.

MRAPI pre-defined reader/writer lock attributes:
Attribute num:	Description:	Datatype:	Default:
MRAPI_ERROR_EXT	Indicates whether or not this reader/writer lock 
has extended error checking enabled.  	mrapi_boolean_t	MRAPI_FALSE

MRAPI_DOMAIN_SHARED	Indicates whether or not the reader/writer 
lock is shareable across domains.  	mrapi_boolean_t	MRAPI_TRUE


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

void mrapi_rwl_set_attribute(
 	MRAPI_OUT mrapi_rwl_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_IN void* attribute,
 	MRAPI_IN size_t attr_size,
 	MRAPI_OUT mrapi_status_t* status)
{
  if (attributes == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else {
    mrapi_impl_rwl_set_attribute(attributes,attribute_num,attribute,attr_size,status);
  }
}





/************************************************************************
mrapi_rwl_get_attribute

DESCRIPTION
Returns the attribute that corresponds to the given attribute_num 
for this reader/writer lock.  The attribute may be viewed but 
may not be changed (for this reader/writer lock).

RETURN VALUE
On success *status is set to MRAPI_SUCCESS and the attribute 
value is filled in.  On error, *status is set to the appropriate 
error defined below and the attribute value is undefined.  The 
attribute identified by the attribute_num is returned in the 
void* attribute parameter. When extended error checking is enabled, 
if this function is called on a reader/writer lock that no longer 
exists, an MRAPI_EDELETED error code will be returned.  When 
extended error checking is disabled, the MRAPI_ERR_RWL_INVALID 
error will be returned.

ERRORS
MRAPI_ERR_PARAMETER Invalid attribute parameter.
MRAPI_ERR_RWL_INVALID Argument is not a valid rwl handle.
MRAPI_ERR_ATTR_NUM Unknown attribute number
MRAPI_ERR_ATTR_SIZE Incorrect attribute size


NOTE
It is up to the implementation as to whether a reader/writer 
lock may be shared across domains.  This is specified as an attribute 
during creation and the default is MRAPI_FALSE.

***********************************************************************/
void mrapi_rwl_get_attribute (
 	MRAPI_IN mrapi_rwl_hndl_t rwl,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_OUT void* attribute,
 	MRAPI_IN size_t attribute_size,
 	MRAPI_OUT mrapi_status_t* status)
{

  
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else  if (attribute == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else if ( mrapi_impl_valid_rwl_hndl(rwl,status)) {
    mrapi_impl_rwl_get_attribute(rwl,attribute_num,attribute,attribute_size,status);
  }
}

/************************************************************************
mrapi_rwl_get

DESCRIPTION
Given a rwl_id, this function returns the MRAPI handle for referencing that reader/writer lock.

RETURN VALUE
On success the reader/writer lock handle is returned and *status 
is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate 
error defined below.

ERRORS
MRAPI_ERR_RWL_ID_INVALID
The rwl_id parameter does not refer to a valid reader/writer 
lock or it was called with rwl_id set to MRAPI_RWL_ID_ANY.

MRAPI_ERR_NODE_NOTINIT
The calling node is not initialized.

MRAPI_ERR_DOMAIN_NOTSHARED
This resource can not be shared by this domain. 



NOTE

***********************************************************************/
mrapi_rwl_hndl_t mrapi_rwl_get(
 	MRAPI_IN mrapi_rwl_id_t rwl_id,
 	MRAPI_OUT mrapi_status_t* status)
{
  mrapi_rwl_hndl_t rwl;

  
  *status = MRAPI_ERR_RWL_ID_INVALID;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_rwl_get(&rwl,rwl_id)) {
    *status = MRAPI_SUCCESS;
  }
  return rwl;
}

/************************************************************************
mrapi_rwl_delete

DESCRIPTION
This function deletes the reader/writer lock.  A reader/writer 
lock can only be deleted if it is not locked.  If the reader/writer 
lock attributes indicate extended error checking is enabled then 
all subsequent lock requests will be notified that the reader/writer 
lock was deleted.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status 
is set to the appropriate error defined below.  When extended 
error checking is enabled, if this function is called on a reader/writer 
lock that no longer exists, an MRAPI_EDELETED error code will 
be returned.  When extended error checking is disabled, the MRAPI_ERR_RWL_INVALID 
error will be returned.

ERRORS
MRAPI_ERR_RWL_INVALID
Argument is not a valid reader/writer lock handle.

MRAPI_ERR_RWL_LOCKED	The reader/writer lock was locked and cannot be deleted.



NOTE

***********************************************************************/
void mrapi_rwl_delete(
 	MRAPI_IN mrapi_rwl_hndl_t rwl,
 	MRAPI_OUT mrapi_status_t* status)
{

  
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (  mrapi_impl_valid_rwl_hndl(rwl,status)) {
    if (mrapi_impl_rwl_delete(rwl)) {
      *status = MRAPI_SUCCESS;
    } else {
      *status = MRAPI_ERR_RWL_LOCKED;
    }
  }
}

/************************************************************************
mrapi_rwl_lock

DESCRIPTION
This function attempts to obtain a single lock on the reader/writer 
lock and will block until a lock is available or the timeout 
is reached (if timeout is non-zero).  A node may only have one 
reader lock or one writer lock at any given time.  The mode parameter 
is used to specify the type of lock: MRAPI_READER (shared) or 
MRAPI_WRITER (exclusive).  If the lock can't be obtained for 
some other reason, this function will return the appropriate 
error code below.  

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status 
is set to the appropriate error defined below.  When extended 
error checking is enabled, if lock is called on a reader/writer 
lock that no longer exists, an MRAPI_EDELETED error code will 
be returned.  When extended error checking is disabled, the MRAPI_ERR_RWL_INVALID 
error will be returned.  In both cases the attempt to lock will 
fail.

ERRORS
MRAPI_ERR_RWL_INVALID
Argument is not a valid reader/writer lock handle.

MRAPI_ERR_RWL_DELETED	If the reader/writer lock has been deleted 
then if MRAPI_ERROR_EXT attribute is set, MRAPI will  return 
MRAPI_EDELETED otherwise MRAPI will just return MRAPI_ERR_RWL_INVALID. 

MRAPI_TIMEOUT	Timeout was reached.

MRAPI_ERR_RWL_LOCKED	The caller already has a lock MRAPI_ERR_PARAMETER	Invalid mode.

NOTE

***********************************************************************/
void mrapi_rwl_lock(
 MRAPI_IN mrapi_rwl_hndl_t rwl,
 MRAPI_IN mrapi_rwl_mode_t mode,
 MRAPI_IN mrapi_timeout_t timeout,
 MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( mrapi_impl_valid_rwl_hndl(rwl,status)) {
    mrapi_impl_rwl_lock(rwl,mode,timeout,status);
  }
}

/************************************************************************
mrapi_rwl_trylock

DESCRIPTION
This function attempts to obtain a single lock on the reader/writer 
lock.  A node may only have one reader lock or one writer lock 
at any given time.  The mode parameter is used to specify the 
type of lock: MRAPI_READER (shared) or MRAPI_WRITER (exclusive). 
 If the lock can't be obtained because a reader lock was requested 
and there is already a writer lock or a writer lock was requested 
and there is already any lock then the function will immediately 
return MRAPI_FALSE.  If the request can't be satisfied for any 
other reason, then this function will immediately return the 
appropriate error code below. 

RETURN VALUE
Returns MRAPI_TRUE if the lock was acquired, returns MRAPI_FALSE 
otherwise.  If there was an error then *status will be set to 
indicate the error from the table below, otherwise *status will 
indicate MRAPI_SUCCESS.  If the lock could not be obtained then 
*status will be either MRAPI_ELOCKED or one of the error conditions 
in the table below.  When extended error checking is enabled, 
if trylock is called on a reader/writer lock that no longer exists, 
an MRAPI_EDELETED error code will be returned.  When extended 
error checking is disabled, the MRAPI_ERR_RWL_INVALID error will 
be returned and the lock will fail.

ERRORS
MRAPI_ERR_RWL_INVALID
Argument is not a valid reader/writer lock handle.

MRAPI_ERR_RWL_DELETED	If the reader/writer lock has been deleted 
then if MRAPI_ERROR_EXT attribute is set, MRAPI will  return 
MRAPI_EDELETED otherwise MRAPI will just return MRAPI_ERR_RWL_INVALID. 

MRAPI_ERR_RWL_LOCKED 		The reader/writer lock is already exclusively 
locked.

MRAPI_ERR_PARAMETER	Invalid mode.



NOTE

***********************************************************************/
mrapi_boolean_t mrapi_rwl_trylock(
 MRAPI_IN mrapi_rwl_hndl_t rwl,
 MRAPI_IN mrapi_rwl_mode_t mode,
 MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_SUCCESS;

  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( mrapi_impl_valid_rwl_hndl(rwl,status)) {
    if (mrapi_impl_rwl_lock(rwl,mode,1,status)) {
      return MRAPI_TRUE;
    }
  }
  return MRAPI_FALSE;
}

/************************************************************************
mrapi_rwl_unlock

DESCRIPTION
This function releases a single lock.   The lock to be released 
will be either a reader lock or a writer lock, as specified by 
the mode parameter used when the lock was obtained.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status 
is set to the appropriate error defined below.  When extended 
error checking is enabled, if this function is called on a reader/writer 
lock that no longer exists, an MRAPI_EDELETED error code will 
be returned.  When extended error checking is disabled, the MRAPI_ERR_RWL_INVALID 
error will be returned.

ERRORS
MRAPI_ERR_RWL_INVALID	
Argument is not a valid reader/writer lock handle.

MRAPI_ERR_RWL_NOTLOCKED
This node does not currently hold the given type (reader/writer) of lock.

NOTE

***********************************************************************/
void mrapi_rwl_unlock (
 MRAPI_IN mrapi_rwl_hndl_t rwl,
 MRAPI_IN mrapi_rwl_mode_t mode,
 MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( mrapi_impl_valid_rwl_hndl(rwl,status)) {
    mrapi_impl_rwl_unlock(rwl,mode,status);
  } 
}
