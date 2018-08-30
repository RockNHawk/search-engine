#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "head.h"

struct dir_merge_args {
	math_index_t          index;
	dir_merge_callbk      fun;
	void                 *args;

	/* subpath set */
	uint32_t              set_sz;
	struct subpath_ele  **eles;

	/* dir-merge path strings */
	uint32_t       longpath;
	char           (*base_paths)[MAX_DIR_PATH_NAME_LEN];
	char           (*full_paths)[MAX_DIR_PATH_NAME_LEN];
};

/*
 * functions below are for debug purpose.
 */
static LIST_IT_CALLBK(dele_pathid)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(path_id, uint32_t, pa_extra);

	if (sp->path_id == *path_id) {
		list_detach_one(pa_now->now, pa_head, pa_now, pa_fwd);
		free(sp);
		return LIST_RET_BREAK;
	}

	LIST_GO_OVER;
}

static void
del_subpath_of_path_id(struct subpaths *subpaths, uint32_t path_id)
{
	list_foreach(&subpaths->li, &dele_pathid, &path_id);
}

static void
print_all_dir_strings(struct dir_merge_args *dm_args)
{
	uint32_t i, j;
	struct subpath_ele *ele;

	for (i = 0; i < dm_args->set_sz; i++) {
		ele = dm_args->eles[i];
		printf("[%u] %s ", i, dm_args->full_paths[i]);

		printf("(duplicates: ");
		for (j = 0; j <= ele->dup_cnt; j++)
			printf("%u~path#%u ", ele->rid[j], ele->dup[j]->path_id);
		printf(")\n");
	}
}

/*
 * directory search callback function.
 */
static enum ds_ret
dir_search_callbk(const char* path, const char *srchpath,
                  uint32_t level, void *arg)
{
	uint32_t i;
	enum ds_ret ret = DS_RET_CONTINUE;
	P_CAST(dm_args, struct dir_merge_args, arg);

	for (i = 0; i < dm_args->set_sz; i++) {
		// printf("[%s]\n", srchpath);
		if (srchpath[1] != '\0') {
		sprintf(dm_args->full_paths[i], "%s/%s",
		        dm_args->full_paths[i], srchpath);
		sprintf(dm_args->base_paths[i], "%s/%s",
		        dm_args->base_paths[i], srchpath + 2);
		}
	}

#ifdef DEBUG_DIR_MERGE
	printf("post merging at directories:\n");
	print_all_dir_strings(dm_args);
	printf("\n");
#endif
	if (DIR_MERGE_RET_STOP == dm_args->fun(
			dm_args->full_paths,
			dm_args->base_paths,
			dm_args->eles,
			dm_args->set_sz,
			level, dm_args->args)
	   ) {
		ret = DS_RET_STOP_ALLDIR;
	}

#ifdef DEBUG_DIR_MERGE
	printf("ret = %d.\n", ret);
#endif
	return ret;
}

/*
 * dir-merge initialization related functions.
 */
list
dir_merge_subpath_set(enum dir_merge_pathset_type pathset_type,
	            struct subpaths *subpaths, int *n_uniq_paths)
{
	list subpath_set = LIST_NULL;
	struct subpath_ele_added added;
	
	switch (pathset_type) {
	case DIR_PATHSET_PREFIX_PATH:
		added = prefix_subpath_set_from_subpaths(subpaths, &subpath_set);
		break;
	case DIR_PATHSET_LEAFROOT_PATH:
	default:
		added = lr_subpath_set_from_subpaths(subpaths, &subpath_set);
	}

	*n_uniq_paths = added.new_uniq;

	return subpath_set;
}

struct assoc_ele_and_pathstr_args {
	uint32_t                     i;
	math_index_t                 index;
	uint32_t                     longpath;
	size_t                       max_strlen;
	char                         (*base_paths)[MAX_DIR_PATH_NAME_LEN];
	char                         (*full_paths)[MAX_DIR_PATH_NAME_LEN];
	struct subpath_ele           **eles;
	enum dir_merge_pathset_type  pathset_type;
};

