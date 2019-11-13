/*
 * Reader Writer locks test code.
 * All the contents of this file are overwritten during automated
 * testing. Please consider this before changing anything in this file.
 */

#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <kern/test161.h>
#include <spinlock.h>
#include <cdefs.h>

#define CREATELOOPS 8
#define NRWLOOPS 120
#define NTHREADS 32
#define SYNCHTEST_YIELDER_MAX 16

static volatile unsigned long testval1 = 3;
static volatile unsigned long testval2 = 5;
static volatile unsigned long testval3 = 7;
static volatile int32_t test_treading;
static volatile int32_t test_twriting;
static volatile int32_t test_tread;
static volatile int32_t test_twrite;

static struct rwlock *testrw = NULL;
static struct semaphore *donesem = NULL;

static struct spinlock status_lock;
static struct spinlock treading_lock;
static struct spinlock twriting_lock;
static struct spinlock tread_lock;
static struct spinlock twrite_lock;
static bool test_status = TEST161_FAIL;

static void increment_test_treading(void);
static void decrement_test_treading(void);
//static void increment_test_twriting(void);
//static void decrement_test_twriting(void);
static void increment_test_tread(void);
static void decrement_test_tread(void);
static void increment_test_twrite(void);
static void decrement_test_twrite(void);

static void rwtestreadthread(void *, unsigned long); 
static void rwtestwritethread(void *, unsigned long);

static 
void 
increment_test_treading(void) {
	spinlock_acquire(&treading_lock);
	test_treading++;
	spinlock_release(&treading_lock);
}

static 
void 
decrement_test_treading(void) {
	spinlock_acquire(&treading_lock);
	test_treading--;
	spinlock_release(&treading_lock);
}

//static 
//void 
//increment_test_twriting(void) {
//	spinlock_acquire(&twriting_lock);
//	test_twriting++;
//	spinlock_release(&twriting_lock);
//}
//
//static 
//void 
//decrement_test_twriting(void) {
//	spinlock_acquire(&twriting_lock);
//	test_twriting--;
//	spinlock_release(&twriting_lock);
//}

static 
void 
increment_test_tread(void) {
	spinlock_acquire(&tread_lock);
	test_tread++;
	spinlock_release(&tread_lock);
}

static 
void 
decrement_test_tread(void) {
	spinlock_acquire(&tread_lock);
	test_tread--;
	spinlock_release(&tread_lock);
}

static
void 
increment_test_twrite(void) {
	spinlock_acquire(&twrite_lock);
	test_twrite++;
	spinlock_release(&twrite_lock);
}

static 
void 
decrement_test_twrite(void) {
	spinlock_acquire(&twrite_lock);
	test_twrite--;
	spinlock_release(&twrite_lock);
}

static
bool
failif(bool condition) {
	if(condition) {
		spinlock_acquire(&status_lock);
		test_status = TEST161_FAIL;
		spinlock_release(&status_lock);
	}
	return condition; 
}

static
void
rwtestreadthread(void *junk, unsigned long num) {
	(void)junk;

	int i; 
	unsigned long val1, val2, val3;
	
	for (i = 0; i<NRWLOOPS; i++) {
		kprintf_t(".");
		rwlock_acquire_read(testrw);
		increment_test_treading();
		random_yielder(4);
		spinlock_acquire(&treading_lock);
		KASSERT(test_treading > 0);
		spinlock_release(&treading_lock);
		KASSERT(test_twriting == 0); 		// No need of spinlocks, as it is not suppossed to be a shared resource.

		val1 = testval1;
		val2 = testval2;
		val3 = testval3;

		random_yielder(4);
		spinlock_acquire(&treading_lock);
		KASSERT(test_treading > 0);
		spinlock_release(&treading_lock);
		KASSERT(test_twriting == 0); 		// No need of spinlocks, as it is not suppossed to be a shared resource.

		if (val1 != testval1) {
			goto fail;
		}
	
		random_yielder(4);
		spinlock_acquire(&treading_lock);
		KASSERT(test_treading > 0);
		spinlock_release(&treading_lock);
		KASSERT(test_twriting == 0); 		// No need of spinlocks, as it is not suppossed to be a shared resource.

		if (val2 != testval2) {
			goto fail;
		}

		random_yielder(4);
		spinlock_acquire(&treading_lock);
		KASSERT(test_treading > 0);
		spinlock_release(&treading_lock);
		KASSERT(test_twriting == 0); 		// No need of spinlocks, as it is not suppossed to be a shared resource.

		if (val3 != testval3) {
			goto fail;
		}

		random_yielder(4);
		spinlock_acquire(&treading_lock);
		KASSERT(test_treading > 0);
		spinlock_release(&treading_lock);
		KASSERT(test_twriting == 0); 		// No need of spinlocks, as it is not suppossed to be a shared resource.

		decrement_test_treading();
		decrement_test_tread();
		kprintf_n("reader thread: %lu\n", num);
		rwlock_release_read(testrw);
	}

	V(donesem);
	return;

fail:
	rwlock_release_read(testrw);
	failif(true);
	V(donesem);
	return;
}

