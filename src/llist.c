/******************************************************************************
*   TinTin++                                                                  *
*   Copyright (C) 2004 (See CREDITS file)                                     *
*                                                                             *
*   This program is protected under the GNU GPL (See COPYING)                 *
*                                                                             *
*   This program is free software; you can redistribute it and/or modify      *
*   it under the terms of the GNU General Public License as published by      *
*   the Free Software Foundation; either version 2 of the License, or         *
*   (at your option) any later version.                                       *
*                                                                             *
*   This program is distributed in the hope that it will be useful,           *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*   GNU General Public License for more details.                              *
*                                                                             *
*   You should have received a copy of the GNU General Public License         *
*   along with this program; if not, write to the Free Software               *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*******************************************************************************/

/*********************************************************************/
/* file: llist.c - linked-list datastructure                         */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/

#include "tintin.h"

/*
	init list - return: ptr to listhead
*/

struct listroot *init_list()
{
	struct listroot *listhead;

	if ((listhead = calloc(1, sizeof(struct listroot))) == NULL)
	{
		fprintf(stderr, "couldn't alloc listhead\n");
		exit(1);
	}
	listhead->flags = LIST_FLAG_DEFAULT;

	return listhead;
}

/*
	kill list - run throught list and free nodes
*/

void kill_list(struct session *ses, int index)
{
	struct listnode *node;

	while ((node = ses->list[index]->f_node))
	{
		deletenode_list(ses, node, index);
	}
}

/*
	This function will clear all lists associated with a session
*/

DO_COMMAND(do_killall)
{
	int cnt;

	push_call("kill_all(%p,%p)",ses,arg);

	for (cnt = 0 ; cnt < LIST_MAX ; cnt++)
	{
		kill_list(ses, cnt);

		if (arg == NULL)
		{
			free(ses->list[cnt]);
		}
	}

	if (arg != NULL)
	{
		tintin_puts2("#KILLALL: LISTS CLEARED.", ses);
	}
	pop_call();
	return ses;
}

/*
	make a copy of a list - return: ptr to copy
*/

struct listroot *copy_list(struct session *ses, struct listroot *sourcelist, int index)
{
	struct listnode *node;

	ses->list[index] = init_list();

	for (node = sourcelist->f_node ; node ; node = node->next)
	{
		insertnode_list(ses, node->left, node->right, node->pr, index);
	}
	ses->list[index]->flags = sourcelist->flags;

	return ses->list[index];
}

/*
	create a node containing the ltext, rtext fields and stuff it
	into the list - in lexicographical order, or by numerical
	priority (dependent on mode) - Mods by Joann Ellsworth 2/2/94
	- More mods by Scandum
*/

void insertnode_list(struct session *ses, const char *ltext, const char *rtext, const char *prtext, int index)
{
	struct listnode *node, *newnode;

	push_call("insertnode_list(%p,%p,%p)",ses,ltext,rtext);

	if ((newnode = calloc(1, sizeof(struct listnode))) == NULL)
	{
		fprintf(stderr, "couldn't calloc listhead\n");
		dump_stack();
		exit(1);
	}

	newnode->left  = strdup(ltext);
	newnode->right = strdup(rtext);
	newnode->pr    = strdup(prtext);

	if (ses->class && index != LIST_CLASS)
	{
		newnode->class = ses->class;

		ses->class->data += NODE_FLAG_MAX;
	}

	ses->list[index]->count++;

	switch (list_table[index].mode)
	{
		case PRIORITY:
			for (node = ses->list[index]->f_node ; node ; node = node->next)
			{
				if (atof(prtext) < atof(node->pr))
				{
					INSERT_LEFT(newnode, node, ses->list[index]->f_node, next, prev);
					pop_call();
					return;
				}
			}
			LINK(newnode, ses->list[index]->f_node, ses->list[index]->l_node, next, prev);
			break;

		case ALPHA:
			for (node = ses->list[index]->f_node ; node ; node = node->next)
			{
				if (strcmp(ltext, node->left) < 0)
				{
					INSERT_LEFT(newnode, node, ses->list[index]->f_node, next, prev);
					pop_call();
					return;
				}
			}
			LINK(newnode, ses->list[index]->f_node, ses->list[index]->l_node, next, prev);
			break;

		case APPEND:
			LINK(newnode, ses->list[index]->f_node, ses->list[index]->l_node, next, prev);
			break;

		default:
			tintin_printf2(NULL, "insertnode_list: bad list_table mode: %d", list_table[index].mode);
			break;
	}
	pop_call();
	return;
}


void updatenode_list(struct session *ses, const char *ltext, const char *rtext, const char *prtext, int index)
{
	struct listnode *node;

	push_call("updatenode_list(%p,%p,%p,%p,%p)",ses,ltext,rtext,prtext,index);

	for (node = ses->list[index]->f_node ; node ; node = node->next)
	{
		if (strcmp(node->left, ltext) == 0)
		{
			if (strcmp(node->right, rtext) != 0)
			{
				free(node->right);
				node->right = strdup(rtext);
			}

			switch (list_table[index].mode)
			{
				case PRIORITY:
					if (atof(node->pr) == atof(prtext))
					{
						pop_call();
						return;
					}
					deletenode_list(ses, node, index);
					insertnode_list(ses, ltext, rtext, prtext, index);
					break;

				case APPEND:
					if (strcmp(node->pr, prtext) != 0)
					{
						free(node->pr);
						node->pr = strdup(prtext);
					}
					UNLINK(node, ses->list[index]->f_node, ses->list[index]->l_node, next, prev);
					LINK(node, ses->list[index]->f_node, ses->list[index]->l_node, next, prev);
					break;

				case ALPHA:
					if (strcmp(node->pr, prtext) != 0)
					{
						free(node->pr);
						node->pr = strdup(prtext);
					}
					break;

				default:
					tintin_printf2(ses, "updatenode_list: bad mode");
					break;
			}
			pop_call();
			return;
		}
	}
	insertnode_list(ses, ltext, rtext, prtext, index);
	pop_call();
	return;
}

