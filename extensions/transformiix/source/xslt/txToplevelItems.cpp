#include "txToplevelItems.h"
#include "txStylesheet.h"
#include "txInstructions.h"

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
}
