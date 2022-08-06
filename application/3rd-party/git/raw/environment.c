/*
 * We put all the git config variables in this same object
 * file, so that programs can link against the config parser
 * without having to link against all the rest of git.
 *
 * In particular, no need to bring in libz etc unless needed,
 * even if you might want to know where the git directory etc
 * are.
 */
#include "cache.h"
#include "branch.h"
#include "environment.h"
#include "repository.h"
#include "config.h"
#include "refs.h"
#include "fmt-merge-msg.h"
#include "commit.h"
#include "strvec.h"
#include "object-store.h"
#include "tmp-objdir.h"
#include "chdir-notify.h"
#include "shallow.h"

#ifdef __VSF__
static void __git_environment_public_mod_init(void *ctx);
define_vsf_git_mod(git_environment_public,
	sizeof(struct __git_environment_public_ctx_t),
	GIT_MOD_ENVIRONMENT_PUBLIC,
    __git_environment_public_mod_init
)

static void __git_environment_convert_public_mod_init(void *ctx);
define_vsf_git_mod(git_environment_convert_public,
	sizeof(struct __git_environment_convert_public_ctx_t),
	GIT_MOD_ENVIRONMENT_CONVERT_PUBLIC,
    __git_environment_convert_public_mod_init
)

static void __git_environment_branch_public_mod_init(void *ctx);
define_vsf_git_mod(git_environment_branch_public,
	sizeof(struct __git_environment_branch_public_ctx_t),
	GIT_MOD_ENVIRONMENT_BRANCH_PUBLIC,
    __git_environment_branch_public_mod_init
)

static void __git_environment_merge_public_mod_init(void *ctx);
define_vsf_git_mod(git_environment_merge_public,
	sizeof(struct __git_environment_merge_public_ctx_t),
	GIT_MOD_ENVIRONMENT_MERGE_PUBLIC,
    __git_environment_merge_public_mod_init
)

struct __git_environment_ctx_t {
	enum log_refs_config __log_all_ref_updates;	// = LOG_REFS_UNSET;

	char *__git_namespace;
	char *__super_prefix;
	int __git_work_tree_initialized;
	int __the_shared_repository;	// = PERM_UMASK;
	int __need_shared_repository_from_config;	// = 1;

	struct {
		int __initialized;
	} get_super_prefix;
	struct {
		int __cached_result;		// = -1;
	} print_sha1_ellipsis;
};
static void __git_environment_mod_init(void *ctx);
define_vsf_git_mod(git_environment,
	sizeof(struct __git_environment_ctx_t),
	GIT_MOD_ENVIRONMENT,
	__git_environment_mod_init
)
#	define git_environment_ctx		((struct __git_environment_ctx_t *)vsf_git_ctx(git_environment))
#	define repository_format_precious_objects	(git_environment_ctx->__repository_format_precious_objects)
#	define repository_format_worktree_config	(git_environment_ctx->__repository_format_worktree_config)
#	define object_creation_mode		(git_environment_ctx->__object_creation_mode)
#	define notes_ref_name			(git_environment_ctx->__notes_ref_name)
#	define grafts_replace_parents	(git_environment_ctx->__grafts_replace_parents)
#	define log_all_ref_updates		(git_environment_ctx->__log_all_ref_updates)
#	define git_namespace			(git_environment_ctx->__git_namespace)
#	define super_prefix				(git_environment_ctx->__super_prefix)
#else
int trust_executable_bit = 1;
int trust_ctime = 1;
int check_stat = 1;
int has_symlinks = 1;
int minimum_abbrev = 4, default_abbrev = -1;
int __ignore_case;
int assume_unchanged;
int prefer_symlink_refs;
int is_bare_repository_cfg = -1; /* unspecified */
int warn_ambiguous_refs = 1;
int warn_on_object_refname_ambiguity = 1;
int repository_format_precious_objects;
int repository_format_worktree_config;
const char *git_commit_encoding;
const char *git_log_output_encoding;
char *apply_default_whitespace;
char *apply_default_ignorewhitespace;
const char *git_attributes_file;
const char *git_hooks_path;
int zlib_compression_level = Z_BEST_SPEED;
int pack_compression_level = Z_DEFAULT_COMPRESSION;
int fsync_object_files;
int use_fsync = -1;
size_t packed_git_window_size = DEFAULT_PACKED_GIT_WINDOW_SIZE;
size_t packed_git_limit = DEFAULT_PACKED_GIT_LIMIT;
size_t delta_base_cache_limit = 96 * 1024 * 1024;
unsigned long big_file_threshold = 512 * 1024 * 1024;
int pager_use_color = 1;
const char *editor_program;
const char *askpass_program;
const char *excludes_file;
enum auto_crlf_t auto_crlf = AUTO_CRLF_FALSE;
int read_replace_refs = 1;
char *git_replace_ref_base;
enum eol core_eol = EOL_UNSET;
int global_conv_flags_eol = CONV_EOL_RNDTRP_WARN;
char *check_roundtrip_encoding = "SHIFT-JIS";
unsigned whitespace_rule_cfg = WS_DEFAULT_RULE;
enum branch_track git_branch_track = BRANCH_TRACK_REMOTE;
enum rebase_setup_type autorebase = AUTOREBASE_NEVER;
enum push_default_type push_default = PUSH_DEFAULT_UNSPECIFIED;
#endif
#ifndef OBJECT_CREATION_MODE
#define OBJECT_CREATION_MODE OBJECT_CREATION_USES_HARDLINKS
#endif
#ifndef __VSF__
enum object_creation_mode object_creation_mode = OBJECT_CREATION_MODE;
char *notes_ref_name;
int grafts_replace_parents = 1;
int core_apply_sparse_checkout;
int core_sparse_checkout_cone;
int merge_log_config = -1;
int precomposed_unicode = -1; /* see probe_utf8_pathname_composition() */
unsigned long pack_size_limit_cfg;
enum log_refs_config log_all_ref_updates = LOG_REFS_UNSET;
#endif

