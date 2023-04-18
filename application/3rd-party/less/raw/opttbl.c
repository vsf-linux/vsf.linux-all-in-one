/*
 * Copyright (C) 1984-2022  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */


/*
 * The option table.
 */

#include "less.h"
#include "option.h"

#ifdef __VSF__
#	define quiet				(less_public_ctx->__quiet)
#	define how_search			(less_public_ctx->__how_search)
#	define top_scroll			(less_public_ctx->__top_scroll)
#	define pr_type				(less_public_ctx->__pr_type)
#	define bs_mode				(less_public_ctx->__bs_mode)
#	define know_dumb			(less_public_ctx->__know_dumb)
#	define quit_at_eof			(less_public_ctx->__quit_at_eof)
#	define quit_if_one_screen	(less_public_ctx->__quit_if_one_screen)
#	define squeeze				(less_public_ctx->__squeeze)
#	define tabstop				(less_public_ctx->__tabstop)
#	define back_scroll			(less_public_ctx->__back_scroll)
#	define forw_scroll			(less_public_ctx->__forw_scroll)
#	define caseless				(less_public_ctx->__caseless)
#	define linenums				(less_public_ctx->__linenums)
#	define autobuf				(less_public_ctx->__autobuf)
#	define bufspace				(less_public_ctx->__bufspace)
#	define ctldisp				(less_public_ctx->__ctldisp)
#	define force_open			(less_public_ctx->__force_open)
#	define swindow				(less_public_ctx->__swindow)
#	define jump_sline			(less_public_ctx->__jump_sline)
#	define jump_sline_fraction	(less_public_ctx->__jump_sline_fraction)
#	define shift_count_fraction	(less_public_ctx->__shift_count_fraction)
#	define chopline				(less_public_ctx->__chopline)
#	define no_init				(less_public_ctx->__no_init)
#	define no_keypad			(less_public_ctx->__no_keypad)
#	define twiddle				(less_public_ctx->__twiddle)
#	define show_attn			(less_public_ctx->__show_attn)
#	define shift_count			(less_public_ctx->__shift_count)
#	define status_col			(less_public_ctx->__status_col)
#	define use_lessopen			(less_public_ctx->__use_lessopen)
#	define quit_on_intr			(less_public_ctx->__quit_on_intr)
#	define follow_mode			(less_public_ctx->__follow_mode)
#	define oldbot				(less_public_ctx->__oldbot)
#	define opt_use_backslash	(less_public_ctx->__opt_use_backslash)
#	define rscroll_char			(less_public_ctx->__rscroll_char)
#	define rscroll_attr			(less_public_ctx->__rscroll_attr)
#	define no_hist_dups			(less_public_ctx->__no_hist_dups)
#	define mousecap				(less_public_ctx->__mousecap)
#	define wheel_lines			(less_public_ctx->__wheel_lines)
#	define perma_marks			(less_public_ctx->__perma_marks)
#	define linenum_width		(less_public_ctx->__linenum_width)
#	define status_col_width		(less_public_ctx->__status_col_width)
#	define incr_search			(less_public_ctx->__incr_search)
#	define use_color			(less_public_ctx->__use_color)
#	define want_filesize		(less_public_ctx->__want_filesize)
#	define status_line			(less_public_ctx->__status_line)
#	define header_lines			(less_public_ctx->__header_lines)
#	define header_cols			(less_public_ctx->__header_cols)
#	define nonum_headers		(less_public_ctx->__nonum_headers)
#	define redraw_on_quit		(less_public_ctx->__redraw_on_quit)
#	define def_search_type		(less_public_ctx->__def_search_type)
#if HILITE_SEARCH
#	define hilite_search		(less_public_ctx->__hilite_search)
#endif
#	define less_is_more			(less_public_ctx->__less_is_more)
#else
/*
 * Variables controlled by command line options.
 */
public int quiet;               /* Should we suppress the audible bell? */
public int how_search;          /* Where should forward searches start? */
public int top_scroll;          /* Repaint screen from top?
                                   (alternative is scroll from bottom) */
