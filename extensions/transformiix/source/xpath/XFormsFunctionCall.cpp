/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is TransforMiiX XSLT processor code.
 *
 * The Initial Developer of the Original Code is
 * The MITRE Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Aaron Reed <aaronr@us.ibm.com> (Original Author)
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

/*
 * XFormsFunctionCall
 * A representation of the XPath NodeSet funtions
 */

#include "FunctionLib.h"
#include "nsAutoPtr.h"
#include "txNodeSet.h"
#include "txAtoms.h"
#include "txIXPathContext.h"
#include "txTokenizer.h"
#include "XFormsFunctions.h"
#include <math.h>
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMElement.h"
#include "nsIXFormsUtilitiesService.h"
#include "nsServiceManagerUtils.h"  // needed for do_GetService?

/*
 * Creates a XFormsFunctionCall of the given type
 */
XFormsFunctionCall::XFormsFunctionCall(XFormsFunctions aType, nsIDOMNode *aResolverNode)
    : mType(aType)
    , mResolverNode(aResolverNode)
{
}

/*
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ContextState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
 */
nsresult
XFormsFunctionCall::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
  *aResult = nsnull;
  nsresult rv = NS_OK;
  txListIterator iter(&params);

  switch (mType) {
    case AVG:
    {
      nsRefPtr<txNodeSet> nodes;
      nsresult rv = evaluateToNodeSet((Expr*)iter.next(), aContext,
                                      getter_AddRefs(nodes));
      NS_ENSURE_SUCCESS(rv, rv);
   
      double res = 0;
      PRInt32 i;
      for (i = 0; i < nodes->size(); ++i) {
        nsAutoString resultStr;
        txXPathNodeUtils::appendNodeValue(nodes->get(i), resultStr);
        res += Double::toDouble(resultStr);
      }
   
      if (nodes->size() > 0)
      {
        res = (res/(nodes->size()));
      }
      else
      {
        res = Double::NaN;
      }
      return aContext->recycler()->getNumberResult(res, aResult);
    }
    case BOOLEANFROMSTRING:
    {
      if (!requireParams(1, 1, aContext))
        return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;

      PRInt32 retvalue = -1;
      nsAutoString booleanValue;
      if (iter.hasNext()) {
        evaluateToString((Expr*)iter.next(), aContext, booleanValue);

        if (!booleanValue.IsEmpty()) {
          if (booleanValue.EqualsLiteral("1")) {
            aContext->recycler()->getBoolResult(PR_TRUE, aResult);
          }
          else if (booleanValue.EqualsLiteral("0")) {
            aContext->recycler()->getBoolResult(PR_FALSE, aResult);
          }
          else if (booleanValue.LowerCaseEqualsLiteral("true")) {
            aContext->recycler()->getBoolResult(PR_TRUE, aResult);
          }
          else if (booleanValue.LowerCaseEqualsLiteral("false")) {
            aContext->recycler()->getBoolResult(PR_FALSE, aResult);
          }
          else {
            // if not "0","1", "true", or "false" (case insensitive), then send 
            //   return value will be false.  This per the XForms errata
            //   dated 2004/10/15
            aContext->recycler()->getBoolResult(PR_FALSE, aResult);
          }
        }

      }
      return NS_OK;
    }
    case COUNTNONEMPTY:
    {
      nsRefPtr<txNodeSet> nodes;
      nsresult rv = evaluateToNodeSet((Expr*)iter.next(), aContext,
                                      getter_AddRefs(nodes));
      NS_ENSURE_SUCCESS(rv, rv);
   
      double res = 0, test = 0;
      PRInt32 i, count=0;
      for (i = 0; i < nodes->size(); ++i) {
        nsAutoString resultStr;
        txXPathNodeUtils::appendNodeValue(nodes->get(i), resultStr);
        if (!resultStr.IsEmpty())
        {
          count++;
        }
      }
   
      return aContext->recycler()->getNumberResult(count, aResult);
    }
    case DAYSFROMDATE:
    {
      if (!requireParams(1, 1, aContext))
        return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;
   
#if 0
      // first make sure that the parameter is a proper xsd:dateTime or
      //   xsd:date
      nsAutoString dateTime;
      if (iter.hasNext()) {
        evaluateToString((Expr*)iter.next(), aContext, dateTime);
   
        if (!duration.IsEmpty()) {
          nsCOMPtr<nsIXFormsUtilitiesService>xformsService = 
                do_GetService("@mozilla.org/xforms-utility-service;1", &rv);

          if (xformsService) {
            nsAutoString dateTimeType(NS_LITERAL_STRING("dateTime"));
            PRBool isDateTime, isDate;
            nsAutoString schemaNS(NS_LITERAL_STRING(NS_NAMESPACE_SCHEMA));
            xformsService->ValidateString(dateTime, dateTimeType, 
                                          schemaNS, &isDateTime);
            if (!isDateTime) {
              nsAutoString dateType(NS_LITERAL_STRING("date"));
              xformsService->ValidateString(dateTime, dateType, 
                                            schemaNS, &isDate);
              if (!isDate) {
                return aContext->recycler()->getNumberResult(Double::NaN, aResult);
              }
            }

            // XXX TODO up to this point, it will work.  This is where we need
            //   to probably call into the XForms service and have it do the work
            //   of calling the schema utility function to parse the dateTime
            //   and have it convert the seconds, minutes, etc. to seconds
            //   since it is desirous to not have XPATH require the schema 
            //   validator stuff.
          }
        }
      }
      return aContext->recycler()->getNumberResult(res, aResult);
#endif
      return NS_ERROR_NOT_IMPLEMENTED;
    }
    case IF:
    {
      if (!requireParams(3, 3, aContext))
        return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;
   
      PRBool test;
      nsAutoString valueToReturn;
      if (iter.hasNext()) {
        test = evaluateToBoolean((Expr*)iter.next(), aContext);

        // grab 'true' value to return
        Expr *getvalue = (Expr*)iter.next();
   
        if (!test) {
          // grab 'false' value to return
          getvalue = (Expr*)iter.next();
        }
        evaluateToString(getvalue, aContext, valueToReturn);
      }
   
      return aContext->recycler()->getStringResult(valueToReturn, aResult);
    }
    case INDEX:
    {
      // Given an element's id as the parameter, need to query the element and 
      //   make sure that it is a xforms:repeat node.  Given that, must query 
      //   its index.
      if (!requireParams(1, 1, aContext))
          return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;
   
#if 0
      // XXX TODO This code is all done other than figuring out how to get the
      //   current index from the repeat element.  We'll use the xforms
      //   utility service to get that information for us from the repeat
      //   node that we will pass it.
      nsAutoString instanceId;
      if (iter.hasNext())
      {
        evaluateToString((Expr*)iter.next(), aContext, instanceId);
   
        if (!instanceId.IsEmpty())
        {
          // here document is the XForms document
          nsCOMPtr<nsIDOMDocument> document;
          rv = mResolverNode->GetOwnerDocument(
                                           getter_AddRefs(document));
          NS_ENSURE_SUCCESS(rv, rv);
   
          nsCOMPtr<nsIDOMElement> repeatEle;
          rv = document->GetElementById(instanceId, 
                                        getter_AddRefs(repeatEle));
   
          // make sure that repeatEle really IS a xforms:repeat
          //   element.
          if (repeatEle)
          {
            nsAutoString localname, namespaceURI;
            
            nsCOMPtr<nsIDOMNode> node(do_QueryInterface(repeatEle));
            PRInt32 currIndex = 1;
            node->GetLocalName(localname);
            if (localname.EqualsLiteral("repeat")) {
              node->GetNamespaceURI(namespaceURI);
              if (namespaceURI.EqualsLiteral(NS_NAMESPACE_XFORMS)) {
                // XXX TODO Get current index from the repeat element and
                //   return it.  Probably have to do this inside the
                //   xforms utility service.
   
                return aContext->recycler()->getNumberResult(currIndex, aResult);
              }
            }
             
          }
   
          // if not a repeat element, send xforms-compute-exception
          //   event to the model as per spec
          nsCOMPtr<nsIDOMEvent> event;
          nsCOMPtr<nsIDOMDocumentEvent> doc = do_QueryInterface(document);
          doc->CreateEvent(NS_LITERAL_STRING("Events"), 
                                getter_AddRefs(event));
          NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);
   
          event->InitEvent(NS_LITERAL_STRING("xforms-compute-exception"),
                           PR_TRUE, PR_FALSE);
   
          // find the model to send the event to.  We'll send it to the
          //   resolver's model.  
          //   XXX TODO Better double check this once this function works.
          nsCOMPtr<nsIDOMNode> modelNode;
          nsCOMPtr<nsIXFormsUtilitiesService>xformsService = 
                do_GetService("@mozilla.org/xforms-utility-service;1", &rv);
          NS_ENSURE_SUCCESS(rv, rv);
   
          rv = xformsService->GetModelFromNode(mResolverNode, 
                                               getter_AddRefs(modelNode));
          NS_ENSURE_SUCCESS(rv, rv);
          nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(modelNode);
          PRBool cancelled;
          return target->DispatchEvent(event, &cancelled);
        }
      }
   
      return NS_OK;
#endif /* if 0 */
      return NS_ERROR_NOT_IMPLEMENTED;
    }
    case INSTANCE:
    {
      //XXX XFORMSTODO
      nsresult rv;
      if (!requireParams(1, 1, aContext))
        return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;
   
      // mResolverNode is the node in the XForms document that contained
      //   the expression we are evaluating.  We'll use this to get the
      //   document.  If this isn't here, then something is wrong. Bail.
      if (!mResolverNode)
      {
        return NS_ERROR_FAILURE;
      }
   
      nsRefPtr<txNodeSet> resultSet;
      rv = aContext->recycler()->getNodeSet(getter_AddRefs(resultSet));
      NS_ENSURE_SUCCESS(rv, rv);
   
      nsAutoString instanceId;
      if (iter.hasNext())
      {
        evaluateToString((Expr*)iter.next(), aContext, instanceId);
   
        if (!instanceId.IsEmpty())
        {
          // here document is the XForms document
          nsCOMPtr<nsIDOMDocument> document;
          rv = mResolverNode->GetOwnerDocument(
                                           getter_AddRefs(document));
          NS_ENSURE_SUCCESS(rv, rv);
   
          nsCOMPtr<nsIDOMElement> instEle;
          rv = document->GetElementById(instanceId, 
                                        getter_AddRefs(instEle));
   
          PRBool foundInstance = PR_FALSE;
          nsAutoString localname, namespaceURI;
          if (instEle)
            instEle->GetLocalName(localname);
          if (!localname.IsEmpty() && localname.EqualsLiteral("instance")) {
            instEle->GetNamespaceURI(namespaceURI);
            if (!namespaceURI.IsEmpty() && 
                namespaceURI.EqualsLiteral(NS_NAMESPACE_XFORMS)) {
                foundInstance = PR_TRUE;
            }
          }
   
          if (!foundInstance) {
            // We found a node, but it
            //   return it.
            *aResult = resultSet;
            NS_ADDREF(*aResult);
        
            return NS_OK;
          }
   
          // Make sure that this element is contained in the same
          //   model as the context node of the expression as per
          //   the XForms 1.0 spec.
   
          // first step is to get the contextNode passed in to
          //   the evaluation
   
          const txXPathNode& xpNode = aContext->getContextNode();
          nsCOMPtr<nsIDOMNode> xfContextNode;
          rv = txXPathNativeNode::getNode(xpNode, 
                                     getter_AddRefs(xfContextNode));
          NS_ENSURE_SUCCESS(rv, rv);
   
          // now see if the node we found (instEle) and the 
          //   context node for the evaluation (xfContextNode) link
          //   back to the same model. 
          nsCOMPtr<nsIXFormsUtilitiesService>xformsService = 
                do_GetService("@mozilla.org/xforms-utility-service;1", &rv);
          NS_ENSURE_SUCCESS(rv, rv);
   
          nsCOMPtr<nsIDOMNode> instNode, modelInstance;
          instNode = do_QueryInterface(instEle);
          rv = xformsService->GetModelFromNode(instNode, 
                                     getter_AddRefs(modelInstance));
          NS_ENSURE_SUCCESS(rv, rv);
   
          PRBool modelContainsNode = PR_FALSE;
          rv = xformsService->IsThisModelAssocWithThisNode(
                                     modelInstance, 
                                     xfContextNode,
                                     &modelContainsNode);
          NS_ENSURE_SUCCESS(rv, rv);
   
          if (modelContainsNode)
          {
            // ok, we've found an instance node with the proper id
            //   that fulfills the requirement of being from the
            //   same model as the context node.  Now we need to
            //   return a 'node-set containing just the root
            //   element node of the referenced instance data'.
            //   Wonderful.
   
              nsCOMPtr<nsIDOMNode> instanceRoot;
              rv = xformsService->GetInstanceDocumentRoot(
                                    instanceId,
                                    modelInstance,
                                    getter_AddRefs(instanceRoot));
              NS_ENSURE_SUCCESS(rv, rv);
   
              if(instanceRoot) {
                nsAutoPtr<txXPathNode> txNode(txXPathNativeNode::createXPathNode(instanceRoot));
                if (txNode) {
                  resultSet->add(*txNode);
              }
            }
          }
   
   
            // XXX where we need to do the work
           // if (walker.moveToElementById(instanceId)) {
           //     resultSet->add(walker.getCurrentPosition());
           // }
        }
      }
   
      *aResult = resultSet;
      NS_ADDREF(*aResult);
   
      return NS_OK;
    }
    case MAX:
    {
      nsRefPtr<txNodeSet> nodes;
      nsresult rv = evaluateToNodeSet((Expr*)iter.next(), aContext,
                                      getter_AddRefs(nodes));
      NS_ENSURE_SUCCESS(rv, rv);
   
      double res = Double::NaN, test = 0;
      PRInt32 i;
      for (i = 0; i < nodes->size(); ++i) {
        nsAutoString resultStr;
        txXPathNodeUtils::appendNodeValue(nodes->get(i), resultStr);
        test = Double::toDouble(resultStr);
        if (Double::isNaN(test))
        {
          res = Double::NaN;
          break;
        }
        if ((test > res) || (i==0))
        {
          res = test;
        }
      }
   
      return aContext->recycler()->getNumberResult(res, aResult);
    }
    case MIN:
    {
      nsRefPtr<txNodeSet> nodes;
      nsresult rv = evaluateToNodeSet((Expr*)iter.next(), aContext,
                                      getter_AddRefs(nodes));
      NS_ENSURE_SUCCESS(rv, rv);
   
      double res = Double::NaN, test = 0;
      PRInt32 i;
      for (i = 0; i < nodes->size(); ++i) {
        nsAutoString resultStr;
        txXPathNodeUtils::appendNodeValue(nodes->get(i), resultStr);
        test = Double::toDouble(resultStr);
        if (Double::isNaN(test))
        {
          res = Double::NaN;
        break;
        }
        if ((test < res) || (i==0))
        {
          res = test;
        }
      }
   
      return aContext->recycler()->getNumberResult(res, aResult);
    }
    case MONTHS:
    {
      if (!requireParams(1, 1, aContext))
        return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;
   
#if 0
      // first make sure that the parameter is a propert xsd:duration
      nsAutoString duration;
      if (iter.hasNext()) {
        evaluateToString((Expr*)iter.next(), aContext, duration);
   
        if (!duration.IsEmpty()) {
          nsCOMPtr<nsIXFormsUtilitiesService>xformsService = 
                do_GetService("@mozilla.org/xforms-utility-service;1", &rv);

          if (xformsService) {
            nsAutoString durationType(NS_LITERAL_STRING("duration"));
            PRBool isDuration;
            nsAutoString schemaNS(NS_LITERAL_STRING(NS_NAMESPACE_SCHEMA));
            xformsService->ValidateString(duration, durationType, 
                                      schemaNS, &isDuration);

            // XXX TODO up to this point, it will work.  This is where we need
            //   to probably call into the XForms service and have it do the work
            //   of calling the schema utility function to parse the duration
            //   and have it convert the seconds, minutes, etc. to seconds
            //   since it is desirous to not have XPATH require the schema 
            //   validator stuff.  The below parsing doesn't work but shouldn't
            //   be too far off of the mark if we ever need it.
            if (isDuration) {
            }
            else {
              dbl = Double::NaN;
            }
          }
        }
      }
      return aContext->recycler()->getNumberResult(res, aResult);
#endif
      return NS_ERROR_NOT_IMPLEMENTED;
    }
    case NOW:
    {
      PRExplodedTime time;
      char ctime[60];
   
      PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &time);
      int gmtoffsethour = time.tm_params.tp_gmt_offset < 0 ? -1*time.tm_params.tp_gmt_offset / 3600 : time.tm_params.tp_gmt_offset / 3600;
      int remainder = time.tm_params.tp_gmt_offset%3600;
      int gmtoffsetminute = remainder ? remainder/60 : 00;
      char zone_location[20];
      sprintf(zone_location, "%c%02d:%02d\0", 
              time.tm_params.tp_gmt_offset < 0 ? '-' : '+',
              gmtoffsethour, gmtoffsetminute);
   
      PR_FormatTime(ctime, sizeof(ctime), "%Y-%m-%dT%H:%M:%S\0", &time);
      strcat(ctime, zone_location);
      NS_ConvertASCIItoUTF16 localtime(ctime);
   
      return aContext->recycler()->getStringResult(localtime, aResult);
    }
    case PROPERTY:
    {
      if (!requireParams(1, 1, aContext))
        return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;
   
      nsAutoString property;
      if (iter.hasNext()) {
        evaluateToString((Expr*)iter.next(), aContext, property);
   
        if (!property.IsEmpty()) {
          // This function can handle "version" and "conformance-level"
          //   which is all that the XForms 1.0 spec is worried about
          if (property.Equals(NS_LITERAL_STRING("version")))
            property.Assign(NS_LITERAL_STRING("1.0"));
          else if (property.Equals(NS_LITERAL_STRING("conformance-level")))
            property.Assign(NS_LITERAL_STRING("basic"));
        }
      }
   
      return aContext->recycler()->getStringResult(property, aResult);
    }
    case SECONDS:
    {
      double dbl=0;
      if (!requireParams(1, 1, aContext))
          return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;
   
#if 0
      // first make sure that the parameter is a propert xsd:duration
      nsAutoString duration;
      if (iter.hasNext()) {
        evaluateToString((Expr*)iter.next(), aContext, duration);
   
        if (!duration.IsEmpty()) {
          nsCOMPtr<nsIXFormsUtilitiesService>xformsService = 
                do_GetService("@mozilla.org/xforms-utility-service;1", &rv);

          if (xformsService) {
            nsAutoString durationType(NS_LITERAL_STRING("duration"));
            PRBool isDuration;
            nsAutoString schemaNS(NS_LITERAL_STRING(NS_NAMESPACE_SCHEMA));
            xformsService->ValidateString(duration, durationType, 
                                      schemaNS, &isDuration);

            // XXX TODO up to this point, it will work.  This is where we need
            //   to probably call into the XForms service and have it do the work
            //   of calling the schema utility function to parse the duration
            //   and have it convert the seconds, minutes, etc. to seconds
            //   since it is desirous to not have XPATH require the schema 
            //   validator stuff.  The below parsing doesn't work but shouldn't
            //   be too far off of the mark if we ever need it.
            if (isDuration) {
              int seconds = 0;
              xformsService->SecondsFromDuration(duration, &seconds);
              nsAString::const_iterator start, end;
              nsAString::const_iterator substart, subend;
              nsString seconds;
              nsString minutes;
              nsString hours;
              nsString days;
              duration.BeginReading(start);
              duration.EndReading(end);

              while(end != start) {
                --end;
                if( *end == 'S' ) {
                  subend = end;
                  substart = end;
                  while( (*substart>='0') && (*substart<='9') ) {
                    --substart;
                  }
                  end = substart;
                  ++substart;
                  seconds = Substring(substart, subend);
                } /* seconds */
                else if( *end == 'M' ) {
                  subend = end;
                  substart = end;
                  while( (*substart>='0') && (*substart<='9') ) {
                    --substart;
                  }
                  end = substart;
                  ++substart;
                  seconds = Substring(substart, subend);
                } /* minutes */
                else if( *end == 'H' ) {
                  subend = end;
                  substart = end;
                  while( (*substart>='0') && (*substart<='9') ) {
                    --substart;
                  }
                  end = substart;
                  ++substart;
                  seconds = Substring(substart, subend);
                } /* hours */
                else if( *end == 'D' ) {
                  subend = end;
                  substart = end;
                  while( (*substart>='0') && (*substart<='9') ) {
                    --substart;
                  }
                  end = substart;
                  ++substart;
                  seconds = Substring(substart, subend);
                } /* days */
              }
            }
            else {
              dbl = Double::NaN;
            }
          }
        }
      }
      return aContext->recycler()->getNumberResult(dbl, aResult);
#endif /* if 0 */
      return NS_ERROR_NOT_IMPLEMENTED;
   
    }
    case SECONDSFROMDATETIME:
    {
      if (!requireParams(1, 1, aContext))
          return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;
   
#if 0
      // first make sure that the parameter is a proper xsd:dateTime
      nsAutoString dateTime;
      if (iter.hasNext()) {
        evaluateToString((Expr*)iter.next(), aContext, dateTime);
   
        if (!duration.IsEmpty()) {
          nsCOMPtr<nsIXFormsUtilitiesService>xformsService = 
                do_GetService("@mozilla.org/xforms-utility-service;1", &rv);

          if (xformsService) {
            nsAutoString dateTimeType(NS_LITERAL_STRING("dateTime"));
            PRBool isDateTime;
            nsAutoString schemaNS(NS_LITERAL_STRING(NS_NAMESPACE_SCHEMA));
            xformsService->ValidateString(dateTime, dateTimeType, 
                                          schemaNS, &isDateTime);
            if (isDateTime) {
              // XXX TODO up to this point, it will work.  This is where we need
              //   to probably call into the XForms service and have it do the work
              //   of calling the schema utility function to parse the dateTime
              //   and have it convert the seconds, minutes, etc. to seconds
              //   since it is desirous to not have XPATH require the schema 
              //   validator stuff.
            }
            else {
              dbl = Double::NaN;
            }
          }
        }
      }
      return aContext->recycler()->getNumberResult(dbl, aResult);
#endif
    }
  } /* switch() */

  aContext->receiveError(NS_LITERAL_STRING("Internal error"),
                         NS_ERROR_UNEXPECTED);
  return NS_ERROR_UNEXPECTED;
}

