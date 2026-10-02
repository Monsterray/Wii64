/**
 * Wii64 - FuncTree.c
 * Copyright (C) 2009, 2010 Mike Slegeir
 * 
 * Handles a BST of functions ordered by their address
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#include <stdlib.h>
#include "../r4300.h"
#include "Recompile.h"
#include "../Recomp-Cache.h"

#include "../../main/perf_prof.h"

static inline PowerPC_func_node** _find(PowerPC_func_node** node, unsigned int addr){
#ifdef PERF_PROF
	unsigned int depth = 0;
#endif
	while(*node){
#ifdef PERF_PROF
		depth++;
#endif
		if(addr < (*node)->function->start_addr)
			node = &(*node)->left;
		else if(addr >= (*node)->function->end_addr)
			node = &(*node)->right;
		else
			break;
	}
#ifdef PERF_PROF
	perfProf_treeDepth(depth);
#endif
	return node;
}

PowerPC_func* find_func(PowerPC_func_node** root, unsigned int addr){
	unsigned long long timer = perfProf_subsystemBegin(PERF_SUB_LOOKUP);
	start_section(FUNCS_SECTION);
	PowerPC_func_node* node = *_find(root, addr);
	end_section(FUNCS_SECTION);
	perfProf_subsystemEnd(PERF_SUB_LOOKUP, timer);
	return node ? node->function : NULL;
}

/* The first func, by address, that overlaps [lo, hi], or NULL. The funcs
   in a block do not overlap: a compile frees the ones its func overlaps. */
PowerPC_func* find_func_overlap(PowerPC_func_node** root, unsigned int lo, unsigned int hi){
	PowerPC_func_node* node = *root;
	PowerPC_func* first = NULL;
	while(node){
		if(lo >= node->function->end_addr)
			node = node->right;
		else {
			if(hi >= node->function->start_addr) first = node->function;
			node = node->left;
		}
	}
	return first;
}

void insert_func(PowerPC_func_node** root, PowerPC_func* func){
	PowerPC_func_node** node = _find(root, func->start_addr);
	if(*node) return; // Avoid a memory leak if this function exists

	*node = MetaCache_Alloc(sizeof(PowerPC_func_node));
	(*node)->function = func;
	(*node)->left = (*node)->right = NULL;
}

void remove_func(PowerPC_func_node** root, PowerPC_func* func){
	PowerPC_func_node** node = _find(root, func->start_addr);
	if(!*node) return; // Avoid a memory error if the function doesn't exist

	PowerPC_func_node* old = *node;
	if(!(*node)->left)
		*node = (*node)->right;
	else if(!(*node)->right)
		*node = (*node)->left;
	else {
		// The node has two children, find the node's predecessor and swap
		PowerPC_func_node** pre;
		for(pre = &(*node)->left; (*pre)->right; pre = &(*pre)->right);
		(*node)->function = (*pre)->function;
		old = *pre;
		*pre = (*pre)->left;
	}

	MetaCache_Free(old);
}
