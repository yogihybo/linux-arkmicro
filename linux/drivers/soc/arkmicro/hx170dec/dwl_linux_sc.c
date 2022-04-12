/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2017, VeriSilicon Inc. All rights reserved        --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "dwl_linux.h"
#include "dwl.h"
#include <linux/vmalloc.h>
#include <linux/mm.h>

#include<linux/slab.h>

//#include <assert.h>
//#include <errno.h>
//#include <signal.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

/*------------------------------------------------------------------------------
    Function name   : DWLInit
    Description     : Initialize a DWL instance

    Return type     : const void * - pointer to a DWL instance

    Argument        : void * param - not in use, application passes NULL
------------------------------------------------------------------------------*/
const void *DWLInit(DWLInitParam_t * param)
{
    hX170dwl_t *dec_dwl;

	dec_dwl = (hX170dwl_t *)kcalloc(1, sizeof(hX170dwl_t), GFP_KERNEL);
	if (!dec_dwl)
	{
		 printk("error!!!\n");
		 goto err;
	}

    if(dec_dwl == NULL)
    {
        printk("failed to alloc hX170dwl_t struct\n");
        return NULL;
    }

    dec_dwl->clientType = param->clientType;

    switch (dec_dwl->clientType)
    {
	    case DWL_CLIENT_TYPE_H264_DEC:
	    case DWL_CLIENT_TYPE_MPEG4_DEC:
	    case DWL_CLIENT_TYPE_JPEG_DEC:
	    case DWL_CLIENT_TYPE_VC1_DEC:
	    case DWL_CLIENT_TYPE_MPEG2_DEC:
	    case DWL_CLIENT_TYPE_VP6_DEC:
	    case DWL_CLIENT_TYPE_VP8_DEC:
	    case DWL_CLIENT_TYPE_RV_DEC:
	    case DWL_CLIENT_TYPE_AVS_DEC:
	    case DWL_CLIENT_TYPE_PP:
	    {
	        break;
	    }
	    default:
	    {
	        printk("Unknown client type no. %d\n", dec_dwl->clientType);
	        goto err;
	    }
    }

	dec_dwl->regSize = 404;
	dec_dwl->numCores = 1;
    return dec_dwl;

    err:
    DWLRelease(dec_dwl);
    return NULL;
}

/*------------------------------------------------------------------------------
    Function name   : DWLRelease
    Description     : Release a DWl instance

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - instance to be released
------------------------------------------------------------------------------*/
i32 DWLRelease(const void *instance)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    kfree(dec_dwl);

//  printk("DWLRelease SUCCESS\n");

    return (DWL_OK);
}

/* HW locking */

/*------------------------------------------------------------------------------
    Function name   : DWLReserveHwPipe
    Description     :
    Return type     : i32
    Argument        : const void *instance
    Argument        : i32 *coreID - ID of the reserved HW core
------------------------------------------------------------------------------*/
i32 DWLReserveHwPipe(const void *instance/*, i32 *coreID*/)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

//    assert(dec_dwl != NULL);
//    assert(dec_dwl->clientType != DWL_CLIENT_TYPE_PP);

    DWL_DEBUG("Start\n");

    dec_dwl->bPPReserved = 1;

    return DWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : DWLReserveHw
    Description     :
    Return type     : i32
    Argument        : const void *instance
    Argument        : i32 *coreID - ID of the reserved HW core
------------------------------------------------------------------------------*/
i32 DWLReserveHw(const void *instance/*, i32 *coreID*/)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    int isPP;

//    assert(dec_dwl != NULL);

    isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;

    DWL_DEBUG(" %s\n", isPP ? "PP" : "DEC");

    return DWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : DWLReleaseHw
    Description     :
    Return type     : void
    Argument        : const void *instance
------------------------------------------------------------------------------*/
void DWLReleaseHw(const void *instance/*, i32 coreID*/)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    int isPP;
	i32 coreID = 0;

//    assert((u32)coreID < dec_dwl->numCores);
//    assert(dec_dwl != NULL);

    isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;

    if ((u32) coreID >= dec_dwl->numCores)
        return;

    DWL_DEBUG(" %s core %d\n", isPP ? "PP" : "DEC", coreID);

    if (isPP)
    {
//        assert(coreID == 0);
    }
    else
    {
        if (dec_dwl->bPPReserved)
        {
            /* decoder has reserved PP also => release it */
            DWL_DEBUG("DEC released PP core %d\n", coreID);

            dec_dwl->bPPReserved = 0;

//            assert(coreID == 0);
        }
    }
}
