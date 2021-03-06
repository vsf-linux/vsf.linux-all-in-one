#include "git-compat-util.h"
#include "cache.h"
#include "config.h"
#include "branch.h"
#include "refs.h"
#include "refspec.h"
#include "remote.h"
#include "sequencer.h"
#include "commit.h"
#include "worktree.h"

struct tracking {
	struct refspec_item spec;
	struct string_list *srcs;
	const char *remote;
	int matches;
};

static int find_tracked_branch(struct remote *remote, void *priv)
{
	struct tracking *tracking = priv;

	if (!remote_find_tracking(remote, &tracking->spec)) {
		if (++tracking->matches == 1) {
			string_list_append(tracking->srcs, tracking->spec.src);
			tracking->remote = remote->name;
		} else {
			free(tracking->spec.src);
			string_list_clear(tracking->srcs, 0);
		}
		tracking->spec.src = NULL;
	}

	return 0;
}

static int should_setup_rebase(const char *origin)
{
	switch (autorebase) {
	case AUTOREBASE_NEVER:
		return 0;
	case AUTOREBASE_LOCAL:
		return origin == NULL;
	case AUTOREBASE_REMOTE:
		return origin != NULL;
	case AUTOREBASE_ALWAYS:
		return 1;
	}
	return 0;
}

/**
 * Install upstream tracking configuration for a branch; specifically, add
 * `branch.<name>.remote` and `branch.<name>.merge` entries.
 *
 * `flag` contains integer flags for options; currently only
 * BRANCH_CONFIG_VERBOSE is checked.
 *
 * `local` is the name of the branch whose configuration we're installing.
 *
 * `origin` is the name of the remote owning the upstream branches. NULL means
 * the upstream branches are local to this repo.
 *
 * `remotes` is a list of refs that are upstream of local
 */
static int install_branch_config_multiple_remotes(int flag, const char *local,
		const char *origin, struct string_list *remotes)
{
	const char *shortname = NULL;
	struct strbuf key = STRBUF_INIT;
	struct string_list_item *item;
	int rebasing = should_setup_rebase(origin);

	if (!remotes->nr)
		BUG("must provide at least one remote for branch config");
	if (rebasing && remotes->nr > 1)
		die(_("cannot inherit upstream tracking configuration of "
		      "multiple refs when rebasing is requested"));

	/*
	 * If the new branch is trying to track itself, something has gone
	 * wrong. Warn the user and don't proceed any further.
	 */
	if (!origin)
		for_each_string_list_item(item, remotes)
			if (skip_prefix(item->string, "refs/heads/", &shortname)
			    && !strcmp(local, shortname)) {
				warning(_("not setting branch '%s' as its own upstream"),
					local);
				return 0;
			}

	strbuf_addf(&key, "branch.%s.remote", local);
	if (git_config_set_gently(key.buf, origin ? origin : ".") < 0)
		goto out_err;

	strbuf_reset(&key);
	strbuf_addf(&key, "branch.%s.merge", local);
	/*
	 * We want to overwrite any existing config with all the branches in
	 * "remotes". Override any existing config, then write our branches. If
	 * more than one is provided, use CONFIG_REGEX_NONE to preserve what
	 * we've written so far.
	 */
	if (git_config_set_gently(key.buf, NULL) < 0)
		goto out_err;
	for_each_string_list_item(item, remotes)
		if (git_config_set_multivar_gently(key.buf, item->string, CONFIG_REGEX_NONE, 0) < 0)
			goto out_err;

	if (rebasing) {
		strbuf_reset(&key);
		strbuf_addf(&key, "branch.%s.rebase", local);
		if (git_config_set_gently(key.buf, "true") < 0)
			goto out_err;
	}
	strbuf_release(&key);

	if (flag & BRANCH_CONFIG_VERBOSE) {
		struct strbuf tmp_ref_name = STRBUF_INIT;
		struct string_list friendly_ref_names = STRING_LIST_INIT_DUP;

		for_each_string_list_item(item, remotes) {
			shortname = item->string;
			skip_prefix(shortname, "refs/heads/", &shortname);
			if (origin) {
				strbuf_addf(&tmp_ref_name, "%s/%s",
					    origin, shortname);
				string_list_append_nodup(
					&friendly_ref_names,
					strbuf_detach(&tmp_ref_name, NULL));
			} else {
				string_list_append(
					&friendly_ref_names, shortname);
			}
		}

		if (remotes->nr == 1) {
			/*
			 * Rebasing is only allowed in the case of a single
			 * upstream branch.
			 */
			printf_ln(rebasing ?
				_("branch '%s' set up to track '%s' by rebasing.") :
				_("branch '%s' set up to track '%s'."),
				local, friendly_ref_names.items[0].string);
		} else {
			printf_ln(_("branch '%s' set up to track:"), local);
			for_each_string_list_item(item, &friendly_ref_names)
				printf_ln("  %s", item->string);
		}

		string_list_clear(&friendly_ref_names, 0);
	}

	return 0;

out_err:
	strbuf_release(&key);
	error(_("unable to write upstream branch configuration"));

	advise(_("\nAfter fixing the error cause you may try to fix up\n"
		"the remote tracking information by invoking:"));
	if (remotes->nr == 1)
		advise("  git branch --set-upstream-to=%s%s%s",
			origin ? origin : "",
			origin ? "/" : "",
			remotes->items[0].string);
	else {
		advise("  git config --add branch.\"%s\".remote %s",
			local, origin ? origin : ".");
		for_each_string_list_item(item, remotes)
			advise("  git config --add branch.\"%s\".merge %s",
				local, item->string);
	}

	return -1;
}

