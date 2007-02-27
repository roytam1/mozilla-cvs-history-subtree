/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=78:
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
 * JS ABC generation.
 */
#include "jsstddef.h"
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <string.h>
#include "jstypes.h"
#include "jsarena.h" /* Added by JSIFY */
#include "jsutil.h" /* Added by JSIFY */
#include "jsbit.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jsemitabc.h"
#include "jsfun.h"
#include "jsnum.h"
#include "jsopcode.h"
#include "jsparse.h"
#include "jsregexp.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"

#include <vector>
#include "abc/RuntimeConstants.h"
#include "abc/ActionBlockEmitter.h"

JS_FRIEND_API(JSBool)
js_InitCodeGenerator(JSContext *cx, JSCodeGenerator *cg,
                     JSArenaPool *codePool, JSArenaPool *notePool,
                     const char *filename, uintN lineno,
                     JSPrincipals *principals)
{
    memset(cg, 0, sizeof *cg);
    TREE_CONTEXT_INIT(&cg->treeContext);
    cg->treeContext.flags |= TCF_COMPILING;
    cg->filename = filename;
    cg->firstLine = lineno;
    cg->principals = principals;
    return JS_TRUE;
}

JS_FRIEND_API(void)
js_FinishCodeGenerator(JSContext *cx, JSCodeGenerator *cg)
{
    TREE_CONTEXT_FINISH(&cg->treeContext);
}

/* XXX too many "... statement" L10N gaffes below -- fix via js.msg! */
const char js_with_statement_str[] = "with statement";
const char js_finally_block_str[]  = "finally block";
const char js_script_str[]         = "script";

static const char *statementName[] = {
    "label statement",       /* LABEL */
    "if statement",          /* IF */
    "else statement",        /* ELSE */
    "switch statement",      /* SWITCH */
    "block",                 /* BLOCK */
    js_with_statement_str,   /* WITH */
    "catch block",           /* CATCH */
    "try block",             /* TRY */
    js_finally_block_str,    /* FINALLY */
    js_finally_block_str,    /* SUBROUTINE */
    "do loop",               /* DO_LOOP */
    "for loop",              /* FOR_LOOP */
    "for/in loop",           /* FOR_IN_LOOP */
    "while loop",            /* WHILE_LOOP */
};

static const char *
StatementName(JSCodeGenerator *cg)
{
    if (!cg->treeContext.topStmt)
        return js_script_str;
    return statementName[cg->treeContext.topStmt->type];
}

static void
ReportStatementTooLarge(JSContext *cx, JSCodeGenerator *cg)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NEED_DIET,
                         StatementName(cg));
}

JSBool
js_InStatement(JSTreeContext *tc, JSStmtType type)
{
    JSStmtInfo *stmt;

    for (stmt = tc->topStmt; stmt; stmt = stmt->down) {
        if (stmt->type == type)
            return JS_TRUE;
    }
    return JS_FALSE;
}

JSBool
js_IsGlobalReference(JSTreeContext *tc, JSAtom *atom, JSBool *loopyp)
{
    JSStmtInfo *stmt;
    JSObject *obj;
    JSScope *scope;

    *loopyp = JS_FALSE;
    for (stmt = tc->topStmt; stmt; stmt = stmt->down) {
        if (stmt->type == STMT_WITH)
            return JS_FALSE;
        if (STMT_IS_LOOP(stmt)) {
            *loopyp = JS_TRUE;
            continue;
        }
        if (stmt->flags & SIF_SCOPE) {
            obj = ATOM_TO_OBJECT(stmt->atom);
            JS_ASSERT(LOCKED_OBJ_GET_CLASS(obj) == &js_BlockClass);
            scope = OBJ_SCOPE(obj);
            if (SCOPE_GET_PROPERTY(scope, ATOM_TO_JSID(atom)))
                return JS_FALSE;
        }
    }
    return JS_TRUE;
}

void
js_PushStatement(JSTreeContext *tc, JSStmtInfo *stmt, JSStmtType type,
                 ptrdiff_t top)
{
    stmt->type = type;
    stmt->flags = 0;
    SET_STATEMENT_TOP(stmt, top);
    stmt->atom = NULL;
    stmt->down = tc->topStmt;
    tc->topStmt = stmt;
    if (STMT_LINKS_SCOPE(stmt)) {
        stmt->downScope = tc->topScopeStmt;
        tc->topScopeStmt = stmt;
    } else {
        stmt->downScope = NULL;
    }
    if (STMT_TYPE_IS_LOOP(type) ||
        type == STMT_SWITCH ||
        type == STMT_LABEL) {
        stmt->loopIndex = cg->loopIndex++;
    }
}

void
js_PushBlockScope(JSTreeContext *tc, JSStmtInfo *stmt, JSAtom *blockAtom,
                  ptrdiff_t top)
{
    JSObject *blockObj;

    js_PushStatement(tc, stmt, STMT_BLOCK, top);
    stmt->flags |= SIF_SCOPE;
    blockObj = ATOM_TO_OBJECT(blockAtom);
    blockObj->slots[JSSLOT_PARENT] = OBJECT_TO_JSVAL(tc->blockChain);
    stmt->downScope = tc->topScopeStmt;
    tc->topScopeStmt = stmt;
    tc->blockChain = blockObj;
    stmt->atom = blockAtom;
}

void
js_PopStatement(JSTreeContext *tc)
{
    JSStmtInfo *stmt;
    JSObject *blockObj;

    stmt = tc->topStmt;
    tc->topStmt = stmt->down;
    if (STMT_LINKS_SCOPE(stmt)) {
        tc->topScopeStmt = stmt->downScope;
        if (stmt->flags & SIF_SCOPE) {
            blockObj = ATOM_TO_OBJECT(stmt->atom);
            tc->blockChain = JSVAL_TO_OBJECT(blockObj->slots[JSSLOT_PARENT]);
        }
    }
    if (STMT_TYPE_IS_LOOP(type) ||
        type == STMT_SWITCH ||
        type == STMT_LABEL) {
        --cg->loopIndex;
        JS_ASSERT(cg->loopIndex == stmt->loopIndex);
    }
}

JSBool
js_PopStatementCG(JSContext *cx, JSCodeGenerator *cg)
{
    JSStmtInfo *stmt;

    stmt = cg->treeContext.topStmt;
    if (!STMT_IS_TRYING(stmt) &&
        (!BackPatch(cx, cg, stmt->breaks, CG_NEXT(cg), JSOP_GOTO) ||
         !BackPatch(cx, cg, stmt->continues, CG_CODE(cg, stmt->update),
                    JSOP_GOTO))) {
        return JS_FALSE;
    }
    js_PopStatement(&cg->treeContext);
    return JS_TRUE;
}

JSBool
js_DefineCompileTimeConstant(JSContext *cx, JSCodeGenerator *cg, JSAtom *atom,
                             JSParseNode *pn)
{
    jsdouble dval;
    jsint ival;
    JSAtom *valueAtom;
    JSAtomListElement *ale;

    /* XXX just do numbers for now */
    if (pn->pn_type == TOK_NUMBER) {
        dval = pn->pn_dval;
        valueAtom = (JSDOUBLE_IS_INT(dval, ival) && INT_FITS_IN_JSVAL(ival))
                    ? js_AtomizeInt(cx, ival, 0)
                    : js_AtomizeDouble(cx, dval, 0);
        if (!valueAtom)
            return JS_FALSE;
        ale = js_IndexAtom(cx, atom, &cg->constList);
        if (!ale)
            return JS_FALSE;
        ALE_SET_VALUE(ale, ATOM_KEY(valueAtom));
    }
    return JS_TRUE;
}

JSStmtInfo *
js_LexicalLookup(JSTreeContext *tc, JSAtom *atom, jsint *slotp, JSBool letdecl)
{
    JSStmtInfo *stmt;
    JSObject *obj;
    JSScope *scope;
    JSScopeProperty *sprop;
    jsval v;

    for (stmt = tc->topScopeStmt; stmt; stmt = stmt->downScope) {
        if (stmt->type == STMT_WITH) {
            /* Ignore with statements enclosing a single let declaration. */
            if (letdecl)
                continue;
            break;
        }

        /* Skip "maybe scope" statements that don't contain let bindings. */
        if (!(stmt->flags & SIF_SCOPE))
            continue;

        obj = ATOM_TO_OBJECT(stmt->atom);
        JS_ASSERT(LOCKED_OBJ_GET_CLASS(obj) == &js_BlockClass);
        scope = OBJ_SCOPE(obj);
        sprop = SCOPE_GET_PROPERTY(scope, ATOM_TO_JSID(atom));
        if (sprop) {
            JS_ASSERT(sprop->flags & SPROP_HAS_SHORTID);

            if (slotp) {
                /*
                 * Use LOCKED_OBJ_GET_SLOT since we know obj is single-
                 * threaded and owned by this compiler activation.
                 */
                v = LOCKED_OBJ_GET_SLOT(obj, JSSLOT_BLOCK_DEPTH);
                JS_ASSERT(JSVAL_IS_INT(v) && JSVAL_TO_INT(v) >= 0);
                *slotp = JSVAL_TO_INT(v) + sprop->shortid;
            }
            return stmt;
        }
    }

    if (slotp)
        *slotp = -1;
    return stmt;
}

JSBool
js_LookupCompileTimeConstant(JSContext *cx, JSCodeGenerator *cg, JSAtom *atom,
                             jsval *vp)
{
    JSBool ok;
    JSStackFrame *fp;
    JSStmtInfo *stmt;
    jsint slot;
    JSAtomListElement *ale;
    JSObject *obj, *pobj;
    JSProperty *prop;
    uintN attrs;

    /*
     * fp chases cg down the stack, but only until we reach the outermost cg.
     * This enables propagating consts from top-level into switch cases in a
     * function compiled along with the top-level script.  All stack frames
     * with matching code generators should be flagged with JSFRAME_COMPILING;
     * we check sanity here.
     */
    *vp = JSVAL_VOID;
    ok = JS_TRUE;
    fp = cx->fp;
    do {
        JS_ASSERT(fp->flags & JSFRAME_COMPILING);

        obj = fp->varobj;
        if (obj == fp->scopeChain) {
            /* XXX this will need revising when 'let const' is added. */
            stmt = js_LexicalLookup(&cg->treeContext, atom, &slot, JS_FALSE);
            if (stmt)
                return JS_TRUE;

            ATOM_LIST_SEARCH(ale, &cg->constList, atom);
            if (ale) {
                *vp = ALE_VALUE(ale);
                return JS_TRUE;
            }

            /*
             * Try looking in the variable object for a direct property that
             * is readonly and permanent.  We know such a property can't be
             * shadowed by another property on obj's prototype chain, or a
             * with object or catch variable; nor can prop's value be changed,
             * nor can prop be deleted.
             */
            prop = NULL;
            if (OBJ_GET_CLASS(cx, obj) == &js_FunctionClass) {
                ok = js_LookupHiddenProperty(cx, obj, ATOM_TO_JSID(atom),
                                             &pobj, &prop);
                if (!ok)
                    break;
                if (prop) {
#ifdef DEBUG
                    JSScopeProperty *sprop = (JSScopeProperty *)prop;

                    /*
                     * Any hidden property must be a formal arg or local var,
                     * which will shadow a global const of the same name.
                     */
                    JS_ASSERT(sprop->getter == js_GetArgument ||
                              sprop->getter == js_GetLocalVariable);
#endif
                    OBJ_DROP_PROPERTY(cx, pobj, prop);
                    break;
                }
            }

            ok = OBJ_LOOKUP_PROPERTY(cx, obj, ATOM_TO_JSID(atom), &pobj, &prop);
            if (ok) {
                if (pobj == obj &&
                    (fp->flags & (JSFRAME_EVAL | JSFRAME_COMPILE_N_GO))) {
                    /*
                     * We're compiling code that will be executed immediately,
                     * not re-executed against a different scope chain and/or
                     * variable object.  Therefore we can get constant values
                     * from our variable object here.
                     */
                    ok = OBJ_GET_ATTRIBUTES(cx, obj, ATOM_TO_JSID(atom), prop,
                                            &attrs);
                    if (ok && !(~attrs & (JSPROP_READONLY | JSPROP_PERMANENT)))
                        ok = OBJ_GET_PROPERTY(cx, obj, ATOM_TO_JSID(atom), vp);
                }
                if (prop)
                    OBJ_DROP_PROPERTY(cx, pobj, prop);
            }
            if (!ok || prop)
                break;
        }
        fp = fp->down;
    } while ((cg = cg->parent) != NULL);
    return ok;
}

static JSBool
EmitAtomOp(JSContext *cx, JSParseNode *pn, JSOp op, JSCodeGenerator *cg)
{
    ActionBlockEmitter *abc = cg->abcEmitter;
    const char *name = js_AtomToPrintableString(cx, pn->pn_atom);

    std::vector<std::string> qualifiers;
    qualifiers.push_back("");

    switch (pn->pn_type) {
      case TOK_DOT:
        abc->GetProperty(name, qualifiers, false, false, false);
        break;

      case TOK_NAME:
        abc->FindProperty(name, qualifiers, false, false, false);
        abc->GetProperty(name, qualifiers, false, false, false);
        break;

      case TOK_STRING:
        /* XXX should use UTF-8 */
        abc->PushString(name);
        break;

      default:
        JS_ASSERT(0);
        break;
    }
    JS_ASSERT(0);
    return JS_FALSE;
}

