/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */

/* avl.h - avl tree definitions */
/*
 * Copyright (c) 1993 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */


#ifndef _AVL
#define _AVL

/*
 * this structure represents a generic avl tree node.
 */

typedef struct avlnode {
	caddr_t		avl_data;
	signed char	avl_bf;
	struct avlnode	*avl_left;
	struct avlnode	*avl_right;
} Avlnode;

#define NULLAVL	((Avlnode *) NULL)

/* balance factor values */
#define LH 	-1
#define EH 	0
#define RH 	1

/* avl routines */
#define avl_getone(x)	(x == 0 ? 0 : (x)->avl_data)
#define avl_onenode(x)	(x == 0 || ((x)->avl_left == 0 && (x)->avl_right == 0))
extern int		avl_insert();
extern caddr_t		avl_delete();
extern caddr_t		avl_find();
extern caddr_t		avl_getfirst();
extern caddr_t		avl_getnext();
extern int		avl_dup_error();
extern int		avl_apply();
extern int		avl_free();

/* apply traversal types */
#define AVL_PREORDER	1
#define AVL_INORDER	2
#define AVL_POSTORDER	3
/* what apply returns if it ran out of nodes */
#define AVL_NOMORE	-6

#ifndef _IFP
#define _IFP
typedef int	(*IFP)();
#endif

caddr_t avl_find_lin( Avlnode *root, caddr_t data, IFP fcmp );

#endif /* _AVL */