static
void
rwtestwritethread(void *junk, unsigned long num) 
{
	(void)junk;
	
	int i;
	for (i=0; i<NRWLOOPS; i++) {
		kprintf_t(".");
		rwlock_acquire_write(testrw);			
		test_twriting++;
		random_yielder(4);
		KASSERT(test_treading == 0);
		KASSERT(test_twriting == 1);

		testval1 = num;
		testval2 = num*num;
		testval3 = num%3;

		if (testval2 != testval1*testval1) {
			goto fail;
		}

		random_yielder(4);
		KASSERT(test_treading == 0);
		KASSERT(test_twriting == 1);
		
		if (testval2 != testval1*testval1) {
			goto fail;
		}

		random_yielder(4);
		KASSERT(test_treading == 0);
		KASSERT(test_twriting == 1);

		if (testval2%3 != (testval3*testval3)%3) {
			goto fail;
		}

		random_yielder(4);
		KASSERT(test_treading == 0);
		KASSERT(test_twriting == 1);

		if (testval3 != testval1%3) {
			goto fail;
		}

		random_yielder(4);
		KASSERT(test_treading == 0);
		KASSERT(test_twriting == 1);

		if (testval1 != num) {
			goto fail;
		}

		random_yielder(4);
		KASSERT(test_treading == 0);
		KASSERT(test_twriting == 1);

		if (testval2 != num*num) {
			goto fail;
		}

		random_yielder(4);
		KASSERT(test_treading == 0);
		KASSERT(test_twriting == 1);

		if (testval3 != num%3) {
			goto fail;
		}

		random_yielder(4);
		KASSERT(test_treading == 0);
		KASSERT(test_twriting == 1);
		
		test_twriting--;
		decrement_test_twrite();
		kprintf_n("writer thread: %lu\n", num);
		rwlock_release_write(testrw);
	}

	V(donesem);
	return;

fail:
	rwlock_release_write(testrw);
	failif(true);
	V(donesem);
	return;
}

int rwtest(int nargs, char **args) {
	(void)nargs;
	(void)args;
		
	int i, result;
	kprintf_n("Starting rwt1...\n");
	for (i = 0; i<CREATELOOPS; i++) {
		kprintf_n(".");
		testrw = rwlock_create("testrw");	
		if (testrw == NULL) {
			panic("rwt1: rwlock_create failed\n");
		}
		donesem = sem_create("donesem", 0);
		if (donesem == NULL) {
			panic("rwt1: sem_create failed\n");
		}
		if (i != CREATELOOPS - 1) {
			rwlock_destroy(testrw);
			sem_destroy(donesem);
		}
	}
	spinlock_init(&status_lock);
	spinlock_init(&treading_lock);
	spinlock_init(&twriting_lock);
	spinlock_init(&tread_lock);
	spinlock_init(&twrite_lock);
	test_status = TEST161_SUCCESS;

	for (i = 0; i<NTHREADS; i++) {
		result = thread_fork("rwtest", NULL, rwtestreadthread, NULL, i);
		if (result) {
			panic("rwt1: thread_fork failed: %s\n", strerror(result));
		}
		increment_test_tread();
		result = thread_fork("rwtest", NULL, rwtestwritethread, NULL, i);
		if (result) {
			panic("rwt1: thread_fork failed: %s\n", strerror(result));
		}
		increment_test_twrite();
	}
	for (i = 0; i<(NTHREADS+NTHREADS); i++) { // NTHREADS reader + NTHREADS writer
		kprintf_n(".");
		P(donesem);
	}

	rwlock_destroy(testrw);
	sem_destroy(donesem);
	spinlock_cleanup(&status_lock);
	spinlock_cleanup(&treading_lock);
	spinlock_cleanup(&twriting_lock);
	spinlock_cleanup(&tread_lock);
	spinlock_cleanup(&twrite_lock);
	testrw = NULL;
	donesem = NULL;

	KASSERT(test_treading == 0 && test_twriting == 0);
	kprintf_n("\n");
	success(test_status, SECRET, "rwt1");

	return 0;
}

int rwtest2(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt2 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt2");

	return 0;
}

int rwtest3(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt3 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt3");

	return 0;
}

int rwtest4(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt4 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt4");

	return 0;
}

int rwtest5(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt5 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt5");

	return 0;
}
