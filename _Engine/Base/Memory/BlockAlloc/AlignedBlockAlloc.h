/*
=============================================================================
	File:	AlignedBlockAllocator.h
	Desc:
=============================================================================
*/

#ifndef __MX_MEMORY_ALIGNED_BLOCK_ALLOC_H__
#define __MX_MEMORY_ALIGNED_BLOCK_ALLOC_H__

namespace lol {

// by James Johnson [kmeson@telocity.com]
///////////////////////////////////////////////////////////////////////////////
// Class:           AlignedBlockAllocator
// Base Classes:    std::allocator<T>
// Type:            template class
// Inheritance:     public
// Desc:            implements a block aligned allocator. Allocation occurs on
//                  T_ALIGNMENT boundary. This boundary must be a power of 2.
//                  The minimum block size allocated is T_BLOCKSIZE and must be
//                  non-ZERO. Allocations smaller than	T_BLOCKSIZE are rounded
//                  up to the nears multiple of T_BLOCKSIZE.
//
//                  Default alignment is 16 bytes
//                  Default block size is 8 bytes
//
// Usage:
//
//    std::vector<float,AlignedBlockAllocator<float> >	vecFloatAlign16by8;
//    vecFloatAlign16by8.resize(1024,1.0f)
//
///////////////////////////////////////////////////////////////////////////////

template < typename T, mxUInt T_ALIGNMENT = 16, mxUInt T_BLOCKSIZE = 8 >
class AlignedBlockAllocator : public std::allocator< T > {
private:
	typedef std::allocator< T >  BASE_TYPE;

public:
	AlignedBlockAllocator()
	{}

	AlignedBlockAllocator & operator = ( const aligned_Alloc& r )
	{
		BASE_TYPE::operator = ( r );
		return *this;
	}

	pointer allocate( size_type n, const void *hint )
	{
		pointer p = NULL;
		mxUInt byteCount = sizeof( T ) * n;
		mxUInt byteCountLeft = byteCount % T_BLOCKSIZE;
		if(byteCountLeft){
			byteCount += T_BLOCKSIZE -  byteCountLeft;
		}
		if ( ! hint ) {
			p = reinterpret_cast< pointer >( _aligned_malloc(byteCount,T_ALIGNMENT) );
		} else {
			p = reinterpret_cast< pointer >( _aligned_realloc((void*)hint,byteCount,T_ALIGNMENT) );
		}
		return p;
	}

	void deallocate(pointer p, size_type n){
		_aligned_free(p);
	}

	void construct(pointer p, const T &val){
		new(p) T(val);
	}

	void destroy(pointer p){
		p->~T();
	}
};

}//End of namespace lol

#endif /* ! __MX_MEMORY_ALIGNED_BLOCK_ALLOC_H__ */

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