/*
 * This routine tries to optimize name gets and sets to stack slot loads and
 * stores, given the variables object and scope chain in cx's top frame, the
 * compile-time context in tc, and a TOK_NAME node pn.  It returns false on
 * error, true on success.
 *
 * The caller can inspect pn->pn_slot for a non-negative slot number to tell
 * whether optimization occurred, in which case BindNameToSlot also updated
 * pn->pn_op.  If pn->pn_slot is still -1 on return, pn->pn_op nevertheless
 * may have been optimized, e.g., from JSOP_NAME to JSOP_ARGUMENTS.  Whether
 * or not pn->pn_op was modified, if this function finds an argument or local
 * variable name, pn->pn_attrs will contain the property's attributes after a
 * successful return.
 *
 * NB: if you add more opcodes specialized from JSOP_NAME, etc., don't forget
 * to update the TOK_FOR (for-in) and TOK_ASSIGN (op=, e.g. +=) special cases
 * in js_EmitTree.
 */
static JSBool
BindNameToSlot(JSContext *cx, JSTreeContext *tc, JSParseNode *pn,
               JSBool letdecl)
{
    JSAtom *atom;
    JSStmtInfo *stmt;
    jsint slot;
    JSOp op;
    JSStackFrame *fp;
    JSObject *obj, *pobj;
    JSClass *clasp;
    JSBool optimizeGlobals;
    JSPropertyOp getter;
    uintN attrs;
    JSAtomListElement *ale;
    JSProperty *prop;
    JSScopeProperty *sprop;

    JS_ASSERT(pn->pn_type == TOK_NAME);
    if (pn->pn_slot >= 0 || pn->pn_op == JSOP_ARGUMENTS)
        return JS_TRUE;

    /* QNAME references can never be optimized to use arg/var storage. */
    if (pn->pn_op == JSOP_QNAMEPART)
        return JS_TRUE;

    /*
     * We can't optimize if we are compiling a with statement and its body,
     * or we're in a catch block whose exception variable has the same name
     * as this node.  FIXME: we should be able to optimize catch vars to be
     * block-locals.
     */
    atom = pn->pn_atom;
    stmt = js_LexicalLookup(tc, atom, &slot, letdecl);
    if (stmt) {
        if (stmt->type == STMT_WITH)
            return JS_TRUE;

        JS_ASSERT(stmt->flags & SIF_SCOPE);
        JS_ASSERT(slot >= 0);
        op = pn->pn_op;
        switch (op) {
          case JSOP_NAME:     op = JSOP_GETLOCAL; break;
          case JSOP_SETNAME:  op = JSOP_SETLOCAL; break;
          case JSOP_INCNAME:  op = JSOP_INCLOCAL; break;
          case JSOP_NAMEINC:  op = JSOP_LOCALINC; break;
          case JSOP_DECNAME:  op = JSOP_DECLOCAL; break;
          case JSOP_NAMEDEC:  op = JSOP_LOCALDEC; break;
          case JSOP_FORNAME:  op = JSOP_FORLOCAL; break;
          case JSOP_DELNAME:  op = JSOP_FALSE; break;
          default: JS_ASSERT(0);
        }
        if (op != pn->pn_op) {
            pn->pn_op = op;
            pn->pn_slot = slot;
        }
        return JS_TRUE;
    }

    /*
     * A Script object can be used to split an eval into a compile step done
     * at construction time, and an execute step done separately, possibly in
     * a different scope altogether.  We therefore cannot do any name-to-slot
     * optimizations, but must lookup names at runtime.  Note that script_exec
     * ensures that its caller's frame has a Call object, so arg and var name
     * lookups will succeed.
     */
    fp = cx->fp;
    if (fp->flags & JSFRAME_SCRIPT_OBJECT)
        return JS_TRUE;

    /*
     * We can't optimize if var and closure (a local function not in a larger
     * expression and not at top-level within another's body) collide.
     * XXX suboptimal: keep track of colliding names and deoptimize only those
     */
    if (tc->flags & TCF_FUN_CLOSURE_VS_VAR)
        return JS_TRUE;

    /*
     * We can't optimize if we're not compiling a function body, whether via
     * eval, or directly when compiling a function statement or expression.
     */
    obj = fp->varobj;
    clasp = OBJ_GET_CLASS(cx, obj);
    if (clasp != &js_FunctionClass && clasp != &js_CallClass) {
        /* Check for an eval or debugger frame. */
        if (fp->flags & JSFRAME_SPECIAL)
            return JS_TRUE;

        /*
         * Optimize global variable accesses if there are at least 100 uses
         * in unambiguous contexts, or failing that, if least half of all the
         * uses of global vars/consts/functions are in loops.
         */
        optimizeGlobals = (tc->globalUses >= 100 ||
                           (tc->loopyGlobalUses &&
                            tc->loopyGlobalUses >= tc->globalUses / 2));
        if (!optimizeGlobals)
            return JS_TRUE;
    } else {
        optimizeGlobals = JS_FALSE;
    }

    /*
     * We can't optimize if we are in an eval called inside a with statement.
     */
    if (fp->scopeChain != obj)
        return JS_TRUE;

    op = pn->pn_op;
    getter = NULL;
#ifdef __GNUC__
    attrs = slot = 0;   /* quell GCC overwarning */
#endif
    if (optimizeGlobals) {
        /*
         * We are optimizing global variables, and there is no pre-existing
         * global property named atom.  If atom was declared via const or var,
         * optimize pn to access fp->vars using the appropriate JOF_QVAR op.
         */
        ATOM_LIST_SEARCH(ale, &tc->decls, atom);
        if (!ale) {
            /* Use precedes declaration, or name is never declared. */
            return JS_TRUE;
        }

        attrs = (ALE_JSOP(ale) == JSOP_DEFCONST)
                ? JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT
                : JSPROP_ENUMERATE | JSPROP_PERMANENT;

        /* Index atom so we can map fast global number to name. */
        JS_ASSERT(tc->flags & TCF_COMPILING);
        ale = js_IndexAtom(cx, atom, &((JSCodeGenerator *) tc)->atomList);
        if (!ale)
            return JS_FALSE;

        /* Defend against tc->numGlobalVars 16-bit overflow. */
        slot = ALE_INDEX(ale);
        if ((slot + 1) >> 16)
            return JS_TRUE;

        if ((uint16)(slot + 1) > tc->numGlobalVars)
            tc->numGlobalVars = (uint16)(slot + 1);
    } else {
        /*
         * We may be able to optimize name to stack slot. Look for an argument
         * or variable property in the function, or its call object, not found
         * in any prototype object.  Rewrite pn_op and update pn accordingly.
         * NB: We know that JSOP_DELNAME on an argument or variable evaluates
         * to false, due to JSPROP_PERMANENT.
         */
        if (!js_LookupHiddenProperty(cx, obj, ATOM_TO_JSID(atom), &pobj, &prop))
            return JS_FALSE;
        sprop = (JSScopeProperty *) prop;
        if (sprop) {
            if (pobj == obj) {
                getter = sprop->getter;
                attrs = sprop->attrs;
                slot = (sprop->flags & SPROP_HAS_SHORTID) ? sprop->shortid : -1;
            }
            OBJ_DROP_PROPERTY(cx, pobj, prop);
        }
    }

    if (optimizeGlobals || getter) {
        if (optimizeGlobals) {
            switch (op) {
              case JSOP_NAME:     op = JSOP_GETGVAR; break;
              case JSOP_SETNAME:  op = JSOP_SETGVAR; break;
              case JSOP_SETCONST: /* NB: no change */ break;
              case JSOP_INCNAME:  op = JSOP_INCGVAR; break;
              case JSOP_NAMEINC:  op = JSOP_GVARINC; break;
              case JSOP_DECNAME:  op = JSOP_DECGVAR; break;
              case JSOP_NAMEDEC:  op = JSOP_GVARDEC; break;
              case JSOP_FORNAME:  /* NB: no change */ break;
              case JSOP_DELNAME:  /* NB: no change */ break;
              default: JS_ASSERT(0);
            }
        } else if (getter == js_GetLocalVariable ||
                   getter == js_GetCallVariable) {
            switch (op) {
              case JSOP_NAME:     op = JSOP_GETVAR; break;
              case JSOP_SETNAME:  op = JSOP_SETVAR; break;
              case JSOP_SETCONST: op = JSOP_SETVAR; break;
              case JSOP_INCNAME:  op = JSOP_INCVAR; break;
              case JSOP_NAMEINC:  op = JSOP_VARINC; break;
              case JSOP_DECNAME:  op = JSOP_DECVAR; break;
              case JSOP_NAMEDEC:  op = JSOP_VARDEC; break;
              case JSOP_FORNAME:  op = JSOP_FORVAR; break;
              case JSOP_DELNAME:  op = JSOP_FALSE; break;
              default: JS_ASSERT(0);
            }
        } else if (getter == js_GetArgument ||
                   (getter == js_CallClass.getProperty &&
                    fp->fun && (uintN) slot < fp->fun->nargs)) {
            switch (op) {
              case JSOP_NAME:     op = JSOP_GETARG; break;
              case JSOP_SETNAME:  op = JSOP_SETARG; break;
              case JSOP_INCNAME:  op = JSOP_INCARG; break;
              case JSOP_NAMEINC:  op = JSOP_ARGINC; break;
              case JSOP_DECNAME:  op = JSOP_DECARG; break;
              case JSOP_NAMEDEC:  op = JSOP_ARGDEC; break;
              case JSOP_FORNAME:  op = JSOP_FORARG; break;
              case JSOP_DELNAME:  op = JSOP_FALSE; break;
              default: JS_ASSERT(0);
            }
        }
        if (op != pn->pn_op) {
            pn->pn_op = op;
            pn->pn_slot = slot;
        }
        pn->pn_attrs = attrs;
    }

    if (pn->pn_slot < 0) {
        /*
         * We couldn't optimize pn, so it's not a global or local slot name.
         * Now we must check for the predefined arguments variable.  It may be
         * overridden by assignment, in which case the function is heavyweight
         * and the interpreter will look up 'arguments' in the function's call
         * object.
         */
        if (pn->pn_op == JSOP_NAME &&
            atom == cx->runtime->atomState.argumentsAtom) {
            pn->pn_op = JSOP_ARGUMENTS;
            return JS_TRUE;
        }

        tc->flags |= TCF_FUN_USES_NONLOCALS;
    }
    return JS_TRUE;
}

static JSBool
EmitPropOp(JSContext *cx, JSParseNode *pn, JSOp op, JSCodeGenerator *cg)
{
    JSParseNode *pn2, *pndot, *pnup, *pndown;
    ptrdiff_t top;

    pn2 = pn->pn_expr;
    if (op == JSOP_GETPROP &&
        pn->pn_type == TOK_DOT &&
        pn2->pn_type == TOK_NAME) {
        /* Try to optimize arguments.length into JSOP_ARGCNT. */
        if (!BindNameToSlot(cx, &cg->treeContext, pn2, JS_FALSE))
            return JS_FALSE;
    }

    /*
     * If the object operand is also a dotted property reference, reverse the
     * list linked via pn_expr temporarily so we can iterate over it from the
     * bottom up (reversing again as we go), to avoid excessive recursion.
     */
    if (pn2->pn_type == TOK_DOT) {
        pndot = pn2;
        pnup = NULL;
        top = CG_OFFSET(cg);
        for (;;) {
            /* Reverse pndot->pn_expr to point up, not down. */
            pndot->pn_offset = top;
            pndown = pndot->pn_expr;
            pndot->pn_expr = pnup;
            if (pndown->pn_type != TOK_DOT)
                break;
            pnup = pndot;
            pndot = pndown;
        }

        /* pndown is a primary expression, not a dotted property reference. */
        if (!js_EmitTree(cx, cg, pndown))
            return JS_FALSE;

        do {
            /* Walk back up the list, emitting annotated name ops. */
            if (!EmitAtomOp(cx, pndot, pndot->pn_op, cg))
                return JS_FALSE;

            /* Reverse the pn_expr link again. */
            pnup = pndot->pn_expr;
            pndot->pn_expr = pndown;
            pndown = pndot;
        } while ((pndot = pnup) != NULL);
    } else {
        if (!js_EmitTree(cx, cg, pn2))
            return JS_FALSE;
    }

    JS_ASSERT(pn->pn_atom);
    return EmitAtomOp(cx, pn, op, cg);
}

