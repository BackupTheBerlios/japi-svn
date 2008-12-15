/*
	Copyright (c) 2006, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// First the UTF-8 traits

template<>
template<class ByteIterator>
uint32
MEncodingTraits<kEncodingUTF8>::GetNextCharLength(
	ByteIterator		inText)
{
	uint32 result = 1;

	if (*inText & 0x080)
	{
		uint32 r = 1;
		
		if ((*inText & 0x0E0) == 0x0C0)
			r = 2;
		else if ((*inText & 0x0F0) == 0x0E0)
			r = 3;
		else if ((*inText & 0x0F8) == 0x0F0)
			r = 4;
		
		for (uint32 i = 1; (inText[i] & 0x0c0) == 0x080 and i < r; ++i)
			++result;
	}

	return result;
}

template<>
template<class ByteIterator>
uint32
MEncodingTraits<kEncodingUTF8>::GetPrevCharLength(
	ByteIterator		inText)
{
	int32 result = 1;
	
	if ((inText[-1] & 0x0080) != 0)
	{
		result = 2;

		while ((inText[-result] & 0x00C0) == 0x0080 and result < 6)
			result++;
	
		switch (result)
		{
			case 2:	if ((inText[-result] & 0x00E0) != 0x00C0) result = 1; break;
			case 3:	if ((inText[-result] & 0x00F0) != 0x00E0) result = 1; break;
			case 4:	if ((inText[-result] & 0x00F8) != 0x00F0) result = 1; break;
			case 5:	if ((inText[-result] & 0x00FC) != 0x00F8) result = 1; break;
			case 6:	if ((inText[-result] & 0x00FE) != 0x00FC) result = 1; break;
			default:										  result = 1; break;
		}
	}
	
	return result;
}

template<>
template<class ByteIterator>
void
MEncodingTraits<kEncodingUTF8>::ReadUnicode(
	ByteIterator		inText,
	uint32&				outLength,
	wchar_t&			outUnicode)
{
	outUnicode = 0x0FFFD;
	outLength = 1;
	
	if ((*inText & 0x080) == 0)
	{
		outUnicode = static_cast<wchar_t>(*inText);
	}
	else if ((*inText & 0x0E0) == 0x0C0)
	{
		outUnicode = static_cast<wchar_t>(((inText[0] & 0x01F) << 6) | (inText[1] & 0x03F));
		outLength = 2;
		
		if (outUnicode < 0x080 or (inText[1] & 0x000c0) != 0x00080)
		{
			outUnicode = 0x0FFFD;
			outLength = GetNextCharLength(inText);
		}
	}
	else if ((*inText & 0x0F0) == 0x0E0)
	{
		outUnicode = static_cast<wchar_t>(((inText[0] & 0x00F) << 12) | ((inText[1] & 0x03F) << 6) | (inText[2] & 0x03F));
		outLength = 3;

		if (outUnicode < 0x000000800 or (inText[1] & 0x000c0) != 0x00080 or (inText[2] & 0x000c0) != 0x00080)
		{
			outUnicode = 0x0FFFD;
			outLength = GetNextCharLength(inText);
		}
	}
	else if ((*inText & 0x0F8) == 0x0F0)
	{
		outUnicode = static_cast<wchar_t>(((inText[0] & 0x007) << 18) | ((inText[1] & 0x03F) << 12) | ((inText[2] & 0x03F) << 6) | (inText[3] & 0x03F));
		outLength = 4;

		if (outUnicode < 0x00001000 or (inText[1] & 0x000c0) != 0x00080 or (inText[2] & 0x000c0) != 0x00080 or
				(inText[3] & 0x000c0) != 0x00080)
		{
			outUnicode = 0x0FFFD;
			outLength = GetNextCharLength(inText);
		}
	}
}

template<>
template<class ByteIterator>
uint32
MEncodingTraits<kEncodingUTF8>::WriteUnicode(
	ByteIterator&		inText,
	wchar_t				inUnicode)
{
	uint32 result = 0;
	
	/* 
		Note basv:
		To remove warnings the results of the conversion are casted to char. 
		This should be no problem since its in UTF8 encoding
		Also when ByteIter should be > 8 byte then the char will be promoted again. This is a good
		thing.
	*/

	if (inUnicode < 0x080)
	{
		*inText++ = static_cast<char> (inUnicode);
		result = 1;
	}
	else if (inUnicode < 0x0800)
	{
		*inText++ = static_cast<char> (0x0c0 | (inUnicode >> 6));
		*inText++ = static_cast<char> (0x080 | (inUnicode & 0x3f));
		result = 2;
	}
	else if (inUnicode < 0x00010000)
	{
		*inText++ = static_cast<char> (0x0e0 | (inUnicode >> 12));
		*inText++ = static_cast<char> (0x080 | ((inUnicode >> 6) & 0x3f));
		*inText++ = static_cast<char> (0x080 | (inUnicode & 0x3f));
		result = 3;
	}
	else
	{
		*inText++ = static_cast<char> (0x0f0 | (inUnicode >> 18));
		*inText++ = static_cast<char> (0x080 | ((inUnicode >> 12) & 0x3f));
		*inText++ = static_cast<char> (0x080 | ((inUnicode >> 6) & 0x3f));
		*inText++ = static_cast<char> (0x080 | (inUnicode & 0x3f));
		result = 4;
	}
	
	return result;
}

