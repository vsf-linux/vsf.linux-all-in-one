#include "cache.h"
#include "trace2/tr2_cmd_name.h"

#define TR2_ENVVAR_PARENT_NAME "GIT_TRACE2_PARENT_NAME"

#ifdef __VSF__
struct __git_trace2_cmd_name_ctx_t {
	struct strbuf __tr2cmdname_hierarchy;
	
};
static void __git_trace2_cmd_name_mod_init(void *ctx)
{
	struct __git_trace2_cmd_name_ctx_t *__git_trace2_cmd_name_ctx = ctx;
	__git_trace2_cmd_name_ctx->__tr2cmdname_hierarchy = STRBUF_INIT;
}
define_vsf_git_mod(git_trace2_cmd_name,
	sizeof(struct __git_trace2_cmd_name_ctx_t),
	GIT_MOD_TRACE2_CMD_NAME,
	__git_trace2_cmd_name_mod_init
)
#	define git_trace2_cmd_name_ctx	((struct __git_trace2_cmd_name_ctx_t *)vsf_git_ctx(git_trace2_cmd_name))

#	define tr2cmdname_hierarchy		(git_trace2_cmd_name_ctx->__tr2cmdname_hierarchy)
#else
static struct strbuf tr2cmdname_hierarchy = STRBUF_INIT;
#endif

void tr2_cmd_name_append_hierarchy(const char *name)
{
	const char *parent_name = getenv(TR2_ENVVAR_PARENT_NAME);

	strbuf_reset(&tr2cmdname_hierarchy);
	if (parent_name && *parent_name) {
		strbuf_addstr(&tr2cmdname_hierarchy, parent_name);
		strbuf_addch(&tr2cmdname_hierarchy, '/');
	}
	strbuf_addstr(&tr2cmdname_hierarchy, name);

	setenv(TR2_ENVVAR_PARENT_NAME, tr2cmdname_hierarchy.buf, 1);
}

const char *tr2_cmd_name_get_hierarchy(void)
{
	return tr2cmdname_hierarchy.buf;
}

void tr2_cmd_name_release(void)
{
	strbuf_release(&tr2cmdname_hierarchy);
}