static JSBool
EmitElemOp(JSContext *cx, JSParseNode *pn, JSOp op, JSCodeGenerator *cg)
{
    JSParseNode *left, *right, *next, ltmp, rtmp;
    jsint slot;

    if (pn->pn_arity == PN_LIST) {
        /* Left-associative operator chain to avoid too much recursion. */
        JS_ASSERT(pn->pn_op == JSOP_GETELEM || pn->pn_op == JSOP_IMPORTELEM);
        JS_ASSERT(pn->pn_count >= 3);
        left = pn->pn_head;
        right = PN_LAST(pn);
        next = left->pn_next;
        JS_ASSERT(next != right);

        /*
         * Try to optimize arguments[0][j]... into JSOP_ARGSUB<0> followed by
         * one or more index expression and JSOP_GETELEM op pairs.
         */
        if (left->pn_type == TOK_NAME && next->pn_type == TOK_NUMBER) {
            if (!BindNameToSlot(cx, &cg->treeContext, left, JS_FALSE))
                return JS_FALSE;
        }

        /*
         * Check whether we generated JSOP_ARGSUB, just above, and have only
         * one more index expression to emit.  Given arguments[0][j], we must
         * skip the while loop altogether, falling through to emit code for j
         * (in the subtree referenced by right), followed by the annotated op,
         * at the bottom of this function.
         */
        JS_ASSERT(next != right || pn->pn_count == 3);
        if (left == pn->pn_head) {
            if (!js_EmitTree(cx, cg, left))
                return JS_FALSE;
        }
        while (next != right) {
            if (!js_EmitTree(cx, cg, next))
                return JS_FALSE;
            JS_ASSERT(0);
            next = next->pn_next;
        }
    } else {
        if (pn->pn_arity == PN_NAME) {
            /*
             * Set left and right so pn appears to be a TOK_LB node, instead
             * of a TOK_DOT node.  See the TOK_FOR/IN case in js_EmitTree, and
             * EmitDestructuringOps nearer below.  In the destructuring case,
             * the base expression (pn_expr) of the name may be null, which
             * means we have to emit a JSOP_BINDNAME.
             */
            left = pn->pn_expr;
            if (!left) {
                left = &ltmp;
                left->pn_type = TOK_OBJECT;
                left->pn_op = JSOP_BINDNAME;
                left->pn_arity = PN_NULLARY;
                left->pn_pos = pn->pn_pos;
                left->pn_atom = pn->pn_atom;
            }
            right = &rtmp;
            right->pn_type = TOK_STRING;
            JS_ASSERT(ATOM_IS_STRING(pn->pn_atom));
            right->pn_op = js_IsIdentifier(ATOM_TO_STRING(pn->pn_atom))
                           ? JSOP_QNAMEPART
                           : JSOP_STRING;
            right->pn_arity = PN_NULLARY;
            right->pn_pos = pn->pn_pos;
            right->pn_atom = pn->pn_atom;
        } else {
            JS_ASSERT(pn->pn_arity == PN_BINARY);
            left = pn->pn_left;
            right = pn->pn_right;
        }

        /* Try to optimize arguments[0] (e.g.) into JSOP_ARGSUB<0>. */
        if (op == JSOP_GETELEM &&
            left->pn_type == TOK_NAME &&
            right->pn_type == TOK_NUMBER) {
            if (!BindNameToSlot(cx, &cg->treeContext, left, JS_FALSE))
                return JS_FALSE;
        }

        if (!js_EmitTree(cx, cg, left))
            return JS_FALSE;
    }

    /* The right side of the descendant operator is implicitly quoted. */
    JS_ASSERT(op != JSOP_DESCENDANTS || right->pn_type != TOK_STRING ||
              right->pn_op == JSOP_QNAMEPART);
    if (!js_EmitTree(cx, cg, right))
        return JS_FALSE;
    JS_ASSERT(0);
    return JS_FALSE;
}

static JSBool
EmitNumberOp(JSContext *cx, jsdouble dval, JSCodeGenerator *cg)
{
    cg->abcEmitter->PushNumber(dval, TYPE_number);
    return JS_TRUE;
}

static JSBool
EmitSwitch(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn,
           JSStmtInfo *stmtInfo)
{
    JSOp switchOp;
    JSBool ok, hasDefault, constPropagated;
    ptrdiff_t top, off, defaultOffset;
    JSParseNode *pn2, *pn3, *pn4;
    uint32 caseCount, tableLength;
    JSParseNode **table;
    jsdouble d;
    jsint i, low, high;
    jsval v;
    JSAtom *atom;
    JSAtomListElement *ale;
    intN noteIndex;
    size_t switchSize, tableSize;
    JSObject *obj;
    jsint count;

    /* Try for most optimal, fall back if not dense ints, and per ECMAv2. */
    switchOp = JSOP_TABLESWITCH;
    ok = JS_TRUE;
    hasDefault = constPropagated = JS_FALSE;
    defaultOffset = -1;

    /*
     * If the switch contains let variables scoped by its body, model the
     * resulting block on the stack first, before emitting the discriminant's
     * bytecode (in case the discriminant contains a stack-model dependency
     * such as a let expression).
     */
    pn2 = pn->pn_right;
    if (pn2->pn_type == TOK_LEXICALSCOPE) {
        atom = pn2->pn_atom;
        obj = ATOM_TO_OBJECT(atom);
        OBJ_SET_BLOCK_DEPTH(cx, obj, cg->stackDepth);

        /*
         * Push the body's block scope before discriminant code-gen for proper
         * static block scope linkage in case the discriminant contains a let
         * expression.  The block's locals must lie under the discriminant on
         * the stack so that case-dispatch bytecodes can find the discriminant
         * on top of stack.
         */
        js_PushBlockScope(&cg->treeContext, stmtInfo, atom, -1);
        stmtInfo->type = STMT_SWITCH;

        count = OBJ_BLOCK_COUNT(cx, obj);
        cg->stackDepth += count;
        if ((uintN)cg->stackDepth > cg->maxStackDepth)
            cg->maxStackDepth = cg->stackDepth;

        /* Emit JSOP_ENTERBLOCK before code to evaluate the discriminant. */
        JS_ASSERT(0); /* EMIT_ATOM_INDEX_OP(JSOP_ENTERBLOCK, ALE_INDEX(ale)); */

        /*
         * Pop the switch's statement info around discriminant code-gen.  Note
         * how this leaves cg->treeContext.blockChain referencing the switch's
         * block scope object, which is necessary for correct block parenting
         * in the case where the discriminant contains a let expression.
         */
        cg->treeContext.topStmt = stmtInfo->down;
        cg->treeContext.topScopeStmt = stmtInfo->downScope;
    }
#ifdef __GNUC__
    else {
        atom = NULL;
        count = -1;
    }
#endif

    /*
     * Emit code for the discriminant first (or nearly first, in the case of a
     * switch whose body is a block scope).
     */
    if (!js_EmitTree(cx, cg, pn->pn_left))
        return JS_FALSE;

    /* Switch bytecodes run from here till end of final case. */
    if (pn2->pn_type == TOK_LC) {
        js_PushStatement(&cg->treeContext, stmtInfo, STMT_SWITCH, top);
    } else {
        /* Re-push the switch's statement info record. */
        cg->treeContext.topStmt = cg->treeContext.topScopeStmt = stmtInfo;

        /* Set the statement info record's idea of top. */
        stmtInfo->update = top;

        /* Advance pn2 to refer to the switch case list. */
        pn2 = pn2->pn_expr;
    }

    caseCount = pn2->pn_count;
    tableLength = 0;
    table = NULL;

    if (caseCount == 0 ||
        (caseCount == 1 &&
         (hasDefault = (pn2->pn_head->pn_type == TOK_DEFAULT)))) {
        caseCount = 0;
        low = 0;
        high = -1;
    } else {
#define INTMAP_LENGTH   256
        jsbitmap intmap_space[INTMAP_LENGTH];
        jsbitmap *intmap = NULL;
        int32 intmap_bitlen = 0;

        low  = JSVAL_INT_MAX;
        high = JSVAL_INT_MIN;

        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            if (pn3->pn_type == TOK_DEFAULT) {
                hasDefault = JS_TRUE;
                caseCount--;    /* one of the "cases" was the default */
                continue;
            }

            JS_ASSERT(pn3->pn_type == TOK_CASE);
            if (switchOp == JSOP_CONDSWITCH)
                continue;

            pn4 = pn3->pn_left;
            switch (pn4->pn_type) {
              case TOK_NUMBER:
                d = pn4->pn_dval;
                if (JSDOUBLE_IS_INT(d, i) && INT_FITS_IN_JSVAL(i)) {
                    pn3->pn_val = INT_TO_JSVAL(i);
                } else {
                    atom = js_AtomizeDouble(cx, d, 0);
                    if (!atom) {
                        ok = JS_FALSE;
                        goto release;
                    }
                    pn3->pn_val = ATOM_KEY(atom);
                }
                break;
              case TOK_STRING:
                pn3->pn_val = ATOM_KEY(pn4->pn_atom);
                break;
              case TOK_NAME:
                if (!pn4->pn_expr) {
                    ok = js_LookupCompileTimeConstant(cx, cg, pn4->pn_atom, &v);
                    if (!ok)
                        goto release;
                    if (!JSVAL_IS_VOID(v)) {
                        pn3->pn_val = v;
                        constPropagated = JS_TRUE;
                        break;
                    }
                }
                /* FALL THROUGH */
              case TOK_PRIMARY:
                if (pn4->pn_op == JSOP_TRUE) {
                    pn3->pn_val = JSVAL_TRUE;
                    break;
                }
                if (pn4->pn_op == JSOP_FALSE) {
                    pn3->pn_val = JSVAL_FALSE;
                    break;
                }
                /* FALL THROUGH */
              default:
                switchOp = JSOP_CONDSWITCH;
                continue;
            }

            JS_ASSERT(JSVAL_IS_NUMBER(pn3->pn_val) ||
                      JSVAL_IS_STRING(pn3->pn_val) ||
                      JSVAL_IS_BOOLEAN(pn3->pn_val));

            if (switchOp != JSOP_TABLESWITCH)
                continue;
            if (!JSVAL_IS_INT(pn3->pn_val)) {
                switchOp = JSOP_LOOKUPSWITCH;
                continue;
            }
            i = JSVAL_TO_INT(pn3->pn_val);
            if ((jsuint)(i + (jsint)JS_BIT(15)) >= (jsuint)JS_BIT(16)) {
                switchOp = JSOP_LOOKUPSWITCH;
                continue;
            }
            if (i < low)
                low = i;
            if (high < i)
                high = i;

            /*
             * Check for duplicates, which require a JSOP_LOOKUPSWITCH.
             * We bias i by 65536 if it's negative, and hope that's a rare
             * case (because it requires a malloc'd bitmap).
             */
            if (i < 0)
                i += JS_BIT(16);
            if (i >= intmap_bitlen) {
                if (!intmap &&
                    i < (INTMAP_LENGTH << JS_BITS_PER_WORD_LOG2)) {
                    intmap = intmap_space;
                    intmap_bitlen = INTMAP_LENGTH << JS_BITS_PER_WORD_LOG2;
                } else {
                    /* Just grab 8K for the worst-case bitmap. */
                    intmap_bitlen = JS_BIT(16);
                    intmap = (jsbitmap *)
                        JS_malloc(cx,
                                  (JS_BIT(16) >> JS_BITS_PER_WORD_LOG2)
                                  * sizeof(jsbitmap));
                    if (!intmap) {
                        JS_ReportOutOfMemory(cx);
                        return JS_FALSE;
                    }
                }
                memset(intmap, 0, intmap_bitlen >> JS_BITS_PER_BYTE_LOG2);
            }
            if (JS_TEST_BIT(intmap, i)) {
                switchOp = JSOP_LOOKUPSWITCH;
                continue;
            }
            JS_SET_BIT(intmap, i);
        }

      release:
        if (intmap && intmap != intmap_space)
            JS_free(cx, intmap);
        if (!ok)
            return JS_FALSE;

        /*
         * Compute table length and select lookup instead if overlarge or
         * more than half-sparse.
         */
        if (switchOp == JSOP_TABLESWITCH) {
            tableLength = (uint32)(high - low + 1);
            if (tableLength >= JS_BIT(16) || tableLength > 2 * caseCount)
                switchOp = JSOP_LOOKUPSWITCH;
        }
    }

    /*
     * Emit a note with two offsets: first tells total switch code length,
     * second tells offset to first JSOP_CASE if condswitch.
     */
    noteIndex = js_NewSrcNote3(cx, cg, SRC_SWITCH, 0, 0);
    if (noteIndex < 0)
        return JS_FALSE;

    if (switchOp == JSOP_CONDSWITCH) {
        /*
         * 0 bytes of immediate for unoptimized ECMAv2 switch.
         */
        switchSize = 0;
    } else if (switchOp == JSOP_TABLESWITCH) {
        /*
         * 3 offsets (len, low, high) before the table, 1 per entry.
         */
        switchSize = (size_t)(JUMP_OFFSET_LEN * (3 + tableLength));
    } else {
        /*
         * JSOP_LOOKUPSWITCH:
         * 1 offset (len) and 1 atom index (npairs) before the table,
         * 1 atom index and 1 jump offset per entry.
         */
        switchSize = (size_t)(JUMP_OFFSET_LEN + ATOM_INDEX_LEN +
                              (ATOM_INDEX_LEN + JUMP_OFFSET_LEN) * caseCount);
    }

    /*
     * Emit switchOp followed by switchSize bytes of jump or lookup table.
     *
     * If switchOp is JSOP_LOOKUPSWITCH or JSOP_TABLESWITCH, it is crucial
     * to emit the immediate operand(s) by which bytecode readers such as
     * BuildSpanDepTable discover the length of the switch opcode *before*
     * calling js_SetJumpOffset (which may call BuildSpanDepTable).  It's
     * also important to zero all unknown jump offset immediate operands,
     * so they can be converted to span dependencies with null targets to
     * be computed later (js_EmitN zeros switchSize bytes after switchOp).
     */
    JS_ASSERT(0); /* if (js_EmitN(cx, cg, switchOp, switchSize) < 0) return JS_FALSE; */

    off = -1;
    if (switchOp == JSOP_CONDSWITCH) {
        intN caseNoteIndex = -1;
        JSBool beforeCases = JS_TRUE;

        /* Emit code for evaluating cases and jumping to case statements. */
        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            pn4 = pn3->pn_left;
            if (pn4 && !js_EmitTree(cx, cg, pn4))
                return JS_FALSE;
            if (!pn4) {
                JS_ASSERT(pn3->pn_type == TOK_DEFAULT);
                continue;
            }
            JS_ASSERT(0); /* off = EmitJump(cx, cg, JSOP_CASE, 0); */
            pn3->pn_offset = off;
            if (beforeCases)
                beforeCases = JS_FALSE;
        }

        /* Emit default even if no explicit default statement. */
        JS_ASSERT(0); /* defaultOffset = EmitJump(cx, cg, JSOP_DEFAULT, 0); */
    } else {
        if (switchOp == JSOP_TABLESWITCH) {
            /* Fill in switch bounds, which we know fit in 16-bit offsets. */

            /*
             * Use malloc to avoid arena bloat for programs with many switches.
             * We free table if non-null at label out, so all control flow must
             * exit this function through goto out or goto bad.
             */
            if (tableLength != 0) {
                tableSize = (size_t)tableLength * sizeof *table;
                table = (JSParseNode **) JS_malloc(cx, tableSize);
                if (!table)
                    return JS_FALSE;
                memset(table, 0, tableSize);
                for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
                    if (pn3->pn_type == TOK_DEFAULT)
                        continue;
                    i = JSVAL_TO_INT(pn3->pn_val);
                    i -= low;
                    JS_ASSERT((uint32)i < tableLength);
                    table[i] = pn3;
                }
            }
        } else {
            JS_ASSERT(switchOp == JSOP_LOOKUPSWITCH);

            /* Fill in the number of cases. */
        }

        /*
         * After this point, all control flow involving JSOP_TABLESWITCH
         * must set ok and goto out to exit this function.  To keep things
         * simple, all switchOp cases exit that way.
         */
        if (constPropagated) {
            /*
             * Skip switchOp, as we are not setting jump offsets in the two
             * for loops below.
             */
            if (switchOp == JSOP_TABLESWITCH) {
                for (i = 0; i < (jsint)tableLength; i++) {
                    pn3 = table[i];
                    if (pn3 &&
                        (pn4 = pn3->pn_left) != NULL &&
                        pn4->pn_type == TOK_NAME) {
                        /* Note a propagated constant with the const's name. */
                        JS_ASSERT(!pn4->pn_expr);
                        JS_ASSERT(0);
                    }
                }
            } else {
                for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
                    pn4 = pn3->pn_left;
                    if (pn4 && pn4->pn_type == TOK_NAME) {
                        /* Note a propagated constant with the const's name. */
                        JS_ASSERT(!pn4->pn_expr);
                        JS_ASSERT(0);
                    }
                }
            }
        }
    }

    /* Emit code for each case's statements, copying pn_offset up to pn3. */
    for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
        if (switchOp == JSOP_CONDSWITCH && pn3->pn_type != TOK_DEFAULT)
            CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, pn3->pn_offset);
        pn4 = pn3->pn_right;
        ok = js_EmitTree(cx, cg, pn4);
        if (!ok)
            goto out;
        pn3->pn_offset = pn4->pn_offset;
        if (pn3->pn_type == TOK_DEFAULT)
            off = pn3->pn_offset - top;
    }

    if (!hasDefault) {
        /* If no default case, offset for default is to end of switch. */
        off = CG_OFFSET(cg) - top;
    }

    /* We better have set "off" by now. */
    JS_ASSERT(off != -1);

    /* Set the default offset (to end of switch if no default). */
    if (switchOp == JSOP_CONDSWITCH) {
        JS_ASSERT(0);
    } else {
        JS_ASSERT(0);
    }

    if (switchOp == JSOP_TABLESWITCH) {
        /* Fill in the jump table, if there is one. */
        for (i = 0; i < (jsint)tableLength; i++) {
            pn3 = table[i];
            off = pn3 ? pn3->pn_offset - top : 0;
            JS_ASSERT(0); /* ok = js_SetJumpOffset(cx, cg, pc, off); */
        }
    } else if (switchOp == JSOP_LOOKUPSWITCH) {
        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            if (pn3->pn_type == TOK_DEFAULT)
                continue;
            JS_ASSERT(0); /* SET_ATOM_INDEX(pc, ALE_INDEX(ale)); */

            JS_ASSERT(0); /* js_SetJumpOffset(cx, cg, pc, off); */
        }
    }

