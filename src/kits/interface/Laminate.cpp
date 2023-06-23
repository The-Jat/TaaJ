/*
 * Copyright 2001-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Axel Dörfler, axeld@pinc-software.de
 *		Adrian Oanca, adioanca@cotty.iren.ro
 *		Ingo Weinhold. ingo_weinhold@gmx.de
 *		Julian Harnath, julian.harnath@rwth-aachen.de
 *		Joseph Groover, looncraz@looncraz.net
 */


#include <Laminate.h>

#include <algorithm>
#include <new>

#include <math.h>
#include <stdio.h>

#include <Application.h>
#include <KBitmap.h>
#include <KButton.h>
#include <Cursor.h>
#include <File.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>
#include <InterfaceDefs.h>
#include <KLayout.h>
#include <LayoutContext.h>
#include <KLayoutUtils.h>

#include <KMenuBar.h>
#include <Message.h>
#include <MessageQueue.h>

#include <ObjectList.h>
#include <KPicture.h>
#include <Point.h>
#include <Polygon.h>
#include <PropertyInfo.h>
#include <Region.h>
#include <KScrollBar.h>
#include <Shape.h>
#include <KShelf.h>
#include <String.h>
#include <Khidki.h>

#include <AppMisc.h>
#include <AppServerLink.h>
#include <binary_compatibility/Interface.h>
#include <binary_compatibility/Support.h>
#include <MessagePrivate.h>
#include <MessageUtils.h>
#include <PortLink.h>
#include <ServerProtocol.h>
#include <ServerProtocolStructs.h>
#include <ShapePrivate.h>
#include <KToolTip.h>
#include <KToolTipManager.h>
#include <TokenSpace.h>
#include <ViewPrivate.h>

using std::nothrow;

//#define DEBUG_BVIEW
#ifdef DEBUG_BVIEW
#	include <stdio.h>
#	define STRACE(x) printf x
#	define BVTRACE _PrintToStream()
#else
#	define STRACE(x) ;
#	define BVTRACE ;
#endif


static property_info sViewPropInfo[] = {
	{ "Frame", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER, 0 }, "The view's frame rectangle.", 0,
		{ B_RECT_TYPE }
	},
	{ "Hidden", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER, 0 }, "Whether or not the view is hidden.",
		0, { B_BOOL_TYPE }
	},
	{ "Shelf", { 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Directs the scripting message to the "
			"shelf.", 0
	},
	{ "View", { B_COUNT_PROPERTIES, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the number of child views.", 0,
		{ B_INT32_TYPE }
	},
	{ "View", { 0 },
		{ B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, B_NAME_SPECIFIER, 0 },
		"Directs the scripting message to the specified view.", 0
	},

	{ 0 }
};


//	#pragma mark -


static inline uint32
get_uint32_color(rgb_color color)
{
	return B_BENDIAN_TO_HOST_INT32(*(uint32*)&color);
		// rgb_color is always in rgba format, no matter what endian;
		// we always return the int32 value in host endian.
}


static inline rgb_color
get_rgb_color(uint32 value)
{
	value = B_HOST_TO_BENDIAN_INT32(value);
	return *(rgb_color*)&value;
}


//	#pragma mark -


namespace BPrivate {

KViewState::KViewState()
{
	pen_location.Set(0, 0);
	pen_size = 1.0;

	// NOTE: the clipping_region is empty
	// on construction but it is not used yet,
	// we avoid having to keep track of it via
	// this flag
	clipping_region_used = false;

	high_color = (rgb_color){ 0, 0, 0, 255 };
	low_color = (rgb_color){ 255, 255, 255, 255 };
	view_color = low_color;
	which_view_color = B_NO_COLOR;
	which_view_color_tint = B_NO_TINT;

	which_high_color = B_NO_COLOR;
	which_high_color_tint = B_NO_TINT;

	which_low_color = B_NO_COLOR;
	which_low_color_tint = B_NO_TINT;

	pattern = B_SOLID_HIGH;
	drawing_mode = B_OP_COPY;

	origin.Set(0, 0);

	line_join = B_MITER_JOIN;
	line_cap = B_BUTT_CAP;
	miter_limit = B_DEFAULT_MITER_LIMIT;
	fill_rule = B_NONZERO;

	alpha_source_mode = B_PIXEL_ALPHA;
	alpha_function_mode = B_ALPHA_OVERLAY;

	scale = 1.0;

	font = *be_plain_font;
	font_flags = font.Flags();
	font_aliasing = false;

	// We only keep the B_VIEW_CLIP_REGION_BIT flag invalidated,
	// because we should get the clipping region from app_server.
	// The other flags do not need to be included because the data they
	// represent is already in sync with app_server - app_server uses the
	// same init (default) values.
	valid_flags = ~B_VIEW_CLIP_REGION_BIT;

	archiving_flags = B_VIEW_FRAME_BIT | B_VIEW_RESIZE_BIT;
}


void
KViewState::UpdateServerFontState(BPrivate::PortLink &link)
{
debug_printf("[KViewState]{UpdateServerFontState} start\n");
	link.StartMessage(AS_VIEW_SET_FONT_STATE);
	link.Attach<uint16>(font_flags);
		// always present

	if (font_flags & B_FONT_FAMILY_AND_STYLE)
		link.Attach<uint32>(font.FamilyAndStyle());

	if (font_flags & B_FONT_SIZE)
		link.Attach<float>(font.Size());

	if (font_flags & B_FONT_SHEAR)
		link.Attach<float>(font.Shear());

	if (font_flags & B_FONT_ROTATION)
		link.Attach<float>(font.Rotation());

	if (font_flags & B_FONT_FALSE_BOLD_WIDTH)
		link.Attach<float>(font.FalseBoldWidth());

	if (font_flags & B_FONT_SPACING)
		link.Attach<uint8>(font.Spacing());

	if (font_flags & B_FONT_ENCODING)
		link.Attach<uint8>(font.Encoding());

	if (font_flags & B_FONT_FACE)
		link.Attach<uint16>(font.Face());

	if (font_flags & B_FONT_FLAGS)
		link.Attach<uint32>(font.Flags());

debug_printf("[KViewState]{UpdateServerFontState} ends\n");
}


void
KViewState::UpdateServerState(BPrivate::PortLink &link)
{
debug_printf("[KViewState]{UpdateServerState}\n");

	UpdateServerFontState(link);

	link.StartMessage(AS_VIEW_SET_STATE_2);

	ViewSetStateInfo info;
	info.penLocation = pen_location;
	info.penSize = pen_size;
	info.highColor = high_color;
	info.lowColor = low_color;
	info.whichHighColor = which_high_color;
	info.whichLowColor = which_low_color;
	info.whichHighColorTint = which_high_color_tint;
	info.whichLowColorTint = which_low_color_tint;
	info.pattern = pattern;
	info.drawingMode = drawing_mode;
	info.origin = origin;
	info.scale = scale;
	info.lineJoin = line_join;
	info.lineCap = line_cap;
	info.miterLimit = miter_limit;
	info.fillRule = fill_rule;
	info.alphaSourceMode = alpha_source_mode;
	info.alphaFunctionMode = alpha_function_mode;
	info.fontAntialiasing = font_aliasing;
	link.Attach<ViewSetStateInfo>(info);

	// BAffineTransform is transmitted as a double array
	double _transform[6];
	if (transform.Flatten(_transform, sizeof(_transform)) != B_OK)
		return;
	link.Attach<double[6]>(_transform);

	// we send the 'local' clipping region... if we have one...
	// TODO: Could be optimized, but is low prio, since most views won't
	// have a custom clipping region.
	if (clipping_region_used) {
		int32 count = clipping_region.CountRects();
		link.Attach<int32>(count);
		for (int32 i = 0; i < count; i++)
			link.Attach<BRect>(clipping_region.RectAt(i));
	} else {
		// no clipping region
		link.Attach<int32>(-1);
	}

	// Although we might have a 'local' clipping region, when we call
	// KView::GetClippingRegion() we ask for the 'global' one and it
	// is kept on server, so we must invalidate B_VIEW_CLIP_REGION_BIT flag

	valid_flags = ~B_VIEW_CLIP_REGION_BIT;


debug_printf("[KViewState]{UpdateServerState}ends\n");
}


void
KViewState::UpdateFrom(BPrivate::PortLink &link)
{
	link.StartMessage(AS_VIEW_GET_STATE_2);

	int32 code;
	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return;

	ViewGetStateInfo info;
	link.Read<ViewGetStateInfo>(&info);

	// set view's font state
	font_flags = B_FONT_ALL;
	font.SetFamilyAndStyle(info.fontID);
	font.SetSize(info.fontSize);
	font.SetShear(info.fontShear);
	font.SetRotation(info.fontRotation);
	font.SetFalseBoldWidth(info.fontFalseBoldWidth);
	font.SetSpacing(info.fontSpacing);
	font.SetEncoding(info.fontEncoding);
	font.SetFace(info.fontFace);
	font.SetFlags(info.fontFlags);

	// set view's state
	pen_location = info.viewStateInfo.penLocation;
	pen_size = info.viewStateInfo.penSize;
	high_color = info.viewStateInfo.highColor;
	low_color = info.viewStateInfo.lowColor;
	pattern = info.viewStateInfo.pattern;
	drawing_mode = info.viewStateInfo.drawingMode;
	origin = info.viewStateInfo.origin;
	scale = info.viewStateInfo.scale;
	line_join = info.viewStateInfo.lineJoin;
	line_cap = info.viewStateInfo.lineCap;
	miter_limit = info.viewStateInfo.miterLimit;
	fill_rule = info.viewStateInfo.fillRule;
	alpha_source_mode = info.viewStateInfo.alphaSourceMode;
	alpha_function_mode = info.viewStateInfo.alphaFunctionMode;
	font_aliasing = info.viewStateInfo.fontAntialiasing;

	// BAffineTransform is transmitted as a double array
	double _transform[6];
	link.Read<double[6]>(&_transform);
	if (transform.Unflatten(B_AFFINE_TRANSFORM_TYPE, _transform,
		sizeof(_transform)) != B_OK) {
		return;
	}

	// read the user clipping
	// (that's NOT the current View visible clipping but the additional
	// user specified clipping!)
	int32 clippingRectCount;
	link.Read<int32>(&clippingRectCount);
	if (clippingRectCount >= 0) {
		clipping_region.MakeEmpty();
		for (int32 i = 0; i < clippingRectCount; i++) {
			BRect rect;
			link.Read<BRect>(&rect);
			clipping_region.Include(rect);
		}
	} else {
		// no user clipping used
		clipping_region_used = false;
	}

	valid_flags = ~B_VIEW_CLIP_REGION_BIT;
}

}	// namespace BPrivate


//	#pragma mark -


// archiving constants
namespace {
	const char* const kSizesField = "KView:sizes";
		// kSizesField = {min, max, pref}
	const char* const kAlignmentField = "KView:alignment";
	const char* const kLayoutField = "KView:layout";
}


struct KView::LayoutData {
	LayoutData()
		:
		fMinSize(),
		fMaxSize(),
		fPreferredSize(),
		fAlignment(),
		fLayoutInvalidationDisabled(0),
		fLayout(NULL),
		fLayoutContext(NULL),
		fLayoutItems(5, false),
		fLayoutValid(true),		// TODO: Rethink these initial values!
		fMinMaxValid(true),		//
		fLayoutInProgress(false),
		fNeedsRelayout(true)
	{
	}

	status_t
	AddDataToArchive(BMessage* archive)
	{
		status_t err = archive->AddSize(kSizesField, fMinSize);

		if (err == B_OK)
			err = archive->AddSize(kSizesField, fMaxSize);

		if (err == B_OK)
			err = archive->AddSize(kSizesField, fPreferredSize);

		if (err == B_OK)
			err = archive->AddAlignment(kAlignmentField, fAlignment);

		return err;
	}

	void
	PopulateFromArchive(BMessage* archive)
	{
		archive->FindSize(kSizesField, 0, &fMinSize);
		archive->FindSize(kSizesField, 1, &fMaxSize);
		archive->FindSize(kSizesField, 2, &fPreferredSize);
		archive->FindAlignment(kAlignmentField, &fAlignment);
	}

	BSize			fMinSize;
	BSize			fMaxSize;
	BSize			fPreferredSize;
	BAlignment		fAlignment;
	int				fLayoutInvalidationDisabled;
	KLayout*		fLayout;
	BLayoutContext*	fLayoutContext;
	BObjectList<KLayoutItem> fLayoutItems;
	bool			fLayoutValid;
	bool			fMinMaxValid;
	bool			fLayoutInProgress;
	bool			fNeedsRelayout;
};


KView::KView(const char* name, uint32 flags, KLayout* layout)
	:
	BHandler(name)
{
	_InitData(BRect(0, 0, -1, -1), name, B_FOLLOW_NONE,
		flags | B_SUPPORTS_LAYOUT);
	SetLayout(layout);
}


KView::KView(BRect frame, const char* name, uint32 resizingMode, uint32 flags)
	:
	BHandler(name)
{
debug_printf("[KView]{KView constructor}\n");

	_InitData(frame, name, resizingMode, flags);
}


KView::KView(BMessage* archive)
	:
	BHandler(BUnarchiver::PrepareArchive(archive))
{
	BUnarchiver unarchiver(archive);
	if (!archive)
		debugger("KView cannot be constructed from a NULL archive.");

	BRect frame;
	archive->FindRect("_frame", &frame);

	uint32 resizingMode;
	if (archive->FindInt32("_resize_mode", (int32*)&resizingMode) != B_OK)
		resizingMode = 0;

	uint32 flags;
	if (archive->FindInt32("_flags", (int32*)&flags) != B_OK)
		flags = 0;

	_InitData(frame, Name(), resizingMode, flags);

	font_family family;
	font_style style;
	if (archive->FindString("_fname", 0, (const char**)&family) == B_OK
		&& archive->FindString("_fname", 1, (const char**)&style) == B_OK) {
		BFont font;
		font.SetFamilyAndStyle(family, style);

		float size;
		if (archive->FindFloat("_fflt", 0, &size) == B_OK)
			font.SetSize(size);

		float shear;
		if (archive->FindFloat("_fflt", 1, &shear) == B_OK
			&& shear >= 45.0 && shear <= 135.0)
			font.SetShear(shear);

		float rotation;
		if (archive->FindFloat("_fflt", 2, &rotation) == B_OK
			&& rotation >=0 && rotation <= 360)
			font.SetRotation(rotation);

		SetFont(&font, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE
			| B_FONT_SHEAR | B_FONT_ROTATION);
	}

	int32 color = 0;
	if (archive->FindInt32("_color", 0, &color) == B_OK)
		SetHighColor(get_rgb_color(color));
	if (archive->FindInt32("_color", 1, &color) == B_OK)
		SetLowColor(get_rgb_color(color));
	if (archive->FindInt32("_color", 2, &color) == B_OK)
		SetViewColor(get_rgb_color(color));

	float tint = B_NO_TINT;
	if (archive->FindInt32("_uicolor", 0, &color) == B_OK
		&& color != B_NO_COLOR) {
		if (archive->FindFloat("_uitint", 0, &tint) != B_OK)
			tint = B_NO_TINT;

		SetHighUIColor((color_which)color, tint);
	}
	if (archive->FindInt32("_uicolor", 1, &color) == B_OK
		&& color != B_NO_COLOR) {
		if (archive->FindFloat("_uitint", 1, &tint) != B_OK)
			tint = B_NO_TINT;

		SetLowUIColor((color_which)color, tint);
	}
	if (archive->FindInt32("_uicolor", 2, &color) == B_OK
		&& color != B_NO_COLOR) {
		if (archive->FindFloat("_uitint", 2, &tint) != B_OK)
			tint = B_NO_TINT;

		SetViewUIColor((color_which)color, tint);
	}

	uint32 evMask;
	uint32 options;
	if (archive->FindInt32("_evmask", 0, (int32*)&evMask) == B_OK
		&& archive->FindInt32("_evmask", 1, (int32*)&options) == B_OK)
		SetEventMask(evMask, options);

	BPoint origin;
	if (archive->FindPoint("_origin", &origin) == B_OK)
		SetOrigin(origin);

	float scale;
	if (archive->FindFloat("_scale", &scale) == B_OK)
		SetScale(scale);

	BAffineTransform transform;
	if (archive->FindFlat("_transform", &transform) == B_OK)
		SetTransform(transform);

	float penSize;
	if (archive->FindFloat("_psize", &penSize) == B_OK)
		SetPenSize(penSize);

	BPoint penLocation;
	if (archive->FindPoint("_ploc", &penLocation) == B_OK)
		MovePenTo(penLocation);

	int16 lineCap;
	int16 lineJoin;
	float lineMiter;
	if (archive->FindInt16("_lmcapjoin", 0, &lineCap) == B_OK
		&& archive->FindInt16("_lmcapjoin", 1, &lineJoin) == B_OK
		&& archive->FindFloat("_lmmiter", &lineMiter) == B_OK)
		SetLineMode((cap_mode)lineCap, (join_mode)lineJoin, lineMiter);

	int16 fillRule;
	if (archive->FindInt16("_fillrule", &fillRule) == B_OK)
		SetFillRule(fillRule);

	int16 alphaBlend;
	int16 modeBlend;
	if (archive->FindInt16("_blend", 0, &alphaBlend) == B_OK
		&& archive->FindInt16("_blend", 1, &modeBlend) == B_OK)
		SetBlendingMode( (source_alpha)alphaBlend, (alpha_function)modeBlend);

	uint32 drawingMode;
	if (archive->FindInt32("_dmod", (int32*)&drawingMode) == B_OK)
		SetDrawingMode((drawing_mode)drawingMode);

	fLayoutData->PopulateFromArchive(archive);

	if (archive->FindInt16("_show", &fShowLevel) != B_OK)
		fShowLevel = 0;

	if (BUnarchiver::IsArchiveManaged(archive)) {
		int32 i = 0;
		while (unarchiver.EnsureUnarchived("_views", i++) == B_OK)
				;
		unarchiver.EnsureUnarchived(kLayoutField);

	} else {
		BMessage msg;
		for (int32 i = 0; archive->FindMessage("_views", i, &msg) == B_OK;
			i++) {
			BArchivable* object = instantiate_object(&msg);
			if (KView* child = dynamic_cast<KView*>(object))
				AddChild(child);
		}
	}
}


BArchivable*
KView::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data , "KView"))
		return NULL;

	return new(std::nothrow) KView(data);
}


status_t
KView::Archive(BMessage* data, bool deep) const
{
	BArchiver archiver(data);
	status_t ret = BHandler::Archive(data, deep);

	if (ret != B_OK)
		return ret;

	if ((fState->archiving_flags & B_VIEW_FRAME_BIT) != 0)
		ret = data->AddRect("_frame", Bounds().OffsetToCopy(fParentOffset));

	if (ret == B_OK)
		ret = data->AddInt32("_resize_mode", ResizingMode());

	if (ret == B_OK)
		ret = data->AddInt32("_flags", Flags());

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_EVENT_MASK_BIT) != 0) {
		ret = data->AddInt32("_evmask", fEventMask);
		if (ret == B_OK)
			ret = data->AddInt32("_evmask", fEventOptions);
	}

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_FONT_BIT) != 0) {
		BFont font;
		GetFont(&font);

		font_family family;
		font_style style;
		font.GetFamilyAndStyle(&family, &style);
		ret = data->AddString("_fname", family);
		if (ret == B_OK)
			ret = data->AddString("_fname", style);
		if (ret == B_OK)
			ret = data->AddFloat("_fflt", font.Size());
		if (ret == B_OK)
			ret = data->AddFloat("_fflt", font.Shear());
		if (ret == B_OK)
			ret = data->AddFloat("_fflt", font.Rotation());
	}

	// colors
	if (ret == B_OK)
		ret = data->AddInt32("_color", get_uint32_color(HighColor()));
	if (ret == B_OK)
		ret = data->AddInt32("_color", get_uint32_color(LowColor()));
	if (ret == B_OK)
		ret = data->AddInt32("_color", get_uint32_color(ViewColor()));

	if (ret == B_OK)
		ret = data->AddInt32("_uicolor", (int32)HighUIColor());
	if (ret == B_OK)
		ret = data->AddInt32("_uicolor", (int32)LowUIColor());
	if (ret == B_OK)
		ret = data->AddInt32("_uicolor", (int32)ViewUIColor());

	if (ret == B_OK)
		ret = data->AddFloat("_uitint", fState->which_high_color_tint);
	if (ret == B_OK)
		ret = data->AddFloat("_uitint", fState->which_low_color_tint);
	if (ret == B_OK)
		ret = data->AddFloat("_uitint", fState->which_view_color_tint);

