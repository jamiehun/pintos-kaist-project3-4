/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "lib/kernel/hash.h"


/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		/* TODO: Insert the page into the spt. */
	}
err:
	return false;
}


/* spt에서 가상주소 va와 대응되는 페이지 구조체를 찾아서 반환함 
   Find VA from spt and return page. On error, return NULL. */
struct page *spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {

	/* TODO: Fill this function. */

	/* 1) 새로 페이지를 할당받아서 페이지에 대한 va를 기존 va로 받음 
	      대신, va를 저장할 때는 pg_round_down을 통해 시작점을 기준으로 설정 => va에 대한 페이지 준비 */
	struct page *temp_page = (struct page *)malloc(sizeof(struct page)); // ??? page를 새로 할당받을 생각
	temp_page->va = pg_round_down(va);


	/* 2) spt 테이블에서 위에서 만든 hash_elem을 찾음(page 구조체에 hash_elem 추가)
	      hash_table의 hash_elem이 곧 page이므로 저장해도 됨 */
	struct hash_elem *temp_hash_elem = hash_find(&spt->table, &temp_page->hash_elem);

	if (temp_hash_elem == NULL)
		return NULL;

    /* 3) temp_page의 경우 다 썼기 때문에 free하여 할당 해제함 */
	free(temp_page);

    /* 4) hash_elem != hash_entry를 통해 나온 hash_elem 
		  hash_entry에서 나온 내용은 hash_elem, 곧 page라고 할 수 있음 */
	return hash_entry(temp_hash_elem, struct page, hash_elem);
}

/*  주어진 보조테이블에 페이지 구조체를 삽입 (+ 존재하는지 검사)
	Insert PAGE into spt with validation. */
bool spt_insert_page (struct supplemental_page_table *spt UNUSED, struct page *page UNUSED) {
	
	/* TODO: Fill this function. */
	
	/* !!! 원본과 다름 !!!*/
	/* === 원본 === */
	// temp_elem 기준으로 판단
	// if(temp_elem == NULL)
	//	return true;
	
	/* === 나의 방식 === */
	// 구현이유 : Gitbook 기준 
	// This function should checks that the virtual address does not exist 
	// in the given supplemental page table.
	
	struct page *temp_page = spt_find_page(spt, page->va);

	if(temp == NULL){
		struct hash_elem *temp_elem = hash_insert(&spt->table, &page->hash_elem);
		return true;
	}

	return false;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */

	return swap_in (page, frame->kva);
}

/* 보조테이블을 초기화하는 함수 (해시테이블 자료 구조로 구현)
   새로운 프로세스 시작하거나 do_fork로 자식 프로세스 생성될 때 위 함수 호출*/
/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->table, spt_hash, spt_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}

/********** Project 3: Virtual Memory **********/
/* hash_hash_func에 대한 결과값
   Computes and returns the hash value for hash element E, given
   auxiliary data AUX. */
uint64_t spt_hash(const struct hash_elem *e, void *aux){
	const struct page *temp_page = hash_entry(elem, struct page, hash_elem);
	return hash_bytes(temp_page->va, sizeof(temp_page->va)); // temp_page->va 크기의 주소를 해시값으로 반환
}

/* hash_less_func에 대한 반환 값
   Compares the value of two hash elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
bool spt_less(const struct hash_elem *a, const struct hash_elem *b, void *aux){
	const struct page *page_a = hash_entry(a, struct page, hash_elem);
	const struct page *page_b = hash_entry(b, struct page, hash_elem);

	return ((page_a->va < page_b->va) ? true : false);
}