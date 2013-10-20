#include "mem/heapAllocator.h"
using namespace mem;

#include "mem/util.h"
#include "util/bit.h"
#include "util/forever.h"
#include "util/stl.h"
#include "util/string.h"

const size_t HeapAllocator::MinAllocationSize;
const size_t HeapAllocator::MaxAllocationSize;
const size_t HeapAllocator::MaxSmallBinSize;
const size_t HeapAllocator::MaxTreeBinSize;
const size_t HeapAllocator::NumSmallBins;
const size_t HeapAllocator::NumTreeBins;
const size_t HeapAllocator::NumBins;
const size_t HeapAllocator::BlockHeaderSize;
const size_t HeapAllocator::BlockOverheadSize;
const size_t HeapAllocator::BlockAllocatedBitMask;
const size_t HeapAllocator::BlockFencePostBitMask;
const size_t HeapAllocator::BlockExternalBitMask;
const size_t HeapAllocator::BlockFlagsBitMask;
const size_t HeapAllocator::BlockSizeBitMask;
const size_t HeapAllocator::SegmentExternalBitMask;
const size_t HeapAllocator::SegmentFlagsBitMask;
const size_t HeapAllocator::SegmentOffsetBitMask;

HeapAllocator::HeapAllocator(size_t initialAllocSize, size_t alignment) :
    _reserve(nullptr),
    _binMap(0),
    _headSegment(nullptr),
    _newSegmentSize(initialAllocSize),
    _alignment(alignment),
    _doSystemAllocation(true),
    _doBlockMerging(true),
    _doSegmentMerging(true)
{
    //Log::debug("Initializing allocator to size %zu, alignment %zu", initialAllocSize, alignment);
    std::fill(std::begin(_bins), std::end(_bins), nullptr);

    BlockHeader* block = _allocNewSegment(initialAllocSize, false);
    _linkBlock(block);
}

HeapAllocator::~HeapAllocator()
{
    // Nothing to do
}

void* HeapAllocator::allocate(size_t numBytes, size_t, size_t)
{
    // Force a minimum allocation size. This ensures a zero byte
    // allocation actually returns something valid and that we
    // don't stomp over internal information stored in free blocks
    size_t allocSize = std::max(numBytes, MinAllocationSize);

    //Log::debug("Requesting alloc of %zu bytes (align=%zu)", allocSize, _alignment);
    void* mem = nullptr;

    // Extra large allocations are allocated their own segments and not managed
    // by the bin structure.
    if (_isSmallAlloc(numBytes)) {
        mem = _allocFromSmallBin(allocSize);
        if (mem) {
            assert((size_t)(mem)%_alignment == 0 && "Alignment incorrect");
            return mem;
        }

        mem = _allocFromReserve(allocSize);
        if (mem) {
            assert((size_t)(mem)%_alignment == 0 && "Alignment incorrect");
            return mem;
        }

        mem = _allocFromTreeBin(allocSize);
        if (mem) {
            assert((size_t)(mem)%_alignment == 0 && "Alignment incorrect");
            return mem;
        }
    } else if (!_isLargeAlloc(numBytes)) {
        mem = _allocFromTreeBin(allocSize);
        if (mem) {
            assert((size_t)(mem)%_alignment == 0 && "Alignment incorrect");
            return mem;
        }

        mem = _allocFromReserve(allocSize);
        if (mem) {
            assert((size_t)(mem)%_alignment == 0 && "Alignment incorrect");
            return mem;
        }
    }

    // No luck in the reserve or any of the bins
    if (_doSystemAllocation) {
        mem = _allocFromSystem(allocSize);
        if (mem) {
            assert((size_t)(mem)%_alignment == 0 && "Alignment incorrect");
            return mem;
        }
    }

    //Log::debug("Unable to allocate memory of size %zu", allocSize);
    assert(false && "Out of memory");

    return nullptr;
}

void HeapAllocator::release(void* addr)
{
    if (addr) {
        BlockHeader* header = _getDataHeader(addr);
        //Log::debug("Releasing block %p", header);

        assert(header);
        assert(_blockBelongsToAllocator(header) && "Address doesn't belong to this allocator");
        assert(_isBlockAllocated(header) && "Double free on address");

        _setBlockAllocated(header, false);

        if (_isBlockExternal(header)) {
            Segment* segment = _getSegment(header); 
            _releaseExternalSegment(segment);
        } else {
            _linkBlock(header);
        }
    }
}

void HeapAllocator::clear() 
{
    // Reset the first block of each segment to be the maximum size and not in use.
    Segment* segment = _headSegment;
    while (segment) {
        if (_isSegmentExternal(segment)) {
            _releaseExternalSegment(segment);
        } else {
            BlockHeader* block = _getFirstSegmentBlock(segment);    
            _setBlockAllocated(block, false);
            _setBlockFencePost(block, true);
            _setBlockSize(block, segment->size - sizeof(Segment));
            _reconcileFooter(block);
        }
        segment = segment->next;
    }
}