//	NOTE: we do not use this flag any more
//	if ( 1 ){
//		ret = data->AddInt32("_dbuf", 1);
//	}

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_ORIGIN_BIT) != 0)
		ret = data->AddPoint("_origin", Origin());

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_SCALE_BIT) != 0)
		ret = data->AddFloat("_scale", Scale());

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_TRANSFORM_BIT) != 0) {
		BAffineTransform transform = Transform();
		ret = data->AddFlat("_transform", &transform);
	}

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_PEN_SIZE_BIT) != 0)
		ret = data->AddFloat("_psize", PenSize());

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_PEN_LOCATION_BIT) != 0)
		ret = data->AddPoint("_ploc", PenLocation());

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_LINE_MODES_BIT) != 0) {
		ret = data->AddInt16("_lmcapjoin", (int16)LineCapMode());
		if (ret == B_OK)
			ret = data->AddInt16("_lmcapjoin", (int16)LineJoinMode());
		if (ret == B_OK)
			ret = data->AddFloat("_lmmiter", LineMiterLimit());
	}

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_FILL_RULE_BIT) != 0)
		ret = data->AddInt16("_fillrule", (int16)FillRule());

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_BLENDING_BIT) != 0) {
		source_alpha alphaSourceMode;
		alpha_function alphaFunctionMode;
		GetBlendingMode(&alphaSourceMode, &alphaFunctionMode);

		ret = data->AddInt16("_blend", (int16)alphaSourceMode);
		if (ret == B_OK)
			ret = data->AddInt16("_blend", (int16)alphaFunctionMode);
	}

	if (ret == B_OK && (fState->archiving_flags & B_VIEW_DRAWING_MODE_BIT) != 0)
		ret = data->AddInt32("_dmod", DrawingMode());

	if (ret == B_OK)
		ret = fLayoutData->AddDataToArchive(data);

	if (ret == B_OK)
		ret = data->AddInt16("_show", fShowLevel);

	if (deep && ret == B_OK) {
		for (KView* child = fFirstChild; child != NULL && ret == B_OK;
			child = child->fNextSibling)
			ret = archiver.AddArchivable("_views", child, deep);

		if (ret == B_OK)
			ret = archiver.AddArchivable(kLayoutField, GetLayout(), deep);
	}

	return archiver.Finish(ret);
}


status_t
KView::AllUnarchived(const BMessage* from)
{
	BUnarchiver unarchiver(from);
	status_t err = B_OK;

	int32 count;
	from->GetInfo("_views", NULL, &count);

	for (int32 i = 0; err == B_OK && i < count; i++) {
		KView* child;
		err = unarchiver.FindObject<KView>("_views", i, child);
		if (err == B_OK)
			err = _AddChild(child, NULL) ? B_OK : B_ERROR;
	}

	if (err == B_OK) {
		KLayout*& layout = fLayoutData->fLayout;
		err = unarchiver.FindObject(kLayoutField, layout);
		if (err == B_OK && layout) {
			fFlags |= B_SUPPORTS_LAYOUT;
			fLayoutData->fLayout->SetOwner(this);
		}
	}

	return err;
}


status_t
KView::AllArchived(BMessage* into) const
{
	return BHandler::AllArchived(into);
}


KView::~KView()
{
	STRACE(("KView(%s)::~KView()\n", this->Name()));

	if (fOwner != NULL) {
		debugger("Trying to delete a view that belongs to a window. "
			"Call RemoveSelf first.");
	}

	// we also delete all our children

	KView* child = fFirstChild;
	while (child) {
		KView* nextChild = child->fNextSibling;

		delete child;
		child = nextChild;
	}

	SetLayout(NULL);
	_RemoveLayoutItemsFromLayout(true);

	delete fLayoutData;

	_RemoveSelf();

	if (fToolTip != NULL)
		fToolTip->ReleaseReference();

	if (fVerScroller != NULL)
		fVerScroller->SetTarget((KView*)NULL);
	if (fHorScroller != NULL)
		fHorScroller->SetTarget((KView*)NULL);

	SetName(NULL);

	_RemoveCommArray();
	delete fState;
}


BRect
KView::Bounds() const
{
debug_printf("[KView]{Bounds}\n");

	_CheckLock();

	if (fIsPrinting)
		return fState->print_rect;

	return fBounds;
}


void
KView::_ConvertToParent(BPoint* point, bool checkLock) const
{
	if (!fParent)
		return;

	if (checkLock)
		_CheckLock();

	// - our scrolling offset
	// + our bounds location within the parent
	point->x += -fBounds.left + fParentOffset.x;
	point->y += -fBounds.top + fParentOffset.y;
}


void
KView::ConvertToParent(BPoint* point) const
{
	_ConvertToParent(point, true);
}


BPoint
KView::ConvertToParent(BPoint point) const
{
	ConvertToParent(&point);

	return point;
}


void
KView::_ConvertFromParent(BPoint* point, bool checkLock) const
{
	if (!fParent)
		return;

	if (checkLock)
		_CheckLock();

	// - our bounds location within the parent
	// + our scrolling offset
	point->x += -fParentOffset.x + fBounds.left;
	point->y += -fParentOffset.y + fBounds.top;
}


void
KView::ConvertFromParent(BPoint* point) const
{
	_ConvertFromParent(point, true);
}


BPoint
KView::ConvertFromParent(BPoint point) const
{
	ConvertFromParent(&point);

	return point;
}


void
KView::ConvertToParent(BRect* rect) const
{
	if (!fParent)
		return;

	_CheckLock();

	// - our scrolling offset
	// + our bounds location within the parent
	rect->OffsetBy(-fBounds.left + fParentOffset.x,
		-fBounds.top + fParentOffset.y);
}


BRect
KView::ConvertToParent(BRect rect) const
{
	ConvertToParent(&rect);

	return rect;
}


void
KView::ConvertFromParent(BRect* rect) const
{
	if (!fParent)
		return;

	_CheckLock();

	// - our bounds location within the parent
	// + our scrolling offset
	rect->OffsetBy(-fParentOffset.x + fBounds.left,
		-fParentOffset.y + fBounds.top);
}


BRect
KView::ConvertFromParent(BRect rect) const
{
	ConvertFromParent(&rect);

	return rect;
}


void
KView::_ConvertToScreen(BPoint* point, bool checkLock) const
{
	if (!fParent) {
		if (fOwner)
			fOwner->ConvertToScreen(point);

		return;
	}

	if (checkLock)
		_CheckOwnerLock();

	_ConvertToParent(point, false);
	fParent->_ConvertToScreen(point, false);
}


void
KView::ConvertToScreen(BPoint* point) const
{
	_ConvertToScreen(point, true);
}


BPoint
KView::ConvertToScreen(BPoint point) const
{
	ConvertToScreen(&point);

	return point;
}


void
KView::_ConvertFromScreen(BPoint* point, bool checkLock) const
{
	if (!fParent) {
		if (fOwner)
			fOwner->ConvertFromScreen(point);

		return;
	}

	if (checkLock)
		_CheckOwnerLock();

	_ConvertFromParent(point, false);
	fParent->_ConvertFromScreen(point, false);
}


void
KView::ConvertFromScreen(BPoint* point) const
{
	_ConvertFromScreen(point, true);
}


BPoint
KView::ConvertFromScreen(BPoint point) const
{
	ConvertFromScreen(&point);

	return point;
}


void
KView::ConvertToScreen(BRect* rect) const
{
	BPoint offset(0.0, 0.0);
	ConvertToScreen(&offset);
	rect->OffsetBy(offset);
}


BRect
KView::ConvertToScreen(BRect rect) const
{
	ConvertToScreen(&rect);

	return rect;
}


void
KView::ConvertFromScreen(BRect* rect) const
{
	BPoint offset(0.0, 0.0);
	ConvertFromScreen(&offset);
	rect->OffsetBy(offset);
}


BRect
KView::ConvertFromScreen(BRect rect) const
{
	ConvertFromScreen(&rect);

	return rect;
}


uint32
KView::Flags() const
{
	_CheckLock();
	return fFlags & ~_RESIZE_MASK_;
}


void
KView::SetFlags(uint32 flags)
{
	if (Flags() == flags)
		return;

	if (fOwner) {
		if (flags & B_PULSE_NEEDED) {
			_CheckLock();
			if (fOwner->fPulseRunner == NULL)
				fOwner->SetPulseRate(fOwner->PulseRate());
		}

		uint32 changesFlags = flags ^ fFlags;
		if (changesFlags & (B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE
				| B_FRAME_EVENTS | B_SUBPIXEL_PRECISE
				| B_TRANSPARENT_BACKGROUND)) {
			_CheckLockAndSwitchCurrent();

			fOwner->fLink->StartMessage(AS_VIEW_SET_FLAGS);
			fOwner->fLink->Attach<uint32>(flags);
			fOwner->fLink->Flush();
		}
	}

	/* Some useful info:
		fFlags is a unsigned long (32 bits)
		* bits 1-16 are used for KView's flags
		* bits 17-32 are used for KView' resize mask
		* _RESIZE_MASK_ is used for that. Look into View.h to see how
			it's defined
	*/
	fFlags = (flags & ~_RESIZE_MASK_) | (fFlags & _RESIZE_MASK_);

	fState->archiving_flags |= B_VIEW_FLAGS_BIT;
}


BRect
KView::Frame() const
{
	return Bounds().OffsetToCopy(fParentOffset.x, fParentOffset.y);
}


void
KView::Hide()
{
debug_printf("[KView]{Hide}\n");

	if (fOwner && fShowLevel == 0) {
	debug_printf("[KView]{Hide}fOwner && fShowLevel == 0\n");
		_CheckLockAndSwitchCurrent();
		fOwner->fLink->StartMessage(AS_VIEW_HIDE_2);// tododo done
		fOwner->fLink->Flush();
	}
	fShowLevel++;

	if (fShowLevel == 1)
		_InvalidateParentLayout();

debug_printf("[KView]{Hide}end\n");
}


void
KView::Show()
{
debug_printf("[KView]{Show}\n");

	fShowLevel--;
	if (fOwner && fShowLevel == 0) {
	debug_printf("[KView]{Show}fOwnwe&&fShowLevel==0\n");
		_CheckLockAndSwitchCurrent();
		fOwner->fLink->StartMessage(AS_VIEW_SHOW_2);// tododo done
		fOwner->fLink->Flush();
	}

	if (fShowLevel == 0)
		_InvalidateParentLayout();

debug_printf("[KView]{Show}end\n");
}


bool
KView::IsFocus() const
{
	if (fOwner) {
		_CheckLock();
		return fOwner->CurrentFocus() == this;
	} else
		return false;
}


bool
KView::IsHidden(const KView* lookingFrom) const
{
debug_printf("[KView]{IsHidden}\n");

	if (fShowLevel > 0)
		return true;

	// may we be egocentric?
	if (lookingFrom == this)
		return false;

	// we have the same visibility state as our
	// parent, if there is one
	if (fParent)
		return fParent->IsHidden(lookingFrom);

	// if we're the top view, and we're interested
	// in the "global" view, we're inheriting the
	// state of the window's visibility
	if (fOwner && lookingFrom == NULL)
		return fOwner->IsHidden();

	return false;
}


bool
KView::IsHidden() const
{
debug_printf("[KView]{IsHidden()} Into IsHidden()...\n");

	return IsHidden(NULL);
}


bool
KView::IsPrinting() const
{
	return fIsPrinting;
}


BPoint
KView::LeftTop() const
{
	return Bounds().LeftTop();
}


void
KView::SetResizingMode(uint32 mode)
{
	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_RESIZE_MODE);
		fOwner->fLink->Attach<uint32>(mode);
	}

	// look at SetFlags() for more info on the below line
	fFlags = (fFlags & ~_RESIZE_MASK_) | (mode & _RESIZE_MASK_);
}


uint32
KView::ResizingMode() const
{
	return fFlags & _RESIZE_MASK_;
}


void
KView::SetViewCursor(const BCursor* cursor, bool sync)
{
	if (cursor == NULL || fOwner == NULL)
		return;

	_CheckLock();

	ViewSetViewCursorInfo info;
	info.cursorToken = cursor->fServerToken;
	info.viewToken = _get_object_token_(this);
	info.sync = sync;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_VIEW_CURSOR);
	link.Attach<ViewSetViewCursorInfo>(info);

	if (sync) {
		// Make sure the server has processed the message.
		int32 code;
		link.FlushWithReply(code);
	}
}


void
KView::Flush() const
{
	if (fOwner)
		fOwner->Flush();
}


void
KView::Sync() const
{
	_CheckOwnerLock();
	if (fOwner)
		fOwner->Sync();
}


KWindow*
KView::Window() const
{
	return fOwner;
}


//	#pragma mark - Hook Functions


void
KView::AttachedToWindow()
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::AttachedToWindow()\n", Name()));
}


void
KView::AllAttached()
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::AllAttached()\n", Name()));
}


void
KView::DetachedFromWindow()
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::DetachedFromWindow()\n", Name()));
}


void
KView::AllDetached()
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::AllDetached()\n", Name()));
}


void
KView::Draw(BRect updateRect)
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::Draw()\n", Name()));
}


void
KView::DrawAfterChildren(BRect updateRect)
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::DrawAfterChildren()\n", Name()));
}


void
KView::FrameMoved(BPoint newPosition)
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::FrameMoved()\n", Name()));
}


void
KView::FrameResized(float newWidth, float newHeight)
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::FrameResized()\n", Name()));
}


void
KView::GetPreferredSize(float* _width, float* _height)
{
debug_printf("[KView]{GetPreferredSize}\n");
	STRACE(("\tHOOK: KView(%s)::GetPreferredSize()\n", Name()));

	if (_width != NULL)
		*_width = fBounds.Width();
	if (_height != NULL)
		*_height = fBounds.Height();
}


void
KView::ResizeToPreferred()
{
debug_printf("[KView]{ResizeToPreferred}\n");
	STRACE(("\tHOOK: KView(%s)::ResizeToPreferred()\n", Name()));

	float width;
	float height;
	GetPreferredSize(&width, &height);

	ResizeTo(width, height);
}


void
KView::KeyDown(const char* bytes, int32 numBytes)
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::KeyDown()\n", Name()));

	if (Window())
		Window()->_KeyboardNavigation();
}


void
KView::KeyUp(const char* bytes, int32 numBytes)
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::KeyUp()\n", Name()));
}


void
KView::MouseDown(BPoint where)
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::MouseDown()\n", Name()));
}


void
KView::MouseUp(BPoint where)
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::MouseUp()\n", Name()));
}


void
KView::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::MouseMoved()\n", Name()));
}


void
KView::Pulse()
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::Pulse()\n", Name()));
}


void
KView::TargetedByScrollView(KScrollView* scroll_view)
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::TargetedByScrollView()\n", Name()));
}


void
KView::WindowActivated(bool active)
{
	// Hook function
	STRACE(("\tHOOK: KView(%s)::WindowActivated()\n", Name()));
}


//	#pragma mark - Input Functions


void
KView::BeginRectTracking(BRect startRect, uint32 style)
{
	if (_CheckOwnerLockAndSwitchCurrent()) {
		fOwner->fLink->StartMessage(AS_VIEW_BEGIN_RECT_TRACK);
		fOwner->fLink->Attach<BRect>(startRect);
		fOwner->fLink->Attach<uint32>(style);
		fOwner->fLink->Flush();
	}
}


void
KView::EndRectTracking()
{
	if (_CheckOwnerLockAndSwitchCurrent()) {
		fOwner->fLink->StartMessage(AS_VIEW_END_RECT_TRACK);
		fOwner->fLink->Flush();
	}
}


void
KView::DragMessage(BMessage* message, BRect dragRect, BHandler* replyTo)
{
	if (!message)
		return;

	_CheckOwnerLock();

	// calculate the offset
	BPoint offset;
	uint32 buttons;
	BMessage* current = fOwner->CurrentMessage();
	if (!current || current->FindPoint("be:view_where", &offset) != B_OK)
		GetMouse(&offset, &buttons, false);
	offset -= dragRect.LeftTop();

	if (!dragRect.IsValid()) {
		DragMessage(message, NULL, B_OP_BLEND, offset, replyTo);
		return;
	}

	// TODO: that's not really what should happen - the app_server should take
	// the chance *NOT* to need to drag a whole bitmap around but just a frame.

	// create a drag bitmap for the rect
	KBitmap* bitmap = new(std::nothrow) KBitmap(dragRect, B_RGBA32);
	if (bitmap == NULL)
		return;

	uint32* bits = (uint32*)bitmap->Bits();
	uint32 bytesPerRow = bitmap->BytesPerRow();
	uint32 width = dragRect.IntegerWidth() + 1;
	uint32 height = dragRect.IntegerHeight() + 1;
	uint32 lastRow = (height - 1) * width;

	memset(bits, 0x00, height * bytesPerRow);

	// top
	for (uint32 i = 0; i < width; i += 2)
		bits[i] = 0xff000000;

	// bottom
	for (uint32 i = (height % 2 == 0 ? 1 : 0); i < width; i += 2)
		bits[lastRow + i] = 0xff000000;

	// left
	for (uint32 i = 0; i < lastRow; i += width * 2)
		bits[i] = 0xff000000;

	// right
	for (uint32 i = (width % 2 == 0 ? width : 0); i < lastRow; i += width * 2)
		bits[width - 1 + i] = 0xff000000;

	DragMessage(message, bitmap, B_OP_BLEND, offset, replyTo);
}


void
KView::DragMessage(BMessage* message, KBitmap* image, BPoint offset,
	BHandler* replyTo)
{
	DragMessage(message, image, B_OP_COPY, offset, replyTo);
}


void
KView::DragMessage(BMessage* message, KBitmap* image,
	drawing_mode dragMode, BPoint offset, BHandler* replyTo)
{
	if (message == NULL)
		return;

	if (image == NULL) {
		// TODO: workaround for drags without a bitmap - should not be necessary if
		//	we move the rectangle dragging into the app_server
		image = new(std::nothrow) KBitmap(BRect(0, 0, 0, 0), B_RGBA32);
		if (image == NULL)
			return;
	}

	if (replyTo == NULL)
		replyTo = this;

	if (replyTo->Looper() == NULL)
		debugger("DragMessage: warning - the Handler needs a looper");

	_CheckOwnerLock();

	if (!message->HasInt32("buttons")) {
		BMessage* msg = fOwner->CurrentMessage();
		uint32 buttons;

		if (msg == NULL
			|| msg->FindInt32("buttons", (int32*)&buttons) != B_OK) {
			BPoint point;
			GetMouse(&point, &buttons, false);
		}

		message->AddInt32("buttons", buttons);
	}

	BMessage::Private privateMessage(message);
	privateMessage.SetReply(BMessenger(replyTo, replyTo->Looper()));

	int32 bufferSize = message->FlattenedSize();
	char* buffer = new(std::nothrow) char[bufferSize];
	if (buffer != NULL) {
		message->Flatten(buffer, bufferSize);

		fOwner->fLink->StartMessage(AS_VIEW_DRAG_IMAGE);
		fOwner->fLink->Attach<int32>(image->_ServerToken());
		fOwner->fLink->Attach<int32>((int32)dragMode);
		fOwner->fLink->Attach<BPoint>(offset);
		fOwner->fLink->Attach<int32>(bufferSize);
		fOwner->fLink->Attach(buffer, bufferSize);

		// we need to wait for the server
		// to actually process this message
		// before we can delete the bitmap
		int32 code;
		fOwner->fLink->FlushWithReply(code);

		delete [] buffer;
	} else {
		fprintf(stderr, "KView::DragMessage() - no memory to flatten drag "
			"message\n");
	}

	delete image;
}


void
KView::GetMouse(BPoint* _location, uint32* _buttons, bool checkMessageQueue)
{
	if (_location == NULL && _buttons == NULL)
		return;

	_CheckOwnerLockAndSwitchCurrent();

	uint32 eventOptions = fEventOptions | fMouseEventOptions;
	bool noHistory = eventOptions & B_NO_POINTER_HISTORY;
	bool fullHistory = eventOptions & B_FULL_POINTER_HISTORY;

	if (checkMessageQueue && !noHistory) {
		Window()->UpdateIfNeeded();
		BMessageQueue* queue = Window()->MessageQueue();
		queue->Lock();

		// Look out for mouse update messages

		BMessage* message;
		for (int32 i = 0; (message = queue->FindMessage(i)) != NULL; i++) {
			switch (message->what) {
				case B_MOUSE_MOVED:
				case B_MOUSE_UP:
				case B_MOUSE_DOWN:
					bool deleteMessage;
					if (!Window()->_StealMouseMessage(message, deleteMessage))
						continue;

					if (!fullHistory && message->what == B_MOUSE_MOVED) {
						// Check if the message is too old. Some applications
						// check the message queue in such a way that mouse
						// messages *must* pile up. This check makes them work
						// as intended, although these applications could simply
						// use the version of KView::GetMouse() that does not
						// check the history. Also note that it isn't a problem
						// to delete the message in case there is not a newer
						// one. If we don't find a message in the queue, we will
						// just fall back to asking the app_sever directly. So
						// the imposed delay will not be a problem on slower
						// computers. This check also prevents another problem,
						// when the message that we use is *not* removed from
						// the queue. Subsequent calls to GetMouse() would find
						// this message over and over!
						bigtime_t eventTime;
						if (message->FindInt64("when", &eventTime) == B_OK
							&& system_time() - eventTime > 10000) {
							// just discard the message
							if (deleteMessage)
								delete message;
							continue;
						}
					}
					if (_location != NULL)
						message->FindPoint("screen_where", _location);
					if (_buttons != NULL)
						message->FindInt32("buttons", (int32*)_buttons);
					queue->Unlock();
						// we need to hold the queue lock until here, because
						// the message might still be used for something else

					if (_location != NULL)
						ConvertFromScreen(_location);

					if (deleteMessage)
						delete message;

					return;
			}
		}
		queue->Unlock();
	}

	// If no mouse update message has been found in the message queue,
	// we get the current mouse location and buttons from the app_server

	fOwner->fLink->StartMessage(AS_GET_MOUSE);

	int32 code;
	if (fOwner->fLink->FlushWithReply(code) == B_OK
		&& code == B_OK) {
		BPoint location;
		uint32 buttons;
		fOwner->fLink->Read<BPoint>(&location);
		fOwner->fLink->Read<uint32>(&buttons);
			// TODO: ServerWindow replies with an int32 here

		ConvertFromScreen(&location);
			// TODO: in beos R5, location is already converted to the view
			// local coordinate system, so if an app checks the window message
			// queue by itself, it might not find what it expects.
			// NOTE: the fact that we have mouse coords in screen space in our
			// queue avoids the problem that messages already in the queue will
			// be outdated as soon as a window or even the view moves. The
			// second situation being quite common actually, also with regards
			// to scrolling. An app reading these messages would have to know
			// the locations of the window and view for each message...
			// otherwise it is potentially broken anyways.
		if (_location != NULL)
			*_location = location;
		if (_buttons != NULL)
			*_buttons = buttons;
	} else {
		if (_location != NULL)
			_location->Set(0, 0);
		if (_buttons != NULL)
			*_buttons = 0;
	}
}