out:
    if (table)
        JS_free(cx, table);
    if (ok) {
        ok = js_PopStatementCG(cx, cg);
        if (ok && pn->pn_right->pn_type == TOK_LEXICALSCOPE) {
            JS_ASSERT(0); /* EMIT_UINT16_IMM_OP(JSOP_LEAVEBLOCK, count); */
            cg->stackDepth -= count;
        }
    }
    return ok;

bad:
    ok = JS_FALSE;
    goto out;
}

JSBool
js_EmitFunctionBytecode(JSContext *cx, JSCodeGenerator *cg, JSParseNode *body)
{
    if (cg->treeContext.flags & TCF_FUN_IS_GENERATOR) {
        /* if (js_Emit1(cx, cg, JSOP_GENERATOR) < 0)
            return JS_FALSE; */
    }

    if (!js_EmitTree(cx, cg, body))
        return JS_FALSE;
    JS_ASSERT(0); /* js_Emit1(cx, cg, JSOP_STOP) >= 0 */
    return JS_TRUE;
}

JSBool
js_EmitFunctionBody(JSContext *cx, JSCodeGenerator *cg, JSParseNode *body,
                    JSFunction *fun)
{
    JSStackFrame *fp, frame;
    JSObject *funobj;
    JSBool ok;

    fp = cx->fp;
    funobj = fun->object;
    JS_ASSERT(!fp || (fp->fun != fun && fp->varobj != funobj &&
                      fp->scopeChain != funobj));
    memset(&frame, 0, sizeof frame);
    frame.fun = fun;
    frame.varobj = frame.scopeChain = funobj;
    frame.down = fp;
    frame.flags = JS_HAS_COMPILE_N_GO_OPTION(cx)
                  ? JSFRAME_COMPILING | JSFRAME_COMPILE_N_GO
                  : JSFRAME_COMPILING;
    cx->fp = &frame;
    ok = js_EmitFunctionBytecode(cx, cg, body);
    cx->fp = fp;
    if (!ok)
        return JS_FALSE;

    JS_ASSERT(0); /* if (!js_NewScriptFromCG(cx, cg, fun)) return JS_FALSE; */

    JS_ASSERT(FUN_INTERPRETED(fun));
    if (cg->treeContext.flags & TCF_FUN_HEAVYWEIGHT)
        fun->flags |= JSFUN_HEAVYWEIGHT;
    return JS_TRUE;
}

static JSBool
MaybeEmitVarDecl(JSContext *cx, JSCodeGenerator *cg, JSOp prologOp,
                 JSParseNode *pn, jsatomid *result)
{
    const char *name = js_AtomToPrintableString(cx, pn->pn_atom);
    int traitKind = (prologOp == JSOP_DEFCONST)
                    ? ActionBlockConstants::TRAIT_Const
                    : ActionBlockConstants::TRAIT_Var;
    const char *type = (pn->pn_attrs & JSPROP_INT_TYPE)
                       ? "int"
                       : "*";

    cg->abcEmitter->AddVarOrConst(name, "", traitKind, type);
    return JS_TRUE;
}

static JSBool
EmitVariables(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn,
              JSBool inLetHead)
{
    ActionBlockEmitter *abc = cg->abcEmitter;
    JSTreeContext *tc;
    JSBool let, forInVar;
    JSBool forInLet, popScope;
    JSStmtInfo *stmt, *scopeStmt;
    ptrdiff_t off, noteIndex, tmp;
    JSParseNode *pn2, *pn3;
    JSOp op;
    jsatomid atomIndex;
    uintN oldflags;

    std::vector<std::string> qualifiers;
    qualifiers.push_back("");

    /*
     * Let blocks and expressions have a parenthesized head in which the new
     * scope is not yet open. Initializer evaluation uses the parent node's
     * lexical scope. If popScope is true below, then we hide the top lexical
     * block from any calls to BindNameToSlot hiding in pn2->pn_expr so that
     * it won't find any names in the new let block.
     *
     * The same goes for let declarations in the head of any kind of for loop.
     * Unlike a let declaration 'let x = i' within a block, where x is hoisted
     * to the start of the block, a 'for (let x = i...) ...' loop evaluates i
     * in the containing scope, and puts x in the loop body's scope.
     */
    tc = &cg->treeContext;
    let = (pn->pn_op == JSOP_NOP);
    forInVar = (pn->pn_extra & PNX_FORINVAR) != 0;
    forInLet = let && forInVar;
    popScope = (inLetHead || (let && (tc->flags & TCF_IN_FOR_INIT)));
    JS_ASSERT(!popScope || let);

    off = noteIndex = -1;
    for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
        if (!BindNameToSlot(cx, &cg->treeContext, pn2, let))
            return JS_FALSE;
        JS_ASSERT(pn2->pn_slot >= 0 || !let);

        op = pn2->pn_op;
        if (op == JSOP_ARGUMENTS) {
            /* JSOP_ARGUMENTS => no initializer */
            JS_ASSERT(!pn2->pn_expr && !let);
            pn3 = NULL;
#ifdef __GNUC__
            atomIndex = 0;            /* quell GCC overwarning */
#endif
        } else {
            if (!MaybeEmitVarDecl(cx, cg, pn->pn_op, pn2, &atomIndex))
                return JS_FALSE;

            pn3 = pn2->pn_expr;
            if (pn3) {
#if 0
                /*
                 * If this is a 'for (let x = i in o) ...' let declaration,
                 * throw away i if it is a useless expression.
                 */
                if (forInLet) {
                    JSBool useful = JS_FALSE;

                    JS_ASSERT(pn->pn_count == 1);
                    if (!CheckSideEffects(cx, tc, pn3, &useful))
                        return JS_FALSE;
                    if (!useful)
                        return JS_TRUE;
                }
#endif

                if (op == JSOP_SETNAME) {
                    JS_ASSERT(!let);
                    abc->FindProperty(name, qualifiers, false, false, false);
                }
                if (pn->pn_op == JSOP_DEFCONST &&
                    !js_DefineCompileTimeConstant(cx, cg, pn2->pn_atom,
                                                  pn3)) {
                    return JS_FALSE;
                }

                /* Evaluate expr in the outer lexical scope if requested. */
                if (popScope) {
                    stmt = tc->topStmt;
                    scopeStmt = tc->topScopeStmt;

                    tc->topStmt = stmt->down;
                    tc->topScopeStmt = scopeStmt->downScope;
                }
#ifdef __GNUC__
                else {
                    stmt = scopeStmt = NULL;    /* quell GCC overwarning */
                }
#endif

                oldflags = cg->treeContext.flags;
                cg->treeContext.flags &= ~TCF_IN_FOR_INIT;
                if (!js_EmitTree(cx, cg, pn3))
                    return JS_FALSE;
                cg->treeContext.flags |= oldflags & TCF_IN_FOR_INIT;

                if (popScope) {
                    tc->topStmt = stmt;
                    tc->topScopeStmt = scopeStmt;
                }
            }
        }

        /*
         * 'for (var x in o) ...' and 'for (var x = i in o) ...' call the
         * TOK_VAR case, but only the initialized case (a strange one that
         * falls out of ECMA-262's grammar) wants to run past this point.
         * Both cases must conditionally emit a JSOP_DEFVAR, above.  Note
         * that the parser error-checks to ensure that pn->pn_count is 1.
         *
         * 'for (let x = i in o) ...' must evaluate i before the loop, and
         * subject it to useless expression elimination.  The variable list
         * in pn is a single let declaration if pn_op == JSOP_NOP.  We test
         * the let local in order to break early in this case, as well as in
         * the 'for (var x in o)' case.
         *
         * XXX Narcissus keeps track of variable declarations in the node
         * for the script being compiled, so there's no need to share any
         * conditional prolog code generation there.  We could do likewise,
         * but it's a big change, requiring extra allocation, so probably
         * not worth the trouble for SpiderMonkey.
         */
        JS_ASSERT(pn3 == pn2->pn_expr);
        if (forInVar && (!pn3 || let)) {
            JS_ASSERT(pn->pn_count == 1);
            break;
        }

        if (pn2 == pn->pn_head &&
            !inLetHead &&
            js_NewSrcNote2(cx, cg, SRC_DECL,
                           (pn->pn_op == JSOP_DEFCONST)
                           ? SRC_DECL_CONST
                           : (pn->pn_op == JSOP_DEFVAR)
                           ? SRC_DECL_VAR
                           : SRC_DECL_LET) < 0) {
            return JS_FALSE;
        }
#if 1
        if (op == JSOP_SETNAME)
            abc->SetProperty(name, qualifiers, false, false, false);
        else
            JS_ASSERT(0);
#else
        if (op == JSOP_ARGUMENTS) {
            if (js_Emit1(cx, cg, op) < 0)
                return JS_FALSE;
        } else if (pn2->pn_slot >= 0) {
            EMIT_UINT16_IMM_OP(op, atomIndex);
        } else {
            EMIT_ATOM_INDEX_OP(op, atomIndex);
        }
#endif
    }

    JS_ASSERT(0); /*!(pn->pn_extra & PNX_POPVAR) || js_Emit1(cx, cg, JSOP_POP) >= 0*/;
    return JS_FALSE;
}