#ifndef PROTECT_HFS_DEFAULT
#define PROTECT_HFS_DEFAULT 0
#endif
#ifndef __VSF__
int protect_hfs = PROTECT_HFS_DEFAULT;
#endif

#ifndef PROTECT_NTFS_DEFAULT
#define PROTECT_NTFS_DEFAULT 1
#endif
#ifndef __VSF__
int protect_ntfs = PROTECT_NTFS_DEFAULT;
const char *core_fsmonitor;

/*
 * The character that begins a commented line in user-editable file
 * that is subject to stripspace.
 */
char comment_line_char = '#';
int auto_comment_line_char;

/* Parallel index stat data preload? */
int core_preload_index = 1;

/* This is set by setup_git_dir_gently() and/or git_default_config() */
char *git_work_tree_cfg;

static char *git_namespace;

static char *super_prefix;
#endif

#ifdef __VSF__
static void __git_environment_mod_init(void *ctx)
{
	struct __git_environment_ctx_t *__git_environment_ctx = ctx;
	__git_environment_ctx->__the_shared_repository = PERM_UMASK;
	__git_environment_ctx->__need_shared_repository_from_config = 1;
	__git_environment_ctx->print_sha1_ellipsis.__cached_result = -1;
}
static void __git_environment_public_mod_init(void *ctx)
{
    struct __git_environment_public_ctx_t *__git_environment_public_ctx = ctx;
    __git_environment_public_ctx->__trust_executable_bit = 1;
	__git_environment_public_ctx->__trust_ctime = 1;
	__git_environment_public_ctx->__check_stat = 1;
	__git_environment_public_ctx->__has_symlinks = 1;
	__git_environment_public_ctx->__minimum_abbrev = 4;
	__git_environment_public_ctx->__default_abbrev = -1;
	__git_environment_public_ctx->__warn_ambiguous_refs = 1;
	__git_environment_public_ctx->__warn_on_object_refname_ambiguity = 1;
	__git_environment_public_ctx->__zlib_compression_level = Z_BEST_SPEED;
	__git_environment_public_ctx->__pack_compression_level = Z_DEFAULT_COMPRESSION;
	__git_environment_public_ctx->__packed_git_window_size = DEFAULT_PACKED_GIT_WINDOW_SIZE;
	__git_environment_public_ctx->__packed_git_limit = DEFAULT_PACKED_GIT_LIMIT;
	__git_environment_public_ctx->__delta_base_cache_limit = 96 * 1024 * 1024;
	__git_environment_public_ctx->__big_file_threshold = 512 * 1024 * 1024;
	__git_environment_public_ctx->__use_fsync = -1;
	__git_environment_public_ctx->__read_replace_refs = 1;
	__git_environment_public_ctx->__protect_hfs = PROTECT_HFS_DEFAULT;
	__git_environment_public_ctx->__protect_ntfs = PROTECT_NTFS_DEFAULT;
	__git_environment_public_ctx->__precomposed_unicode = -1;
	__git_environment_public_ctx->__core_preload_index = 1;
	__git_environment_public_ctx->__comment_line_char = '#';
	__git_environment_public_ctx->__log_all_ref_updates = LOG_REFS_UNSET;
	__git_environment_public_ctx->__autorebase = AUTOREBASE_NEVER;
	__git_environment_public_ctx->____push_default = PUSH_DEFAULT_UNSPECIFIED;
	__git_environment_public_ctx->__object_creation_mode = OBJECT_CREATION_MODE;
	__git_environment_public_ctx->__grafts_replace_parents = 1;
	__git_environment_public_ctx->__is_bare_repository_cfg = -1;
	__git_environment_public_ctx->__pager_use_color = 1;
	__git_environment_public_ctx->__whitespace_rule_cfg = WS_DEFAULT_RULE;
}
static void __git_environment_convert_public_mod_init(void *ctx)
{
	struct __git_environment_convert_public_ctx_t *__git_environment_convert_public_ctx = ctx;
	__git_environment_convert_public_ctx->__auto_crlf = AUTO_CRLF_FALSE;
	__git_environment_convert_public_ctx->__core_eol = EOL_UNSET;
	__git_environment_convert_public_ctx->__global_conv_flags_eol = CONV_EOL_RNDTRP_WARN;
	__git_environment_convert_public_ctx->__check_roundtrip_encoding = "SHIFT-JIS";
}
static void __git_environment_branch_public_mod_init(void *ctx)
{
    struct __git_environment_branch_public_ctx_t *__git_environment_branch_public_ctx = ctx;
    __git_environment_branch_public_ctx->__git_branch_track = BRANCH_TRACK_REMOTE;
}
static void __git_environment_merge_public_mod_init(void *ctx)
{
    struct __git_environment_merge_public_ctx_t *__git_environment_merge_public_ctx = ctx;
    __git_environment_merge_public_ctx->__merge_log_config = -1;
}
#endif

