#include "txError.h"

class txInstruction
{
public:
    virtual nsresult execute(txExecutionState& aEs) = 0;

    txInstruction* mNext;
};

