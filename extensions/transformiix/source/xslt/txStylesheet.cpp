#include "txStylesheet.h"
#include "Expr.h"
#include "txXSLTPatterns.h"
#include "txToplevelItems.h"
#include "txInstructions.h"
#include "primitives.h"

txStylesheet::txStylesheet()
    : mNamedTemplates(PR_FALSE),
      mDecimalFormats(PR_TRUE),
      mAttributeSets(PR_TRUE)
{
}

nsresult
txStylesheet::init()
{
    mRootFrame = new ImportFrame;
    NS_ENSURE_TRUE(mRootFrame, NS_ERROR_OUT_OF_MEMORY);
    
    return NS_OK;
}

txStylesheet::~txStylesheet()
{
    // Delete all ImportFrames
    txListIterator frameIter(&mImportFrames);
    while (frameIter.hasNext()) {
        delete (ImportFrame*)frameIter.next();
    }

    txListIterator instrIter(&mTemplateInstructions);
    while (instrIter.hasNext()) {
        delete (txInstruction*)instrIter.next();
    }
}

txInstruction*
txStylesheet::findTemplate(Node* aNode,
                           const txExpandedName& aMode,
                           txIMatchContext* aContext,
                           ImportFrame* aImportedBy,
                           ImportFrame** aImportFrame)
{
    NS_ASSERTION(aImportFrame, "missing ImportFrame pointer");
    NS_ASSERTION(aNode, "missing node");

    txInstruction* matchTemplate = 0;
    ImportFrame* endFrame = 0;
    txListIterator frameIter(&mImportFrames);

    if (aImportedBy) {
        ImportFrame* curr = (ImportFrame*)frameIter.next();
        while (curr != aImportedBy)
               curr = (ImportFrame*)frameIter.next();

        endFrame = aImportedBy->mFirstNotImported;
    }

#ifdef PR_LOGGING
    txPattern match = 0;
#endif

    ImportFrame* frame;
    while (!matchTemplate &&
           (frame = (ImportFrame*)frameIter.next()) &&
           frame != endFrame) {

        // get templatelist for this mode
        txList* templates;
        templates = (txList*)frame->mMatchableTemplates.get(aMode);

        if (templates) {
            txListIterator templateIter(templates);

            // Find template with highest priority
            MatchableTemplate* templ;
            while (!matchTemplate &&
                   (templ = (MatchableTemplate*)templateIter.next())) {
                if (templ->mMatch->matches(aNode, aContext)) {
                    matchTemplate = templ->mFirstInstruction;
                    *aImportFrame = frame;
#ifdef PR_LOGGING
                    match = templ->mMatch;
#endif
                }
            }
        }
    }

#ifdef PR_LOGGING
    String mode;
    if (aMode.mLocalName) {
        TX_GET_ATOM_STRING(aMode.mLocalName, mode);
    }
    if (matchTemplate) {
        String matchAttr;
        match->toString(matchAttr);
        // matchTemplate can be a document (see addLREStylesheet)
        PR_LOG(txLog::xslt, PR_LOG_DEBUG,
               ("MatchTemplate, Pattern %s, Mode %s, Node %s\n",
                NS_LossyConvertUCS2toASCII(matchAttr).get(),
                NS_LossyConvertUCS2toASCII(mode).get(),
                NS_LossyConvertUCS2toASCII(aNode->getNodeName()).get()));
    }
    else {
        PR_LOG(txLog::xslt, PR_LOG_DEBUG,
               ("No match, Node %s, Mode %s\n", 
                NS_LossyConvertUCS2toASCII(aNode->getNodeName()).get(),
                NS_LossyConvertUCS2toASCII(mode).get()));
    }
#endif
    
    // XXX get default template

    return matchTemplate;
}

txDecimalFormat*
txStylesheet::getDecimalFormat(const txExpandedName& aName)
{
    return (txDecimalFormat*)mDecimalFormats.get(aName);
}

txInstruction*
txStylesheet::getAttributeSet(const txExpandedName& aName)
{
    return (txInstruction*)mAttributeSets.get(aName);
}

txOutputFormat*
txStylesheet::getOutputFormat()
{
    return &mOutputFormat;
}