int install_branch_config(int flag, const char *local, const char *origin,
		const char *remote)
{
	int ret;
	struct string_list remotes = STRING_LIST_INIT_DUP;

	string_list_append(&remotes, remote);
	ret = install_branch_config_multiple_remotes(flag, local, origin, &remotes);
	string_list_clear(&remotes, 0);
	return ret;
}

static int inherit_tracking(struct tracking *tracking, const char *orig_ref)
{
	const char *bare_ref;
	struct branch *branch;
	int i;

	bare_ref = orig_ref;
	skip_prefix(orig_ref, "refs/heads/", &bare_ref);

	branch = branch_get(bare_ref);
	if (!branch->remote_name) {
		warning(_("asked to inherit tracking from '%s', but no remote is set"),
			bare_ref);
		return -1;
	}

	if (branch->merge_nr < 1 || !branch->merge_name || !branch->merge_name[0]) {
		warning(_("asked to inherit tracking from '%s', but no merge configuration is set"),
			bare_ref);
		return -1;
	}

	tracking->remote = xstrdup(branch->remote_name);
	for (i = 0; i < branch->merge_nr; i++)
		string_list_append(tracking->srcs, branch->merge_name[i]);
	return 0;
}

/*
 * This is called when new_ref is branched off of orig_ref, and tries
 * to infer the settings for branch.<new_ref>.{remote,merge} from the
 * config.
 */
static void setup_tracking(const char *new_ref, const char *orig_ref,
			   enum branch_track track, int quiet)
{
	struct tracking tracking;
	struct string_list tracking_srcs = STRING_LIST_INIT_DUP;
	int config_flags = quiet ? 0 : BRANCH_CONFIG_VERBOSE;

	memset(&tracking, 0, sizeof(tracking));
	tracking.spec.dst = (char *)orig_ref;
	tracking.srcs = &tracking_srcs;
	if (track != BRANCH_TRACK_INHERIT)
		for_each_remote(find_tracked_branch, &tracking);
	else if (inherit_tracking(&tracking, orig_ref))
		return;

	if (!tracking.matches)
		switch (track) {
		case BRANCH_TRACK_ALWAYS:
		case BRANCH_TRACK_EXPLICIT:
		case BRANCH_TRACK_OVERRIDE:
		case BRANCH_TRACK_INHERIT:
			break;
		default:
			return;
		}

