#ifndef _DARRAY_H
#define _DARRAY_H

/* File: darray.h
 * --------------
 * Defines the interface for the DynamicArray ADT.
 * The DArray allows the client to store any number of elements of any desired
 * base type and is appropriate for a wide variety of storage problems. It 
 * supports efficient element access, and appending/inserting/deleting elements
 * as well as optional sorting and searching. In all cases, the DArray imposes 
 * no upper bound on the number of elements and deals with all its own memory 
 * management. The client specifies the size (in bytes) of the elements that 
 * will be stored in the array when it is created. Thereafter the client and 
 * the DArray can refer to elements via (void*) ptrs.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Type: DArray
 * ----------------
 * Defines the DArray type itself. The client can declare variables of type 
 * DArray, but these variables must be initialized with the result of ArrayNew.
 * The DArray is implemented with pointers, so all client copies in variables
 * or parameters will be "shallow" -- they will all actually point to the
 * same DArray structure.  Only calls to ArrayNew create new arrays.
 * The struct declaration below is "incomplete"- the implementation
 * details are literally not visible in the client .h file.
 */
typedef struct DArrayImplementation *DArray;


/* ArrayCompareFn
 * --------------
 * ArrayCompareFn is a pointer to a client-supplied function which the
 * DArray uses to sort or search the elements. The comparator takes two 
 * (const void*) pointers (these will point to elements) and returns an int.
 * The comparator should indicate the ordering of the two elements
 * using the same convention as the strcmp library function:
 * If elem1 is "less than" elem2, return a negative number.
 * If elem1 is "greater than" elem2, return a positive number.
 * If the two elements are "equal", return 0.
 */
#if defined(WIN32)
	typedef int (__cdecl *ArrayCompareFn)(const void *elem1, const void *elem2);
#else
	typedef int (*ArrayCompareFn)(const void *elem1, const void *elem2);
#endif


/* ArrayMapFn
 * ----------
 * ArrayMapFn defines the space of functions that can be used to map over
 * the elements in a DArray.  A map function is called with a pointer to
 * the element and a client data pointer passed in from the original
 * caller.
 */
typedef void (*ArrayMapFn)(void *elem, void *clientData);

/* ArrayMapFn2
 * ----------_
 * Same as ArrayMapFn, but can return 0 to stop the mapping.
 * Used by ArrayMap2
 */
typedef int (*ArrayMapFn2)(void *elem, void *clientData);

 /* ArrayElementFreeFn
 * ------------------
 * ArrayElementFreeFn defines the space of functions that can be used as the
 * clean-up function for an element as it is deleted from the array
 * or when the entire array of elements is freed.  The cleanup function is 
 * called with a pointer to an element about to be deleted.
 */
typedef void (*ArrayElementFreeFn)(void *elem);


/* ArrayNew
 * --------
 * Creates a new DArray and returns it. There are zero elements in the array.
 * to start. The elemSize parameter specifies the number of bytes that a single 
 * element of this array should take up.  For example, if you want to store 
 * elements of type Binky, you would pass sizeof(Binky) as this parameter.
 * An assert is raised if the size is not greater than zero.
 *
 * The numElemsToAllocate parameter specifies the initial allocated length 
 * of the array, as well as the dynamic reallocation increment for when the 
 * array grows.  Rather than growing the array one element at a time as 
 * elements are added (which is rather inefficient), you will grow the array 
 * in chunks of numElemsToAllocate size.  The "allocated length" is the number
 * of elements that have been allocated, the "logical length" is the number of 
 * those slots actually being currently used.
 * 
 * A new array is initially allocated to the size of numElemsToAllocate, the
 * logical length is zero.  As elements are added, those allocated slots fill
 * up and when the initial allocation is all used, grow the array by another 
 * numElemsToAllocate elements. You will continue growing the array in chunks
 * like this as needed.  Thus the allocated length will always be a multiple
 * of numElemsToAllocate.  Don't worry about using realloc to shrink the array 
 * allocation if many elements are deleted from the array.  It turns out that 
 * many implementations of realloc don't even pay attention to such a request 
 * so there is little point in asking.  Just leave the array over-allocated.
 *
 * The numElemsToAllocate is the client's opportunity to tune the resizing
 * behavior for their particular needs.  If constructing large arrays, 
 * specifying a large allocation chunk size will result in fewer resizing
 * operations.  If using small arrays, a small allocation chunk size will 
 * result in less space going unused. If the client passes 0 for 
 * numElemsToAllocate, the implementation will use the default value of 8.
 *
 * The elemFreeFn is the function that will be called on an element that
 * is about to be deleted (using ArrayDeleteAt) or on each element in the
 * array when the entire array is being freed (using ArrayFree).  This function
 * is your chance to do any deallocation/cleanup required for the element
 * (such as freeing any pointers contained in the element). The client can pass 
 * NULL for the cleanupFn if the elements don't require any handling on free. 
 */
