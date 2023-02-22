#define USE_THE_INDEX_COMPATIBILITY_MACROS
#include "builtin.h"
#include "cache.h"
#include "config.h"
#include "attr.h"
#include "quote.h"
#include "parse-options.h"

#ifdef __VSF__
struct __git_builtin_check_attr_ctx_t {
	int __all_attrs;
	int __cached_attrs;
	int __stdin_paths;
	int __nul_term_line;
	const struct option __check_attr_options[5];
};
static void __git_builtin_check_attr_mod_init(void *ctx);
define_vsf_git_mod(git_builtin_check_attr,
	sizeof(struct __git_builtin_check_attr_ctx_t),
	GIT_MOD_BUILTIN_CHECK_ATTR,
	__git_builtin_check_attr_mod_init
)
#	define git_builtin_check_attr_ctx	((struct __git_builtin_check_attr_ctx_t *)vsf_git_ctx(git_builtin_check_attr))
#	define all_attrs					(git_builtin_check_attr_ctx->__all_attrs)
#	define cached_attrs					(git_builtin_check_attr_ctx->__cached_attrs)
#	define stdin_paths					(git_builtin_check_attr_ctx->__stdin_paths)
#	define nul_term_line				(git_builtin_check_attr_ctx->__nul_term_line)
#	define check_attr_options			(git_builtin_check_attr_ctx->__check_attr_options)
#else
static int all_attrs;
static int cached_attrs;
static int stdin_paths;
#endif
static const char * const check_attr_usage[] = {
N_("git check-attr [-a | --all | <attr>...] [--] <pathname>..."),
N_("git check-attr --stdin [-z] [-a | --all | <attr>...]"),
NULL
};

#ifdef __VSF__
static void __git_builtin_check_attr_mod_init(void *ctx)
{
	struct __git_builtin_check_attr_ctx_t *__git_builtin_check_attr_ctx = ctx;
const struct option __check_attr_options[] = {
#else
static int nul_term_line;

static const struct option check_attr_options[] = {
#endif
	OPT_BOOL('a', "all", &all_attrs, N_("report all attributes set on file")),
	OPT_BOOL(0,  "cached", &cached_attrs, N_("use .gitattributes only from the index")),
	OPT_BOOL(0 , "stdin", &stdin_paths, N_("read file names from stdin")),
	OPT_BOOL('z', NULL, &nul_term_line,
		 N_("terminate input and output records by a NUL character")),
	OPT_END()
};
#ifdef __VSF__
	VSF_LINUX_ASSERT(dimof(__check_attr_options) <= dimof(__git_builtin_check_attr_ctx->__check_attr_options));
	memcpy(__git_builtin_check_attr_ctx->__check_attr_options, __check_attr_options, sizeof(__check_attr_options));
}
#endif

static void output_attr(struct attr_check *check, const char *file)
{
	int j;
	int cnt = check->nr;

	for (j = 0; j < cnt; j++) {
		const char *value = check->items[j].value;

		if (ATTR_TRUE(value))
			value = "set";
		else if (ATTR_FALSE(value))
			value = "unset";
		else if (ATTR_UNSET(value))
			value = "unspecified";

		if (nul_term_line) {
			printf("%s%c" /* path */
			       "%s%c" /* attrname */
			       "%s%c" /* attrvalue */,
			       file, 0,
			       git_attr_name(check->items[j].attr), 0, value, 0);
		} else {
			quote_c_style(file, NULL, stdout, 0);
			printf(": %s: %s\n",
			       git_attr_name(check->items[j].attr), value);
		}
	}
}

static void check_attr(const char *prefix,
		       struct attr_check *check,
		       int collect_all,
		       const char *file)
{
	char *full_path =
		prefix_path(prefix, prefix ? strlen(prefix) : 0, file);

	if (collect_all) {
		git_all_attrs(&the_index, full_path, check);
	} else {
		git_check_attr(&the_index, full_path, check);
	}
	output_attr(check, file);

	free(full_path);
}

static void check_attr_stdin_paths(const char *prefix,
				   struct attr_check *check,
				   int collect_all)
{
	struct strbuf buf = STRBUF_INIT;
	struct strbuf unquoted = STRBUF_INIT;
	strbuf_getline_fn getline_fn;

	getline_fn = nul_term_line ? strbuf_getline_nul : strbuf_getline_lf;
	while (getline_fn(&buf, stdin) != EOF) {
		if (!nul_term_line && buf.buf[0] == '"') {
			strbuf_reset(&unquoted);
			if (unquote_c_style(&unquoted, buf.buf, NULL))
				die("line is badly quoted");
			strbuf_swap(&buf, &unquoted);
		}
		check_attr(prefix, check, collect_all, buf.buf);
		maybe_flush_or_die(stdout, "attribute to stdout");
	}
	strbuf_release(&buf);
	strbuf_release(&unquoted);
}

static NORETURN void error_with_usage(const char *msg)
{
	error("%s", msg);
	usage_with_options(check_attr_usage, check_attr_options);
}

int cmd_check_attr(int argc, const char **argv, const char *prefix)
{
	struct attr_check *check;
	int cnt, i, doubledash, filei;

	if (!is_bare_repository())
		setup_work_tree();

	git_config(git_default_config, NULL);

	argc = parse_options(argc, argv, prefix, check_attr_options,
			     check_attr_usage, PARSE_OPT_KEEP_DASHDASH);

	if (read_cache() < 0) {
		die("invalid cache");
	}

	if (cached_attrs)
		git_attr_set_direction(GIT_ATTR_INDEX);

	doubledash = -1;
	for (i = 0; doubledash < 0 && i < argc; i++) {
		if (!strcmp(argv[i], "--"))
			doubledash = i;
	}

	/* Process --all and/or attribute arguments: */
	if (all_attrs) {
		if (doubledash >= 1)
			error_with_usage("Attributes and --all both specified");

		cnt = 0;
		filei = doubledash + 1;
	} else if (doubledash == 0) {
		error_with_usage("No attribute specified");
	} else if (doubledash < 0) {
		if (!argc)
			error_with_usage("No attribute specified");

		if (stdin_paths) {
			/* Treat all arguments as attribute names. */
			cnt = argc;
			filei = argc;
		} else {
			/* Treat exactly one argument as an attribute name. */
			cnt = 1;
			filei = 1;
		}
	} else {
		cnt = doubledash;
		filei = doubledash + 1;
	}

	/* Check file argument(s): */
	if (stdin_paths) {
		if (filei < argc)
			error_with_usage("Can't specify files with --stdin");
	} else {
		if (filei >= argc)
			error_with_usage("No file specified");
	}

	check = attr_check_alloc();
	if (!all_attrs) {
		for (i = 0; i < cnt; i++) {
			const struct git_attr *a = git_attr(argv[i]);

			if (!a)
				return error("%s: not a valid attribute name",
					     argv[i]);
			attr_check_append(check, a);
		}
	}

	if (stdin_paths)
		check_attr_stdin_paths(prefix, check, all_attrs);
	else {
		for (i = filei; i < argc; i++)
			check_attr(prefix, check, all_attrs, argv[i]);
		maybe_flush_or_die(stdout, "attribute to stdout");
	}

	attr_check_free(check);
	return 0;
}
