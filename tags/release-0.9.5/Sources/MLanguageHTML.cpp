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

/*	$Id: MLanguageHTML.cpp 48 2005-08-03 08:06:02Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 28 2004 15:26:34
*/

#include "MJapi.h"

#include "MLanguageHTML.h"
#include "MTextBuffer.h"
#include "MUnicode.h"
#include "MFile.h"
#include "MSelection.h"

#include <stack>
#include <cassert>

using namespace std;

enum State
{
	START,
		ELEMENT_START,
			
			COMMENT_DTD,
			COMMENT,
		
			END_ELEMENT_START,
				
				END_ELEMENT_KEYWORD,
				END_ELEMENT_REMAINDER,
			
			ELEMENT_KEYWORD,
				ELEMENT,
					ELEMENT_ATTRIBUTE_KEYWORD,
						ELEMENT_ATTRIBUTE_ASSIGNMENT,
							ELEMENT_ATTRIBUTE_VALUE,
							ELEMENT_ATTRIBUTE_STRING_SINGLE_QUOTED,
							ELEMENT_ATTRIBUTE_STRING_DOUBLE_QUOTED,

	SPECIAL,
	
	CSS_START,
		CSS_COMMENT_OUTSIDE_BLOCK,
		CSS_BLOCK,
			CSS_COMMENT_INSIDE_BLOCK,
			CSS_KEYWORD,
				CSS_ASSIGNMENT,
					CSS_COMMENT_INSIDE_ASSIGNMENT,
					CSS_VALUE,
						CSS_VALUE_KEYWORD,
						CSS_VALUE_DOUBLE_QUOTED_STRING,
						CSS_VALUE_SINGLE_QUOTED_STRING,
						CSS_COLOR,
	
	JAVASCRIPT_START,
		JAVASCRIPT_IDENTIFIER,
		JAVASCRIPT_COMMENT,
		JAVASCRIPT_STRING_SINGLE_QUOTED,
		JAVASCRIPT_STRING_DOUBLE_QUOTED

};

const uint32
	kHTMLKeyWordMask =			1 << 0,
	kHTMLAttributeMask =		1 << 1,
	kCSSKeyWordMask =			1 << 2,
	kCSSAttributeMask =			1 << 3,
	kJavaScriptKeyWordMask =	1 << 4;

const char
	kCommentPrefix[] = "<!--",
	kCommentPostfix[] = "-->";

MLanguageHTML::MLanguageHTML()
{
}