bool HeapAllocator::check(std::vector<HeapAllocator::Block>* corruptBlocks) 
{
    bool foundCorrupt = false;

    Segment* segment = _headSegment;
    while (segment) {
        BlockHeader* block = _getFirstSegmentBlock(segment);
        while (block) {
            if (!_checkBlock(block)) {
                foundCorrupt = true;
                if (corruptBlocks) {
                    Block b;    
                    b.addr = (void*)block;
                    b.data = _getBlockData(block);
                    b.segment = (void*)segment;
                    b.size = _getBlockSize(block);
                    b.isAllocated = _isBlockAllocated(block);
                    corruptBlocks->push_back(b);
                }
            }
            block = _getNextBlock(block);
        } 
        segment = segment->next;
    }

    return !foundCorrupt;
}

typename HeapAllocator::Stats HeapAllocator::getStats() const
{
    HeapAllocator::Stats stats;
    stats.allocatedBytes = 0;
    stats.freeBytes = 0;
    stats.allocatedBlocks = 0;
    stats.freeBlocks = 0;
    stats.overheadBytes = 0;
    stats.numRegularSegments = 0;
    stats.numExternalSegments = 0;

    Segment* segment = _headSegment;
    while (segment) {
        if (_isSegmentExternal(segment)) {
            stats.numExternalSegments++;
        } else {
            stats.numRegularSegments++;
        }

        stats.overheadBytes += _getSegmentOverhead(segment);

        BlockHeader* block = _getFirstSegmentBlock(segment);
        while (block) {
            if (_isBlockAllocated(block)) {
                stats.allocatedBytes += _getBlockSize(block);
                stats.allocatedBlocks++;
            } else {
                stats.freeBytes += _getBlockSize(block);
                stats.freeBlocks++;
            }
            stats.overheadBytes += BlockOverheadSize;
            block = _getNextBlock(block);
        }
        segment = segment->next;
    }

    return stats;
}

std::vector<typename HeapAllocator::Block> HeapAllocator::getBlocks() const 
{
    std::vector<Block> blocks;

    Segment* segment = _headSegment;
    while (segment) {
        BlockHeader* block = _getFirstSegmentBlock(segment);
        while (block) {
            Block b;    
            b.addr = (void*)block;
            b.data = _getBlockData(block);
            b.segment = (void*)segment;
            b.size = _getBlockSize(block);
            b.bin = _getBinIndex(b.size);
            b.isAllocated = _isBlockAllocated(block);
            blocks.push_back(b);

            block = _getNextBlock(block);
        }
        segment = segment->next;
    }

    return blocks;
}

void* HeapAllocator::_allocFromSmallBin(size_t numBytes)
{
    size_t binIndex = _getBinIndex(numBytes);
    assert(binIndex < NumSmallBins);

    //Log::debug("Attempting to allocate %zu bytes from small bins", numBytes);

    // Be sure to only include small bins in the bin map
    BinMap binMap = _binMap;
    binMap &= 
        ~((static_cast<BinMap>(1) << binIndex) - 1) &
        ((static_cast<BinMap>(1) << NumSmallBins) - 1);

    binIndex = util::findFirstSet(binMap);
    if (binIndex == 0) {
        //Log::debug("... No bins contain blocks of sufficient size");
        return nullptr;
    }
    binIndex--;

    assert(binIndex < NumSmallBins);
    BlockHeader* block = _bins[binIndex];
    assert(block);

    // Only free blocks should ever be in the bin
    assert(!_isBlockAllocated(block));
    _unlinkSmallBinBlock(block, binIndex);

    // If the block we found is at least twice the size we requested, split it
    size_t blockSize = _getBlockSize(block);
    if (blockSize >= 2*numBytes) {
        BlockHeader* split = _splitBlock(block, numBytes);

        // Have to set this before linking again to prevent it from being merged right back in
        _setBlockAllocated(split, true);
        if (split != block) {
            _linkBlock(block);
        }
        block = split;
    }

    _setBlockAllocated(block, true);

    //Log::debug("Allocated block %p from small bin %zu", block, binIndex);
    return _getBlockData(block);
}