static int
JSOpToABCIfKind(JSOp op)
{
    /*
     * Given: if (x == y) foo(); else bar();
     * SM:    x == y; JSOP_IFEQ L1; foo(); goto L2; L1: bar(); L2: ...
     * AVM:   x; y; OP_ne L1; foo(); OP_goto L2; L1: bar(); L2: ...
     */
    switch (op) {
      case JSOP_FALSE:
        return RuntimeConstants::IF_true;
      case JSOP_TRUE:
        return RuntimeConstants::IF_false;
      case JSOP_LT:
        return RuntimeConstants::IF_nlt;
      case JSOP_LE:
        return RuntimeConstants::IF_nle;
      case JSOP_GT:
        return RuntimeConstants::IF_ngt;
      case JSOP_GE:
        return RuntimeConstants::IF_nge;
      case JSOP_EQ:
        return RuntimeConstants::IF_ne;
      case JSOP_NE:
        return RuntimeConstants::IF_eq;
      case JSOP_NEW_EQ:
        return RuntimeConstants::IF_strict_ne;
      case JSOP_NEW_NE:
        return RuntimeConstants::IF_strict_eq;
    }
    return -1;
}

static int
EmitCondition(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn)
{
    ActionBlockEmitter *abc = cg->abcEmitter;
    int kind = JSOpToABCIfKind(pn->pn_op);

    if (kind < 0) {
        if (!js_EmitTree(cx, cg, pn))
            return -1;
        abc->ToBoolean(TYPE_boolean);
        kind = RuntimeConstants::IF_false;
    } else {
        switch (pn->pn_arity) {
          case PN_BINARY:
            if (!js_EmitTree(cx, cg, pn->pn_left))
                return -1;
            if (!js_EmitTree(cx, cg, pn->pn_right))
                return -1;
            break;

          default:
            if (!js_EmitTree(cx, cg, pn))
                return -1;
        }
    }
    abc->If(kind);
    return kind;
}

static int
JSOpToABCOp(JSOp op)
{
    switch (op) {
      case JSOP_BITOR:
        return RuntimeConstants::BitwiseOrOp;
      case JSOP_BITXOR:
        return RuntimeConstants::BitwiseXorOp;
      case JSOP_BITAND:
        return RuntimeConstants::BitwiseAndOp;
      case JSOP_EQ:
        return RuntimeConstants::EqualsOp;
      case JSOP_NE:
        return RuntimeConstants::NotEqualsOp;
      case JSOP_NEW_EQ:
        return RuntimeConstants::StrictEqualsOp;
      case JSOP_NEW_NE:
        return RuntimeConstants::StrictNotEqualsOp;
      case JSOP_LT:
        return RuntimeConstants::LessThanOp;
      case JSOP_LE:
        return RuntimeConstants::LessThanOrEqualOp;
      case JSOP_GT:
        return RuntimeConstants::GreaterThanOp;
      case JSOP_GE:
        return RuntimeConstants::GreaterThanOrEqualOp;
      case JSOP_IN:
        return RuntimeConstants::InOp;
      case JSOP_INSTANCEOF:
        return RuntimeConstants::InstanceOfOp;
      case JSOP_LSH
        return RuntimeConstants::LeftShiftOp;
      case JSOP_RSH
        return RuntimeConstants::RightShiftOp;
      case JSOP_URSH
        return RuntimeConstants::UnsignedRightShiftOp;
      case JSOP_ADD:
        return RuntimeConstants::BinaryPlusOp;
      case JSOP_SUB:
        return RuntimeConstants::BinaryMinusOp;
      case JSOP_MUL:
        return RuntimeConstants::MultiplyOp;
      case JSOP_DIV:
        return RuntimeConstants::DivideOp;
      case JSOP_MOD:
        return RuntimeConstants::ModulusOp;
      case JSOP_TYPEOF:
        return RuntimeConstants::TypeOfOp;
      case JSOP_VOID:
        return RuntimeConstants::VoidOp;
      case JSOP_POS:
        return RuntimeConstants::UnaryPlus;
      case JSOP_NEG:
        return RuntimeConstants::UnaryMinus;
      case JSOP_NOT:
        return RuntimeConstants::LogicalNotOp;
      case JSOP_BITNOT:
        return RuntimeConstants::BitwiseNotOp;
      default:
        JS_ASSERT(0);
    }
}

