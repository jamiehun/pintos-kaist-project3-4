/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* 익명페이지 서브시스템을 초기화 */
/* Initialize the data for anonymous pages */
void vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	swap_disk = NULL;
}

/* 이 함수는 처음을 page->operation에 있는 익명페이지에 대한 핸들러를 설정 */
/* 익명 페이지를 초기화하는데 사용됨 */
/* Initialize the file mapping */
bool anon_initializer (struct page *page, enum vm_type type, void *kva) {
	// 0) uninit page를 만들어주고 0으로 초기화 (simons')
	struct uninit_page *uninit = &page->uninit;
	memset(uninit, 0, sizeof(struct uninit_page));

	/* Set up the handler */
	// 1) page의 operation 설정
	page->operations = &anon_ops;

	// 2) anon_page를 page의 anon으로 설정 
	struct anon_page *anon_page = &page->anon;

	anon_page->test = 4;

	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
