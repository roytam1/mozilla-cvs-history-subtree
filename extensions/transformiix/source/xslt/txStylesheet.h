#ifndef TRANSFRMX_TXSTYLESHEET_H
#define TRANSFRMX_TXSTYLESHEET_H

#include "txOutputFormat.h"
#include "txExpandedNameMap.h"
#include "List.h"
#include "Expr.h"

class txToplevelItem;
class txPattern;
class txInstruction;
class txTemplateItem;
class txDecimalFormat;

class txStylesheet
{
public:
    class ImportFrame;
    friend class txStylesheetCompilerState;
    // To be able to do some cleaning up in destructor
    friend class ImportFrame;

    txStylesheet();
    ~txStylesheet();
    nsresult init();
    
    txInstruction* findTemplate(Node* aNode,
                                const txExpandedName& aMode,
                                txIMatchContext* aContext,
                                ImportFrame* aImportedBy,
                                ImportFrame** aImportFrame);
    txDecimalFormat* getDecimalFormat(const txExpandedName& aName);
    txInstruction* getAttributeSet(const txExpandedName& aName);
    txOutputFormat* getOutputFormat();

    /**
     * Called by the stylesheet compiler once all stylesheets has been read.
     */
    nsresult doneCompiling();

    /**
     * Add a key to the stylesheet
     */
    nsresult addKey(const txExpandedName& aName, txPattern* aMatch,
                    Expr* aUse);

    /**
     * Contain information that is import precedence dependant.
     */
    class ImportFrame {
    public:
        ImportFrame();
        ~ImportFrame();

        // List of toplevel items
        txList mToplevelItems;

        // Map of template modes, each item in the map is a txList
        // of templates
        txExpandedNameMap mMatchableTemplates;

        // ImportFrame which is the first one *not* imported by this frame
        ImportFrame* mFirstNotImported;
    };

private:
    class MatchableTemplate {
    public:
        MatchableTemplate(txInstruction* aFirstInstruction,
                          txPattern* aPattern,
                          double aPriority)
            : mFirstInstruction(aFirstInstruction),
              mMatch(aPattern),
              mPriority(aPriority)
        {
        }
        txInstruction* mFirstInstruction;
        txPattern* mMatch;
        double mPriority;
    };

    nsresult addTemplate(txTemplateItem* aTemplate, ImportFrame* aImportFrame);


    // List of ImportFrames
    txList mImportFrames;
    
    // output format
    txOutputFormat mOutputFormat;

    // List of first instructions of templates. This is the owner of all
    // instructions used in templates
    txList mTemplateInstructions;
    
    // Root importframe
    ImportFrame* mRootFrame;
    
    // Named templates
    txExpandedNameMap mNamedTemplates;
    
    // Map with all decimal-formats
    txExpandedNameMap mDecimalFormats;

    // Map with all named attribute sets
    txExpandedNameMap mAttributeSets;
};


/**
 * txNameTestItem holds both an txNameTest and a bool for use in
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
