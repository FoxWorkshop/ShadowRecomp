#pragma once

#include <plume_render_interface.h>

#include <cassert>
#include <cstdint>
#include <mutex>
#include <vector>

// A reserved slice of a staging area.
// - memory: write the bytes here on the CPU.
// - buffer + offset: where the render thread copies from on the GPU.
// - deviceAddress: GPU address of the same spot; unused by staged uploads,
//   kept for the other upload allocators.
struct UploadAllocation
{
	const plume::RenderBuffer* buffer;
	uint64_t offset;
	uint8_t* memory;
	uint64_t deviceAddress;
};

// Shared staging area for index/vertex buffer uploads on discrete GPUs.
//
// Flow per upload: game thread reserves a slice and writes bytes; render
// thread copies the slice to its final GPU buffer.
//
// Lifetime: one ring per game frame, see g_stagedBufferUploads. Present
// rewinds a ring once the GPU has finished the frames that read it, so live
// bytes never get overwritten.
//
// Size: 16MB per block is a floor, not a cap. A frame needing more gets
// another block; blocks are reused after the rewind.
struct StagedBufferUploadAllocator
{
	static constexpr size_t SIZE = 16 * 1024 * 1024;

	// One 16MB GPU block of CPU-writable memory the ring hands out pieces of.
	struct Block
	{
		std::unique_ptr<plume::RenderBuffer> buffer;
		uint8_t* memory = nullptr;
	};

	std::mutex mutex;
	std::vector<Block> buffers;
	uint32_t index = 0;
	uint32_t offset = 0;

	UploadAllocation allocate(uint32_t size, uint32_t alignment, plume::RenderDevice& device)
	{
		assert(size <= SIZE);

		std::lock_guard lock(mutex);

		// Line the write cursor up to the alignment boundary.
		offset = (offset + alignment - 1) & ~(alignment - 1);

		// Block full: continue at the start of a fresh one.
		if (offset + size > SIZE)
		{
			++index;
			offset = 0;
		}

		// First time this block is used: make room for it in the list.
		if (buffers.size() <= index)
			buffers.resize(index + 1);

		auto& buffer = buffers[index];
		if (buffer.buffer == nullptr)
		{
			// New block: create its GPU buffer and open CPU write access once.
			buffer.buffer = device.createBuffer(plume::RenderBufferDesc::UploadBuffer(SIZE, plume::RenderBufferFlag::VERTEX | plume::RenderBufferFlag::INDEX));
			buffer.memory = reinterpret_cast<uint8_t*>(buffer.buffer->map());
		}

		// Carve the slice out and move the cursor past it.
		auto ref = buffer.buffer->at(offset);
		offset += size;

		return { ref.ref, ref.offset, buffer.memory + ref.offset, 0 };
	}

	// Back to the first block. Called once per frame after the GPU is done
	// with it, so slices can be handed out again.
	void reset()
	{
		std::lock_guard lock(mutex);
		index = 0;
		offset = 0;
	}
};
