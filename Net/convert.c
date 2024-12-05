#include "convert.h"

union Pack {
	unsigned long whole;
	char chunks[4];
};

void pack_float(float den, char* bin) {

	// Check if the float is between 0 and 1, or the function won't return
	// Simplicity for negligible inaccuracy
	if ((int)den == 0) {
		return 0;
	}

	// Extract the float's sign
	int sign = den < 0;

	// We have the float's sign, so make it absolute
	if (sign == 1) {
		den *= -1;
	}

	unsigned int whole = (unsigned int)den;
	float fraction = den - whole;

	// Convert each part to binary
	// Since there's already a data type for ints, the computer already stores them in raw binary
	// So we don't have to convert the whole to binary ourselves (it would be redundant)
	// However, there's no data type for fractions, so the computer doesn't know how to store them in raw binary
	// While floats can contain fractions, they do NOT contain a fraction's RAW binary
	// What they really contain is the raw binary of a number's scientific notation
	unsigned int fracBin = 0;
	unsigned int fracWidth = 0; // Number of bits required to represent the fraction in binary
	while (fraction != 0) {
		fraction *= 2;

		unsigned int whole = (unsigned int)fraction;
		fracBin |= whole << fracWidth;
		fraction -= whole;

		fracWidth++;
	}

	// Whole binaries go from right to left
	// But fraction binaries actually go from LEFT to right
	// So now we have to reverse the frac bits
	// And no, we couldn't process them that way from the start
	// Because we didn't know the bit width of fracBin
	// But now we do ;)
	unsigned int revFracBin = 0;
	unsigned int revFracWidth = 0;
	while (fracWidth > 0) {
		revFracBin |= ((fracBin & (1 << revFracWidth)) >> revFracWidth) << (fracWidth - 1);
		revFracWidth++;
		fracWidth--;
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
	p.whole = (sign << 31) | (biEx << 23) | mantissa << (23 - mantissaWidth);
	bin[0] = p.chunks[0];
	bin[1] = p.chunks[1];
	bin[2] = p.chunks[2];
	bin[3] = p.chunks[3];
}

float unpack_float(char* bin) {

	// If this condition isn't checked, the function will return infinity
	if (bin == 0)
		return 0;

	union Pack p;
	p.chunks[0] = bin[0];
	p.chunks[1] = bin[1];
	p.chunks[2] = bin[2];
	p.chunks[3] = bin[3];

	unsigned int sign = (p.whole & (0b1 << 31)) >> 31;
	unsigned int biEx = (p.whole & (0b11111111 << 23)) >> 23;
	unsigned int unBiEx = biEx - 127;

	float mantissaDen = 1.0f;
	float currentPlace = 0.5f;
	unsigned int mantissaBin = p.whole & 0b11111111111111111111111;
	signed int iMantissaBin = 22; // If this isn't signed, the loop will never end
	while (iMantissaBin >= 0) {
		if (((mantissaBin & (1 << iMantissaBin)) >> iMantissaBin) == 1)
			mantissaDen += currentPlace;
		iMantissaBin--;
		currentPlace /= 2;
	}

	float den = mantissaDen;
	if (sign == 1)
		mantissaDen *= -1;

	float expander = 1;
	float factor = 2;

	if (unBiEx < 0) {
		factor = 1 / 2;
		unBiEx *= -1;
	}
	
	for (unsigned int i = 0; i < unBiEx; i++)
		expander *= factor;

	return mantissaDen * expander;
}