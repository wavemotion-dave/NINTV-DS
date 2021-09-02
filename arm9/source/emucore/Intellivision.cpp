
#include "Intellivision.h"

TYPEDEF_STRUCT_PACK( _IntellivisionState
{
    StateHeader          header;
    StateChunk           cpu;
    CP1610State          cpuState;
    StateChunk           stic;
    AY38900State         sticState;
    StateChunk           psg;
    AY38914State         psgState;
    StateChunk           RAM8bit;
    RAMState             RAM8bitState;
    UINT16               RAM8bitImage[RAM8BIT_SIZE];
    StateChunk           RAM16bit;
    RAMState             RAM16bitState;
    UINT16               RAM16bitImage[RAM16BIT_SIZE];
    StateChunk           GRAM;
    RAMState             GRAMState;
    UINT16               GRAMImage[GRAM_SIZE];
    StateChunk           ivoice;
    IntellivoiceState    ivoiceState;
    StateChunk           ecs;
    ECSState             ecsState;
    StateChunk           eof;
} IntellivisionState; )

/**
 * Initializes all of the basic hardware included in the Intellivision
 * Master Component as well as the ECS and Intellivoice peripherals.
 * This method is called only once when the Intellivision is constructed.
 */
Intellivision::Intellivision()
    : Emulator("Intellivision"),
      player1Controller(0, "Hand Controller #1"),
      player2Controller(1, "Hand Controller #2"),
      psg(0x01F0, &player1Controller, &player2Controller),
      RAM8bit(RAM8BIT_SIZE, 0x0100, 8),
      RAM16bit(RAM16BIT_SIZE, 0x0200, 16),
      execROM("Executive ROM", "exec.bin", 0, 2, 0x1000, 0x1000),
      grom("GROM", "grom.bin", 0, 1, 0x0800, 0x3000),
      gram(),
      cpu(&memoryBus, 0x1000, 0x1004),
      stic(&memoryBus, &grom, &gram)
{
    // define the video pixel dimensions
    videoWidth = 160;
    videoHeight = 192;

    //make the pin connections from the CPU to the STIC
    stic.connectPinOut(AY38900_PIN_OUT_SR1, &cpu, CP1610_PIN_IN_INTRM);
    stic.connectPinOut(AY38900_PIN_OUT_SR2, &cpu, CP1610_PIN_IN_BUSRQ);
    cpu.connectPinOut(CP1610_PIN_OUT_BUSAK, &stic, AY38900_PIN_IN_SST);

    //add the player one hand controller
    AddInputConsumer(&player1Controller);

    //add the player two hand controller
    AddInputConsumer(&player2Controller);

    //add the 8-bit RAM
    AddRAM(&RAM8bit);

    //add the 16-bit RAM
    AddRAM(&RAM16bit);

    //add the executive ROM
    AddROM(&execROM);

    //add the GROM
    AddROM(&grom);

    //add the GRAM
    AddRAM(&gram);

    //add the backtab ram
    AddRAM(&stic.backtab);

    //add the CPU
    AddProcessor(&cpu);

    //add the STIC
    AddProcessor(&stic);
    AddVideoProducer(&stic);

    //add the STIC registers
    AddRAM(&stic.registers);

    //add the PSG
    AddProcessor(&psg);
    AddAudioProducer(&psg);

    //add the PSG registers
    AddRAM(&psg.registers);

    AddPeripheral(&ecs);
    AddPeripheral(&intellivoice);
}

