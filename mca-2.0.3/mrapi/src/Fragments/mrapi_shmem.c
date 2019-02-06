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
	* Neither the name of the <organization> nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

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
mrapi_shmem_create

DESCRIPTION
This function creates a shared memory segment.  The size parameter 
specifies the size of the shared memory region in bytes.  Unless 
you want the defaults, attributes must be set before the call 
to mrapi_shmem_create().  A list of nodes that can access the 
shared memory can be passed in the nodes parameter and nodes_size 
should contain the number of nodes in the list.  If nodes is 
NULL, then all nodes will be allowed to access the shared memory. 
 Once a shared memory segment has been created, its attributes 
may not be changed.  If the attributes parameter is NULL, then 
implementation defined default attributes will be used.  In the 
case where the shared memory segment already exists, status will 
be set to MRAPI_EXISTS and the handle returned will not be a 
valid handle.  If shmem_id is set to MRAPI_SHMEM_ID_ANY, then 
MRAPI will choose an internal id for you.  All nodes in the nodes 
list must be initialized nodes in the system.

RETURN VALUE
On success a shared memory segment handle is returned, the address 
is filled in and *status is set to MRAPI_SUCCESS.  On error, 
*status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_SHM_INVALID
The shmem_id is not a valid shared memory segment id.

MRAPI_ERR_SHM_NODES_INCOMPAT	The list of nodes is not compatible 
for setting up shared memory.

MRAPI_ERR_SHM_EXISTS	This shared memory segment is already created.

MRAPI_ERR_MEM_LIMIT No memory available.

MRAPI_ERR_NODE_NOTINIT
The calling node is not initialized or one of the nodes in the list of nodes to share with is not initialized.

MRAPI_ERR_PARAMETER
Incorrect size, attributes, attribute_size,  or nodes_size parameter.

NOTE

***********************************************************************/
mrapi_shmem_hndl_t mrapi_shmem_create(
 	MRAPI_IN mrapi_shmem_id_t shmem_id,
 	MRAPI_IN mrapi_uint_t size,
 	MRAPI_IN mrapi_node_t* nodes,
 	MRAPI_IN mrapi_uint_t nodes_size,
 	MRAPI_IN mrapi_shmem_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t attr_size,
 	MRAPI_OUT mrapi_status_t* status)
{
  mrapi_shmem_hndl_t shm = 0;


  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT; /* fixme: how to handle domain_notinit? */
  } else if (!mrapi_impl_shmem_validID(shmem_id)) {
    *status = MRAPI_ERR_SHM_ID_INVALID;
  }else if (mrapi_impl_shmem_exists(shmem_id /*key */)) {
    *status = MRAPI_ERR_SHM_EXISTS;
  } else {
    mrapi_impl_shmem_create(&shm,shmem_id /* key */,size,attributes,status);
  } 
  return shm;
}