/*
 * Repository-local GIT_* environment variables; see cache.h for details.
 */
const char * const local_repo_env[] = {
	ALTERNATE_DB_ENVIRONMENT,
	CONFIG_ENVIRONMENT,
	CONFIG_DATA_ENVIRONMENT,
	CONFIG_COUNT_ENVIRONMENT,
	DB_ENVIRONMENT,
	GIT_DIR_ENVIRONMENT,
	GIT_WORK_TREE_ENVIRONMENT,
	GIT_IMPLICIT_WORK_TREE_ENVIRONMENT,
	GRAFT_ENVIRONMENT,
	INDEX_ENVIRONMENT,
	NO_REPLACE_OBJECTS_ENVIRONMENT,
	GIT_REPLACE_REF_BASE_ENVIRONMENT,
	GIT_PREFIX_ENVIRONMENT,
	GIT_SUPER_PREFIX_ENVIRONMENT,
	GIT_SHALLOW_FILE_ENVIRONMENT,
	GIT_COMMON_DIR_ENVIRONMENT,
	NULL
};

static char *expand_namespace(const char *raw_namespace)
{
	struct strbuf buf = STRBUF_INIT;
	struct strbuf **components, **c;

	if (!raw_namespace || !*raw_namespace)
		return xstrdup("");

	strbuf_addstr(&buf, raw_namespace);
	components = strbuf_split(&buf, '/');
	strbuf_reset(&buf);
	for (c = components; *c; c++)
		if (strcmp((*c)->buf, "/") != 0)
			strbuf_addf(&buf, "refs/namespaces/%s", (*c)->buf);
	strbuf_list_free(components);
	if (check_refname_format(buf.buf, 0))
		die(_("bad git namespace path \"%s\""), raw_namespace);
	strbuf_addch(&buf, '/');
	return strbuf_detach(&buf, NULL);
}

const char *getenv_safe(struct strvec *argv, const char *name)
{
	const char *value = getenv(name);

	if (!value)
		return NULL;

	strvec_push(argv, value);
	return argv->v[argv->nr - 1];
}

