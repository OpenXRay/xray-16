 #ifndef _HASHTABLE_H
#define _HASHTABLE_H

/* File: hashtable.h
 * ------------------
 * Defines the interface for the HashTable ADT.
 * The HashTable allows the client to store any number of elements of any 
 * type in a hash table for fast storage and retrieval.  The client specifies 
 * the size (in bytes) of the elements that will be stored in the table when
 * it is created. Thereafter the client and the HashTable refer to elements 
 * via (void*) ptrs. The HashTable imposes no upper bound on the number of 
 * elements and deals with all its own memory management.
 *
 * The client-supplied information (in the form of the number of buckets 
 * to use and the hashing function to be applied to each element) is employed 
 * to divide elements in buckets with hopefully only few collisions, resulting 
 * in Enter & Lookup performance in constant-time.  The HashTable also supports 
 * iterating over all elements by use of mapping function.
 */
 
/* Type: HashTable
 * ----------------
 * Defines the HashTable type itself. The client can declare variables of type 
 * HashTable, but these variables must be initialized with the result of 
 * TableNew.  The HashTable is implemented with pointers, so all client 
 * copies in variables or parameters will be "shallow" -- they will all 
 * actually point to the same HashTable structure.  Only calls to TableNew
 * create new tables. The struct declaration below is "incomplete"- the 
 * implementation details are literally not visible in the client .h file.
 */
typedef struct HashImplementation *HashTable;


/* TableHashFn
 * -----------
 * TableHashFn is a pointer to a client-supplied function which the
 * HashTable uses to hash elements. The hash function takes a (const void*)
 * pointer to an element and the number of buckets and returns an int, 
 * which represents the hash code for this element.  The returned hash code 
 * should be within the range 0 to numBuckets-1 and should be stable (i.e. 
 * an element's hash code should not change over time).
 * For best performance, the hash function should be designed to 
 * uniformly distribute elements over the available number of buckets.
 */
typedef int (*TableHashFn)(const void *elem, int numBuckets);


/* TableCompareFn
 * --------------
 * TableCompareFn is a pointer to a client-supplied function which the
 * HashTable uses to compare elements. The comparator takes two 
 * (const void*) pointers (these will point to elements) and returns an int.
 * The comparator should indicate the ordering of the two elements
 * using the same convention as the strcmp library function:
 * If elem1 is "less than" elem2, return a negative number.
 * If elem1 is "greater than" elem2, return a positive number.
 * If the two elements are "equal", return 0.
 */
#if defined(WIN32)
// explicitly set __cdecl so that Win devs can change default calling convention
typedef int (__cdecl *TableCompareFn)(const void *elem1, const void *elem2);
#else
typedef int (*TableCompareFn)(const void *elem1, const void *elem2);
#endif

 /* TableMapFn
 * ----------
 * TableMapFn defines the space of functions that can be used to map over
 * the elements in a HashTable.  A map function is called with a pointer to
 * the element and a client data pointer passed in from the original caller.
 */
typedef void (*TableMapFn)(void *elem, void *clientData);

 /* TableMapFn2
 * ----------
 * Same as TableMapFn, but can return 0 to stop the mapping.
 * Used by TableMap2.
 */
typedef int (*TableMapFn2)(void *elem, void *clientData);


/* TableElementFreeFn
 * ------------------
 * TableElementFreeFn defines the space of functions that can be used as the
 * clean-up function for each element as it is deleted from the array
 * or when the entire array of elements is freed.  The cleanup function is 
 * called with a pointer to an element about to be deleted.
 */
typedef void (*TableElementFreeFn)(void *elem);

