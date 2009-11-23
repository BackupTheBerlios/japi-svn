//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <tidy/tidy.h>

#include "MXHTMLTools.h"
#include "MError.h"

using namespace std;

namespace MXHTMLTools
{

class MTidy
{
  public:
					MTidy();
					
					~MTidy();

	void			SetText(
						const string&	inText);

	void			Cleanup();

	void			GetErrors(
						Problems&		outProblems);

	void			GetText(
						string&			outText);

  private:

	static void		BackInserter(
						void*			inData,
						byte			inByte)
					{
						string& s = *reinterpret_cast<string*>(inData);
						s += char(inByte);
					}

	static Bool		ReportCallback(
						TidyDoc			tdoc,
						TidyReportLevel	lvl,
						uint			line,
						uint			col,
						ctmbstr			mssg);

	bool			Report(
						TidyDoc			tdoc,
						TidyReportLevel	lvl,
						uint			line,
						uint			col,
						ctmbstr			mssg);

	TidyDoc			mTidyDoc;
	Problems		mProblems;
	TidyOutputSink	mOutSink, mErrSink;
	string			mOut, mErr;
};

MTidy::MTidy()
{
	mTidyDoc = tidyCreate();
	
	tidySetAppData(mTidyDoc, this);
	tidySetReportFilter(mTidyDoc, &MTidy::ReportCallback);
	
	tidyInitSink(&mErrSink, &mErr, &MTidy::BackInserter);
	tidySetErrorSink(mTidyDoc, &mErrSink);
	
	tidyOptSetBool(mTidyDoc, TidyXhtmlOut, yes);
	tidyOptSetBool(mTidyDoc, TidyQuiet, yes);
//	tidyOptSetBool(mTidyDoc, TidyMakeBare, yes);
	tidyOptSetBool(mTidyDoc, TidyLogicalEmphasis, yes);
	tidySetCharEncoding(mTidyDoc, "utf8");
	tidyOptSetInt(mTidyDoc, TidyIndentContent, TidyAutoState);
	tidyOptSetInt(mTidyDoc, TidyIndentSpaces, 2);
	tidyOptSetInt(mTidyDoc, TidyWrapLen, 0);
}
				
MTidy::~MTidy()
{
	tidyRelease(mTidyDoc);
}

Bool MTidy::ReportCallback(
	TidyDoc			tdoc,
	TidyReportLevel	lvl,
	uint			line,
	uint			col,
	ctmbstr			mssg)
{
	MTidy* self = reinterpret_cast<MTidy*>(tidyGetAppData(tdoc));
	
	Bool result = no;
	if (self->Report(tdoc, lvl, line, col, mssg))
		result = yes;
	return result;
}

bool MTidy::Report(
	TidyDoc			tdoc,
	TidyReportLevel	lvl,
	uint			line,
	uint			col,
	ctmbstr			mssg)
{
	Problem err;

	switch (lvl)
	{
		case TidyInfo:		err.kind = info; break;
		case TidyWarning:	err.kind = warning; break;
		case TidyError:		err.kind = error; break;
		default:
			cerr << "Unhandled problem: " << mssg << endl;
			break;
	}
	err.line = line;
	err.column = col;
	err.message = mssg ? mssg : "";
	mProblems.push_back(err);
	
	return true;
}

void MTidy::SetText(
	const string&	inText)
{
	 int err = tidyParseString(mTidyDoc, inText.c_str());
	 if (err < 0)
	 	THROW(("Tidy error parsing text"));
}

void MTidy::GetText(
	string&			outText)
{
	outText.clear();
	
	TidyOutputSink sink;
	tidyInitSink(&sink, &outText, &MTidy::BackInserter);
	
	int err = tidyOptSetBool(mTidyDoc, TidyForceOutput, yes);
	if (err >= 0)
		err = tidySaveSink(mTidyDoc, &sink);
	if (err < 0)
		THROW(("Error saving tidied text"));
}

void MTidy::Cleanup()
{
	int err = tidyCleanAndRepair(mTidyDoc);
	if (err >= 0)
		err = tidyRunDiagnostics(mTidyDoc);
	if (err < 0)
	 	THROW(("Tidy error cleaning text"));
}

void MTidy::GetErrors(
	Problems&		outProblems)
{
	swap(mProblems, outProblems);
}

void ValidateXHTML(
	const std::string&	inText,
	Problems&			outProblems)
{
	MTidy tidy;
	tidy.SetText(inText);
	tidy.Cleanup();
	tidy.GetErrors(outProblems);
}

void ConvertAnyToXHTML(
	std::string&		ioText,
	Problems&			outProblems)
{
	MTidy tidy;
	tidy.SetText(ioText);
	tidy.Cleanup();
	tidy.GetText(ioText);
	tidy.GetErrors(outProblems);
}

}