void setup_git_env(const char *git_dir)
{
	const char *shallow_file;
	const char *replace_ref_base;
	struct set_gitdir_args args = { NULL };
	struct strvec to_free = STRVEC_INIT;

	args.commondir = getenv_safe(&to_free, GIT_COMMON_DIR_ENVIRONMENT);
	args.object_dir = getenv_safe(&to_free, DB_ENVIRONMENT);
	args.graft_file = getenv_safe(&to_free, GRAFT_ENVIRONMENT);
	args.index_file = getenv_safe(&to_free, INDEX_ENVIRONMENT);
	args.alternate_db = getenv_safe(&to_free, ALTERNATE_DB_ENVIRONMENT);
	if (getenv(GIT_QUARANTINE_ENVIRONMENT)) {
		args.disable_ref_updates = 1;
	}

	repo_set_gitdir(the_repository, git_dir, &args);
	strvec_clear(&to_free);

	if (getenv(NO_REPLACE_OBJECTS_ENVIRONMENT))
		read_replace_refs = 0;
	replace_ref_base = getenv(GIT_REPLACE_REF_BASE_ENVIRONMENT);
	free(git_replace_ref_base);
	git_replace_ref_base = xstrdup(replace_ref_base ? replace_ref_base
							  : "refs/replace/");
	free(git_namespace);
	git_namespace = expand_namespace(getenv(GIT_NAMESPACE_ENVIRONMENT));
	shallow_file = getenv(GIT_SHALLOW_FILE_ENVIRONMENT);
	if (shallow_file)
		set_alternate_shallow_file(the_repository, shallow_file, 0);
}

int is_bare_repository(void)
{
	/* if core.bare is not 'false', let's see if there is a work tree */
	return is_bare_repository_cfg && !get_git_work_tree();
}

int have_git_dir(void)
{
	return startup_info->have_repository
		|| the_repository->gitdir;
}

const char *get_git_dir(void)
{
	if (!the_repository->gitdir)
		BUG("git environment hasn't been setup");
	return the_repository->gitdir;
}

const char *get_git_common_dir(void)
{
	if (!the_repository->commondir)
		BUG("git environment hasn't been setup");
	return the_repository->commondir;
}

const char *get_git_namespace(void)
{
	if (!git_namespace)
		BUG("git environment hasn't been setup");
	return git_namespace;
}

const char *strip_namespace(const char *namespaced_ref)
{
	const char *out;
	if (skip_prefix(namespaced_ref, get_git_namespace(), &out))
		return out;
	return NULL;
}

const char *get_super_prefix(void)
{
#ifdef __VSF__
#	define initialized				(git_environment_ctx->get_super_prefix.__initialized)
#else
	static int initialized;
#endif
	if (!initialized) {
		super_prefix = xstrdup_or_null(getenv(GIT_SUPER_PREFIX_ENVIRONMENT));
		initialized = 1;
	}
	return super_prefix;
#ifdef __VSF__
#	undef initialized
#endif
}

#ifdef __VSF__
#	define git_work_tree_initialized	(git_environment_ctx->__git_work_tree_initialized)
#else
static int git_work_tree_initialized;
#endif

/*
 * Note.  This works only before you used a work tree.  This was added
 * primarily to support git-clone to work in a new repository it just
 * created, and is not meant to flip between different work trees.
 */
void set_git_work_tree(const char *new_work_tree)
{
	if (git_work_tree_initialized) {
		struct strbuf realpath = STRBUF_INIT;

		strbuf_realpath(&realpath, new_work_tree, 1);
		new_work_tree = realpath.buf;
		if (strcmp(new_work_tree, the_repository->worktree))
			die("internal error: work tree has already been set\n"
			    "Current worktree: %s\nNew worktree: %s",
			    the_repository->worktree, new_work_tree);
		strbuf_release(&realpath);
		return;
	}
	git_work_tree_initialized = 1;
	repo_set_worktree(the_repository, new_work_tree);
}

const char *get_git_work_tree(void)
{
	return the_repository->worktree;
}

char *get_object_directory(void)
{
	if (!the_repository->objects->odb)
		BUG("git environment hasn't been setup");
	return the_repository->objects->odb->path;
}

