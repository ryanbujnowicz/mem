#ifndef MEM_HEAPALLOCATOR_H
#define MEM_HEAPALLOCATOR_H

#include <cassert>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

#include "util/units.h"
#include "mem/alignment.h"
#include "mem/allocator.h"

struct BlockHeader
{
    // Do not use info directly, instead use _getBlockSize(), _isBlockAllocated(), etc.
    // and _isSegmentHeadBlock
    size_t head;

    // Only used if this block is unallocated. This are links in a chain of blocks
    // of a given size. The total size of this information determines the minimum 
    // allocated size of a block.
    BlockHeader* next;
    BlockHeader* prev;
};

/**
 * Used for blocks in the tree bins. BlockHeaders from these bins can be converted
 * to BlockTreeHeader freely since blocks in those bins are guaranteed to be large
 * enough. A BlockTreeHeader can also be cast to a BlockHeader.
 *
 * BlockTreeHeader allows for the manipulation of a bit-wise trie data structure
 * for finding blocks of the correct size.
 */
struct BlockTreeHeader
{
    // Do not use info directly, instead use _getBlockSize(), _isBlockAllocated(), etc.
    size_t head;

    // Blocks of the same size are chained together in the tree
    // This is a double-linked list that isn't circular
    BlockTreeHeader* next;
    BlockTreeHeader* prev;

    // Tree information
    BlockTreeHeader* parent;
    BlockTreeHeader* child[2];
};

struct BlockFooter
{
    size_t foot;
};

struct Segment
{
    size_t size;
    size_t flags;
    Segment* next;
    Segment* prev;
};

namespace mem {

/**
 * General purpose allocator based on dlmalloc and designed to work well for a variety of 
 * different requests.
 *
 * The majority of game-time allocations will be using this allocator. The goals of 
 * this allocator are in order of most important:
 *
 * 1. Handle allocations/frees of any size.
 * 2. Preventing memory leaks and buffer overruns.
 * 3. General tunability of performance/space.
 * 4. Memory overhead.
 * 5. Speed of small allocations.
 * 6. Speed of large allocations.
 *
 * A binning strategy with 64 bins is used to to ease searching of free blocks. The first
 * 32 bins are the exact-fit bins and are between 16 and 256 bytes linearly increasing by 8.
 * The remaining 32 bins start from the range 256-384 bytes, and increase by 128 bytes
 * every two bins. The last bin stores any requests that aren't large enough to hit the large 
 * request threshold.
 *
 * Allocations are made using a cascading search strategy which hopes to minimize search
 * time while finding the closest fit. 
 */
class HeapAllocator : public mem::Allocator
{
public:
    struct Block
    {
        void* addr;
        void* data;
        void* segment;
        size_t size;
        size_t bin;
        bool isAllocated;
    };

    struct Stats
    {
        size_t allocatedBytes;
        size_t freeBytes;
        size_t overheadBytes;
        size_t allocatedBlocks;
        size_t freeBlocks;
        size_t numRegularSegments;
        size_t numExternalSegments;
    };

public:
    HeapAllocator(
            size_t initialAllocSize = util::kilobytes(64), 
            size_t alignment = util::bytes(4));
    ~HeapAllocator();

    virtual void* allocate(size_t size, size_t alignment = DefaultAlignment, size_t offset = 0) override;
    virtual void release(void* addr) override;
    virtual size_t getAllocationSize(void* addr) const override
    {
        // Not implemented
        return 0;
    }

    virtual void clear();

    /**
     * Checks for invalid memory blocks indicating corruption.
     *
     * This is a fairly slow operation, do not call in a time-critical section.
     * Returns true if allocator is valid, corruptBlocks if non-null will
     * contain a list of corrupted blocks.
     */
    bool check(std::vector<Block>* corruptBlocks = nullptr);

    Stats getStats() const;
    std::vector<Block> getBlocks() const;