/************************************************************************
mrapi_shmem_init_attributes

DESCRIPTION
Unless you want the defaults, this call must be used to initialize 
the values of an mrapi_shmem_attributes_t structure prior to 
mrapi_shmem_set_attribute().  You w would then use mrapi_shmem_set_attribute() 
to change any default values prior to calling mrapi_shmem_create().


RETURN VALUE
On success *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below.

ERRORS
MRAPI_ERR_PARAMETER
Invalid attributes parameter.

NOTE

***********************************************************************/
void mrapi_shmem_init_attributes(
                                 MRAPI_OUT mrapi_shmem_attributes_t* attributes,
                                 MRAPI_OUT mrapi_status_t* status)
{
  if (attributes == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else {
    mrapi_impl_shmem_init_attributes(attributes);
  }
}

/************************************************************************
mrapi_shmem_set_attribute

DESCRIPTION
This function is used to change default values of an mrapi_shmem_attributes_t 
data structure prior to calling mrapi_shmem_create().  If the 
user wants to control which physical memory is used, then that 
is done by setting the MRAPI_SHMEM_RESOURCE attribute to the 
resource in the metadata tree.  The user would first need to 
call mrapi_resources_get() and then iterate over the tree to 
find the desired resource (see the example use case for more 
details).

MRAPI pre-defined shared memory attributes:
Attribute num:	Description:	Datatype:	Default:
MRAPI_SHMEM_RESOURCE	The physical memory resource in the metadata 
resource tree that the memory should be allocated from. 	
mrapi_resource_t	MRAPI_SHMEM_ANY

MRAPI_SHMEM_ADDRESS	The requested address for a shared memory region	
mrapi_uint_t	MRAPI_SHMEM_ANY

MRAPI_DOMAIN_SHARED	Indicates whether or not this remote memory 
is shareable across domains.  	
mrapi_boolean_t	MRAPI_TRUE

MRAPI_SHMEM_SIZE	Returns the size of the shared memory segment 
in bytes.  This attribute can only be set through the size parameter 
passed in to create.	
mrapi_size_t	No default.

MRAPI_SHMEM_ADDRESS	if MRAPI_SHMEM_ANY then not necessarily contiguous, 
if <address> then contiguous;non-contiguous should be used with 
care and will not work in contexts that cannot handle virtual 
memory	
mrapi_addr_t	MRAPI_SHMEM_ANY_CONTIGUOUS


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
void mrapi_shmem_set_attribute(
 	MRAPI_OUT mrapi_shmem_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_IN void* attribute,
 	MRAPI_IN size_t attr_size,
 	MRAPI_OUT mrapi_status_t* status)
{


  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (attributes == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else {
    mrapi_impl_shmem_set_attribute(attributes,attribute_num,attribute,attr_size,status);
  }
}

/************************************************************************
mrapi_shmem_get_attribute

DESCRIPTION
Returns the attribute that corresponds to the given attribute_num 
for this shared memory.  The attributes may be viewed but may 
not be changed (for this shared memory).

RETURN VALUE
On success *status is set to MRAPI_SUCCESS and the attribute 
value is filled in.  On error, *status is set to the appropriate 
error defined below and the attribute value is undefined.  The 
attribute identified by the attribute_num is returned in the 
void* attribute parameter.

ERRORS
MRAPI_ERR_PARAMETER Invalid attribute parameter.
MRAPI_ERR_SHM_INVALID Argument is not a valid shmem handle.
MRAPI_ERR_ATTR_NUM Unknown attribute number
MRAPI_ERR_ATTR_SIZE Incorrect attribute size

NOTE

***********************************************************************/
void mrapi_shmem_get_attribute(
 	MRAPI_IN mrapi_shmem_hndl_t shmem,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_OUT void* attribute,
 	MRAPI_IN size_t attribute_size,
 	MRAPI_OUT mrapi_status_t* status)
{
  if (attribute == NULL) {
    *status = MRAPI_ERR_PARAMETER;
  } else if ( ! mrapi_impl_valid_shmem_hndl(shmem)) {
    *status = MRAPI_ERR_SHM_INVALID;
  }else {
    mrapi_impl_shmem_get_attribute(shmem,attribute_num,attribute,attribute_size,status);
  }
}

/************************************************************************
mrapi_shmem_get

DESCRIPTION
Given a shmem_id this function returns the MRAPI handle for referencing that 
shared memory segment.

RETURN VALUE
On success the shared memory segment handle is returned and *status 
is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate 
error defined below.

ERRORS
MRAPI_ERR_SHM_ID_INVALID The shmem_id is not a valid shared memory id or it 
was called with shmem_id set to MRAPI_SHMEM_ID_ANY.

MRAPI_ERR_NODE_NOTINIT The calling node is not initialized.

MRAPI_ERR_SHM_NODE_NOTSHARED	This shared memory is not shareable 
with the calling node.  Which nodes it is shareable with was 
specified on the call to mrapi_shmem_create().

MRAPI_ERR_DOMAIN_NOTSHARED	This resource can not be shared by this domain.


NOTE
Shared memory is the only MRAPI primitive that is always shareable 
across domains.  Which nodes it is shared with is specified in 
the call to mrapi_shmem_create().

***********************************************************************/
mrapi_shmem_hndl_t mrapi_shmem_get(
 	MRAPI_IN mrapi_shmem_id_t shmem_id,
 	MRAPI_OUT mrapi_status_t* status)
{
  mrapi_shmem_hndl_t shmem = 0;


 
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (!mrapi_impl_shmem_exists(shmem_id)) {
    *status = MRAPI_ERR_SHM_ID_INVALID;
  } else if (mrapi_impl_shmem_get(&shmem,shmem_id /*key */)) {
    *status = MRAPI_SUCCESS;
  } else {
    *status = MRAPI_ERR_NODE_NOTINIT;
  }
  return shmem;
}

/************************************************************************
mrapi_shmem_attach

DESCRIPTION
This function attaches the caller to the shared memory segment and returns its address.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below.

ERRORS
MRAPI_ERR_SHM_INVALID Argument is not a valid shared memory segment handle.
MRAPI_ERR_SHM_ATTACHED	The calling node is already attached to the shared memory.

NOTE

***********************************************************************/
void* mrapi_shmem_attach(
 	MRAPI_IN mrapi_shmem_hndl_t shmem,
 	MRAPI_OUT mrapi_status_t* status)
{
  void* addr = NULL;

  *status = MRAPI_ERR_SHM_INVALID; 
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( ! mrapi_impl_valid_shmem_hndl(shmem)) {
    *status = MRAPI_ERR_SHM_INVALID;
  } else if (mrapi_impl_shmem_attached(shmem)) {
    *status = MRAPI_ERR_SHM_ATTACHED;
  } else {
    addr = mrapi_impl_shmem_attach(shmem);
    if ( addr != NULL) {
      *status = MRAPI_SUCCESS;
    } 
  }
  return addr;
}

/************************************************************************
mrapi_shmem_detach

DESCRIPTION
This function detaches the caller from the shared memory segment. 
 All nodes must detach before any node can delete the memory.


RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the 
appropriate error defined below.

ERRORS
MRAPI_ERR_SHM_INVALID Argument is not a valid shared memory segment handle.
MRAPI_ERR_SHM_NOTATTACHED	The calling node is not attached to the shared memory.


NOTE

***********************************************************************/
void mrapi_shmem_detach(
 	MRAPI_IN mrapi_shmem_hndl_t shmem,
 	MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_ERR_SHM_INVALID;
  
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (!mrapi_impl_valid_shmem_hndl(shmem)) {
  } else if (!mrapi_impl_shmem_attached(shmem)) {
    *status = MRAPI_ERR_SHM_NOTATTACHED;
  } else if (mrapi_impl_shmem_detach(shmem)) {
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_shmem_delete

DESCRIPTION
This function deletes the shared memory segment if there are 
no nodes still attached to it.  All nodes must detach before 
any node can delete the memory.  Otherwise, delete will fail 
and there are no automatic retries nor deferred delete.


RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_SHM_INVALID
Argument is not a valid shared memory segment handle.
MRAPI_ERR_SHM_ATTCH
There are nodes still attached to this shared memory segment thus it could not be deleted.


NOTE

***********************************************************************/
void mrapi_shmem_delete(
 	MRAPI_IN mrapi_shmem_hndl_t shmem,
 	MRAPI_OUT mrapi_status_t* status)
{

  
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if ( ! mrapi_impl_valid_shmem_hndl(shmem)) {
    *status = MRAPI_ERR_SHM_INVALID;
  }  else if (mrapi_impl_shmem_delete (shmem)) {
    *status = MRAPI_SUCCESS;
  } 
}
