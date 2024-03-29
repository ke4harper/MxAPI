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

/*
 * mca.h
 * Version 1.131, May 2010
*/

#ifndef MCA_H
#define MCA_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "Fragments/mca_inc.h"

/*
 * MCA type definitions
 */
typedef int    			mca_int_t;
typedef char 			mca_int8_t;
typedef short 			mca_int16_t;
typedef int 			mca_int32_t;
typedef long long 		mca_int64_t;
typedef unsigned int            mca_uint_t;
typedef unsigned char 	        mca_uint8_t;
typedef unsigned short 	        mca_uint16_t;
typedef unsigned int 		mca_uint32_t;
typedef unsigned long long 	mca_uint64_t;
typedef unsigned char		mca_boolean_t;
typedef unsigned int            mca_node_t;
typedef unsigned int		mca_status_t;
typedef unsigned int		mca_timeout_t;
typedef unsigned int		mca_domain_t;

/* Constants */
#define MCA_TRUE			  1
#define MCA_FALSE			  0
#define MCA_NULL			  0	/* MCA Zero value */
#define	MCA_INFINITE		        (~0)	/* Wait forever, no timeout */
#define MCA_RETURN_VALUE_INVALID        (~0) 
#define MCA_NODE_INVALID                (~0) 
#define MCA_DOMAIN_INVALID              (~0) 

/* In/out parameter indication macros */
#ifndef MCA_IN
#define MCA_IN const
#endif /* MCA_IN */

#ifndef MCA_OUT
#define MCA_OUT
#endif /* MCA_OUT */

/* Alignment macros */
#ifdef __GNUC__
#define MCA_DECL_ALIGNED __attribute__ ((aligned (32)))
#else
#define MCA_DECL_ALIGNED /* MCA_DECL_ALIGNED alignment macro currently only
							supports GNU compiler */
#endif /* __GNUC__ */

/*
 * MCA organization id's (for assignment of organization specific attribute numbers)
 */
#define MCA_ORG_ID_PSI 0 	/* PolyCore Software, Inc. */
#define MCA_ORG_ID_FSL 1 	/* Freescale, Inc. */
#define MCA_ORG_ID_MGC 2 	/* Mentor Graphics, Corp. */
#define MCA_ORG_ID_TBA 3 	/* To be assigned */
/* And so forth */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MCA_H */