void
KView::MakeFocus(bool focus)
{
debug_printf("[KView]{MakeFocus} Into MakeFocus...\n");

	if (fOwner == NULL)
		return;

	// TODO: If this view has focus and focus == false,
	// will there really be no other view with focus? No
	// cycling to the next one?
	KView* focusView = fOwner->CurrentFocus();
	if (focus) {
		// Unfocus a previous focus view
		if (focusView != NULL && focusView != this)
			focusView->MakeFocus(false);

		// if we want to make this view the current focus view
		fOwner->_SetFocus(this, true);
	} else {
		// we want to unfocus this view, but only if it actually has focus
		if (focusView == this)
			fOwner->_SetFocus(NULL, true);
	}
}


KScrollBar*
KView::ScrollBar(orientation direction) const
{
	switch (direction) {
		case B_VERTICAL:
			return fVerScroller;

		case B_HORIZONTAL:
			return fHorScroller;

		default:
			return NULL;
	}
}


void
KView::ScrollBy(float deltaX, float deltaY)
{
	ScrollTo(BPoint(fBounds.left + deltaX, fBounds.top + deltaY));
}


void
KView::ScrollTo(BPoint where)
{
	// scrolling by fractional values is not supported
	where.x = roundf(where.x);
	where.y = roundf(where.y);

	// no reason to process this further if no scroll is intended.
	if (where.x == fBounds.left && where.y == fBounds.top)
		return;

	// make sure scrolling is within valid bounds
	if (fHorScroller) {
		float min, max;
		fHorScroller->GetRange(&min, &max);

		if (where.x < min)
			where.x = min;
		else if (where.x > max)
			where.x = max;
	}
	if (fVerScroller) {
		float min, max;
		fVerScroller->GetRange(&min, &max);

		if (where.y < min)
			where.y = min;
		else if (where.y > max)
			where.y = max;
	}

	_CheckLockAndSwitchCurrent();

	float xDiff = where.x - fBounds.left;
	float yDiff = where.y - fBounds.top;

	// if we're attached to a window tell app_server about this change
	if (fOwner) {
		fOwner->fLink->StartMessage(AS_VIEW_SCROLL);
		fOwner->fLink->Attach<float>(xDiff);
		fOwner->fLink->Attach<float>(yDiff);

		fOwner->fLink->Flush();

//		fState->valid_flags &= ~B_VIEW_FRAME_BIT;
	}

	// we modify our bounds rectangle by deltaX/deltaY coord units hor/ver.
	fBounds.OffsetTo(where.x, where.y);

	// then set the new values of the scrollbars
	if (fHorScroller && xDiff != 0.0)
		fHorScroller->SetValue(fBounds.left);
	if (fVerScroller && yDiff != 0.0)
		fVerScroller->SetValue(fBounds.top);

}


status_t
KView::SetEventMask(uint32 mask, uint32 options)
{
	if (fEventMask == mask && fEventOptions == options)
		return B_OK;

	// don't change the mask if it's zero and we've got options
	if (mask != 0 || options == 0)
		fEventMask = mask | (fEventMask & 0xffff0000);
	fEventOptions = options;

	fState->archiving_flags |= B_VIEW_EVENT_MASK_BIT;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_EVENT_MASK);
		fOwner->fLink->Attach<uint32>(mask);
		fOwner->fLink->Attach<uint32>(options);
		fOwner->fLink->Flush();
	}

	return B_OK;
}


uint32
KView::EventMask()
{
	return fEventMask;
}


status_t
KView::SetMouseEventMask(uint32 mask, uint32 options)
{
	// Just don't do anything if the view is not yet attached
	// or we were called outside of KView::MouseDown()
	if (fOwner != NULL
		&& fOwner->CurrentMessage() != NULL
		&& fOwner->CurrentMessage()->what == B_MOUSE_DOWN) {
		_CheckLockAndSwitchCurrent();
		fMouseEventOptions = options;

		fOwner->fLink->StartMessage(AS_VIEW_SET_MOUSE_EVENT_MASK);
		fOwner->fLink->Attach<uint32>(mask);
		fOwner->fLink->Attach<uint32>(options);
		fOwner->fLink->Flush();
		return B_OK;
	}

	return B_ERROR;
}


//	#pragma mark - Graphic State Functions


void
KView::PushState()
{
	_CheckOwnerLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_VIEW_PUSH_STATE);

	// initialize origin, scale and transform, new states start "clean".
	fState->valid_flags |= B_VIEW_SCALE_BIT | B_VIEW_ORIGIN_BIT
		| B_VIEW_TRANSFORM_BIT;
	fState->scale = 1.0f;
	fState->origin.Set(0, 0);
	fState->transform.Reset();
}


void
KView::PopState()
{
	_CheckOwnerLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_VIEW_POP_STATE);
	_FlushIfNotInTransaction();

	// invalidate all flags (except those that are not part of pop/push)
	fState->valid_flags = B_VIEW_VIEW_COLOR_BIT;
}


void
KView::SetOrigin(BPoint where)
{
	SetOrigin(where.x, where.y);
}


void
KView::SetOrigin(float x, float y)
{
	if (fState->IsValid(B_VIEW_ORIGIN_BIT)
		&& x == fState->origin.x && y == fState->origin.y)
		return;

	fState->origin.x = x;
	fState->origin.y = y;

	if (_CheckOwnerLockAndSwitchCurrent()) {
		fOwner->fLink->StartMessage(AS_VIEW_SET_ORIGIN);
		fOwner->fLink->Attach<float>(x);
		fOwner->fLink->Attach<float>(y);

		fState->valid_flags |= B_VIEW_ORIGIN_BIT;
	}

	// our local coord system origin has changed, so when archiving we'll add
	// this too
	fState->archiving_flags |= B_VIEW_ORIGIN_BIT;
}


BPoint
KView::Origin() const
{
	if (!fState->IsValid(B_VIEW_ORIGIN_BIT)) {
		// we don't keep graphics state information, therefor
		// we need to ask the server for the origin after PopState()
		_CheckOwnerLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_ORIGIN);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK && code == B_OK)
			fOwner->fLink->Read<BPoint>(&fState->origin);

		fState->valid_flags |= B_VIEW_ORIGIN_BIT;
	}

	return fState->origin;
}


void
KView::SetScale(float scale) const
{
	if (fState->IsValid(B_VIEW_SCALE_BIT) && scale == fState->scale)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_SCALE);
		fOwner->fLink->Attach<float>(scale);

		fState->valid_flags |= B_VIEW_SCALE_BIT;
	}

	fState->scale = scale;
	fState->archiving_flags |= B_VIEW_SCALE_BIT;
}


float
KView::Scale() const
{
	if (!fState->IsValid(B_VIEW_SCALE_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_SCALE);

 		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK && code == B_OK)
			fOwner->fLink->Read<float>(&fState->scale);

		fState->valid_flags |= B_VIEW_SCALE_BIT;
	}

	return fState->scale;
}


void
KView::SetTransform(BAffineTransform transform)
{
	if (fState->IsValid(B_VIEW_TRANSFORM_BIT) && transform == fState->transform)
		return;

	if (fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_TRANSFORM);
		fOwner->fLink->Attach<BAffineTransform>(transform);

		fState->valid_flags |= B_VIEW_TRANSFORM_BIT;
	}

	fState->transform = transform;
	fState->archiving_flags |= B_VIEW_TRANSFORM_BIT;
}


BAffineTransform
KView::Transform() const
{
	if (!fState->IsValid(B_VIEW_TRANSFORM_BIT) && fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_TRANSFORM);

 		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK && code == B_OK)
			fOwner->fLink->Read<BAffineTransform>(&fState->transform);

		fState->valid_flags |= B_VIEW_TRANSFORM_BIT;
	}

	return fState->transform;
}


void
KView::TranslateBy(double x, double y)
{
	if (fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_AFFINE_TRANSLATE);
		fOwner->fLink->Attach<double>(x);
		fOwner->fLink->Attach<double>(y);

		fState->valid_flags &= ~B_VIEW_TRANSFORM_BIT;
	}

	fState->archiving_flags |= B_VIEW_TRANSFORM_BIT;
}


void
KView::ScaleBy(double x, double y)
{
	if (fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_AFFINE_SCALE);
		fOwner->fLink->Attach<double>(x);
		fOwner->fLink->Attach<double>(y);

		fState->valid_flags &= ~B_VIEW_TRANSFORM_BIT;
	}

	fState->archiving_flags |= B_VIEW_TRANSFORM_BIT;
}


void
KView::RotateBy(double angleRadians)
{
	if (fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_AFFINE_ROTATE);
		fOwner->fLink->Attach<double>(angleRadians);

		fState->valid_flags &= ~B_VIEW_TRANSFORM_BIT;
	}

	fState->archiving_flags |= B_VIEW_TRANSFORM_BIT;
}


void
KView::SetLineMode(cap_mode lineCap, join_mode lineJoin, float miterLimit)
{
	if (fState->IsValid(B_VIEW_LINE_MODES_BIT)
		&& lineCap == fState->line_cap && lineJoin == fState->line_join
		&& miterLimit == fState->miter_limit)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		ViewSetLineModeInfo info;
		info.lineJoin = lineJoin;
		info.lineCap = lineCap;
		info.miterLimit = miterLimit;

		fOwner->fLink->StartMessage(AS_VIEW_SET_LINE_MODE);
		fOwner->fLink->Attach<ViewSetLineModeInfo>(info);

		fState->valid_flags |= B_VIEW_LINE_MODES_BIT;
	}

	fState->line_cap = lineCap;
	fState->line_join = lineJoin;
	fState->miter_limit = miterLimit;

	fState->archiving_flags |= B_VIEW_LINE_MODES_BIT;
}


join_mode
KView::LineJoinMode() const
{
	// This will update the current state, if necessary
	if (!fState->IsValid(B_VIEW_LINE_MODES_BIT))
		LineMiterLimit();

	return fState->line_join;
}


cap_mode
KView::LineCapMode() const
{
	// This will update the current state, if necessary
	if (!fState->IsValid(B_VIEW_LINE_MODES_BIT))
		LineMiterLimit();

	return fState->line_cap;
}


float
KView::LineMiterLimit() const
{
	if (!fState->IsValid(B_VIEW_LINE_MODES_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_LINE_MODE);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK && code == B_OK) {

			ViewSetLineModeInfo info;
			fOwner->fLink->Read<ViewSetLineModeInfo>(&info);

			fState->line_cap = info.lineCap;
			fState->line_join = info.lineJoin;
			fState->miter_limit = info.miterLimit;
		}

		fState->valid_flags |= B_VIEW_LINE_MODES_BIT;
	}

	return fState->miter_limit;
}


void
KView::SetFillRule(int32 fillRule)
{
	if (fState->IsValid(B_VIEW_FILL_RULE_BIT) && fillRule == fState->fill_rule)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_FILL_RULE);
		fOwner->fLink->Attach<int32>(fillRule);

		fState->valid_flags |= B_VIEW_FILL_RULE_BIT;
	}

	fState->fill_rule = fillRule;

	fState->archiving_flags |= B_VIEW_FILL_RULE_BIT;
}


int32
KView::FillRule() const
{
	if (!fState->IsValid(B_VIEW_FILL_RULE_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_FILL_RULE);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK && code == B_OK) {

			int32 fillRule;
			fOwner->fLink->Read<int32>(&fillRule);

			fState->fill_rule = fillRule;
		}

		fState->valid_flags |= B_VIEW_FILL_RULE_BIT;
	}

	return fState->fill_rule;
}


void
KView::SetDrawingMode(drawing_mode mode)
{
	if (fState->IsValid(B_VIEW_DRAWING_MODE_BIT)
		&& mode == fState->drawing_mode)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_DRAWING_MODE);
		fOwner->fLink->Attach<int8>((int8)mode);

		fState->valid_flags |= B_VIEW_DRAWING_MODE_BIT;
	}

	fState->drawing_mode = mode;
	fState->archiving_flags |= B_VIEW_DRAWING_MODE_BIT;
}


drawing_mode
KView::DrawingMode() const
{
	if (!fState->IsValid(B_VIEW_DRAWING_MODE_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_DRAWING_MODE);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			int8 drawingMode;
			fOwner->fLink->Read<int8>(&drawingMode);

			fState->drawing_mode = (drawing_mode)drawingMode;
			fState->valid_flags |= B_VIEW_DRAWING_MODE_BIT;
		}
	}

	return fState->drawing_mode;
}


void
KView::SetBlendingMode(source_alpha sourceAlpha, alpha_function alphaFunction)
{
	if (fState->IsValid(B_VIEW_BLENDING_BIT)
		&& sourceAlpha == fState->alpha_source_mode
		&& alphaFunction == fState->alpha_function_mode)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		ViewBlendingModeInfo info;
		info.sourceAlpha = sourceAlpha;
		info.alphaFunction = alphaFunction;

		fOwner->fLink->StartMessage(AS_VIEW_SET_BLENDING_MODE);
		fOwner->fLink->Attach<ViewBlendingModeInfo>(info);

		fState->valid_flags |= B_VIEW_BLENDING_BIT;
	}

	fState->alpha_source_mode = sourceAlpha;
	fState->alpha_function_mode = alphaFunction;

	fState->archiving_flags |= B_VIEW_BLENDING_BIT;
}


void
KView::GetBlendingMode(source_alpha* _sourceAlpha,
	alpha_function* _alphaFunction) const
{
	if (!fState->IsValid(B_VIEW_BLENDING_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_BLENDING_MODE);

		int32 code;
 		if (fOwner->fLink->FlushWithReply(code) == B_OK && code == B_OK) {
 			ViewBlendingModeInfo info;
			fOwner->fLink->Read<ViewBlendingModeInfo>(&info);

			fState->alpha_source_mode = info.sourceAlpha;
			fState->alpha_function_mode = info.alphaFunction;

			fState->valid_flags |= B_VIEW_BLENDING_BIT;
		}
	}

	if (_sourceAlpha)
		*_sourceAlpha = fState->alpha_source_mode;

	if (_alphaFunction)
		*_alphaFunction = fState->alpha_function_mode;
}


void
KView::MovePenTo(BPoint point)
{
	MovePenTo(point.x, point.y);
}


void
KView::MovePenTo(float x, float y)
{
	if (fState->IsValid(B_VIEW_PEN_LOCATION_BIT)
		&& x == fState->pen_location.x && y == fState->pen_location.y)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_PEN_LOC);
		fOwner->fLink->Attach<BPoint>(BPoint(x, y));

		fState->valid_flags |= B_VIEW_PEN_LOCATION_BIT;
	}

	fState->pen_location.x = x;
	fState->pen_location.y = y;

	fState->archiving_flags |= B_VIEW_PEN_LOCATION_BIT;
}


void
KView::MovePenBy(float x, float y)
{
	// this will update the pen location if necessary
	if (!fState->IsValid(B_VIEW_PEN_LOCATION_BIT))
		PenLocation();

	MovePenTo(fState->pen_location.x + x, fState->pen_location.y + y);
}


BPoint
KView::PenLocation() const
{
	if (!fState->IsValid(B_VIEW_PEN_LOCATION_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_PEN_LOC);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<BPoint>(&fState->pen_location);

			fState->valid_flags |= B_VIEW_PEN_LOCATION_BIT;
		}
	}

	return fState->pen_location;
}


void
KView::SetPenSize(float size)
{
	if (fState->IsValid(B_VIEW_PEN_SIZE_BIT) && size == fState->pen_size)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_PEN_SIZE);
		fOwner->fLink->Attach<float>(size);

		fState->valid_flags |= B_VIEW_PEN_SIZE_BIT;
	}

	fState->pen_size = size;
	fState->archiving_flags	|= B_VIEW_PEN_SIZE_BIT;
}


float
KView::PenSize() const
{
	if (!fState->IsValid(B_VIEW_PEN_SIZE_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_PEN_SIZE);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<float>(&fState->pen_size);

			fState->valid_flags |= B_VIEW_PEN_SIZE_BIT;
		}
	}

	return fState->pen_size;
}


void
KView::SetHighColor(rgb_color color)
{
	SetHighUIColor(B_NO_COLOR);

	// are we up-to-date already?
	if (fState->IsValid(B_VIEW_HIGH_COLOR_BIT)
		&& fState->high_color == color)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_HIGH_COLOR);
		fOwner->fLink->Attach<rgb_color>(color);

		fState->valid_flags |= B_VIEW_HIGH_COLOR_BIT;
	}

	fState->high_color = color;

	fState->archiving_flags |= B_VIEW_HIGH_COLOR_BIT;
}


rgb_color
KView::HighColor() const
{
	if (!fState->IsValid(B_VIEW_HIGH_COLOR_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_HIGH_COLOR);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<rgb_color>(&fState->high_color);

			fState->valid_flags |= B_VIEW_HIGH_COLOR_BIT;
		}
	}

	return fState->high_color;
}


void
KView::SetHighUIColor(color_which which, float tint)
{
	if (fState->IsValid(B_VIEW_WHICH_HIGH_COLOR_BIT)
		&& fState->which_high_color == which
		&& fState->which_high_color_tint == tint)
		return;

	if (fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_HIGH_UI_COLOR);
		fOwner->fLink->Attach<color_which>(which);
		fOwner->fLink->Attach<float>(tint);

		fState->valid_flags |= B_VIEW_WHICH_HIGH_COLOR_BIT;
	}

	fState->which_high_color = which;
	fState->which_high_color_tint = tint;

	if (which != B_NO_COLOR) {
		fState->archiving_flags |= B_VIEW_WHICH_HIGH_COLOR_BIT;
		fState->archiving_flags &= ~B_VIEW_HIGH_COLOR_BIT;
		fState->valid_flags |= B_VIEW_HIGH_COLOR_BIT;

		fState->high_color = tint_color(ui_color(which), tint);
	} else {
		fState->valid_flags &= ~B_VIEW_HIGH_COLOR_BIT;
		fState->archiving_flags &= ~B_VIEW_WHICH_HIGH_COLOR_BIT;
	}
}


color_which
KView::HighUIColor(float* tint) const
{
	if (!fState->IsValid(B_VIEW_WHICH_HIGH_COLOR_BIT)
		&& fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_HIGH_UI_COLOR);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<color_which>(&fState->which_high_color);
			fOwner->fLink->Read<float>(&fState->which_high_color_tint);
			fOwner->fLink->Read<rgb_color>(&fState->high_color);

			fState->valid_flags |= B_VIEW_WHICH_HIGH_COLOR_BIT;
			fState->valid_flags |= B_VIEW_HIGH_COLOR_BIT;
		}
	}

	if (tint != NULL)
		*tint = fState->which_high_color_tint;

	return fState->which_high_color;
}


void
KView::SetLowColor(rgb_color color)
{
	SetLowUIColor(B_NO_COLOR);

	if (fState->IsValid(B_VIEW_LOW_COLOR_BIT)
		&& fState->low_color == color)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_LOW_COLOR);
		fOwner->fLink->Attach<rgb_color>(color);

		fState->valid_flags |= B_VIEW_LOW_COLOR_BIT;
	}

	fState->low_color = color;

	fState->archiving_flags |= B_VIEW_LOW_COLOR_BIT;
}


rgb_color
KView::LowColor() const
{
	if (!fState->IsValid(B_VIEW_LOW_COLOR_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_LOW_COLOR);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<rgb_color>(&fState->low_color);

			fState->valid_flags |= B_VIEW_LOW_COLOR_BIT;
		}
	}

	return fState->low_color;
}


void
KView::SetLowUIColor(color_which which, float tint)
{
	if (fState->IsValid(B_VIEW_WHICH_LOW_COLOR_BIT)
		&& fState->which_low_color == which
		&& fState->which_low_color_tint == tint)
		return;

	if (fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_LOW_UI_COLOR);
		fOwner->fLink->Attach<color_which>(which);
		fOwner->fLink->Attach<float>(tint);

		fState->valid_flags |= B_VIEW_WHICH_LOW_COLOR_BIT;
	}

	fState->which_low_color = which;
	fState->which_low_color_tint = tint;

	if (which != B_NO_COLOR) {
		fState->archiving_flags |= B_VIEW_WHICH_LOW_COLOR_BIT;
		fState->archiving_flags &= ~B_VIEW_LOW_COLOR_BIT;
		fState->valid_flags |= B_VIEW_LOW_COLOR_BIT;

		fState->low_color = tint_color(ui_color(which), tint);
	} else {
		fState->valid_flags &= ~B_VIEW_LOW_COLOR_BIT;
		fState->archiving_flags &= ~B_VIEW_WHICH_LOW_COLOR_BIT;
	}
}


color_which
KView::LowUIColor(float* tint) const
{
	if (!fState->IsValid(B_VIEW_WHICH_LOW_COLOR_BIT)
		&& fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_LOW_UI_COLOR);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<color_which>(&fState->which_low_color);
			fOwner->fLink->Read<float>(&fState->which_low_color_tint);
			fOwner->fLink->Read<rgb_color>(&fState->low_color);

			fState->valid_flags |= B_VIEW_WHICH_LOW_COLOR_BIT;
			fState->valid_flags |= B_VIEW_LOW_COLOR_BIT;
		}
	}

	if (tint != NULL)
		*tint = fState->which_low_color_tint;

	return fState->which_low_color;
}


