/*
 * SPDX-FileCopyrightText: 2022 Yu Zhu <891085309@qq.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <FlexLayout.h>
#include <plutovg.h>
#include "meui/event.h"

#include "common/list.h"
#include "common/class.h"

typedef FlexNodeRef box_t;

#include "style.h"
#include "pqueue.h"

enum BOX_TYPE
{
    BOX_TYPE_DIV,
    BOX_TYPE_STACK,
    BOX_TYPE_CANVAS,
};

enum BOX_STATE
{
    BOX_STATE_DEFAULT,
    BOX_STATE_FOCUS,
    BOX_STATE_ACTIVE,
    BOX_STATE_HOVER,
    BOX_STATE_SELECT,
    BOX_STATE_DISABLE,
    BOX_STATE_CHECK,
    BOX_STATE_MAX,
};

class(Box)
{
    box_t node;
    enum BOX_STATE state;
    int state_changed;
    box_style_t *style_array[BOX_STATE_MAX];
    box_style_t style;
    char *text;

    double scroll_top, scroll_left;
    double scroll_width, scroll_height;
    double client_width, client_height;
    double offset_width, offset_height;

    int out_of_screen;
    int index_in_parent; // for search hitbox
    struct
    {
        plutovg_matrix_t to_screen_matrix; // box to screen
    } result;

    size_t queue_pos;

    void (*draw)(Box * this, plutovg_t * pluto);
};

box_t box_new(enum BOX_TYPE type);
void box_free(box_t node);
void box_free_recursive(box_t node);

void box_set_style(box_t node, box_style_t *style, enum BOX_STATE state);
enum BOX_STATE box_get_state(box_t node);
void box_set_state(box_t node, enum BOX_STATE state);
box_style_t *box_get_style(box_t node, enum BOX_STATE state);

const char *box_get_text_content(box_t node);
void box_set_text_content(box_t node, const char *text);

int box_get_client_width(box_t node);
int box_get_client_height(box_t node);
int box_get_scroll_width(box_t node);
int box_get_scroll_height(box_t node);
int box_get_scroll_top(box_t node);
void box_set_scroll_top(box_t node, int scroll_top);
int box_get_scroll_left(box_t node);
void box_set_scroll_left(box_t node, int scroll_left);
int box_hit(box_t node, int x, int y);

FlexSize box_measure_text(void *context, FlexSize constrainedSize);

void box_updateStyleRecursive(box_t node);
void box_drawRecursive(plutovg_t *pluto, box_t node, pqueue_t *pq);
void box_draw(box_t root);
box_t box_search_queue(box_t node, int x, int y);