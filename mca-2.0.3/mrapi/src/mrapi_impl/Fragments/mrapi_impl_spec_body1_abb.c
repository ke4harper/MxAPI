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

/*-------------------------------------------------------------------
	the mrapi_impl API function definitions
	-------------------------------------------------------------------*/

	/***************************************************************************
	Function:  mrapi_impl_display_status

	Description:  formats the status parameter as a string by copying it into the
	  user supplied buffer

	Parameters:

	Returns:

	Note: This convenience function implementation was moved from mrapi.c to
	  eliminate circular dependencies between the layers.

	***************************************************************************/
char* mrapi_impl_display_status(mrapi_status_t status, char* status_message, size_t size) {
	if ((size < MRAPI_MAX_STATUS_SIZE) || (status_message == NULL)) {
		fprintf(stderr, "ERROR: size passed to mrapi_display_status must be at least %d and status_message must not be NULL.\n", MRAPI_MAX_STATUS_SIZE);
		return status_message;
	}
	memset(status_message, 0, size);
	switch (status) {
#if (__unix__||__MINGW32__)
	case (MRAPI_SUCCESS): strcpy(status_message, "MRAPI_SUCCESS"); return(status_message); break;
	case (MRAPI_TIMEOUT): strcpy(status_message, "MRAPI_TIMEOUT"); return(status_message); break;
	case (MRAPI_INCOMPLETE): strcpy(status_message, "MRAPI_INCOMPLETE"); return(status_message); break;
	case (MRAPI_ERR_ATTR_NUM): strcpy(status_message, "MRAPI_ERR_ATTR_NUM"); return(status_message); break;
	case (MRAPI_ERR_ATTR_READONLY): strcpy(status_message, "MRAPI_ERR_ATTR_READONLY"); return(status_message); break;
	case (MRAPI_ERR_ATTR_SIZE): strcpy(status_message, "MRAPI_ERR_ATTR_SIZE"); return(status_message); break;
	case (MRAPI_ERR_DOMAIN_INVALID): strcpy(status_message, "MRAPI_ERR_DOMAIN_INVALID"); return(status_message); break;
	case (MRAPI_ERR_DOMAIN_NOTSHARED): strcpy(status_message, "MRAPI_ERR_DOMAIN_NOTSHARED"); return(status_message); break;
	case (MRAPI_ERR_MEM_LIMIT): strcpy(status_message, "MRAPI_ERR_MEM_LIMIT"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_DELETED): strcpy(status_message, "MRAPI_ERR_MUTEX_DELETED"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_EXISTS): strcpy(status_message, "MRAPI_ERR_MUTEX_EXISTS"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_ID_INVALID): strcpy(status_message, "MRAPI_ERR_MUTEX_ID_INVALID"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_INVALID): strcpy(status_message, "MRAPI_ERR_MUTEX_INVALID"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_KEY): strcpy(status_message, "MRAPI_ERR_MUTEX_KEY"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_LIMIT): strcpy(status_message, "MRAPI_ERR_MUTEX_LIMIT"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_LOCKED): strcpy(status_message, "MRAPI_ERR_MUTEX_LOCKED"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_LOCKORDER): strcpy(status_message, "MRAPI_ERR_MUTEX_LOCKORDER"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_NOTLOCKED): strcpy(status_message, "MRAPI_ERR_MUTEX_NOTLOCKED"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_NOTVALID): strcpy(status_message, "MRAPI_ERR_MUTEX_NOTVALID"); return(status_message); break;
	case (MRAPI_ERR_NODE_FINALFAILED): strcpy(status_message, "MRAPI_ERR_NODE_FINALFAILED"); return(status_message); break;
	case (MRAPI_ERR_NODE_INITIALIZED): strcpy(status_message, "MRAPI_ERR_NODE_INITIALIZED"); return(status_message); break;
	case (MRAPI_ERR_NODE_INVALID): strcpy(status_message, "MRAPI_ERR_NODE_INVALID"); return(status_message); break;
	case (MRAPI_ERR_NODE_NOTINIT): strcpy(status_message, "MRAPI_ERR_NODE_NOTINIT"); return(status_message); break;
	case (MRAPI_ERR_NOT_SUPPORTED): strcpy(status_message, "MRAPI_ERR_NOT_SUPPORTED"); return(status_message); break;
	case (MRAPI_ERR_PARAMETER): strcpy(status_message, "MRAPI_ERR_PARAMETER"); return(status_message); break;
	case (MRAPI_ERR_REQUEST_CANCELED): strcpy(status_message, "MRAPI_ERR_REQUEST_CANCELED"); return(status_message); break;
	case (MRAPI_ERR_REQUEST_INVALID): strcpy(status_message, "MRAPI_ERR_REQUEST_INVALID"); return(status_message); break;
	case (MRAPI_ERR_REQUEST_LIMIT): strcpy(status_message, "MRAPI_ERR_REQUEST_LIMIT"); return(status_message); break;
	case (MRAPI_ERR_RMEM_ID_INVALID): strcpy(status_message, "MRAPI_ERR_RMEMID_INVALID"); return(status_message); break;
	case (MRAPI_ERR_RMEM_ATTACH): strcpy(status_message, "MRAPI_ERR_RMEM_ATTACH"); return(status_message); break;
	case (MRAPI_ERR_RMEM_ATTACHED): strcpy(status_message, "MRAPI_ERR_RMEM_ATTACHED"); return(status_message); break;
	case (MRAPI_ERR_RMEM_ATYPE): strcpy(status_message, "MRAPI_ERR_RMEM_ATYPE"); return(status_message); break;
	case (MRAPI_ERR_RMEM_ATYPE_NOTVALID): strcpy(status_message, "MRAPI_ERR_RMEM_ATYPE_NOTVALID"); return(status_message); break;
	case (MRAPI_ERR_RMEM_BLOCKED): strcpy(status_message, "MRAPI_ERR_RMEM_BLOCKED"); return(status_message); break;
	case (MRAPI_ERR_RMEM_BUFF_OVERRUN): strcpy(status_message, "MRAPI_ERR_RMEM_BUFF_OVERRUN"); return(status_message); break;
	case (MRAPI_ERR_RMEM_CONFLICT): strcpy(status_message, "MRAPI_ERR_RMEM_CONFLICT"); return(status_message); break;
	case (MRAPI_ERR_RMEM_EXISTS): strcpy(status_message, "MRAPI_ERR_RMEM_EXISTS"); return(status_message); break;
	case (MRAPI_ERR_RMEM_INVALID): strcpy(status_message, "MRAPI_ERR_RMEM_INVALID"); return(status_message); break;
	case (MRAPI_ERR_RMEM_NOTATTACHED): strcpy(status_message, "MRAPI_ERR_RMEM_NOTATTACHED"); return(status_message); break;
	case (MRAPI_ERR_RMEM_NOTOWNER): strcpy(status_message, "MRAPI_ERR_RMEM_NOTOWNER"); return(status_message); break;
	case (MRAPI_ERR_RMEM_STRIDE): strcpy(status_message, "MRAPI_ERR_RMEM_STRIDE"); return(status_message); break;
	case (MRAPI_ERR_RMEM_TYPENOTVALID): strcpy(status_message, "MRAPI_ERR_RMEM_TYPENOTVALID"); return(status_message); break;
	case (MRAPI_ERR_RSRC_COUNTER_INUSE): strcpy(status_message, "MRAPI_ERR_RSRC_COUNTER_INUSE"); return(status_message); break;
	case (MRAPI_ERR_RSRC_INVALID): strcpy(status_message, "MRAPI_ERR_RSRC_INVALID"); return(status_message); break;
	case (MRAPI_ERR_RSRC_INVALID_CALLBACK): strcpy(status_message, "MRAPI_ERR_RSRC_INVALID_CALLBACK"); return(status_message); break;
	case (MRAPI_ERR_RSRC_INVALID_EVENT): strcpy(status_message, "MRAPI_ERR_RSRC_INVALID_EVENT"); return(status_message); break;
	case (MRAPI_ERR_RSRC_INVALID_SUBSYSTEM): strcpy(status_message, "MRAPI_ERR_RSRC_INVALID_SUBSYSTEM"); return(status_message); break;
	case (MRAPI_ERR_RSRC_INVALID_TREE): strcpy(status_message, "MRAPI_ERR_RSRC_INVALID_TREE"); return(status_message); break;
	case (MRAPI_ERR_RSRC_NOTDYNAMIC): strcpy(status_message, "MRAPI_ERR_RSRC_NOTDYNAMIC"); return(status_message); break;
	case (MRAPI_ERR_RSRC_NOTOWNER): strcpy(status_message, "MRAPI_ERR_RSRC_NOTOWNER"); return(status_message); break;
	case (MRAPI_ERR_RSRC_NOTSTARTED): strcpy(status_message, "MRAPI_ERR_RSRC_NOTSTARTED"); return(status_message); break;
	case (MRAPI_ERR_RSRC_STARTED): strcpy(status_message, "MRAPI_ERR_RSRC_STARTED"); return(status_message); break;
	case (MRAPI_ERR_RWL_DELETED): strcpy(status_message, "MRAPI_ERR_RWL_DELETED"); return(status_message); break;
	case (MRAPI_ERR_RWL_EXISTS): strcpy(status_message, "MRAPI_ERR_RWL_EXISTS"); return(status_message); break;
	case (MRAPI_ERR_RWL_ID_INVALID): strcpy(status_message, "MRAPI_ERR_RWL_ID_INVALID"); return(status_message); break;
	case (MRAPI_ERR_RWL_INVALID): strcpy(status_message, "MRAPI_ERR_RWL_INVALID"); return(status_message); break;
	case (MRAPI_ERR_RWL_LIMIT): strcpy(status_message, "MRAPI_ERR_RWL_LIMIT"); return(status_message); break;
	case (MRAPI_ERR_RWL_LOCKED): strcpy(status_message, "MRAPI_ERR_RWL_LOCKED"); return(status_message); break;
	case (MRAPI_ERR_RWL_NOTLOCKED): strcpy(status_message, "MRAPI_ERR_RWL_NOTLOCKED"); return(status_message); break;
	case (MRAPI_ERR_SEM_DELETED): strcpy(status_message, "MRAPI_ERR_SEM_DELETED"); return(status_message); break;
	case (MRAPI_ERR_SEM_EXISTS): strcpy(status_message, "MRAPI_ERR_SEM_EXISTS"); return(status_message); break;
	case (MRAPI_ERR_SEM_ID_INVALID): strcpy(status_message, "MRAPI_ERR_SEM_ID_INVALID"); return(status_message); break;
	case (MRAPI_ERR_SEM_INVALID): strcpy(status_message, "MRAPI_ERR_SEM_INVALID"); return(status_message); break;
	case (MRAPI_ERR_SEM_LIMIT): strcpy(status_message, "MRAPI_ERR_SEM_LIMIT"); return(status_message); break;
	case (MRAPI_ERR_SEM_LOCKED): strcpy(status_message, "MRAPI_ERR_SEM_LOCKED"); return(status_message); break;
	case (MRAPI_ERR_SEM_LOCKLIMIT): strcpy(status_message, "MRAPI_ERR_SEM_LOCKLIMIT"); return(status_message); break;
	case (MRAPI_ERR_SEM_NOTLOCKED): strcpy(status_message, "MRAPI_ERR_SEM_NOTLOCKED"); return(status_message); break;
	case (MRAPI_ERR_SHM_ATTACHED): strcpy(status_message, "MRAPI_ERR_SHMEM_ATTACHED"); return(status_message); break;
	case (MRAPI_ERR_SHM_ATTCH): strcpy(status_message, "MRAPI_ERR_SHMEM_ATTCH"); return(status_message); break;
	case (MRAPI_ERR_SHM_EXISTS): strcpy(status_message, "MRAPI_ERR_SHMEM_EXISTS"); return(status_message); break;
	case (MRAPI_ERR_SHM_ID_INVALID): strcpy(status_message, "MRAPI_ERR_SHMEM_ID_INVALID"); return(status_message); break;
	case (MRAPI_ERR_SHM_INVALID): strcpy(status_message, "MRAPI_ERR_SHM_INVALID"); return(status_message); break;
	case (MRAPI_ERR_SHM_NODES_INCOMPAT): strcpy(status_message, "MRAPI_ERR_SHM_NODES_INCOMPAT"); return(status_message); break;
	case (MRAPI_ERR_SHM_NODE_NOTSHARED): strcpy(status_message, "MRAPI_ERR_SHM_NODE_NOTSHARED"); return(status_message); break;
	case (MRAPI_ERR_SHM_NOTATTACHED): strcpy(status_message, "MRAPI_ERR_SHM_NOTATTACHED"); return(status_message); break;
	default: strcpy(status_message, "UNKNOWN ERROR"); return(status_message); break;
#else
	case (MRAPI_SUCCESS): strcpy_s(status_message, size, "MRAPI_SUCCESS"); return(status_message); break;
	case (MRAPI_TIMEOUT): strcpy_s(status_message, size, "MRAPI_TIMEOUT"); return(status_message); break;
	case (MRAPI_INCOMPLETE): strcpy_s(status_message, size, "MRAPI_INCOMPLETE"); return(status_message); break;
	case (MRAPI_ERR_ATTR_NUM): strcpy_s(status_message, size, "MRAPI_ERR_ATTR_NUM"); return(status_message); break;
	case (MRAPI_ERR_ATTR_READONLY): strcpy_s(status_message, size, "MRAPI_ERR_ATTR_READONLY"); return(status_message); break;
	case (MRAPI_ERR_ATTR_SIZE): strcpy_s(status_message, size, "MRAPI_ERR_ATTR_SIZE"); return(status_message); break;
	case (MRAPI_ERR_DOMAIN_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_DOMAIN_INVALID"); return(status_message); break;
	case (MRAPI_ERR_DOMAIN_NOTSHARED): strcpy_s(status_message, size, "MRAPI_ERR_DOMAIN_NOTSHARED"); return(status_message); break;
	case (MRAPI_ERR_MEM_LIMIT): strcpy_s(status_message, size, "MRAPI_ERR_MEM_LIMIT"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_DELETED): strcpy_s(status_message, size, "MRAPI_ERR_MUTEX_DELETED"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_EXISTS): strcpy_s(status_message, size, "MRAPI_ERR_MUTEX_EXISTS"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_ID_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_MUTEX_ID_INVALID"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_MUTEX_INVALID"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_KEY): strcpy_s(status_message, size, "MRAPI_ERR_MUTEX_KEY"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_LIMIT): strcpy_s(status_message, size, "MRAPI_ERR_MUTEX_LIMIT"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_LOCKED): strcpy_s(status_message, size, "MRAPI_ERR_MUTEX_LOCKED"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_LOCKORDER): strcpy_s(status_message, size, "MRAPI_ERR_MUTEX_LOCKORDER"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_NOTLOCKED): strcpy_s(status_message, size, "MRAPI_ERR_MUTEX_NOTLOCKED"); return(status_message); break;
	case (MRAPI_ERR_MUTEX_NOTVALID): strcpy_s(status_message, size, "MRAPI_ERR_MUTEX_NOTVALID"); return(status_message); break;
	case (MRAPI_ERR_NODE_FINALFAILED): strcpy_s(status_message, size, "MRAPI_ERR_NODE_FINALFAILED"); return(status_message); break;
	case (MRAPI_ERR_NODE_INITIALIZED): strcpy_s(status_message, size, "MRAPI_ERR_NODE_INITIALIZED"); return(status_message); break;
	case (MRAPI_ERR_NODE_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_NODE_INVALID"); return(status_message); break;
	case (MRAPI_ERR_NODE_NOTINIT): strcpy_s(status_message, size, "MRAPI_ERR_NODE_NOTINIT"); return(status_message); break;
	case (MRAPI_ERR_NOT_SUPPORTED): strcpy_s(status_message, size, "MRAPI_ERR_NOT_SUPPORTED"); return(status_message); break;
	case (MRAPI_ERR_PARAMETER): strcpy_s(status_message, size, "MRAPI_ERR_PARAMETER"); return(status_message); break;
	case (MRAPI_ERR_REQUEST_CANCELED): strcpy_s(status_message, size, "MRAPI_ERR_REQUEST_CANCELED"); return(status_message); break;
	case (MRAPI_ERR_REQUEST_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_REQUEST_INVALID"); return(status_message); break;
	case (MRAPI_ERR_REQUEST_LIMIT): strcpy_s(status_message, size, "MRAPI_ERR_REQUEST_LIMIT"); return(status_message); break;
	case (MRAPI_ERR_RMEM_ID_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_RMEMID_INVALID"); return(status_message); break;
	case (MRAPI_ERR_RMEM_ATTACH): strcpy_s(status_message, size, "MRAPI_ERR_RMEM_ATTACH"); return(status_message); break;
	case (MRAPI_ERR_RMEM_ATTACHED): strcpy_s(status_message, size, "MRAPI_ERR_RMEM_ATTACHED"); return(status_message); break;
	case (MRAPI_ERR_RMEM_ATYPE): strcpy_s(status_message, size, "MRAPI_ERR_RMEM_ATYPE"); return(status_message); break;
	case (MRAPI_ERR_RMEM_ATYPE_NOTVALID): strcpy_s(status_message, size, "MRAPI_ERR_RMEM_ATYPE_NOTVALID"); return(status_message); break;
	case (MRAPI_ERR_RMEM_BLOCKED): strcpy_s(status_message, size, "MRAPI_ERR_RMEM_BLOCKED"); return(status_message); break;
	case (MRAPI_ERR_RMEM_BUFF_OVERRUN): strcpy_s(status_message, size, "MRAPI_ERR_RMEM_BUFF_OVERRUN"); return(status_message); break;
	case (MRAPI_ERR_RMEM_CONFLICT): strcpy_s(status_message, size, "MRAPI_ERR_RMEM_CONFLICT"); return(status_message); break;
	case (MRAPI_ERR_RMEM_EXISTS): strcpy_s(status_message, size, "MRAPI_ERR_RMEM_EXISTS"); return(status_message); break;
	case (MRAPI_ERR_RMEM_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_RMEM_INVALID"); return(status_message); break;
	case (MRAPI_ERR_RMEM_NOTATTACHED): strcpy_s(status_message, size, "MRAPI_ERR_RMEM_NOTATTACHED"); return(status_message); break;
	case (MRAPI_ERR_RMEM_NOTOWNER): strcpy_s(status_message, size, "MRAPI_ERR_RMEM_NOTOWNER"); return(status_message); break;
	case (MRAPI_ERR_RMEM_STRIDE): strcpy_s(status_message, size, "MRAPI_ERR_RMEM_STRIDE"); return(status_message); break;
	case (MRAPI_ERR_RMEM_TYPENOTVALID): strcpy_s(status_message, size, "MRAPI_ERR_RMEM_TYPENOTVALID"); return(status_message); break;
	case (MRAPI_ERR_RSRC_COUNTER_INUSE): strcpy_s(status_message, size, "MRAPI_ERR_RSRC_COUNTER_INUSE"); return(status_message); break;
	case (MRAPI_ERR_RSRC_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_RSRC_INVALID"); return(status_message); break;
	case (MRAPI_ERR_RSRC_INVALID_CALLBACK): strcpy_s(status_message, size, "MRAPI_ERR_RSRC_INVALID_CALLBACK"); return(status_message); break;
	case (MRAPI_ERR_RSRC_INVALID_EVENT): strcpy_s(status_message, size, "MRAPI_ERR_RSRC_INVALID_EVENT"); return(status_message); break;
	case (MRAPI_ERR_RSRC_INVALID_SUBSYSTEM): strcpy_s(status_message, size, "MRAPI_ERR_RSRC_INVALID_SUBSYSTEM"); return(status_message); break;
	case (MRAPI_ERR_RSRC_INVALID_TREE): strcpy_s(status_message, size, "MRAPI_ERR_RSRC_INVALID_TREE"); return(status_message); break;
	case (MRAPI_ERR_RSRC_NOTDYNAMIC): strcpy_s(status_message, size, "MRAPI_ERR_RSRC_NOTDYNAMIC"); return(status_message); break;
	case (MRAPI_ERR_RSRC_NOTOWNER): strcpy_s(status_message, size, "MRAPI_ERR_RSRC_NOTOWNER"); return(status_message); break;
	case (MRAPI_ERR_RSRC_NOTSTARTED): strcpy_s(status_message, size, "MRAPI_ERR_RSRC_NOTSTARTED"); return(status_message); break;
	case (MRAPI_ERR_RSRC_STARTED): strcpy_s(status_message, size, "MRAPI_ERR_RSRC_STARTED"); return(status_message); break;
	case (MRAPI_ERR_RWL_DELETED): strcpy_s(status_message, size, "MRAPI_ERR_RWL_DELETED"); return(status_message); break;
	case (MRAPI_ERR_RWL_EXISTS): strcpy_s(status_message, size, "MRAPI_ERR_RWL_EXISTS"); return(status_message); break;
	case (MRAPI_ERR_RWL_ID_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_RWL_ID_INVALID"); return(status_message); break;
	case (MRAPI_ERR_RWL_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_RWL_INVALID"); return(status_message); break;
	case (MRAPI_ERR_RWL_LIMIT): strcpy_s(status_message, size, "MRAPI_ERR_RWL_LIMIT"); return(status_message); break;
	case (MRAPI_ERR_RWL_LOCKED): strcpy_s(status_message, size, "MRAPI_ERR_RWL_LOCKED"); return(status_message); break;
	case (MRAPI_ERR_RWL_NOTLOCKED): strcpy_s(status_message, size, "MRAPI_ERR_RWL_NOTLOCKED"); return(status_message); break;
	case (MRAPI_ERR_SEM_DELETED): strcpy_s(status_message, size, "MRAPI_ERR_SEM_DELETED"); return(status_message); break;
	case (MRAPI_ERR_SEM_EXISTS): strcpy_s(status_message, size, "MRAPI_ERR_SEM_EXISTS"); return(status_message); break;
	case (MRAPI_ERR_SEM_ID_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_SEM_ID_INVALID"); return(status_message); break;
	case (MRAPI_ERR_SEM_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_SEM_INVALID"); return(status_message); break;
	case (MRAPI_ERR_SEM_LIMIT): strcpy_s(status_message, size, "MRAPI_ERR_SEM_LIMIT"); return(status_message); break;
	case (MRAPI_ERR_SEM_LOCKED): strcpy_s(status_message, size, "MRAPI_ERR_SEM_LOCKED"); return(status_message); break;
	case (MRAPI_ERR_SEM_LOCKLIMIT): strcpy_s(status_message, size, "MRAPI_ERR_SEM_LOCKLIMIT"); return(status_message); break;
	case (MRAPI_ERR_SEM_NOTLOCKED): strcpy_s(status_message, size, "MRAPI_ERR_SEM_NOTLOCKED"); return(status_message); break;
	case (MRAPI_ERR_SHM_ATTACHED): strcpy_s(status_message, size, "MRAPI_ERR_SHMEM_ATTACHED"); return(status_message); break;
	case (MRAPI_ERR_SHM_ATTCH): strcpy_s(status_message, size, "MRAPI_ERR_SHMEM_ATTCH"); return(status_message); break;
	case (MRAPI_ERR_SHM_EXISTS): strcpy_s(status_message, size, "MRAPI_ERR_SHMEM_EXISTS"); return(status_message); break;
	case (MRAPI_ERR_SHM_ID_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_SHMEM_ID_INVALID"); return(status_message); break;
	case (MRAPI_ERR_SHM_INVALID): strcpy_s(status_message, size, "MRAPI_ERR_SHM_INVALID"); return(status_message); break;
	case (MRAPI_ERR_SHM_NODES_INCOMPAT): strcpy_s(status_message, size, "MRAPI_ERR_SHM_NODES_INCOMPAT"); return(status_message); break;
	case (MRAPI_ERR_SHM_NODE_NOTSHARED): strcpy_s(status_message, size, "MRAPI_ERR_SHM_NODE_NOTSHARED"); return(status_message); break;
	case (MRAPI_ERR_SHM_NOTATTACHED): strcpy_s(status_message, size, "MRAPI_ERR_SHM_NOTATTACHED"); return(status_message); break;
	default: strcpy_s(status_message, size, "UNKNOWN ERROR"); return(status_message); break;
#endif  /* !(__unix__||__MINGW32__) */
	};
}

/***************************************************************************
Function:  mrapi_impl_lock_type_get

Description:  determine semaphore lock: RWL, SEM, MUTEX

Parameters:

Returns:	lock type

***************************************************************************/
mrapi_lock_type mrapi_impl_lock_type_get(uint32_t hndl, mrapi_status_t* status)
{
	uint16_t s, d;
	mrapi_lock_type type = MRAPI_LOCK_UNKNOWN;

	if (!mrapi_impl_decode_hndl(hndl, &s) || (s >= MRAPI_MAX_SEMS)) {
		*status = MRAPI_FALSE;
	}
	else
	{
		if (mrapi_db->sems[s].valid == MRAPI_TRUE) {
			*status = MRAPI_TRUE;
			switch (mrapi_db->sems[s].type) {
			case (MUTEX): type = MRAPI_LOCK_MUTEX; break;
			case (SEM):  type = MRAPI_LOCK_SEM; break;
			case (RWL):  type = MRAPI_LOCK_RWL; break;
			}
		}
		else {
			/* lock the database */
			mrapi_impl_sem_ref_t ref = { sems_semid, s };
			mrapi_assert(mrapi_impl_access_database_pre(ref, MRAPI_TRUE));

			/* check to see if it is a deleted sem that had extended error checking set */
			for (d = 0; d < MRAPI_MAX_SEMS; d++) {
				if ((mrapi_db->sems[d].deleted == MRAPI_TRUE) &&
					(mrapi_db->sems[d].handle == hndl)) {
					switch (mrapi_db->sems[d].type) {
					case (MUTEX): *status = MRAPI_ERR_MUTEX_DELETED; break;
					case (SEM):  *status = MRAPI_ERR_SEM_DELETED; break;
					case (RWL):  *status = MRAPI_ERR_RWL_DELETED; break;
					};
					break;
				}
			}

			/* unlock the database */
			mrapi_assert(mrapi_impl_access_database_post(ref));
		}
	}

	return type;
}

/***************************************************************************
Function:  mrapi_impl_access_database_pre

Description: locks the database (blocking)

Parameters: none

Returns: none

***************************************************************************/
mrapi_boolean_t mrapi_impl_access_database_pre(mrapi_impl_sem_ref_t ref, mrapi_boolean_t fail_on_error)
{
	// if we are told to only use the global sem, then ignore the id passed in
	if (use_global_only) {
		ref.set = semid;
		ref.member = 0;
	}

	if (!use_spin_lock)
	{
		if (!sys_sem_lock(ref)) {
#if (__unix__||__MINGW32__)
			mrapi_dprintf(4, "mrapi_impl_access_database_pre - errno:%s", strerror(errno));
			//fprintf(stderr,"FATAL ERROR: unable to lock mrapi database: errno:%s\n",strerror(errno));
			if (fail_on_error) {
				fprintf(stderr, "FATAL ERROR: unable to lock mrapi database: errno:%s id=%x\n", strerror(errno), ref.set);
				exit(1);
			}
#else
			char buf[80];
			strerror_s(buf, 80, errno);
			mrapi_dprintf(4, "mrapi_impl_access_database_pre - errno:%s", buf);
			//fprintf(stderr,"FATAL ERROR: unable to lock mrapi database: errno:%s\n",strerror(errno));
			if (fail_on_error) {
				fprintf(stderr, "FATAL ERROR: unable to lock mrapi database: errno:%s id=%x\n", buf, ref.set);
				exit(1);
			}
#endif  /* !(__unix__||__MINGW32__) */
			return MRAPI_FALSE;
		}
	}
	else  /* spin lock */
	{
		int32_t lock = (int32_t)mrapi_tid;
		int32_t unlock = 0;
		int32_t prev;
		mrapi_status_t status;

		int usec = 1;
		int delay = 1;

		while (1)
		{
			if (mrapi_impl_atomic_cas(NULL, &mrapi_db->sems[ref.member].spin, &lock, &unlock, &prev, sizeof(int32_t), &status))
			{
				break;
			}
			sys_os_usleep(usec * sys_os_rand());
		}
	}

	mrapi_dprintf(4, "mrapi_impl_access_database_pre (got the internal mrapi db lock)");

	return MRAPI_TRUE;
}

/***************************************************************************
Function:  mrapi_impl_access_database_pre_multiple

Description: locks the database (blocking)

Parameters: none

Returns: none

***************************************************************************/
mrapi_boolean_t mrapi_impl_access_database_pre_multiple(mrapi_impl_sem_ref_t* ref, int count, mrapi_boolean_t fail_on_error)
{
	int i = 0;

	// if we are told to only use the global sem, then ignore the id passed in
	if (use_global_only) {
		mrapi_impl_sem_ref_t sref = { semid, 0 };
		return mrapi_impl_access_database_pre(sref, fail_on_error);
	}

	if (!use_spin_lock)
	{
		if (!sys_sem_lock_multiple(ref, count, TRUE)) {
#if (__unix__||__MINGW32__)
			mrapi_dprintf(4, "mrapi_impl_access_database_pre_multiple - errno:%s", strerror(errno));
			//fprintf(stderr,"FATAL ERROR: unable to lock mrapi database: errno:%s\n",strerror(errno));
			if (fail_on_error) {
				fprintf(stderr, "FATAL ERROR: unable to lock mrapi database: errno:%s id=%x\n", strerror(errno), ref[0].set);
				exit(1);
			}
#else
			char buf[80];
			strerror_s(buf, 80, errno);
			mrapi_dprintf(4, "mrapi_impl_access_database_pre_multiple - errno:%s", buf);
			//fprintf(stderr,"FATAL ERROR: unable to lock mrapi database: errno:%s\n",strerror(errno));
			if (fail_on_error) {
				fprintf(stderr, "FATAL ERROR: unable to lock mrapi database: errno:%s id=%x\n", buf, ref[0].set);
				exit(1);
			}
#endif  /* !(__unix__||__MINGW32__) */
			return MRAPI_FALSE;
		}
	}
	else  /* spin lock */
	{
		int32_t lock = (int32_t)mrapi_tid;
		int32_t unlock = 0;
		int32_t prev;
		mrapi_status_t status;

		int usec = 1;
		int delay = 1;

		while (1)
		{
			for (i = 0; i < count; i++)
			{
				if (!mrapi_impl_atomic_cas(NULL, &mrapi_db->sems[ref[i].member].spin, &lock, &unlock, &prev, sizeof(int32_t), &status))
				{
					/* Not able to collect all the spin locks; revert */
					int j = 0;
					for (j = 0; j < i; j++)
					{
						mrapi_db->sems[ref[j].member].spin = unlock;
					}
					break;
				}
			}
			if (count == i)
			{
				break;
			}
			sys_os_usleep(usec * sys_os_rand());
		}
	}

	mrapi_dprintf(4, "mrapi_impl_access_database_pre_multiple (got the internal mrapi db lock)");

	return MRAPI_TRUE;
}

/***************************************************************************
Function:  mrapi_impl_access_database_post

Description: unlocks the database

Parameters: none

Returns: none

***************************************************************************/
mrapi_boolean_t mrapi_impl_access_database_post(mrapi_impl_sem_ref_t ref)
{
	// if we are told to only use the global sem, then ignore the id passed in
	if (use_global_only) {
		ref.set = semid;
		ref.member = 0;
	}

	mrapi_dprintf(4, "mrapi_impl_access_database_post (released the internal mrapi db lock)");

	if (!use_spin_lock)
	{
		if (!sys_sem_unlock(ref)) {
#if (__unix__||__MINGW32__)
			mrapi_dprintf(4, "mrapi_impl_access_database_post (id=%d)- errno:%s", id, strerror(errno));
#else
			char buf[80];
			strerror_s(buf, 80, errno);
			mrapi_dprintf(4, "mrapi_impl_access_database_post (id=%d)- errno:%s", ref.set, buf);
#endif  /* !(__unix__||__MINGW32__) */
			fflush(stdout);
			return MRAPI_FALSE;
		}
	}
	else  /* spin lock */
	{
		int32_t unlock = 0;

		mrapi_db->sems[ref.member].spin = unlock;
	}

	return MRAPI_TRUE;
}

/***************************************************************************
Function:  mrapi_impl_access_database_post_multiple

Description: unlocks the database

Parameters: none

Returns: none

***************************************************************************/
mrapi_boolean_t mrapi_impl_access_database_post_multiple(mrapi_impl_sem_ref_t* ref, int count)
{
	int i = 0;

	// if we are told to only use the global sem, then ignore the id passed in
	if (use_global_only) {
		mrapi_impl_sem_ref_t sref = { semid, 0 };
		return mrapi_impl_access_database_post(sref);
	}

	mrapi_dprintf(4, "mrapi_impl_access_database_post_multiple (released the internal mrapi db lock)");

	if (!use_spin_lock)
	{
		if (!sys_sem_unlock_multiple(ref, count)) {
#if (__unix__||__MINGW32__)
			mrapi_dprintf(4, "mrapi_impl_access_database_post (id=%d)- errno:%s", id, strerror(errno));
#else
			char buf[80];
			strerror_s(buf, 80, errno);
			mrapi_dprintf(4, "mrapi_impl_access_database_post_multiple (id=%d)- errno:%s", ref[0].set, buf);
#endif  /* !(__unix__||__MINGW32__) */
			fflush(stdout);
			return MRAPI_FALSE;
		}
	}
	else  /* spin lock */
	{
		int32_t unlock = 0;
		for (i = 0; i < count; i++)
		{
			mrapi_db->sems[ref[i].member].spin = unlock;
		}
	}

	return MRAPI_TRUE;
}

/***************************************************************************
Function: mrapi_impl_whoami

Description: Gets the pid,tid pair for the caller and  then
	looks up the corresponding node and domain info in our database.

Parameters:

Returns: boolean indicating success or failure

***************************************************************************/
MCA_INLINE mrapi_boolean_t mrapi_impl_whoami(mrapi_node_t* node_id,
	uint32_t* n_index,
	mrapi_domain_t* domain_id,
	uint32_t* d_index)
{
	if (mrapi_db == NULL) { return MRAPI_FALSE; }
	if (mrapi_pid == (pid_t)-1) { return MRAPI_FALSE; }
	else {
		*n_index = mrapi_nindex;
		*d_index = mrapi_dindex;
		*node_id = mrapi_node_id;
		*domain_id = mrapi_domain_id;
	}
	return MRAPI_TRUE;
}

/***************************************************************************
Function: mrapi_impl_encode_hndl
Description:
 Our handles are very simple - a 32 bit integer is encoded with
 an index (16 bits gives us a range of 0:64K indices).  The index is the
 index into the sems, shmems or rmems array depending on what type of
 resource we are dealing with.

Parameters:

Returns: the handle
***************************************************************************/
uint32_t mrapi_impl_encode_hndl(uint16_t type_index)
{
	uint32_t valid = MCA_HANDLE_MASK;
	return (valid | type_index);
}

/***************************************************************************
Function: mrapi_impl_decode_hndl

Description: Decodes the given handle into it's database index.

Parameters:

Returns: true/false indicating success or failure
***************************************************************************/
mrapi_boolean_t mrapi_impl_decode_hndl(uint32_t handle,
	uint16_t *type_index)
{
	uint32_t valid = MCA_HANDLE_MASK;

	*type_index = (handle & 0x00ffffff);

	/* check that the valid bit is set */
	if ((handle & valid) == valid) {
		return MRAPI_TRUE;
	}
	return MRAPI_FALSE;
}

/***************************************************************************
Function: mrapi_impl_test
Description: Checks if the request has completed
Parameters:

Returns: true/false indicating success or failure
***************************************************************************/
mrapi_boolean_t mrapi_impl_test(const mrapi_request_t* request,
	mrapi_status_t* status)
{
	mrapi_boolean_t rc = MRAPI_FALSE;
	uint16_t r;

	/* lock the database */
	mrapi_impl_sem_ref_t ref = { requests_semid, 0 };
	mrapi_assert(mrapi_impl_access_database_pre(ref, MRAPI_TRUE));

	mrapi_dprintf(1, "mrapi_impl_test r=%08x", *request);

	if (mrapi_impl_decode_hndl(*request, &r) &&
		(r < MRAPI_MAX_REQUESTS) &&
		(mrapi_db->requests[r].valid == MRAPI_TRUE)) {
		if (mrapi_db->requests[r].completed) {
			*status = mrapi_db->requests[r].status;
			rc = MRAPI_TRUE;
			//free up the request
			memset(&mrapi_db->requests[r], 0, sizeof(mrapi_request_data));
		}
	}

	/* mrapi non-blocking functions always complete immediately */
	if (!rc) {
		mrapi_dprintf(1, "ASSERT: request=%08x r=%d valid=%d completed=%d\n",
			*request, r, mrapi_db->requests[r].valid, mrapi_db->requests[r].completed);
		mrapi_dprintf(1, "  If you are seeing this error, it's possible you have already called test/wait on this request (we recycle the requests).");
	}
	mrapi_assert(rc);

	/* unlock the database */
	mrapi_assert(mrapi_impl_access_database_post(ref));

	return rc;
}

/***************************************************************************
Function: mrapi_impl_canceled_request
Description: Checks if the request handle refers to a canceled request
Parameters:

Returns: true/false indicating success or failure
***************************************************************************/
mrapi_boolean_t mrapi_impl_canceled_request(const mrapi_request_t* request) {
	mrapi_boolean_t rc = MRAPI_FALSE;
	uint16_t r;

	/* lock the database */
	mrapi_impl_sem_ref_t ref = { requests_semid, 0 };
	mrapi_assert(mrapi_impl_access_database_pre(ref, MRAPI_TRUE));

	mrapi_dprintf(1, "mrapi_impl_canceled_request");

	if (mrapi_impl_decode_hndl(*request, &r) &&
		(r < MRAPI_MAX_REQUESTS) &&
		(mrapi_db->requests[r].valid == MRAPI_TRUE)) {
		rc = mrapi_db->requests[r].cancelled;
	}

	/* unlock the database */
	mrapi_assert(mrapi_impl_access_database_post(ref));

	return rc;
}

/***************************************************************************
Function: mrapi_impl_setup_request
Description:
Parameters:

Returns: the index into the requests array
***************************************************************************/
unsigned mrapi_impl_setup_request() {
	mrapi_node_t node;
	mrapi_domain_t domain;
	uint32_t n = 0;
	uint32_t d = 0;
	uint32_t i = 0;
	mrapi_boolean_t rc;

	/* lock the database */
	mrapi_impl_sem_ref_t ref = { requests_semid, 0 };
	mrapi_assert(mrapi_impl_access_database_pre(ref, MRAPI_TRUE));

	rc = mrapi_impl_whoami(&node, &n, &domain, &d);

	if (rc) {
		for (i = 0; i < MRAPI_MAX_REQUESTS; i++) {
			if (mrapi_db->requests[i].valid == MRAPI_FALSE) {
				mrapi_db->requests[i].valid = MRAPI_TRUE;
				mrapi_db->requests[i].domain_id = domain;
				mrapi_db->requests[i].node_num = node;
				mrapi_db->requests[i].cancelled = MRAPI_FALSE;
				break;
			}
		}
	}

	if (i < MRAPI_MAX_REQUESTS) {
		mrapi_dprintf(3, "mrapi_impl_setup_request i=%d valid=%d domain_id=%d node_num=%d cancelled = %d",
			i,
			mrapi_db->requests[i].valid,
			mrapi_db->requests[i].domain_id,
			mrapi_db->requests[i].node_num,
			mrapi_db->requests[i].cancelled);
	}
	else {
		mrapi_assert(0);
	}

	/* unlock the database */
	mrapi_assert(mrapi_impl_access_database_post(ref));
	return i;
}

/***************************************************************************
Function: mrapi_impl_valid_request_hndl
Description: Checks if the request handle refers to a valid request
Parameters:

Returns: true/false indicating success or failure
***************************************************************************/
mrapi_boolean_t mrapi_impl_valid_request_hndl(const mrapi_request_t* request)
{
	uint16_t r;
	mrapi_boolean_t rc = MRAPI_FALSE;

	/* lock the database */
	mrapi_impl_sem_ref_t ref = { requests_semid, 0 };
	mrapi_assert(mrapi_impl_access_database_pre(ref, MRAPI_TRUE));

	mrapi_dprintf(1, "mrapi_impl_valid_request_handle handle=0x%x", *request);

	if (mrapi_impl_decode_hndl(*request, &r) &&
		(r < MRAPI_MAX_REQUESTS) &&
		(mrapi_db->requests[r].valid == MRAPI_TRUE)) {
		rc = MRAPI_TRUE;
	}

	/* unlock the database */
	mrapi_assert(mrapi_impl_access_database_post(ref));
	return rc;
}

/***************************************************************************
Function: mrapi_impl_valid_mutex_hndl
Description: Checks if the mutex handle refers to a valid mutex
Parameters:

Returns: true/false indicating success or failure
***************************************************************************/
mrapi_boolean_t mrapi_impl_valid_mutex_hndl(mrapi_mutex_hndl_t mutex,
	mrapi_status_t* status)
{
	*status = MRAPI_ERR_MUTEX_INVALID;
	if (mrapi_impl_valid_lock_hndl(mutex, status)) {
		*status = MRAPI_SUCCESS;
		return MRAPI_TRUE;
	}
	return MRAPI_FALSE;
}

/***************************************************************************
Function: mrapi_impl_valid_rwl_hndl
Description: Checks if the rwl handle refers to a valid rwl
Parameters:

Returns: true/false indicating success or failure
***************************************************************************/
mrapi_boolean_t mrapi_impl_valid_rwl_hndl(mrapi_rwl_hndl_t rwl,
	mrapi_status_t* status)
{
	*status = MRAPI_ERR_RWL_INVALID;
	if (mrapi_impl_valid_lock_hndl(rwl, status)) {
		*status = MRAPI_SUCCESS;
		return MRAPI_TRUE;
	}
	return MRAPI_FALSE;
}

/***************************************************************************
Function: mrapi_impl_valid_sem_hndl
Description: Checks if the sem handle refers to a valid semaphore
Parameters:

Returns: true/false indicating success or failure
***************************************************************************/
mrapi_boolean_t mrapi_impl_valid_sem_hndl(mrapi_sem_hndl_t sem,
	mrapi_status_t* status)
{
	*status = MRAPI_ERR_SEM_INVALID;
	if (mrapi_impl_valid_lock_hndl(sem, status)) {
		*status = MRAPI_SUCCESS;
		return MRAPI_TRUE;
	}
	return MRAPI_FALSE;
}

/***************************************************************************
Function: mrapi_impl_valid_lock_hndl
Description: Checks if the sem handle refers to a valid semaphore
Parameters:

Returns: true/false indicating success or failure
***************************************************************************/
mrapi_boolean_t mrapi_impl_valid_lock_hndl(mrapi_sem_hndl_t sem,
	mrapi_status_t* status) {
	uint16_t s, d;
	mrapi_boolean_t rc = MRAPI_FALSE;

	if (!mrapi_impl_decode_hndl(sem, &s) || (s >= MRAPI_MAX_SEMS)) {
		return MRAPI_FALSE;
	}

	if (mrapi_db->sems[s].valid == MRAPI_TRUE) {
		rc = MRAPI_TRUE;
	}
	else {
		/* lock the database */
		mrapi_impl_sem_ref_t ref = { sems_semid, s };
		mrapi_assert(mrapi_impl_access_database_pre(ref, MRAPI_TRUE));

		/* check to see if it is a deleted sem that had extended error checking set */
		for (d = 0; d < MRAPI_MAX_SEMS; d++) {
			if ((mrapi_db->sems[d].deleted == MRAPI_TRUE) &&
				(mrapi_db->sems[d].handle == sem)) {
				switch (mrapi_db->sems[d].type) {
				case (MUTEX): *status = MRAPI_ERR_MUTEX_DELETED; break;
				case (SEM):  *status = MRAPI_ERR_SEM_DELETED; break;
				case (RWL):  *status = MRAPI_ERR_RWL_DELETED; break;
				};
				break;
			}
		}

		/* unlock the database */
		mrapi_assert(mrapi_impl_access_database_post(ref));
	}
	return rc;
}

/***************************************************************************
Function: mrapi_impl_valid_shmem_hndl

Description: Checks if the sem handle refers to a valid shmem segment

Parameters:

Returns: true/false indicating success or failure
***************************************************************************/
mrapi_boolean_t mrapi_impl_valid_shmem_hndl(mrapi_shmem_hndl_t shmem)
{
	uint16_t s;
	mrapi_boolean_t rc = MRAPI_TRUE;

	/* lock the database */
	mrapi_impl_sem_ref_t ref = { shmems_semid, 0 };
	mrapi_assert(mrapi_impl_access_database_pre(ref, MRAPI_TRUE));
	if (!mrapi_impl_decode_hndl(shmem, &s) || (s >= MRAPI_MAX_SHMEMS)) {
		rc = MRAPI_FALSE;
	}
	if ((rc) && (s < MRAPI_MAX_SHMEMS) && !mrapi_db->shmems[s].valid) {
		rc = MRAPI_FALSE;
	}
	/* unlock the database */
	mrapi_assert(mrapi_impl_access_database_post(ref));
	return rc;
}

/***************************************************************************
Function:mrapi_impl_valid_status_param
Description: checks if the given status is a valid status parameter
Parameters: status
Returns:MRAPI_TRUE/MRAPI_FALSE
***************************************************************************/
mrapi_boolean_t mrapi_impl_valid_status_param(const mrapi_status_t* mrapi_status)
{
	return (mrapi_status != NULL);
}

/***************************************************************************
Function:mrapi_impl_mutex_validID
Description: checks if the given ID is valid
Parameters:
Returns:MRAPI_TRUE/MRAPI_FALSE
***************************************************************************/
mrapi_boolean_t mrapi_impl_mutex_validID(mrapi_mutex_id_t mutex) { return MRAPI_TRUE; }

/***************************************************************************
Function:mrapi_impl_sem_validID
Description: checks if the given ID is valid
Parameters:
Returns:MRAPI_TRUE/MRAPI_FALSE
***************************************************************************/
mrapi_boolean_t mrapi_impl_sem_validID(mrapi_sem_id_t sem) { return MRAPI_TRUE; }

/***************************************************************************
Function:mrapi_impl_rwl_validID
Description: checks if the given ID is valid
Parameters:
Returns:MRAPI_TRUE/MRAPI_FALSE
***************************************************************************/
mrapi_boolean_t mrapi_impl_rwl_validID(mrapi_rwl_id_t rwl) { return MRAPI_TRUE; }

/***************************************************************************
Function:mrapi_impl_shmem_validID
Description: checks if the given ID is valid
Parameters:
Returns:MRAPI_TRUE/MRAPI_FALSE
***************************************************************************/
mrapi_boolean_t mrapi_impl_shmem_validID(mrapi_shmem_id_t shmem) { return MRAPI_TRUE; }

/***************************************************************************
Function:mrapi_impl_rmem_validID
Description: checks if the given ID is valid
Parameters:
Returns:MRAPI_TRUE/MRAPI_FALSE
***************************************************************************/
mrapi_boolean_t mrapi_impl_rmem_validID(mrapi_rmem_id_t rmem) { return MRAPI_TRUE; }

/***************************************************************************
Function: mrapi_impl_initialized
Description: checks if the given domain_id/node_id is already assoc w/ this pid/tid
Parameters: node_id,domain_id
Returns:MRAPI_TRUE/MRAPI_FALSE
***************************************************************************/
mrapi_boolean_t mrapi_impl_initialized()
{
	mrapi_node_t node;
	mrapi_domain_t domain;
	uint32_t n, d;

	return mrapi_impl_whoami(&node, &n, &domain, &d);
}

/***************************************************************************
Function:mrapi_valid_node_num

Description: checks if the given node_num is a valid node_num for this system

Parameters: node_num: the node num to be checked

Returns:MRAPI_TRUE/MRAPI_FALSE
***************************************************************************/
mrapi_boolean_t mrapi_impl_valid_node_num(mrapi_node_t node_num)
{
	return MRAPI_TRUE;
}

/***************************************************************************
Function: mcapi_impl_get_node_num

Description: gets the node_num

Parameters: node_num: the pointer to be filled in

Returns: boolean indicating success (the node num was found) or failure
 (couldn't find the node num).
***************************************************************************/
mrapi_boolean_t mrapi_impl_get_node_num(mrapi_node_t* node)
{
	uint32_t n, d;
	mrapi_domain_t domain;

	return mrapi_impl_whoami(node, &n, &domain, &d);
}

/************************************************************************
mrapi_impl_node_id_get

DESCRIPTION
Returns the node id associated with the local node and domain.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_NODE_NOTINIT The calling node is not intialized.

NOTE
This implementation was moved from mrapi.c to
  eliminate circular dependencies between the layers.
***********************************************************************/
mrapi_node_t mrapi_impl_node_id_get(mrapi_status_t* status)
{
	mca_node_t node = MRAPI_NODE_INVALID;

	*status = MRAPI_SUCCESS;

	if (!mrapi_impl_initialized() || !mrapi_impl_get_node_num(&node)) {
		*status = MRAPI_ERR_NODE_NOTINIT;
	}
	return node;
}

/***************************************************************************
Function: mcapi_impl_get_domain_num

Description: gets the domain_num

Parameters: domain_num: the pointer to be filled in

Returns: boolean indicating success (the node num was found) or failure
 (couldn't find the node num).
***************************************************************************/
mrapi_boolean_t mrapi_impl_get_domain_num(mrapi_domain_t* domain)
{
	uint32_t n, d;
	mrapi_node_t node;

	return mrapi_impl_whoami(&node, &n, domain, &d);
}

/***************************************************************************
Function:mrapi_valid_domain_num

Description:

Parameters:

Returns:MRAPI_TRUE/MRAPI_FALSE
***************************************************************************/
mrapi_boolean_t mrapi_impl_valid_domain_num(mrapi_domain_t domain_num)
{
	return MRAPI_TRUE;
}