bool
KView::HasDefaultColors() const
{
	// If we don't have any of these flags, then we have default colors
	uint32 testMask = B_VIEW_VIEW_COLOR_BIT | B_VIEW_HIGH_COLOR_BIT
		| B_VIEW_LOW_COLOR_BIT | B_VIEW_WHICH_VIEW_COLOR_BIT
		| B_VIEW_WHICH_HIGH_COLOR_BIT | B_VIEW_WHICH_LOW_COLOR_BIT;

	return (fState->archiving_flags & testMask) == 0;
}


bool
KView::HasSystemColors() const
{
	return fState->which_view_color == B_PANEL_BACKGROUND_COLOR
		&& fState->which_high_color == B_PANEL_TEXT_COLOR
		&& fState->which_low_color == B_PANEL_BACKGROUND_COLOR
		&& fState->which_view_color_tint == B_NO_TINT
		&& fState->which_high_color_tint == B_NO_TINT
		&& fState->which_low_color_tint == B_NO_TINT;
}


void
KView::AdoptParentColors()
{
	AdoptViewColors(Parent());
}


void
KView::AdoptSystemColors()
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(B_PANEL_BACKGROUND_COLOR);
	SetHighUIColor(B_PANEL_TEXT_COLOR);
}


void
KView::AdoptViewColors(KView* view)
{
	if (view == NULL || (view->Window() != NULL && !view->LockLooper()))
		return;

	float tint = B_NO_TINT;
	float viewTint = tint;
	color_which viewWhich = view->ViewUIColor(&viewTint);

	// View color
	if (viewWhich != B_NO_COLOR)
		SetViewUIColor(viewWhich, viewTint);
	else
		SetViewColor(view->ViewColor());

	// Low color
	color_which which = view->LowUIColor(&tint);
	if (which != B_NO_COLOR)
		SetLowUIColor(which, tint);
	else if (viewWhich != B_NO_COLOR)
		SetLowUIColor(viewWhich, viewTint);
	else
		SetLowColor(view->LowColor());

	// High color
	which = view->HighUIColor(&tint);
	if (which != B_NO_COLOR)
		SetHighUIColor(which, tint);
	else
		SetHighColor(view->HighColor());

	if (view->Window() != NULL)
		view->UnlockLooper();
}


void
KView::SetViewColor(rgb_color color)
{
	SetViewUIColor(B_NO_COLOR);

	if (fState->IsValid(B_VIEW_VIEW_COLOR_BIT)
		&& fState->view_color == color)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_VIEW_COLOR);
		fOwner->fLink->Attach<rgb_color>(color);
		fOwner->fLink->Flush();

		fState->valid_flags |= B_VIEW_VIEW_COLOR_BIT;
	}

	fState->view_color = color;

	fState->archiving_flags |= B_VIEW_VIEW_COLOR_BIT;
}


rgb_color
KView::ViewColor() const
{
	if (!fState->IsValid(B_VIEW_VIEW_COLOR_BIT) && fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_VIEW_COLOR);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<rgb_color>(&fState->view_color);

			fState->valid_flags |= B_VIEW_VIEW_COLOR_BIT;
		}
	}

	return fState->view_color;
}


void
KView::SetViewUIColor(color_which which, float tint)
{
	if (fState->IsValid(B_VIEW_WHICH_VIEW_COLOR_BIT)
		&& fState->which_view_color == which
		&& fState->which_view_color_tint == tint)
		return;

	if (fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_VIEW_UI_COLOR);
		fOwner->fLink->Attach<color_which>(which);
		fOwner->fLink->Attach<float>(tint);

		fState->valid_flags |= B_VIEW_WHICH_VIEW_COLOR_BIT;
	}

	fState->which_view_color = which;
	fState->which_view_color_tint = tint;

	if (which != B_NO_COLOR) {
		fState->archiving_flags |= B_VIEW_WHICH_VIEW_COLOR_BIT;
		fState->archiving_flags &= ~B_VIEW_VIEW_COLOR_BIT;
		fState->valid_flags |= B_VIEW_VIEW_COLOR_BIT;

		fState->view_color = tint_color(ui_color(which), tint);
	} else {
		fState->valid_flags &= ~B_VIEW_VIEW_COLOR_BIT;
		fState->archiving_flags &= ~B_VIEW_WHICH_VIEW_COLOR_BIT;
	}

	if (!fState->IsValid(B_VIEW_WHICH_LOW_COLOR_BIT))
		SetLowUIColor(which, tint);
}


color_which
KView::ViewUIColor(float* tint) const
{
	if (!fState->IsValid(B_VIEW_WHICH_VIEW_COLOR_BIT)
		&& fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_VIEW_UI_COLOR);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<color_which>(&fState->which_view_color);
			fOwner->fLink->Read<float>(&fState->which_view_color_tint);
			fOwner->fLink->Read<rgb_color>(&fState->view_color);

			fState->valid_flags |= B_VIEW_WHICH_VIEW_COLOR_BIT;
			fState->valid_flags |= B_VIEW_VIEW_COLOR_BIT;
		}
	}

	if (tint != NULL)
		*tint = fState->which_view_color_tint;

	return fState->which_view_color;
}


void
KView::ForceFontAliasing(bool enable)
{
	if (fState->IsValid(B_VIEW_FONT_ALIASING_BIT)
		&& enable == fState->font_aliasing)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_PRINT_ALIASING);
		fOwner->fLink->Attach<bool>(enable);

		fState->valid_flags |= B_VIEW_FONT_ALIASING_BIT;
	}

	fState->font_aliasing = enable;
	fState->archiving_flags |= B_VIEW_FONT_ALIASING_BIT;
}


void
KView::SetFont(const BFont* font, uint32 mask)
{
	if (!font || mask == 0)
		return;

	if (mask == B_FONT_ALL) {
		fState->font = *font;
	} else {
		// TODO: move this into a BFont method
		if (mask & B_FONT_FAMILY_AND_STYLE)
			fState->font.SetFamilyAndStyle(font->FamilyAndStyle());

		if (mask & B_FONT_SIZE)
			fState->font.SetSize(font->Size());

		if (mask & B_FONT_SHEAR)
			fState->font.SetShear(font->Shear());

		if (mask & B_FONT_ROTATION)
			fState->font.SetRotation(font->Rotation());

		if (mask & B_FONT_FALSE_BOLD_WIDTH)
			fState->font.SetFalseBoldWidth(font->FalseBoldWidth());

		if (mask & B_FONT_SPACING)
			fState->font.SetSpacing(font->Spacing());

		if (mask & B_FONT_ENCODING)
			fState->font.SetEncoding(font->Encoding());

		if (mask & B_FONT_FACE)
			fState->font.SetFace(font->Face());

		if (mask & B_FONT_FLAGS)
			fState->font.SetFlags(font->Flags());
	}

	fState->font_flags |= mask;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fState->UpdateServerFontState(*fOwner->fLink);
		fState->valid_flags |= B_VIEW_FONT_BIT;
	}

	fState->archiving_flags |= B_VIEW_FONT_BIT;
	// TODO: InvalidateLayout() here for convenience?
}


void
KView::GetFont(BFont* font) const
{
	if (!fState->IsValid(B_VIEW_FONT_BIT)) {
		// we don't keep graphics state information, therefor
		// we need to ask the server for the origin after PopState()
		_CheckOwnerLockAndSwitchCurrent();

		// TODO: add a font getter!
		fState->UpdateFrom(*fOwner->fLink);
	}

	*font = fState->font;
}


void
KView::GetFontHeight(font_height* height) const
{
	fState->font.GetHeight(height);
}


void
KView::SetFontSize(float size)
{
	BFont font;
	font.SetSize(size);

	SetFont(&font, B_FONT_SIZE);
}


float
KView::StringWidth(const char* string) const
{
	return fState->font.StringWidth(string);
}


float
KView::StringWidth(const char* string, int32 length) const
{
	return fState->font.StringWidth(string, length);
}


void
KView::GetStringWidths(char* stringArray[], int32 lengthArray[],
	int32 numStrings, float widthArray[]) const
{
	fState->font.GetStringWidths(const_cast<const char**>(stringArray),
		const_cast<const int32*>(lengthArray), numStrings, widthArray);
}


void
KView::TruncateString(BString* string, uint32 mode, float width) const
{
	fState->font.TruncateString(string, mode, width);
}


void
KView::ClipToPicture(KPicture* picture, BPoint where, bool sync)
{
	_ClipToPicture(picture, where, false, sync);
}


void
KView::ClipToInversePicture(KPicture* picture, BPoint where, bool sync)
{
	_ClipToPicture(picture, where, true, sync);
}


void
KView::GetClippingRegion(BRegion* region) const
{
	if (!region)
		return;

	// NOTE: the client has no idea when the clipping in the server
	// changed, so it is always read from the server
	region->MakeEmpty();


	if (fOwner) {
		if (fIsPrinting && _CheckOwnerLock()) {
			region->Set(fState->print_rect);
			return;
		}

		_CheckLockAndSwitchCurrent();
		fOwner->fLink->StartMessage(AS_VIEW_GET_CLIP_REGION);

 		int32 code;
 		if (fOwner->fLink->FlushWithReply(code) == B_OK
 			&& code == B_OK) {
			fOwner->fLink->ReadRegion(region);
			fState->valid_flags |= B_VIEW_CLIP_REGION_BIT;
		}
	}
}


void
KView::ConstrainClippingRegion(BRegion* region)
{
	if (_CheckOwnerLockAndSwitchCurrent()) {
		fOwner->fLink->StartMessage(AS_VIEW_SET_CLIP_REGION);

		if (region) {
			int32 count = region->CountRects();
			fOwner->fLink->Attach<int32>(count);
			if (count > 0)
				fOwner->fLink->AttachRegion(*region);
		} else {
			fOwner->fLink->Attach<int32>(-1);
			// '-1' means that in the app_server, there won't be any 'local'
			// clipping region (it will be NULL)
		}

		_FlushIfNotInTransaction();

		fState->valid_flags &= ~B_VIEW_CLIP_REGION_BIT;
		fState->archiving_flags |= B_VIEW_CLIP_REGION_BIT;
	}
}


void
KView::ClipToRect(BRect rect)
{
	_ClipToRect(rect, false);
}


void
KView::ClipToInverseRect(BRect rect)
{
	_ClipToRect(rect, true);
}


void
KView::ClipToShape(BShape* shape)
{
	_ClipToShape(shape, false);
}


void
KView::ClipToInverseShape(BShape* shape)
{
	_ClipToShape(shape, true);
}


//	#pragma mark - Drawing Functions


void
KView::DrawBitmapAsync(const KBitmap* bitmap, BRect bitmapRect, BRect viewRect,
	uint32 options)
{
	if (bitmap == NULL || fOwner == NULL
		|| !bitmapRect.IsValid() || !viewRect.IsValid())
		return;

	_CheckLockAndSwitchCurrent();

	ViewDrawBitmapInfo info;
	info.bitmapToken = bitmap->_ServerToken();
	info.options = options;
	info.viewRect = viewRect;
	info.bitmapRect = bitmapRect;

	fOwner->fLink->StartMessage(AS_VIEW_DRAW_BITMAP);
	fOwner->fLink->Attach<ViewDrawBitmapInfo>(info);

	_FlushIfNotInTransaction();
}


void
KView::DrawBitmapAsync(const KBitmap* bitmap, BRect bitmapRect, BRect viewRect)
{
	DrawBitmapAsync(bitmap, bitmapRect, viewRect, 0);
}


void
KView::DrawBitmapAsync(const KBitmap* bitmap, BRect viewRect)
{
	if (bitmap && fOwner) {
		DrawBitmapAsync(bitmap, bitmap->Bounds().OffsetToCopy(B_ORIGIN),
			viewRect, 0);
	}
}


void
KView::DrawBitmapAsync(const KBitmap* bitmap, BPoint where)
{
	if (bitmap == NULL || fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	ViewDrawBitmapInfo info;
	info.bitmapToken = bitmap->_ServerToken();
	info.options = 0;
	info.bitmapRect = bitmap->Bounds().OffsetToCopy(B_ORIGIN);
	info.viewRect = info.bitmapRect.OffsetToCopy(where);

	fOwner->fLink->StartMessage(AS_VIEW_DRAW_BITMAP);
	fOwner->fLink->Attach<ViewDrawBitmapInfo>(info);

	_FlushIfNotInTransaction();
}


void
KView::DrawBitmapAsync(const KBitmap* bitmap)
{
	DrawBitmapAsync(bitmap, PenLocation());
}


void
KView::DrawBitmap(const KBitmap* bitmap, BRect bitmapRect, BRect viewRect,
	uint32 options)
{
	if (fOwner) {
		DrawBitmapAsync(bitmap, bitmapRect, viewRect, options);
		Sync();
	}
}


void
KView::DrawBitmap(const KBitmap* bitmap, BRect bitmapRect, BRect viewRect)
{
	if (fOwner) {
		DrawBitmapAsync(bitmap, bitmapRect, viewRect, 0);
		Sync();
	}
}


void
KView::DrawBitmap(const KBitmap* bitmap, BRect viewRect)
{
	if (bitmap && fOwner) {
		DrawBitmap(bitmap, bitmap->Bounds().OffsetToCopy(B_ORIGIN), viewRect,
			0);
	}
}


void
KView::DrawBitmap(const KBitmap* bitmap, BPoint where)
{
	if (fOwner) {
		DrawBitmapAsync(bitmap, where);
		Sync();
	}
}


void
KView::DrawBitmap(const KBitmap* bitmap)
{
	DrawBitmap(bitmap, PenLocation());
}


void
KView::DrawTiledBitmapAsync(const KBitmap* bitmap, BRect viewRect,
	BPoint phase)
{
	if (bitmap == NULL || fOwner == NULL || !viewRect.IsValid())
		return;

	_CheckLockAndSwitchCurrent();

	ViewDrawBitmapInfo info;
	info.bitmapToken = bitmap->_ServerToken();
	info.options = B_TILE_BITMAP;
	info.viewRect = viewRect;
	info.bitmapRect = bitmap->Bounds().OffsetToCopy(phase);

	fOwner->fLink->StartMessage(AS_VIEW_DRAW_BITMAP);
	fOwner->fLink->Attach<ViewDrawBitmapInfo>(info);

	_FlushIfNotInTransaction();
}


void
KView::DrawTiledBitmap(const KBitmap* bitmap, BRect viewRect, BPoint phase)
{
	if (fOwner) {
		DrawTiledBitmapAsync(bitmap, viewRect, phase);
		Sync();
	}
}


void
KView::DrawChar(char c)
{
	DrawString(&c, 1, PenLocation());
}


void
KView::DrawChar(char c, BPoint location)
{
	DrawString(&c, 1, location);
}


void
KView::DrawString(const char* string, escapement_delta* delta)
{
	if (string == NULL)
		return;

	DrawString(string, strlen(string), PenLocation(), delta);
}


void
KView::DrawString(const char* string, BPoint location, escapement_delta* delta)
{
	if (string == NULL)
		return;

	DrawString(string, strlen(string), location, delta);
}


void
KView::DrawString(const char* string, int32 length, escapement_delta* delta)
{
	DrawString(string, length, PenLocation(), delta);
}


void
KView::DrawString(const char* string, int32 length, BPoint location,
	escapement_delta* delta)
{
	if (fOwner == NULL || string == NULL || length < 1)
		return;

	_CheckLockAndSwitchCurrent();

	ViewDrawStringInfo info;
	info.stringLength = length;
	info.location = location;
	if (delta != NULL)
		info.delta = *delta;

	// quite often delta will be NULL
	if (delta)
		fOwner->fLink->StartMessage(AS_DRAW_STRING_WITH_DELTA);
	else
		fOwner->fLink->StartMessage(AS_DRAW_STRING);

	fOwner->fLink->Attach<ViewDrawStringInfo>(info);
	fOwner->fLink->Attach(string, length);

	_FlushIfNotInTransaction();

	// this modifies our pen location, so we invalidate the flag.
	fState->valid_flags &= ~B_VIEW_PEN_LOCATION_BIT;
}


void
KView::DrawString(const char* string, const BPoint* locations,
	int32 locationCount)
{
	if (string == NULL)
		return;

	DrawString(string, strlen(string), locations, locationCount);
}


void
KView::DrawString(const char* string, int32 length, const BPoint* locations,
	int32 locationCount)
{
	if (fOwner == NULL || string == NULL || length < 1 || locations == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_DRAW_STRING_WITH_OFFSETS);

	fOwner->fLink->Attach<int32>(length);
	fOwner->fLink->Attach<int32>(locationCount);
	fOwner->fLink->Attach(string, length);
	fOwner->fLink->Attach(locations, locationCount * sizeof(BPoint));

	_FlushIfNotInTransaction();

	// this modifies our pen location, so we invalidate the flag.
	fState->valid_flags &= ~B_VIEW_PEN_LOCATION_BIT;
}


void
KView::StrokeEllipse(BPoint center, float xRadius, float yRadius,
	::pattern pattern)
{
	StrokeEllipse(BRect(center.x - xRadius, center.y - yRadius,
		center.x + xRadius, center.y + yRadius), pattern);
}


void
KView::StrokeEllipse(BRect rect, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_ELLIPSE);
	fOwner->fLink->Attach<BRect>(rect);

	_FlushIfNotInTransaction();
}


void
KView::FillEllipse(BPoint center, float xRadius, float yRadius,
	::pattern pattern)
{
	FillEllipse(BRect(center.x - xRadius, center.y - yRadius,
		center.x + xRadius, center.y + yRadius), pattern);
}


void
KView::FillEllipse(BPoint center, float xRadius, float yRadius,
	const BGradient& gradient)
{
	FillEllipse(BRect(center.x - xRadius, center.y - yRadius,
		center.x + xRadius, center.y + yRadius), gradient);
}


void
KView::FillEllipse(BRect rect, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_ELLIPSE);
	fOwner->fLink->Attach<BRect>(rect);

	_FlushIfNotInTransaction();
}


void
KView::FillEllipse(BRect rect, const BGradient& gradient)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_ELLIPSE_GRADIENT);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
KView::StrokeArc(BPoint center, float xRadius, float yRadius, float startAngle,
	float arcAngle, ::pattern pattern)
{
	StrokeArc(BRect(center.x - xRadius, center.y - yRadius, center.x + xRadius,
		center.y + yRadius), startAngle, arcAngle, pattern);
}


void
KView::StrokeArc(BRect rect, float startAngle, float arcAngle,
	::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_ARC);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(startAngle);
	fOwner->fLink->Attach<float>(arcAngle);

	_FlushIfNotInTransaction();
}


void
KView::FillArc(BPoint center,float xRadius, float yRadius, float startAngle,
	float arcAngle, ::pattern pattern)
{
	FillArc(BRect(center.x - xRadius, center.y - yRadius, center.x + xRadius,
		center.y + yRadius), startAngle, arcAngle, pattern);
}


void
KView::FillArc(BPoint center,float xRadius, float yRadius, float startAngle,
	float arcAngle, const BGradient& gradient)
{
	FillArc(BRect(center.x - xRadius, center.y - yRadius, center.x + xRadius,
		center.y + yRadius), startAngle, arcAngle, gradient);
}


void
KView::FillArc(BRect rect, float startAngle, float arcAngle,
	::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_ARC);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(startAngle);
	fOwner->fLink->Attach<float>(arcAngle);

	_FlushIfNotInTransaction();
}


void
KView::FillArc(BRect rect, float startAngle, float arcAngle,
	const BGradient& gradient)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_ARC_GRADIENT);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(startAngle);
	fOwner->fLink->Attach<float>(arcAngle);
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
KView::StrokeBezier(BPoint* controlPoints, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_BEZIER);
	fOwner->fLink->Attach<BPoint>(controlPoints[0]);
	fOwner->fLink->Attach<BPoint>(controlPoints[1]);
	fOwner->fLink->Attach<BPoint>(controlPoints[2]);
	fOwner->fLink->Attach<BPoint>(controlPoints[3]);

	_FlushIfNotInTransaction();
}


void
KView::FillBezier(BPoint* controlPoints, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_BEZIER);
	fOwner->fLink->Attach<BPoint>(controlPoints[0]);
	fOwner->fLink->Attach<BPoint>(controlPoints[1]);
	fOwner->fLink->Attach<BPoint>(controlPoints[2]);
	fOwner->fLink->Attach<BPoint>(controlPoints[3]);

	_FlushIfNotInTransaction();
}


void
KView::FillBezier(BPoint* controlPoints, const BGradient& gradient)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_BEZIER_GRADIENT);
	fOwner->fLink->Attach<BPoint>(controlPoints[0]);
	fOwner->fLink->Attach<BPoint>(controlPoints[1]);
	fOwner->fLink->Attach<BPoint>(controlPoints[2]);
	fOwner->fLink->Attach<BPoint>(controlPoints[3]);
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
KView::StrokePolygon(const BPolygon* polygon, bool closed, ::pattern pattern)
{
	if (polygon == NULL)
		return;

	StrokePolygon(polygon->fPoints, polygon->fCount, polygon->Frame(), closed,
		pattern);
}


void
KView::StrokePolygon(const BPoint* pointArray, int32 numPoints, bool closed,
	::pattern pattern)
{
	BPolygon polygon(pointArray, numPoints);

	StrokePolygon(polygon.fPoints, polygon.fCount, polygon.Frame(), closed,
		pattern);
}