void
MLanguageHTML::Init()
{
	const char* keywords[] = {
		"a", "abbr", "acronym", "address", "applet", "area", "b", "base", "basefont", "bdo",
		"big", "blockquote", "body", "br", "button", "caption",
		"center", "cite", "code", "col", "colgroup", "dd", "del", "dfn", "dir", "div",
		"dl", "dt", "em", "fieldset", "font", "form", "frame", "frameset", "h1",
		"h2", "h3", "h4", "h5", "h6", "head", "hr", "html", "i", "iframe", "img",
		"input", "ins", "isindex", "kbd", "label", "legend", "li", "link", "map", "menu",
		"meta", "noframes", "noscript", "object", "ol", "optgroup", "option", "p",
		"param", "pre", "q", "s", "samp", "script", "select", "small", "span",
		"strike", "strong", "style", "sub", "sup", "table", "tbody", "td", "textarea",
		"tfoot", "th", "thead", "title", "tr", "tt", "u", "ul", "var",
		nil
	};
	
	const char* attributes[] = {
		"abbr", "accept-charset", "accept", "accesskey", "action", "align", "alink",
		"alt", "archive", "axis", "background", "bgcolor", "border", "cellpadding",
		"cellspacing", "char", "charoff", "charset", "checked", "cite", "class",
		"classid", "clear", "code", "codebase", "codetype", "color", "cols", "colspan",
		"compact", "content", "coords", "data", "datetime", "declare", "defer", "dir",
		"disabled", "enctype", "face", "for", "frame", "frameborder", "headers",
		"height", "href", "hreflang", "hspace", "http-equiv", "id", "ismap", "label",
		"lang", "language", "link", "longdesc", "marginheight", "marginwidth",
		"maxlength", "media", "method", "multiple", "name", "nohref", "noresize",
		"noshade", "nowrap", "object", "onblur", "onchange", "onclick", "ondblclick",
		"onfocus", "onkeydown", "onkeypress", "onkeyup", "onload", "onmousedown",
		"onmousemove", "onmouseout", "onmouseover", "onmouseup", "onreset", "onselect",
		"onsubmit", "onunload", "profile", "prompt", "readonly", "rel", "rev", "rows",
		"rowspan", "rules", "scheme", "scope", "scrolling", "selected", "shape",
		"size", "span", "src", "standby", "start", "style", "summary", "tabindex",
		"target", "text", "title", "type", "usemap", "valign", "value", "valuetype",
		"version", "vlink", "vspace", "width",
		nil
	};

	const char* css_keywords[] = {
		"azimuth", "background", "background-attachment", "background-color", "background-image",
		"background-position", "background-repeat", "border", "border-bottom", "border-bottom-color",
		"border-bottom-style", "border-bottom-width", "border-collapse", "border-color", "border-left",
		"border-left-color", "border-left-style", "border-left-width", "border-right", "border-right-color",
		"border-right-style", "border-right-width", "border-spacing", "border-style", "border-top",
		"border-top-color", "border-top-style", "border-top-width", "border-width", "bottom",
		"caption-side", "clear", "clip", "color", "content", "counter-increment", "counter-reset", "cue", "cue-after",
		"cue-before", "cursor", "direction", "display", "elevation", "empty-cells", "float", "font", "font-family",
		"font-size", "font-size-adjust", "font-stretch", "font-style", "font-variant", "font-weight",
		"height", "left", "letter-spacing", "line-height", "list-style", "list-style-image", "list-style-position",
		"list-style-type", "margin", "margin-bottom", "margin-color", "margin-left", "margin-right",
		"margin-top", "marker-offset", "marks", "max-height", "max-width", "min-height", "min-width", "orphans",
		"outline", "outline-color", "outline-style", "outline-width", "overflow", "padding", "padding-bottom",
		"padding-left", "padding-right", "padding-top", "page", "page-break-after", "page-break-before",
		"page-break-inside", "pause", "pause-after", "pause-before", "pitch", "pitch-range", "play-during",
		"position", "quotes", "richness", "right", "size", "speak", "speak-header", "speak-numeral",
		"speak-punctuation", "speech-rate", "stress", "table-layout", "text-align", "text-decoration",
		"text-indent", "text-shadow", "text-transform", "top", "unicode-bidi", "vertical-align",
		"visibility", "voice-family", "volume", "white-space", "widows", "width", "word-spacing", "z-index",
		nil
	};
	
	const char* css_attributes[] = {
		"above", "absolute", "always", "armenian", "auto", "avoid", "background-attachment", "background-color",
		"background-image", "background-position", "background-repeat", "baseline", "behind", "below",
		"bidi-override", "blink", "block", "bold", "bolder", "border-style", "border-top-width", "border-width",
		"both", "bottom", "capitalize", "caption", "center", "center-left", "center-right", "circle",
		"cjk-ideographic", "close-quote", "code", "collapse", "compact", "condensed", "continuous", "crop", "cross",
		"crosshair", "cue-after", "cue-before", "decimal", "decimal-leading-zero", "default", "digits", "disc",
		"e-resize", "embed", "expanded", "extra-condensed", "extra-expanded", "far-left", "far-right", "fast",
		"faster", "fixed", "font-family", "font-size", "font-style", "font-variant", "font-weight", "georgian",
		"hebrew", "help", "hidden", "hide", "high", "higher", "hiragana", "hiragana-iroha", "icon", "inherit", "inline",
		"inline-table", "inside", "invert", "italic", "justify", "katakana", "katakana-iroha", "landscape", "left",
		"left-side", "leftwards", "level", "lighter", "line-height", "line-through", "list-item",
		"list-style-image", "list-style-position", "list-style-type", "loud", "low", "lower", "lower-alpha",
		"lower-greek", "lower-latin", "lower-roman", "lowercase", "ltr", "marker", "medium", "menu", "message-box",
		"middle", "move", "n-resize", "narrower", "ne-resize", "no-close-quote", "no-open-quote", "no-repeat",
		"none", "normal", "nowrap", "nw-resize", "oblique", "once", "open-quote", "outline-color", "outline-style",
		"outline-width", "outside", "overline", "pointer", "portrait", "pre", "relative", "repeat", "repeat-x",
		"repeat-y", "repeat?", "right", "right-side", "rightwards", "rtl", "run-in", "s-resize", "scroll",
		"se-resize", "semi-condensed", "semi-expanded", "separate", "show", "silent", "slow", "slower", "small-caps",
		"small-caption", "soft", "solid", "spell-out", "square", "static", "status-bar", "sub", "super", "sw-resize", "table",
		"table-caption", "table-cell", "table-column", "table-column-group", "table-footer-group",
		"table-header-group", "table-row", "table-row-group", "text", "text-bottom", "text-top", "top",
		"transparent", "ultra-condensed", "ultra-expanded", "underline", "upper-alpha", "upper-latin",
		"upper-roman", "uppercase", "visible", "w-resize", "wait", "wider", "x-fast", "x-high", "x-loud", "x-low",
		"x-slow", "x-soft", "z-index",
		
		"maroon", "red", "orange", "yellow", "olive", "purple", "fuchsia", "white", "lime",
		"green", "navy", "blue", "aqua", "teal", "black", "silver", "gray",

		"serif", "sans-serif", "cursive", "fantasy", "monospace",

		nil
	};
	
	const char* js_keywords[] = {
		"abstract", "boolean", "break", "byte", "case", "catch", "char", "class", "const", "continue",
		"debugger", "default", "delete", "do", "document", "double", "else", "enum", "export", "extends",
		"false", "extends", "false", "final", "finally", "float", "for", "function", "goto", "if", "implements",
		"import", "in", "instanceof", "", "int", "interface", "long", "native", "new", "null", "package",
		"private", "protected", "public", "return", "short", "static", "super", "switch", "synchonized",
		"this", "throw", "throws", "transient", "true", "try", "typeof", "var", "void", "while", "window", "with",
		nil
	};
	
	AddKeywords(keywords);
	AddKeywords(attributes);
	AddKeywords(css_keywords);
	AddKeywords(css_attributes);
	AddKeywords(js_keywords);

	GenerateDFA();
}

