#include "txToplevelItems.h"
#include "txStylesheet.h"

txInstructionContainer::~txInstructionContainer()
{
    txInstruction* instr = mFirstInstruction;
    while (instr) {
        txInstruction* next = instr->mNext;
        delete instr;
        instr = next;
    }
}

txStripSpaceItem::~txStripSpaceItem()
{
    txListIterator iter(&mNameTestItems);
    txNameTestItem* item;
    while ((item = (txNameTestItem*)iter.next())) {
        delete item;
    }
}

txTemplateItem::~txTemplateItem()
{
    if (!mOwnsInstructions) {
        // If we don't own our items make sure that we don't delete them in the
        // txInstructionContainer dtor
        mFirstInstruction = 0;
    }
}
