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
mrapi_rmem_create

SYNOPSIS
mrapi_rmem_hndl_t mrapi_rmem_create(
        MRAPI_IN mrapi_rmem_id_t rmem_id,
        MRAPI_IN void* mem,
        MRAPI_IN mrapi_rmem_atype_t access_type,
        MRAPI_IN mrapi_rmem_attributes_t* attributes,
        MRAPI_IN mrapi_uint_t size,
        MRAPI_OUT mrapi_status_t* status
);

DESCRIPTION
This function promotes a private or shared memory segment on the calling node 
to a remote memory segment and returns a handle.  The mem parameter is a pointer 
to the base address of the local memory buffer (see Section 3.5.2).  Once a 
memory segment has been created, its attributes may not be changed.  If the 
attributes are NULL, then implementation defined default attributes will be 
used.  If rmem_id is set to MRAPI_RMEM_ID_ANY, then MRAPI will choose an 
internal id.  access_type specifies access semantics.  Access semantics are 
per remote memory buffer instance, and are either strict (meaning all clients
 must use the same access type), or any (meaning that clients may use any type 
supported by the MRAPI implementation).  Implementations may define multiple 
access types (depending on underlying silicon capabilities), but must provide 
at minimum: MRAPI_RMEM_ATYPE_ANY (which indicates any semantics), and  
MRAPI_RMEM_ATYPE_DEFAULT, which has strict semantics  Note that MRAPI_RMEM_ATYPE_ANY 
is only valid for remote memory buffer creation, clients must use 
MRAPI_RMEM_ATYPE_DEFAULT or another specific type of access mechanism provided 
by the MRAPI implementation (DMA, etc.)   Specifying any type of access (even default) 
other than MRAPI_RMEM_ATYPE_ANY forces strict mode.  The access type is explicity 
passed in to create rather than being an attribute because it is so system specific, 
there is no easy way to define an attribute with a default value.

RETURN VALUE
On success a remote memory segment handle is returned, the address is filled in and 
*status is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate error 
defined below.  In the case where the remote memory segment already exists, status 
will be set to MRAPI_EXISTS and the handle returned will not be a valid handle.

ERRORS
MRAPI_ERR_RMEM_ID_INVALID The rmem_id is not a valid remote memory segment id.
MRAPI_ERR_RMEM_EXISTS   This remote memory segment is already created.
MRAPI_ERR_MEM_LIMIT No memory available.
MRAPI_ERR_RMEM_TYPENOTVALID     Invalid access_type parameter
MRAPI_ERR_NODE_NOTINIT The calling node is not initialized.
MRAPI_ERR_PARAMETER Incorrect attributes, rmem, or size  parameter.
MRAPI_ERR_RMEM_CONFLICT  The memory pointer + size collides with another remote memory.

NOTE
This function is for promoting a segment of local memory (heap or stack, but stack 
would be dangerous and should be done with care) or an already created shared memory 
segment to rmem, but that also should be done with care.


***********************************************************************/

mrapi_rmem_hndl_t mrapi_rmem_create(
 	MRAPI_IN mrapi_rmem_id_t rmem_id,
 	MRAPI_IN void* mem,
 	MRAPI_IN mrapi_rmem_atype_t access_type,
 	MRAPI_IN mrapi_rmem_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t size,
 	MRAPI_OUT mrapi_status_t* status)
{
  mrapi_rmem_hndl_t rmem = 0;

  
  *status = MRAPI_ERR_MEM_LIMIT;
  
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (!mrapi_impl_rmem_validID(rmem_id)) {
    *status = MRAPI_ERR_RMEM_ID_INVALID;
  } else if (mrapi_impl_rmem_exists(rmem_id)) {
    *status = MRAPI_ERR_RMEM_EXISTS;
  } else if (!mrapi_impl_valid_atype(access_type)) {
    *status = MRAPI_ERR_RMEM_TYPENOTVALID;
  } else {
    mrapi_impl_rmem_create (&rmem,rmem_id,mem,access_type,attributes,size,status); 
  }
  return rmem;
}