void* HeapAllocator::_allocFromTreeBin(size_t numBytes)
{
    // This function will only search the tree bins, 32 and up
    int binIndex = _getBinIndex(numBytes);
    binIndex = std::max(static_cast<int>(NumSmallBins), binIndex);

    //Log::debug("Attempting to allocate %zu bytes from tree bins", numBytes);

    BinMap binMap = _binMap;
    binMap &= ~((static_cast<BinMap>(1) << binIndex) - 1);

    BlockTreeHeader* treeBlock = nullptr;
    while (!treeBlock) {
        // Consult the binmap to find the next bin to check
        binIndex = util::findFirstSet(binMap);
        if (binIndex == 0) {
            //Log::debug("... No bins contain blocks of sufficient size");
            return nullptr;
        }
        binIndex--;

        binMap = util::resetBit(binMap, binIndex);

        // Search trie tree for closest block
        //Log::debug("... Checking bin %zu", binIndex);
        treeBlock = _findTreeBlock(binIndex, numBytes);
    }

    // Convert the treeBlock to a block, it's just a regular block from here on out
    BlockHeader* block = (BlockHeader*)(treeBlock);
    size_t blockSize = _getBlockSize(block);

    // Test if a split is in order. We must unlink the block before splitting
    //Log::debug("... Found (%p,%zu) in bin %zu", treeBlock, blockSize, binIndex);
    _unlinkTreeBlock(treeBlock, binIndex);
    assert(!_isBlockAllocated(block));

    if (blockSize > numBytes + BlockOverheadSize) {
        // Splits don't always return two blocks but if this one does, add the remainder back to a bin
        BlockHeader* split = _splitBlock(block, numBytes);

        // Have to set this before linking again to prevent it from being merged right back in
        _setBlockAllocated(split, true);
        if (split != block) {
            _linkBlock(block);
        }
        block = split;
    } else {
        _setBlockAllocated(block, true);
    } 

    //Log::debug("Allocated block %p from bin %zu", block, binIndex);
    return _getBlockData(block);
}

void* HeapAllocator::_allocFromReserve(size_t numBytes)
{
    //Log::debug("Attempting to allocate %zu bytes from reserve ", numBytes);

    size_t reserveSize = _reserve ? _getBlockSize(_reserve) : 0;
    if (reserveSize < numBytes + BlockOverheadSize) {
        //Log::debug("... Reserve is empty or too small");
        return nullptr;
    }

    //Log::debug("... Reserve block (%p,%zu) found", _reserve, reserveSize);

    BlockHeader* split = _splitReserveBlock(numBytes);
    //Log::debug("Allocated block %p from reserve", split);

    _setBlockAllocated(split, true);
    return _getBlockData(split);
}

void* HeapAllocator::_allocFromSystem(size_t numBytes)
{
    assert(_doSystemAllocation);
    bool isExternalSegment = _isLargeAlloc(numBytes);

    // The size to allocate must be at least numBytes and doubles
    // everytime we do a system allocation.
    size_t newSegmentSize = std::max(numBytes + BlockOverheadSize, _newSegmentSize); 
    newSegmentSize = std::min(newSegmentSize, MaxAllocationSize - BlockOverheadSize);
    _newSegmentSize *= 2;

    BlockHeader* block = _allocNewSegment(newSegmentSize, isExternalSegment);

    BlockHeader* split = block;
    if (!isExternalSegment) {
        split = _splitBlock(block, numBytes);

        // Link only if there actually was a split made
        if (split != block) {
            // Have to set this before linking again to prevent it from being merged right back in
            _setBlockAllocated(split, true);
            _linkBlock(block);
        }
    }

    // Do this again, it doesn't hurt 
    _setBlockAllocated(split, true); 

    //Log::debug("Allocated block %p from new segment", split);
    return _getBlockData(split);
}

size_t HeapAllocator::_getBinIndex(size_t numBytes) const
{
    assert(numBytes > 0);
    if (_isSmallAlloc(numBytes)) {
        return numBytes/8;
    } else if (numBytes < MaxTreeBinSize) {
        // This is very magical and is based on a property of the bits representing sizes.
        // It just so happens that the index of the most significant set bit uniquely
        // narrows down the bit index to one of two bins. The value of the next bit
        // further narrows it down.
        // e.g. size=100000000 has its first bit in index 9. This is in the first
        // tree bin which is 9 + 23 + (9 - 9) = 32.
        size_t lsb = util::findLastSet(numBytes);
        size_t bin = lsb + 23 + (lsb - 9);

        if (util::checkBit(numBytes, lsb - 2)) {
            bin++;
        }
        return bin;
    } else {
        // Any allocations larger than the max tree bin size get stuffed
        // into the last tree bin. This can happen naturally when large blocks
        // get coalesced.
        return NumBins - 1;
    }
}

void HeapAllocator::_initBlock(BlockHeader* block, size_t numBytes, bool isExternal) const
{
    _setBlockSize(block, numBytes);
    _setBlockAllocated(block, false);
    _setBlockExternal(block, isExternal);
    _reconcileFooter(block);
    block->next = nullptr;
    block->prev = nullptr;
}