	if (tracking.matches > 1)
		die(_("not tracking: ambiguous information for ref %s"),
		    orig_ref);

	if (tracking.srcs->nr < 1)
		string_list_append(tracking.srcs, orig_ref);
	if (install_branch_config_multiple_remotes(config_flags, new_ref,
				tracking.remote, tracking.srcs) < 0)
		exit(-1);

	string_list_clear(tracking.srcs, 0);
}

int read_branch_desc(struct strbuf *buf, const char *branch_name)
{
	char *v = NULL;
	struct strbuf name = STRBUF_INIT;
	strbuf_addf(&name, "branch.%s.description", branch_name);
	if (git_config_get_string(name.buf, &v)) {
		strbuf_release(&name);
		return -1;
	}
	strbuf_addstr(buf, v);
	free(v);
	strbuf_release(&name);
	return 0;
}

/*
 * Check if 'name' can be a valid name for a branch; die otherwise.
 * Return 1 if the named branch already exists; return 0 otherwise.
 * Fill ref with the full refname for the branch.
 */
int validate_branchname(const char *name, struct strbuf *ref)
{
	if (strbuf_check_branch_ref(ref, name))
		die(_("'%s' is not a valid branch name"), name);

	return ref_exists(ref->buf);
}

/*
 * Check if a branch 'name' can be created as a new branch; die otherwise.
 * 'force' can be used when it is OK for the named branch already exists.
 * Return 1 if the named branch already exists; return 0 otherwise.
 * Fill ref with the full refname for the branch.
 */
int validate_new_branchname(const char *name, struct strbuf *ref, int force)
{
	struct worktree **worktrees;
	const struct worktree *wt;

	if (!validate_branchname(name, ref))
		return 0;

	if (!force)
		die(_("a branch named '%s' already exists"),
		    ref->buf + strlen("refs/heads/"));

	worktrees = get_worktrees();
	wt = find_shared_symref(worktrees, "HEAD", ref->buf);
	if (wt && !wt->is_bare)
		die(_("cannot force update the branch '%s' "
		      "checked out at '%s'"),
		    ref->buf + strlen("refs/heads/"), wt->path);
	free_worktrees(worktrees);

	return 1;
}

static int check_tracking_branch(struct remote *remote, void *cb_data)
{
	char *tracking_branch = cb_data;
	struct refspec_item query;
	memset(&query, 0, sizeof(struct refspec_item));
	query.dst = tracking_branch;
	return !remote_find_tracking(remote, &query);
}

static int validate_remote_tracking_branch(char *ref)
{
	return !for_each_remote(check_tracking_branch, ref);
}

static const char upstream_not_branch[] =
N_("cannot set up tracking information; starting point '%s' is not a branch");
static const char upstream_missing[] =
N_("the requested upstream branch '%s' does not exist");
static const char upstream_advice[] =
N_("\n"
"If you are planning on basing your work on an upstream\n"
"branch that already exists at the remote, you may need to\n"
"run \"git fetch\" to retrieve it.\n"
"\n"
"If you are planning to push out a new local branch that\n"
"will track its remote counterpart, you may want to use\n"
"\"git push -u\" to set the upstream config as you push.");

