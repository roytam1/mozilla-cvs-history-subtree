#include "txError.h"
#include "txOutputFormat.h"
#include "XMLUtils.h"
#include "txStylesheet.h"

class txPattern;
class Expr;
class txInstruction;

class txToplevelItem
{
public:
    virtual ~txToplevelItem()
    {
    }

    enum type {
        attributeSet,
        dummy,
        import,
        //namespaceAlias,
        output,
        param,
        stripSpace, //also used for preserve-space
        templ,
        variable
    };

    virtual type getType();
};

class txInstructionContainer : public txToplevelItem
{
public:
    txInstructionContainer() : mFirstInstruction(0)
    {
    }

    virtual ~txInstructionContainer();

    txInstruction* mFirstInstruction;
};

// xsl:attribute-set
class txAttributeSetItem : public txInstructionContainer
{
public:
    txAttributeSetItem(const txExpandedName aName) : mName(aName)
    {
    }

    virtual txToplevelItem::type getType()
    {
        return txToplevelItem::attributeSet;
    }

    txExpandedName mName;
};

// xsl:import
class txImportItem : public txToplevelItem
{
public:
    ~txImportItem()
    {
        delete mFrame;
    }

    virtual txToplevelItem::type getType()
    {
        return txToplevelItem::import;
    }

    txStylesheet::ImportFrame* mFrame;
};

// xsl:output
class txOutputItem : public txToplevelItem
{
public:
    virtual txToplevelItem::type getType()
    {
        return txToplevelItem::output;
    }

    txOutputFormat mFormat;
};

// insertionpoint for xsl:include
class txDummyItem : public txToplevelItem
{
public:
    virtual txToplevelItem::type getType()
    {
        return txToplevelItem::dummy;
    }
};

// xsl:param at top level
class txParamItem : public txInstructionContainer
{
public:
    txParamItem(const txExpandedName& aName) : mName(aName)
    {
    }

    virtual txToplevelItem::type getType()
    {
        return txToplevelItem::param;
    }

    txExpandedName mName;
};

// xsl:strip-space and xsl:preserve-space
class txStripSpaceItem : public txToplevelItem
{
public:
    virtual ~txStripSpaceItem();

    virtual txToplevelItem::type getType()
    {
        return txToplevelItem::stripSpace;
    }

    addNameTest(txAtom* aLocalName, PRInt32 aNSID, MBool stripSpace);

private:
    txList mNameTestItems;
};

// xsl:template
class txTemplateItem : public txInstructionContainer
{
public:
    txTemplateItem(txPattern* aMatch, const txExpandedName& aName,
                   const txExpandedName& aMode, double aPrio);
    virtual ~txTemplateItem();

    virtual txToplevelItem::type getType()
    {
        return txToplevelItem::templ;
    }
    
    txPattern* mMatch;
    txExpandedName mName;
    txExpandedName mMode;
    double mPrio;
};

// xsl:variable at top level
class txVariableItem : public txInstructionContainer
{
public:
    txVariableItem(const txExpandedName& aName) : mName(aName)
    {
    }
    
    virtual txToplevelItem::type getType()
    {
        return txToplevelItem::variable;
    }

    txExpandedName mName;
};