BlockHeader* HeapAllocator::_linkBlock(BlockHeader* block)
{
    assert(block);

    // Link the block either as the reserve or into the bins
    size_t blockSize = _getBlockSize(block); 

    // Attempt to coalesce before we link since we'd end up unlinking it anyway
    //Log::debug("Linking block (%p,%zu)", block, blockSize);
    bool isSmallAlloc = _isSmallAlloc(blockSize);

    if (_doBlockMerging && !isSmallAlloc) {
        block = _coalesceAdjacentBlocks(block);
        blockSize = _getBlockSize(block);
    }

    size_t reserveSize = 0;
    if (!_reserve && !isSmallAlloc) {
        //Log::debug("... (%p,%zu) is linked as new reserve", block, blockSize);
        _reserve = block;
    } else if (_reserve && blockSize > (reserveSize = _getBlockSize(_reserve))) {
        //Log::debug("... Replacing reserve (%p,%zu) with block (%p,%zu)", _reserve, reserveSize, block, blockSize);
        BlockHeader* oldReserve = _unlinkReserveBlock();
        _linkBlockToBins(oldReserve);
        _reserve = block;
    } else {
        _linkBlockToBins(block);
    }

    assert(_checkBlock(block));
    return block;
}

BlockHeader* HeapAllocator::_linkBlockToBins(BlockHeader* block)
{
    assert(block);
    size_t blockSize = _getBlockSize(block);
    size_t binIndex = _getBinIndex(blockSize);
    bool isSmallAlloc = _isSmallAlloc(blockSize);

    if (isSmallAlloc) {
        _linkSmallBlock(block, binIndex);
    } else {
        _linkTreeBlock(block, binIndex);
    }

    return block;
}

BlockHeader* HeapAllocator::_linkSmallBlock(BlockHeader* block, size_t binIndex)
{
    assert(block);
    assert(binIndex < NumSmallBins);

    BlockHeader* head = _bins[binIndex];
    if (!head) {
        assert(!block->next && !block->prev);
        //Log::debug("... No other blocks in bin %zu, linking as start block", binIndex);

        block->next = block;
        block->prev = block;
    } else {
        //Log::debug("... Linking block before %p in small bin %zu", head, binIndex);

        block->prev = head->prev;
        block->next = head;
        head->prev->next = block; 
        head->prev = block; 
    }
    _bins[binIndex] = block;
    _binMap = util::setBit(_binMap, binIndex);
    return block;
}


BlockHeader* HeapAllocator::_unlinkBlock(BlockHeader* block)
{
    assert(block);
    size_t blockSize = _getBlockSize(block);
    if (block == _reserve) {
        _unlinkReserveBlock();
    } else if (_isSmallAlloc(blockSize)) {
        _unlinkSmallBinBlock(block, _getBinIndex(blockSize));
    } else {
        _unlinkTreeBlock((BlockTreeHeader*)block, _getBinIndex(blockSize));
    }
    return block;
}

BlockHeader* HeapAllocator::_unlinkSmallBinBlock(BlockHeader* block, size_t binIndex)
{
    assert(block && !_isBlockAllocated(block) && _getBinIndex(_getBlockSize(block)) == binIndex);

    //Log::debug("Unlinking free block %p from bin %zu", block, binIndex);

    // A blocks next/prev pointer is always valid since it's a circular linked list
    assert(block->prev && block->next);
    block->prev->next = block->next;
    block->next->prev = block->prev;

    if (block == _bins[binIndex]) {
        if (block->next == block) {
            //Log::debug("... %p was only block in bin, clearing bin", block);
            _bins[binIndex] = nullptr;
            _binMap = util::resetBit(_binMap, binIndex);
        } else {
            //Log::debug("... %p was first block in bin, new first block is %p", block, block->next);
            _bins[binIndex] = block->next;
        }
    }

    block->next = nullptr;
    block->prev = nullptr;
    return block;
}

BlockHeader* HeapAllocator::_coalesceAdjacentBlocks(BlockHeader* block)
{
    //Log::debug("Attempting to coalesce block %p with its neighbours", block);

    // Attempt to merge at most 3 blocks at once. This prevents us from having to unlink/merge/link
    // and then unlink/merge/link again. In theory this can be expanded to merge N blocks
    // as long as they are all unused and contiguous but I'm not sure if the performance benefits
    // are there. Maybe this can be configurable?
    BlockHeader* blocksToMerge[3];
    size_t numBlocks = 0;

    // A min-sized block is created as the last block of a segment to be used as a fencepost
    // We don't want to merge this block
    BlockHeader* prevBlock = _getPrevBlock(block);
    if (prevBlock && !_isBlockAllocated(prevBlock) && _getBlockSize(prevBlock) != 0) {
        assert(_checkBlock(prevBlock));
        blocksToMerge[numBlocks++] = prevBlock;
        _unlinkBlock(prevBlock);
    }

    blocksToMerge[numBlocks++] = block;

    BlockHeader* nextBlock = _getNextBlock(block);
    if (nextBlock && !_isBlockAllocated(nextBlock) && _getBlockSize(block) != 0) {
        blocksToMerge[numBlocks++] = nextBlock;
        _unlinkBlock(nextBlock);
    }

    // Either prev or next, or both will be merged
    if (numBlocks > 1) {
        block = _mergeBlocks(blocksToMerge, numBlocks);
    } 

    assert(_checkBlock(block));
    return block;
}

