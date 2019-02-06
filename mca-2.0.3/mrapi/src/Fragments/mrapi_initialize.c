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
mrapi_initialize

DESCRIPTION
mrapi_initialize() initializes the MRAPI environment on a given 
MRAPI node in a given MRAPI domain. It has to be called by each 
node using MRAPI. mrapi_parameters is used to pass implementation 
specific initialization parameters. mrapi_info is used to obtain 
information from the MRAPI implementation, including MRAPI and 
the underlying implementation version numbers, implementation 
vendor identification, the number of nodes in the topology, the 
number of ports on the local node and vendor specific implementation 
information, see the header files for additional information. 
A node is a process, a thread, or a processor (or core) with 
an independent program counter running a piece of code. In other 
words, an MRAPI node is an independent thread of control. An 
MRAPI node can call mrapi_initialize() once per node, and it 
is an error to call mrapi_initialize() multiple times from a 
given node, unless mrapi_finalize() is called in between. A given 
MRAPI implementation will specify what is a node (i.e., what 
thread of control  process, thread, or other -- is a node) in 
that implementation. A thread and process are just two examples 
of threads of control, and there could be others. 

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_NOT_INITFAILED 	The MRAPI environment could not be initialized.
MRAPI_ERR_NODE_INITIALIZED 	The MRAPI environment has already been initialized.
MRAPI_ERR_NODE_INVALID The node_id parameter is not valid.
MRAPI_ERR_DOMAIN_INVALID The domain_id parameter is not valid.
MRAPI_ERR_PARAMETER Invalid mrapi_parameters or mrapi_info  parameter.

NOTE

***********************************************************************/
void mrapi_initialize(
 	MRAPI_IN mrapi_domain_t domain_id,
 	MRAPI_IN mrapi_node_t node_id,
 	MRAPI_IN mrapi_parameters_t init_parameters,
 	MRAPI_OUT mrapi_info_t* mrapi_info,
 	MRAPI_OUT mrapi_status_t* status)
{
  *status = MRAPI_ERR_NODE_INITFAILED;

  if (!mrapi_impl_valid_parameters_param(init_parameters)) {
    *status = MRAPI_ERR_PARAMETER;
  } else if (!mrapi_impl_valid_info_param(mrapi_info)) {
    *status = MRAPI_ERR_PARAMETER;
  } else if (!mrapi_impl_valid_node_num(node_id)) {
    *status = MRAPI_ERR_NODE_INVALID;
  } else if (!mrapi_impl_valid_domain_num(domain_id)) {
    *status = MRAPI_ERR_DOMAIN_INVALID;
  } else if (mrapi_impl_initialized(domain_id,node_id)) {
    *status = MRAPI_ERR_NODE_INITIALIZED;
  } else if (mrapi_impl_initialize(domain_id,node_id,status)) {
#if (__unix__||__MINGW32__)
    (void)strncpy(mrapi_info->mrapi_version,MRAPI_VERSION,sizeof(MRAPI_VERSION));
#else
    (void)strncpy_s(mrapi_info->mrapi_version,sizeof(mrapi_info->mrapi_version),MRAPI_VERSION,sizeof(MRAPI_VERSION));
#endif  /* !(__unix__||__MINGW32__) */
    /*printf("MRAPI VERSION=%s\n",mrapi_info->mrapi_version);*/
    *status = MRAPI_SUCCESS;
  } 
}

/************************************************************************
mrapi_finalize

DESCRIPTION
mrapi_finalize() finalizes the MRAPI environment on a given MRAPI 
node and domain. It has to be called by each node using MRAPI. 
 It is an error to call mrapi_finalize() without first calling 
mrapi_initialize().  An MRAPI node can call mrapi_finalize() 
once for each call to mrapi_initialize(), but it is an error 
to call mrapi_finalize() multiple times from a given <domain,node> 
unless mrapi_initialize() has been called prior to each mrapi_finalize() 
call.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_NODE_FINALFAILED The MRAPI environment could not be finalized.

NOTE

***********************************************************************/
void mrapi_finalize(
                    MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (! mrapi_impl_finalize()) {
    *status = MRAPI_ERR_NODE_FINALFAILED;
  }
}

/************************************************************************
mrapi_domain_id_get

DESCRIPTION
Returns the domain id associated with the local node.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_NODE_NOTINIT The calling node is not intialized.

NOTE

***********************************************************************/
mrapi_domain_t mrapi_domain_id_get(
 	MRAPI_OUT mrapi_status_t* status)
{
  mca_domain_t domain = MRAPI_DOMAIN_INVALID;
  *status = MRAPI_SUCCESS;

  if  (!mrapi_impl_get_domain_num(&domain)) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  }
  return domain;
}

/************************************************************************
mrapi_node_id_get

DESCRIPTION
Returns the node id associated with the local node and domain.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_NODE_NOTINIT The calling node is not intialized.

NOTE

***********************************************************************/
mrapi_node_t mrapi_node_id_get(
 	MRAPI_OUT mrapi_status_t* status)
{
  // Implementation moved to mrapi_impl/mrapi_impl_spec.c
  return mrapi_impl_node_id_get(status);
}