// Then the UTF-16 BE traits

template<>
template<class ByteIterator>
uint32
MEncodingTraits<kEncodingUTF16BE>::GetNextCharLength(
	ByteIterator		inText)
{
	uint32 result = 2;
	if (static_cast<unsigned char>(*inText) >= 0x0D8 and
		static_cast<unsigned char>(*inText) <= 0x0DB)
	{
		inText += 2;
		if (static_cast<unsigned char>(*inText) >= 0xDC and
			static_cast<unsigned char>(*inText) <= 0xDF)
		{
			result = 4;
		}
	}
	return result;
}

template<>
template<class ByteIterator>
void
MEncodingTraits<kEncodingUTF16BE>::ReadUnicode(
	ByteIterator		inText,
	uint32&				outLength,
	wchar_t&			outUnicode)
{
	ByteIterator iter(inText);

	unsigned char ch1 = static_cast<unsigned char>(*iter++);
	unsigned char ch2 = static_cast<unsigned char>(*iter++);
	outUnicode = static_cast<wchar_t>((ch1 << 8) | ch2);
	outLength = 2;
	
	if (outUnicode >= 0xD800 and outUnicode <= 0xDBFF)
	{
		ch1 = static_cast<unsigned char>(*iter++);
		ch2 = static_cast<unsigned char>(*iter++);
		
		wchar_t c = static_cast<wchar_t>((ch1 << 8) | ch2);
		if (c >= 0xDC00 and c <= 0xDFFF)
		{
			outUnicode = (outUnicode - 0xD800) * 0x400 + (c - 0xDC00) + 0x10000;
			outLength = 4;
		}
	}
}

template<>
template<class ByteIterator>
uint32
MEncodingTraits<kEncodingUTF16BE>::WriteUnicode(
	ByteIterator&		inText,
	wchar_t				inUnicode)
{
	uint32 result;
	
	if (inUnicode <= 0xFFFF)
	{
		*inText++ = static_cast<char>(inUnicode >> 8);
		*inText++ = static_cast<char>(inUnicode & 0x0ff);
		
		result = 2;
	}
	else
	{
		wchar_t h = (inUnicode - 0x10000) / 0x400 + 0xD800;
		wchar_t l = (inUnicode - 0x10000) % 0x400 + 0xDC00;

		*inText++ = static_cast<char>(h >> 8);
		*inText++ = static_cast<char>(h & 0x0ff);
		*inText++ = static_cast<char>(l >> 8);
		*inText++ = static_cast<char>(l & 0x0ff);
		
		result = 4;
	}
	
	return result;
}

// Then the UTF-16 LE traits

template<>
template<class ByteIterator>
uint32
MEncodingTraits<kEncodingUTF16LE>::GetNextCharLength(
	ByteIterator		inText)
{
	uint32 result = 2;
	if (static_cast<unsigned char>(*(inText + 1)) >= 0x0D8 and
		static_cast<unsigned char>(*(inText + 1)) <= 0x0DB)
	{
		inText += 2;
		if (static_cast<unsigned char>(*(inText + 1)) >= 0xDC and
			static_cast<unsigned char>(*(inText + 1)) <= 0xDF)
		{
			result = 4;
		}
	}
	return result;
}