/************************************************************************
mrapi_rmem_init_attributes

DESCRIPTION
Unless you want the defaults, this call must be used to initialize 
the values of an mrapi_rmem_attributes_t structure prior to mrapi_rmem_set_attribute(). 
 You would then use mrapi_rmem_set_attribute() to change any 
default values prior to calling mrapi_rmem_create().

RETURN VALUE
On success *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below.

ERRORS
MRAPI_ERR_PARAMETER
Invalid attributes parameter
NOTE

***********************************************************************/
void mrapi_rmem_init_attributes(
                                 MRAPI_OUT mrapi_rmem_attributes_t* attributes,
                                 MRAPI_OUT mrapi_status_t* status)
{
  if (attributes == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else {
    mrapi_impl_rmem_init_attributes(attributes);
  }
}

/************************************************************************
mrapi_rmem_set_attribute

DESCRIPTION
This function is used to change default values of an mrapi_rmem_attributes_t 
data structure prior to calling mrapi_rmem_create().  

MRAPI pre-defined remote memory attributes:
Attribute num:	Description:	Datatype:	Default:
MRAPI_DOMAIN_SHARED	Indicates whether or not this remote memory 
is shareable across domains.  	mrapi_boolean_t	MRAPI_TRUE

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
void mrapi_rmem_set_attribute(
 	MRAPI_OUT mrapi_rmem_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_IN void* attribute,
 	MRAPI_IN size_t attr_size,
 	MRAPI_OUT mrapi_status_t* status)
{
  if (attributes == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else {
    mrapi_impl_rmem_set_attribute(attributes,attribute_num,attribute,attr_size,status);
  }
}

/************************************************************************
mrapi_rmem_get_attribute

DESCRIPTION
Returns the attribute that corresponds to the given attribute_num 
for this remote memory.  The attributes may be viewed but may 
not be changed (for this remote memory).

RETURN VALUE
On success *status is set to MRAPI_SUCCESS and the attribute 
value is filled in.  On error, *status is set to the appropriate 
error defined below and the attribute value is undefined.  The 
attribute identified by the attribute_num is returned in the 
void* attribute parameter.


ERRORS
MRAPI_ERR_PARAMETER Invalid attribute parameter.
MRAPI_ERR_RMEM_INVALID Argument is not a valid remote memory handle.
MRAPI_ERR_ATTR_NUM Unknown attribute number
MRAPI_ERR_ATTR_SIZE Incorrect attribute size

NOTE

***********************************************************************/
void mrapi_rmem_get_attribute(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_OUT void* attribute,
 	MRAPI_IN size_t attribute_size,
 	MRAPI_OUT mrapi_status_t* status)
{

  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (attribute == NULL) {
      *status = MRAPI_ERR_PARAMETER;
  } else if ( ! mrapi_impl_valid_rmem_hndl(rmem)) {
    *status = MRAPI_ERR_RMEM_INVALID;
  }else {
    mrapi_impl_rmem_get_attribute(rmem,attribute_num,attribute,attribute_size,status);
  }
}

/************************************************************************
mrapi_rmem_get

SYNOPSIS
mrapi_rmem_hndl_t mrapi_rmem_get(
MRAPI_IN mrapi_rmem_id_t rmem_id,
        MRAPI_IN mrapi_rmem_atype_t access_type,
        MRAPI_OUT mrapi_status_t* status
);

DESCRIPTION
Given a rmem_id, this function returns the MRAPI handle referencing to that 
remote memory segment. access_type specifies access semantics.  Access semantics 
are per remote memory buffer instance, and are either strict (meaning all clients 
must use the same access type), or any (meaning that clients may use any type 
supported by the MRAPI implementation).  Implementations may define multiple 
access types (depending on underlying silicon capabilities), but must provide at 
minimum: MRAPI_RMEM_ATYPE_ANY (which indicates any semantics), and  
MRAPI_RMEM_ATYPE_DEFAULT, which has strict semantics  Note that MRAPI_RMEM_ATYPE_ANY 
is only valid for remote memory buffer creation, clients must use MRAPI_RMEM_ATYPE_DEFAULT 
or another specific type of access mechanism provided by the MRAPI implementation (DMA, etc.)  
The access type must match the access type that the memory was created with unless the 
memory was created with the MRAPI_RMEM_ATYPE_ANY type.  See Section 3.5.2 for a 
discussion of remote memory access types.


RETURN VALUE
On success the remote memory segment handle is returned and *status is set to 
MRAPI_SUCCESS.  On error, *status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_RMEM_ID_INVALID
The rmem_id parameter does not refer to a valid remote memory segment or it was called 
with rmem_id set to MRAPI_RMEM_ID_ANY.
MRAPI_ERR_RMEM_ATYPE_NOTVALID   Invalid access_type parameter.
MRAPI_ERR_NODE_NOTINIT The calling node is not initialized.
MRAPI_ERR_DOMAIN_NOTSHARED This resource can not be shared by this domain.
MRAPI_ERR_RMEM_ATYPE    Type specified on attach is incompatible with type specified on create.


NOTE

**********************************************************************/
mrapi_rmem_hndl_t mrapi_rmem_get(
 MRAPI_IN mrapi_rmem_id_t rmem_id,
 	MRAPI_IN mrapi_rmem_atype_t access_type,
 	MRAPI_OUT mrapi_status_t* status)
{
  mrapi_rmem_hndl_t rmem = 0;


  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (!mrapi_impl_valid_atype(access_type)) {
    *status = MRAPI_ERR_RMEM_TYPENOTVALID;
  } else if (mrapi_impl_rmem_get(&rmem,rmem_id)) {
    *status = MRAPI_SUCCESS;
  } else {
    *status = MRAPI_ERR_RMEM_ID_INVALID;
  }
  return rmem;
}
/************************************************************************
mrapi_rmem_attach

DESCRIPTION
This function attaches the caller to the remote memory segment. 
 Once this is done, the caller may use the mrapi_rmem_read() 
and mrapi_rmem_write() functions.  

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below.

ERRORS
MRAPI_ERR_RMEM_INVALID Argument is not a valid remote memory segment handle.
MRAPI_ERR_RMEM_ATTACHED	The calling node is already attached to the remote memory.


NOTE
***********************************************************************/
void mrapi_rmem_attach(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_IN mrapi_rmem_atype_t access_type,
 	MRAPI_OUT mrapi_status_t* status)
{
  

  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( ! mrapi_impl_valid_rmem_hndl(rmem)) {
    *status = MRAPI_ERR_RMEM_INVALID;
  } else if (mrapi_impl_rmem_attach(rmem)) {
    *status = MRAPI_SUCCESS;
  } else {
    *status = MRAPI_ERR_RMEM_TYPENOTVALID;
  }
}





/************************************************************************
mrapi_rmem_detach

DESCRIPTION
This function detaches the caller from the remote memory segment. 
 All attached nodes must detach before any node can delete the 
memory. 

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below.

ERRORS
MRAPI_ERR_RMEM_INVALID Argument is not a valid remote memory segment handle.
MRAPI_ERR_RMEM_NOTATTACHED	The caller is not attached to the remote memory.

NOTE

***********************************************************************/
void mrapi_rmem_detach(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_ERR_RMEM_INVALID;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_valid_rmem_hndl(rmem) &&
             (mrapi_impl_rmem_detach(rmem))) {
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_rmem_delete

DESCRIPTION
This function demotes the remote memory segment.  All attached 
nodes must detach before the node can delete the memory.  Otherwise, 
delete will fail and there are no automatic retries nor deferred 
delete.  Note that memory is not de-allocated it is just no longer 
accessible via the MRAPI remote memory function calls.  Only 
the node that created the remote memory can delete it.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below.

ERRORS
MRAPI_ERR_RMEM_INVALID Argument is not a valid remote memory segment handle.
MRAPI_ERR_RMEM_ATTACH
Unable to demote the remote memory because other nodes are still attached to it.
MRAPI_ERR_RMEM_NOTOWNER	The calling node is not the one that created the remote memory.

NOTE

***********************************************************************/
void mrapi_rmem_delete(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_OUT mrapi_status_t* status)
{

  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( ! mrapi_impl_valid_rmem_hndl(rmem)) {
    *status = MRAPI_ERR_RMEM_INVALID;
  }  else if (mrapi_impl_rmem_delete (rmem)) {
    *status = MRAPI_SUCCESS;
  } 
}

/************************************************************************
mrapi_rmem_read

DESCRIPTION
This function performs num_strides memory reads, where each read 
is of size bytes_per_access bytes.  The i-th read copies bytes_per_access 
bytes of data from rmem with offset rmem_offset + i*rmem_stride 
to local_buf with offset local_offset + i*local_stride, where 
0 <= i < num_strides.  The local_buf_size represents the number 
of bytes in the local_buf.  

This supports scatter/gather type accesses.  To perform a single 
read, without the need for scatter/gather,  set the num_strides 
parameter to 1.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below.

ERRORS
MRAPI_ERR_RMEM_INVALID Argument is not a valid remote memory segment handle.

MRAPI_ERR_RMEM_BUFF_OVERRUN	rmem_offset + (rmem_stride * num_strides 
) would fall out of bounds of the remote memory buffer.

MRAPI_ERR_RMEM_STRIDE
num_strides>1 and rmem_stride and/or local_stride are less than bytes_per_access.

MRAPI_ERR_RMEM_NOTATTACHED	The caller is not attached to the remote memory.

MRAPI_ERR_PARAMETER	Either the local_buf is invalid or the buf_size is zero or bytes_per_access is zero.



NOTE

***********************************************************************/

void mrapi_rmem_read(
                     MRAPI_IN mrapi_rmem_hndl_t rmem,
                     MRAPI_IN mrapi_uint32_t rmem_offset,
                     MRAPI_OUT void* local_buf,
                     MRAPI_IN mrapi_uint32_t local_offset,
                     MRAPI_IN mrapi_uint32_t bytes_per_access,
                     MRAPI_IN mrapi_uint32_t num_strides,
                     MRAPI_IN mrapi_uint32_t rmem_stride,
                     MRAPI_IN mrapi_uint32_t local_stride,
                     MRAPI_OUT mrapi_status_t* status)
{


  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( ! mrapi_impl_valid_rmem_hndl(rmem)) {
    *status = MRAPI_ERR_RMEM_INVALID;
  } else if ((rmem_stride < bytes_per_access) || (local_stride < bytes_per_access)) {
    *status = MRAPI_ERR_RMEM_STRIDE;
  } else if (!mrapi_impl_rmem_attached(rmem)) {
    *status = MRAPI_ERR_RMEM_NOTATTACHED;
  }else {
    mrapi_impl_rmem_read (rmem,rmem_offset,local_buf,local_offset,bytes_per_access,num_strides,rmem_stride,local_stride,status); 
  }
}

/************************************************************************
mrapi_rmem_read_i

DESCRIPTION

This (non-blocking) function performs num_strides memory reads, 
where each read is of size bytes_per_access bytes.  The i-th 
read copies bytes_per_access bytes of data from rmem with offset 
rmem_offset + i*rmem_stride to local_buf with offset local_offset 
+ i*local_stride, where 0 <= i < num_strides. 

This supports scatter/gather type accesses.  To perform a single 
read, without the need for scatter/gather,  set the num_strides 
parameter to 1.



RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status 
is set to the appropriate error defined below. Use mrapi_test(), 
mrapi_wait() or mrapi_wait_any() to test for completion of the 
operation.

ERRORS
MRAPI_ERR_RMEM_INVALID
Argument is not a valid remote memory segment handle.
MRAPI_ERR_RMEM_BUFF_OVERRUN	rmem_offset + (rmem_stride * num_strides 
) would fall out of bounds of the remote memory buffer.
MRAPI_ERR_RMEM_STRIDE
num_strides>1 and rmem_stride and/or local_stride are less than bytes_per_access.
MRAPI_ERR_REQUEST_LIMIT	No more request handles available.
MRAPI_ERR_RMEM_NOTATTACHED	The caller is not attached to the remote memory.
MRAPI_ERR_RMEM_BLOCKED	We have hit a hardware limit of the number 
of asynchronous DMA/cache operations that can be pending ("in 
flight") simultaneously.  Thus we now have to block because the 
resource is busy.
MRAPI_ERR_PARAMETER	Either the local_buf is invalid or the buf_size is zero or bytes_per_access is zero.

NOTE

***********************************************************************/
void mrapi_rmem_read_i(
                       MRAPI_IN mrapi_rmem_hndl_t rmem,
                       MRAPI_IN mrapi_uint32_t rmem_offset,
                       MRAPI_OUT void* local_buf,
                       MRAPI_IN mrapi_uint32_t local_offset,
                       MRAPI_IN mrapi_uint32_t bytes_per_access,
                       MRAPI_IN mrapi_uint32_t num_strides,
                       MRAPI_IN mrapi_uint32_t rmem_stride,
                       MRAPI_IN mrapi_uint32_t local_stride,
                       MRAPI_OUT mrapi_request_t* mrapi_request,
                       MRAPI_OUT mrapi_status_t* status)
{
  

   *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( ! mrapi_impl_valid_rmem_hndl(rmem)) {
    *status = MRAPI_ERR_RMEM_INVALID;
  } else if ((rmem_stride < bytes_per_access) || (local_stride < bytes_per_access)) {
    *status = MRAPI_ERR_RMEM_STRIDE;
  } else if (!mrapi_impl_rmem_attached(rmem)) {
    *status = MRAPI_ERR_RMEM_NOTATTACHED;
  } else {
    mrapi_impl_rmem_read_i (rmem,rmem_offset,local_buf,local_offset,bytes_per_access,num_strides,rmem_stride,local_stride,status,mrapi_request);
  }
}

/************************************************************************
mrapi_rmem_write

DESCRIPTION
This function performs num_strides memory writes, where each 
write is of size bytes_per_access bytes.  The i-th write copies 
bytes_per_access bytes of data from local_buf with offset local_offset 
+ i*local_stride to rmem with offset rmem_offset + i*rmem_stride, 
where 0 <= i < num_strides. 

This supports scatter/gather type accesses.  To perform a single 
write, without the need for scatter/gather,  set the num_strides 
parameter to 1.


RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below.

ERRORS
MRAPI_ERR_RMEM_INVALID
Argument is not a valid remote memory segment handle.
MRAPI_ERR_RMEM_BUFF_OVERRUN	rmem_offset + (rmem_stride * num_strides 
) would fall out of bounds of the remote memory buffer.
MRAPI_ERR_RMEM_STRIDE
num_strides>1 and rmem_stride and/or local_stride are less than bytes_per_access.
MRAPI_ERR_RMEM_NOTATTACHED	The caller is not attached to the remote memory.
MRAPI_ERR_PARAMETER	Either the local_buf is invalid  or bytes_per_access is zero.


NOTE

***********************************************************************/
void mrapi_rmem_write(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_IN mrapi_uint32_t rmem_offset,
 	MRAPI_IN void* local_buf,
 	MRAPI_IN mrapi_uint32_t local_offset,
 	MRAPI_IN mrapi_uint32_t bytes_per_access,
  MRAPI_IN mrapi_uint32_t num_strides,
 	MRAPI_IN mrapi_uint32_t rmem_stride,
 	MRAPI_IN mrapi_uint32_t local_stride,
 	MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( ! mrapi_impl_valid_rmem_hndl(rmem)) {
    *status = MRAPI_ERR_RMEM_INVALID;
  } else if ((rmem_stride < bytes_per_access) || (local_stride < bytes_per_access)) {
    *status = MRAPI_ERR_RMEM_STRIDE;
  } else if (!mrapi_impl_rmem_attached(rmem)) {
    *status = MRAPI_ERR_RMEM_NOTATTACHED;
  } else {
    mrapi_impl_rmem_write (rmem,rmem_offset,local_buf,local_offset,bytes_per_access,num_strides,rmem_stride,local_stride,status); 
  }

}

/************************************************************************
mrapi_rmem_write_i

DESCRIPTION
This (non-blocking) function performs num_strides memory writes, 
where each write is of size bytes_per_access bytes.  The i-th 
write copies bytes_per_access bytes of data from local_buf with 
offset local_offset + i*local_stride to rmem with offset rmem_offset 
+ i*rmem_stride, where 0 <= i < num_strides. 

This supports scatter/gather type accesses.  To perform a single 
write, without the need for scatter/gather,  set the num_strides 
parameter to 1.


RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status 
is set to the appropriate error defined below. Use mrapi_test(), 
mrapi_wait() or mrapi_wait_any() to test for completion of the 
operation.

ERRORS
MRAPI_ERR_RMEM_INVALID
Argument is not a valid remote memory segment handle.
MRAPI_ERR_RMEM_BUFF_OVERRUN	rmem_offset + (rmem_stride * num_strides 
) would fall out of bounds of the remote memory buffer.
MRAPI_ERR_RMEM_STRIDE
num_strides>1 and rmem_stride and/or local_stride are less than bytes_per_access.
MRAPI_ERR_REQUEST_LIMIT	No more request handles available.
MRAPI_ERR_RMEM_NOTATTACHED	The caller is not attached to the remote memory.
MRAPI_ERR_RMEM_BLOCKED	We have hit a hardware limit of the number 
of asynchronous DMA/cache operations that can be pending ("in 
flight") simultaneously.  Thus we now have to block because the 
resource is busy.
MRAPI_ERR_PARAMETER	Either the local_buf is invalid or bytes_per_access is zero.


NOTE

***********************************************************************/
void mrapi_rmem_write_i(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_IN mrapi_uint32_t rmem_offset,
 	MRAPI_IN void* local_buf,
 	MRAPI_IN mrapi_uint32_t local_offset,
 	MRAPI_IN mrapi_uint32_t bytes_per_access,
        MRAPI_IN mrapi_uint32_t num_strides,
 	MRAPI_IN mrapi_uint32_t rmem_stride,
 	MRAPI_IN mrapi_uint32_t local_stride,
 	MRAPI_OUT mrapi_request_t* mrapi_request,
 	MRAPI_OUT mrapi_status_t* status)
{
  

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( ! mrapi_impl_valid_rmem_hndl(rmem)) {
    *status = MRAPI_ERR_RMEM_INVALID;
  } else if ((rmem_stride < bytes_per_access) || (local_stride < bytes_per_access)) {
    *status = MRAPI_ERR_RMEM_STRIDE;
  } else if (!mrapi_impl_rmem_attached(rmem)) {
    *status = MRAPI_ERR_RMEM_NOTATTACHED;
  } else {
    mrapi_impl_rmem_write_i (rmem,rmem_offset,local_buf,local_offset,bytes_per_access,num_strides,rmem_stride,local_stride,status,mrapi_request); 
  }
}

/************************************************************************
mrapi_rmem_flush

DESCRIPTION
This function flushes the remote memory.  Support for this function 
is optional and on some systems this may not be supportable. 
 However, if an implementation wants to support coherency back 
to main backing store then this is the way to do it.  Note, that 
this is not an automatic synch back to other viewers of the remote 
data and they would need to also perform a synch, so it is application 
managed coherency.  If writes are synchronizing, then a flush 
will be a no-op.


RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below. 

ERRORS
MRAPI_ERR_NOT_SUPPORTED	The flush call is not supported
MRAPI_ERR_RMEM_INVALID Argument is not a valid remote memory segment handle.
MRAPI_ERR_RMEM_NOTATTACHED	The caller is not attached to the remote memory.

NOTE

***********************************************************************/
void mrapi_rmem_flush(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_ERR_NOT_SUPPORTED;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (!mrapi_impl_rmem_attached(rmem)) {
    *status = MRAPI_ERR_RMEM_NOTATTACHED;
  } 
}

/************************************************************************
mrapi_rmem_sync

DESCRIPTION
This function synchronizes the remote memory.  This function 
provides application managed coherency.  It does not guarantee 
that all clients of the rmem buffer will see the updates, see 
corresponding mrapi_rmem_flush().  For some underlying hardware 
this may not be possible.  MRAPI implementation can return an 
error if the synch cannot be performed.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below. 

ERRORS
MRAPI_ERR_NOT_SUPPORTED	The synch call is not supported
MRAPI_ERR_RMEM_INVALID Argument is not a valid remote memory segment handle.
MRAPI_ERR_RMEM_NOTATTACHED	The caller is not attached to the remote memory.

NOTE

***********************************************************************/
void mrapi_rmem_sync(
        MRAPI_IN mrapi_rmem_hndl_t rmem,
        MRAPI_OUT mrapi_status_t* status
        )
{
  *status = MRAPI_ERR_NOT_SUPPORTED;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (!mrapi_impl_rmem_attached(rmem)) {
    *status = MRAPI_ERR_RMEM_NOTATTACHED;
  }
}
