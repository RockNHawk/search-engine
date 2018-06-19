#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include <assert.h>

#include "postlist.h"

/* buffer setup/free macro */
#define SETUP_BUFFER(_po) \
	if (_po->buf == NULL) { \
		_po->buf = malloc(_po->buf_sz); \
		_po->tot_sz += _po->buf_sz; \
	} do {} while (0)

#define FREE_BUFFER(_po) \
	if (_po->buf) { \
		free(_po->buf); \
		_po->buf = NULL; \
		_po->tot_sz -= _po->buf_sz; \
	} do {} while (0)

void postlist_print_info(struct postlist *po)
{
	printf("==== memory posting list info ====\n");
	printf("%u blocks (%.2f KB).\n", po->n_blk,
	       (float)po->tot_sz / 1024.f);
	printf("item size: %u\n", po->item_sz);
	printf("buffer size: %u\n", po->buf_sz);

	skippy_print(&po->skippy);
}

struct postlist *
postlist_create(uint32_t skippy_spans, uint32_t buf_sz, uint32_t item_sz,
                void *buf_arg, struct postlist_callbks calls)
{
	struct postlist *ret;
	ret = malloc(sizeof(struct postlist));

	/* assign initial values */
	ret->head = ret->tail = NULL;
	ret->tot_sz = sizeof(struct postlist);
	ret->n_blk  = 0;
	skippy_init(&ret->skippy, skippy_spans);

	ret->buf = NULL;
	ret->buf_sz = buf_sz;
	ret->buf_end = 0;
	ret->buf_arg = buf_arg;

	ret->on_flush    = calls.on_flush;
	ret->on_rebuf    = calls.on_rebuf;
	ret->on_free     = calls.on_free;

	/* leave iterator-related initializations to postlist_start() */
	ret->item_sz = item_sz;

	return ret;
}

static struct postlist_node *create_node(uint32_t key, size_t size)
{
	struct postlist_node *ret;
	ret = malloc(sizeof(struct postlist_node));

	/* assign initial values */
	skippy_node_init(&ret->sn, key);
	ret->blk = malloc(size);
	ret->blk_sz = size;

	return ret;
}

static void
append_node(struct postlist *po, struct postlist_node *node)
{
	if (po->head == NULL)
		po->head = node;

	po->tail = node;
	po->tot_sz += node->blk_sz;
	po->n_blk ++;

#ifdef DEBUG_POSTLIST
	printf("appending skippy node...\n");
#endif
	skippy_append(&po->skippy, &node->sn);
}

static uint32_t postlist_flush(struct postlist *po)
{
	uint32_t flush_key, flush_sz;
	struct postlist_node *node;

	/* invoke flush callback and get flush size */
	flush_key = po->on_flush(po->buf, &po->buf_end, po->buf_arg);
	flush_sz = po->buf_end;

	/* append new node with copy of current buffer */
	node = create_node(flush_key, flush_sz);
	append_node(po, node);

	/* flush to new tail node */
	memcpy(node->blk, po->buf, flush_sz);

	/* reset buffer */
	po->buf_end = 0;

	return flush_sz;
}

size_t
postlist_write(struct postlist *po, const void *in, size_t size)
{
	size_t flush_sz = 0;

	/* setup buffer for writting */
	SETUP_BUFFER(po);

	/* flush buffer under inefficient buffer space */
	if (po->buf_end + size > po->buf_sz)
		flush_sz = postlist_flush(po);

	/* write into buffer */
	assert(po->buf_end + size <= po->buf_sz);
	memcpy(po->buf + po->buf_end, in, size);
	po->buf_end += size;

#ifdef DEBUG_POSTLIST
	printf("buffer after writting: [%u/%u]\n",
	       po->buf_end, po->buf_sz);
#endif

	return flush_sz;
}

size_t postlist_write_complete(struct postlist *po)
{
	size_t flush_sz;

	if (po->buf)
		flush_sz = postlist_flush(po);

	FREE_BUFFER(po);
	return flush_sz;
}

void postlist_free(struct postlist *po)
{
	po->on_free(po->buf_arg);
	skippy_free(&po->skippy, struct postlist_node, sn,
	            free(p->blk); free(p));
	free(po->buf);
	free(po);
}

static void forward_cur(struct postlist_node **cur)
{
	struct skippy_node *next = (*cur)->sn.next[0];

	/* forward one step */
	*cur = MEMBER_2_STRUCT(next, struct postlist_node, sn);
}

static void rebuf_cur(struct postlist *po)
{
	struct postlist_node *cur = po->cur;

	if (cur) {
		/* refill buffer */
		memcpy(po->buf, cur->blk, cur->blk_sz);
		po->buf_end = cur->blk_sz;

		/* invoke rebuf callback */
		po->on_rebuf(po->buf, &po->buf_end, po->buf_arg);

	} else {
		/* reset buffer variables anyway */
		po->buf_end = 0;
	}

	po->buf_idx = 0;
}

bool postlist_start(void *po_)
{
	struct postlist *po = (struct postlist*)po_;

	if (po->head == NULL)
		return 0;

	/* setup buffer for rebuffering */
	SETUP_BUFFER(po);

	/* initial buffer filling */
	po->cur = po->head;
	rebuf_cur(po);

	return (po->buf_end != 0);
}

bool postlist_next(void *po_)
{
	struct postlist *po = (struct postlist*)po_;
	po->buf_idx += po->item_sz;

	do {
		if (po->buf_idx < po->buf_end) {
			return 1;
		} else {
			forward_cur(&po->cur);
			rebuf_cur(po);
		}

	} while (po->buf_end != 0);

	return 0;
}

void* postlist_cur_item(void *po_)
{
	struct postlist *po = (struct postlist*)po_;
	return po->buf + po->buf_idx;
}

uint64_t postlist_cur_item_id(void *item)
{
	uint32_t *curID = (uint32_t*)item;
	return (uint64_t)(*curID);
}

bool postlist_jump(void *po_, uint64_t target_)
{
	struct postlist *po = (struct postlist*)po_;
	uint32_t            target = (uint32_t)target_;
	struct skippy_node *jump_to;
	uint32_t           *curID;

	/* using skip-list */
	jump_to = skippy_node_jump(&po->cur->sn, target);

	if (jump_to != &po->cur->sn) {
		/* if we can jump over some blocks */
#ifdef DEBUG_POSTLIST
		printf("jump to a different node.\n");
#endif
		po->cur = MEMBER_2_STRUCT(jump_to, struct postlist_node, sn);
		rebuf_cur(po);
	}
#ifdef DEBUG_POSTLIST
	else
		printf("stay in the same node.\n");
#endif

	/* at this point, there shouldn't be any problem to access
	 * current posting item:
	 * case 1: if had jumped to a new block, the buffer must
	 * be non-empty.
	 * case 2: if we stay in the old block, it is guaranteed
	 * that current posting item is a valid pointer. */

	/* seek in the current block until we get to an ID greater or
	 * equal to target ID. */
	do {
		/* docID must be the first member of structure */
		curID = (uint32_t*)postlist_cur_item(po);

		if (*curID >= target) {
			return 1;
		}

	} while (postlist_next(po));

	return 0;
}

void postlist_finish(void *po_)
{
	struct postlist *po = (struct postlist*)po_;
	FREE_BUFFER(po);
}
