#pragma once
#include <stdint.h>

struct math_prefix_loc {
	uint32_t qr, dr;
};

struct math_prefix_cell { /* 24 bytes */
	uint64_t qmask;
	uint64_t dmask;
	uint32_t cnt;
	uint16_t qmiss;
	uint16_t dmiss;
};

struct math_prefix_qry {
	uint32_t  n, n_dirty;
	struct math_prefix_loc *dirty;
	struct math_prefix_cell **cell;
};

struct math_prefix_qry pq_allocate(uint32_t);

void pq_print(struct math_prefix_qry, uint32_t);

void pq_free(struct math_prefix_qry);

void pq_reset(struct math_prefix_qry*);

uint64_t pq_hit(struct math_prefix_qry*,
                uint32_t, uint32_t,
                uint32_t, uint32_t);

struct pq_align_res {
	uint64_t qmask;
	uint64_t dmask;
	uint32_t r_cnt;
};

struct pq_align_res
pq_align_v1(struct math_prefix_qry*, uint32_t*,
            struct math_prefix_loc*, uint32_t); 

struct pq_align_res
pq_align_v2(struct math_prefix_qry*, uint32_t*,
            struct math_prefix_loc*, uint32_t); 

struct pq_align_res
pq_align_v3(struct math_prefix_qry*, uint32_t*,
            struct math_prefix_loc*, uint32_t); 

void pq_print_dirty_array(struct math_prefix_qry*);

int pq_mask_is_clean(struct math_prefix_qry*);