public int pr_type;             /* Type of prompt (short, medium, long) */
public int bs_mode;             /* How to process backspaces */
public int know_dumb;           /* Don't complain about dumb terminals */
public int quit_at_eof;         /* Quit after hitting end of file twice */
public int quit_if_one_screen;  /* Quit if EOF on first screen */
public int squeeze;             /* Squeeze multiple blank lines into one */
public int tabstop;             /* Tab settings */
public int back_scroll;         /* Repaint screen on backwards movement */
public int forw_scroll;         /* Repaint screen on forward movement */
public int caseless;            /* Do "caseless" searches */
public int linenums;            /* Use line numbers */
public int autobuf;             /* Automatically allocate buffers as needed */
public int bufspace;            /* Max buffer space per file (K) */
public int ctldisp;             /* Send control chars to screen untranslated */
public int force_open;          /* Open the file even if not regular file */
public int swindow;             /* Size of scrolling window */
public int jump_sline;          /* Screen line of "jump target" */
public long jump_sline_fraction = -1;
public long shift_count_fraction = -1;
public int chopline;            /* Truncate displayed lines at screen width */
public int no_init;             /* Disable sending ti/te termcap strings */
public int no_keypad;           /* Disable sending ks/ke termcap strings */
public int twiddle;             /* Show tildes after EOF */
public int show_attn;           /* Hilite first unread line */
public int shift_count;         /* Number of positions to shift horizontally */
public int status_col;          /* Display a status column */
public int use_lessopen;        /* Use the LESSOPEN filter */
public int quit_on_intr;        /* Quit on interrupt */
public int follow_mode;         /* F cmd Follows file desc or file name? */
public int oldbot;              /* Old bottom of screen behavior {{REMOVE}} */
public int opt_use_backslash;   /* Use backslash escaping in option parsing */
public char rscroll_char;       /* Char which marks chopped lines with -S */
public int rscroll_attr;        /* Attribute of rscroll_char */
public int no_hist_dups;        /* Remove dups from history list */
public int mousecap;            /* Allow mouse for scrolling */
public int wheel_lines;         /* Number of lines to scroll on mouse wheel scroll */
public int perma_marks;         /* Save marks in history file */
public int linenum_width;       /* Width of line numbers */
public int status_col_width;    /* Width of status column */
public int incr_search;         /* Incremental search */
public int use_color;           /* Use UI color */
public int want_filesize;       /* Scan to EOF if necessary to get file size */
public int status_line;         /* Highlight entire marked lines */
public int header_lines;        /* Freeze header lines at top of screen */
public int header_cols;         /* Freeze header columns at left of screen */
public int nonum_headers;       /* Don't give headers line numbers */
public int redraw_on_quit;      /* Redraw last screen after term deinit */
public int def_search_type;     /* */
#if HILITE_SEARCH
public int hilite_search;       /* Highlight matched search patterns? */
#endif

public int less_is_more = 0;    /* Make compatible with POSIX more */
#endif

/*
 * Long option names.
 */
