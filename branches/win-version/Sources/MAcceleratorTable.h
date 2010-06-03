//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MACCELERATOR_H
#define MACCELERATOR_H

extern const uint32 kValidModifiersMask;

class MAcceleratorTable
{
  public:

	static MAcceleratorTable&
					Instance();

	static MAcceleratorTable&
					EditKeysInstance();

	void			RegisterAcceleratorKey(
						uint32			inCommand,
						uint32			inKeyValue,
						uint32			inModifiers);

	bool			GetAcceleratorKeyForCommand(
						uint32			inCommand,
						uint32&			outKeyValue,
						uint32&			outModifiers);
	
	bool			IsAcceleratorKey(
						uint32			inKeyCode,
						uint32			inModifiers,
						uint32&			outCommand);

	bool			IsNavigationKey(
						uint32			inKeyValue,
						uint32			inModifiers,
						MKeyCommand&	outCommand);

  private:

					MAcceleratorTable();
					MAcceleratorTable(
						const MAcceleratorTable&);
	MAcceleratorTable&
					operator=(
						const MAcceleratorTable&);
	
	struct MAcceleratorTableImp*	mImpl;
};

#endif