    /**
     * Options for changing the behaviour of the allocator.
     */
    void enableSystemAllocation(bool enable) { _doSystemAllocation = enable; }
    void enableBlockMerging(bool enable) { _doSegmentMerging = enable; }
    void enableSegmentMerging(bool enable) { _doSegmentMerging = enable; }

protected:
    static const size_t BlockAllocatedBitMask = static_cast<size_t>(1) << (sizeof(size_t)*CHAR_BIT - 1);
    static const size_t BlockFencePostBitMask = static_cast<size_t>(1) << (sizeof(size_t)*CHAR_BIT - 2);
    static const size_t BlockExternalBitMask = static_cast<size_t>(1) << (sizeof(size_t)*CHAR_BIT - 3);
    static const size_t BlockFlagsBitMask = (BlockAllocatedBitMask|BlockFencePostBitMask|BlockExternalBitMask);
    static const size_t BlockSizeBitMask = ~BlockFlagsBitMask;

    static const size_t SegmentExternalBitMask = static_cast<size_t>(1) << (sizeof(size_t)*CHAR_BIT - 1);
    static const size_t SegmentFlagsBitMask = SegmentExternalBitMask;
    static const size_t SegmentOffsetBitMask = ~SegmentFlagsBitMask;

    // Min size is to account for next/prev pointers in free blocks
    static const size_t MinAllocationSize = 2*sizeof(BlockHeader*);
    static const size_t MaxAllocationSize = ~BlockFlagsBitMask;
    static const size_t LargeAllocBoundary = util::megabytes(32);
    static const size_t MaxSmallBinSize = 255;
    static const size_t MaxTreeBinSize = util::kilobytes(16) - 1;
    static const size_t NumSmallBins = 32;
    static const size_t NumTreeBins = 32;
    static const size_t NumBins = NumSmallBins + NumTreeBins;

    static const size_t BlockHeaderSize = sizeof(BlockHeader) - 2*sizeof(BlockHeader*);
    static const size_t BlockOverheadSize = BlockHeaderSize + sizeof(BlockFooter);

    void* _allocFromSmallBin(size_t numBytes);
    void* _allocFromTreeBin(size_t numBytes);
    void* _allocFromReserve(size_t numBytes);
    void* _allocFromSystem(size_t numBytes);

    bool _isSmallAlloc(size_t numBytes) const { return numBytes <= MaxSmallBinSize; }
    bool _isLargeAlloc(size_t numBytes) const { return numBytes > LargeAllocBoundary; }
    size_t _getBinIndex(size_t numBytes) const;

    // Return the block to be linked/unlinked if success, nullptr otherwise
    void _initBlock(BlockHeader* block, size_t numBytes, bool isExternal) const;
    BlockHeader* _linkBlock(BlockHeader* block);
    BlockHeader* _linkBlockToBins(BlockHeader* block);
    BlockHeader* _linkSmallBlock(BlockHeader* block, size_t binIndex);
    BlockHeader* _unlinkBlock(BlockHeader* block);
    BlockHeader* _unlinkSmallBinBlock(BlockHeader* block, size_t binIndex);
    BlockHeader* _unlinkReserveBlock();
    BlockHeader* _coalesceAdjacentBlocks(BlockHeader* block);
    BlockHeader* _mergeBlocks(BlockHeader** blocks, size_t numBlocks);

    // Returns a block of given size split off the given block. Rest of the split
    // is added to appropriately sized bin or from the system. If the remainder is of a 
    // sufficiently small size, no split may occur and the whole block will be 
    // returned instead (no split).
    BlockHeader* _splitReserveBlock(size_t numBytes);
    BlockHeader* _splitBlock(BlockHeader* block, size_t numBytes) const;

    // Creates the minimum segment to store numBytes and returns a block
    // for the new segment. External blocks are not managed by the bin
    // structure and are freed as whole.
    BlockHeader* _allocNewSegment(size_t numBytes, bool isExternal); 
    BlockHeader* _linkSegment(Segment* segment);
    void _releaseExternalSegment(Segment* segment);

    // Retrieves next contiguous block header in a segment
    // This is not fast, only call for time-insensitive operations
    BlockHeader* _getNextBlock(BlockHeader* block) const;
    BlockHeader* _getPrevBlock(BlockHeader* block) const;