BOOL Intellivision::SaveState(const CHAR* filename)
{
	BOOL didSave = FALSE;
	IntellivisionState state = {9};
	size_t totalStateSize = sizeof(IntellivisionState);

	size_t hsize = sizeof(StateHeader);
	size_t csize = sizeof(StateChunk);
	size_t cpusize = sizeof(CP1610State);
	size_t sticsize = sizeof(AY38900State);
	size_t psgsize = sizeof(AY38914State);
	size_t ivoicesize = sizeof(IntellivoiceState);
	size_t ecssize = sizeof(ECSState);
	size_t ramsize = sizeof(RAMState);
	size_t ram8imgsize = sizeof(state.RAM8bitImage);
	size_t ram16imgsize = sizeof(state.RAM16bitImage);
	size_t gramimgsize = sizeof(state.GRAMImage);

	state.header.emu = FOURCHAR('EMUS');
	state.header.state = FOURCHAR('TATE');
	state.header.emuID = ID_EMULATOR_BLISS;
	state.header.version = FOURCHAR(EMU_STATE_VERSION);
	state.header.sys = FOURCHAR('SYS\0');
	state.header.sysID = ID_SYSTEM_INTELLIVISION;
	state.header.cart = FOURCHAR('CART');
	state.header.cartID = currentRip->GetCRC();

	state.cpu.id = FOURCHAR('CPU\0');
	state.cpu.size = sizeof(CP1610State);
	state.cpuState = cpu.getState();

	state.stic.id = FOURCHAR('STIC');
	state.stic.size = sizeof(AY38900State);
	state.sticState = stic.getState();

	state.psg.id = FOURCHAR('PSG\0');
	state.psg.size = sizeof(AY38914State);
	state.psgState = psg.getState();

	state.RAM8bit.id = FOURCHAR('RAM0');
	state.RAM8bit.size = sizeof(RAMState) + sizeof(state.RAM8bitImage);
	state.RAM8bitState = RAM8bit.getState(state.RAM8bitImage);

	state.RAM16bit.id = FOURCHAR('RAM1');
	state.RAM16bit.size = sizeof(RAMState) + sizeof(state.RAM16bitImage);
	state.RAM16bitState = RAM16bit.getState(state.RAM16bitImage);

	state.GRAM.id = FOURCHAR('GRAM');
	state.GRAM.size = sizeof(RAMState) + sizeof(state.GRAMImage);
	state.GRAMState = gram.getState(state.GRAMImage);

	// TODO: only if ivoice is used for this cart?
	state.ivoice.id = FOURCHAR('VOIC');
	state.ivoice.size = sizeof(IntellivoiceState);
	state.ivoiceState = intellivoice.getState();

	// TODO: only if ecs is used for this cart?
	state.ecs.id = FOURCHAR('ECS\0');
	state.ecs.size = sizeof(ECSState);
	state.ecsState = ecs.getState();

	state.eof.id = FOURCHAR('EOF\0');
	state.eof.size = sizeof(IntellivisionState);

	FILE* file = fopen(filename, "wb");

	if (file == NULL) {
		printf("Error: Unable to create file %s\n", filename);
		didSave = FALSE;
	}

	if (file != NULL && totalStateSize == fwrite(&state, 1, totalStateSize, file)) {
		didSave = TRUE;
	} else {
		printf("Error: could not write %zu bytes to file %s\n", totalStateSize, filename);
		didSave = FALSE;
	}

	if (file) {
		fclose(file);
		file = NULL;
	}

	return didSave;
}