JSBool
js_EmitTree(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn)
{
    ActionBlockEmitter *abc = cg->abcEmitter;
    JSBool ok;
    JSStmtInfo *stmt, stmtInfo;
    ptrdiff_t top, off, tmp, beq, jmp;
    JSParseNode *pn2, *pn3;
    JSAtom *atom;
    JSAtomListElement *ale;
    jsatomid atomIndex;
    intN noteIndex;
    JSSrcNoteType noteType;
    jsbytecode *pc;
    JSOp op;
    JSTokenType type;
    uint32 argc;
    int stackDummy;

    if (!JS_CHECK_STACK_SIZE(cx, stackDummy)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        return JS_FALSE;
    }

    ok = JS_TRUE;
    cg->emitLevel++;
    pn->pn_offset = top = CG_OFFSET(cg);

    switch (pn->pn_type) {
      case TOK_FUNCTION:
      {
        void *cg2mark;
        JSCodeGenerator *cg2;
        JSFunction *fun;

        /* Generate code for the function's body. */
        cg2mark = JS_ARENA_MARK(&cx->tempPool);
        JS_ARENA_ALLOCATE_TYPE(cg2, JSCodeGenerator, &cx->tempPool);
        if (!cg2) {
            JS_ReportOutOfMemory(cx);
            return JS_FALSE;
        }
        if (!js_InitCodeGenerator(cx, cg2, cg->codePool, cg->notePool,
                                  cg->filename, pn->pn_pos.begin.lineno,
                                  cg->principals)) {
            return JS_FALSE;
        }
        cg2->treeContext.flags = (uint16) (pn->pn_flags | TCF_IN_FUNCTION);
        cg2->treeContext.tryCount = pn->pn_tryCount;
        cg2->parent = cg;
        fun = (JSFunction *) JS_GetPrivate(cx, ATOM_TO_OBJECT(pn->pn_funAtom));
        if (!js_EmitFunctionBody(cx, cg2, pn->pn_body, fun))
            return JS_FALSE;

        /*
         * We need an activation object if an inner peeks out, or if such
         * inner-peeking caused one of our inners to become heavyweight.
         */
        if (cg2->treeContext.flags &
            (TCF_FUN_USES_NONLOCALS | TCF_FUN_HEAVYWEIGHT)) {
            cg->treeContext.flags |= TCF_FUN_HEAVYWEIGHT;
        }
        js_FinishCodeGenerator(cx, cg2);
        JS_ARENA_RELEASE(&cx->tempPool, cg2mark);

        /* Make the function object a literal in the outer script's pool. */
        JS_ASSERT(0);

        /* Emit a bytecode pointing to the closure object in its immediate. */
        if (pn->pn_op != JSOP_NOP) {
            JS_ASSERT(0); /* EMIT_ATOM_INDEX_OP(pn->pn_op, atomIndex) */
        } else {
            JS_ASSERT(0); /* emit prolog op to declare fun in cg */
        }
        break;
      }

      case TOK_IF:
        /* Initialize so we can detect else-if chains and avoid recursion. */
        stmtInfo.type = STMT_IF;
        beq = jmp = -1;
        noteIndex = -1;

        /* Emit code for the condition before pushing stmtInfo. */
        if (EmitCondition(cx, cg, pn->pn_kid1) < 0)
            return JS_FALSE;
        top = CG_OFFSET(cg);
        js_PushStatement(&cg->treeContext, &stmtInfo, STMT_IF, top);

        /* Emit code for the then and optional else parts. */
        if (!js_EmitTree(cx, cg, pn->pn_kid2))
            return JS_FALSE;

        pn3 = pn->pn_kid3;
        if (!pn3) {
            /* No else part, fixup the branch-if-false to come here. */
            abc->PatchIf(abc->getIP());
        } else {
            /* Modify stmtInfo so we know we're in the else part. */
            stmtInfo.type = STMT_ELSE;

            abc->Else();
            abc->PatchIf(abc->getIP());

            if (!js_EmitTree(cx, cg, pn3))
                return JS_FALSE;

            abc->PatchElse(abc->getIP());
        }
        ok = js_PopStatementCG(cx, cg);
        break;

      case TOK_SWITCH:
        /* Out of line to avoid bloating js_EmitTree's stack frame size. */
        ok = EmitSwitch(cx, cg, pn, &stmtInfo);
        break;

      case TOK_WHILE:
        js_PushStatement(&cg->treeContext, &stmtInfo, STMT_WHILE_LOOP, top);

        abc->LoopBegin();
        if (!js_EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;
        abc->PatchLoopBegin(abc->getIP());

        abc->PatchContinue(cg->loopIndex);

        /* Inline expansion of abc->LoopEnd to avoid node argument. */
        kind = EmitCondition(cx, cg, pn->pn_left);
        if (kind < 0)
            return JS_FALSE;
        abc->Emitter::LoopEnd(kind);
        abc->PatchBreak(cg->loopIndex);

        ok = js_PopStatementCG(cx, cg);
        break;

      case TOK_DO:
        JS_ASSERT(0); /* emit an ABC label */

        /* Compile the loop body. */
        top = CG_OFFSET(cg);
        js_PushStatement(&cg->treeContext, &stmtInfo, STMT_DO_LOOP, top);
        if (!js_EmitTree(cx, cg, pn->pn_left))
            return JS_FALSE;

        /* Set loop and enclosing label update offsets, for continue. */
        stmt = &stmtInfo;
        do {
            stmt->update = CG_OFFSET(cg);
        } while ((stmt = stmt->down) != NULL && stmt->type == STMT_LABEL);

        /* Compile the loop condition, now that continues know where to go. */
        if (!js_EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;

        /*
         * No source note needed, because JSOP_IFNE is used only for do-while.
         * If we ever use JSOP_IFNE for other purposes, we can still avoid yet
         * another note here, by storing (jmp - top) in the SRC_WHILE note's
         * offset, and fetching that delta in order to decompile recursively.
         */
        JS_ASSERT(0); /* if (EmitJump(cx, cg, JSOP_IFNE, top - CG_OFFSET(cg)) < 0) return JS_FALSE; */
        ok = js_PopStatementCG(cx, cg);
        break;

      case TOK_FOR:
        beq = 0;                /* suppress gcc warnings */
        pn2 = pn->pn_left;
        js_PushStatement(&cg->treeContext, &stmtInfo, STMT_FOR_LOOP, top);

        if (pn2->pn_type == TOK_IN) {
            JSBool emitIFEQ;

            /* Set stmtInfo type for later testing. */
            stmtInfo.type = STMT_FOR_IN_LOOP;
            noteIndex = -1;

            /*
             * If the left part is 'var x', emit code to define x if necessary
             * using a prolog opcode, but do not emit a pop.  If the left part
             * is 'var x = i', emit prolog code to define x if necessary; then
             * emit code to evaluate i, assign the result to x, and pop the
             * result off the stack.
             *
             * All the logic to do this is implemented in the outer switch's
             * TOK_VAR case, conditioned on pn_extra flags set by the parser.
             *
             * In the 'for (var x = i in o) ...' case, the js_EmitTree(...pn3)
             * called here will generate the proper note for the assignment
             * op that sets x = i, hoisting the initialized var declaration
             * out of the loop: 'var x = i; for (x in o) ...'.
             *
             * In the 'for (var x in o) ...' case, nothing but the prolog op
             * (if needed) should be generated here, we must emit the note
             * just before the JSOP_FOR* opcode in the switch on pn3->pn_type
             * a bit below, so nothing is hoisted: 'for (var x in o) ...'.
             *
             * A 'for (let x = i in o)' loop must not be hoisted, since in
             * this form the let variable is scoped by the loop body (but not
             * the head).  The initializer expression i must be evaluated for
             * any side effects.  So we hoist only i in the let case.
             */
            pn3 = pn2->pn_left;
            type = pn3->pn_type;
            cg->treeContext.flags |= TCF_IN_FOR_INIT;
            if (TOKEN_TYPE_IS_DECL(type) && !js_EmitTree(cx, cg, pn3))
                return JS_FALSE;
            cg->treeContext.flags &= ~TCF_IN_FOR_INIT;

            /* Emit a push to allocate the iterator. */
            JS_ASSERT(0); /* if (js_Emit1(cx, cg, JSOP_STARTITER) < 0) return JS_FALSE; */

            /* Compile the object expression to the right of 'in'. */
            if (!js_EmitTree(cx, cg, pn2->pn_right))
                return JS_FALSE;

            /*
             * Emit a bytecode to convert top of stack value to the iterator
             * object depending on the loop variant (for-in, for-each-in, or
             * destructuring for-in).
             */
            JS_ASSERT(pn->pn_op == JSOP_FORIN || pn->pn_op == JSOP_FOREACH);
            if (js_Emit1(cx, cg, pn->pn_op) < 0)
                return JS_FALSE;

            top = CG_OFFSET(cg);
            SET_STATEMENT_TOP(&stmtInfo, top);

            /*
             * Compile a JSOP_FOR* bytecode based on the left hand side.
             *
             * Initialize op to JSOP_SETNAME in case of |for ([a, b] in o)...|
             * or similar, to signify assignment, rather than declaration, to
             * the decompiler.  EmitDestructuringOps takes a prolog bytecode
             * parameter and emits the appropriate source note, defaulting to
             * assignment, so JSOP_SETNAME is not critical here; many similar
             * ops could be used -- just not JSOP_NOP (which means 'let').
             */
            emitIFEQ = JS_TRUE;
            switch (type) {
              case TOK_LET:
              case TOK_VAR:
                JS_ASSERT(pn3->pn_arity == PN_LIST && pn3->pn_count == 1);
                pn3 = pn3->pn_head;
                /* FALL THROUGH */

              case TOK_NAME:
                if (pn3->pn_slot >= 0) {
                    op = pn3->pn_op;
                    switch (op) {
                      case JSOP_GETARG:   /* FALL THROUGH */
                      case JSOP_SETARG:   op = JSOP_FORARG; break;
                      case JSOP_GETVAR:   /* FALL THROUGH */
                      case JSOP_SETVAR:   op = JSOP_FORVAR; break;
                      case JSOP_GETGVAR:  /* FALL THROUGH */
                      case JSOP_SETGVAR:  op = JSOP_FORNAME; break;
                      case JSOP_GETLOCAL: /* FALL THROUGH */
                      case JSOP_SETLOCAL: op = JSOP_FORLOCAL; break;
                      default:            JS_ASSERT(0);
                    }
                } else {
                    pn3->pn_op = JSOP_FORNAME;
                    if (!BindNameToSlot(cx, &cg->treeContext, pn3, JS_FALSE))
                        return JS_FALSE;
                    op = pn3->pn_op;
                }
                if (pn3->pn_slot >= 0) {
                    if (pn3->pn_attrs & JSPROP_READONLY) {
                        JS_ASSERT(op == JSOP_FORVAR);
                        op = JSOP_GETVAR;
                    }
                    atomIndex = (jsatomid) pn3->pn_slot;
                    EMIT_UINT16_IMM_OP(op, atomIndex);
                } else {
                    if (!EmitAtomOp(cx, pn3, op, cg))
                        return JS_FALSE;
                }
                break;

              case TOK_DOT:
#if 0
                useful = JS_FALSE;
                if (!CheckSideEffects(cx, &cg->treeContext, pn3->pn_expr,
                                      &useful)) {
                    return JS_FALSE;
                }
                if (!useful) {
                    if (!EmitPropOp(cx, pn3, JSOP_FORPROP, cg))
                        return JS_FALSE;
                    break;
                }
#endif
                /* FALL THROUGH */

              case TOK_LB:
                /*
                 * We separate the first/next bytecode from the enumerator
                 * variable binding to avoid any side-effects in the index
                 * expression (e.g., for (x[i++] in {}) should not bind x[i]
                 * or increment i at all).
                 */
                emitIFEQ = JS_FALSE;
                if (!js_Emit1(cx, cg, JSOP_FORELEM))
                    return JS_FALSE;

                /*
                 * Emit a SRC_WHILE note with offset telling the distance to
                 * the loop-closing jump (we can't reckon from the branch at
                 * the top of the loop, because the loop-closing jump might
                 * need to be an extended jump, independent of whether the
                 * branch is short or long).
                 */
                noteIndex = js_NewSrcNote(cx, cg, SRC_WHILE);
                if (noteIndex < 0)
                    return JS_FALSE;
                beq = EmitJump(cx, cg, JSOP_IFEQ, 0);
                if (beq < 0)
                    return JS_FALSE;

                /* Now that we're safely past the IFEQ, commit side effects. */
                if (!EmitElemOp(cx, pn3, JSOP_ENUMELEM, cg))
                    return JS_FALSE;
                break;

              default:
                JS_ASSERT(0);
            }

            if (emitIFEQ) {
                /* Pop and test the loop condition generated by JSOP_FOR*. */
                JS_ASSERT(0); /* beq = EmitJump(cx, cg, JSOP_IFEQ, 0) */;
            }
        } else {
            if (pn2->pn_kid1) {
                cg->treeContext.flags |= TCF_IN_FOR_INIT;
                if (!js_EmitTree(cx, cg, pn3))
                    return JS_FALSE;
                cg->treeContext.flags &= ~TCF_IN_FOR_INIT;
            }

            top = CG_OFFSET(cg);
            SET_STATEMENT_TOP(&stmtInfo, top);
            if (!pn2->pn_kid2) {
                /* No loop condition: flag this fact in the source notes. */
            } else {
                if (!js_EmitTree(cx, cg, pn2->pn_kid2))
                    return JS_FALSE;
                JS_ASSERT(0); /* beq = EmitJump(cx, cg, JSOP_IFEQ, 0) */;
            }

            /* Set pn3 (used below) here to avoid spurious gcc warnings. */
            pn3 = pn2->pn_kid3;
        }

        /* Emit code for the loop body. */
        if (!js_EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;

        if (pn2->pn_type != TOK_IN) {
            if (pn3) {
                /* Set loop and enclosing "update" offsets, for continue. */
                stmt = &stmtInfo;
                do {
                    stmt->update = CG_OFFSET(cg);
                } while ((stmt = stmt->down) != NULL &&
                         stmt->type == STMT_LABEL);

                if (!js_EmitTree(cx, cg, pn3))
                    return JS_FALSE;
            }
        }

        /* Emit the loop-closing jump and fixup all jump offsets. */
        JS_ASSERT(0); /* jmp = EmitJump(cx, cg, JSOP_GOTO, top - CG_OFFSET(cg)) */;
        if (beq > 0)
            JS_ASSERT(0); /* CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, beq) */;

        /* Now fixup all breaks and continues (before for/in's JSOP_ENDITER). */
        if (!js_PopStatementCG(cx, cg))
            return JS_FALSE;

        if (pn2->pn_type == TOK_IN)
            JS_ASSERT(0); /* js_Emit1(cx, cg, JSOP_ENDITER) < 0) return JS_FALSE; */
        break;

      case TOK_BREAK:
        stmt = cg->treeContext.topStmt;
        atom = pn->pn_atom;
        if (atom) {
            while (stmt->type != STMT_LABEL || stmt->atom != atom)
                stmt = stmt->down;
        } else {
            ale = NULL;
            while (!STMT_IS_LOOP(stmt) && stmt->type != STMT_SWITCH)
                stmt = stmt->down;
        }

        abc->Break(stmt->loopIndex);
        break;

      case TOK_CONTINUE:
        stmt = cg->treeContext.topStmt;
        atom = pn->pn_atom;
        if (atom) {
            /* Find the loop statement enclosed by the matching label. */
            JSStmtInfo *loop = NULL;

            while (stmt->type != STMT_LABEL || stmt->atom != atom) {
                if (STMT_IS_LOOP(stmt))
                    loop = stmt;
                stmt = stmt->down;
            }
            stmt = loop;
        } else {
            ale = NULL;
            while (!STMT_IS_LOOP(stmt))
                stmt = stmt->down;
        }

        abc->Continue(stmt->loopIndex);
        break;

      case TOK_WITH:
        if (!js_EmitTree(cx, cg, pn->pn_left))
            return JS_FALSE;
        js_PushStatement(&cg->treeContext, &stmtInfo, STMT_WITH, CG_OFFSET(cg));

        /* XXX more to do -- see with_object_reg, etc. */
        abc->PushWith();
        if (!js_EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;
        abc->PopScope();

        ok = js_PopStatementCG(cx, cg);
        break;

#if 0
      case TOK_TRY:
      {
        ptrdiff_t start, end, catchJump, catchStart, finallyCatch;
        intN depth;
        JSParseNode *lastCatch;

        catchJump = catchStart = finallyCatch = -1;

        /*
         * Push stmtInfo to track jumps-over-catches and gosubs-to-finally
         * for later fixup.
         *
         * When a finally block is 'active' (STMT_FINALLY on the treeContext),
         * non-local jumps (including jumps-over-catches) result in a GOSUB
         * being written into the bytecode stream and fixed-up later (c.f.
         * EmitBackPatchOp and BackPatch).
         */
        js_PushStatement(&cg->treeContext, &stmtInfo,
                         pn->pn_kid3 ? STMT_FINALLY : STMT_TRY,
                         CG_OFFSET(cg));

        /*
         * About JSOP_SETSP: an exception can be thrown while the stack is in
         * an unbalanced state, and this imbalance causes problems with things
         * like function invocation later on.
         *
         * To fix this, we compute the 'balanced' stack depth upon try entry,
         * and then restore the stack to this depth when we hit the first catch
         * or finally block.  We can't just zero the stack, because things like
         * for/in and with that are active upon entry to the block keep state
         * variables on the stack.
         */
        depth = cg->stackDepth;

        /* Mark try location for decompilation, then emit try block. */
        if (js_Emit1(cx, cg, JSOP_TRY) < 0)
            return JS_FALSE;
        start = CG_OFFSET(cg);
        if (!js_EmitTree(cx, cg, pn->pn_kid1))
            return JS_FALSE;

        /* GOSUB to finally, if present. */
        if (pn->pn_kid3) {
            if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                return JS_FALSE;
            jmp = EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, &GOSUBS(stmtInfo));
            if (jmp < 0)
                return JS_FALSE;

            /* JSOP_RETSUB pops the return pc-index, balancing the stack. */
            cg->stackDepth = depth;
        }

        /* Emit (hidden) jump over catch and/or finally. */
        if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
            return JS_FALSE;
        jmp = EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, &catchJump);
        if (jmp < 0)
            return JS_FALSE;

        end = CG_OFFSET(cg);

        /* If this try has a catch block, emit it. */
        pn2 = pn->pn_kid2;
        lastCatch = NULL;
        if (pn2) {
            jsint count = 0;    /* previous catch block's population */

            catchStart = end;

            /*
             * The emitted code for a catch block looks like:
             *
             * [ leaveblock ]                      only if 2nd+ catch block
             * enterblock                          with SRC_CATCH
             * exception
             * setlocalpop <slot>                  or destructuring code
             * [< catchguard code >]               if there's a catchguard
             * [ifeq <offset to next catch block>]         " "
             * < catch block contents >
             * leaveblock
             * goto <end of catch blocks>          non-local; finally applies
             *
             * If there's no catch block without a catchguard, the last
             * <offset to next catch block> points to rethrow code.  This
             * code will [gosub] to the finally code if appropriate, and is
             * also used for the catch-all trynote for capturing exceptions
             * thrown from catch{} blocks.
             */
            for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
                ptrdiff_t guardJump, catchNote;

                guardJump = GUARDJUMP(stmtInfo);
                if (guardJump == -1) {
                    /* Set stack to original depth (see SETSP comment above). */
                    EMIT_UINT16_IMM_OP(JSOP_SETSP, (jsatomid)depth);
                    cg->stackDepth = depth;
                } else {
                    JS_ASSERT(cg->stackDepth == depth);

                    /* Fix up and clean up previous catch block. */
                    CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, guardJump);

                    /* Set cx->throwing to protect cx->exception from the GC. */
                    if (!js_Emit1(cx, cg, JSOP_THROWING) < 0)
                        return JS_FALSE;

                    /*
                     * Emit an unbalanced [leaveblock] for the previous catch,
                     * whose block object count is saved below.
                     */
                    if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                        return JS_FALSE;
                    JS_ASSERT(count >= 0);
                    EMIT_UINT16_IMM_OP(JSOP_LEAVEBLOCK, count);
                }

                /*
                 * Annotate the JSOP_ENTERBLOCK that's about to be generated
                 * by the call to js_EmitTree immediately below.  Save this
                 * source note's index in stmtInfo for use by the TOK_CATCH:
                 * case, where the length of the catch guard is set as the
                 * note's offset.
                 */
                catchNote = js_NewSrcNote2(cx, cg, SRC_CATCH, 0);
                if (catchNote < 0)
                    return JS_FALSE;
                CATCHNOTE(stmtInfo) = catchNote;

                /*
                 * Emit the lexical scope and catch body.  Save the catch's
                 * block object population via count, for use when targeting
                 * guardJump at the next catch (the guard mismatch case).
                 */
                JS_ASSERT(pn3->pn_type == TOK_LEXICALSCOPE);
                count = OBJ_BLOCK_COUNT(cx, ATOM_TO_OBJECT(pn3->pn_atom));
                if (!js_EmitTree(cx, cg, pn3))
                    return JS_FALSE;

                /* gosub <finally>, if required */
                if (pn->pn_kid3) {
                    jmp = EmitBackPatchOp(cx, cg, JSOP_BACKPATCH,
                                          &GOSUBS(stmtInfo));
                    if (jmp < 0)
                        return JS_FALSE;
                    JS_ASSERT(cg->stackDepth == depth);
                }

                /*
                 * Jump over the remaining catch blocks.  This will get fixed
                 * up to jump to after catch/finally.
                 */
                if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                    return JS_FALSE;
                jmp = EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, &catchJump);
                if (jmp < 0)
                    return JS_FALSE;

                /*
                 * Save a pointer to the last catch node to handle try-finally
                 * and try-catch(guard)-finally special cases.
                 */
                lastCatch = pn3->pn_expr;
            }
        }

        /*
         * We emit a [setsp][gosub] sequence for running finally code while
         * letting an uncaught exception pass thrown from within the try in a
         * try-finally.  The [gosub] and [retsub] opcodes will take care of
         * stacking and rethrowing any exception pending across the finally.
         *
         * For rethrowing after a try-catch(guard)-finally, we have a problem:
         * all the guards have mismatched, leaving cx->exception still set but
         * cx->throwing clear, so that no exception appears to be pending for
         * [gosub] to stack and [retsub] to rethrow.  We must emit a special
         * [throwing] opcode in front of the [setsp][gosub] finally sequence.
         * This opcode will restore cx->throwing to true before running the
         * finally.
         *
         * For rethrowing after a try-catch(guard) without a finally, we emit
         * [throwing] before the [setsp][exception][throw] rethrow sequence.
         */
        if (pn->pn_kid3 || (lastCatch && lastCatch->pn_kid2)) {
            /*
             * Last catch guard jumps to the rethrow code sequence if none
             * of the guards match.  Target guardJump at the beginning of the
             * rethrow sequence, just in case a guard expression throws and
             * leaves the stack unbalanced.
             */
            if (lastCatch && lastCatch->pn_kid2) {
                CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, GUARDJUMP(stmtInfo));
                if (pn->pn_kid3 && !js_Emit1(cx, cg, JSOP_THROWING) < 0)
                    return JS_FALSE;
            }

            /*
             * Emit another stack fixup, because the catch could itself
             * throw an exception in an unbalanced state, and the finally
             * may need to call functions.  If there is no finally, only
             * guarded catches, the rethrow code below nevertheless needs
             * stack fixup.
             */
            finallyCatch = CG_OFFSET(cg);
            EMIT_UINT16_IMM_OP(JSOP_SETSP, (jsatomid)depth);
            cg->stackDepth = depth;

            if (pn->pn_kid3) {
                jmp = EmitBackPatchOp(cx, cg, JSOP_BACKPATCH,
                                      &GOSUBS(stmtInfo));
                if (jmp < 0)
                    return JS_FALSE;

                JS_ASSERT(cg->stackDepth == depth);
                JS_ASSERT((uintN)depth <= cg->maxStackDepth);
            } else {
                if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0 ||
                    js_Emit1(cx, cg, JSOP_EXCEPTION) < 0 ||
                    js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0 ||
                    js_Emit1(cx, cg, JSOP_THROW) < 0) {
                    return JS_FALSE;
                }
            }
        }

        /*
         * If we have a finally, it belongs here, and we have to fix up the
         * gosubs that might have been emitted before non-local jumps.
         */
        if (pn->pn_kid3) {
            if (!BackPatch(cx, cg, GOSUBS(stmtInfo), CG_NEXT(cg), JSOP_GOSUB))
                return JS_FALSE;

            /*
             * The stack budget must be balanced at this point.  All [gosub]
             * calls emitted before this point will push two stack slots, one
             * for the pending exception (or JSVAL_HOLE if there is no pending
             * exception) and one for the [retsub] pc-index.
             */
            JS_ASSERT(cg->stackDepth == depth);
            cg->stackDepth += 2;
            if ((uintN)cg->stackDepth > cg->maxStackDepth)
                cg->maxStackDepth = cg->stackDepth;

            /* Now indicate that we're emitting a subroutine body. */
            stmtInfo.type = STMT_SUBROUTINE;
            if (!UpdateLineNumberNotes(cx, cg, pn->pn_kid3))
                return JS_FALSE;
            if (js_Emit1(cx, cg, JSOP_FINALLY) < 0 ||
                !js_EmitTree(cx, cg, pn->pn_kid3) ||
                js_Emit1(cx, cg, JSOP_RETSUB) < 0) {
                return JS_FALSE;
            }

            /* Restore stack depth budget to its balanced state. */
            JS_ASSERT(cg->stackDepth == depth + 2);
            cg->stackDepth = depth;
        }
        if (!js_PopStatementCG(cx, cg))
            return JS_FALSE;

        if (js_NewSrcNote(cx, cg, SRC_ENDBRACE) < 0 ||
            js_Emit1(cx, cg, JSOP_NOP) < 0) {
            return JS_FALSE;
        }

        /* Fix up the end-of-try/catch jumps to come here. */
        if (!BackPatch(cx, cg, catchJump, CG_NEXT(cg), JSOP_GOTO))
            return JS_FALSE;

        /*
         * Add the try note last, to let post-order give us the right ordering
         * (first to last for a given nesting level, inner to outer by level).
         */
        if (pn->pn_kid2) {
            JS_ASSERT(end != -1 && catchStart != -1);
            if (!js_NewTryNote(cx, cg, start, end, catchStart))
                return JS_FALSE;
        }

        /*
         * If we've got a finally, mark try+catch region with additional
         * trynote to catch exceptions (re)thrown from a catch block or
         * for the try{}finally{} case.
         */
        if (pn->pn_kid3) {
            JS_ASSERT(finallyCatch != -1);
            if (!js_NewTryNote(cx, cg, start, finallyCatch, finallyCatch))
                return JS_FALSE;
        }
        break;
      }

      case TOK_CATCH:
      {
        ptrdiff_t catchStart, guardJump;

        /*
         * Morph STMT_BLOCK to STMT_CATCH, note the block entry code offset,
         * and save the block object atom.
         */
        stmt = cg->treeContext.topStmt;
        JS_ASSERT(stmt->type == STMT_BLOCK && (stmt->flags & SIF_SCOPE));
        stmt->type = STMT_CATCH;
        catchStart = stmt->update;
        atom = stmt->atom;

        /* Go up one statement info record to the TRY or FINALLY record. */
        stmt = stmt->down;
        JS_ASSERT(stmt->type == STMT_TRY || stmt->type == STMT_FINALLY);

        /* Pick up the pending exception and bind it to the catch variable. */
        if (js_Emit1(cx, cg, JSOP_EXCEPTION) < 0)
            return JS_FALSE;
        pn2 = pn->pn_kid1;
        switch (pn2->pn_type) {
          case TOK_NAME:
            /* Inline BindNameToSlot, adding block depth to pn2->pn_slot. */
            pn2->pn_slot += OBJ_BLOCK_DEPTH(cx, ATOM_TO_OBJECT(atom));
            EMIT_UINT16_IMM_OP(JSOP_SETLOCALPOP, pn2->pn_slot);
            break;

          default:
            JS_ASSERT(0);
        }

        /* Emit the guard expression, if there is one. */
        if (pn->pn_kid2) {
            if (!js_EmitTree(cx, cg, pn->pn_kid2))
                return JS_FALSE;
            if (!js_SetSrcNoteOffset(cx, cg, CATCHNOTE(*stmt), 0,
                                     CG_OFFSET(cg) - catchStart)) {
                return JS_FALSE;
            }
            /* ifeq <next block> */
            guardJump = EmitJump(cx, cg, JSOP_IFEQ, 0);
            if (guardJump < 0)
                return JS_FALSE;
            GUARDJUMP(*stmt) = guardJump;
        }

        /* Emit the catch body. */
        if (!js_EmitTree(cx, cg, pn->pn_kid3))
            return JS_FALSE;

        /*
         * Annotate the JSOP_LEAVEBLOCK that will be emitted as we unwind via
         * our TOK_LEXICALSCOPE parent, so the decompiler knows to pop.
         */
        off = cg->stackDepth;
        if (js_NewSrcNote2(cx, cg, SRC_CATCH, off) < 0)
            return JS_FALSE;
        break;
      }
