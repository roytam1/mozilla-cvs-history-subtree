txStylesheet::~txStylesheet()
{
  // Delete all ImportFrames
  txListIterator iter(&mImportFrames);
  while (iter.hasNext())
      delete (ImportFrame*)iter.next();
}

nsresult txStylesheet::doneCompiling()
{
    nsresult rv = NS_OK;
    txListIterator frameIter(frame);
    ImportFrame* frame = 0;
    while ((frame = (ImportFrame*)frameIter.next())) {
        txListIterator itemIter(frame);
        itemIter.resetToEnd();
        txToplevelItem* item = 0;
        while ((item = (txToplevelItem*)itemIter.previous())) {
            switch (item->getType()) {
                case txToplevelItem::output:
                {
                    mOutputFormat.merge(((txOutputItem*)item)->mFormat);
                    delete item;
                }
                case txToplevelItem::templ:
                {
                    rv = addTemplate((txTemplateItem*)item);
                    NS_ENSURE_SUCCESS(rv, rv);
                }
            }
        }
        itemIter.remove();
        itemIter.next();
    }
}

void txStylesheet::addTemplate(txTemplateItem* aTemplate,
                               ImportFrame* aImportFrame)
{
    NS_ASSERTION(aTemplate, "missing template");

    MBool isUsed = MB_FALSE;

    

    nsresult rv = NS_OK;
    if (!aTemplate->mName.isNull()) {
        rv = mNamedTemplates.add(name, aTemplate);
        NS_ENSURE_TRUE(NS_SUCCEEDED(rv) || rv == NS_ERROR_XSLT_ALREADY_SET,
                       rv);

        isUsed = MB_TRUE;
    }

    if (!aTemplate->mMatch) {
        // This is no error, see section 6 Named Templates
        if (!isUsed) {
            delete aTemplate;
        }

        return NS_OK;
    }

    // get the txList for the right mode
    txList* templates =
        (txList*)aImportFrame->mMatchableTemplates.get(aTemplate->mMode);

    if (!templates) {
        templates = new txList;
        if (!templates) {
            if (!isUsed) {
                delete aTemplate;
            }
            return NS_ERROR_OUT_OF_MEMORY;
        }

        rv = aImportFrame->mMatchableTemplates.add(mode, templates);
        if (NS_FAILED(rv)) {
            delete templates;
            if (!isUsed) {
                delete aTemplate;
            }
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
        if (!hasPriority) {
            priority = simple->getDefaultPriority();
        }
        MatchableTemplate* nt = new MatchableTemplate(aXslTemplate,
                                                      simple,
                                                      priority);
        if (!nt) {
            NS_ASSERTION(0, "out of mem");
            return;
        }
        txListIterator templ(templates);
        MBool isLast = MB_TRUE;
        while (templ.hasNext() && isLast) {
            MatchableTemplate* mt = (MatchableTemplate*)templ.next();
            if (priority < mt->mPriority) {
                continue;
            }
            templ.addBefore(nt);
            isLast = MB_FALSE;
        }
        if (isLast)
            templates->add(nt);
    }
}

    
nsresult addKey(const txExpandedName& aName, txPattern* aMatch, Expr* aUse)
{
    delete aMatch;
    delete aUse;
    return NS_OK;
}
