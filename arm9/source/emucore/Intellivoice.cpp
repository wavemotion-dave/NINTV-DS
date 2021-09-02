
#include "Intellivoice.h"

IntellivoiceState Intellivoice::getState()
{
    IntellivoiceState state = {0};

    state.sp0256State = sp0256.getState();

    return state;
}

void Intellivoice::setState(IntellivoiceState state)
{
    sp0256.setState(state.sp0256State);
}