nsresult
txStylesheet::doneCompiling()
{
    nsresult rv = NS_OK;
    // Collect all importframes into a single ordered list
    rv = mImportFrames.add(mRootFrame);
    NS_ENSURE_SUCCESS(rv, rv);
    
    mRootFrame = nsnull;
    
    // XXX traverse tree of importframes and fill mImportFrames

    // Loop through importframes and process all items
    txListIterator frameIter(&mImportFrames);
    ImportFrame* frame;
    while ((frame = (ImportFrame*)frameIter.next())) {
        txListIterator itemIter(&frame->mToplevelItems);
        itemIter.resetToEnd();
        txToplevelItem* item;
        while ((item = (txToplevelItem*)itemIter.previous())) {
            switch (item->getType()) {
                case txToplevelItem::dummy:
                case txToplevelItem::import:
                {
                    delete item;
                }
                case txToplevelItem::output:
                {
                    mOutputFormat.merge(((txOutputItem*)item)->mFormat);
                    delete item;
                }
                case txToplevelItem::templ:
                {
                    rv = addTemplate((txTemplateItem*)item, frame);
                    NS_ENSURE_SUCCESS(rv, rv);

                    delete item;
                }
            }
        }
        itemIter.remove(); //remove() moves to the previous
        itemIter.next();
    }

    return NS_OK;
}

// XXX This one isn't OOM safe.
nsresult
txStylesheet::addTemplate(txTemplateItem* aTemplate,
                          ImportFrame* aImportFrame)
{
    NS_ASSERTION(aTemplate, "missing template");

    txInstruction* instr = aTemplate->mFirstInstruction;
    nsresult rv = mTemplateInstructions.add(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    // mTemplateInstructions now owns the instructions
    aTemplate->mFirstInstruction = 0;

    rv = NS_OK;
    if (!aTemplate->mName.isNull()) {
        rv = mNamedTemplates.add(aTemplate->mName, instr);
        NS_ENSURE_TRUE(NS_SUCCEEDED(rv) || rv == NS_ERROR_XSLT_ALREADY_SET,
                       rv);
    }

    if (!aTemplate->mMatch) {
        // This is no error, see section 6 Named Templates
        delete aTemplate;

        return NS_OK;
    }

    // get the txList for the right mode
    txList* templates =
        (txList*)aImportFrame->mMatchableTemplates.get(aTemplate->mMode);

    if (!templates) {
        templates = new txList;
        if (!templates) {
            delete aTemplate;
            return NS_ERROR_OUT_OF_MEMORY;
        }

        rv = aImportFrame->mMatchableTemplates.add(aTemplate->mMode,
                                                   templates);
        if (NS_FAILED(rv)) {
            delete templates;
            delete aTemplate;
            return rv;
        }
    }

    // Add the simple patterns to the list of matchable templates, according
    // to default priority
    txList simpleMatches;
    txPattern* pattern = aTemplate->mMatch;
    aTemplate->mMatch->getSimplePatterns(simpleMatches);
    txListIterator simples(&simpleMatches);
    while (simples.hasNext()) {
        txPattern* simple = (txPattern*)simples.next();
        if (simple != pattern && pattern) {
            // txUnionPattern, it doesn't own the txLocPathPatterns no more,
            // so delete it. (only once, of course)
            delete pattern;
            pattern = 0;
        }
        double priority = aTemplate->mPrio;
        if (Double::isNaN(priority)) {
            priority = simple->getDefaultPriority();
            NS_ASSERTION(!Double::isNaN(priority),
                         "simple pattern without default priority");
        }
        MatchableTemplate* nt = new MatchableTemplate(instr, simple, priority);

        txListIterator templ(templates);
        MBool added = MB_FALSE;
        while (templ.hasNext()) {
            MatchableTemplate* mt = (MatchableTemplate*)templ.next();
            if (priority > mt->mPriority) {
                templ.addBefore(nt);
                added = MB_TRUE;
                break;
            }
        }
        if (!added) {
            templates->add(nt);
        }
    }

    return NS_OK;
}

nsresult
txStylesheet::addKey(const txExpandedName& aName,
                     txPattern* aMatch, Expr* aUse)
{
    // XXX implement me
    delete aMatch;
    delete aUse;
    return NS_OK;
}

txStylesheet::ImportFrame::ImportFrame()
    : mMatchableTemplates(MB_TRUE),
      mFirstNotImported(nsnull)
{
}

txStylesheet::ImportFrame::~ImportFrame()
{
    // Delete templates in mMatchableTemplates
    txExpandedNameMap::iterator mapIter(mMatchableTemplates);
    while (mapIter.next()) {
        txListIterator templIter((txList*)mapIter.value());
        MatchableTemplate* templ;
        while ((templ = (MatchableTemplate*)templIter.next())) {
            delete templ->mMatch;
            delete templ;
        }
    }
    
    txListIterator tlIter(&mToplevelItems);
    while (tlIter.hasNext()) {
        delete (txToplevelItem*)tlIter.next();
    }
}