#endif

      case TOK_VAR:
        if (!EmitVariables(cx, cg, pn, JS_FALSE))
            return JS_FALSE;
        break;

      case TOK_RETURN:
        /* Push a return value */
        pn2 = pn->pn_kid;
        if (pn2) {
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
        }

        abc->Return(pn2 ? TYPE_any : TYPE_void);
        break;

      case TOK_LC:
        JS_ASSERT(pn->pn_arity == PN_LIST);
        js_PushStatement(&cg->treeContext, &stmtInfo, STMT_BLOCK, top);
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
        }
        ok = js_PopStatementCG(cx, cg);
        break;

      case TOK_SEMI:
        pn2 = pn->pn_kid;
        if (pn2) {
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
        }
        break;

      case TOK_COLON:
        /* Emit an annotated nop so we know to decompile a label. */
        atom = pn->pn_atom;
        ale = js_IndexAtom(cx, atom, &cg->atomList);
        if (!ale)
            return JS_FALSE;
        js_PushStatement(&cg->treeContext, &stmtInfo, STMT_LABEL,
                         CG_OFFSET(cg));
        stmtInfo.atom = atom;
        if (!js_EmitTree(cx, cg, pn->pn_expr))
            return JS_FALSE;
        if (!js_PopStatementCG(cx, cg))
            return JS_FALSE;
        break;

      case TOK_COMMA:
        /*
         * Emit SRC_PCDELTA notes on each JSOP_POP between comma operands.
         * These notes help the decompiler bracket the bytecodes generated
         * from each sub-expression that follows a comma.
         */
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
        }
        break;

      case TOK_ASSIGN:
        /*
         * Check left operand type and generate specialized code for it.
         * Specialize to avoid ECMA "reference type" values on the operand
         * stack, which impose pervasive runtime "GetValue" costs.
         */
        pn2 = pn->pn_left;
        JS_ASSERT(pn2->pn_type != TOK_RP);
        atomIndex = (jsatomid) -1;              /* quell GCC overwarning */

        const char *name = js_AtomToPrintableString(cx, pn->pn_atom);
        std::vector<std::string> qualifiers;
        qualifiers.push_back("");

        switch (pn2->pn_type) {
          case TOK_NAME:
            if (!BindNameToSlot(cx, &cg->treeContext, pn2, JS_FALSE))
                return JS_FALSE;
            if (pn2->pn_slot >= 0) {
                atomIndex = (jsatomid) pn2->pn_slot;
            } else {
                abc->FindProperty(name, qualifiers, false, false, false);
            }
            break;
          case TOK_DOT:
            if (!js_EmitTree(cx, cg, pn2->pn_expr))
                return JS_FALSE;
            break;
          case TOK_LB:
            JS_ASSERT(pn2->pn_arity == PN_BINARY);
            if (!js_EmitTree(cx, cg, pn2->pn_left))
                return JS_FALSE;
            if (!js_EmitTree(cx, cg, pn2->pn_right))
                return JS_FALSE;
            break;
          default:
            JS_ASSERT(0);
        }

        op = pn->pn_op;
#if 1
        JS_ASSERT(0);
#else
        if (op == JSOP_GETTER || op == JSOP_SETTER) {
            /* We'll emit these prefix bytecodes after emitting the r.h.s. */
        } else if (op != JSOP_NOP) {
            /* If += or similar, dup the left operand and get its value. */
            switch (pn2->pn_type) {
              case TOK_NAME:
                if (pn2->pn_op != JSOP_SETNAME) {
                    XXX /*EMIT_UINT16_IMM_OP((pn2->pn_op == JSOP_SETGVAR)
                                       ? JSOP_GETGVAR
                                       : (pn2->pn_op == JSOP_SETARG)
                                       ? JSOP_GETARG
                                       : (pn2->pn_op == JSOP_SETLOCAL)
                                       ? JSOP_GETLOCAL
                                       : JSOP_GETVAR,
                                       atomIndex)*/;
                    break;
                }
                /* FALL THROUGH */
              case TOK_DOT:
                XXX /*if (js_Emit1(cx, cg, JSOP_DUP) < 0) return JS_FALSE*/;
                XXX /*EMIT_ATOM_INDEX_OP((pn2->pn_type == TOK_NAME)
                                   ? JSOP_GETXPROP
                                   : JSOP_GETPROP,
                                   atomIndex)*/;
                break;
              case TOK_LB:
                XXX /*if (js_Emit1(cx, cg, JSOP_DUP2) < 0) return JS_FALSE*/;
                XXX /*if (js_Emit1(cx, cg, JSOP_GETELEM) < 0) return JS_FALSE*/;
                break;
              default:;
            }
        }
