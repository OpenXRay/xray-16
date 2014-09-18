/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: In-place "new" operator.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#ifndef _NEW_

/**
 * @brief In-place "new" operator.
 * @param size - size of allocated memory block.
 * @param ptr - pointer to pre-allocated memory block.
 * @return pointer to allocated memory block.
 */
inline void* operator new(size_t size, void* ptr)
{
	size;
	return ptr;
}

/**
 * @brief Matching in-place "delete" operator.
 * @param ptr1 - pointer to pre-allocated memory block.
 * @param ptr2 - pointer to pre-allocated memory block.
 */
inline void operator delete(void* ptr1, void* ptr2)
{
	ptr1; ptr2;
}

#endif
