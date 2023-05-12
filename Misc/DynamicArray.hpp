/*
*		File name:
*			DynamicArray.hpp
*
*		Use:
*			Minimalistic dynamic array.
*
*		Author:
*			Xyrem ( https://reversing.info | Xyrem@reversing.info )
*/


#pragma once
#include "..\Common.hpp"

template<typename T>
class DynamicArray
{
private:
	T* Objects;
	int Count;
	int MaxCapacity;
	KSPIN_LOCK Spinlock;

	/*
	*	Initialize the array by allocating memory for the objects. 
	*/
	void Initialize( )
	{
		KIRQL Irql = EnterLock( );
		if ( !Objects )
		{
			// Simply accept 64 entries as a starting point.
			MaxCapacity = sizeof( T ) * 64;
			Objects = (T*)ExAllocatePool( POOL_TYPE::NonPagedPoolNx, MaxCapacity );

			// Should never happen, unless the system is out of resources, so just crash the system at that point.
			if ( !Objects )
				__fastfail( 'POOL' );

			// Erase all artifacts of previously allocated stuff.
			memset( Objects, 0, MaxCapacity );
		}
		ExitLock( Irql );
	}

public:

	/*
	*	Enter a spinlock for thread safety.
	*/
	KIRQL EnterLock( )
	{
		return KeAcquireSpinLockRaiseToDpc( &Spinlock );
	}

	/*
	*	Leave the spinlock.
	*/
	void ExitLock( _In_ KIRQL Irql )
	{
		KeReleaseSpinLock( &Spinlock, Irql );
	}

	/*
	*	Removes all entries.
	*/
	void Clear( )
	{
		KIRQL Irql = EnterLock( );
		if ( Objects && Count )
		{
			memset( Objects, 0, sizeof( T ) * Count );
			Count = 0;
		}
		ExitLock( Irql );
	}

	/*
	*	Remove all entries and free the allocation.
	*/
	void Destroy( )
	{
		if( !Objects )
			return;
		
		KIRQL Irql = EnterLock( );

		// Erase everything.
		memset( Objects, 0, Count * sizeof( T ) );
		
		// Free the allocation.
		ExFreePool( Objects );

		Count = 0;
		MaxCapacity = 0;
		Objects = 0;
		ExitLock( Irql );
	}

	/*
	*	Add an item to the array.
	*/
	void Insert( _In_ T Item )
	{
		// Has the array not been initialized yet?
		if ( !Objects )
			Initialize( );

		KIRQL Irql = EnterLock( );
		
		// Have we ran out of space for a new item? If so, allocate a pool x2 the size and copy over data.
		if ( sizeof(T) + Count * sizeof(T) > MaxCapacity )
		{
			MaxCapacity *= 2;
			T* NewArray = (T*)ExAllocatePool( POOL_TYPE::NonPagedPoolNx, MaxCapacity );

			// If this is null, the system is out of resources, there's nothing we can do now, so crash the system.
			if ( !NewArray )
				__fastfail( 'POOL' );

			// We can ignore the *existing* items as they will be overwritten anyway.
			memset( &NewArray[Count], 0, MaxCapacity - (sizeof(T) * Count));

			// Copy over the existing items to the new array.
			// Suppressing / disabling the warning for C6387 isnt working for some reason, so well we ignore this lol..
			memcpy( NewArray, Objects, Count * sizeof( T ) );

			// Free the original pool.
			ExFreePool( Objects );
			
			// Set the new array.
			Objects = NewArray;
		}

		// Insert the new item.
		Objects[ Count++ ] = Item;
		ExitLock( Irql );
	}

	/*
	*	Checks if the item is in the array.
	*/
	bool Contains( _In_ T Item )
	{
		// Has the array not been initialized yet? If so, initialize and return false as there
		// will not be any entries anyway.
		if ( !Objects )
		{
			Initialize( );
			return false;
		}

		KIRQL Irql = EnterLock( );
		// Loop through the array in search of the item.
		for ( int i = 0; i < Count; i++ )
		{
			if ( Objects[ i ] == Item )
			{
				ExitLock( Irql );
				return true;
			}
		}

		// Not found
		ExitLock( Irql );
		return false;
	}

	/*
	*	Get the item in the array with the index provided,
	*	also do certain sanity checks.
	*/
	T operator[]( int i )
	{
		// Has the array not been initialized yet? If so, initialize and return nothing as there
		// will not be any entries anyway.
		if ( !Objects )
		{
			Initialize( );
			return {};
		}

		// Is the index specified more than the number of items, or is it negative? If so trigger a crash to analyze this bug..
		if ( i > Count || i < 0 )
			__fastfail( 0xBAD128 );

		// Return the item.
		return Objects[ i ];
	}

	/*
	*	Gets the number of items.
	*/
	int Size( )
	{
		return Count;
	}
};