#endif

        /* Now emit the right operand (it may affect the namespace). */
        if (!js_EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;

        /* If += etc., emit the binary operator with a decompiler note. */
        if (op != JSOP_NOP) {
            JS_ASSERT(0) /*if (js_Emit1(cx, cg, op) < 0) return JS_FALSE*/;
        }

        /* Finally, emit the specialized assignment bytecode. */
        switch (pn2->pn_type) {
          case TOK_NAME:
            if (pn2->pn_slot < 0 || !(pn2->pn_attrs & JSPROP_READONLY)) {
                if (pn2->pn_slot >= 0) {
                    JS_ASSERT(0) /*EMIT_UINT16_IMM_OP(pn2->pn_op, atomIndex)*/;
                } else {
          case TOK_DOT:
                    abc->SetProperty(name, qualifiers, false, false, false);
                }
            }
            break;
          case TOK_LB:
            JS_ASSERT(0) /*if (js_Emit1(cx, cg, JSOP_SETELEM) < 0) return JS_FALSE*/;
            break;
          default:
            JS_ASSERT(0);
        }
        break;

#if 0
      case TOK_HOOK:
        /* Emit the condition, then branch if false to the else part. */
        if (!js_EmitTree(cx, cg, pn->pn_kid1))
            return JS_FALSE;
        XXX /*beq = EmitJump(cx, cg, JSOP_IFEQ, 0)*/;
        if (!js_EmitTree(cx, cg, pn->pn_kid2))
            return JS_FALSE;

        /* Jump around else, fixup the branch, emit else, fixup jump. */
        XXX /*jmp = EmitJump(cx, cg, JSOP_GOTO, 0)*/;
        XXX /*CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, beq)*/;

        /*
         * Because each branch pushes a single value, but our stack budgeting
         * analysis ignores branches, we now have to adjust cg->stackDepth to
         * ignore the value pushed by the first branch.  Execution will follow
         * only one path, so we must decrement cg->stackDepth.
         *
         * Failing to do this will foil code, such as the try/catch/finally
         * exception handling code generator, that samples cg->stackDepth for
         * use at runtime (JSOP_SETSP), or in let expression and block code
         * generation, which must use the stack depth to compute local stack
         * indexes correctly.
         */
        JS_ASSERT(cg->stackDepth > 0);
        cg->stackDepth--;
        if (!js_EmitTree(cx, cg, pn->pn_kid3))
            return JS_FALSE;
        XXX /*CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, jmp)*/;
        break;
#endif

      case TOK_OR:
      case TOK_AND:
        if (!js_EmitTree(cx, cg, pn->pn_left))
            return JS_FALSE;
        abc->Dup();
        abc->ToBoolean(TYPE_any);
        abc->Emitter::If((pn->pn_type == TOK_AND)
                         ? RuntimeConstants::IF_false
                         : RuntimeConstants::IF_true);
        abc->Pop();
        if (!js_EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;
        abc->PatchIf(abc->getIP());
        break;

      case TOK_BITOR:
      case TOK_BITXOR:
      case TOK_BITAND:
      case TOK_EQOP:
      case TOK_RELOP:
      case TOK_IN:
      case TOK_INSTANCEOF:
      case TOK_SHOP:
      case TOK_PLUS:
      case TOK_MINUS:
      case TOK_STAR:
      case TOK_DIVOP:
        int op_index = JSOpToABCOp(pn->pn_op);
        if (pn->pn_arity == PN_LIST) {
            /* Left-associative operator chain: avoid too much recursion. */
            pn2 = pn->pn_head;
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
            while ((pn2 = pn2->pn_next) != NULL) {
                if (!js_EmitTree(cx, cg, pn2))
                    return JS_FALSE;
                abc->InvokeBinary(op_index);
            }
        } else {

            /* Binary operators that evaluate both operands unconditionally. */
            if (!js_EmitTree(cx, cg, pn->pn_left))
                return JS_FALSE;
            if (!js_EmitTree(cx, cg, pn->pn_right))
                return JS_FALSE;
            abc->InvokeBinary(op_index);
        }
        break;

#if 0
      case TOK_THROW:
#endif
      case TOK_UNARYOP:
      {
        uintN oldflags;

        /* Unary op, including unary +/-. */
        pn2 = pn->pn_kid;
        op = pn->pn_op;
        if (op == JSOP_TYPEOF) {
            for (pn3 = pn2; pn3->pn_type == TOK_RP; pn3 = pn3->pn_kid)
                continue;
            if (pn3->pn_type != TOK_NAME)
                op = JSOP_TYPEOFEXPR;
        }
        oldflags = cg->treeContext.flags;
        cg->treeContext.flags &= ~TCF_IN_FOR_INIT;
        if (!js_EmitTree(cx, cg, pn2))
            return JS_FALSE;
        cg->treeContext.flags |= oldflags & TCF_IN_FOR_INIT;

        int op_index = JSOpToABCOp(op);
        abc->InvokeBinary(op_index, 0, 0, NULL);
        break;
      }

      case TOK_INC:
      case TOK_DEC:
      {
        intN depth;

        /* Emit lvalue-specialized code for ++/-- operators. */
        pn2 = pn->pn_kid;
        JS_ASSERT(pn2->pn_type != TOK_RP);
        op = pn->pn_op;
        depth = cg->stackDepth;
        switch (pn2->pn_type) {
          case TOK_NAME:
            pn2->pn_op = op;
            if (!BindNameToSlot(cx, &cg->treeContext, pn2, JS_FALSE))
                return JS_FALSE;
            op = pn2->pn_op;
            if (pn2->pn_slot >= 0) {
                if (pn2->pn_attrs & JSPROP_READONLY) {
                    /* Incrementing a declared const: just get its value. */
                    op = ((js_CodeSpec[op].format & JOF_TYPEMASK) == JOF_CONST)
                         ? JSOP_GETGVAR
                         : JSOP_GETVAR;
                }
                atomIndex = (jsatomid) pn2->pn_slot;
                EMIT_UINT16_IMM_OP(op, atomIndex);
            } else {
                if (!EmitAtomOp(cx, pn2, op, cg))
                    return JS_FALSE;
            }
            break;
          case TOK_DOT:
            if (!EmitPropOp(cx, pn2, op, cg))
                return JS_FALSE;
            ++depth;
            break;
          case TOK_LB:
            if (!EmitElemOp(cx, pn2, op, cg))
                return JS_FALSE;
            depth += 2;
            break;
          default:
            JS_ASSERT(0);
        }

        /*
         * Allocate another stack slot for GC protection in case the initial
         * value being post-incremented or -decremented is not a number, but
         * converts to a jsdouble.  In the TOK_NAME cases, op has 0 operand
         * uses and 1 definition, so we don't need an extra stack slot -- we
         * can use the one allocated for the def.
         */
        if (pn2->pn_type != TOK_NAME &&
            (js_CodeSpec[op].format & JOF_POST) &&
            (uintN)depth == cg->maxStackDepth) {
            ++cg->maxStackDepth;
        }
        break;
      }

      case TOK_DELETE:
        /*
         * Under ECMA 3, deleting a non-reference returns true -- but alas we
         * must evaluate the operand if it appears it might have side effects.
         */
        pn2 = pn->pn_kid;
        switch (pn2->pn_type) {
          case TOK_NAME:
            pn2->pn_op = JSOP_DELNAME;
            if (!BindNameToSlot(cx, &cg->treeContext, pn2, JS_FALSE))
                return JS_FALSE;
            op = pn2->pn_op;
            if (op == JSOP_FALSE) {
                abc->PushBoolean(false);
            } else {
                if (!EmitAtomOp(cx, pn2, op, cg))
                    return JS_FALSE;
            }
            break;
          case TOK_DOT:
            if (!EmitPropOp(cx, pn2, JSOP_DELPROP, cg))
                return JS_FALSE;
            break;
          case TOK_LB:
            if (!EmitElemOp(cx, pn2, JSOP_DELELEM, cg))
                return JS_FALSE;
            break;
          default:
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
            abc->PushBoolean(false);
        }
        break;

      case TOK_DOT:
        /*
         * Pop a stack operand, convert it to object, get a property named by
         * this bytecode's immediate-indexed atom operand, and push its value
         * (not a reference to it).  This bytecode sets the virtual machine's
         * "obj" register to the left operand's ToObject conversion result,
         * for use by JSOP_PUSHOBJ.
         */
        ok = EmitPropOp(cx, pn, pn->pn_op, cg);
        break;

      case TOK_LB:
        /*
         * Pop two operands, convert the left one to object and the right one
         * to property name (atom or tagged int), get the named property, and
         * push its value.  Set the "obj" register to the result of ToObject
         * on the left operand.
         */
        ok = EmitElemOp(cx, pn, pn->pn_op, cg);
        break;

#if 0
      case TOK_NEW:
#endif
      case TOK_LP:
      {
        const char *name;
        uintN oldflags;

        std::vector<std::string> qualifiers;
        qualifiers.push_back("");

        /*
         * Emit function call or operator new (constructor call) code.
         * First, emit code for the left operand to evaluate the callable or
         * constructable object expression.
         *
         * For E4X, if this expression is a dotted member reference, select
         * JSOP_GETMETHOD instead of JSOP_GETPROP.  ECMA-357 separates XML
         * method lookup from the normal property id lookup done for native
         * objects.
         */
        pn2 = pn->pn_head;
        switch (pn2->pn_type) {
          case TOK_NAME:
            name = js_AtomToPrintableString(cx, pn2->pn_atom);
            abc->FindProperty(name, qualifiers, false, false, false);
            break;
          default:
            JS_ASSERT(0);
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
        }

        /*
         * Emit code for each argument in order, then emit the JSOP_*CALL or
         * JSOP_NEW bytecode with a two-byte immediate telling how many args
         * were pushed on the operand stack.
         */
        oldflags = cg->treeContext.flags;
        cg->treeContext.flags &= ~TCF_IN_FOR_INIT;
        for (pn2 = pn2->pn_next; pn2; pn2 = pn2->pn_next) {
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
        }
        cg->treeContext.flags |= oldflags & TCF_IN_FOR_INIT;

        argc = pn->pn_count - 1;
        abc->CallProperty(name, qualifiers, argc, false, false, false, false);
        break;
      }

#if 0
      case TOK_LEXICALSCOPE:
      {
        JSObject *obj;
        jsint count;

        atom = pn->pn_atom;
        obj = ATOM_TO_OBJECT(atom);
        js_PushBlockScope(&cg->treeContext, &stmtInfo, atom, CG_OFFSET(cg));

        OBJ_SET_BLOCK_DEPTH(cx, obj, cg->stackDepth);
        count = OBJ_BLOCK_COUNT(cx, obj);
        cg->stackDepth += count;
        if ((uintN)cg->stackDepth > cg->maxStackDepth)
            cg->maxStackDepth = cg->stackDepth;

        XXX /*EMIT_ATOM_INDEX_OP(JSOP_ENTERBLOCK, ALE_INDEX(ale))*/;

        if (!js_EmitTree(cx, cg, pn->pn_expr))
            return JS_FALSE;

        /* Emit the JSOP_LEAVEBLOCK or JSOP_LEAVEBLOCKEXPR opcode. */
        XXX /*EMIT_UINT16_IMM_OP(pn->pn_op, count)*/;
        cg->stackDepth -= count;

        ok = js_PopStatementCG(cx, cg);
        break;
      }

      case TOK_LET:
        /* Let statements have their variable declarations on the left. */
        if (pn->pn_arity == PN_BINARY) {
            pn2 = pn->pn_right;
            pn = pn->pn_left;
        } else {
            pn2 = NULL;
        }

        /* Non-null pn2 means that pn is the variable list from a let head. */
        JS_ASSERT(pn->pn_arity == PN_LIST);
        if (!EmitVariables(cx, cg, pn, pn2 != NULL))
            return JS_FALSE;

        /* Thus non-null pn2 is the body of the let block or expression. */
        if (pn2 && !js_EmitTree(cx, cg, pn2))
            return JS_FALSE;
        break;
#endif

      case TOK_RB:
        /*
         * Emit code for [a, b, c] of the form:
         *   t = new Array; t[0] = a; t[1] = b; t[2] = c; t;
         * but use a stack slot for t and avoid dup'ing and popping it via
         * the JSOP_NEWINIT and JSOP_INITELEM bytecodes.
         */
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (pn2->pn_type == TOK_COMMA) {
                abc->PushUndefined();
            } else {
                if (!js_EmitTree(cx, cg, pn2))
                    return JS_FALSE;
            }
        }

        abc->NewArray(pn->pn_cont);
        break;

      case TOK_RC:
        /*
         * Emit code for {p:a, '%q':b, 2:c} of the form:
         *   t = new Object; t.p = a; t['%q'] = b; t[2] = c; t;
         * but use a stack slot for t and avoid dup'ing and popping it via
         * the JSOP_NEWINIT and JSOP_INITELEM bytecodes.
         */
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            /* Emit an index for t[2], else map an atom for t.p or t['%q']. */
            pn3 = pn2->pn_left;
            switch (pn3->pn_type) {
              case TOK_NUMBER:
                if (!EmitNumberOp(cx, pn3->pn_dval, cg))
                    return JS_FALSE;
                break;
              case TOK_NAME:
              case TOK_STRING:
                abc->PushString(js_AtomToPrintableString(cx, pn3->pn_atom));
                break;
              default:
                JS_ASSERT(0);
            }

            /* Emit code for the property initializer. */
            if (!js_EmitTree(cx, cg, pn2->pn_right))
                return JS_FALSE;
        }

        abc->NewObject(pn->pn_count);
        break;

      case TOK_RP:
      {
        uintN oldflags;

        /*
         * The node for (e) has e as its kid, enabling users who want to nest
         * assignment expressions in conditions to avoid the error correction
         * done by Condition (from x = y to x == y) by double-parenthesizing.
         */
        oldflags = cg->treeContext.flags;
        cg->treeContext.flags &= ~TCF_IN_FOR_INIT;
        if (!js_EmitTree(cx, cg, pn->pn_kid))
            return JS_FALSE;
        cg->treeContext.flags |= oldflags & TCF_IN_FOR_INIT;
        break;
      }

      case TOK_NAME:
        if (!BindNameToSlot(cx, &cg->treeContext, pn, JS_FALSE))
            return JS_FALSE;
        /* FALL THROUGH */

      case TOK_STRING:
      case TOK_OBJECT:
        /*
         * The scanner and parser associate JSOP_NAME with TOK_NAME, although
         * other bytecodes may result instead (JSOP_BINDNAME/JSOP_SETNAME,
         * JSOP_FORNAME, etc.).  Among JSOP_*NAME* variants, only JSOP_NAME
         * may generate the first operand of a call or new expression, so only
         * it sets the "obj" virtual machine register to the object along the
         * scope chain in which the name was found.
         *
         * Token types for STRING and OBJECT have corresponding bytecode ops
         * in pn_op and emit the same format as NAME, so they share this code.
         */
        ok = EmitAtomOp(cx, pn, pn->pn_op, cg);
        break;

      case TOK_NUMBER:
        ok = EmitNumberOp(cx, pn->pn_dval, cg);
        break;

      case TOK_PRIMARY:
        switch (pn->pn_op) {
          case JSOP_NULL:
            abc->PushNull();
            break;
          case JSOP_FALSE:
            abc->PushBoolean(false);
            break;
          case JSOP_TRUE:
            abc->PushBoolean(true);
            break;
          case JSOP_THIS:
            abc->PushThis();
            break;
          default:
            JS_ASSERT(0);
        }
        break;

      default:
        JS_ASSERT(0);
    }

    if (ok && --cg->emitLevel == 0 && cg->spanDeps)
        ok = OptimizeSpanDeps(cx, cg);

    return ok;
}