static LIST_IT_CALLBK(assoc_ele_and_pathstr)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(args, struct assoc_ele_and_pathstr_args, pa_extra);
	char *append = args->full_paths[args->i];
	char *append_base = args->base_paths[args->i];
	size_t len;

	/* initialize */
	append += sprintf(args->full_paths[args->i], "%s/", args->index->dir);
	append_base += sprintf(args->base_paths[args->i], "./");

	/* make path string */
	if (args->pathset_type == DIR_PATHSET_PREFIX_PATH) {
		math_index_mk_prefix_path_str(ele->dup[0], ele->prefix_len, append_base);

	    if (math_index_mk_prefix_path_str(ele->dup[0], ele->prefix_len, append)) {
			args->i = 0; /* indicates error */
			return LIST_RET_BREAK;
		}
	} else {
		math_index_mk_path_str(ele->dup[0], append_base);

		if (math_index_mk_path_str(ele->dup[0], append)) {
			args->i = 0; /* indicates error */
			return LIST_RET_BREAK;
		}
	}

	/* associate paths[i] with a subpath set element */
	args->eles[args->i] = ele;

	/* compare and update max string length */
	len = strlen(args->base_paths[args->i]);
	if (len > args->max_strlen) {
		args->max_strlen = len;
		args->longpath = args->i;
	}

	args->i ++;
	LIST_GO_OVER;
}

/*
 * math_index_dir_merge() main function.
 */
#define NEW_SUBPATHS_DIR_BUF(_n_paths) \
	malloc(_n_paths * MAX_DIR_PATH_NAME_LEN);

int math_index_dir_merge(math_index_t index,
	enum dir_merge_type dir_merge_type,
	enum dir_merge_pathset_type pathset_type,
	list subpath_set, int n, dir_merge_callbk fun, void *args)
{
	int ret = 0;
	struct assoc_ele_and_pathstr_args assoc_args;

	struct dir_merge_args dm_args = {index, fun, args, 0 /* set size */,
	                                 NULL /* set elements */,
	                                 0 /* longpath index */,
	                                 NULL /* base paths */,
	                                 NULL /* full paths */};

	/* allocate unique subpath string buffers */
	dm_args.set_sz = n;
	dm_args.eles = malloc(sizeof(struct subpath_ele*) * n);
	dm_args.longpath = 0 /* assign real number later */;
	dm_args.base_paths = NEW_SUBPATHS_DIR_BUF(n);
	dm_args.full_paths = NEW_SUBPATHS_DIR_BUF(n);

	/* make unique subpath strings and return longest path index (longpath) */
	assoc_args.i = 0;
	assoc_args.index = index;
	assoc_args.longpath = 0;
	assoc_args.max_strlen = 0;
	assoc_args.full_paths = dm_args.full_paths;
	assoc_args.base_paths = dm_args.base_paths;
	assoc_args.eles = dm_args.eles;
	assoc_args.pathset_type = pathset_type;

	list_foreach(&subpath_set, &assoc_ele_and_pathstr, &assoc_args);

	if (assoc_args.i == 0) {
		/* last function fails for some reason */
		fprintf(stderr, "path strings are not fully generated.\n");
		ret = 1;
		goto exit;
	}

#ifdef DEBUG_DIR_MERGE
	{
		int i;
		printf("base path strings:\n");
		for (i = 0; i < n; i++)
			printf("path string [%u]: %s\n", i, dm_args.full_paths[i]);
	}
#endif

	/* now we get longpath index, we can pass it to dir-merge */
	dm_args.longpath = assoc_args.longpath;

#ifdef DEBUG_DIR_MERGE
	printf("longest path : [%u] %s (length=%lu)\n", dm_args.longpath,
	       dm_args.full_paths[dm_args.longpath], assoc_args.max_strlen);
	printf("\n");

	printf("subpath set (size=%u):\n", dm_args.set_sz);
	subpath_set_print(&subpath_set, stdout);
	printf("\n");
#endif

	/* now we can start merge path directories */
	if (dir_merge_type == DIR_MERGE_BREADTH_FIRST) {
		dir_search_bfs(dm_args.full_paths[dm_args.longpath],
		               &dir_search_callbk, &dm_args);
	} else if (dir_merge_type == DIR_MERGE_DEPTH_FIRST) {
		dir_search_podfs(dm_args.full_paths[dm_args.longpath],
		               &dir_search_callbk, &dm_args);
	} else if (dir_merge_type == DIR_MERGE_DIRECT) {

		dir_search_callbk(".", ".", 0, &dm_args);
	} else {
		fprintf(stderr, "DIR_MERGE type %u not implemented.\n", dir_merge_type);
		abort();
	}

exit:
	free(dm_args.eles);
	free(dm_args.base_paths);
	free(dm_args.full_paths);

	return ret;
}