void
MLanguageHTML::StyleLine(
	const MTextBuffer&	inText,
	uint32				inOffset,
	uint32				inLength,
	uint16&				ioState)
{
	MTextBuffer::const_iterator text = inText.begin() + inOffset;
	MTextBuffer::const_iterator end = inText.end();
	uint32 i = 0, s = 0, kws = 0;
	bool leave = false, style, script, flagX, escape = false;
	
	SetStyle(0, kLTextColor);
	
	if (inLength == 0)
		return;
	
	enum {
		kStyleBit = sizeof(ioState) * 8 - 1,
		kFlagXBit = kStyleBit - 1,
		kScriptBit = kFlagXBit - 1
	};
	
	const uint16
		kStyleMask =	1 << kStyleBit,
		kFlagXMask =	1 << kFlagXBit,
		kScriptMask =	1 << kScriptBit,
		kStateMask =	static_cast<uint16>(~(kStyleMask | kFlagXMask | kScriptMask));

	style =		(ioState & kStyleMask);
	flagX =	(ioState & kFlagXMask);
	script =	(ioState & kScriptMask);

	ioState &= kStateMask;

	uint32 maxOffset = inOffset + inLength;
	
	while (not leave and i < maxOffset)
	{
		char c = 0;
		if (i < inLength)
			c = text[i];
		++i;
		
		switch (ioState)
		{
			case START:
				if (c == '<')
					ioState = ELEMENT_START;
				else if (c == '&')
					ioState = SPECIAL;
				else if (c == 0 or c == '\n')
					leave = true;
					
				if ((leave or ioState != START) and s < i)
				{
					SetStyle(s, kLTextColor);
					s = i - 1;
				}
				break;

			case SPECIAL:
				if (c == 0 or c == '\n')
				{
					SetStyle(s, kLTextColor);
					ioState = START;
					leave = true;
				}
				else if (c == ';')
				{
					SetStyle(s, kLCharConstColor);
					s = i;
					ioState = START;
				}
				else if (isspace(c))
					ioState = START;
				break;
			
			case ELEMENT_START:
				switch (c)
				{
					case 0:
					case '\n':
						SetStyle(s, kLTagColor);
						leave = true;
						break;

					case '>':
						SetStyle(s, kLTagColor);
						s = i;
						SetStyle(s, kLTextColor);
						ioState = START;
						break;

					case '!':
						if (i == s + 2)
						{
							SetStyle(s, kLTagColor);
							ioState = COMMENT_DTD;
						}
						break;
					
					case '/':
						ioState = END_ELEMENT_START;
						break;

					default:
						if (isalpha(c))
						{
							SetStyle(s, kLTagColor);
							s = i - 1;
							kws = Move(tolower(c), 1);
							ioState = ELEMENT_KEYWORD;
						}
						break;
				}
				break;

			case COMMENT_DTD:
				if (c == '-' and text[i] == '-' and i == s + 3 and text[i - 2] == '!' and text[i - 3] == '<')
				{
					SetStyle(s, kLTagColor);
					s = i - 1;
					ioState = COMMENT;
				}
				else if (c == '>')
				{
					SetStyle(s, kLTagColor);
					s = i;
					ioState = START;
				}
				else if (c == 0 or c == '\n')
				{
					SetStyle(s, kLTagColor);
					leave = true;
				}
				break;
				
			case COMMENT:
				if (c == '-' and text[i] == '-')
				{
					SetStyle(s, kLCommentColor);
					s = ++i;
					ioState = COMMENT_DTD;
				}
				else if (c == 0 or c == '\n')
				{
					SetStyle(s, kLCommentColor);
					leave = true;
				}
				break;

			case ELEMENT_KEYWORD:
				if (not isalnum(c))
				{
					MTextBuffer::const_iterator w(text + s);
					
					if (w == "script")
					{
						SetStyle(s, kLKeyWordColor);
						script = true;
					}
					else if (w == "style")
					{
						SetStyle(s, kLKeyWordColor);
						style = true;
					}
					else if (IsKeyWord(kws) & kHTMLKeyWordMask)
						SetStyle(s, kLKeyWordColor);
					else
						SetStyle(s, kLTagColor);
					
					s = --i;
					ioState = ELEMENT;
				}
				else if (kws)
					kws = Move(tolower(c), kws);
				break;
			
			case ELEMENT:
				switch (c)
				{
					case 0:
					case '\n':
						SetStyle(s, kLTagColor);
						leave = true;
						break;

					case '>':
						SetStyle(s, kLTagColor);
						s = i;
						SetStyle(s, kLTextColor);
						
						if (style)
							ioState = CSS_START;
						else if (script)
							ioState = JAVASCRIPT_START;
						else
							ioState = START;
						break;

					default:
						if (isalpha(c))
						{
							SetStyle(s, kLTagColor);
							s = i - 1;
							kws = Move(tolower(c), 1);
							ioState = ELEMENT_ATTRIBUTE_KEYWORD;
						}
						break;
				}
				break;

			case ELEMENT_ATTRIBUTE_KEYWORD:
				if (not isalnum(c))
				{
					if (IsKeyWord(kws) & kHTMLAttributeMask)
						SetStyle(s, kLAttribColor);
					else
						SetStyle(s, kLTagColor);

					s = --i;
					ioState = ELEMENT_ATTRIBUTE_ASSIGNMENT;
				}
				else if (kws)
					kws = Move(tolower(c), kws);
				break;
			
			case ELEMENT_ATTRIBUTE_ASSIGNMENT:
				if (c == '\n' or c == 0)
				{
					SetStyle(s, kLTagColor);
					leave = true;
				}
				else if (c == '=')
				{
					SetStyle(s, kLTagColor);
					ioState = ELEMENT_ATTRIBUTE_VALUE;
					s = i;
				}
				else if (not isspace(c))
				{
					s = --i;
					ioState = ELEMENT;
				}				
				break;
			
			case ELEMENT_ATTRIBUTE_VALUE:
				if (c == '\'')
					ioState = ELEMENT_ATTRIBUTE_STRING_SINGLE_QUOTED;
				else if (c == '\"')
					ioState = ELEMENT_ATTRIBUTE_STRING_DOUBLE_QUOTED;
				else if (not isspace(c))
				{
					s = --i;
					ioState = ELEMENT;
				}
				break;
			
			case ELEMENT_ATTRIBUTE_STRING_DOUBLE_QUOTED:
				if (c == '"')
				{
					SetStyle(s, kLStringColor);
					s = i;
					ioState = ELEMENT;
				}
				else if (c == '\n' or c == 0)
				{
					SetStyle(s, kLStringColor);
					leave = true;
				}
				break;
			
			case ELEMENT_ATTRIBUTE_STRING_SINGLE_QUOTED:
				if (c == '\'')
				{
					SetStyle(s, kLStringColor);
					s = i;
					ioState = ELEMENT;
				}
				else if (c == '\n' or c == 0)
				{
					SetStyle(s, kLStringColor);
					leave = true;
				}
				break;

			case END_ELEMENT_START:
				switch (c)
				{
					case 0:
					case '\n':
						SetStyle(s, kLTagColor);
						leave = true;
						break;

					case '>':
						SetStyle(s, kLTagColor);
						s = i;
						SetStyle(s, kLTextColor);
						ioState = START;
						break;

					default:
						if (isalpha(c))
						{
							SetStyle(s, kLTagColor);
							s = i - 1;
							kws = Move(tolower(c), 1);
							ioState = END_ELEMENT_KEYWORD;
						}
						break;
				}
				break;
				
			case END_ELEMENT_KEYWORD:
				if (not isalnum(c))
				{
					if (IsKeyWord(kws) & kHTMLKeyWordMask)
						SetStyle(s, kLKeyWordColor);
					else
						SetStyle(s, kLTagColor);
					
					s = --i;
					ioState = END_ELEMENT_REMAINDER;
				}
				else if (kws)
					kws = Move(tolower(c), kws);
				break;
			
				break;

			case END_ELEMENT_REMAINDER:
				switch (c)
				{
					case 0:
					case '\n':
						SetStyle(s, kLTagColor);
						leave = true;
						break;

					case '>':
						SetStyle(s, kLTagColor);
						s = i;
						SetStyle(s, kLTextColor);
						ioState = START;
						break;
				}
				break;
			
			case CSS_START:
				style = false;
				
				if (c == '<' and (text + i) == "!--")
				{
					SetStyle(s, kLTagColor);
					SetStyle(s + 1, kLCommentColor);
					i += 3;
					s = i;
				}
				else if (c == '<' and (text + i) == "/style")
				{
					s = --i;
					ioState = ELEMENT_START;
				}
				else if (c == '{')
					ioState = CSS_BLOCK;
				else if (c == '/' and text[i] == '*')
					ioState = CSS_COMMENT_OUTSIDE_BLOCK;
				else if (c == 0 or c == '\n')
					leave = true;
				
				if ((leave or ioState != CSS_START) and s < i)
				{
					SetStyle(s, kLTextColor);
					s = --i;
				}
				break;

			case CSS_COMMENT_OUTSIDE_BLOCK:
			case CSS_COMMENT_INSIDE_BLOCK:
			case CSS_COMMENT_INSIDE_ASSIGNMENT:
				if (c == '*' and text[i] == '/' and (s == 0 or s + 2 < i))
				{
					SetStyle(s, kLCommentColor);
					s = ++i;
					
					if (ioState == CSS_COMMENT_OUTSIDE_BLOCK)
						ioState = CSS_START;
					else if (ioState == CSS_COMMENT_INSIDE_BLOCK)
						ioState = CSS_BLOCK;
					else
						ioState = CSS_ASSIGNMENT;
				}
				else if (c == 0 || c == '\n')
				{
					SetStyle(s, kLCommentColor);
					leave = true;
				}
				break;
			
			case CSS_BLOCK:
				if (c == '}' or c == '<')
				{
					s = --i;
					ioState = CSS_START;
				}
				else if (c == '/' and text[i] == '*')
					ioState = CSS_COMMENT_INSIDE_BLOCK;
				else if (isalpha(c))
				{
					kws = 1;
					ioState = CSS_KEYWORD;
				}
				else if (c == 0 or c == '\n')
					leave = true;
				
				if ((leave or ioState != CSS_BLOCK) and s < i)
				{
					SetStyle(s, kLTextColor);
					s = --i;
				}
				break;
			
			case CSS_KEYWORD:
				if (not isalnum(c) and c != '-')
				{
					if (s + 1 < i and IsKeyWord(kws) & kCSSKeyWordMask)
						SetStyle(s, kLKeyWordColor);
					else
						SetStyle(s, kLTextColor);
					
					s = --i;
					ioState = CSS_ASSIGNMENT;
				}
				else if (kws)
					kws = Move(tolower(c), kws);
				break;
			
			case CSS_ASSIGNMENT:
				if (c == 0 or c == '\n')
					leave = true;
				else if (c == ':')
				{
					SetStyle(s, kLTextColor);
					ioState = CSS_VALUE;
					s = i;
				}
				else if (not isspace(c))
				{
					ioState = CSS_BLOCK;
					s = --i;
				}
				break;
			
			case CSS_VALUE:
				if (c == '}' or c == '<')
					ioState = CSS_START;
				else if (c == '#')
					ioState = CSS_COLOR;
				else if (c == ';')
					ioState = CSS_BLOCK;
				else if (c == '"')
					ioState = CSS_VALUE_DOUBLE_QUOTED_STRING;
				else if (c == '\'')
					ioState = CSS_VALUE_SINGLE_QUOTED_STRING;
				else if (c == '/' and text[i] == '*')
					ioState = CSS_COMMENT_INSIDE_ASSIGNMENT;
				else if (isalpha(c))
				{
					kws = 1;
					ioState = CSS_VALUE_KEYWORD;
				}
				else if (c == 0 or c == '\n')
					leave = true;
				
				if ((leave or ioState != CSS_VALUE) and s < i)
				{
					SetStyle(s, kLTextColor);
					s = --i;
				}
				break;

			case CSS_VALUE_KEYWORD:
				if (not isalnum(c) and c != '-')
				{
					if (s + 1 < i and IsKeyWord(kws) & kCSSAttributeMask)
						SetStyle(s, kLAttribColor);
					else
						SetStyle(s, kLTextColor);
					
					s = --i;
					ioState = CSS_VALUE;
				}
				else if (kws)
					kws = Move(tolower(c), kws);
				break;
			
			case CSS_COLOR:
				if (s + 1 < i and not isxdigit(c))
				{
					if (i - s == 5 or i - s == 8)
						SetStyle(s, kLCharConstColor);
					else
						SetStyle(s, kLTextColor);

					s = --i;
					ioState = CSS_VALUE;
				}
				break;
			
			case CSS_VALUE_DOUBLE_QUOTED_STRING:
				if (s + 1 < i and c == '"' and text[i - 2] != '\\')
				{
					SetStyle(s, kLStringColor);
					s = i;
					ioState = CSS_VALUE;
				}
				else if (c == '\n' or c == 0)
				{
					SetStyle(s, kLStringColor);
					if (c == '\n' and text[i - 2] != '\\')
						ioState = CSS_VALUE;
					leave = true;
				}
				break;
			
			case CSS_VALUE_SINGLE_QUOTED_STRING:
				if (s + 1 < i and c == '\'' and text[i - 2] != '\\')
				{
					SetStyle(s, kLStringColor);
					s = i;
					ioState = CSS_VALUE;
				}
				else if (c == '\n' or c == 0)
				{
					SetStyle(s, kLStringColor);
					if (c == '\n' and text[i - 2] != '\\')
						ioState = CSS_VALUE;
					leave = true;
				}
				break;
			
			// Javascript part
			
			case JAVASCRIPT_START:
				script = false;
				
				if (c == '<' and (text + i) == "!--")
				{
					SetStyle(s, kLTagColor);
					SetStyle(i + 1, kLCommentColor);
					i += 3;
					s = i;
				}
				else if (c == '<' and (text + i) == "/script")
				{
					SetStyle(s, kLTagColor);
					SetStyle(i, kLKeyWordColor);
					i += 7;
					s = i;
					ioState = END_ELEMENT_REMAINDER;
				}
				else if (c == '/' and (text[i] == '*' or text[i] == '/'))
				{
					flagX = text[i] == '*';
					ioState = JAVASCRIPT_COMMENT;
				}
				else if (isalpha(c))
				{
					kws = Move(tolower(c), 1);
					ioState = JAVASCRIPT_IDENTIFIER;
				}
				else if (c == '\'')
					ioState = JAVASCRIPT_STRING_SINGLE_QUOTED;
				else if (c == '"')
					ioState = JAVASCRIPT_STRING_DOUBLE_QUOTED;
				else if (c == 0 or c == '\n')
					leave = true;
					
				if ((leave or ioState != JAVASCRIPT_START) and s < i)
				{
					SetStyle(s, kLTextColor);
					s = i - 1;
				}
				break;
			
			case JAVASCRIPT_COMMENT:
				if (flagX and (s == 0 or s + 1 < i) and c == '*' and text[i] == '/')
				{
					SetStyle(s, kLCommentColor);
					s = i + 1;
					ioState = JAVASCRIPT_START;
				}
//				else if (c == '-' and text[i] == '-')
//				{
//					SetStyle(s, kLCommentColor);
//					s = i + 1;
//					ioState = ELEMENT;
//				}
				else if (c == '<' and (text + i) == "/script")
				{
					SetStyle(s, kLTagColor);
					SetStyle(i, kLKeyWordColor);
					i += 7;
					s = i;
					ioState = END_ELEMENT_REMAINDER;
				}
				else if (c == '\n' or c == 0)
				{
					SetStyle(s, kLCommentColor);
					if (not flagX and c == '\n')
						ioState = JAVASCRIPT_START;
					leave = true;
				}
				break;

			case JAVASCRIPT_IDENTIFIER:
				if (not isalnum(c) and c != '_')
				{
					if (s + 1 < i and IsKeyWord(kws) & kJavaScriptKeyWordMask)
						SetStyle(s, kLKeyWordColor);
					else
						SetStyle(s, kLTextColor);
					
					s = --i;
					ioState = JAVASCRIPT_START;
				}
				else if (kws)
					kws = Move(c, kws);
				break;
			
			case JAVASCRIPT_STRING_DOUBLE_QUOTED:
			case JAVASCRIPT_STRING_SINGLE_QUOTED:
				if (not escape and
					((ioState == JAVASCRIPT_STRING_SINGLE_QUOTED and c == '\'') or
					 (ioState == JAVASCRIPT_STRING_DOUBLE_QUOTED and c == '"')))
				{
					SetStyle(s, kLStringColor);
					s = i;
					ioState = JAVASCRIPT_START;
				}
				else if (c == '\n' or c == 0)
				{
					if (text[-2] == '\\' and text[-3] != '\\')
						SetStyle(s, kLStringColor);
					else
					{
						SetStyle(s, kLTextColor);
						ioState = JAVASCRIPT_START;
					}
					
					leave = true;
				}
				else
					escape = not escape and (c == '\\');
				break;
			
			default:	// error condition, gracefully leave the loop
				leave = true;
				break;
		}
	}
	
	ioState = (ioState & kStateMask) | (style << kStyleBit) | (script << kScriptBit) | (flagX << kFlagXBit);
}

