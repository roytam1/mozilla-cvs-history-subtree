/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 *
 * The Initial Developer is
 * Ben Bucksch <http://www.bucksch.org>
 * of Beonex <http://www.beonex.com>
 *
 * The contents of this file are too trivial to have copyright protection
 * and are thus in the public domain.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsIGenericFactory.h"

#include "mozSRoaming.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(mozSRoaming)

static const nsModuleComponentInfo components[] =
{
  { "Session Roaming",
    NS_SESSIONROAMING_CID,
    NS_SESSIONROAMING_CONTRACTID,
    mozSRoamingConstructor },
};


NS_IMPL_NSGETMODULE(mozSRoamingModule, components)
