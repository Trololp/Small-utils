// Measure sorting performance
//

#include "stdafx.h"
#include <cstdlib>
#include <algorithm>
//#include <Windows.h>

#pragma warning( disable: 4996 )

//Defines
typedef unsigned int UINT;
typedef unsigned long long int ULLINT;
typedef int(*__compar_d_fn_t) (const void *, const void *, void *);

//Macro
#define swap_uint(a, b) {UINT tmp = (a) ; (a) = (b); (b) = tmp;}
//Structs

//Globals


UINT* gimme_random_array(UINT count)
{
	UINT* mem = (UINT*)malloc(count * sizeof(UINT));

	for (int i = 0; i < count; i++)
	{
		mem[i] = rand();
	}
	return mem;
}

UINT* gimme_inverse_array(UINT count)
{
	UINT* mem = (UINT*)malloc(count * sizeof(UINT));

	for (int i = 0; i < count; i++)
	{
		mem[i] = count - i;
	}
	return mem;
}

UINT* gimme_sorted_array(UINT count)
{
	UINT* mem = (UINT*)malloc(count * sizeof(UINT));

	for (int i = 0; i < count; i++)
	{
		mem[i] = count;
	}
	return mem;
}

void randomize_array(UINT* arr, UINT count)
{
	for (int i = 0; i < count; i++)
	{
		arr[i] = rand();
	}
}

void read_cycles(ULLINT* cycles)
{
	UINT tmp_v1, tmp_v2;
	__asm {
		xor eax, eax
		cpuid
		rdtsc
		mov tmp_v1, eax
		mov tmp_v2, edx
	}

	*cycles = tmp_v2;
	*cycles <<= 32;
	*cycles += tmp_v1;
}

void measure_load(void(__stdcall *test_fn)(UINT* arr, UINT count))
{
	// 1024 2048 4096 8192 16384 etc... elements

	UINT start_element_count = 1024;

	for (int stage = 0; stage < 6; stage++)
	{
		UINT* rand_array = gimme_random_array(start_element_count);

		ULLINT cycles = 0;

		// 16 times
		for (int i = 0; i < 16; i++)
		{
			randomize_array(rand_array, start_element_count);

			ULLINT before_cycles;
			read_cycles(&before_cycles);

			// Load
			test_fn(rand_array, start_element_count);


			ULLINT after_cycles;
			read_cycles(&after_cycles);

			cycles += (after_cycles - before_cycles);
		}

		cycles /= 16;

		printf("%d (%d) : %llu \n", stage + 1, start_element_count, cycles > 300 ? cycles - 300 : 0);

		free(rand_array);

		start_element_count *= 2;
	}

}

bool test_sorted_arr(UINT* arr, UINT count)
{
	bool result = true;

	for (int i = 0; i < count - 1; i++)
	{
		if (arr[i] > arr[i + 1])
			result = false;
	}
	return result;
}

