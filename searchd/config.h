/* for MAX_TXT_SEG_BYTES & MAX_TXT_SEG_LEN */
#include "txt-seg/config.h"
#define MAX_TERM_BYTES MAX_TXT_SEG_BYTES

/* consider both math & term */
#define MAX_QUERY_BYTES     (MAX_TXT_SEG_BYTES * 32)
#define MAX_QUERY_WSTR_LEN  (MAX_TXT_SEG_LEN * 32)

#define MAX_MERGE_POSTINGS 4096
#define SNIPPET_PADDING 40

//#define DEBUG_POST_MERGE

/* max mark score, type of mnc_score_t */
#define MNC_MARK_SCORE 99

/*
#define MNC_DEBUG
#define MNC_SMALL_BITMAP
*/

//#define DEBUG_MATH_SEARCH

#define RANK_SET_DEFAULT_VOL 45
#define DEFAULT_RES_PER_PAGE 10

//#define DEBUG_PROXIMITY
#define ENABLE_PROXIMITY_SCORE

#define MAX_HIGHLIGHT_OCCURS 8

#define DEBUG_MATH_SCORE_POSTING
//#define VERBOSE_SEARCH
