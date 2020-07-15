#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define standardLength 10 // standard intial size of the vector's elements array

static void growVector(vector* v) {
	v->elems = realloc(v->elems, (v->allocLength + v->initialLength) * v->elemSize);
	assert(v->elems != NULL);
	v->allocLength = v->allocLength + v->initialLength;
}

void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation) {
	assert(elemSize > 0);
	assert(initialAllocation >= 0);
	if (initialAllocation == 0) {
		v->initialLength = standardLength;
		v->allocLength = standardLength;
	}
	else {
		v->initialLength = initialAllocation;
		v->allocLength = initialAllocation;
	}
	v->elems = malloc(elemSize * v->initialLength);
	assert(v->elems != NULL);
	v->elemSize = elemSize;
	v->logicalLength = 0;
	v->freeFn = freeFn;
}

void VectorDispose(vector *v) {
	// free each element of the elements array with the supplied free function if necessary
	if (v->freeFn != NULL) {
		for (size_t i = 0; i < v->allocLength; i++) {
			(*(v->freeFn))((char*)v->elems + i * v->elemSize);
		}
	}
	free(v->elems);
}

int VectorLength(const vector *v) { 
	return v->logicalLength;
}

void *VectorNth(const vector *v, int position) { 
	assert(position >= 0 && position < v->logicalLength);
	return (char*)v->elems + position * v->elemSize;
}

void VectorReplace(vector *v, const void *elemAddr, int position) {
	assert(position >= 0 && position < v->logicalLength);

	void* destination = (char*)v->elems + position * v->elemSize; // destination address of the element in the array

	if (v->freeFn != NULL) { // delete element
		(*(v->freeFn))(destination);
	}
	
	memcpy(destination, elemAddr, v->elemSize);
}

void VectorInsert(vector *v, const void *elemAddr, int position) {
	assert(position >= 0 && position <= v->logicalLength);
	// need more room for elements
	if (v->logicalLength == v->allocLength) {
		growVector(v);
	}
	
	void* destination = (char*)v->elems + position * v->elemSize; // destination address of the element in the array
	// shift elements in the array to the right in order to make room for the new element
	memcpy((char*)destination + v->elemSize, destination, (v->logicalLength - position) * v->elemSize); 
	memcpy((char*)v->elems + position * v->elemSize, elemAddr, v->elemSize);
	v->logicalLength++;
}

void VectorAppend(vector *v, const void *elemAddr) {	
	// need more room for elements
	if (v->logicalLength == v->allocLength) { 
		growVector(v);
	}
	
	memcpy((char*)v->elems + v->logicalLength * v->elemSize, elemAddr, v->elemSize);
	v->logicalLength++;
}

void VectorDelete(vector *v, int position) {
	assert(position >= 0 && position < v->logicalLength);

	void* elemAddress = (char*)v->elems + position * v->elemSize; // destination address of the element in the array

	if (v->freeFn != NULL) { // delete element
		(*(v->freeFn))(elemAddress);
	}
	// shift elements over to the left to fill in the gap
	memcpy(elemAddress, (char*)elemAddress + v->elemSize, (v->logicalLength - position) * v->elemSize);
	v->logicalLength--;
}

void VectorSort(vector *v, VectorCompareFunction compare) {
	assert(compare != NULL);
	qsort(v->elems, v->logicalLength, v->elemSize, compare);
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData) {
	assert(mapFn != NULL);

	for (int i = 0; i < v->logicalLength; i++) {
		mapFn((char*)v->elems + i * v->elemSize, auxData);
	}
}

static const int kNotFound = -1;

int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted) {
	assert(key != NULL && searchFn != NULL);
	assert(startIndex >= 0 && startIndex <= v->logicalLength);

	if (isSorted) { // binary search
		void* elemAddress = bsearch(key, v->elems, v->logicalLength, v->elemSize, searchFn);
		if (elemAddress != NULL) {
			return (elemAddress - v->elems) / v->elemSize;
		}
		else {
			return -1;
		}
	}
	else { // linear search
		for (int i = startIndex; i < v->logicalLength; i++) {
			if (searchFn(v->elems + i * v->elemSize, key) == 0) {
				return i;
			}
		}
	}
	return -1; // element not found
} 