void
KView::StrokePolygon(const BPoint* pointArray, int32 numPoints, BRect bounds,
	bool closed, ::pattern pattern)
{
	if (pointArray == NULL
		|| numPoints <= 1
		|| fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	BPolygon polygon(pointArray, numPoints);
	polygon.MapTo(polygon.Frame(), bounds);

	if (fOwner->fLink->StartMessage(AS_STROKE_POLYGON,
			polygon.fCount * sizeof(BPoint) + sizeof(BRect) + sizeof(bool)
				+ sizeof(int32)) == B_OK) {
		fOwner->fLink->Attach<BRect>(polygon.Frame());
		fOwner->fLink->Attach<bool>(closed);
		fOwner->fLink->Attach<int32>(polygon.fCount);
		fOwner->fLink->Attach(polygon.fPoints, polygon.fCount * sizeof(BPoint));

		_FlushIfNotInTransaction();
	} else {
		fprintf(stderr, "ERROR: Can't send polygon to app_server!\n");
	}
}


void
KView::FillPolygon(const BPolygon* polygon, ::pattern pattern)
{
	if (polygon == NULL
		|| polygon->fCount <= 2
		|| fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	if (fOwner->fLink->StartMessage(AS_FILL_POLYGON,
			polygon->fCount * sizeof(BPoint) + sizeof(BRect) + sizeof(int32))
				== B_OK) {
		fOwner->fLink->Attach<BRect>(polygon->Frame());
		fOwner->fLink->Attach<int32>(polygon->fCount);
		fOwner->fLink->Attach(polygon->fPoints,
			polygon->fCount * sizeof(BPoint));

		_FlushIfNotInTransaction();
	} else {
		fprintf(stderr, "ERROR: Can't send polygon to app_server!\n");
	}
}


void
KView::FillPolygon(const BPolygon* polygon, const BGradient& gradient)
{
	if (polygon == NULL
		|| polygon->fCount <= 2
		|| fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	if (fOwner->fLink->StartMessage(AS_FILL_POLYGON_GRADIENT,
			polygon->fCount * sizeof(BPoint) + sizeof(BRect) + sizeof(int32))
				== B_OK) {
		fOwner->fLink->Attach<BRect>(polygon->Frame());
		fOwner->fLink->Attach<int32>(polygon->fCount);
		fOwner->fLink->Attach(polygon->fPoints,
			polygon->fCount * sizeof(BPoint));
		fOwner->fLink->AttachGradient(gradient);

		_FlushIfNotInTransaction();
	} else {
		fprintf(stderr, "ERROR: Can't send polygon to app_server!\n");
	}
}


void
KView::FillPolygon(const BPoint* pointArray, int32 numPoints, ::pattern pattern)
{
	if (pointArray == NULL)
		return;

	BPolygon polygon(pointArray, numPoints);
	FillPolygon(&polygon, pattern);
}


void
KView::FillPolygon(const BPoint* pointArray, int32 numPoints,
	const BGradient& gradient)
{
	if (pointArray == NULL)
		return;

	BPolygon polygon(pointArray, numPoints);
	FillPolygon(&polygon, gradient);
}


void
KView::FillPolygon(const BPoint* pointArray, int32 numPoints, BRect bounds,
	::pattern pattern)
{
	if (pointArray == NULL)
		return;

	BPolygon polygon(pointArray, numPoints);

	polygon.MapTo(polygon.Frame(), bounds);
	FillPolygon(&polygon, pattern);
}


void
KView::FillPolygon(const BPoint* pointArray, int32 numPoints, BRect bounds,
	const BGradient& gradient)
{
	if (pointArray == NULL)
		return;

	BPolygon polygon(pointArray, numPoints);

	polygon.MapTo(polygon.Frame(), bounds);
	FillPolygon(&polygon, gradient);
}


void
KView::StrokeRect(BRect rect, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_RECT);
	fOwner->fLink->Attach<BRect>(rect);

	_FlushIfNotInTransaction();
}


void
KView::FillRect(BRect rect, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	// NOTE: ensuring compatibility with R5,
	// invalid rects are not filled, they are stroked though!
	if (!rect.IsValid())
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_RECT);
	fOwner->fLink->Attach<BRect>(rect);

	_FlushIfNotInTransaction();
}


void
KView::FillRect(BRect rect, const BGradient& gradient)
{
	if (fOwner == NULL)
		return;

	// NOTE: ensuring compatibility with R5,
	// invalid rects are not filled, they are stroked though!
	if (!rect.IsValid())
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_RECT_GRADIENT);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
KView::StrokeRoundRect(BRect rect, float xRadius, float yRadius,
	::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_ROUNDRECT);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(xRadius);
	fOwner->fLink->Attach<float>(yRadius);

	_FlushIfNotInTransaction();
}


void
KView::FillRoundRect(BRect rect, float xRadius, float yRadius,
	::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_ROUNDRECT);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(xRadius);
	fOwner->fLink->Attach<float>(yRadius);

	_FlushIfNotInTransaction();
}


void
KView::FillRoundRect(BRect rect, float xRadius, float yRadius,
	const BGradient& gradient)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_ROUNDRECT_GRADIENT);
	fOwner->fLink->Attach<BRect>(rect);
	fOwner->fLink->Attach<float>(xRadius);
	fOwner->fLink->Attach<float>(yRadius);
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
KView::FillRegion(BRegion* region, ::pattern pattern)
{
	if (region == NULL || fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_REGION);
	fOwner->fLink->AttachRegion(*region);

	_FlushIfNotInTransaction();
}


void
KView::FillRegion(BRegion* region, const BGradient& gradient)
{
	if (region == NULL || fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_REGION_GRADIENT);
	fOwner->fLink->AttachRegion(*region);
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
KView::StrokeTriangle(BPoint point1, BPoint point2, BPoint point3, BRect bounds,
	::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_TRIANGLE);
	fOwner->fLink->Attach<BPoint>(point1);
	fOwner->fLink->Attach<BPoint>(point2);
	fOwner->fLink->Attach<BPoint>(point3);
	fOwner->fLink->Attach<BRect>(bounds);

	_FlushIfNotInTransaction();
}


void
KView::StrokeTriangle(BPoint point1, BPoint point2, BPoint point3,
	::pattern pattern)
{
	if (fOwner) {
		// we construct the smallest rectangle that contains the 3 points
		// for the 1st point
		BRect bounds(point1, point1);

		// for the 2nd point
		if (point2.x < bounds.left)
			bounds.left = point2.x;

		if (point2.y < bounds.top)
			bounds.top = point2.y;

		if (point2.x > bounds.right)
			bounds.right = point2.x;

		if (point2.y > bounds.bottom)
			bounds.bottom = point2.y;

		// for the 3rd point
		if (point3.x < bounds.left)
			bounds.left = point3.x;

		if (point3.y < bounds.top)
			bounds.top = point3.y;

		if (point3.x > bounds.right)
			bounds.right = point3.x;

		if (point3.y > bounds.bottom)
			bounds.bottom = point3.y;

		StrokeTriangle(point1, point2, point3, bounds, pattern);
	}
}


void
KView::FillTriangle(BPoint point1, BPoint point2, BPoint point3,
	::pattern pattern)
{
	if (fOwner) {
		// we construct the smallest rectangle that contains the 3 points
		// for the 1st point
		BRect bounds(point1, point1);

		// for the 2nd point
		if (point2.x < bounds.left)
			bounds.left = point2.x;

		if (point2.y < bounds.top)
			bounds.top = point2.y;

		if (point2.x > bounds.right)
			bounds.right = point2.x;

		if (point2.y > bounds.bottom)
			bounds.bottom = point2.y;

		// for the 3rd point
		if (point3.x < bounds.left)
			bounds.left = point3.x;

		if (point3.y < bounds.top)
			bounds.top = point3.y;

		if (point3.x > bounds.right)
			bounds.right = point3.x;

		if (point3.y > bounds.bottom)
			bounds.bottom = point3.y;

		FillTriangle(point1, point2, point3, bounds, pattern);
	}
}


void
KView::FillTriangle(BPoint point1, BPoint point2, BPoint point3,
	const BGradient& gradient)
{
	if (fOwner) {
		// we construct the smallest rectangle that contains the 3 points
		// for the 1st point
		BRect bounds(point1, point1);

		// for the 2nd point
		if (point2.x < bounds.left)
			bounds.left = point2.x;

		if (point2.y < bounds.top)
			bounds.top = point2.y;

		if (point2.x > bounds.right)
			bounds.right = point2.x;

		if (point2.y > bounds.bottom)
			bounds.bottom = point2.y;

		// for the 3rd point
		if (point3.x < bounds.left)
			bounds.left = point3.x;

		if (point3.y < bounds.top)
			bounds.top = point3.y;

		if (point3.x > bounds.right)
			bounds.right = point3.x;

		if (point3.y > bounds.bottom)
			bounds.bottom = point3.y;

		FillTriangle(point1, point2, point3, bounds, gradient);
	}
}


void
KView::FillTriangle(BPoint point1, BPoint point2, BPoint point3,
	BRect bounds, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_TRIANGLE);
	fOwner->fLink->Attach<BPoint>(point1);
	fOwner->fLink->Attach<BPoint>(point2);
	fOwner->fLink->Attach<BPoint>(point3);
	fOwner->fLink->Attach<BRect>(bounds);

	_FlushIfNotInTransaction();
}


void
KView::FillTriangle(BPoint point1, BPoint point2, BPoint point3, BRect bounds,
	const BGradient& gradient)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	fOwner->fLink->StartMessage(AS_FILL_TRIANGLE_GRADIENT);
	fOwner->fLink->Attach<BPoint>(point1);
	fOwner->fLink->Attach<BPoint>(point2);
	fOwner->fLink->Attach<BPoint>(point3);
	fOwner->fLink->Attach<BRect>(bounds);
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
KView::StrokeLine(BPoint toPoint, ::pattern pattern)
{
	StrokeLine(PenLocation(), toPoint, pattern);
}


void
KView::StrokeLine(BPoint start, BPoint end, ::pattern pattern)
{
	if (fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	ViewStrokeLineInfo info;
	info.startPoint = start;
	info.endPoint = end;

	fOwner->fLink->StartMessage(AS_STROKE_LINE);
	fOwner->fLink->Attach<ViewStrokeLineInfo>(info);

	_FlushIfNotInTransaction();

	// this modifies our pen location, so we invalidate the flag.
	fState->valid_flags &= ~B_VIEW_PEN_LOCATION_BIT;
}


void
KView::StrokeShape(BShape* shape, ::pattern pattern)
{
	if (shape == NULL || fOwner == NULL)
		return;

	shape_data* sd = (shape_data*)shape->fPrivateData;
	if (sd->opCount == 0 || sd->ptCount == 0)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_STROKE_SHAPE);
	fOwner->fLink->Attach<BRect>(shape->Bounds());
	fOwner->fLink->Attach<int32>(sd->opCount);
	fOwner->fLink->Attach<int32>(sd->ptCount);
	fOwner->fLink->Attach(sd->opList, sd->opCount * sizeof(uint32));
	fOwner->fLink->Attach(sd->ptList, sd->ptCount * sizeof(BPoint));

	_FlushIfNotInTransaction();
}


void
KView::FillShape(BShape* shape, ::pattern pattern)
{
	if (shape == NULL || fOwner == NULL)
		return;

	shape_data* sd = (shape_data*)(shape->fPrivateData);
	if (sd->opCount == 0 || sd->ptCount == 0)
		return;

	_CheckLockAndSwitchCurrent();
	_UpdatePattern(pattern);

	fOwner->fLink->StartMessage(AS_FILL_SHAPE);
	fOwner->fLink->Attach<BRect>(shape->Bounds());
	fOwner->fLink->Attach<int32>(sd->opCount);
	fOwner->fLink->Attach<int32>(sd->ptCount);
	fOwner->fLink->Attach(sd->opList, sd->opCount * sizeof(int32));
	fOwner->fLink->Attach(sd->ptList, sd->ptCount * sizeof(BPoint));

	_FlushIfNotInTransaction();
}


void
KView::FillShape(BShape* shape, const BGradient& gradient)
{
	if (shape == NULL || fOwner == NULL)
		return;

	shape_data* sd = (shape_data*)(shape->fPrivateData);
	if (sd->opCount == 0 || sd->ptCount == 0)
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_FILL_SHAPE_GRADIENT);
	fOwner->fLink->Attach<BRect>(shape->Bounds());
	fOwner->fLink->Attach<int32>(sd->opCount);
	fOwner->fLink->Attach<int32>(sd->ptCount);
	fOwner->fLink->Attach(sd->opList, sd->opCount * sizeof(int32));
	fOwner->fLink->Attach(sd->ptList, sd->ptCount * sizeof(BPoint));
	fOwner->fLink->AttachGradient(gradient);

	_FlushIfNotInTransaction();
}


void
KView::BeginLineArray(int32 count)
{
	if (fOwner == NULL)
		return;

	if (count <= 0)
		debugger("Calling BeginLineArray with a count <= 0");

	_CheckLock();

	if (fCommArray) {
		debugger("Can't nest BeginLineArray calls");
			// not fatal, but it helps during
			// development of your app and is in
			// line with R5...
		delete[] fCommArray->array;
		delete fCommArray;
	}

	// TODO: since this method cannot return failure, and further AddLine()
	//	calls with a NULL fCommArray would drop into the debugger anyway,
	//	we allow the possible std::bad_alloc exceptions here...
	fCommArray = new _array_data_;
	fCommArray->count = 0;

	// Make sure the fCommArray is initialized to reasonable values in cases of
	// bad_alloc. At least the exception can be caught and EndLineArray won't
	// crash.
	fCommArray->array = NULL;
	fCommArray->maxCount = 0;

	fCommArray->array = new ViewLineArrayInfo[count];
	fCommArray->maxCount = count;
}


void
KView::AddLine(BPoint start, BPoint end, rgb_color color)
{
	if (fOwner == NULL)
		return;

	if (!fCommArray)
		debugger("BeginLineArray must be called before using AddLine");

	_CheckLock();

	const uint32 &arrayCount = fCommArray->count;
	if (arrayCount < fCommArray->maxCount) {
		fCommArray->array[arrayCount].startPoint = start;
		fCommArray->array[arrayCount].endPoint = end;
		fCommArray->array[arrayCount].color = color;

		fCommArray->count++;
	}
}


void
KView::EndLineArray()
{
	if (fOwner == NULL)
		return;

	if (fCommArray == NULL)
		debugger("Can't call EndLineArray before BeginLineArray");

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_STROKE_LINEARRAY);
	fOwner->fLink->Attach<int32>(fCommArray->count);
	fOwner->fLink->Attach(fCommArray->array,
		fCommArray->count * sizeof(ViewLineArrayInfo));

	_FlushIfNotInTransaction();

	_RemoveCommArray();
}


void
KView::SetDiskMode(char* filename, long offset)
{
	// TODO: implement
	// One BeBook version has this to say about SetDiskMode():
	//
	// "Begins recording a picture to the file with the given filename
	// at the given offset. Subsequent drawing commands sent to the view
	// will be written to the file until EndPicture() is called. The
	// stored commands may be played from the file with DrawPicture()."
}


void
KView::BeginPicture(KPicture* picture)
{
	if (_CheckOwnerLockAndSwitchCurrent()
		&& picture && picture->fUsurped == NULL) {
		picture->Usurp(fCurrentPicture);
		fCurrentPicture = picture;

		fOwner->fLink->StartMessage(AS_VIEW_BEGIN_PICTURE);
	}
}


void
KView::AppendToPicture(KPicture* picture)
{
	_CheckLockAndSwitchCurrent();

	if (picture && picture->fUsurped == NULL) {
		int32 token = picture->Token();

		if (token == -1) {
			BeginPicture(picture);
		} else {
			picture->SetToken(-1);
			picture->Usurp(fCurrentPicture);
			fCurrentPicture = picture;
			fOwner->fLink->StartMessage(AS_VIEW_APPEND_TO_PICTURE);
			fOwner->fLink->Attach<int32>(token);
		}
	}
}


KPicture*
KView::EndPicture()
{
	if (_CheckOwnerLockAndSwitchCurrent() && fCurrentPicture) {
		int32 token;

		fOwner->fLink->StartMessage(AS_VIEW_END_PICTURE);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK
			&& fOwner->fLink->Read<int32>(&token) == B_OK) {
			KPicture* picture = fCurrentPicture;
			fCurrentPicture = picture->StepDown();
			picture->SetToken(token);

			// TODO do this more efficient e.g. use a shared area and let the
			// client write into it
			picture->_Download();
			return picture;
		}
	}

	return NULL;
}


void
KView::SetViewBitmap(const KBitmap* bitmap, BRect srcRect, BRect dstRect,
	uint32 followFlags, uint32 options)
{
	_SetViewBitmap(bitmap, srcRect, dstRect, followFlags, options);
}


void
KView::SetViewBitmap(const KBitmap* bitmap, uint32 followFlags, uint32 options)
{
	BRect rect;
 	if (bitmap)
		rect = bitmap->Bounds();

 	rect.OffsetTo(B_ORIGIN);

	_SetViewBitmap(bitmap, rect, rect, followFlags, options);
}


void
KView::ClearViewBitmap()
{
	_SetViewBitmap(NULL, BRect(), BRect(), 0, 0);
}


status_t
KView::SetViewOverlay(const KBitmap* overlay, BRect srcRect, BRect dstRect,
	rgb_color* colorKey, uint32 followFlags, uint32 options)
{
	if (overlay == NULL || (overlay->fFlags & B_BITMAP_WILL_OVERLAY) == 0)
		return B_BAD_VALUE;

	status_t status = _SetViewBitmap(overlay, srcRect, dstRect, followFlags,
		options | AS_REQUEST_COLOR_KEY);
	if (status == B_OK) {
		// read the color that will be treated as transparent
		fOwner->fLink->Read<rgb_color>(colorKey);
	}

	return status;
}


status_t
KView::SetViewOverlay(const KBitmap* overlay, rgb_color* colorKey,
	uint32 followFlags, uint32 options)
{
	if (overlay == NULL)
		return B_BAD_VALUE;

	BRect rect = overlay->Bounds();
 	rect.OffsetTo(B_ORIGIN);

	return SetViewOverlay(overlay, rect, rect, colorKey, followFlags, options);
}


void
KView::ClearViewOverlay()
{
	_SetViewBitmap(NULL, BRect(), BRect(), 0, 0);
}


void
KView::CopyBits(BRect src, BRect dst)
{
	if (fOwner == NULL)
		return;

	if (!src.IsValid() || !dst.IsValid())
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_VIEW_COPY_BITS);
	fOwner->fLink->Attach<BRect>(src);
	fOwner->fLink->Attach<BRect>(dst);

	_FlushIfNotInTransaction();
}


void
KView::DrawPicture(const KPicture* picture)
{
	if (picture == NULL)
		return;

	DrawPictureAsync(picture, PenLocation());
	Sync();
}


void
KView::DrawPicture(const KPicture* picture, BPoint where)
{
	if (picture == NULL)
		return;

	DrawPictureAsync(picture, where);
	Sync();
}


void
KView::DrawPicture(const char* filename, long offset, BPoint where)
{
	if (!filename)
		return;

	DrawPictureAsync(filename, offset, where);
	Sync();
}


void
KView::DrawPictureAsync(const KPicture* picture)
{
	if (picture == NULL)
		return;

	DrawPictureAsync(picture, PenLocation());
}


void
KView::DrawPictureAsync(const KPicture* picture, BPoint where)
{
	if (picture == NULL)
		return;

	if (_CheckOwnerLockAndSwitchCurrent() && picture->Token() > 0) {
		fOwner->fLink->StartMessage(AS_VIEW_DRAW_PICTURE);
		fOwner->fLink->Attach<int32>(picture->Token());
		fOwner->fLink->Attach<BPoint>(where);

		_FlushIfNotInTransaction();
	}
}


void
KView::DrawPictureAsync(const char* filename, long offset, BPoint where)
{
	if (!filename)
		return;

	// TODO: Test
	BFile file(filename, B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

	file.Seek(offset, SEEK_SET);

	KPicture picture;
	if (picture.Unflatten(&file) < B_OK)
		return;

	DrawPictureAsync(&picture, where);
}


void
KView::BeginLayer(uint8 opacity)
{
	if (_CheckOwnerLockAndSwitchCurrent()) {
		fOwner->fLink->StartMessage(AS_VIEW_BEGIN_LAYER);
		fOwner->fLink->Attach<uint8>(opacity);
		_FlushIfNotInTransaction();
	}
}


void
KView::EndLayer()
{
	if (_CheckOwnerLockAndSwitchCurrent()) {
		fOwner->fLink->StartMessage(AS_VIEW_END_LAYER);
		_FlushIfNotInTransaction();
	}
}


void
KView::Invalidate(BRect invalRect)
{
	if (fOwner == NULL)
		return;

	// NOTE: This rounding of the invalid rect is to stay compatible with BeOS.
	// On the server side, the invalid rect will be converted to a BRegion,
	// which rounds in a different manner, so that it really includes the
	// fractional coordinates of a BRect (ie ceilf(rect.right) &
	// ceilf(rect.bottom)), which is also what BeOS does. So we have to do the
	// different rounding here to stay compatible in both ways.
	invalRect.left = (int)invalRect.left;
	invalRect.top = (int)invalRect.top;
	invalRect.right = (int)invalRect.right;
	invalRect.bottom = (int)invalRect.bottom;
	if (!invalRect.IsValid())
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_VIEW_INVALIDATE_RECT);
	fOwner->fLink->Attach<BRect>(invalRect);

// TODO: determine why this check isn't working correctly.
#if 0
	if (!fOwner->fUpdateRequested) {
		fOwner->fLink->Flush();
		fOwner->fUpdateRequested = true;
	}
#else
	fOwner->fLink->Flush();
#endif
}


