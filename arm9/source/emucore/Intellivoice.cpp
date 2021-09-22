#include "Intellivoice.h"

void Intellivoice::getState(IntellivoiceState *state)
{
    sp0256.getState(&state->sp0256State);
}

void Intellivoice::setState(IntellivoiceState *state)
{
    sp0256.setState(&state->sp0256State);
}