DArray ArrayNew(int elemSize, int numElemsToAllocate, 
                ArrayElementFreeFn elemFreeFn);



 /* ArrayFree
 * ----------
 * Frees up all the memory for the array and elements. It DOES NOT 
 * automatically free memory owned by pointers embedded in the elements. 
 * This would require knowledge of the structure of the elements which the 
 * DArray does not have. However, it will iterate over the elements calling
 * the elementFreeFn earlier supplied to ArrayNew and therefore, the client, 
 * who knows what the elements are, can do the appropriate deallocation of any 
 * embedded pointers through that function.  After calling this, the value of 
 * what array is pointing to is undefined.
 */
void ArrayFree(DArray array);


/* ArrayLength
 * -----------
 * Returns the logical length of the array, i.e. the number of elements
 * currently in the array.  Must run in constant time.
 */
int ArrayLength(const DArray array);


/* ArrayNth
 * --------
 * Returns a pointer to the element numbered n in the specified array.  
 * Numbering begins with 0.  An assert is raised if n is less than 0 or greater 
 * than the logical length minus 1. Note this function returns a pointer into 
 * the DArray's element storage, so the pointer should be used with care.
 * This function must operate in constant time.
 *
 * We could have written the DArray without this sort of access, but it
 * is useful and efficient to offer it, although the client needs to be 
 * careful when using it. In particular, a pointer returned by ArrayNth 
 * becomes invalid after any calls which involve insertion, deletion or 
 * sorting the array, as all of these may rearrange the element storage.
 */ 
void *ArrayNth(DArray array, int n);


/* ArrayAppend
 * -----------
 * Adds a new element to the end of the specified array.  The element is 
 * passed by address, the element contents are copied from the memory pointed 
 * to by newElem.  Note that right after this call, the new element will be 
 * the last in the array; i.e. its element number will be the logical length 
 * minus 1. This function must run in constant time (neglecting 
 * the memory reallocation time which may be required occasionally).
 */
void ArrayAppend(DArray array, const void *newElem);

/* ArrayInsertAt
 * -------------
 * Inserts a new element into the array, placing it at the position n.
 * An assert is raised if n is less than 0 or greater than the logical length.
 * The array elements after position n will be shifted over to make room. The 
 * element is passed by address, the new element's contents are copied from 
 * the memory pointed to by newElem. This function runs in linear time.
 */
void ArrayInsertAt(DArray array, const void *newElem, int n);

/* ArrayInsertSorted
 * -------------
 * Inserts a new element into the array, placing it at the position indicated by
 * a binary search of the array using comparator.
 * The array MUST be sorted prior to calling InsertSorted.
 * Note that if you only ever call InsertSorted, the array will always be sorted.
 */
void ArrayInsertSorted(DArray array, const void *newElem, ArrayCompareFn comparator);

 /* ArrayDeleteAt
 * -------------
 * Deletes the element numbered n from the array. Before being removed,
 * the elemFreeFn that was supplied to ArrayNew will be called on the element.
 * An assert is raised if n is less than 0 or greater than the logical length 
 * minus one. All the elements after position n will be shifted over to fill 
 * the gap.  This function runs in linear time. It does not shrink the 
 * allocated size of the array when an element is deleted, the array just 
 * stays over-allocated.
 */