void deletenode_list(struct session *ses, struct listnode *node, int index)
{
	push_call("deletenode_list(%p,%p,%p)", ses, node, index);

	if ((node->next == NULL && node != ses->list[index]->l_node)
	||  (node->prev == NULL && node != ses->list[index]->f_node))
	{
		tintin_puts2("#ERROR: delete_nodelist: unlink error.", NULL);
		dump_stack();
	}
	else
	{
		if (node == ses->list[index]->update)
		{
			ses->list[index]->update = node->next;
		}

		if (index == LIST_CLASS)
		{
			if (node->data >= NODE_FLAG_MAX)
			{
				class_kill(ses, node->left);
			}
		}

		if (ses->class == node)
		{
			ses->class = NULL;
		}
		UNLINK(node, ses->list[index]->f_node, ses->list[index]->l_node, next, prev);

		free(node->left);
		free(node->right);
		free(node->pr);
		free(node);

		if (node->class)
		{
			node->class->data -= NODE_FLAG_MAX;
		}

		ses->list[index]->count--;
	}
	pop_call();
	return;
}

/*
	search for a node containing the ltext in left-field
*/

struct listnode *searchnode_list(struct listroot *listhead, const char *cptr)
{
	struct listnode *node;

	for (node = listhead->f_node ; node ; node = node->next)
	{
		if (!strcmp(node->left, cptr))
		{
			return node;
		}
	}
	return NULL;
}

/*
	search for a node that has cptr as a beginning
*/

struct listnode *searchnode_list_begin(struct listroot *listhead, const char *cptr, int mode)
{
	struct listnode *node;
	int len;

	len = strlen(cptr);

	for (node = listhead->f_node ; node ; node = node->next)
	{
		if (strncmp(node->left, cptr, len) != 0)
		{
			continue;
		}

		if (node->left[len] == ' ' || node->left[len] == '\0')
		{
			return node;
		}
	}
	return NULL;
}

/*
	show contens of a node on screen
*/

void shownode_list(struct session *ses, struct listnode *nptr, int index)
{
	char buf[BUFFER_SIZE], out[BUFFER_SIZE];

	switch (list_table[index].args)
	{
		case 3:
			sprintf(buf, "#%s <118>{<088>%s<118>}<168>=<118>{<088>%s<118>} <168>@ <118>{<088>%s<118>}", list_table[index].name, nptr->left, nptr->right, nptr->pr);
			break;
		case 2:
			sprintf(buf, "#%s <118>{<088>%s<118>}<168>=<118>{<088>%s<118>}", list_table[index].name, nptr->left, nptr->right);
			break;
		case 1:
			sprintf(buf, "#%s <118>{<088>%s<118>}", list_table[index].name, nptr->left);
			break;
		default:
			sprintf(buf, "ERROR: list_table[index].args == 0");
			break;
	}

	substitute(ses, buf, out, SUB_COL);

	tintin_puts2(out, ses);
}

/*
	list contens of a list on screen
*/

void show_list(struct session *ses, struct listroot *listhead, int index)
{
	struct listnode *node;

	tintin_header(ses, " %s ", list_table[index].name_multi);

	for (node = listhead->f_node ; node ; node = node->next)
	{
		shownode_list(ses, node, index);
	}
}


int show_node_with_wild(struct session *ses, const char *cptr, int index)
{
	struct listnode *node;
	int flag = FALSE;

	for (node = ses->list[index]->f_node ; node ; node = node->next)
	{
		if (regexp(cptr, node->left))
		{
			shownode_list(ses, node, index);
			flag = TRUE;
		}
	}
	return flag;
}

struct listnode *search_node_with_wild(struct listroot *listhead, const char *cptr)
{
	struct listnode *node;

	for (node = listhead->f_node ; node ; node = node->next)
	{
		if (regexp(cptr, node->left))
		{
			return node;
		}
	}
	return NULL;
}


/*
	create a node containing the ltext, rtext fields and place at the
	end of a list - as insertnode_list(), but not alphabetical
*/

void addnode_list(struct listroot *listhead, const char *ltext, const char *rtext, const char *prtext)
{
	struct listnode *newnode;

	if ((newnode = calloc(1, sizeof(struct listnode))) == NULL)
	{
		fprintf(stderr, "couldn't calloc listhead");
		exit(1);
	}
	newnode->left  = calloc(1, strlen(ltext)  + 1);
	newnode->right = calloc(1, strlen(rtext)  + 1);
	newnode->pr    = calloc(1, strlen(prtext) + 1);

	sprintf(newnode->left,  "%s", ltext);
	sprintf(newnode->right, "%s", rtext);
	sprintf(newnode->pr,    "%s", prtext);

	LINK(newnode, listhead->f_node, listhead->l_node, next, prev);

	listhead->count++;
}

int count_list(struct listroot *listhead)
{
	struct listnode *node;
	int cnt = 0;
	for (node = listhead->f_node ; node ; node = node->next)
	{
		cnt++;
	}
	return cnt;
}