bool
MLanguageHTML::Balance(
	const MTextBuffer&	inText,
	uint32&				ioOffset,
	uint32&				ioLength)
{
	return false;
}

bool
MLanguageHTML::IsBalanceChar(
	wchar_t				inChar)
{
	return false;
}

bool
MLanguageHTML::IsSmartIndentLocation(
	const MTextBuffer&	inText,
	uint32				inOffset)
{
	return false;
}

bool
MLanguageHTML::IsSmartIndentCloseChar(
	wchar_t				inChar)
{
	return false;
}

void
MLanguageHTML::CommentLine(
	string&				ioLine)
{
	ioLine.insert(0, kCommentPrefix);
	ioLine.append(kCommentPostfix);
}

void
MLanguageHTML::UncommentLine(
	string&				ioLine)
{
	uint32 n = sizeof(kCommentPrefix) - 1;
	
	if (ioLine.substr(0, n) == kCommentPrefix)
		ioLine.erase(0, n);
	
	n = sizeof(kCommentPostfix) - 1;
	if (ioLine.substr(ioLine.length() - n, n) == kCommentPostfix)
		ioLine.erase(ioLine.length() - n, n);
}

uint32
MLanguageHTML::MatchLanguage(
	const string&		inFile,
	MTextBuffer&		inText)
{
	uint32 result = 0;
	if (FileNameMatches("*.html;*.htm;*.shtml;*.xhtml", inFile))
	{
		result += 90;
	}
	else if (FileNameMatches("*.css", inFile))
	{
		result += 90;
	}
	else if (FileNameMatches("*.js", inFile))
	{
		result += 90;
	}
	else
	{
		MSelection s(nil);
		
		if (inText.Find(0, "<!DOCTYPE\\s+HTML\\s+",
			kDirectionForward, false, true, s))
		{
			result += 75;
		}			
	}
	return result;
}

bool MLanguageHTML::Softwrap() const
{
	return true;
}

uint16
MLanguageHTML::GetInitialState(
	const string&		inFile,
	MTextBuffer&		inText)
{
	uint16 result = START;
	if (FileNameMatches("*.css", inFile))
		result = CSS_START;
	else if (FileNameMatches("*.js", inFile))
		result = JAVASCRIPT_START;
	return result;
}