#ifdef TX_TO_STRING
nsresult
XFormsFunctionCall::getNameAtom(nsIAtom** aAtom)
{
  switch (mType) {
    case AVG:
    {
      *aAtom = txXPathAtoms::avg;
      break;
    }
    case BOOLEANFROMSTRING:
    {
      *aAtom = txXPathAtoms::booleanFromString;
      break;
    }
    case COUNTNONEMPTY:
    {
      *aAtom = txXPathAtoms::countNonEmpty;
      break;
    }
    case DAYSFROMDATE:
    {
      *aAtom = txXPathAtoms::daysFromDate;
      break;
    }
    case IF:
    {
      *aAtom = txXPathAtoms::ifFunc;
      break;
    }
    case INDEX:
    {
      *aAtom = txXPathAtoms::index;
      break;
    }
    case INSTANCE:
    {
      *aAtom = txXPathAtoms::instance;
      break;
    }
    case MAX:
    {
      *aAtom = txXPathAtoms::max;
      break;
    }
    case MIN:
    {
      *aAtom = txXPathAtoms::min;
      break;
    }
    case MONTHS:
    {
      *aAtom = txXPathAtoms::months;
      break;
    }
    case NOW:
    {
      *aAtom = txXPathAtoms::now;
      break;
    }
    case PROPERTY:
    {
      *aAtom = txXPathAtoms::property;
      break;
    }
    case SECONDS:
    {
      *aAtom = txXPathAtoms::seconds;
      break;
    }
    case SECONDSFROMDATETIME:
    {
      *aAtom = txXPathAtoms::secondsFromDateTime;
      break;
    }
    default:
    {
      *aAtom = 0;
      return NS_ERROR_FAILURE;
    }
  }
  NS_ADDREF(*aAtom);
  return NS_OK;
}
#endif