void ArrayDeleteAt(DArray array, int n);

 /* ArrayDeleteAt
 * -------------
 * Removes the element numbered n from the array. The element will not be freed
 * before being removed. All the elements after position n will be shifted over to fill 
 * the gap.  This function runs in linear time. It does not shrink the 
 * allocated size of the array when an element is deleted, the array just 
 * stays over-allocated.
 */
void ArrayRemoveAt(DArray array, int n);

/* ArrayReplaceAt
 * -------------
 * Overwrites the element numbered n from the array with a new value. Before 
 * being overwritten, the elemFreeFn that was supplied to ArrayNew is called 
 * on the old element. Then that position in the array will get a new value by
 * copying the new element's contents from the memory pointed to by newElem.
 * An assert is raised if n is less than 0 or greater than the logical length 
 * minus one. None of the other elements are affected or rearranged by this
 * operation and the size of the array remains constant. This function must
 * operate in constant time.
 */
void ArrayReplaceAt(DArray array, const void *newElem, int n);


/* ArraySort
 * ---------
 * Sorts the specified array into ascending order according to the supplied
 * comparator.  The numbering of the elements will change to reflect the 
 * new ordering. An assert is raised if the comparator is NULL.
 */
void ArraySort(DArray array, ArrayCompareFn comparator);


#define NOT_FOUND -1	// returned when a search fails to find the key

/* ArraySearch
 * -----------
 * Searches the specified array for an element whose contents match
 * the element passed as the key.  Uses the comparator argument to test
 * for equality. The "fromIndex" parameter controls where the search
 * starts looking from. If the client desires to search the entire array,
 * they should pass 0 as the fromIndex. The function will search from
 * there to the end of the array. The "isSorted" parameter allows the client 
 * to specify that the array is already in sorted order, and thus it uses a 
 * faster binary search.  If isSorted is false, a simple linear search is 
 * used. If a match is found, the position of the matching element is returned
 * else the function returns NOT_FOUND.  Calling this function does not 
 * re-arrange or change contents of DArray or modify the key in any way.
 * An assert is raised if fromIndex is less than 0 or greater than
 * the logical length (although searching from logical length will never
 * find anything, allowing this case means you can search an entirely empty
 * array from 0 without getting an assert). An assert is raised if the
 * comparator is NULL.
 */
int ArraySearch(DArray array, const void *key, ArrayCompareFn comparator, 
                  int fromIndex, int isSorted);


/* ArrayMap
 * -----------
 * Iterates through each element in the array in order (from element 0 to
 * element n-1) and calls the function fn for that element.  The function is 
 * called with the address of the array element and the clientData pointer.  
 * The clientData value allows the client to pass extra state information to 
 * the client-supplied function, if necessary.  If no client data is required, 
 * this argument should be NULL. An assert is raised if map function is NULL.
 */
void ArrayMap(DArray array, ArrayMapFn fn, void *clientData);

/* ArrayMapBackwards
 * -----------
 * Same as ArrayMap, but goes through the array from end to front.  This
 * makes it safe to free elements during the mapping.
 */
void ArrayMapBackwards(DArray array, ArrayMapFn fn, void *clientData);

/* ArrayMap2
 * -----------
 * Same as ArrayMap, but allows the mapping to be stopped by returning 0
 * from the mapping function.  If the mapping was stopped, the element
 * it was stopped at will be returned.  If it wasn't stopped, then NULL
 * will be returned.
 */
void * ArrayMap2(DArray array, ArrayMapFn2 fn, void *clientData);

/* ArrayMapBackwards2
 * ------------
 * Goes through the array backwards, and allows you to stop the mapping.
 */
void * ArrayMapBackwards2(DArray array, ArrayMapFn2 fn, void *clientData);

/* ArrayClear
 * -----------
 * Deletes all elements in the array, but without freeing the array.
 */
void ArrayClear(DArray array);

/* ArrayGetDataPtr
 * -----------
 * Obtain the pointer to the actual data storage
 */
void *ArrayGetDataPtr(DArray array);

/* ArraySetDataPtr
 * -----------
 * Set the pointer to the actual data storage, which must be allocated with malloc
 */
void ArraySetDataPtr(DArray array, void *ptr, int count, int capacity);

#ifdef __cplusplus
}
#endif

#endif //_DARRAY_