BOOL Intellivision::LoadState(const CHAR* filename)
{
	BOOL didLoadState = FALSE;
	IntellivisionState state = {9};
	size_t totalStateSize = sizeof(IntellivisionState);

	FILE* file = fopen(filename, "rb");

	if (file == NULL) {
		printf("Error: Unable to open file %s\n", filename);
		return FALSE;
	}
#if 0
	// read in the whole file
	if (totalStateSize != fread(&state, 1, totalStateSize, file)) {
		printf("Error: could not read state (%zu bytes) from file %s\n", totalStateSize, filename);
		goto close;
	}
#else
	BOOL isParsing = FALSE;
	StateChunk chunk = {0};

	// read in the header
	if (sizeof(StateHeader) != fread(&state, 1, sizeof(StateHeader), file)) {
		printf("Error: could not read state header (%zu bytes) from file %s\n", totalStateSize, filename);
		goto close;
	}

	// validate file header
	if (state.header.emu != FOURCHAR('EMUS') || state.header.state != FOURCHAR('TATE')) {
		printf("Error: invalid header in file %s\n", filename);
		goto close;
	}

	if (state.header.emuID != ID_EMULATOR_BLISS) {
		printf("Error: invalid emulator ID %x in file %s\n", state.header.emuID, filename);
		goto close;
	}

	if (FOURCHAR(EMU_STATE_VERSION) != FOURCHAR('dev\0') && state.header.version != FOURCHAR('dev\0') && state.header.version != FOURCHAR(EMU_STATE_VERSION)) {
		printf("Error: invalid emulator version 0x%08x (expected 0x%08x) in file %s\n", state.header.version, EMU_STATE_VERSION, filename);
		goto close;
	}

	if (state.header.sys != FOURCHAR('SYS\0')) {
		printf("Error: expected 'SYS ' chunk in file %s\n", filename);
		goto close;
	}

	if (state.header.sysID != ID_SYSTEM_INTELLIVISION) {
		printf("Error: invalid system ID %x in file %s\n", state.header.sysID, filename);
		goto close;
	}

	if (state.header.cart != FOURCHAR('CART')) {
		printf("Error: expected 'CART' chunk in file %s\n", filename);
		goto close;
	}

	if (state.header.cartID != 0x00000000 && state.header.cartID != currentRip->GetCRC()) {
		printf("Error: cartridge mismatch in file %s\n", filename);
		goto close;
	}

	isParsing = TRUE;
	while (isParsing) {
		size_t fpos = ftell(file);
		if (sizeof(StateChunk) != fread(&chunk, 1, sizeof(StateChunk), file)) {
			isParsing = FALSE;
			break;
		}

		switch (chunk.id) {
			default:
				fpos = ftell(file);
				break;
			case FOURCHAR('CPU\0'):
				if (chunk.size == sizeof(state.cpuState)) {
					state.cpu = chunk;
					fread(&state.cpuState, 1, state.cpu.size, file);
				}
				break;
			case FOURCHAR('STIC'):
				if (chunk.size == sizeof(state.sticState)) {
					state.stic = chunk;
					fread(&state.sticState, 1, state.stic.size, file);
				}
				break;
			case FOURCHAR('PSG\0'):
				if (chunk.size == sizeof(state.psgState)) {
					state.psg = chunk;
					fread(&state.psgState, 1, state.psg.size, file);
				}
				break;
			case FOURCHAR('RAM0'):
				if (chunk.size == sizeof(state.RAM8bitState) + sizeof(state.RAM8bitImage)) {
					state.RAM8bit = chunk;
					fread(&state.RAM8bitState, 1, state.RAM8bit.size, file);
				}
				break;
			case FOURCHAR('RAM1'):
				if (chunk.size == sizeof(state.RAM16bitState) + sizeof(state.RAM16bitImage)) {
					state.RAM16bit = chunk;
					fread(&state.RAM16bitState, 1, state.RAM16bit.size, file);
				}
				break;
			case FOURCHAR('GRAM'):
				if (chunk.size == sizeof(state.GRAMState) + sizeof(state.GRAMImage)) {
					state.GRAM = chunk;
					fread(&state.GRAMState, 1, state.GRAM.size, file);
				}
				break;
			case FOURCHAR('VOIC'):
				// TODO: only if ivoice/ecs is used for this cart?
				if (chunk.size == sizeof(state.ivoiceState)) {
					state.ivoice = chunk;
					fread(&state.ivoiceState, 1, state.ivoice.size, file);
				}
				break;
			case FOURCHAR('ECS\0'):
				// TODO: only if ivoice/ecs is used for this cart?
				if (chunk.size == sizeof(state.ecsState)) {
					state.ecs = chunk;
					fread(&state.ecsState, 1, state.ecs.size, file);
				}
				break;
			case FOURCHAR('EOF\0'):
				state.eof = chunk;
				isParsing = FALSE;
				break;
		}
	}
#endif
	didLoadState = TRUE;

	cpu.setState(state.cpuState);
	stic.setState(state.sticState);
	psg.setState(state.psgState);
	RAM8bit.setState(state.RAM8bitState, state.RAM8bitImage);
	RAM16bit.setState(state.RAM16bitState, state.RAM16bitImage);
	gram.setState(state.GRAMState, state.GRAMImage);
	intellivoice.setState(state.ivoiceState);
	ecs.setState(state.ecsState);

close:
	fclose(file);
	file = NULL;
end:
	return didLoadState;
}