int odb_mkstemp(struct strbuf *temp_filename, const char *pattern)
{
	int fd;
	/*
	 * we let the umask do its job, don't try to be more
	 * restrictive except to remove write permission.
	 */
	int mode = 0444;
	git_path_buf(temp_filename, "objects/%s", pattern);
	fd = git_mkstemp_mode(temp_filename->buf, mode);
	if (0 <= fd)
		return fd;

	/* slow path */
	/* some mkstemp implementations erase temp_filename on failure */
	git_path_buf(temp_filename, "objects/%s", pattern);
	safe_create_leading_directories(temp_filename->buf);
	return xmkstemp_mode(temp_filename->buf, mode);
}

int odb_pack_keep(const char *name)
{
	int fd;

	fd = open(name, O_RDWR|O_CREAT|O_EXCL, 0600);
	if (0 <= fd)
		return fd;

	/* slow path */
	safe_create_leading_directories_const(name);
	return open(name, O_RDWR|O_CREAT|O_EXCL, 0600);
}

char *get_index_file(void)
{
	if (!the_repository->index_file)
		BUG("git environment hasn't been setup");
	return the_repository->index_file;
}

char *get_graft_file(struct repository *r)
{
	if (!r->graft_file)
		BUG("git environment hasn't been setup");
	return r->graft_file;
}

static void set_git_dir_1(const char *path)
{
	xsetenv(GIT_DIR_ENVIRONMENT, path, 1);
	setup_git_env(path);
}

static void update_relative_gitdir(const char *name,
				   const char *old_cwd,
				   const char *new_cwd,
				   void *data)
{
	char *path = reparent_relative_path(old_cwd, new_cwd, get_git_dir());
	struct tmp_objdir *tmp_objdir = tmp_objdir_unapply_primary_odb();

	trace_printf_key(&trace_setup_key,
			 "setup: move $GIT_DIR to '%s'",
			 path);
	set_git_dir_1(path);
	if (tmp_objdir)
		tmp_objdir_reapply_primary_odb(tmp_objdir, old_cwd, new_cwd);
	free(path);
}

void set_git_dir(const char *path, int make_realpath)
{
	struct strbuf realpath = STRBUF_INIT;

	if (make_realpath) {
		strbuf_realpath(&realpath, path, 1);
		path = realpath.buf;
	}

	set_git_dir_1(path);
	if (!is_absolute_path(path))
		chdir_notify_register(NULL, update_relative_gitdir, NULL);

	strbuf_release(&realpath);
}

const char *get_log_output_encoding(void)
{
	return git_log_output_encoding ? git_log_output_encoding
		: get_commit_output_encoding();
}

const char *get_commit_output_encoding(void)
{
	return git_commit_encoding ? git_commit_encoding : "UTF-8";
}

#ifdef __VSF__
#	define the_shared_repository	(git_environment_ctx->__the_shared_repository)
#	define need_shared_repository_from_config	(git_environment_ctx->__need_shared_repository_from_config)
#else
static int the_shared_repository = PERM_UMASK;
static int need_shared_repository_from_config = 1;
#endif

void set_shared_repository(int value)
{
	the_shared_repository = value;
	need_shared_repository_from_config = 0;
}

int get_shared_repository(void)
{
	if (need_shared_repository_from_config) {
		const char *var = "core.sharedrepository";
		const char *value;
		if (!git_config_get_value(var, &value))
			the_shared_repository = git_config_perm(var, value);
		need_shared_repository_from_config = 0;
	}
	return the_shared_repository;
}

void reset_shared_repository(void)
{
	need_shared_repository_from_config = 1;
}

int use_optional_locks(void)
{
	return git_env_bool(GIT_OPTIONAL_LOCKS_ENVIRONMENT, 1);
}

int print_sha1_ellipsis(void)
{
	/*
	 * Determine if the calling environment contains the variable
	 * GIT_PRINT_SHA1_ELLIPSIS set to "yes".
	 */
#ifdef __VSF__
#	define cached_result			(git_environment_ctx->print_sha1_ellipsis.__cached_result)
#else
	static int cached_result = -1; /* unknown */
#endif

	if (cached_result < 0) {
		const char *v = getenv("GIT_PRINT_SHA1_ELLIPSIS");
		cached_result = (v && !strcasecmp(v, "yes"));
	}
	return cached_result;
}