void test_sort_fn(void(__stdcall *test_fn)(UINT* arr, UINT count))
{
	UINT test_arr[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	UINT test_arr2[10] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
	UINT test_arr3[10] = { 7, 1, 6, 3, 8, 2, 4, 5, 9, 0 };

	UINT* final_test_arr = gimme_random_array(1024);

	printf("before: ");
	printf("[");
	for (int i = 0; i < 10; i++)
	{
		printf("%d, ", test_arr[i]);
	}
	printf("]\n");

	test_fn(test_arr, 10);

	printf("after: ");
	printf("[");
	for (int i = 0; i < 10; i++)
	{
		printf("%d, ", test_arr[i]);
	}
	printf("]\n");


	printf("before: ");
	printf("[");
	for (int i = 0; i < 10; i++)
	{
		printf("%d, ", test_arr2[i]);
	}
	printf("]\n");

	test_fn(test_arr2, 10);

	printf("after: ");
	printf("[");
	for (int i = 0; i < 10; i++)
	{
		printf("%d, ", test_arr2[i]);
	}
	printf("]\n");

	printf("before: ");
	printf("[");
	for (int i = 0; i < 10; i++)
	{
		printf("%d, ", test_arr3[i]);
	}
	printf("]\n");

	test_fn(test_arr3, 10);

	printf("after: ");
	printf("[");
	for (int i = 0; i < 10; i++)
	{
		printf("%d, ", test_arr3[i]);
	}
	printf("]\n");

	test_fn(final_test_arr, 1024);

	printf("final test %s \n", test_sorted_arr(final_test_arr, 1024) ? "passed" : "failed");

	free(final_test_arr);
	return;
}


//test functions
void __stdcall empty_fn(UINT* arr, UINT count)
{
	return;
}

void __stdcall just_add_1_fn(UINT* arr, UINT count)
{
	for (int i = 0; i < count; i++)
	{
		arr[i]++;
	}
}

//sorting algorithms (rule arr[i-1] <= arr[i])
void __stdcall bubble_sort(UINT* arr, UINT count)
{
	if (!arr || count < 2)
		return;

	int i = 0;

	while (true)
	{
		for (int j = i; j < count; j++)
		{
			if (arr[i] <= arr[j])
				continue;
			else
			{
				swap_uint(arr[i], arr[j])
			}
		}

		if (i == count - 1)
			break; // sorted
		else
			i++;
	}
}

// compare sorting function
void compare_sort_fn(UINT* s, UINT* e)
{
	UINT* S = s;
	UINT* E = e;

	if (s >= e)
		return;

	while (e > s)
	{
		if (*s > *e)
			swap_uint(*s, *e)

			s++;
		e--;
	}

	compare_sort_fn(S, e);
	compare_sort_fn(s, E);
}

void __stdcall compare_sort(UINT* arr, UINT count)
{
	if (!arr || count < 2)
		return;

	int tmp = (int)log2(count);

	for (int i = 0; i < tmp; i++)
	{
		compare_sort_fn(arr, arr + count - 1);
	}
}

// C standart library quicksort
int qsort_comp(const int *i, const int *j)
{
	return *i - *j;
}

void __stdcall cstdlib_qsort(UINT* arr, UINT count)
{
	qsort(arr, count, sizeof(UINT), (int(*) (const void *, const void *)) qsort_comp);
}

//from GeeksforGeeks article about quicksort
// it also stack overflows when get inversed array with many items

/* This function takes last element as pivot, places
the pivot element at its correct position in sorted
array, and places all smaller (smaller than pivot)
to left of pivot and all greater elements to right
of pivot */
int partition(UINT* arr, int low, int high)
{
	int pivot = arr[high]; // pivot
	int i = (low - 1); // Index of smaller element and indicates the right position of pivot found so far

	for (int j = low; j <= high - 1; j++)
	{
		// If current element is smaller than the pivot
		if (arr[j] < pivot)
		{
			i++; // increment index of smaller element
			swap_uint(arr[i], arr[j]);
		}
	}
	swap_uint(arr[i + 1], arr[high]);
	return (i + 1);
}

/* The main function that implements QuickSort
arr[] --> Array to be sorted,
low --> Starting index,
high --> Ending index */
void quickSort(UINT* arr, int low, int high)
{
	if (low < high)
	{
		/* pi is partitioning index, arr[p] is now
		at right place */
		int pi = partition(arr, low, high);

		// Separately sort elements before
		// partition and after partition
		quickSort(arr, low, pi - 1);
		quickSort(arr, pi + 1, high);
	}
}

void __stdcall quicksort_fn(UINT* arr, UINT count)
{
	quickSort(arr, 0, count - 1);
}

// From glibc source code

#define SWAP(a, b, size)						      \
  do									      \
    {									      \
      size_t __size = (size);						      \
      char *__a = (a), *__b = (b);					      \
      do								      \
	{								      \
	  char __tmp = *__a;						      \
	  *__a++ = *__b;						      \
	  *__b++ = __tmp;						      \
	} while (--__size > 0);						      \
    } while (0)

/* Discontinue quicksort algorithm when partition gets below this size.
This particular magic number was chosen to work best on a Sun 4/260. */
#define MAX_THRESH 4

/* Stack node declarations used to store unfulfilled partition obligations. */
typedef struct
{
	char *lo;
	char *hi;
} stack_node;

/* The next 4 #defines implement a very fast in-line stack abstraction. */
/* The stack needs log (total_elements) entries (we could even subtract
log(MAX_THRESH)).  Since total_elements has type size_t, we get as
upper bound for log (total_elements):
bits per byte (CHAR_BIT) * sizeof(size_t).  */
#define STACK_SIZE	(CHAR_BIT * sizeof (size_t))
#define PUSH(low, high)	((void) ((top->lo = (low)), (top->hi = (high)), ++top))
#define	POP(low, high)	((void) (--top, (low = top->lo), (high = top->hi)))
#define	STACK_NOT_EMPTY	(stack < top)

void
glibc_quicksort(void *const pbase, size_t total_elems, size_t size,
	__compar_d_fn_t cmp, void *arg)
{
	char *base_ptr = (char *)pbase;

	const size_t max_thresh = MAX_THRESH * size;

	if (total_elems == 0)
		/* Avoid lossage with unsigned arithmetic below.  */
		return;

	if (total_elems > MAX_THRESH)
	{
		char *lo = base_ptr;
		char *hi = &lo[size * (total_elems - 1)];
		stack_node stack[STACK_SIZE];
		stack_node *top = stack;

		PUSH(NULL, NULL);

		while (STACK_NOT_EMPTY)
		{
			char *left_ptr;
			char *right_ptr;

			/* Select median value from among LO, MID, and HI. Rearrange
			LO and HI so the three values are sorted. This lowers the
			probability of picking a pathological pivot value and
			skips a comparison for both the LEFT_PTR and RIGHT_PTR in
			the while loops. */

			char *mid = lo + size * ((hi - lo) / size >> 1);

			if ((*cmp) ((void *)mid, (void *)lo, arg) < 0)
				SWAP(mid, lo, size);
			if ((*cmp) ((void *)hi, (void *)mid, arg) < 0)
				SWAP(mid, hi, size);
			else
				goto jump_over;
			if ((*cmp) ((void *)mid, (void *)lo, arg) < 0)
				SWAP(mid, lo, size);
		jump_over:;

			left_ptr = lo + size;
			right_ptr = hi - size;

			/* Here's the famous ``collapse the walls'' section of quicksort.
			Gotta like those tight inner loops!  They are the main reason
			that this algorithm runs much faster than others. */
			do
			{
				while ((*cmp) ((void *)left_ptr, (void *)mid, arg) < 0)
					left_ptr += size;

				while ((*cmp) ((void *)mid, (void *)right_ptr, arg) < 0)
					right_ptr -= size;

				if (left_ptr < right_ptr)
				{
					SWAP(left_ptr, right_ptr, size);
					if (mid == left_ptr)
						mid = right_ptr;
					else if (mid == right_ptr)
						mid = left_ptr;
					left_ptr += size;
					right_ptr -= size;
				}
				else if (left_ptr == right_ptr)
				{
					left_ptr += size;
					right_ptr -= size;
					break;
				}
			} while (left_ptr <= right_ptr);

			/* Set up pointers for next iteration.  First determine whether
			left and right partitions are below the threshold size.  If so,
			ignore one or both.  Otherwise, push the larger partition's
			bounds on the stack and continue sorting the smaller one. */

			if ((size_t)(right_ptr - lo) <= max_thresh)
			{
				if ((size_t)(hi - left_ptr) <= max_thresh)
					/* Ignore both small partitions. */
					POP(lo, hi);
				else
					/* Ignore small left partition. */
					lo = left_ptr;
			}
			else if ((size_t)(hi - left_ptr) <= max_thresh)
				/* Ignore small right partition. */
				hi = right_ptr;
			else if ((right_ptr - lo) > (hi - left_ptr))
			{
				/* Push larger left partition indices. */
				PUSH(lo, right_ptr);
				lo = left_ptr;
			}
			else
			{
				/* Push larger right partition indices. */
				PUSH(left_ptr, hi);
				hi = right_ptr;
			}
		}
	}

	/* Once the BASE_PTR array is partially sorted by quicksort the rest
	is completely sorted using insertion sort, since this is efficient
	for partitions below MAX_THRESH size. BASE_PTR points to the beginning
	of the array to sort, and END_PTR points at the very last element in
	the array (*not* one beyond it!). */

#define min(x, y) ((x) < (y) ? (x) : (y))

	{
		char *const end_ptr = &base_ptr[size * (total_elems - 1)];
		char *tmp_ptr = base_ptr;
		char *thresh = min(end_ptr, base_ptr + max_thresh);
		char *run_ptr;

		/* Find smallest element in first threshold and place it at the
		array's beginning.  This is the smallest array element,
		and the operation speeds up insertion sort's inner loop. */

		for (run_ptr = tmp_ptr + size; run_ptr <= thresh; run_ptr += size)
			if ((*cmp) ((void *)run_ptr, (void *)tmp_ptr, arg) < 0)
				tmp_ptr = run_ptr;

		if (tmp_ptr != base_ptr)
			SWAP(tmp_ptr, base_ptr, size);

		/* Insertion sort, running from left-hand-side up to right-hand-side.  */

		run_ptr = base_ptr + size;
		while ((run_ptr += size) <= end_ptr)
		{
			tmp_ptr = run_ptr - size;
			while ((*cmp) ((void *)run_ptr, (void *)tmp_ptr, arg) < 0)
				tmp_ptr -= size;

			tmp_ptr += size;
			if (tmp_ptr != run_ptr)
			{
				char *trav;

				trav = run_ptr + size;
				while (--trav >= run_ptr)
				{
					char c = *trav;
					char *hi, *lo;

					for (hi = lo = trav; (lo -= size) >= tmp_ptr; hi = lo)
						*hi = *lo;
					*hi = c;
				}
			}
		}
	}
}

int glibc_quicksort_cmp_func(const int *i, const int *j, void *arg)
{
	return (*i - *j);
}

void __stdcall glibc_quicksort_fn(UINT* arr, UINT count)
{
	glibc_quicksort(arr, count, sizeof(UINT), (__compar_d_fn_t)glibc_quicksort_cmp_func, nullptr);
}

void __stdcall Cpp_sort_fn(UINT* arr, UINT count)
{
	std::sort(arr, arr + count);
}


// merge arr[] <- sub_arr1[p..q], sub_arr2[q+1..r]
void Merge(int* A, unsigned int p, unsigned int q, unsigned int r, int* tmp)
{
	unsigned int sz1 = q - p + 1;
	unsigned int sz2 = r - q;

	for (int i = 0; i < sz1; i++)
		tmp[i] = A[p + i];
	for (int i = 0; i < sz2; i++)
		tmp[i+sz1] = A[q + 1 + i];


	int i = 0;
	int j = 0;
	int k = p;

	while (i < sz1 && j < sz2)
	{
		A[k] = (tmp[i] <= tmp[j + sz1]) ? tmp[i] : tmp[j + sz1];
		int _i = i;
		i += (tmp[i] <= tmp[j + sz1]);
		j += (tmp[_i] > tmp[j + sz1]);
		k++;
	}

	while (i < sz1)
	{
		A[k] = tmp[i];
		i++;
		k++;
	}

	while (j < sz2)
	{
		A[k] = tmp[j+sz1];
		j++;
		k++;
	}

}

void Merge_Sort(int *A, unsigned int p, unsigned int r, int* tmp)
{

	if (p < r)
	{
		unsigned int q = (p + r) / 2;// no need rounding integers in C

		Merge_Sort(A, p, q, tmp);
		Merge_Sort(A, q + 1, r, tmp);
		Merge(A, p, q, r, tmp);
	}
}

void __stdcall merge_sort_fn(UINT* arr, UINT count)
{
	UINT* tmp_arr = (UINT*)malloc(count*sizeof(UINT));

	Merge_Sort((int*)arr, 0, count - 1, (int*)tmp_arr);
}

// test this also
void Insertion_sort(UINT* arr, UINT count)
{
	for (int j = 2; j < count; j++)
	{
		UINT key = arr[j];
		int i = j - 1;

		while ((i > 0) && (arr[i] > key))
		{
			arr[i + 1] = arr[i];
			i--;
			              // here
		}
		arr[i + 1] = key; // was 
	}
}

int main(int argc, char** argv)
{
	bool run_tests = 0;

	if (argc == 2)
	{
		run_tests = true;
	}




	// Setup random
	UINT tmp_v1;
	__asm {rdtsc
	mov tmp_v1, eax
	}
	srand(tmp_v1);

	if (!run_tests)
		test_sort_fn(merge_sort_fn);

	//printf("empty_fn\n");
	//mesure_load(empty_fn);

	//printf("just_add_1_fn\n");
	//mesure_load(just_add_1_fn);

	if (!run_tests)
		system("PAUSE");

	if (!run_tests)
		return 0;

	//printf("bubble_sort\n");
	//measure_load(bubble_sort);

	printf("compare_sort\n");
	measure_load(compare_sort);

	printf("cstdlib_qsort\n");
	measure_load(cstdlib_qsort);

	printf("quicksort_fn\n");
	measure_load(quicksort_fn);

	printf("glibc_quicksort_fn\n");
	measure_load(glibc_quicksort_fn);

	printf("Cpp_sort_fn\n");
	measure_load(Cpp_sort_fn);

	printf("merge_sort_fn\n");
	measure_load(merge_sort_fn);


	return 0;
}