BlockHeader* HeapAllocator::_mergeBlocks(BlockHeader** blocks, size_t numBlocks)
{
    assert(numBlocks > 1);

    size_t totalSize = 0;

    // It is assumed that all blocks are unlinked
    //Log::debug("Merging %zu blocks", numBlocks); 
    for (size_t i = 0; i < numBlocks; ++i) {
        assert(blocks[i]);
        assert(_getSegment(blocks[i]) == _getSegment(blocks[0]));

        size_t blockSize = _getBlockSize(blocks[i]);
        //Log::debug("... Block (%p,%zu)", blocks[i], blockSize);

        totalSize += blockSize + BlockOverheadSize;
    }

    // This inherits the flags of the first block which is OK
    BlockHeader* merged = blocks[0];
    _setBlockSize(merged, totalSize - BlockOverheadSize);
    _reconcileFooter(merged);

    // The merged block is returned unlinked
    //Log::debug("New merged block (%p,%zu)", merged, totalSize - BlockOverheadSize);
    return merged;
}

BlockHeader* HeapAllocator::_unlinkReserveBlock()
{
    //Log::debug("Unlinking reserve %p", _reserve);
    BlockHeader* block = _reserve;
    _reserve = nullptr;

    block->next = nullptr;
    block->prev = nullptr;
    return block;
}

BlockHeader* HeapAllocator::_splitReserveBlock(size_t numBytes)
{
    BlockHeader* block = _splitBlock(_reserve, numBytes);

    if (block == _reserve) {
        //Log::debug("Remainder smaller than min allocation size, returning whole reserve");
        return _unlinkReserveBlock();
    }

    if (_isSmallAlloc(_getBlockSize(_reserve))) {
        //Log::debug("Reserve is small enough, adding to small bins");

        BlockHeader* oldReserve = _reserve;
        _unlinkReserveBlock();
        _linkBlockToBins(oldReserve);
    }

    return block;
}

BlockHeader* HeapAllocator::_splitBlock(BlockHeader* block, size_t numBytes) const
{
    assert(block && !_isBlockAllocated(block));
    assert(numBytes >= MinAllocationSize);
    assert(_getBlockSize(block) >= numBytes + BlockOverheadSize);

    //Log::debug("Splitting block %p by %zu", block, numBytes);

    size_t remainder = _getBlockSize(block) - numBytes - BlockOverheadSize;
    size_t totalSplitSize = remainder + BlockOverheadSize;

    // Account for alignment by allocating more if we need to
    size_t alignmentCorrection = (((size_t)block) + totalSplitSize + BlockHeaderSize)%_alignment;
    remainder -= alignmentCorrection;
    totalSplitSize -= alignmentCorrection;
    numBytes += alignmentCorrection;

    //Log::debug("... Alignment correction of %zu", alignmentCorrection);

    // Don't split if the block is small enough
    if (remainder < MinAllocationSize + BlockOverheadSize) {
        //Log::debug("... Remainder smaller than min allocation size, returning whole block");
        return block;
    }

    // Flags otherwise remain the same
    _setBlockSize(block, remainder);
    _reconcileFooter(block);

    BlockHeader* split = (BlockHeader*)(((char*)block) + totalSplitSize);
    _initBlock(split, numBytes, _isBlockExternal(block));
    _setBlockFencePost(split, false);

    //Log::debug("Split %p into blocks (%p,%zu) and (%p,%zu)", 
            //block, block, remainder, split, numBytes);

    return split;
}

