#include "convert.h"

union Pack {
	unsigned long bin;
	char bytes[4];
};

void pack_float(float den, char* bin) {

	// Check if the float is between 0 and 1, or the function won't return
	// Negligible inaccuracy for simplicity
	if ((int)den == 0) {
		bin[0] = 0;
		bin[1] = 0;
		bin[2] = 0;
		bin[3] = 0;
		return;
	}

	int sign = den < 0;

	// We have the float's sign, so make it absolute
	if (sign == 1) {
		den *= -1;
	}

	unsigned int whole = (unsigned int)den;
	float part = den - whole;

	// Convert the part to binary
	unsigned int partBin = 0;
	unsigned int partWidth = 0; // Number of bits in partBin
	while (part != 0) {
		part *= 2;

		unsigned int whole = (unsigned int)part;
		partBin |= whole << partWidth;
		part -= whole;

		partWidth++;
	}

	// Whole binaries go from right to left
	// But fraction binaries actually go from LEFT to right
	// So now we have to reverse the frac bits
	// And no, we couldn't process them that way from the start
	// Because we didn't know the bit width of fracBin
	// But now we do ;)
	unsigned int revFracBin = 0;
	unsigned int revFracWidth = 0;
	while (partWidth > 0) {
		revFracBin |= ((partBin & (1 << revFracWidth)) >> revFracWidth) << (partWidth - 1);
		revFracWidth++;
		partWidth--;
	}

	// Glue the two binaries together to create the raw mantissa
	unsigned int denBin = (whole << revFracWidth) | revFracBin;
	unsigned int point = revFracWidth;

	// Normalize the raw mantissa
	unsigned int normalizer = 31;
	while (((denBin & (1 << normalizer)) >> normalizer) != 1)
		normalizer--;

	// Will need this later to pad the mantissa bits
	unsigned int mantissaWidth = normalizer;

	// Biased exponent
	// Subtracting the fraction's bit width from the normalizer gives us the unbiased exponent
	unsigned int biEx = (normalizer - revFracWidth) + 127;

	// Extract the actual mantissa bits
	// Decrement normalizer before first extraction
	// Normalizer initially indicates the very first non-zero bit in the raw mantissa
	// We do not include that bit in the actual mantissa
	unsigned int mantissa = 0;
	while (normalizer > 0) {
		normalizer--;
		mantissa |= denBin & (1 << normalizer);
	}

	union Pack p;
 	p.bin = (sign << 31) | (biEx << 23) | mantissa << (23 - mantissaWidth);
	int n = 1;
	if (*(char*)&n) {
		bin[0] = p.bytes[3];
		bin[1] = p.bytes[2];
		bin[2] = p.bytes[1];
		bin[3] = p.bytes[0];
	}
	else {
		bin[0] = p.bytes[0];
		bin[1] = p.bytes[1];
		bin[2] = p.bytes[2];
		bin[3] = p.bytes[3];
	}
}

float unpack_float(char* bin) {

	// If this condition isn't checked, the function will return infinity
	if (bin == 0) {
		return 0;
	}

	union Pack p;
	int n = 1;
	if (*(char*)&n) {
		p.bytes[0] = bin[3];
		p.bytes[1] = bin[2];
		p.bytes[2] = bin[1];
		p.bytes[3] = bin[0];
	}
	else {
		p.bytes[0] = bin[0];
		p.bytes[1] = bin[1];
		p.bytes[2] = bin[2];
		p.bytes[3] = bin[3];
	}

	unsigned int sign = (p.bin & (0b00000001 << 31)) >> 31;
	unsigned int biEx = (p.bin & (0b11111111 << 23)) >> 23;
	unsigned int unBiEx = biEx - 127;

	float mantissaDen = 1.0f;
	float currentPlace = 0.5f;
	unsigned int mantissaBin = p.bin & 0b11111111111111111111111;
	signed int iMantissaBin = 22; // If this isn't signed, the loop will never end
	while (iMantissaBin >= 0) {
		if (((mantissaBin & (1 << iMantissaBin)) >> iMantissaBin) == 1) {
			mantissaDen += currentPlace;
		}
		iMantissaBin--;
		currentPlace /= 2;
	}

	float den = mantissaDen;
	if (sign == 1) {
		mantissaDen *= -1;
	}

	float expander = 1;
	float factor = 2;

	if (unBiEx < 0) {
		factor = 0.5f;
		unBiEx *= -1;
	}
	
	for (unsigned int i = 0; i < unBiEx; i++) {
		expander *= factor;
	}

	return mantissaDen * expander;
}