#ifndef TRANSFRMX_TXSTYLESHEET_H
#define TRANSFRMX_TXSTYLESHEET_H

#include "txOutputFormat.h"

class txToplevelItem;

class txStylesheet
{
public:
    ~txStylesheet();

    friend class txStylesheetCompilerState;

    /**
     * Contain information that is import precedence dependant.
     */
    class ImportFrame {
    public:
        ImportFrame(ImportFrame* aFirstNotImported);
        ~ImportFrame();

        // List of items
        txList mItems;

        // Map of template modes, each item in the map is a txList
        // of templates
        txExpandedNameMap mMatchableTemplates;

        // ImportFrame which is the first one *not* imported by this frame
        ImportFrame* mFirstNotImported;
    };
    // To be able to do some cleaning up in destructor
    friend class ImportFrame;

    /**
     * Called by the stylesheet compiler once all stylesheets has been read.
     */
    nsresult doneCompiling();
    
    /**
     * Add a toplevel item
     */
    addToplevelItem(txToplevelItem* aItem, ImportFrame* aFrame);

    /**
     * Add a key to the stylesheet
     */
    nsresult addKey(const txExpandedName& aName, txPattern* aMatch,
                    Expr* aUse);

private:
    addTemplate(txTemplate* aTemplate);
    
    // List of ImportFrames
    txList mImportFrames;
    
    // output format
    txOutputFormat mOutputFormat;

    // List of first instructions of templates. This is the owner of all
    // instructions used in templates
    txList mTemplateInstructions;
};


/**
 * txNameTestItem holds both an ElementExpr and a bool for use in
 * whitespace stripping.
 */
class txNameTestItem {
public:
    txNameTestItem(txAtom* aLocalName, PRInt32 aNSID, MBool stripSpace)
        : mNameTest(0, aLocalName, aNSID, Node::ELEMENT_NODE),
          mStrips(stripSpace)
    {
    }

    MBool matches(Node* aNode, txIMatchContext* aContext) {
        return mNameTest.matches(aNode, aContext);
    }

    MBool stripsSpace() {
        return mStrips;
    }

    double getDefaultPriority() {
        return mNameTest.getDefaultPriority();
    }

protected:
    txNameTest mNameTest;
    MBool mStrips;
};


#endif //TRANSFRMX_TXSTYLESHEET_H