template<>
template<class ByteIterator>
void
MEncodingTraits<kEncodingUTF16LE>::ReadUnicode(
	ByteIterator		inText,
	uint32&				outLength,
	wchar_t&			outUnicode)
{
	ByteIterator iter(inText);

	unsigned char ch2 = static_cast<unsigned char>(*iter++);
	unsigned char ch1 = static_cast<unsigned char>(*iter++);
	outUnicode = static_cast<wchar_t>((ch1 << 8) | ch2);
	outLength = 2;
	
	if (outUnicode >= 0xD800 and outUnicode <= 0xDBFF)
	{
		ch2 = static_cast<unsigned char>(*iter++);
		ch1 = static_cast<unsigned char>(*iter++);
		
		wchar_t c = static_cast<wchar_t>((ch1 << 8) | ch2);
		if (c >= 0xDC00 and c <= 0xDFFF)
		{
			outUnicode = (outUnicode - 0xD800) * 0x400 + (c - 0xDC00) + 0x10000;
			outLength = 4;
		}
	}
}

template<>
template<class ByteIterator>
uint32
MEncodingTraits<kEncodingUTF16LE>::WriteUnicode(
	ByteIterator&		inText,
	wchar_t				inUnicode)
{
	uint32 result;
	
	if (inUnicode <= 0xFFFF)
	{
		*inText++ = static_cast<char>(inUnicode & 0x0ff);
		*inText++ = static_cast<char>(inUnicode >> 8);
		
		result = 2;
	}
	else
	{
		wchar_t h = (inUnicode - 0x10000) / 0x400 + 0xD800;
		wchar_t l = (inUnicode - 0x10000) % 0x400 + 0xDC00;

		*inText++ = static_cast<char>(h & 0x0ff);
		*inText++ = static_cast<char>(h >> 8);
		*inText++ = static_cast<char>(l & 0x0ff);
		*inText++ = static_cast<char>(l >> 8);
		
		result = 4;
	}
	
	return result;
}

// wchar_t

template<>
template<class ByteIterator>
uint32
MEncodingTraits<kEncodingUCS2>::GetNextCharLength(
	ByteIterator		inText)
{
	return sizeof(wchar_t);
}

template<>
template<class ByteIterator>
void
MEncodingTraits<kEncodingUCS2>::ReadUnicode(
	ByteIterator		inText,
	uint32&				outLength,
	wchar_t&			outUnicode)
{
	std::copy(inText, inText + sizeof(wchar_t), reinterpret_cast<char*>(&outUnicode));
	outLength = sizeof(wchar_t);
}

template<>
template<class ByteIterator>
uint32
MEncodingTraits<kEncodingUCS2>::WriteUnicode(
	ByteIterator&		inText,
	wchar_t				inUnicode)
{
	char* p = reinterpret_cast<char*>(&inUnicode);
	std::copy(p, p + sizeof(wchar_t), inText);
	return sizeof(wchar_t);
}

// MacOS Roman

template<>
template<class ByteIterator>
uint32
MEncodingTraits<kEncodingMacOSRoman>::GetNextCharLength(
	ByteIterator		inText)
{
	return 1;
}

template<>
template<class ByteIterator>
void
MEncodingTraits<kEncodingMacOSRoman>::ReadUnicode(
	ByteIterator		inText,
	uint32&				outLength,
	wchar_t&			outUnicode)
{
	outUnicode = MUnicodeMapping::GetUnicode(kEncodingMacOSRoman, *inText);
	outLength = 1;
}

template<>
template<class ByteIterator>
uint32
MEncodingTraits<kEncodingMacOSRoman>::WriteUnicode(
	ByteIterator&		inText,
	wchar_t				inUnicode)
{
	char ch = MUnicodeMapping::GetChar(kEncodingMacOSRoman, inUnicode);
	*inText++ = ch;
	return 1;
}

// ISO-8859-1
// wow, this one is simple...

template<>
template<class ByteIterator>
uint32
MEncodingTraits<kEncodingISO88591>::GetNextCharLength(
	ByteIterator		inText)
{
	return 1;
}

template<>
template<class ByteIterator>
void
MEncodingTraits<kEncodingISO88591>::ReadUnicode(
	ByteIterator		inText,
	uint32&				outLength,
	wchar_t&			outUnicode)
{
	outUnicode = *inText;
	outLength = 1;
}

template<>
template<class ByteIterator>
uint32
MEncodingTraits<kEncodingISO88591>::WriteUnicode(
	ByteIterator&		inText,
	wchar_t				inUnicode)
{
	*inText++ = static_cast<char>(inUnicode);
	return 1;
}