    // Retrieve various portions of a block given a pointer to someplace in that block
    void* _getBlockData(BlockHeader* block) const;
    BlockFooter* _getBlockFooter(BlockHeader* block) const;
    BlockHeader* _getDataHeader(void* data) const;
    size_t _getBlockSize(BlockHeader* block) const;
    size_t _getBlockSize(BlockTreeHeader* block) const;
    size_t _getBlockSize(BlockFooter* block) const;
    void _setBlockSize(BlockHeader* block, size_t size) const;
    bool _isBlockAllocated(BlockHeader* block) const;
    void _setBlockAllocated(BlockHeader* block, bool isAllocated) const;
    bool _isBlockFencePost(BlockHeader* block) const;
    void _setBlockFencePost(BlockHeader* block, bool isFencePost) const;
    bool _isBlockExternal(BlockHeader* block) const;
    void _setBlockExternal(BlockHeader* block, bool isExternal) const;
    void _setBlockState(BlockHeader* block, size_t size, bool isAllocated) const;

    void _reconcileFooter(BlockHeader* block) const;

    BlockHeader* _getFirstSegmentBlock(Segment* segment) const;
    Segment* _getSegment(BlockHeader* block) const;
    bool _isInSegment(BlockHeader* block, Segment* segment) const;
    bool _areSegmentsAdjacent(Segment* prev, Segment* next) const;
    size_t _getSegmentOverhead(Segment* segment) const;
    bool _isSegmentExternal(Segment* segment) const;
    void _setSegmentExternal(Segment* segment, bool isExternal) const;
    size_t _getSegmentOffset(Segment* segment) const;
    void _setSegmentOffset(Segment* segment, size_t offset) const;

    bool _checkBlock(BlockHeader* block) const;
    bool _blockBelongsToAllocator(BlockHeader* block) const;

    // Block tree manipulation
    BlockTreeHeader* _linkTreeBlock(BlockHeader* block, size_t binIndex);
    BlockTreeHeader* _findTreeBlock(size_t binIndex, size_t numBytes) const;
    void _replaceTreeBlock(BlockTreeHeader* block, BlockTreeHeader* repl, size_t binIndex);
    BlockTreeHeader* _unlinkTreeBlock(BlockTreeHeader* block, size_t binIndex);
    BlockTreeHeader* _unlinkTreeLeafBlock(BlockTreeHeader* leaf);
    BlockTreeHeader* _getSmallestTreeBlock(BlockTreeHeader* root) const;
    size_t _getTreeBinShift(size_t binIndex) const;

    // Root is the root of the bin tree. Head is the head of a chain within the tree.
    bool _isRootTreeBinBlock(BlockTreeHeader* block) { assert(block); return block->parent == block; }
    bool _isHeadTreeBinBlock(BlockTreeHeader* block) { assert(block); return block->parent; }

private:
    // Bins are a circular linked list of blocks. The _bins pointer points to the last
    // accessed block. This attempts to speed up searching of the large bins due to the
    // fact that similar allocation sizes are more likely appear one after another in time.
    // Tree bins are included in here as well, they can be converted directly to BlockTreeHeader.
    BlockHeader* _bins[NumBins];
    BlockHeader* _reserve;

    // binMap stores which of the 64 bins have allocations in them to prevent traversing
    // the entire list when searching for a value.
    typedef int64_t BinMap;
    BinMap _binMap;

    // The segment list tracks the raw memory blocks returned from mmap. This is a singly-linked list which
    // ends in a nullptr.
    Segment* _headSegment;

    // Size to allocate the next time a segment needs to be created. Initialized by the constructor.
    size_t _newSegmentSize;

    size_t _alignment;

