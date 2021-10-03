// =====================================================================================
// Copyright (c) 2021 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, it's source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================
#include "Intellivoice.h"

void Intellivoice::getState(IntellivoiceState *state)
{
    sp0256.getState(&state->sp0256State);
}

void Intellivoice::setState(IntellivoiceState *state)
{
    sp0256.setState(&state->sp0256State);
}