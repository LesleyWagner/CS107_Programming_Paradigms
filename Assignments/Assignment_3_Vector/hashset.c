#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn) {
	assert(hashfn != NULL && comparefn != NULL);
	assert(elemSize > 0 && numBuckets > 0);
	
	h->elemSize = elemSize;
	h->numBuckets = numBuckets;
	h->hashFn = hashfn;
	h->compareFn = comparefn;
	h->freeFn = freefn;
	h->buckets = malloc(numBuckets * sizeof(vector));
	// initialize buckets of elements with allocation of 1
	for (int i = 0; i < numBuckets; i++) {
		VectorNew((char*)h->buckets + i * sizeof(vector), elemSize, freefn, 1);
	}
	assert(h->buckets != NULL);
}

void HashSetDispose(hashset *h) {
	// free each bucket and free the elements inside the bucket
	for (int i = 0; i < h->numBuckets; i++) { 
		VectorDispose((char*)h->buckets + i * sizeof(vector));
	}
	free(h->buckets);
}

int HashSetCount(const hashset *h) { 
	int count = 0;
	for (int i = 0; i < h->numBuckets; i++) {
		count += VectorLength((char*)h->buckets + i * sizeof(vector));
	}
	return count;
}

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData) {
	for (int i = 0; i < h->numBuckets; i++) {
		VectorMap((char*)h->buckets + i * sizeof(vector), mapfn, auxData);
	}
}

void HashSetEnter(hashset *h, const void *elemAddr) {
	assert(elemAddr != NULL);
	int hashcode = (*h->hashFn)(elemAddr, h->numBuckets);
	assert(hashcode >= 0 && hashcode < h->numBuckets);

	vector* bucket = &h->buckets[hashcode];
	int position = VectorSearch(bucket, elemAddr, h->compareFn, 0, false);

	if (position == -1) {
		VectorAppend(bucket, elemAddr);
	}
	else {
		VectorReplace(bucket, elemAddr, position);
	}
}

void* HashSetLookup(const hashset* h, const void* elemAddr) {
	assert(elemAddr != NULL);
	int hashcode = (*h->hashFn)(elemAddr, h->numBuckets);
	assert(hashcode >= 0 && hashcode < h->numBuckets);

	vector* bucket = &h->buckets[hashcode];
	int position = VectorSearch(bucket, elemAddr, h->compareFn, 0, false);

	if (position != -1) {
		return VectorNth(bucket, position);
	}
	else {
		return NULL;
	}
}
