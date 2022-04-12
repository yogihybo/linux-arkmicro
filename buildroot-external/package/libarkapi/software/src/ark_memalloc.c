#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include "ark_common.h"
#include "ark_memalloc.h"

void arkapi_memalloc_free(memalloc_handle *handle, reqbuf_info *pbuf)
{
	MemallocParams params;
	int ret = SUCCESS, i;

	if (!handle || handle->fd <= 0 || !pbuf) {
		printf("%s: handle=%p, handle->fd=%d, pbuf=%p, error.\n", __func__, handle, handle->fd, pbuf);
		return;
	}

	if (!pbuf->count || !pbuf->size) {
		printf("%s: reqbuf=%p, count=%d, size=%d, error.\n", \
		                __func__, pbuf, pbuf->count, pbuf->size);
		return;
	}

	for (i = 0; i < pbuf->count; i++) {
		if (pbuf->phy_addr[i] != 0) {
			params.busAddress = pbuf->phy_addr[i];
			params.size = pbuf->size;
			ioctl(handle->fd, MEMALLOC_IOCSFREEBUFFER, &params);

			if (pbuf->virt_addr[i] != NULL)
				arkapi_mem_unmap(pbuf->virt_addr[i], pbuf->size);

			 ark_dbg("%s: phy_addr[%d]=%p, virt_addr[%d]=%p, size=%d.\n", \
			        __func__,i, pbuf->phy_addr[i], i, pbuf->virt_addr[i], pbuf->size);
		}
	}

	list_del(&pbuf->list);
	free(pbuf);
	--handle->reqbuf_cnt;

	ark_dbg("%s: ret=%d.\n",__func__, ret);
}


void* arkapi_mem_map(unsigned int phy_addr, unsigned int size)
{
	int mem_fd;
	void *virt_addr = NULL;

	if(!size || !phy_addr)
		return NULL;

	mem_fd = open(MEM_PATH, O_RDWR | O_SYNC);
	if (mem_fd < 0) {
		printf("open %s failure.\n",MEM_PATH);
		return virt_addr;
	}

	virt_addr = (unsigned char*)mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, phy_addr);

	ark_dbg("%s: phy_addr=%p ==> addr_virt=%p, size=%d .\n",__func__, phy_addr, virt_addr, size);

	close(mem_fd);

	return virt_addr;
}

int arkapi_mem_unmap(void* virt_addr, unsigned int size)
{
	int ret;

	if(!size  || !virt_addr)
		return -EINVAL;

	ret = munmap((void *)virt_addr, size);

	ark_dbg("%s: virt_addr=%p, size=%d .\n",__func__, virt_addr, size);

	return ret;
}

memalloc_handle *arkapi_memalloc_init(void)
{
	memalloc_handle *handle = NULL;
	reqbuf_info *pbuf;
	int fd;

	fd = open(MEMALLOC_PATH, O_RDWR | O_SYNC);
	if (fd < 0) {
		printf("open %s fail.\n", MEMALLOC_PATH);
		return NULL;
	}

	handle = (memalloc_handle *)malloc(sizeof(memalloc_handle));
	if(!handle){
		printf("%s: malloc handle error.\n", __func__);
		return NULL;
	}

	memset(handle, 0 ,sizeof(memalloc_handle));
	handle->fd = fd;
	INIT_LIST_HEAD(&handle->reqbuf_list);

	ark_dbg("%s: <---success.\n",__func__);

	return handle;
}

