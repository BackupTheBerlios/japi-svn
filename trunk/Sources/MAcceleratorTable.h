#ifndef MACCELERATOR_H
#define MACCELERATOR_H

class MAcceleratorTable
{
  public:

	static MAcceleratorTable&
					Instance();

	void			RegisterAcceleratorKey(
						uint32			inCommand,
						uint32			inKeyValue,
						uint32			inModifiers);

	bool			GetAcceleratorKeyForCommand(
						uint32			inCommand,
						uint32&			outKeyValue,
						uint32&			outModifiers);
	
	bool			IsAcceleratorKey(
						GdkEventKey*	inEvent,
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