void create_branch(struct repository *r,
		   const char *name, const char *start_name,
		   int force, int clobber_head_ok, int reflog,
		   int quiet, enum branch_track track)
{
	struct commit *commit;
	struct object_id oid;
	char *real_ref;
	struct strbuf ref = STRBUF_INIT;
	int forcing = 0;
	int dont_change_ref = 0;
	int explicit_tracking = 0;

	if (track == BRANCH_TRACK_EXPLICIT || track == BRANCH_TRACK_OVERRIDE)
		explicit_tracking = 1;

	if ((track == BRANCH_TRACK_OVERRIDE || clobber_head_ok)
	    ? validate_branchname(name, &ref)
	    : validate_new_branchname(name, &ref, force)) {
		if (!force)
			dont_change_ref = 1;
		else
			forcing = 1;
	}

	real_ref = NULL;
	if (get_oid_mb(start_name, &oid)) {
		if (explicit_tracking) {
			if (advice_enabled(ADVICE_SET_UPSTREAM_FAILURE)) {
				error(_(upstream_missing), start_name);
				advise(_(upstream_advice));
				exit(1);
			}
			die(_(upstream_missing), start_name);
		}
		die(_("not a valid object name: '%s'"), start_name);
	}

	switch (dwim_ref(start_name, strlen(start_name), &oid, &real_ref, 0)) {
	case 0:
		/* Not branching from any existing branch */
		if (explicit_tracking)
			die(_(upstream_not_branch), start_name);
		break;
	case 1:
		/* Unique completion -- good, only if it is a real branch */
		if (!starts_with(real_ref, "refs/heads/") &&
		    validate_remote_tracking_branch(real_ref)) {
			if (explicit_tracking)
				die(_(upstream_not_branch), start_name);
			else
				FREE_AND_NULL(real_ref);
		}
		break;
	default:
		die(_("ambiguous object name: '%s'"), start_name);
		break;
	}

	if ((commit = lookup_commit_reference(r, &oid)) == NULL)
		die(_("not a valid branch point: '%s'"), start_name);
	oidcpy(&oid, &commit->object.oid);

	if (reflog)
		log_all_ref_updates = LOG_REFS_NORMAL;

	if (!dont_change_ref) {
		struct ref_transaction *transaction;
		struct strbuf err = STRBUF_INIT;
		char *msg;

		if (forcing)
			msg = xstrfmt("branch: Reset to %s", start_name);
		else
			msg = xstrfmt("branch: Created from %s", start_name);

		transaction = ref_transaction_begin(&err);
		if (!transaction ||
		    ref_transaction_update(transaction, ref.buf,
					   &oid, forcing ? NULL : null_oid(),
					   0, msg, &err) ||
		    ref_transaction_commit(transaction, &err))
			die("%s", err.buf);
		ref_transaction_free(transaction);
		strbuf_release(&err);
		free(msg);
	}

	if (real_ref && track)
		setup_tracking(ref.buf + 11, real_ref, track, quiet);

	strbuf_release(&ref);
	free(real_ref);
}

void remove_merge_branch_state(struct repository *r)
{
	unlink(git_path_merge_head(r));
	unlink(git_path_merge_rr(r));
	unlink(git_path_merge_msg(r));
	unlink(git_path_merge_mode(r));
	unlink(git_path_auto_merge(r));
	save_autostash(git_path_merge_autostash(r));
}

void remove_branch_state(struct repository *r, int verbose)
{
	sequencer_post_commit_cleanup(r, verbose);
	unlink(git_path_squash_msg(r));
	remove_merge_branch_state(r);
}

void die_if_checked_out(const char *branch, int ignore_current_worktree)
{
	struct worktree **worktrees = get_worktrees();
	const struct worktree *wt;

	wt = find_shared_symref(worktrees, "HEAD", branch);
	if (wt && (!ignore_current_worktree || !wt->is_current)) {
		skip_prefix(branch, "refs/heads/", &branch);
		die(_("'%s' is already checked out at '%s'"), branch, wt->path);
	}

	free_worktrees(worktrees);
}

int replace_each_worktree_head_symref(const char *oldref, const char *newref,
				      const char *logmsg)
{
	int ret = 0;
	struct worktree **worktrees = get_worktrees();
	int i;

	for (i = 0; worktrees[i]; i++) {
		struct ref_store *refs;

		if (worktrees[i]->is_detached)
			continue;
		if (!worktrees[i]->head_ref)
			continue;
		if (strcmp(oldref, worktrees[i]->head_ref))
			continue;

		refs = get_worktree_ref_store(worktrees[i]);
		if (refs_create_symref(refs, "HEAD", newref, logmsg))
			ret = error(_("HEAD of working tree %s is not updated"),
				    worktrees[i]->path);
	}

	free_worktrees(worktrees);
	return ret;
}