reqbuf_info *arkapi_memalloc_reqbuf(memalloc_handle *handle, int size, int count)
{
	reqbuf_info *pbuf = NULL;
	MemallocParams params;
	int ret, i;

	if (!handle || handle->fd <= 0) {
		printf("%s: handle=%p, handle->fd=%d, error.\n", __func__, handle, handle->fd);
		return NULL;
	}

	if (!size || !count  || count > FRAME_MAX) {
		printf("%s: count=%d(max=5), size=%d, error.\n", __func__, count, size);
		return NULL;
	}

	pbuf = (reqbuf_info *)malloc(sizeof(reqbuf_info));
	if (!pbuf) {
		printf("%s: pbuf malloc error.\n", __func__);
		return NULL;
	}

	memset(pbuf, 0, sizeof(reqbuf_info));
	pbuf->count = count;
	pbuf->size  = params.size = size;
	for (i = 0; i < pbuf->count; i++) {
		ret = ioctl(handle->fd, MEMALLOC_IOCXGETBUFFER, &params);
		if (ret != 0) {
			printf("%s: ioctl fail, ret=%d.\n",__func__, ret);
			goto err;
		}
		pbuf->phy_addr[i] = params.busAddress;

		pbuf->virt_addr[i] = arkapi_mem_map(params.busAddress, params.size);
		if (pbuf->virt_addr[i] == NULL) {
			ret = -EINVAL;
			goto err;
		}

		ark_dbg("%s: reqbuf.phy_addr[%d]=%p, reqbuf.virt_addr[%d]=%p, size=%d.\n",\
		                                    __func__,i,pbuf->phy_addr[i],i,pbuf->virt_addr[i], size);
	}

	list_add(&pbuf->list, &handle->reqbuf_list);
	++handle->reqbuf_cnt;

	ark_dbg("%s: success. reqbuf_cnt=%d.\n",__func__, handle->reqbuf_cnt);

	return pbuf;

err:
	for (i = 0; i < pbuf->count; i++) {
		if (pbuf->phy_addr[i] != 0) {
			params.busAddress = pbuf->phy_addr[i];
			params.size = pbuf->size;
			ioctl(handle->fd, MEMALLOC_IOCSFREEBUFFER, &params);
			if (pbuf->virt_addr[i] != NULL)
				arkapi_mem_unmap(&pbuf->virt_addr[i], pbuf->size);
		}
	}
	free(pbuf);
	return NULL;
}

void arkapi_memalloc_release(memalloc_handle *handle)
{
	int ret, i, user_id;
	struct list_head *pos;
	struct list_head *del_tmp;
	reqbuf_info *pbuf;

	if(!handle || handle->fd <= 0){
		printf("%s: handle=%p, handle->fd=%d.\n", __func__, handle, handle->fd);
		return;
	}

	list_for_each_safe(pos, del_tmp, &handle->reqbuf_list) {
		pbuf = list_entry(pos, reqbuf_info, list);
		if (pbuf && pbuf->count > 0 && pbuf->size > 0) {
			arkapi_memalloc_free(handle, pbuf);
			ark_dbg("%s: reqbuf_cnt=%d.\n",__func__, handle->reqbuf_cnt);
		}
	}

	if(handle->fd)
		close(handle->fd);
	if(handle)
		free(handle);
	ark_dbg("%s: <---success.\n",__func__);

}

#if 0
int ark1668_memalloc_flush(int memalloc_fd, unsigned int phy_start, unsigned int size)
{
	MemallocParams param;
	int ret = SUCCESS;

	if (memalloc_fd < 0 || !phy_start || !size)
		return -EINVAL;

	param.busAddress = phy_start;
	param.size = size;

	ret = ioctl(memalloc_fd, MEMALLOC_IOCSFLUSHRAMBUFFER, &param);
	if (ret != 0) {
		printf("%s ioctl fail.\n", __func__);
		return ret;
	}

}

void ark1668_cache_flush(unsigned int virt_start, unsigned int size)
{
	syscall(__ARM_NR_cacheflush, virt_start, virt_start+size, 0);
}

int arkapi_memalloc_flush(memalloc_handle *handle, reqbuf_info *reqbuf, int buf_id)
{
	reqbuf_info *pbuf;

	if(!handle || !reqbuf || buf_id >= FRAME_MAX || buf_id < 0){
		printf("%s: handle=%p, reqbuf=%p, buf_id=%d(max=5)error.\n", \
		                                __func__, handle, reqbuf, buf_id);
		return -EINVAL;
	}

	if(handle->fd <= 0){
		printf("%s: fd <= 0, error.\n", __func__);
		return -EINVAL;
	}

	ark1668_memalloc_flush(handle->fd, pbuf->phy_addr[buf_id], pbuf->size);
	ark1668_cache_flush((unsigned int)pbuf->virt_addr[buf_id], pbuf->size);
	//ark_dbg("%s: success.\n",__func__);

	return SUCCESS;
}

#endif