static const struct optname a_optname      = { "search-skip-screen",   NULL };
static const struct optname b_optname      = { "buffers",              NULL };
static const struct optname B__optname     = { "auto-buffers",         NULL };
static const struct optname c_optname      = { "clear-screen",         NULL };
static const struct optname d_optname      = { "dumb",                 NULL };
static const struct optname D__optname     = { "color",                NULL };
static const struct optname e_optname      = { "quit-at-eof",          NULL };
static const struct optname f_optname      = { "force",                NULL };
static const struct optname F__optname     = { "quit-if-one-screen",   NULL };
#if HILITE_SEARCH
static const struct optname g_optname      = { "hilite-search",        NULL };
#endif
static const struct optname h_optname      = { "max-back-scroll",      NULL };
static const struct optname i_optname      = { "ignore-case",          NULL };
static const struct optname j_optname      = { "jump-target",          NULL };
static const struct optname J__optname     = { "status-column",        NULL };
#if USERFILE
static const struct optname k_optname      = { "lesskey-file",         NULL };
#if HAVE_LESSKEYSRC 
static const struct optname ks_optname     = { "lesskey-src",          NULL };
#endif /* HAVE_LESSKEYSRC */
#endif
static const struct optname K__optname     = { "quit-on-intr",         NULL };
static const struct optname L__optname     = { "no-lessopen",          NULL };
static const struct optname m_optname      = { "long-prompt",          NULL };
static const struct optname n_optname      = { "line-numbers",         NULL };
#if LOGFILE
static const struct optname o_optname      = { "log-file",             NULL };
static const struct optname O__optname     = { "LOG-FILE",             NULL };
#endif
static const struct optname p_optname      = { "pattern",              NULL };
static const struct optname P__optname     = { "prompt",               NULL };
static const struct optname q2_optname     = { "silent",               NULL };
#ifndef __VSF__
static const struct optname q_optname      = { "quiet",                &q2_optname };
#endif
static const struct optname r_optname      = { "raw-control-chars",    NULL };
static const struct optname s_optname      = { "squeeze-blank-lines",  NULL };
static const struct optname S__optname     = { "chop-long-lines",      NULL };
#if TAGS
static const struct optname t_optname      = { "tag",                  NULL };
static const struct optname T__optname     = { "tag-file",             NULL };
#endif
static const struct optname u_optname      = { "underline-special",    NULL };
static const struct optname V__optname     = { "version",              NULL };
static const struct optname w_optname      = { "hilite-unread",        NULL };
static const struct optname x_optname      = { "tabs",                 NULL };
static const struct optname X__optname     = { "no-init",              NULL };
static const struct optname y_optname      = { "max-forw-scroll",      NULL };
static const struct optname z_optname      = { "window",               NULL };
static const struct optname quote_optname  = { "quotes",               NULL };
static const struct optname tilde_optname  = { "tilde",                NULL };
static const struct optname query_optname  = { "help",                 NULL };
static const struct optname pound_optname  = { "shift",                NULL };
static const struct optname keypad_optname = { "no-keypad",            NULL };
static const struct optname oldbot_optname = { "old-bot",              NULL };
static const struct optname follow_optname = { "follow-name",          NULL };
static const struct optname use_backslash_optname = { "use-backslash", NULL };
static const struct optname rscroll_optname = { "rscroll", NULL };
static const struct optname nohistdups_optname = { "no-histdups",      NULL };
static const struct optname mousecap_optname = { "mouse",              NULL };
static const struct optname wheel_lines_optname = { "wheel-lines",     NULL };
static const struct optname perma_marks_optname = { "save-marks",      NULL };
static const struct optname linenum_width_optname = { "line-num-width", NULL };
static const struct optname status_col_width_optname = { "status-col-width", NULL };
static const struct optname incr_search_optname = { "incsearch",       NULL };
static const struct optname use_color_optname = { "use-color",         NULL };
static const struct optname want_filesize_optname = { "file-size",     NULL };
static const struct optname status_line_optname = { "status-line",     NULL };
static const struct optname header_optname = { "header",               NULL };
static const struct optname nonum_headers_optname = { "no-number-headers", NULL };
static const struct optname redraw_on_quit_optname = { "redraw-on-quit", NULL };
static const struct optname search_type_optname = { "search-options", NULL };
#if LESSTEST
static const struct optname ttyin_name_optname = { "tty",              NULL };
static const struct optname rstat_optname  = { "rstat",                NULL };
#endif /*LESSTEST*/


/*
 * Table of all options and their semantics.
 *
 * For BOOL and TRIPLE options, odesc[0], odesc[1], odesc[2] are
 * the description of the option when set to 0, 1 or 2, respectively.
 * For NUMBER options, odesc[0] is the prompt to use when entering
 * a new value, and odesc[1] is the description, which should contain 
 * one %d which is replaced by the value of the number.
 * For STRING options, odesc[0] is the prompt to use when entering
 * a new value, and odesc[1], if not NULL, is the set of characters
 * that are valid in the string.
 */