void
KView::Invalidate(const BRegion* region)
{
debug_printf("[KView]{Invalidate with BRegion}\n");
	if (region == NULL || fOwner == NULL)
		return;

	_CheckLockAndSwitchCurrent();

debug_printf("[KView]{Invalidate with BRegion} Sending AS_VIEW_INVALIDATE_REGION\n");
	fOwner->fLink->StartMessage(AS_VIEW_INVALIDATE_REGION);
	fOwner->fLink->AttachRegion(*region);

// TODO: See above.
#if 0
	if (!fOwner->fUpdateRequested) {
		fOwner->fLink->Flush();
		fOwner->fUpdateRequested = true;
	}
#else
	fOwner->fLink->Flush();
#endif

debug_printf("[KView]{Invalidate with BRegion}end\n");
}


void
KView::Invalidate()
{
debug_printf("[KView]{Invalidate}\n");
	Invalidate(Bounds());
debug_printf("[KView]{Invalidate}end\n");
}


void
KView::DelayedInvalidate(bigtime_t delay)
{
	DelayedInvalidate(delay, Bounds());
}


void
KView::DelayedInvalidate(bigtime_t delay, BRect invalRect)
{
	if (fOwner == NULL)
		return;

	invalRect.left = (int)invalRect.left;
	invalRect.top = (int)invalRect.top;
	invalRect.right = (int)invalRect.right;
	invalRect.bottom = (int)invalRect.bottom;
	if (!invalRect.IsValid())
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_VIEW_DELAYED_INVALIDATE_RECT);
	fOwner->fLink->Attach<bigtime_t>(system_time() + delay);
	fOwner->fLink->Attach<BRect>(invalRect);
	fOwner->fLink->Flush();
}


void
KView::InvertRect(BRect rect)
{
	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_INVERT_RECT);
		fOwner->fLink->Attach<BRect>(rect);

		_FlushIfNotInTransaction();
	}
}


//	#pragma mark - View Hierarchy Functions


void
KView::AddChild(KView* child, KView* before)
{
	STRACE(("KView(%s)::AddChild(child '%s', before '%s')\n",
		this->Name(),
		child != NULL && child->Name() ? child->Name() : "NULL",
		before != NULL && before->Name() ? before->Name() : "NULL"));

	if (!_AddChild(child, before))
		return;

	if (fLayoutData->fLayout)
		fLayoutData->fLayout->AddView(child);
}


bool
KView::AddChild(KLayoutItem* child)
{
	if (!fLayoutData->fLayout)
		return false;
	return fLayoutData->fLayout->AddItem(child);
}


bool
KView::_AddChild(KView* child, KView* before)
{
	if (!child)
		return false;

	if (child->fParent != NULL) {
		debugger("AddChild failed - the view already has a parent.");
		return false;
	}

	if (child == this) {
		debugger("AddChild failed - cannot add a view to itself.");
		return false;
	}

	bool lockedOwner = false;
	if (fOwner && !fOwner->IsLocked()) {
		fOwner->Lock();
		lockedOwner = true;
	}

	if (!_AddChildToList(child, before)) {
		debugger("AddChild failed!");
		if (lockedOwner)
			fOwner->Unlock();
		return false;
	}

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		child->_SetOwner(fOwner);
		child->_CreateSelf();
		child->_Attach();

		if (lockedOwner)
			fOwner->Unlock();
	}

	InvalidateLayout();

	return true;
}


bool
KView::RemoveChild(KView* child)
{
	STRACE(("KView(%s)::RemoveChild(%s)\n", Name(), child->Name()));

	if (!child)
		return false;

	if (child->fParent != this)
		return false;

	return child->RemoveSelf();
}


int32
KView::CountChildren() const
{
	_CheckLock();

	uint32 count = 0;
	KView* child = fFirstChild;

	while (child != NULL) {
		count++;
		child = child->fNextSibling;
	}

	return count;
}


KView*
KView::ChildAt(int32 index) const
{
	_CheckLock();

	KView* child = fFirstChild;
	while (child != NULL && index-- > 0) {
		child = child->fNextSibling;
	}

	return child;
}


KView*
KView::NextSibling() const
{
	return fNextSibling;
}


KView*
KView::PreviousSibling() const
{
	return fPreviousSibling;
}


bool
KView::RemoveSelf()
{
	_RemoveLayoutItemsFromLayout(false);

	return _RemoveSelf();
}


bool
KView::_RemoveSelf()
{
	STRACE(("KView(%s)::_RemoveSelf()\n", Name()));

	// Remove this child from its parent

	KWindow* owner = fOwner;
	_CheckLock();

	if (owner != NULL) {
		_UpdateStateForRemove();
		_Detach();
	}

	KView* parent = fParent;
	if (!parent || !parent->_RemoveChildFromList(this))
		return false;

	if (owner != NULL && !fTopLevelView) {
		// the top level view is deleted by the app_server automatically
		owner->fLink->StartMessage(AS_VIEW_DELETE);
		owner->fLink->Attach<int32>(_get_object_token_(this));
	}

	parent->InvalidateLayout();

	STRACE(("DONE: KView(%s)::_RemoveSelf()\n", Name()));

	return true;
}


void
KView::_RemoveLayoutItemsFromLayout(bool deleteItems)
{
	if (fParent == NULL || fParent->fLayoutData->fLayout == NULL)
		return;

	int32 index = fLayoutData->fLayoutItems.CountItems();
	while (index-- > 0) {
		KLayoutItem* item = fLayoutData->fLayoutItems.ItemAt(index);
		item->RemoveSelf();
			// Removes item from fLayoutItems list
		if (deleteItems)
			delete item;
	}
}


KView*
KView::Parent() const
{
	if (fParent && fParent->fTopLevelView)
		return NULL;

	return fParent;
}


KView*
KView::FindView(const char* name) const
{
	if (name == NULL)
		return NULL;

	if (Name() != NULL && !strcmp(Name(), name))
		return const_cast<KView*>(this);

	KView* child = fFirstChild;
	while (child != NULL) {
		KView* view = child->FindView(name);
		if (view != NULL)
			return view;

		child = child->fNextSibling;
	}

	return NULL;
}


void
KView::MoveBy(float deltaX, float deltaY)
{
	MoveTo(fParentOffset.x + roundf(deltaX), fParentOffset.y + roundf(deltaY));
}


void
KView::MoveTo(BPoint where)
{
	MoveTo(where.x, where.y);
}


void
KView::MoveTo(float x, float y)
{
debug_printf("[KView]{MoveTo}\n");
	if (x == fParentOffset.x && y == fParentOffset.y)
		return;

	// BeBook says we should do this. And it makes sense.
	x = roundf(x);
	y = roundf(y);

	if (fOwner) {
	debug_printf("[KView]{MoveTo} fOwner\n");
		_CheckLockAndSwitchCurrent();
		debug_printf("[KView]{MoveTo}Sending AS_VIEW_MOVE_TO\n");
		fOwner->fLink->StartMessage(AS_VIEW_MOVE_TO);
		fOwner->fLink->Attach<float>(x);
		fOwner->fLink->Attach<float>(y);

//		fState->valid_flags |= B_VIEW_FRAME_BIT;

		_FlushIfNotInTransaction();
	}

	_MoveTo((int32)x, (int32)y);

debug_printf("[KView]{MoveTo}end\n");
}


void
KView::ResizeBy(float deltaWidth, float deltaHeight)
{
debug_printf("[KView]{ResizeBy}\n");
	// BeBook says we should do this. And it makes sense.
	deltaWidth = roundf(deltaWidth);
	deltaHeight = roundf(deltaHeight);

	if (deltaWidth == 0 && deltaHeight == 0)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();
		fOwner->fLink->StartMessage(AS_VIEW_RESIZE_TO);

		fOwner->fLink->Attach<float>(fBounds.Width() + deltaWidth);
		fOwner->fLink->Attach<float>(fBounds.Height() + deltaHeight);

//		fState->valid_flags |= B_VIEW_FRAME_BIT;

		_FlushIfNotInTransaction();
	}

	_ResizeBy((int32)deltaWidth, (int32)deltaHeight);
}


void
KView::ResizeTo(float width, float height)
{
debug_printf("[KView]{ResizeTo} with width and height\n");
	ResizeBy(width - fBounds.Width(), height - fBounds.Height());
}


void
KView::ResizeTo(BSize size)
{
	ResizeBy(size.width - fBounds.Width(), size.height - fBounds.Height());
}


//	#pragma mark - Inherited Methods (from BHandler)


status_t
KView::GetSupportedSuites(BMessage* data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t status = data->AddString("suites", "suite/vnd.Be-view");
	BPropertyInfo propertyInfo(sViewPropInfo);
	if (status == B_OK)
		status = data->AddFlat("messages", &propertyInfo);
	if (status == B_OK)
		return BHandler::GetSupportedSuites(data);
	return status;
}


BHandler*
KView::ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier,
	int32 what, const char* property)
{
	if (message->what == B_WINDOW_MOVE_BY
		|| message->what == B_WINDOW_MOVE_TO) {
		return this;
	}

	BPropertyInfo propertyInfo(sViewPropInfo);
	status_t err = B_BAD_SCRIPT_SYNTAX;
	BMessage replyMsg(B_REPLY);

	switch (propertyInfo.FindMatch(message, index, specifier, what, property)) {
		case 0:
		case 1:
		case 3:
			return this;

		case 2:
			if (fShelf) {
				message->PopSpecifier();
				return fShelf;
			}

			err = B_NAME_NOT_FOUND;
			replyMsg.AddString("message", "This window doesn't have a shelf");
			break;

		case 4:
		{
			if (!fFirstChild) {
				err = B_NAME_NOT_FOUND;
				replyMsg.AddString("message", "This window doesn't have "
					"children.");
				break;
			}
			KView* child = NULL;
			switch (what) {
				case B_INDEX_SPECIFIER:
				{
					int32 index;
					err = specifier->FindInt32("index", &index);
					if (err == B_OK)
						child = ChildAt(index);
					break;
				}
				case B_REVERSE_INDEX_SPECIFIER:
				{
					int32 rindex;
					err = specifier->FindInt32("index", &rindex);
					if (err == B_OK)
						child = ChildAt(CountChildren() - rindex);
					break;
				}
				case B_NAME_SPECIFIER:
				{
					const char* name;
					err = specifier->FindString("name", &name);
					if (err == B_OK)
						child = FindView(name);
					break;
				}
			}

			if (child != NULL) {
				message->PopSpecifier();
				return child;
			}

			if (err == B_OK)
				err = B_BAD_INDEX;

			replyMsg.AddString("message",
				"Cannot find view at/with specified index/name.");
			break;
		}

		default:
			return BHandler::ResolveSpecifier(message, index, specifier, what,
				property);
	}

	if (err < B_OK) {
		replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;

		if (err == B_BAD_SCRIPT_SYNTAX)
			replyMsg.AddString("message", "Didn't understand the specifier(s)");
		else
			replyMsg.AddString("message", strerror(err));
	}

	replyMsg.AddInt32("error", err);
	message->SendReply(&replyMsg);
	return NULL;
}


void
KView::MessageReceived(BMessage* message)
{
	if (!message->HasSpecifiers()) {
		switch (message->what) {
			case B_INVALIDATE:
			{
				BRect rect;
				if (message->FindRect("be:area", &rect) == B_OK)
					Invalidate(rect);
				else
					Invalidate();
				break;
			}

			case B_KEY_DOWN:
			{
				// TODO: cannot use "string" here if we support having different
				// font encoding per view (it's supposed to be converted by
				// KWindow::_HandleKeyDown() one day)
				const char* string;
				ssize_t bytes;
				if (message->FindData("bytes", B_STRING_TYPE,
						(const void**)&string, &bytes) == B_OK)
					KeyDown(string, bytes - 1);
				break;
			}

			case B_KEY_UP:
			{
				// TODO: same as above
				const char* string;
				ssize_t bytes;
				if (message->FindData("bytes", B_STRING_TYPE,
						(const void**)&string, &bytes) == B_OK)
					KeyUp(string, bytes - 1);
				break;
			}

			case B_VIEW_RESIZED:
				FrameResized(message->GetInt32("width", 0),
					message->GetInt32("height", 0));
				break;

			case B_VIEW_MOVED:
				FrameMoved(fParentOffset);
				break;

			case B_MOUSE_DOWN:
			{
				BPoint where;
				message->FindPoint("be:view_where", &where);
				MouseDown(where);
				break;
			}

			case B_MOUSE_IDLE:
			{
				BPoint where;
				if (message->FindPoint("be:view_where", &where) != B_OK)
					break;

				KToolTip* tip;
				if (GetToolTipAt(where, &tip))
					ShowToolTip(tip);
				else
					BHandler::MessageReceived(message);
				break;
			}

			case B_MOUSE_MOVED:
			{
				uint32 eventOptions = fEventOptions | fMouseEventOptions;
				bool noHistory = eventOptions & B_NO_POINTER_HISTORY;
				bool dropIfLate = !(eventOptions & B_FULL_POINTER_HISTORY);

				bigtime_t eventTime;
				if (message->FindInt64("when", (int64*)&eventTime) < B_OK)
					eventTime = system_time();

				uint32 transit;
				message->FindInt32("be:transit", (int32*)&transit);
				// don't drop late messages with these important transit values
				if (transit == B_ENTERED_VIEW || transit == B_EXITED_VIEW)
					dropIfLate = false;

				// TODO: The dropping code may have the following problem: On
				// slower computers, 20ms may just be to abitious a delay.
				// There, we might constantly check the message queue for a
				// newer message, not find any, and still use the only but later
				// than 20ms message, which of course makes the whole thing
				// later than need be. An adaptive delay would be kind of neat,
				// but would probably use additional KWindow members to count
				// the successful versus fruitless queue searches and the delay
				// value itself or something similar.
				if (noHistory
					|| (dropIfLate && (system_time() - eventTime > 20000))) {
					// filter out older mouse moved messages in the queue
					KWindow* window = Window();
					window->_DequeueAll();
					BMessageQueue* queue = window->MessageQueue();
					queue->Lock();

					BMessage* moved;
					for (int32 i = 0; (moved = queue->FindMessage(i)) != NULL;
						 i++) {
						if (moved != message && moved->what == B_MOUSE_MOVED) {
							// there is a newer mouse moved message in the
							// queue, just ignore the current one, the newer one
							// will be handled here eventually
							queue->Unlock();
							return;
						}
					}
					queue->Unlock();
				}

				BPoint where;
				uint32 buttons;
				message->FindPoint("be:view_where", &where);
				message->FindInt32("buttons", (int32*)&buttons);

				if (transit == B_EXITED_VIEW || transit == B_OUTSIDE_VIEW)
					HideToolTip();

				BMessage* dragMessage = NULL;
				if (message->HasMessage("be:drag_message")) {
					dragMessage = new BMessage();
					if (message->FindMessage("be:drag_message", dragMessage)
						!= B_OK) {
						delete dragMessage;
						dragMessage = NULL;
					}
				}

				MouseMoved(where, transit, dragMessage);
				delete dragMessage;
				break;
			}

			case B_MOUSE_UP:
			{
				BPoint where;
				message->FindPoint("be:view_where", &where);
				fMouseEventOptions = 0;
				MouseUp(where);
				break;
			}

			case B_MOUSE_WHEEL_CHANGED:
			{
				KScrollBar* horizontal = ScrollBar(B_HORIZONTAL);
				KScrollBar* vertical = ScrollBar(B_VERTICAL);
				if (horizontal == NULL && vertical == NULL) {
					// Pass the message to the next handler
					BHandler::MessageReceived(message);
					break;
				}

				float deltaX = 0.0f;
				float deltaY = 0.0f;

				if (horizontal != NULL)
					message->FindFloat("be:wheel_delta_x", &deltaX);

				if (vertical != NULL)
					message->FindFloat("be:wheel_delta_y", &deltaY);

				if (deltaX == 0.0f && deltaY == 0.0f)
					break;

				if ((modifiers() & B_CONTROL_KEY) != 0)
					std::swap(horizontal, vertical);

				if (horizontal != NULL && deltaX != 0.0f)
					ScrollWithMouseWheelDelta(horizontal, deltaX);

				if (vertical != NULL && deltaY != 0.0f)
					ScrollWithMouseWheelDelta(vertical, deltaY);

				break;
			}

			// prevent message repeats
			case B_COLORS_UPDATED:
			case B_FONTS_UPDATED:
				break;

			case B_SCREEN_CHANGED:
			{
				// propegate message to child views
				int32 childCount = CountChildren();
				for (int32 i = 0; i < childCount; i++) {
					KView* view = ChildAt(i);
					if (view != NULL)
						view->MessageReceived(message);
				}
				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}

		return;
	}

	// Scripting message

	BMessage replyMsg(B_REPLY);
	status_t err = B_BAD_SCRIPT_SYNTAX;
	int32 index;
	BMessage specifier;
	int32 what;
	const char* property;

	if (message->GetCurrentSpecifier(&index, &specifier, &what, &property)
			!= B_OK) {
		return BHandler::MessageReceived(message);
	}

	BPropertyInfo propertyInfo(sViewPropInfo);
	switch (propertyInfo.FindMatch(message, index, &specifier, what,
			property)) {
		case 0:
			if (message->what == B_GET_PROPERTY) {
				err = replyMsg.AddRect("result", Frame());
			} else if (message->what == B_SET_PROPERTY) {
				BRect newFrame;
				err = message->FindRect("data", &newFrame);
				if (err == B_OK) {
					MoveTo(newFrame.LeftTop());
					ResizeTo(newFrame.Width(), newFrame.Height());
				}
			}
			break;
		case 1:
			if (message->what == B_GET_PROPERTY) {
				err = replyMsg.AddBool("result", IsHidden());
			} else if (message->what == B_SET_PROPERTY) {
				bool newHiddenState;
				err = message->FindBool("data", &newHiddenState);
				if (err == B_OK) {
					if (newHiddenState == true)
						Hide();
					else
						Show();
				}
			}
			break;
		case 3:
			err = replyMsg.AddInt32("result", CountChildren());
			break;
		default:
			return BHandler::MessageReceived(message);
	}

	if (err != B_OK) {
		replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;

		if (err == B_BAD_SCRIPT_SYNTAX)
			replyMsg.AddString("message", "Didn't understand the specifier(s)");
		else
			replyMsg.AddString("message", strerror(err));

		replyMsg.AddInt32("error", err);
	}

	message->SendReply(&replyMsg);
}


status_t
KView::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= KView::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= KView::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= KView::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= KView::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= KView::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			KView::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			k_perform_data_set_layout* data = (k_perform_data_set_layout*)_data;
			KView::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			KView::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			KView::DoLayout();
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_CHANGED:
		{
			KView::LayoutChanged();
			return B_OK;
		}
		case PERFORM_CODE_GET_TOOL_TIP_AT:
		{
			k_perform_data_get_tool_tip_at* data
				= (k_perform_data_get_tool_tip_at*)_data;
			data->return_value
				= KView::GetToolTipAt(data->point, data->tool_tip);
			return B_OK;
		}
		case PERFORM_CODE_ALL_UNARCHIVED:
		{
			perform_data_all_unarchived* data =
				(perform_data_all_unarchived*)_data;

			data->return_value = KView::AllUnarchived(data->archive);
			return B_OK;
		}
		case PERFORM_CODE_ALL_ARCHIVED:
		{
			perform_data_all_archived* data =
				(perform_data_all_archived*)_data;

			data->return_value = KView::AllArchived(data->archive);
			return B_OK;
		}
	}

	return BHandler::Perform(code, _data);
}


// #pragma mark - Layout Functions


BSize
KView::MinSize()
{
	// TODO: make sure this works correctly when some methods are overridden
	float width, height;
	GetPreferredSize(&width, &height);

	return KLayoutUtils::ComposeSize(fLayoutData->fMinSize,
		(fLayoutData->fLayout ? fLayoutData->fLayout->MinSize()
			: BSize(width, height)));
}


BSize
KView::MaxSize()
{
	return KLayoutUtils::ComposeSize(fLayoutData->fMaxSize,
		(fLayoutData->fLayout ? fLayoutData->fLayout->MaxSize()
			: BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED)));
}


BSize
KView::PreferredSize()
{
	// TODO: make sure this works correctly when some methods are overridden
	float width, height;
	GetPreferredSize(&width, &height);

	return KLayoutUtils::ComposeSize(fLayoutData->fPreferredSize,
		(fLayoutData->fLayout ? fLayoutData->fLayout->PreferredSize()
			: BSize(width, height)));
}


BAlignment
KView::LayoutAlignment()
{
	return KLayoutUtils::ComposeAlignment(fLayoutData->fAlignment,
		(fLayoutData->fLayout ? fLayoutData->fLayout->Alignment()
			: BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER)));
}


void
KView::SetExplicitMinSize(BSize size)
{
	fLayoutData->fMinSize = size;
	InvalidateLayout();
}


void
KView::SetExplicitMaxSize(BSize size)
{
	fLayoutData->fMaxSize = size;
	InvalidateLayout();
}


void
KView::SetExplicitPreferredSize(BSize size)
{
	fLayoutData->fPreferredSize = size;
	InvalidateLayout();
}


