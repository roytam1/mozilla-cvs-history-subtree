/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is mozilla.org.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Mark Hammond (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "PyXPCOM.h"

static PyObject *g_obFuncMakeInterface = NULL;

PyObject *PyObject_FromDOMnsISupports(PyObject *pycontext, nsISupports *pis,
                                      PRBool bMakeNicePyObject = PR_TRUE);

struct PyMethodDef 
Py_DOMnsISupports_Methods[] =
{
	{NULL}
};

class Py_DOMnsISupports : public Py_nsISupports
{
public:
	static PyXPCOM_TypeObject *type;
	static void InitType() {
		type = new PyXPCOM_TypeObject(
				"DOMISupports",
				Py_nsISupports::type,
				sizeof(Py_DOMnsISupports),
				Py_DOMnsISupports_Methods,
				nsnull);
		const nsIID &iid = NS_GET_IID(nsISupports);
		// Don't RegisterInterface!
		// RegisterInterface(iid, type);
	}
	//virtual PyObject *getattr(const char *name);
	//virtual int setattr(const char *name, PyObject *val);
	virtual PyObject *MakeInterfaceResult(nsISupports *ps, const nsIID &iid,
	                                      PRBool bMakeNicePyObject = PR_TRUE)
	{
		if (!iid.Equals(NS_GET_IID(nsIClassInfo)))
			return PyObject_FromDOMnsISupports(m_pycontext, ps, bMakeNicePyObject);
		return Py_nsISupports::MakeInterfaceResult(ps, iid, bMakeNicePyObject);
	}

	Py_DOMnsISupports(PyObject *pycontext, nsISupports *p, const nsIID &iid) :
		Py_nsISupports(p, iid, type),
		m_pycontext(pycontext) {
		/* The IID _must_ be the IID of the interface we are wrapping! */
		//NS_ABORT_IF_FALSE(iid.Equals(NS_GET_IID(InterfaceName)), "Bad IID");
		Py_INCREF(pycontext);
	}
	virtual ~Py_DOMnsISupports() {
		Py_DECREF(m_pycontext);
	}
	static PyObject *MakeDefaultWrapper(PyObject *pycontext,
	                                    PyObject *pyis, const nsIID &iid);
protected:
	PyObject *m_pycontext;
};


PyObject *PyObject_FromDOMnsISupports(PyObject *pycontext, nsISupports *pis,
                                      PRBool bMakeNicePyObject /* = PR_TRUE */)
{
	if (pis==NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	nsIID iid = NS_GET_IID(nsISupports);
	Py_DOMnsISupports *ret = new Py_DOMnsISupports(pycontext, pis, iid);
	pis->AddRef();
	if (ret && bMakeNicePyObject)
		return Py_DOMnsISupports::MakeDefaultWrapper(pycontext, ret, iid);
	return ret;
}

// Call back into the Python context
PyObject *
Py_DOMnsISupports::MakeDefaultWrapper(PyObject *pycontext,
                                      PyObject *pyis, 
                                      const nsIID &iid)
{
	NS_PRECONDITION(pyis, "NULL pyobject!");
	PyObject *obIID = NULL;
	PyObject *args = NULL;
	PyObject *mod = NULL;
	PyObject *ret = NULL;

	obIID = Py_nsIID::PyObjectFromIID(iid);
	if (obIID==NULL)
		goto done;

	if (g_obFuncMakeInterface==NULL) {
		PyObject *mod = PyImport_ImportModule("nsdom.context");
		if (mod) 
			g_obFuncMakeInterface = PyObject_GetAttrString(mod, "MakeInterfaceResult");
		Py_XDECREF(mod);
	}
	if (g_obFuncMakeInterface==NULL) goto done;

	args = Py_BuildValue("OOO", pycontext, pyis, obIID);
	if (args==NULL) goto done;
	ret = PyEval_CallObject(g_obFuncMakeInterface, args);
done:
	if (PyErr_Occurred()) {
		NS_ABORT_IF_FALSE(ret==NULL, "Have an error, but also a return val!");
		PyXPCOM_LogError("Creating an interface object to be used as a result failed\n");
		PyErr_Clear();
	}
	Py_XDECREF(mod);
	Py_XDECREF(args);
	Py_XDECREF(obIID);
	if (ret==NULL) // eek - error - return the original with no refcount mod.
		ret = pyis; 
	else
		// no error - decref the old object
		Py_DECREF(pyis);
	// return our obISupports.  If NULL, we are really hosed and nothing we can do.
	return ret;
}

void PyInit_DOMnsISupports()
{
	Py_DOMnsISupports::InitType();
}
PyXPCOM_TypeObject *Py_DOMnsISupports::type = NULL;
//PyXPCOM_INTERFACE_DEFINE(Py_DOMnsISupports, ?, PyMethods_DOMnsISupports)
