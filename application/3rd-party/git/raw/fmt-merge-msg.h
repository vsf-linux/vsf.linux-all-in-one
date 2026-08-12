#ifndef FMT_MERGE_MSG_H
#define FMT_MERGE_MSG_H

#include "strbuf.h"

#define DEFAULT_MERGE_LOG_LEN 20

struct fmt_merge_msg_opts {
	unsigned add_title:1,
		credit_people:1;
	int shortlog_len;
	const char *into_name;
};

#ifdef __VSF__
struct __git_environment_merge_public_ctx_t {
	int __merge_log_config;
};
declare_vsf_git_mod(git_environment_merge_public)
#	define git_environment_merge_public_ctx	((struct __git_environment_merge_public_ctx_t *)vsf_git_ctx(git_environment_merge_public))
#	define merge_log_config					(git_environment_merge_public_ctx->__merge_log_config)
#else
extern int merge_log_config;
#endif
int fmt_merge_msg_config(const char *key, const char *value, void *cb);
int fmt_merge_msg(struct strbuf *in, struct strbuf *out,
		  struct fmt_merge_msg_opts *);


#endif /* FMT_MERGE_MSG_H */
