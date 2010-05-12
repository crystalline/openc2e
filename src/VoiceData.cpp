/*
 *  VoiceData.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Wed 12 May 2010.
 *  Copyright (c) 2010 Alyssa Milburn. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 */

#include "VoiceData.h"
#include "streamutils.h"
#include "Catalogue.h"
#include <boost/format.hpp>

#define NUM_VOICE_FILES 32

// Creatures 1 and 2 store this data in .vce files
VoiceData::VoiceData(std::ifstream &file) {
	// voice files and associated delay
	for (unsigned int i = 0; i < NUM_VOICE_FILES; i++) {
		char temp[4];
		file.read((char *)&temp, 4);
		VoiceEntry entry;
		if (temp[0])
			entry.name = std::string(temp, 4);
		entry.delay = read32(file);
		Voices.push_back(entry);
	}

	// 3 letters per syllable
	for (unsigned int i = 0; i < 3; i++) {
		// 26 possible letters + blank
		for (unsigned int j = 0; j < 27; j++) {
			LookupTable.push_back(read32(file));
		}
	}
}

// c2e stores this data in the catalogue
VoiceData::VoiceData(std::string tagname) {
	if (!catalogue.hasTag(tagname)) return;

	const std::vector<std::string> &tagdata = catalogue.getTag(tagname);
	if (!tagdata.size()) return;

	// the first entry refers to another tag (usually DefaultLanguage),
	// which has the lookup table as hex strings
	std::string languagetag = tagdata[0];
	if (!catalogue.hasTag(languagetag)) return;
	const std::vector<std::string> &lookupdata = catalogue.getTag(languagetag);
	for (unsigned int i = 0; i < lookupdata.size(); i++) {
		uint32 data = strtoul(lookupdata[i].c_str(), NULL, 16);
		LookupTable.push_back(data);
	}
	if (LookupTable.size() != 3 * 27) throw creaturesException(
		boost::str(boost::format("invalid lookup table size %d reading language tag '%s'")
		% LookupTable.size() % languagetag));

	// the remaining entries are pairs of (name, delay)
	for (unsigned int i = 1; i < tagdata.size() - 1; i+=2) {
		VoiceEntry entry;
		entry.name = tagdata[i];
		entry.delay = atoi(tagdata[i+1].c_str());
		Voices.push_back(entry);
	}

	if (Voices.size() != 32) throw creaturesException(
		boost::str(boost::format("invalid voice table size %d reading voice tag '%s'")
		% Voices.size() % tagname));
}

std::vector<unsigned int> VoiceData::GetSentenceFor(std::string in) {
	// we work in lowercase
	std::transform(in.begin(), in.end(), in.begin(), (int(*)(int))tolower);

	// we want the string in the form ' word word ', compressing multiple spaces into one;
	// we rewrite the letters as 0-25 and spaces to 26
	std::vector<unsigned int> out;
	if (!LookupTable.size()) return out;
	out.push_back(26);
	for (unsigned int i = 0; i < in.size(); i++) {
		char c = in[i];
		if (c >= 'a' && c <= 'z')
			out.push_back(c - 'a');
		else if (c == ' ' && out[out.size() - 1] != 26)
			out.push_back(26);
	}
	out.push_back(26);

	return out;
}

static unsigned int bitcount(unsigned int n) {
	unsigned int count = 0;
	while (n) {
		count += n & 1;
		n >>= 1;
	}
	return count;
}

bool VoiceData::NextSyllableFor(std::vector<unsigned int> &sentence, unsigned int &pos, VoiceEntry &syllable) {
	/*
	 * The idea behind the voice generator is to work in groups of three letters (a 'syllable')
	 * and, for each syllable, to transform the sum of the letters into an entry in the voice list.
         *
	 * Voices 0-3 are for syllables in the form 'a b' (so between words; just for delaying),
	 * voices 4-10 for ' ab' (start of a word), voices 11-17 for 'ab ' (end of a word) and voices
	 * 18-31 for 'abc' (so, in the middle of a word).
	 */

	// pos should start at 1
	assert(pos > 0);

	// if there's less than two letters left, no more syllables
	if (pos + 1 >= sentence.size()) return false;

	unsigned int chars[3] = { sentence[pos - 1], sentence[pos], sentence[pos + 1] };
	uint32 sum = chars[0] + chars[1] + chars[2];

	uint32 lookup = LookupTable[chars[0]] & LookupTable[chars[1] + 27] & LookupTable[chars[2] + 27 + 27];
	if (chars[1] == 26)
		lookup &= 0xf; // bits 0-3 (between words)
	if (chars[0] == 26)
		lookup &= 0x7f0; // bits 4-10 (start of word)
	if (chars[2] == 26)
		lookup &= 0x3f800; // bits 11-17 (end of word)
	if (chars[0] < 26 && chars[1] < 26 && chars[2] < 26)
		lookup &= 0xfffc0000; // bits 18-31

	if (lookup) {
		unsigned int index = 1 + (sum % bitcount(lookup));
		// we're looking for the bit index of 'index' within 'lookup'
		// eg, if index is 3 and lookup is 010011 then we want 4
		sum = 0;
		while (index) {
			index -= lookup & 1;
			lookup >>= 1;
			sum++;
		}
		sum--;
	} else {
		// no valid lookup (single-letter word?), so just pick something in range
		if (chars[1] == 26) {
			sum = (sum % 4); // 0-3
		} else if (chars[0] == 26) {
			sum = (sum % 7) + 4; // 4-10
		} else if (chars[2] == 26) {
			sum = (sum % 7) + 11; // 11-17
		} else {
			sum = (sum % 14) + 18; // 18-31
		}
	}
	assert(sum < Voices.size());
	syllable = Voices[sum];

	// skip to the next letter, and then skip over the *next* letter if it's in the middle of a word
	pos++;
	if (pos + 1 < sentence.size() &&
		(sentence[pos - 1] < 26 && sentence[pos] < 26 && sentence[pos + 1] < 26)) {
		pos++;
	}

	return true;
}

/* vim: set noet: */