void
KView::SetExplicitSize(BSize size)
{
	fLayoutData->fMinSize = size;
	fLayoutData->fMaxSize = size;
	fLayoutData->fPreferredSize = size;
	InvalidateLayout();
}


void
KView::SetExplicitAlignment(BAlignment alignment)
{
	fLayoutData->fAlignment = alignment;
	InvalidateLayout();
}


BSize
KView::ExplicitMinSize() const
{
	return fLayoutData->fMinSize;
}


BSize
KView::ExplicitMaxSize() const
{
	return fLayoutData->fMaxSize;
}


BSize
KView::ExplicitPreferredSize() const
{
	return fLayoutData->fPreferredSize;
}


BAlignment
KView::ExplicitAlignment() const
{
	return fLayoutData->fAlignment;
}


bool
KView::HasHeightForWidth()
{
	return (fLayoutData->fLayout
		? fLayoutData->fLayout->HasHeightForWidth() : false);
}


void
KView::GetHeightForWidth(float width, float* min, float* max, float* preferred)
{
	if (fLayoutData->fLayout)
		fLayoutData->fLayout->GetHeightForWidth(width, min, max, preferred);
}


void
KView::SetLayout(KLayout* layout)
{
	if (layout == fLayoutData->fLayout)
		return;

	if (layout && layout->Layout())
		debugger("KView::SetLayout() failed, layout is already in use.");

	fFlags |= B_SUPPORTS_LAYOUT;

	// unset and delete the old layout
	if (fLayoutData->fLayout) {
		fLayoutData->fLayout->RemoveSelf();
		fLayoutData->fLayout->SetOwner(NULL);
		delete fLayoutData->fLayout;
	}

	fLayoutData->fLayout = layout;

	if (fLayoutData->fLayout) {
		fLayoutData->fLayout->SetOwner(this);

		// add all children
		int count = CountChildren();
		for (int i = 0; i < count; i++)
			fLayoutData->fLayout->AddView(ChildAt(i));
	}

	InvalidateLayout();
}


KLayout*
KView::GetLayout() const
{
	return fLayoutData->fLayout;
}


void
KView::InvalidateLayout(bool descendants)
{
debug_printf("[KView]{InvalidateLayout}start\n");
	// printf("KView(%p)::InvalidateLayout(%i), valid: %i, inProgress: %i\n",
	//	this, descendants, fLayoutData->fLayoutValid,
	//	fLayoutData->fLayoutInProgress);

	if (!fLayoutData->fMinMaxValid || fLayoutData->fLayoutInProgress
 			|| fLayoutData->fLayoutInvalidationDisabled > 0) {
		return;
	}
	fLayoutData->fLayoutValid = false;
	fLayoutData->fMinMaxValid = false;
	LayoutInvalidated(descendants);

	if (descendants) {
		for (KView* child = fFirstChild;
			child; child = child->fNextSibling) {
			child->InvalidateLayout(descendants);
		}
	}

	if (fLayoutData->fLayout)
		fLayoutData->fLayout->InvalidateLayout(descendants);
	else
		_InvalidateParentLayout();

	if (fTopLevelView
		&& fOwner != NULL)
		fOwner->PostMessage(B_LAYOUT_WINDOW);

debug_printf("[KView]{InvalidateLayout}start\n");
}


void
KView::EnableLayoutInvalidation()
{
	if (fLayoutData->fLayoutInvalidationDisabled > 0)
		fLayoutData->fLayoutInvalidationDisabled--;
}


void
KView::DisableLayoutInvalidation()
{
	fLayoutData->fLayoutInvalidationDisabled++;
}


bool
KView::IsLayoutInvalidationDisabled()
{
	if (fLayoutData->fLayoutInvalidationDisabled > 0)
		return true;
	return false;
}


bool
KView::IsLayoutValid() const
{
	return fLayoutData->fLayoutValid;
}


void
KView::ResetLayoutInvalidation()
{
	fLayoutData->fMinMaxValid = true;
}


BLayoutContext*
KView::LayoutContext() const
{
	return fLayoutData->fLayoutContext;
}


void
KView::Layout(bool force)
{
	BLayoutContext context;
	_Layout(force, &context);
}


void
KView::Relayout()
{
	if (fLayoutData->fLayoutValid && !fLayoutData->fLayoutInProgress) {
		fLayoutData->fNeedsRelayout = true;
		if (fLayoutData->fLayout)
			fLayoutData->fLayout->RequireLayout();

		// Layout() is recursive, that is if the parent view is currently laid
		// out, we don't call layout() on this view, but wait for the parent's
		// Layout() to do that for us.
		if (!fParent || !fParent->fLayoutData->fLayoutInProgress)
			Layout(false);
	}
}


void
KView::LayoutInvalidated(bool descendants)
{
	// hook method
}


void
KView::DoLayout()
{
	if (fLayoutData->fLayout)
		fLayoutData->fLayout->_LayoutWithinContext(false, LayoutContext());
}


void
KView::SetToolTip(const char* text)
{
	if (text == NULL || text[0] == '\0') {
		SetToolTip((KToolTip*)NULL);
		return;
	}

	if (KTextToolTip* tip = dynamic_cast<KTextToolTip*>(fToolTip))
		tip->SetText(text);
	else
		SetToolTip(new KTextToolTip(text));
}


void
KView::SetToolTip(KToolTip* tip)
{
	if (fToolTip == tip)
		return;
	else if (tip == NULL)
		HideToolTip();

	if (fToolTip != NULL)
		fToolTip->ReleaseReference();

	fToolTip = tip;

	if (fToolTip != NULL)
		fToolTip->AcquireReference();
}


KToolTip*
KView::ToolTip() const
{
	return fToolTip;
}


void
KView::ShowToolTip(KToolTip* tip)
{
	if (tip == NULL)
		return;

	BPoint where;
	GetMouse(&where, NULL, false);

	KToolTipManager::Manager()->ShowTip(tip, ConvertToScreen(where), this);
}


void
KView::HideToolTip()
{
	KToolTipManager::Manager()->HideTip();
}


bool
KView::GetToolTipAt(BPoint point, KToolTip** _tip)
{
	if (fToolTip != NULL) {
		*_tip = fToolTip;
		return true;
	}

	*_tip = NULL;
	return false;
}


void
KView::LayoutChanged()
{
	// hook method
}


void
KView::_Layout(bool force, BLayoutContext* context)
{
//printf("%p->KView::_Layout(%d, %p)\n", this, force, context);
//printf("  fNeedsRelayout: %d, fLayoutValid: %d, fLayoutInProgress: %d\n",
//fLayoutData->fNeedsRelayout, fLayoutData->fLayoutValid,
//fLayoutData->fLayoutInProgress);
	if (fLayoutData->fNeedsRelayout || !fLayoutData->fLayoutValid || force) {
		fLayoutData->fLayoutValid = false;

		if (fLayoutData->fLayoutInProgress)
			return;

		BLayoutContext* oldContext = fLayoutData->fLayoutContext;
		fLayoutData->fLayoutContext = context;

		fLayoutData->fLayoutInProgress = true;
		DoLayout();
		fLayoutData->fLayoutInProgress = false;

		fLayoutData->fLayoutValid = true;
		fLayoutData->fMinMaxValid = true;
		fLayoutData->fNeedsRelayout = false;

		// layout children
		for(KView* child = fFirstChild; child; child = child->fNextSibling) {
			if (!child->IsHidden(child))
				child->_Layout(force, context);
		}

		LayoutChanged();

		fLayoutData->fLayoutContext = oldContext;

		// invalidate the drawn content, if requested
		if (fFlags & B_INVALIDATE_AFTER_LAYOUT)
			Invalidate();
	}
}


void
KView::_LayoutLeft(KLayout* deleted)
{
	// If our layout is added to another layout (via KLayout::AddItem())
	// then we share ownership of our layout. In the event that our layout gets
	// deleted by the layout it has been added to, this method is called so
	// that we don't double-delete our layout.
	if (fLayoutData->fLayout == deleted)
		fLayoutData->fLayout = NULL;
	InvalidateLayout();
}


void
KView::_InvalidateParentLayout()
{
debug_printf("[KView]{_InvalidateParentLayout} start\n");

	if (!fParent)
		return;

	KLayout* layout = fLayoutData->fLayout;
	KLayout* layoutParent = layout ? layout->Layout() : NULL;
	if (layoutParent) {
		layoutParent->InvalidateLayout();
	} else if (fLayoutData->fLayoutItems.CountItems() > 0) {
		int32 count = fLayoutData->fLayoutItems.CountItems();
		for (int32 i = 0; i < count; i++) {
			fLayoutData->fLayoutItems.ItemAt(i)->Layout()->InvalidateLayout();
		}
	} else {
		fParent->InvalidateLayout();
	}

debug_printf("[KView]{_InvalidateParentLayout} end\n");
}


//	#pragma mark - KPrivate Functions


void
KView::_InitData(BRect frame, const char* name, uint32 resizingMode,
	uint32 flags)
{
debug_printf("[KView]{_InitData} Into _InitData...\n");
	// Info: The name of the view is set by BHandler constructor

	STRACE(("KView::_InitData: enter\n"));

	// initialize members
	if ((resizingMode & ~_RESIZE_MASK_) || (flags & _RESIZE_MASK_))
		printf("%s KView::_InitData(): resizing mode or flags swapped\n", name);

	// There are applications that swap the resize mask and the flags in the
	// KView constructor. This does not cause problems under BeOS as it just
	// ors the two fields to one 32bit flag.
	// For now we do the same but print the above warning message.
	// TODO: this should be removed at some point and the original
	// version restored:
	// fFlags = (resizingMode & _RESIZE_MASK_) | (flags & ~_RESIZE_MASK_);
	fFlags = resizingMode | flags;

debug_printf("[KView]{_InitData} fFlags =%d...\n",fFlags);

debug_printf("[KView]{_InitData} before roundf frame.left =%f...\n",frame.left);
debug_printf("[KView]{_InitData} before roundf frame.top =%f...\n",frame.top);
debug_printf("[KView]{_InitData} before roundf frame.right =%f...\n",frame.right);
debug_printf("[KView]{_InitData} before roundf frame.bottom =%f...\n",frame.bottom);

	// handle rounding
	frame.left = roundf(frame.left);
	frame.top = roundf(frame.top);
	frame.right = roundf(frame.right);
	frame.bottom = roundf(frame.bottom);

debug_printf("[KView]{_InitData} after roundf frame.left =%f...\n",frame.left);
debug_printf("[KView]{_InitData} after roundf frame.top =%f...\n",frame.top);
debug_printf("[KView]{_InitData} after roundf frame.right =%f...\n",frame.right);
debug_printf("[KView]{_InitData} after roundf frame.bottom =%f...\n",frame.bottom);

	fParentOffset.Set(frame.left, frame.top);

	fOwner = NULL;
	fParent = NULL;
	fNextSibling = NULL;
	fPreviousSibling = NULL;
	fFirstChild = NULL;

	fShowLevel = 0;
	fTopLevelView = false;

	fCurrentPicture = NULL;
	fCommArray = NULL;

	fVerScroller = NULL;
	fHorScroller = NULL;

	fIsPrinting = false;
	fAttached = false;

	// TODO: Since we cannot communicate failure, we don't use std::nothrow here
	// TODO: Maybe we could auto-delete those views on AddChild() instead?
	fState = new BPrivate::KViewState;

	fBounds = frame.OffsetToCopy(B_ORIGIN);
debug_printf("[KView]{_InitData} fBounds.left =%f...\n",fBounds.left);
debug_printf("[KView]{_InitData} fBounds.top =%f...\n",fBounds.top);
debug_printf("[KView]{_InitData} fBounds.right =%f...\n",fBounds.right);
debug_printf("[KView]{_InitData} fBounds.bottom =%f...\n",fBounds.bottom);
	fShelf = NULL;

	fEventMask = 0;
	fEventOptions = 0;
	fMouseEventOptions = 0;

	fLayoutData = new LayoutData;

	fToolTip = NULL;

	if ((flags & B_SUPPORTS_LAYOUT) != 0) {

	debug_printf("[KView]{_InitData} (flags & B_SUPPORT_LAYOUT) != 0 ...\n");

		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		SetLowUIColor(ViewUIColor());
		SetHighUIColor(B_PANEL_TEXT_COLOR);
	}

debug_printf("[KView]{_InitData} ends\n");
}


void
KView::_RemoveCommArray()
{
debug_printf("[KView]{_RemoveCommArray} Into _RemoveCommArray...\n");

	if (fCommArray) {
		delete [] fCommArray->array;
		delete fCommArray;
		fCommArray = NULL;
	}
}


void
KView::_SetOwner(KWindow* newOwner)
{
debug_printf("[KView]{_SetOwner} Into SetOwner...\n");

	if (!newOwner)
		_RemoveCommArray();

	if (fOwner != newOwner && fOwner) {
		if (fOwner->fFocus == this)
			MakeFocus(false);

		if (fOwner->fLastMouseMovedView == this)
			fOwner->fLastMouseMovedView = NULL;

		fOwner->RemoveHandler(this);
		if (fShelf)
			fOwner->RemoveHandler(fShelf);
	}

	if (newOwner && newOwner != fOwner) {
		newOwner->AddHandler(this);
		if (fShelf)
			newOwner->AddHandler(fShelf);

		if (fTopLevelView)
			SetNextHandler(newOwner);
		else
			SetNextHandler(fParent);
	}

	fOwner = newOwner;

	for (KView* child = fFirstChild; child != NULL; child = child->fNextSibling)
		child->_SetOwner(newOwner);

debug_printf("[KView]{_SetOwner} ends\n");
}


void
KView::_ClipToPicture(KPicture* picture, BPoint where, bool invert, bool sync)
{
debug_printf("[KView]{_ClipToPicture} start\n");
	if (!_CheckOwnerLockAndSwitchCurrent())
		return;

	if (picture == NULL) {
		fOwner->fLink->StartMessage(AS_VIEW_CLIP_TO_PICTURE);
		fOwner->fLink->Attach<int32>(-1);

		// NOTE: No need to sync here, since the -1 token cannot
		// become invalid on the server.
	} else {
		fOwner->fLink->StartMessage(AS_VIEW_CLIP_TO_PICTURE);
		fOwner->fLink->Attach<int32>(picture->Token());
		fOwner->fLink->Attach<BPoint>(where);
		fOwner->fLink->Attach<bool>(invert);

		// NOTE: "sync" defaults to true in public methods. If you know what
		// you are doing, i.e. if you know your KPicture stays valid, you
		// can avoid the performance impact of syncing. In a use-case where
		// the client creates BPictures on the stack, these BPictures may
		// have issued a AS_DELETE_PICTURE command to the ServerApp when Draw()
		// goes out of scope, and the command is processed earlier in the
		// ServerApp thread than the AS_VIEW_CLIP_TO_PICTURE command in the
		// ServerWindow thread, which will then have the result that no
		// ServerPicture is found of the token.
		if (sync)
			Sync();
	}

debug_printf("[KView]{_ClipToPicture}end\n");
}


void
KView::_ClipToRect(BRect rect, bool inverse)
{
	if (_CheckOwnerLockAndSwitchCurrent()) {
		fOwner->fLink->StartMessage(AS_VIEW_CLIP_TO_RECT);
		fOwner->fLink->Attach<bool>(inverse);
		fOwner->fLink->Attach<BRect>(rect);
		_FlushIfNotInTransaction();
	}
}


void
KView::_ClipToShape(BShape* shape, bool inverse)
{
	if (shape == NULL)
		return;

	shape_data* sd = (shape_data*)shape->fPrivateData;
	if (sd->opCount == 0 || sd->ptCount == 0)
		return;

	if (_CheckOwnerLockAndSwitchCurrent()) {
		fOwner->fLink->StartMessage(AS_VIEW_CLIP_TO_SHAPE);
		fOwner->fLink->Attach<bool>(inverse);
		fOwner->fLink->Attach<int32>(sd->opCount);
		fOwner->fLink->Attach<int32>(sd->ptCount);
		fOwner->fLink->Attach(sd->opList, sd->opCount * sizeof(uint32));
		fOwner->fLink->Attach(sd->ptList, sd->ptCount * sizeof(BPoint));
		_FlushIfNotInTransaction();
	}
}


bool
KView::_RemoveChildFromList(KView* child)
{
	if (child->fParent != this)
		return false;

	if (fFirstChild == child) {
		// it's the first view in the list
		fFirstChild = child->fNextSibling;
	} else {
		// there must be a previous sibling
		child->fPreviousSibling->fNextSibling = child->fNextSibling;
	}

	if (child->fNextSibling)
		child->fNextSibling->fPreviousSibling = child->fPreviousSibling;

	child->fParent = NULL;
	child->fNextSibling = NULL;
	child->fPreviousSibling = NULL;

	return true;
}


bool
KView::_AddChildToList(KView* child, KView* before)
{
	if (!child)
		return false;
	if (child->fParent != NULL) {
		debugger("View already belongs to someone else");
		return false;
	}
	if (before != NULL && before->fParent != this) {
		debugger("Invalid before view");
		return false;
	}

	if (before != NULL) {
		// add view before this one
		child->fNextSibling = before;
		child->fPreviousSibling = before->fPreviousSibling;
		if (child->fPreviousSibling != NULL)
			child->fPreviousSibling->fNextSibling = child;

		before->fPreviousSibling = child;
		if (fFirstChild == before)
			fFirstChild = child;
	} else {
		// add view to the end of the list
		KView* last = fFirstChild;
		while (last != NULL && last->fNextSibling != NULL) {
			last = last->fNextSibling;
		}

		if (last != NULL) {
			last->fNextSibling = child;
			child->fPreviousSibling = last;
		} else {
			fFirstChild = child;
			child->fPreviousSibling = NULL;
		}

		child->fNextSibling = NULL;
	}

	child->fParent = this;
	return true;
}


/*!	\brief Creates the server counterpart of this view.
	This is only done for views that are part of the view hierarchy, ie. when
	they are attached to a window.
	RemoveSelf() deletes the server object again.
*/
bool
KView::_CreateSelf()
{
debug_printf("[KView]{_CreateSelf} Into _CreateSelf...\n");

	// AS_VIEW_CREATE & AS_VIEW_CREATE_ROOT do not use the
	// current view mechanism via _CheckLockAndSwitchCurrent() - the token
	// of the view and its parent are both send to the server.

	if (fTopLevelView)
	{
		fOwner->fLink->StartMessage(AS_CREATE_LAMINATE_JADD);
		debug_printf("[KView]{_CreateSelf} AS_CREATE_LAMINATE_JADD...\n");
	}
	else{
 		fOwner->fLink->StartMessage(AS_VIEW_CREATE_2);
		debug_printf("[KView]{_CreateSelf} AS_VIEW_CREATE_2...\n");
 	}

	fOwner->fLink->Attach<int32>(_get_object_token_(this));
	fOwner->fLink->AttachString(Name());
	fOwner->fLink->Attach<BRect>(Frame());
	fOwner->fLink->Attach<BPoint>(LeftTop());
	fOwner->fLink->Attach<uint32>(ResizingMode());
	fOwner->fLink->Attach<uint32>(fEventMask);
	fOwner->fLink->Attach<uint32>(fEventOptions);
	fOwner->fLink->Attach<uint32>(Flags());
	fOwner->fLink->Attach<bool>(IsHidden(this));
	fOwner->fLink->Attach<rgb_color>(fState->view_color);
	if (fTopLevelView)
		fOwner->fLink->Attach<int32>(B_NULL_TOKEN);
	else
		fOwner->fLink->Attach<int32>(_get_object_token_(fParent));
	fOwner->fLink->Flush();

	_CheckOwnerLockAndSwitchCurrent();
	fState->UpdateServerState(*fOwner->fLink);

	// we create all its children, too

	for (KView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
		child->_CreateSelf();
	}

	fOwner->fLink->Flush();

debug_printf("[KView]{_CreateSelf} ends\n");
	return true;
}


/*!	Sets the new view position.
	It doesn't contact the server, though - the only case where this
	is called outside of MoveTo() is as reaction of moving a view
	in the server (a.k.a. B_WINDOW_RESIZED).
	It also calls the KView's FrameMoved() hook.
*/
void
KView::_MoveTo(int32 x, int32 y)
{
debug_printf("[KView]{_MoveTo}\n");
	fParentOffset.Set(x, y);

	if (Window() != NULL && fFlags & B_FRAME_EVENTS) {
debug_printf("[KView]{_MoveTo} Window() != NULL sending B_VIEW_MOVED\n");
		BMessage moved(B_VIEW_MOVED);
		moved.AddInt64("when", system_time());
		moved.AddPoint("where", BPoint(x, y));

		BMessenger target(this);
		target.SendMessage(&moved);
	}

debug_printf("[KView]{_MoveTo}ends\n");
}


/*!	Computes the actual new frame size and recalculates the size of
	the children as well.
	It doesn't contact the server, though - the only case where this
	is called outside of ResizeBy() is as reaction of resizing a view
	in the server (a.k.a. B_WINDOW_RESIZED).
	It also calls the KView's FrameResized() hook.
*/
void
KView::_ResizeBy(int32 deltaWidth, int32 deltaHeight)
{
	fBounds.right += deltaWidth;
	fBounds.bottom += deltaHeight;

	if (Window() == NULL) {
		// we're not supposed to exercise the resizing code in case
		// we haven't been attached to a window yet
		return;
	}

	// layout the children
	if ((fFlags & B_SUPPORTS_LAYOUT) != 0) {
		Relayout();
	} else {
		for (KView* child = fFirstChild; child; child = child->fNextSibling)
			child->_ParentResizedBy(deltaWidth, deltaHeight);
	}

	if (fFlags & B_FRAME_EVENTS) {
		BMessage resized(B_VIEW_RESIZED);
		resized.AddInt64("when", system_time());
		resized.AddInt32("width", fBounds.IntegerWidth());
		resized.AddInt32("height", fBounds.IntegerHeight());

		BMessenger target(this);
		target.SendMessage(&resized);
	}
}