    // Allocator behaviour options
    bool _doSystemAllocation;
    bool _doBlockMerging;
    bool _doSegmentMerging;
};

inline void* HeapAllocator::_getBlockData(BlockHeader* block) const
{
    // The content is just passed the block meta-data
    assert(block);
    return (void*)(((char*)block) + BlockHeaderSize);
}

inline BlockFooter* HeapAllocator::_getBlockFooter(BlockHeader* block) const
{
    assert(block);
    return (BlockFooter*)(((char*)block) + BlockHeaderSize + _getBlockSize(block));
}

inline BlockHeader* HeapAllocator::_getDataHeader(void* data) const
{
    assert(data);
    return (BlockHeader*)((char*)data - BlockHeaderSize);
}

inline size_t HeapAllocator::_getBlockSize(BlockHeader* block) const
{
    assert(block);
    return block->head & BlockSizeBitMask;
}

inline size_t HeapAllocator::_getBlockSize(BlockTreeHeader* block) const
{
    return _getBlockSize((BlockHeader*)block);
}

inline size_t HeapAllocator::_getBlockSize(BlockFooter* footer) const
{
    assert(footer);
    return footer->foot & BlockSizeBitMask;
}

inline void HeapAllocator::_setBlockSize(BlockHeader* block, size_t size) const
{
    assert(block);
    assert(size <= MaxAllocationSize);
    
    // Size is already guaranteed to not have any of the flag bit positions set
    block->head= (block->head & BlockFlagsBitMask) | size;
}

inline bool HeapAllocator::_isBlockAllocated(BlockHeader* block) const
{
    assert(block);
    return (block->head & BlockAllocatedBitMask) == BlockAllocatedBitMask;
}

inline void HeapAllocator::_setBlockAllocated(BlockHeader* block, bool isAllocated) const
{
    assert(block);
    block->head = (block->head & ~BlockAllocatedBitMask) | (BlockAllocatedBitMask*static_cast<size_t>(isAllocated));
}

inline bool HeapAllocator::_isBlockFencePost(BlockHeader* block) const
{
    assert(block);
    return (block->head & BlockFencePostBitMask) == BlockFencePostBitMask;
}

inline void HeapAllocator::_setBlockFencePost(BlockHeader* block, bool isFencePost) const
{
    assert(block);
    block->head = 
        (block->head & ~BlockFencePostBitMask) | 
        (BlockFencePostBitMask*static_cast<size_t>(isFencePost));
}

inline bool HeapAllocator::_isBlockExternal(BlockHeader* block) const
{
    assert(block);
    return (block->head & BlockExternalBitMask) == BlockExternalBitMask;
}

inline void HeapAllocator::_setBlockExternal(BlockHeader* block, bool isAllocated) const
{
    assert(block);
    block->head = 
        (block->head & ~BlockExternalBitMask) | 
        (BlockExternalBitMask*static_cast<size_t>(isAllocated));
}

inline void HeapAllocator::_setBlockState(BlockHeader* block, size_t size, bool isAllocated) const
{
    _setBlockSize(block, size);
    _setBlockAllocated(block, isAllocated);
}

inline void HeapAllocator::_reconcileFooter(BlockHeader* block) const
{
    assert(block);
    BlockFooter* footer = _getBlockFooter(block);
    footer->foot = _getBlockSize(block);
}

inline BlockHeader* HeapAllocator::_getFirstSegmentBlock(Segment* segment) const
{
    assert(segment);
    return (BlockHeader*)(((char*)segment) + sizeof(Segment) + _getSegmentOffset(segment) + sizeof(BlockFooter));
}

inline bool HeapAllocator::_isSegmentExternal(Segment* segment) const
{
    assert(segment);
    return (segment->flags & SegmentExternalBitMask) == SegmentExternalBitMask;
}

inline void HeapAllocator::_setSegmentExternal(Segment* segment, bool isExternal) const
{
    assert(segment);
    segment->flags = 
        (segment->flags & ~SegmentExternalBitMask) | 
        (SegmentExternalBitMask*static_cast<size_t>(isExternal));
}

inline size_t HeapAllocator::_getSegmentOffset(Segment* segment) const
{
    assert(segment);
    return segment->flags & SegmentOffsetBitMask;
}

inline void HeapAllocator::_setSegmentOffset(Segment* segment, size_t offset) const
{
    assert(segment);
    segment->flags = (segment->flags & SegmentFlagsBitMask) | offset;

    // Segments have some offset space (between 0 -> (alignment - 1)) which
    // is specified in the segment itself (set above) and in a hidden
    // structure just before the first block. This allows us to find the
    // offset given a Segment, or given the firt block in that segment.
    BlockFooter* footer = (BlockFooter*)((char*)segment + sizeof(Segment) + offset);
    footer->foot = offset;
}

inline size_t HeapAllocator::_getTreeBinShift(size_t binIndex) const
{
    // Each bin has a maximum size stored within it. This number is number of
    // bits to shift that size to the left such that a 1 is in the most significant
    // place. 9 is a bit of a magic number, but its actually the leftmost set bit
    // in the smallest tree bin (bin 32)
    size_t s = binIndex - NumSmallBins;
    return sizeof(size_t)*CHAR_BIT - 9 - (s >> 1);
}



} // namespace mem

#endif

