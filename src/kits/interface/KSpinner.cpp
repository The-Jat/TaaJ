/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include <KSpinner.h>

#include <stdint.h>
#include <stdlib.h>

#include <PropertyInfo.h>
#include <String.h>
#include <KTextView.h>


static property_info sProperties[] = {
	{
		"MaxValue",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the maximum value of the spinner.",
		0,
		{ B_INT32_TYPE }
	},
	{
		"MaxValue",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the maximum value of the spinner.",
		0,
		{ B_INT32_TYPE }
	},

	{
		"MinValue",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the minimum value of the spinner.",
		0,
		{ B_INT32_TYPE }
	},
	{
		"MinValue",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the minimum value of the spinner.",
		0,
		{ B_INT32_TYPE }
	},

	{
		"Value",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the value of the spinner.",
		0,
		{ B_INT32_TYPE }
	},
	{
		"Value",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the value of the spinner.",
		0,
		{ B_INT32_TYPE }
	},

	{ 0 }
};


//	#pragma mark - KSpinner


KSpinner::KSpinner(BRect frame, const char* name, const char* label,
	BMessage* message, uint32 resizingMode, uint32 flags)
	:
	KAbstractSpinner(frame, name, label, message, resizingMode, flags)
{
	_InitObject();
}


KSpinner::KSpinner(const char* name, const char* label,
	BMessage* message, uint32 flags)
	:
	KAbstractSpinner(name, label, message, flags)
{
	_InitObject();
}


KSpinner::KSpinner(BMessage* data)
	:
	KAbstractSpinner(data)
{
	_InitObject();

	if (data->FindInt32("_min", &fMinValue) != B_OK)
		fMinValue = INT32_MIN;

	if (data->FindInt32("_max", &fMaxValue) != B_OK)
		fMaxValue = INT32_MAX;

	if (data->FindInt32("_val", &fValue) != B_OK)
		fValue = 0;
}


KSpinner::~KSpinner()
{
}


BArchivable*
KSpinner::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "Spinner"))
		return new KSpinner(data);

	return NULL;
}


status_t
KSpinner::Archive(BMessage* data, bool deep) const
{
	status_t status = KAbstractSpinner::Archive(data, deep);
	data->AddString("class", "Spinner");

	if (status == B_OK)
		status = data->AddInt32("_min", fMinValue);

	if (status == B_OK)
		status = data->AddInt32("_max", fMaxValue);

	if (status == B_OK)
		status = data->AddInt32("_val", fValue);

	return status;
}


status_t
KSpinner::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/vnd.Haiku-intenger-spinner");

	BPropertyInfo prop_info(sProperties);
	message->AddFlat("messages", &prop_info);

	return KView::GetSupportedSuites(message);
}


void
KSpinner::AttachedToWindow()
{
	SetValue(fValue);

	KAbstractSpinner::AttachedToWindow();
}


void
KSpinner::Decrement()
{
	SetValue(Value() - 1);
}


void
KSpinner::Increment()
{
	SetValue(Value() + 1);
}


void
KSpinner::SetEnabled(bool enable)
{
	if (IsEnabled() == enable)
		return;

	SetIncrementEnabled(enable && Value() < fMaxValue);
	SetDecrementEnabled(enable && Value() > fMinValue);

	KAbstractSpinner::SetEnabled(enable);
}


void
KSpinner::SetMinValue(int32 min)
{
	fMinValue = min;
	if (fValue < fMinValue)
		SetValue(fMinValue);
}


void
KSpinner::SetMaxValue(int32 max)
{
	fMaxValue = max;
	if (fValue > fMaxValue)
		SetValue(fMaxValue);
}


void
KSpinner::Range(int32* min, int32* max)
{
	*min = fMinValue;
	*max = fMaxValue;
}


void
KSpinner::SetRange(int32 min, int32 max)
{
	SetMinValue(min);
	SetMaxValue(max);
}


void
KSpinner::SetValue(int32 value)
{
	// clip to range
	if (value < fMinValue)
		value = fMinValue;
	else if (value > fMaxValue)
		value = fMaxValue;

	// update the text view
	BString valueString;
	valueString << value;
	TextView()->SetText(valueString.String());

	// update the up and down arrows
	SetIncrementEnabled(IsEnabled() && value < fMaxValue);
	SetDecrementEnabled(IsEnabled() && value > fMinValue);

	if (value == fValue)
		return;

	fValue = value;
	ValueChanged();

	Invoke();
	Invalidate();
}


void
KSpinner::SetValueFromText()
{
	SetValue(atol(TextView()->Text()));
}


//	#pragma mark - KSpinner private methods


void
KSpinner::_InitObject()
{
	fMinValue = INT32_MIN;
	fMaxValue = INT32_MAX;
	fValue = 0;

	TextView()->SetAlignment(B_ALIGN_RIGHT);
	for (uint32 c = 0; c <= 42; c++)
		TextView()->DisallowChar(c);

	TextView()->DisallowChar(',');

	for (uint32 c = 46; c <= 47; c++)
		TextView()->DisallowChar(c);

	for (uint32 c = 58; c <= 127; c++)
		TextView()->DisallowChar(c);
}


// FBC padding

void KSpinner::_ReservedSpinner20() {}
void KSpinner::_ReservedSpinner19() {}
void KSpinner::_ReservedSpinner18() {}
void KSpinner::_ReservedSpinner17() {}
void KSpinner::_ReservedSpinner16() {}
void KSpinner::_ReservedSpinner15() {}
void KSpinner::_ReservedSpinner14() {}
void KSpinner::_ReservedSpinner13() {}
void KSpinner::_ReservedSpinner12() {}
void KSpinner::_ReservedSpinner11() {}
void KSpinner::_ReservedSpinner10() {}
void KSpinner::_ReservedSpinner9() {}
void KSpinner::_ReservedSpinner8() {}
void KSpinner::_ReservedSpinner7() {}
void KSpinner::_ReservedSpinner6() {}
void KSpinner::_ReservedSpinner5() {}
void KSpinner::_ReservedSpinner4() {}
void KSpinner::_ReservedSpinner3() {}
void KSpinner::_ReservedSpinner2() {}
void KSpinner::_ReservedSpinner1() {}