/*!	Relayouts the view according to its resizing mode. */
void
KView::_ParentResizedBy(int32 x, int32 y)
{
	uint32 resizingMode = fFlags & _RESIZE_MASK_;
	BRect newFrame = Frame();

	// follow with left side
	if ((resizingMode & 0x0F00U) == _VIEW_RIGHT_ << 8)
		newFrame.left += x;
	else if ((resizingMode & 0x0F00U) == _VIEW_CENTER_ << 8)
		newFrame.left += x / 2;

	// follow with right side
	if ((resizingMode & 0x000FU) == _VIEW_RIGHT_)
		newFrame.right += x;
	else if ((resizingMode & 0x000FU) == _VIEW_CENTER_)
		newFrame.right += x / 2;

	// follow with top side
	if ((resizingMode & 0xF000U) == _VIEW_BOTTOM_ << 12)
		newFrame.top += y;
	else if ((resizingMode & 0xF000U) == _VIEW_CENTER_ << 12)
		newFrame.top += y / 2;

	// follow with bottom side
	if ((resizingMode & 0x00F0U) == _VIEW_BOTTOM_ << 4)
		newFrame.bottom += y;
	else if ((resizingMode & 0x00F0U) == _VIEW_CENTER_ << 4)
		newFrame.bottom += y / 2;

	if (newFrame.LeftTop() != fParentOffset) {
		// move view
		_MoveTo((int32)roundf(newFrame.left), (int32)roundf(newFrame.top));
	}

	if (newFrame != Frame()) {
		// resize view
		int32 widthDiff = (int32)(newFrame.Width() - fBounds.Width());
		int32 heightDiff = (int32)(newFrame.Height() - fBounds.Height());
		_ResizeBy(widthDiff, heightDiff);
	}
}


void
KView::_Activate(bool active)
{
	WindowActivated(active);

	for (KView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
		child->_Activate(active);
	}
}


void
KView::_Attach()
{
	if (fOwner != NULL) {
		// unmask state flags to force [re]syncing with the app_server
		fState->valid_flags &= ~(B_VIEW_WHICH_VIEW_COLOR_BIT
			| B_VIEW_WHICH_LOW_COLOR_BIT | B_VIEW_WHICH_HIGH_COLOR_BIT);

		if (fState->which_view_color != B_NO_COLOR)
			SetViewUIColor(fState->which_view_color,
				fState->which_view_color_tint);

		if (fState->which_high_color != B_NO_COLOR)
			SetHighUIColor(fState->which_high_color,
				fState->which_high_color_tint);

		if (fState->which_low_color != B_NO_COLOR)
			SetLowUIColor(fState->which_low_color,
				fState->which_low_color_tint);
	}

	AttachedToWindow();

	fAttached = true;

	// after giving the view a chance to do this itself,
	// check for the B_PULSE_NEEDED flag and make sure the
	// window set's up the pulse messaging
	if (fOwner) {
		if (fFlags & B_PULSE_NEEDED) {
			_CheckLock();
			if (fOwner->fPulseRunner == NULL)
				fOwner->SetPulseRate(fOwner->PulseRate());
		}

		if (!fOwner->IsHidden())
			Invalidate();
	}

	for (KView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
		// we need to check for fAttached as new views could have been
		// added in AttachedToWindow() - and those are already attached
		if (!child->fAttached)
			child->_Attach();
	}

	AllAttached();
}


void
KView::_ColorsUpdated(BMessage* message)
{
	if (fTopLevelView
		&& fLayoutData->fLayout != NULL
		&& !fState->IsValid(B_VIEW_WHICH_VIEW_COLOR_BIT)) {
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		SetHighUIColor(B_PANEL_TEXT_COLOR);
	}

	rgb_color color;

	const char* colorName = ui_color_name(fState->which_view_color);
	if (colorName != NULL && message->FindColor(colorName, &color) == B_OK) {
		fState->view_color = tint_color(color, fState->which_view_color_tint);
		fState->valid_flags |= B_VIEW_VIEW_COLOR_BIT;
	}

	colorName = ui_color_name(fState->which_low_color);
	if (colorName != NULL && message->FindColor(colorName, &color) == B_OK) {
		fState->low_color = tint_color(color, fState->which_low_color_tint);
		fState->valid_flags |= B_VIEW_LOW_COLOR_BIT;
	}

	colorName = ui_color_name(fState->which_high_color);
	if (colorName != NULL && message->FindColor(colorName, &color) == B_OK) {
		fState->high_color = tint_color(color, fState->which_high_color_tint);
		fState->valid_flags |= B_VIEW_HIGH_COLOR_BIT;
	}

	MessageReceived(message);

	for (KView* child = fFirstChild; child != NULL;
			child = child->fNextSibling)
		child->_ColorsUpdated(message);

	Invalidate();
}


void
KView::_Detach()
{
	DetachedFromWindow();
	fAttached = false;

	for (KView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
		child->_Detach();
	}

	AllDetached();

	if (fOwner) {
		_CheckLock();

		if (!fOwner->IsHidden())
			Invalidate();

		// make sure our owner doesn't need us anymore

		if (fOwner->CurrentFocus() == this) {
			MakeFocus(false);
			// MakeFocus() is virtual and might not be
			// passing through to the KView version,
			// but we need to make sure at this point
			// that we are not the focus view anymore.
			if (fOwner->CurrentFocus() == this)
				fOwner->_SetFocus(NULL, true);
		}

		if (fOwner->fDefaultButton == this)
			fOwner->SetDefaultButton(NULL);

		if (fOwner->fKeyMenuBar == this)
			fOwner->fKeyMenuBar = NULL;

		if (fOwner->fLastMouseMovedView == this)
			fOwner->fLastMouseMovedView = NULL;

		if (fOwner->fLastViewToken == _get_object_token_(this))
			fOwner->fLastViewToken = B_NULL_TOKEN;

		_SetOwner(NULL);
	}
}


void
KView::_Draw(BRect updateRect)
{
	if (IsHidden(this) || !(Flags() & B_WILL_DRAW))
		return;

	// NOTE: if ViewColor() == B_TRANSPARENT_COLOR and no B_WILL_DRAW
	// -> View is simply not drawn at all

	_SwitchServerCurrentView();

	ConvertFromScreen(&updateRect);

	// TODO: make states robust (the hook implementation could
	// mess things up if it uses non-matching Push- and PopState(),
	// we would not be guaranteed to still have the same state on
	// the stack after having called Draw())
	PushState();
	Draw(updateRect);
	PopState();
	Flush();
}


void
KView::_DrawAfterChildren(BRect updateRect)
{
	if (IsHidden(this) || !(Flags() & B_WILL_DRAW)
		|| !(Flags() & B_DRAW_ON_CHILDREN))
		return;

	_SwitchServerCurrentView();

	ConvertFromScreen(&updateRect);

	// TODO: make states robust (see above)
	PushState();
	DrawAfterChildren(updateRect);
	PopState();
	Flush();
}


void
KView::_FontsUpdated(BMessage* message)
{
	MessageReceived(message);

	for (KView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
		child->_FontsUpdated(message);
	}
}


void
KView::_Pulse()
{
	if ((Flags() & B_PULSE_NEEDED) != 0)
		Pulse();

	for (KView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
		child->_Pulse();
	}
}


void
KView::_UpdateStateForRemove()
{
	// TODO: _CheckLockAndSwitchCurrent() would be good enough, no?
	if (!_CheckOwnerLockAndSwitchCurrent())
		return;

	fState->UpdateFrom(*fOwner->fLink);
//	if (!fState->IsValid(B_VIEW_FRAME_BIT)) {
//		fOwner->fLink->StartMessage(AS_VIEW_GET_COORD);
//
//		status_t code;
//		if (fOwner->fLink->FlushWithReply(code) == B_OK
//			&& code == B_OK) {
//			fOwner->fLink->Read<BPoint>(&fParentOffset);
//			fOwner->fLink->Read<BRect>(&fBounds);
//			fState->valid_flags |= B_VIEW_FRAME_BIT;
//		}
//	}

	// update children as well

	for (KView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
		if (child->fOwner)
			child->_UpdateStateForRemove();
	}
}


inline void
KView::_UpdatePattern(::pattern pattern)
{
	if (fState->IsValid(B_VIEW_PATTERN_BIT) && pattern == fState->pattern)
		return;

	if (fOwner) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_PATTERN);
		fOwner->fLink->Attach< ::pattern>(pattern);

		fState->valid_flags |= B_VIEW_PATTERN_BIT;
	}

	fState->pattern = pattern;
}


void
KView::_FlushIfNotInTransaction()
{
	if (!fOwner->fInTransaction) {
		fOwner->Flush();
	}
}


KShelf*
KView::_Shelf() const
{
	return fShelf;
}


void
KView::_SetShelf(KShelf* shelf)
{
	if (fShelf != NULL && fOwner != NULL)
		fOwner->RemoveHandler(fShelf);

	fShelf = shelf;

	if (fShelf != NULL && fOwner != NULL)
		fOwner->AddHandler(fShelf);
}


status_t
KView::_SetViewBitmap(const KBitmap* bitmap, BRect srcRect, BRect dstRect,
	uint32 followFlags, uint32 options)
{
	if (!_CheckOwnerLockAndSwitchCurrent())
		return B_ERROR;

	int32 serverToken = bitmap ? bitmap->_ServerToken() : -1;

	fOwner->fLink->StartMessage(AS_VIEW_SET_VIEW_BITMAP);
	fOwner->fLink->Attach<int32>(serverToken);
	fOwner->fLink->Attach<BRect>(srcRect);
	fOwner->fLink->Attach<BRect>(dstRect);
	fOwner->fLink->Attach<int32>(followFlags);
	fOwner->fLink->Attach<int32>(options);

	status_t status = B_ERROR;
	fOwner->fLink->FlushWithReply(status);

	return status;
}


bool
KView::_CheckOwnerLockAndSwitchCurrent() const
{
debug_printf("[KView]{_CheckOwnerLockAndSwitchCurrent}\n");

	STRACE(("KView(%s)::_CheckOwnerLockAndSwitchCurrent()\n", Name()));

	if (fOwner == NULL) {
		debugger("View method requires owner and doesn't have one.");
		return false;
	}

	_CheckLockAndSwitchCurrent();

debug_printf("[KView]{_CheckOwnerLockAndSwitchCurrent}ends\n");
	return true;
}


bool
KView::_CheckOwnerLock() const
{
	if (fOwner) {
		fOwner->check_lock();
		return true;
	} else {
		debugger("View method requires owner and doesn't have one.");
		return false;
	}
}


void
KView::_CheckLockAndSwitchCurrent() const
{
debug_printf("[KView]{_CheckLockAndSwitchCurrent}\n");

	STRACE(("KView(%s)::_CheckLockAndSwitchCurrent()\n", Name()));

	if (!fOwner)
		return;

	fOwner->check_lock();

	_SwitchServerCurrentView();

debug_printf("[KView]{_CheckLockAndSwitchCurrent}ends\n");
}


void
KView::_CheckLock() const
{
	if (fOwner)
		fOwner->check_lock();
}


void
KView::_SwitchServerCurrentView() const
{
debug_printf("[KView]{_SwitchServerCurrentView}\n");

	int32 serverToken = _get_object_token_(this);

	if (fOwner->fLastViewToken != serverToken) {
	debug_printf("[KView]{_SwitchServerCurrentView}fOwner->fLastViewToken != serverToken sending AS_SET_CURRENT_VIEW_2\n");
		STRACE(("contacting app_server... sending token: %" B_PRId32 "\n",
			serverToken));
		fOwner->fLink->StartMessage(AS_SET_CURRENT_VIEW_2);
		fOwner->fLink->Attach<int32>(serverToken);

		fOwner->fLastViewToken = serverToken;
	}

debug_printf("[KView]{_SwitchServerCurrentView}ends\n");
}


status_t
KView::ScrollWithMouseWheelDelta(KScrollBar* scrollBar, float delta)
{
	if (scrollBar == NULL || delta == 0.0f)
		return B_BAD_VALUE;

	float smallStep;
	float largeStep;
	scrollBar->GetSteps(&smallStep, &largeStep);

	// pressing the shift key scrolls faster (following the pseudo-standard set
	// by other desktop environments).
	if ((modifiers() & B_SHIFT_KEY) != 0)
		delta *= largeStep;
	else
		delta *= smallStep * 3;

	scrollBar->SetValue(scrollBar->Value() + delta);

	return B_OK;
}


#if __GNUC__ == 2


extern "C" void
_ReservedView1__5KView(KView* view, BRect rect)
{
	view->KView::DrawAfterChildren(rect);
}


extern "C" void
_ReservedView2__5KView(KView* view)
{
	// MinSize()
	perform_data_min_size data;
	view->Perform(PERFORM_CODE_MIN_SIZE, &data);
}


extern "C" void
_ReservedView3__5KView(KView* view)
{
	// MaxSize()
	perform_data_max_size data;
	view->Perform(PERFORM_CODE_MAX_SIZE, &data);
}


extern "C" BSize
_ReservedView4__5KView(KView* view)
{
	// PreferredSize()
	perform_data_preferred_size data;
	view->Perform(PERFORM_CODE_PREFERRED_SIZE, &data);
	return data.return_value;
}


extern "C" BAlignment
_ReservedView5__5KView(KView* view)
{
	// LayoutAlignment()
	perform_data_layout_alignment data;
	view->Perform(PERFORM_CODE_LAYOUT_ALIGNMENT, &data);
	return data.return_value;
}


extern "C" bool
_ReservedView6__5KView(KView* view)
{
	// HasHeightForWidth()
	perform_data_has_height_for_width data;
	view->Perform(PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH, &data);
	return data.return_value;
}


extern "C" void
_ReservedView7__5KView(KView* view, float width, float* min, float* max,
	float* preferred)
{
	// GetHeightForWidth()
	perform_data_get_height_for_width data;
	data.width = width;
	view->Perform(PERFORM_CODE_GET_HEIGHT_FOR_WIDTH, &data);
	if (min != NULL)
		*min = data.min;
	if (max != NULL)
		*max = data.max;
	if (preferred != NULL)
		*preferred = data.preferred;
}


extern "C" void
_ReservedView8__5KView(KView* view, KLayout* layout)
{
	// SetLayout()
	k_perform_data_set_layout data;
	data.layout = layout;
	view->Perform(PERFORM_CODE_SET_LAYOUT, &data);
}


extern "C" void
_ReservedView9__5KView(KView* view, bool descendants)
{
	// LayoutInvalidated()
	perform_data_layout_invalidated data;
	data.descendants = descendants;
	view->Perform(PERFORM_CODE_LAYOUT_INVALIDATED, &data);
}


extern "C" void
_ReservedView10__5KView(KView* view)
{
	// DoLayout()
	view->Perform(PERFORM_CODE_DO_LAYOUT, NULL);
}


#endif	// __GNUC__ == 2


extern "C" bool
B_IF_GCC_2(_ReservedView11__5KView, _ZN5KView15_ReservedView11Ev)(
	KView* view, BPoint point, KToolTip** _toolTip)
{
	// GetToolTipAt()
	k_perform_data_get_tool_tip_at data;
	data.point = point;
	data.tool_tip = _toolTip;
	view->Perform(PERFORM_CODE_GET_TOOL_TIP_AT, &data);
	return data.return_value;
}


extern "C" void
B_IF_GCC_2(_ReservedView12__5KView, _ZN5KView15_ReservedView12Ev)(
	KView* view)
{
	// LayoutChanged();
	view->Perform(PERFORM_CODE_LAYOUT_CHANGED, NULL);
}


void KView::_ReservedView13() {}
void KView::_ReservedView14() {}
void KView::_ReservedView15() {}
void KView::_ReservedView16() {}


KView::KView(const KView& other)
	:
	BHandler()
{
	// this is private and not functional, but exported
}


KView&
KView::operator=(const KView& other)
{
	// this is private and not functional, but exported
	return *this;
}


void
KView::_PrintToStream()
{
	printf("KView::_PrintToStream()\n");
	printf("\tName: %s\n"
		"\tParent: %s\n"
		"\tFirstChild: %s\n"
		"\tNextSibling: %s\n"
		"\tPrevSibling: %s\n"
		"\tOwner(Window): %s\n"
		"\tToken: %" B_PRId32 "\n"
		"\tFlags: %" B_PRId32 "\n"
		"\tView origin: (%f,%f)\n"
		"\tView Bounds rectangle: (%f,%f,%f,%f)\n"
		"\tShow level: %d\n"
		"\tTopView?: %s\n"
		"\tBPicture: %s\n"
		"\tVertical Scrollbar %s\n"
		"\tHorizontal Scrollbar %s\n"
		"\tIs Printing?: %s\n"
		"\tShelf?: %s\n"
		"\tEventMask: %" B_PRId32 "\n"
		"\tEventOptions: %" B_PRId32 "\n",
	Name(),
	fParent ? fParent->Name() : "NULL",
	fFirstChild ? fFirstChild->Name() : "NULL",
	fNextSibling ? fNextSibling->Name() : "NULL",
	fPreviousSibling ? fPreviousSibling->Name() : "NULL",
	fOwner ? fOwner->Name() : "NULL",
	_get_object_token_(this),
	fFlags,
	fParentOffset.x, fParentOffset.y,
	fBounds.left, fBounds.top, fBounds.right, fBounds.bottom,
	fShowLevel,
	fTopLevelView ? "YES" : "NO",
	fCurrentPicture? "YES" : "NULL",
	fVerScroller? "YES" : "NULL",
	fHorScroller? "YES" : "NULL",
	fIsPrinting? "YES" : "NO",
	fShelf? "YES" : "NO",
	fEventMask,
	fEventOptions);

	printf("\tState status:\n"
		"\t\tLocalCoordianteSystem: (%f,%f)\n"
		"\t\tPenLocation: (%f,%f)\n"
		"\t\tPenSize: %f\n"
		"\t\tHighColor: [%d,%d,%d,%d]\n"
		"\t\tLowColor: [%d,%d,%d,%d]\n"
		"\t\tViewColor: [%d,%d,%d,%d]\n"
		"\t\tPattern: %" B_PRIx64 "\n"
		"\t\tDrawingMode: %d\n"
		"\t\tLineJoinMode: %d\n"
		"\t\tLineCapMode: %d\n"
		"\t\tMiterLimit: %f\n"
		"\t\tAlphaSource: %d\n"
		"\t\tAlphaFuntion: %d\n"
		"\t\tScale: %f\n"
		"\t\t(Print)FontAliasing: %s\n"
		"\t\tFont Info:\n",
	fState->origin.x, fState->origin.y,
	fState->pen_location.x, fState->pen_location.y,
	fState->pen_size,
	fState->high_color.red, fState->high_color.blue, fState->high_color.green, fState->high_color.alpha,
	fState->low_color.red, fState->low_color.blue, fState->low_color.green, fState->low_color.alpha,
	fState->view_color.red, fState->view_color.blue, fState->view_color.green, fState->view_color.alpha,
	*((uint64*)&(fState->pattern)),
	fState->drawing_mode,
	fState->line_join,
	fState->line_cap,
	fState->miter_limit,
	fState->alpha_source_mode,
	fState->alpha_function_mode,
	fState->scale,
	fState->font_aliasing? "YES" : "NO");

	fState->font.PrintToStream();

	// TODO: also print the line array.
}


void
KView::_PrintTree()
{
	int32 spaces = 2;
	KView* c = fFirstChild; //c = short for: current
	printf( "'%s'\n", Name() );
	if (c != NULL) {
		while(true) {
			// action block
			{
				for (int i = 0; i < spaces; i++)
					printf(" ");

				printf( "'%s'\n", c->Name() );
			}

			// go deep
			if (c->fFirstChild) {
				c = c->fFirstChild;
				spaces += 2;
			} else {
				// go right
				if (c->fNextSibling) {
					c = c->fNextSibling;
				} else {
					// go up
					while (!c->fParent->fNextSibling && c->fParent != this) {
						c = c->fParent;
						spaces -= 2;
					}

					// that enough! We've reached this view.
					if (c->fParent == this)
						break;

					c = c->fParent->fNextSibling;
					spaces -= 2;
				}
			}
		}
	}
}


// #pragma mark -


KLayoutItem*
KView::KPrivate::LayoutItemAt(int32 index)
{
	return fView->fLayoutData->fLayoutItems.ItemAt(index);
}


int32
KView::KPrivate::CountLayoutItems()
{
	return fView->fLayoutData->fLayoutItems.CountItems();
}


void
KView::KPrivate::RegisterLayoutItem(KLayoutItem* item)
{
	fView->fLayoutData->fLayoutItems.AddItem(item);
}


void
KView::KPrivate::DeregisterLayoutItem(KLayoutItem* item)
{
	fView->fLayoutData->fLayoutItems.RemoveItem(item);
}


bool
KView::KPrivate::MinMaxValid()
{
	return fView->fLayoutData->fMinMaxValid;
}


bool
KView::KPrivate::WillLayout()
{
	KView::LayoutData* data = fView->fLayoutData;
	if (data->fLayoutInProgress)
		return false;
	if (data->fNeedsRelayout || !data->fLayoutValid || !data->fMinMaxValid)
		return true;
	return false;
}
