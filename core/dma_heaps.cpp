/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2020, Raspberry Pi Ltd
 *
 * dma_heaps.cpp - Helper class for dma-heap allocations.
 */

#include "dma_heaps.hpp"

#include <array>
#include <fcntl.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "core/logging.hpp"

namespace
{
/*
 * /dev/dma-heap/linux,cma is the dma-heap allocator, which allows dmaheap-cma
 * to only have to worry about importing.
 *
 * Annoyingly, should the cma heap size be specified on the kernel command line
 * instead of DT, the heap gets named "reserved" instead.
 */

const std::vector<const char *> heapNames
{
    "/dev/dma_heap/linux,cma",
    "/dev/dma_heap/reserved",
};

} // namespace

DmaHeap::DmaHeap()
{
	for (const char *name : heapNames)
	{
		int ret = ::open(name, O_RDWR | O_CLOEXEC, 0);
		if (ret < 0)
		{
			LOG(2, "Failed to open " << name << ": " << ret);
			continue;
		}

		dmaHeapHandle_ = libcamera::UniqueFD(ret);
		break;
	}

	if (!dmaHeapHandle_.isValid())
		LOG_ERROR("Could not open any dmaHeap device");
}

DmaHeap::~DmaHeap()
{
}

libcamera::UniqueFD DmaHeap::alloc(const char *name, std::size_t size)
{
	int ret;

	if (!name)
		return {};

	struct dma_heap_allocation_data alloc = {};

	alloc.len = size;
	alloc.fd_flags = O_CLOEXEC | O_RDWR;

	ret = ::ioctl(dmaHeapHandle_.get(), DMA_HEAP_IOCTL_ALLOC, &alloc);
	if (ret < 0)
	{
		LOG_ERROR("dmaHeap allocation failure for " << name);
		return {};
	}

	libcamera::UniqueFD allocFd(alloc.fd);
	ret = ::ioctl(allocFd.get(), DMA_BUF_SET_NAME, name);
	if (ret < 0)
	{
		LOG_ERROR("dmaHeap naming failure for " << name);
		return {};
	}

	return allocFd;
}