#ifdef __VSF__
struct __less_opttbl_ctx {
	struct optname __q_optname;
	struct loption __option[55];
};
static void __less_opttbl_init_option(struct __less_opttbl_ctx *ctx);
define_vsf_less_mod(less_opttbl,
	sizeof(struct __less_opttbl_ctx),
	VSF_LESS_MOD_OPTTBL,
	__less_opttbl_init_option
)
#	define less_opttbl_ctx		((struct __less_opttbl_ctx *)vsf_linux_dynlib_ctx(&vsf_less_mod_name(less_opttbl)))
#	define option				(less_opttbl_ctx->__option)
static void __less_opttbl_init_option(struct __less_opttbl_ctx *ctx)
{
	ctx->__q_optname = (struct optname) { "quiet", (struct optname *)&q2_optname };
	
struct loption __option[] =
#else
static struct loption option[] =
#endif
{
	{ 'a', (struct optname *)&a_optname,
		TRIPLE, OPT_ONPLUS, &how_search, NULL,
		{
			"Search includes displayed screen",
			"Search skips displayed screen",
			"Search includes all of displayed screen"
		}
	},

	{ 'b', (struct optname *)&b_optname,
		NUMBER|INIT_HANDLER, 64, &bufspace, opt_b, 
		{
			"Max buffer space per file (K): ",
			"Max buffer space per file: %dK",
			NULL
		}
	},
	{ 'B', (struct optname *)&B__optname,
		BOOL, OPT_ON, &autobuf, NULL,
		{
			"Don't automatically allocate buffers",
			"Automatically allocate buffers when needed",
			NULL
		}
	},
	{ 'c', (struct optname *)&c_optname,
		TRIPLE, OPT_OFF, &top_scroll, NULL,
		{
			"Repaint by scrolling from bottom of screen",
			"Repaint by painting from top of screen",
			"Repaint by painting from top of screen"
		}
	},
	{ 'd', (struct optname *)&d_optname,
		BOOL|NO_TOGGLE, OPT_OFF, &know_dumb, NULL,
		{
			"Assume intelligent terminal",
			"Assume dumb terminal",
			NULL
		}
	},
	{ 'D', (struct optname *)&D__optname,
		STRING|REPAINT|NO_QUERY, 0, NULL, opt_D,
		{
			"color desc: ", 
			NULL,
			NULL
		}
	},
	{ 'e', (struct optname *)&e_optname,
		TRIPLE, OPT_OFF, &quit_at_eof, NULL,
		{
			"Don't quit at end-of-file",
			"Quit at end-of-file",
			"Quit immediately at end-of-file"
		}
	},
	{ 'f', (struct optname *)&f_optname,
		BOOL, OPT_OFF, &force_open, NULL,
		{
			"Open only regular files",
			"Open even non-regular files",
			NULL
		}
	},
	{ 'F', (struct optname *)&F__optname,
		BOOL, OPT_OFF, &quit_if_one_screen, NULL,
		{
			"Don't quit if end-of-file on first screen",
			"Quit if end-of-file on first screen",
			NULL
		}
	},
#if HILITE_SEARCH
	{ 'g', (struct optname *)&g_optname,
		TRIPLE|HL_REPAINT, OPT_ONPLUS, &hilite_search, NULL,
		{
			"Don't highlight search matches",
			"Highlight matches for previous search only",
			"Highlight all matches for previous search pattern",
		}
	},
#endif
	{ 'h', (struct optname *)&h_optname,
		NUMBER, -1, &back_scroll, NULL,
		{
			"Backwards scroll limit: ",
			"Backwards scroll limit is %d lines",
			NULL
		}
	},
	{ 'i', (struct optname *)&i_optname,
		TRIPLE|HL_REPAINT, OPT_OFF, &caseless, opt_i,
		{
			"Case is significant in searches",
			"Ignore case in searches",
			"Ignore case in searches and in patterns"
		}
	},
	{ 'j', (struct optname *)&j_optname,
		STRING, 0, NULL, opt_j,
		{
			"Target line: ",
			"0123456789.-",
			NULL
		}
	},
	{ 'J', (struct optname *)&J__optname,
		BOOL|REPAINT, OPT_OFF, &status_col, NULL,
		{
			"Don't display a status column",
			"Display a status column",
			NULL
		}
	},
#if USERFILE
	{ 'k', (struct optname *)&k_optname,
		STRING|NO_TOGGLE|NO_QUERY, 0, NULL, opt_k,
		{ NULL, NULL, NULL }
	},
#if HAVE_LESSKEYSRC 
	{ OLETTER_NONE, (struct optname *)&ks_optname,
		STRING|NO_TOGGLE|NO_QUERY, 0, NULL, opt_ks,
		{ NULL, NULL, NULL }
	},
#endif /* HAVE_LESSKEYSRC */
#endif
	{ 'K', (struct optname *)&K__optname,
		BOOL, OPT_OFF, &quit_on_intr, NULL,
		{
			"Interrupt (ctrl-C) returns to prompt",
			"Interrupt (ctrl-C) exits less",
			NULL
		}
	},
	{ 'L', (struct optname *)&L__optname,
		BOOL, OPT_ON, &use_lessopen, NULL,
		{
			"Don't use the LESSOPEN filter",
			"Use the LESSOPEN filter",
			NULL
		}
	},
	{ 'm', (struct optname *)&m_optname,
		TRIPLE, OPT_OFF, &pr_type, NULL,
		{
			"Short prompt",
			"Medium prompt",
			"Long prompt"
		}
	},
	{ 'n', (struct optname *)&n_optname,
		TRIPLE|REPAINT, OPT_ON, &linenums, NULL,
		{
			"Don't use line numbers",
			"Use line numbers",
			"Constantly display line numbers"
		}
	},
#if LOGFILE
	{ 'o', (struct optname *)&o_optname,
		STRING, 0, NULL, opt_o,
		{ "log file: ", NULL, NULL }
	},
	{ 'O', (struct optname *)&O__optname,
		STRING, 0, NULL, opt__O,
		{ "Log file: ", NULL, NULL }
	},
#endif
	{ 'p', (struct optname *)&p_optname,
		STRING|NO_TOGGLE|NO_QUERY, 0, NULL, opt_p,
		{ NULL, NULL, NULL }
	},
	{ 'P', (struct optname *)&P__optname,
		STRING, 0, NULL, opt__P,
		{ "prompt: ", NULL, NULL }
	},
	{ 'q', (struct optname *)&ctx->__q_optname,
		TRIPLE, OPT_OFF, &quiet, NULL,
		{
			"Ring the bell for errors AND at eof/bof",
			"Ring the bell for errors but not at eof/bof",
			"Never ring the bell"
		}
	},
	{ 'r', (struct optname *)&r_optname,
		TRIPLE|REPAINT, OPT_OFF, &ctldisp, NULL,
		{
			"Display control characters as ^X",
			"Display control characters directly (not recommended)",
			"Display ANSI sequences directly, other control characters as ^X"
		}
	},
	{ 's', (struct optname *)&s_optname,
		BOOL|REPAINT, OPT_OFF, &squeeze, NULL,
		{
			"Display all blank lines",
			"Squeeze multiple blank lines",
			NULL
		}
	},
	{ 'S', (struct optname *)&S__optname,
		BOOL|REPAINT, OPT_OFF, &chopline, NULL,
		{
			"Fold long lines",
			"Chop long lines",
			NULL
		}
	},
#if TAGS
	{ 't', (struct optname *)&t_optname,
		STRING|NO_QUERY, 0, NULL, opt_t,
		{ "tag: ", NULL, NULL }
	},
	{ 'T', (struct optname *)&T__optname,
		STRING, 0, NULL, opt__T,
		{ "tags file: ", NULL, NULL }
	},
#endif
	{ 'u', (struct optname *)&u_optname,
		TRIPLE|REPAINT, OPT_OFF, &bs_mode, NULL,
		{
			"Display underlined text in underline mode",
			"Backspaces cause overstrike",
			"Print backspace as ^H"
		}
	},
	{ 'V', (struct optname *)&V__optname,
		NOVAR, 0, NULL, opt__V,
		{ NULL, NULL, NULL }
	},
	{ 'w', (struct optname *)&w_optname,
		TRIPLE|REPAINT, OPT_OFF, &show_attn, NULL,
		{
			"Don't highlight first unread line",
			"Highlight first unread line after forward-screen",
			"Highlight first unread line after any forward movement",
		}
	},
	{ 'x', (struct optname *)&x_optname,
		STRING|REPAINT, 0, NULL, opt_x,
		{
			"Tab stops: ",
			"0123456789,",
			NULL
		}
	},
	{ 'X', (struct optname *)&X__optname,
		BOOL|NO_TOGGLE, OPT_OFF, &no_init, NULL,
		{
			"Send init/deinit strings to terminal",
			"Don't use init/deinit strings",
			NULL
		}
	},
	{ 'y', (struct optname *)&y_optname,
		NUMBER, -1, &forw_scroll, NULL,
		{
			"Forward scroll limit: ",
			"Forward scroll limit is %d lines",
			NULL
		}
	},
	{ 'z', (struct optname *)&z_optname,
		NUMBER, -1, &swindow, NULL,
		{
			"Scroll window size: ",
			"Scroll window size is %d lines",
			NULL
		}
	},
	{ '"', (struct optname *)&quote_optname,
		STRING, 0, NULL, opt_quote,
		{ "quotes: ", NULL, NULL }
	},
	{ '~', (struct optname *)&tilde_optname,
		BOOL|REPAINT, OPT_ON, &twiddle, NULL,
		{
			"Don't show tildes after end of file",
			"Show tildes after end of file",
			NULL
		}
	},
	{ '?', (struct optname *)&query_optname,
		NOVAR, 0, NULL, opt_query,
		{ NULL, NULL, NULL }
	},
	{ '#', (struct optname *)&pound_optname,
		STRING, 0, NULL, opt_shift,
		{
			"Horizontal shift: ",
			"0123456789.",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&keypad_optname,
		BOOL|NO_TOGGLE, OPT_OFF, &no_keypad, NULL,
		{
			"Use keypad mode",
			"Don't use keypad mode",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&oldbot_optname,
		BOOL, OPT_OFF, &oldbot, NULL,
		{
			"Use new bottom of screen behavior",
			"Use old bottom of screen behavior",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&follow_optname,
		BOOL, FOLLOW_DESC, &follow_mode, NULL,
		{
			"F command follows file descriptor",
			"F command follows file name",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&use_backslash_optname,
		BOOL, OPT_OFF, &opt_use_backslash, NULL,
		{
			"Use backslash escaping in command line parameters",
			"Don't use backslash escaping in command line parameters",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&rscroll_optname,
		STRING|REPAINT|INIT_HANDLER, 0, NULL, opt_rscroll,
		{ "right scroll character: ", NULL, NULL }
	},
	{ OLETTER_NONE, (struct optname *)&nohistdups_optname,
		BOOL, OPT_OFF, &no_hist_dups, NULL,
		{
			"Allow duplicates in history list",
			"Remove duplicates from history list",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&mousecap_optname,
		TRIPLE, OPT_OFF, &mousecap, opt_mousecap,
		{
			"Ignore mouse input",
			"Use the mouse for scrolling",
			"Use the mouse for scrolling (reverse)"
		}
	},
	{ OLETTER_NONE, (struct optname *)&wheel_lines_optname,
		NUMBER|INIT_HANDLER, 0, &wheel_lines, opt_wheel_lines,
		{
			"Lines to scroll on mouse wheel: ",
			"Scroll %d line(s) on mouse wheel",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&perma_marks_optname,
		BOOL, OPT_OFF, &perma_marks, NULL,
		{
			"Don't save marks in history file",
			"Save marks in history file",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&linenum_width_optname,
		NUMBER|REPAINT, MIN_LINENUM_WIDTH, &linenum_width, opt_linenum_width,
		{
			"Line number width: ",
			"Line number width is %d chars",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&status_col_width_optname,
		NUMBER|REPAINT, 2, &status_col_width, opt_status_col_width,
		{
			"Status column width: ",
			"Status column width is %d chars",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&incr_search_optname,
		BOOL, OPT_OFF, &incr_search, NULL,
		{
			"Incremental search is off",
			"Incremental search is on",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&use_color_optname,
		BOOL|REPAINT, OPT_OFF, &use_color, NULL,
		{
			"Don't use color",
			"Use color",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&want_filesize_optname,
		BOOL|REPAINT, OPT_OFF, &want_filesize, opt_filesize,
		{
			"Don't get size of each file",
			"Get size of each file",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&status_line_optname,
		BOOL|REPAINT, OPT_OFF, &status_line, NULL,
		{
			"Don't color each line with its status column color",
			"Color each line with its status column color",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&header_optname,
		STRING|REPAINT, 0, NULL, opt_header,
		{
			"Header lines: ",
			NULL,
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&nonum_headers_optname,
		BOOL|REPAINT, 0, &nonum_headers, NULL,
		{
			"Number header lines",
			"Don't number header lines",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&redraw_on_quit_optname,
		BOOL, OPT_OFF, &redraw_on_quit, NULL,
		{
			"Don't redraw screen when quitting",
			"Redraw last screen when quitting",
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&search_type_optname,
		STRING, 0, NULL, opt_search_type,
		{
			"Search options: ",
			NULL,
			NULL
		}
	},
#if LESSTEST
	{ OLETTER_NONE, (struct optname *)&ttyin_name_optname,
		STRING|NO_TOGGLE, 0, NULL, opt_ttyin_name,
		{
			NULL,
			NULL,
			NULL
		}
	},
	{ OLETTER_NONE, (struct optname *)&rstat_optname,
		STRING|NO_TOGGLE, 0, NULL, opt_rstat,
		{
			NULL,
			NULL,
			NULL
		}
	},
#endif /*LESSTEST*/
	{ '\0', NULL, NOVAR, 0, NULL, NULL, { NULL, NULL, NULL } }
};
#ifdef __VSF__
	VSF_LINUX_ASSERT(dimof(less_opttbl_ctx->__option) >= dimof(__option));
	memcpy(less_opttbl_ctx->__option, __option, sizeof(__option));
}
#endif

/*
 * Initialize each option to its default value.
 */
	public void
init_option(VOID_PARAM)
{
	struct loption *o;
	char *p;

	p = lgetenv("LESS_IS_MORE");
	if (!isnullenv(p))
		less_is_more = 1;

	for (o = option;  o->oletter != '\0';  o++)
	{
		/*
		 * Set each variable to its default.
		 */
		if (o->ovar != NULL)
			*(o->ovar) = o->odefault;
		if (o->otype & INIT_HANDLER)
			(*(o->ofunc))(INIT, (char *) NULL);
	}
}

/*
 * Find an option in the option table, given its option letter.
 */
	public struct loption *
findopt(c)
	int c;
{
	struct loption *o;

	for (o = option;  o->oletter != '\0';  o++)
	{
		if (o->oletter == c)
			return (o);
		if ((o->otype & TRIPLE) && ASCII_TO_UPPER(o->oletter) == c)
			return (o);
	}
	return (NULL);
}

/*
 *
 */
	static int
is_optchar(c)
	char c;
{
	if (ASCII_IS_UPPER(c))
		return 1;
	if (ASCII_IS_LOWER(c))
		return 1;
	if (c == '-')
		return 1;
	return 0;
}

/*
 * Find an option in the option table, given its option name.
 * p_optname is the (possibly partial) name to look for, and
 * is updated to point after the matched name.
 * p_oname if non-NULL is set to point to the full option name.
 */
	public struct loption *
findopt_name(p_optname, p_oname, p_err)
	char **p_optname;
	char **p_oname;
	int *p_err;
{
	char *optname = *p_optname;
	struct loption *o;
	struct optname *oname;
	int len;
	int uppercase;
	struct loption *maxo = NULL;
	struct optname *maxoname = NULL;
	int maxlen = 0;
	int ambig = 0;
	int exact = 0;

	/*
	 * Check all options.
	 */
	for (o = option;  o->oletter != '\0';  o++)
	{
		/*
		 * Check all names for this option.
		 */
		for (oname = o->onames;  oname != NULL;  oname = oname->onext)
		{
			/* 
			 * Try normal match first (uppercase == 0),
			 * then, then if it's a TRIPLE option,
			 * try uppercase match (uppercase == 1).
			 */
			for (uppercase = 0;  uppercase <= 1;  uppercase++)
			{
				len = sprefix(optname, oname->oname, uppercase);
				if (len <= 0 || is_optchar(optname[len]))
				{
					/*
					 * We didn't use all of the option name.
					 */
					continue;
				}
				if (!exact && len == maxlen)
					/*
					 * Already had a partial match,
					 * and now there's another one that
					 * matches the same length.
					 */
					ambig = 1;
				else if (len > maxlen)
				{
					/*
					 * Found a better match than
					 * the one we had.
					 */
					maxo = o;
					maxoname = oname;
					maxlen = len;
					ambig = 0;
					exact = (len == (int)strlen(oname->oname));
				}
				if (!(o->otype & TRIPLE))
					break;
			}
		}
	}
	if (ambig)
	{
		/*
		 * Name matched more than one option.
		 */
		if (p_err != NULL)
			*p_err = OPT_AMBIG;
		return (NULL);
	}
	*p_optname = optname + maxlen;
	if (p_oname != NULL)
		*p_oname = maxoname == NULL ? NULL : maxoname->oname;
	return (maxo);
}