BlockHeader* HeapAllocator::_allocNewSegment(size_t numBytes, bool isExternal)
{
    size_t pageSize = getpagesize();
    numBytes = mem::align(numBytes + sizeof(Segment) + BlockOverheadSize, pageSize);

    //Log::debug("Allocating %zu bytes from mmap as new segment", numBytes);

    Segment* segment = 
        (Segment*)mmap(0, numBytes, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

    // Size includes header
    segment->prev = nullptr;
    segment->next = nullptr;
    segment->size = numBytes - sizeof(Segment);
    _setSegmentExternal(segment, isExternal);
    _setSegmentOffset(segment, 0);

    //Log::debug("... New segment [%p,%zu] external=%zu", segment, segment->size, (size_t)isExternal);

    // Still link the segment even if it is external since
    // we want to keep track of it for the purposes of determining
    // what segment a block belongs to.
    return _linkSegment(segment);
}

BlockHeader* HeapAllocator::_linkSegment(Segment* segment)
{
    assert(segment);
    //Log::debug("Linking segment [%p,%zu] into segments list", segment, segment->size);

    bool doMerge = false;
    bool isExternal = _isSegmentExternal(segment);

    Segment* segIter = _headSegment;
    while (segIter) {
        // It's so unlikely that this new segment will be adjacent to two segments
        // we don't even check
        if (_doSegmentMerging && !isExternal && _areSegmentsAdjacent(segIter, segment)) {
            doMerge = true;
            break;
        }

        if (!segIter->next)
            break;
        segIter = segIter->next;
    }

    BlockHeader* block = nullptr;
    size_t blockSize = 0;

    if (doMerge) {
        //Log::debug("... Segment [%p,%zu] is adjacent to [%p,%zu], merging", 
                //segIter, segIter->size,
                //segment, segment->size);

        size_t numBytes = segment->size + sizeof(Segment);
        segIter->size = segIter->size + numBytes;

        // The first block of the merged segment is no longer a fencepost
        // and neither is the last block of the allocated segment
        //Log::debug("... Merged segment [%p,%zu]", segIter, segIter->size);

        // Merging is going to leave a fence post block at the end of segIter which
        // causes problems since it is a fencepost in the middle of the segment. We'll 
        // re-purpose it by making it the return block of the new segment
        block = _getPrevBlock((BlockHeader*)segment);
        assert(_checkBlock(block));

        blockSize = numBytes - BlockOverheadSize + BlockOverheadSize + _getBlockSize(block);
        _setBlockFencePost(block, false); 
    } else {
        // Determine correct offset to account for alignment
        block = _getFirstSegmentBlock(segment);
        size_t offset = ((size_t)_getBlockData(block))%_alignment;
        _setSegmentOffset(segment, offset);

        //Log::debug("... Segment alignment offset %zu", offset);

        block = _getFirstSegmentBlock(segment);
        blockSize = segment->size - BlockOverheadSize - offset - sizeof(BlockFooter);

        // block is only a fence post if it's not merged
        _setBlockFencePost(block, true); 

        if (!segIter) {
            //Log::debug("... No existing segments, adding %p", segment);
            _headSegment = segment;
            segment->next = nullptr;
        } else {
            //Log::debug("... Adding %p to segment list following %p", segment, segIter);
            segIter->next = segment;
            segment->prev = segIter;
            segment->next = nullptr;
        }

    }

    _initBlock(block, blockSize, isExternal);

    // Split off a min-length block (just a header/footer) which will be used
    // as the right-most fencepost. This blocks purpose is only to mark
    // the end of the segment to prevent extending passed the bounds
    // of the segment, it is marked as unallocated but doesn't get added
    // to a bin. 
    BlockHeader* rightFence = _splitBlock(block, MinAllocationSize);
    assert(rightFence);
    _setBlockAllocated(rightFence, false);
    _setBlockFencePost(rightFence, true); 

    //Log::debug("... New block %p created for segment", block);
    return block;
}

void HeapAllocator::_releaseExternalSegment(Segment* segment)
{
    assert(segment && _isSegmentExternal(segment));
    //Log::debug("Releasing external segment [%p,%zu]", segment, segment->size);

    if (segment == _headSegment) {
        _headSegment = segment->next;
    } else {
        segment->prev->next = segment->next;
    }

    int err = munmap((void*)segment, segment->size + sizeof(Segment));
    assert(err == 0);
}



Segment* HeapAllocator::_getSegment(BlockHeader* block) const
{
    while (block) {
        BlockHeader* prevBlock = _getPrevBlock(block);
        if (prevBlock) {
            block = prevBlock;
        } else {
            break;
        }
    }

    // Segments have a hidden footer right before the first block
    // which specifies the alignment offset.
    BlockFooter* alignFooter = (BlockFooter*)((char*)block - sizeof(BlockFooter));
    size_t offset = alignFooter->foot;

    Segment* segment = (Segment*)(((char*)block) - sizeof(BlockFooter) - offset - sizeof(Segment));
    return segment;
}

BlockHeader* HeapAllocator::_getNextBlock(BlockHeader* block) const
{
    assert(block);

    // The end of this segment is denoted by a fencepost block
    BlockHeader* nextBlock = (BlockHeader*)(((char*)block) + _getBlockSize(block) + BlockOverheadSize);
    if (_isBlockFencePost(nextBlock)) {
        return nullptr;
    }
    return nextBlock;
}

BlockHeader* HeapAllocator::_getPrevBlock(BlockHeader* block) const
{
    assert(block);
    if (_isBlockFencePost(block)) {
        return nullptr;
    }
    BlockFooter* prevFooter = (BlockFooter*)(((char*)block) - sizeof(BlockFooter));
    BlockHeader* prevBlock = (BlockHeader*)(((char*)prevFooter) - _getBlockSize(prevFooter) - BlockHeaderSize);
    return prevBlock;
}

bool HeapAllocator::_isInSegment(BlockHeader* block, Segment* segment) const
{
    assert(block && segment);
    return (char*)block >= ((char*)segment + sizeof(Segment)) &&
            (char*)block < ((char*)segment + sizeof(Segment) + segment->size);
}

bool HeapAllocator::_areSegmentsAdjacent(Segment* prev, Segment* next) const
{
    Segment* adj = (Segment*)((char*)prev + prev->size + sizeof(Segment));
    return adj == next && !_isSegmentExternal(prev) && !_isSegmentExternal(next);
}

size_t HeapAllocator::_getSegmentOverhead(Segment* segment) const
{
    return sizeof(Segment) + _getSegmentOffset(segment) + sizeof(BlockFooter);
}

bool HeapAllocator::_checkBlock(BlockHeader* block) const
{
    // The header/footer being out of sync indicates corruption
    BlockFooter* footer = _getBlockFooter(block);
    if (_getBlockSize(block) != _getBlockSize(footer)) {
        return false;
    }
    return true;
}

bool HeapAllocator::_blockBelongsToAllocator(BlockHeader* block) const
{
    Segment* blockSegment = _getSegment(block);
    assert(blockSegment);

    Segment* segment = _headSegment;
    while (segment) {
        if (segment == blockSegment) {
            return true;
        }
        segment = segment->next;
    }

    return false;
}

BlockTreeHeader* HeapAllocator::_linkTreeBlock(BlockHeader* block, size_t binIndex)
{
    assert(block);
    assert(binIndex >= NumSmallBins && binIndex < NumSmallBins + NumTreeBins);
    BlockTreeHeader* treeBlock = (BlockTreeHeader*)block;
    size_t blockSize = _getBlockSize(block);

    BlockTreeHeader* root = (BlockTreeHeader*)(_bins[binIndex]);
    if (!root) {
        //Log::debug("... (%p,%zu) is first block in tree bin %zu", block, blockSize, binIndex);

        // The root tree block has a null parent. Non-head elements of a tree block chain
        // also have null parents, but they are differentiated by not being == to the bin root
        treeBlock->parent = nullptr;
        treeBlock->child[0] = nullptr;
        treeBlock->child[1] = nullptr;
        treeBlock->next = treeBlock;
        treeBlock->prev = treeBlock;
        _bins[binIndex] = block;

        _binMap = util::setBit(_binMap, binIndex);
        return treeBlock;
    }

    size_t shift = _getTreeBinShift(binIndex);
    //Log::debug("... (%p,%zu) will be added to tree bin %zu under root %p", block, blockSize, binIndex, root);

    // This is the starting bit-sequence representing the root of the block tree
    // at this particular bin index. Each iteration we shift left once and
    // look at the value of the msb to determine whether to follow the left
    // or right subtree.
    size_t bits = _getBlockSize(block) << shift;

    BlockTreeHeader* iter = root;
    FOREVER {
        size_t msb = util::msb(bits);
        //Log::debug("... Processing %p, select child %zu", iter, msb);

        // Found a leaf, add this new block
        BlockTreeHeader* child = iter->child[msb];
        if (!child) {
            //Log::debug("... Adding %p as child %zu of %p", treeBlock, msb, iter);
            iter->child[msb] = treeBlock;

            treeBlock->parent = iter;
            treeBlock->child[0] = nullptr;
            treeBlock->child[1] = nullptr;
            treeBlock->prev = treeBlock;
            treeBlock->next = treeBlock;
            return treeBlock;
        }

        // Blocks with the same size are added as a chain
        if (_getBlockSize(child) == blockSize) {
            assert(treeBlock != child);
            //Log::debug("... Adding %p to a chain with head block %p", treeBlock, child);
            treeBlock->next = child->next;
            treeBlock->prev = child;
            child->next->prev = treeBlock;
            child->next = treeBlock;
            if (child->prev == child) {
                child->prev = treeBlock;
            }
            return treeBlock;
        }

        bits <<= 1;
        iter = child;
    }

    // We should always return out of the forever loop
    assert(false && "Should never exit loop");
    return nullptr;
}

BlockTreeHeader* HeapAllocator::_findTreeBlock(size_t binIndex, size_t numBytes) const
{
    // This attempts to find the closest block which can accomodate numBytes. It does so by finding
    // the sub-tree of root such that all blocks have a size >= numBytes. The smallest block (left-
    // most block) in that subtree is therefore the best fit.
    assert(binIndex >= NumSmallBins && binIndex < NumBins);
    BlockHeader* root = _bins[binIndex];
    assert(root);

    //Log::debug("Searching bin %zu for best-fit to %zu bytes", binIndex, numBytes);

    BlockTreeHeader* treeRoot = (BlockTreeHeader*)root;

    size_t shift = _getTreeBinShift(binIndex);
    size_t bits = _getBlockSize(treeRoot) << shift;
    size_t error = std::numeric_limits<size_t>::max();
    BlockTreeHeader* bestFitBlock = NULL;

    BlockTreeHeader* iter = treeRoot;
    BlockTreeHeader* rest = NULL;
    while (1) {
        if (_getBlockSize(iter) >= numBytes) {
            size_t e = _getBlockSize(iter) - numBytes;
            if (e < error) {
                error = e;
                bestFitBlock = iter;  

                if (error == 0) {
                    return bestFitBlock;
                }
            }
        }

        // Follow the right child if we can
        BlockTreeHeader* rightChild = iter->child[0];
        iter = iter->child[util::msb(bits)];

        if (rightChild && rightChild != iter) {
            rest = rightChild; 
        }
        if (!iter) {
            iter = rest;
            break;
        }
        bits <<= 1;
    }

    // iter is now the first block in a subtree which is all just larger
    // than the desired size. No other block larger than size exists in
    // any other subtree. 
    if (iter) {
        BlockTreeHeader* candidate = _getSmallestTreeBlock(iter);
        if (candidate) {
            size_t candErr = _getBlockSize(candidate) - numBytes;
            if (candErr < error) {
                bestFitBlock = candidate;
            }
        }
    }
    
    return bestFitBlock;
}

BlockTreeHeader* HeapAllocator::_unlinkTreeBlock(BlockTreeHeader* block, size_t binIndex)
{
    //Log::debug("Unlinking tree block %p from bin %zu", block, binIndex);
    assert(block);
    assert(block->prev && block->next);

    // If this block is part of a chain of duplicates, just remove it from there
    // If the block isn't the first in the chain (null parent) we unlink it from the chain. 
    // If it is the first in the chain, we have to link the next node properly into the tree.
    if (block->next != block) {
        //Log::debug("... part of chain");

        // Test for head block of chain
        if (block->parent || (BlockHeader*)block == _bins[binIndex]) {
            BlockTreeHeader* newHead = block->next;
            _replaceTreeBlock(block, newHead, binIndex);
        }

        block->prev->next = block->next;
        block->next->prev = block->prev;
    } else if (!block->child[0] && !block->child[1]) {
        // Any leaf nodes can be removed from their parent without replacement
        if ((BlockHeader*)block == _bins[binIndex]) {
            //Log::debug("... bin %zu is now empty", binIndex);
            _binMap = util::resetBit(_binMap, binIndex);
            _bins[binIndex] = nullptr;
        } else {
            //Log::debug("... leaf node");
            _unlinkTreeLeafBlock(block);
        }
    } else {
        // Other nodes can be replaced by any leaf in the sub-tree starting at that node
        assert(block->child[0] || block->child[1]);

        // There's no reason it needs to be the small tree block, it just needs to be a leaf
        BlockTreeHeader* repl = _getSmallestTreeBlock(block);
        _unlinkTreeLeafBlock(repl);
        _replaceTreeBlock(block, repl, binIndex);
    }

    block->prev = nullptr;
    block->next = nullptr;
    block->parent = nullptr;
    return nullptr;
}

void HeapAllocator::_replaceTreeBlock(BlockTreeHeader* block, BlockTreeHeader* repl, size_t binIndex)
{
    assert(block);
    assert(block->parent || (BlockHeader*)block == _bins[binIndex]);
    //Log::debug("... replacing block %p with %p", block, repl);

    if (block->parent) {
        if (block->parent->child[0] == block) {
            block->parent->child[0] = repl;
        } else if (block->parent->child[1] == block) {
            block->parent->child[1] = repl;
        }
    }

    repl->parent = block->parent;
    repl->child[0] = block->child[0];
    repl->child[1] = block->child[1];

    if (repl->child[0]) {
        repl->child[0]->parent = repl;
    }
    if (repl->child[1]) {
        repl->child[1]->parent = repl;
    }

    if ((BlockHeader*)block == _bins[binIndex]) {
        //Log::debug("... %p was the head of the tree, replacing with %p", block, repl);
        _bins[binIndex] = (BlockHeader*)repl;
        repl->parent = nullptr;
    }
    assert(repl->child[0] != repl);
    assert(repl->child[1] != repl);
}

BlockTreeHeader* HeapAllocator::_unlinkTreeLeafBlock(BlockTreeHeader* leaf)
{
    assert(leaf && leaf->parent);
    if (leaf->parent->child[0] == leaf) {
        leaf->parent->child[0] = nullptr;
    } else {
        leaf->parent->child[1] = nullptr;
    }
    return leaf;
}

BlockTreeHeader* HeapAllocator::_getSmallestTreeBlock(BlockTreeHeader* root) const
{
    // Follow the left-most child if we can. When we hit a leaf that's the smallest block
    // in the root subtree.
    assert(root);
    BlockTreeHeader* iter = root;
    while (iter->child[0] || iter->child[1]) {
        if (iter->child[0]) {
            iter = iter->child[0];
        } else {
            iter = iter->child[1];
        }
    }
    return iter;
}

