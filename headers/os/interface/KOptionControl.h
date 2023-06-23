/*
 * Copyright 2001-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_OPTION_CONTROL_H
#define _K_OPTION_CONTROL_H


#include <KControl.h>


enum {
	B_OPTION_CONTROL_VALUE = '_BMV'
};


class KOptionControl : public KControl {
public:
								KOptionControl(BRect frame, const char* name,
									const char* label, BMessage* message,
									uint32 resizeMask = B_FOLLOW_LEFT_TOP,
									uint32 flags = B_WILL_DRAW);
								KOptionControl(const char* name,
									const char* label, BMessage* message,
									uint32 flags = B_WILL_DRAW);

	virtual						~KOptionControl();

	virtual	void				MessageReceived(BMessage* message);

			status_t			AddOption(const char* name, int32 value);
	virtual	bool				GetOptionAt(int32 index, const char** _name,
									int32* _value) = 0;
	virtual	void				RemoveOptionAt(int32 index) = 0;
	virtual	int32				CountOptions() const = 0;
	virtual	status_t			AddOptionAt(const char* name, int32 value,
									int32 index) = 0;
	virtual	int32				SelectedOption(const char** name = NULL,
									int32* outValue = NULL) const = 0;

	virtual	status_t			SelectOptionFor(int32 value);
	virtual	status_t			SelectOptionFor(const char* name);

protected:
			BMessage*			MakeValueMessage(int32 value);

private:
	// FBC padding and forbidden methods
								KOptionControl();
								KOptionControl(const KOptionControl& other);
			KOptionControl&		operator=(const KOptionControl& other);

	virtual	status_t			_Reserved_OptionControl_0(void*, ...);
	virtual	status_t			_Reserved_OptionControl_1(void*, ...);
	virtual	status_t			_Reserved_OptionControl_2(void*, ...);
	virtual	status_t			_Reserved_OptionControl_3(void*, ...);
	virtual	status_t			_Reserved_OptionControl_4(void*, ...);
	virtual	status_t			_Reserved_OptionControl_5(void*, ...);
	virtual	status_t			_Reserved_OptionControl_6(void*, ...);
	virtual	status_t			_Reserved_OptionControl_7(void*, ...);
	virtual	status_t			_Reserved_OptionControl_8(void*, ...);
	virtual	status_t			_Reserved_OptionControl_9(void*, ...);
	virtual	status_t			_Reserved_OptionControl_10(void*, ...);
	virtual	status_t			_Reserved_OptionControl_11(void*, ...);

private:
			uint32				_reserved[8];
};


#endif // _OPTION_CONTROL_H