#ifdef __cplusplus
extern "C" {
#endif

/* TableNew
 * --------
 * Creates a new HashTable with no entries and returns it. The elemSize 
 * parameter specifies the number of bytes that a single element of the
 * table should take up. For example, if you want to store elements of type 
 * Binky, you would pass sizeof(Binky) as this parameter. An assert is
 * raised if this size is not greater than 0.
 *
 * The nBuckets parameter specifies the number of buckets that the elements
 * will be partitioned into.  Once a HashTable is created, this number does
 * not change.  The nBuckets parameter must be in synch with the behavior of
 * the hashFn, which must return a hash code between 0 and nBuckets-1.   
 * The hashFn parameter specifies the function that is called to retrieve the
 * hash code for a given element.  See the type declaration of TableHashFn
 * above for more information.  An assert is raised if nBuckets is not 
 * greater than 0.
 *
 * The compFn is used for testing equality between elements.  See the
 * type declaration for TableCompareFn above for more information.
 *
 * The elemFreeFn is the function that will be called on an element that is
 * about to be overwritten (by a new entry in TableEnter) or on each element 
 * in the table when the entire table is being freed (using TableFree).  This 
 * function is your chance to do any deallocation/cleanup required,
 * (such as freeing any pointers contained in the element). The client can pass 
 * NULL for the cleanupFn if the elements don't require any handling on free. 
 * An assert is raised if either the hash or compare functions are NULL.
 *
 * nChains is the number of chains to allocate initially in each bucket
 *
 */

HashTable TableNew(int elemSize, int nBuckets, 
                   TableHashFn hashFn, TableCompareFn compFn, 
 					 TableElementFreeFn freeFn);

HashTable TableNew2(int elemSize, int nBuckets, int nChains,
                   TableHashFn hashFn, TableCompareFn compFn, 
 					 TableElementFreeFn freeFn);


 /* TableFree
 * ----------
 * Frees up all the memory for the table and its elements. It DOES NOT 
 * automatically free memory owned by pointers embedded in the elements. This 
 * would require knowledge of the structure of the elements which the HashTable 
 * does not have.  However, it will iterate over the elements calling
 * the elementFreeFn earlier supplied to TableNew and therefore, the client, 
 * who knows what the elements are,can do the appropriate deallocation of any 
 * embedded pointers through that function.
 * After calling this, the value of what table points to is undefined.
 */
void TableFree(HashTable table);


/* TableCount
 * ----------
 * Returns the number of elements currently in the table.
 */
int TableCount(HashTable table);



/* TableEnter
 * ----------
 * Enters a new element into the table. Uses the hash function to determine
 * which bucket to place the new element. Its contents are copied from the
 * memory pointed to by newElem. If there is already an element in the table 
 * which is determined to be equal (using the comparison function) this will 
 * use the contents of the new element to replace the previous element, 
 * calling the free function on the replaced element.
 */
void TableEnter(HashTable table, const void *newElem);

/* TableRemove
 * ----------
 * Remove a element frin the table. If the element does not exist
 * the function returns 0. If it exists, it returns 1 and calls the
 * free function on the removed element.
 */
int TableRemove(HashTable table, const void *delElem);


/* TableLookup
 * ----------
 * Returns a pointer to the table element which matches the elemKey parameter
 * (equality is determined by the comparison function).  If there is no
 * matching element, returns NULL. Calling this function does not 
 * re-arrange or change contents of the table or modify elemKey in any way.
 */
void *TableLookup(HashTable table, const void *elemKey);



/* TableMap
 * -----------
 * Iterates through each element in the table (in any order) and calls the 
 * function fn for that element. The function is called with the address of 
 * the table element and the clientData pointer. The clientData value allows
 * the client to pass extra state information to the client-supplied function,
 * if necessary.  If no client data is required, this argument should be NULL.
 * An assert is raised if the map function is NULL.
 */
void TableMap(HashTable table, TableMapFn fn, void *clientData);

/* TableMapSafe
 * -----------
 * Same as TableMap, but allows elements to be freed during the mapping.
 */
void TableMapSafe(HashTable table, TableMapFn fn, void *clientData);

/* TableMap2
 * -----------
 * Same as TableMap, but allows the mapping to be stopped by returning 0
 * from the mapping function.  If the mapping was stopped, the element
 * it was stopped at will be returned.  If it wasn't stopped, then NULL
 * will be returned.
 */
void * TableMap2(HashTable table, TableMapFn2 fn, void *clientData);

/* TableMapSafe2
 * -----------
 * Same as TableMap2, but allows elements to be freed during the mapping.
 */
void * TableMapSafe2(HashTable table, TableMapFn2 fn, void *clientData);

/* TableClear
 * -----------
 * Clears all the elements in the table without freeing it
 */
void TableClear(HashTable table);

#ifdef __cplusplus
}
#endif

#endif //_HASHTABLE_H
