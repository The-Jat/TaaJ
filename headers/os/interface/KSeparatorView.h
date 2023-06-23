/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_SEPARATOR_VIEW_H
#define _K_SEPARATOR_VIEW_H


#include <Alignment.h>
#include <String.h>
#include <Laminate.h>


class KSeparatorView : public KView {
public:
								KSeparatorView(orientation orientation,
									border_style border = B_PLAIN_BORDER);
								KSeparatorView(const char* name,
									const char* label,
									orientation orientation = B_HORIZONTAL,
									border_style border = B_FANCY_BORDER,
									const BAlignment& alignment
										= BAlignment(B_ALIGN_HORIZONTAL_CENTER,
											B_ALIGN_VERTICAL_CENTER));
								KSeparatorView(const char* name,
									KView* labelView,
									orientation orientation = B_HORIZONTAL,
									border_style border = B_FANCY_BORDER,
									const BAlignment& alignment
										= BAlignment(B_ALIGN_HORIZONTAL_CENTER,
											B_ALIGN_VERTICAL_CENTER));
								KSeparatorView(const char* label = NULL,
									orientation orientation = B_HORIZONTAL,
									border_style border = B_FANCY_BORDER,
									const BAlignment& alignment
										= BAlignment(B_ALIGN_HORIZONTAL_CENTER,
											B_ALIGN_VERTICAL_CENTER));
								KSeparatorView(KView* labelView,
									orientation orientation = B_HORIZONTAL,
									border_style border = B_FANCY_BORDER,
									const BAlignment& alignment
										= BAlignment(B_ALIGN_HORIZONTAL_CENTER,
											B_ALIGN_VERTICAL_CENTER));

								KSeparatorView(BMessage* archive);

	virtual						~KSeparatorView();

	static 	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;

	virtual	void				Draw(BRect updateRect);

	virtual	void				GetPreferredSize(float* width, float* height);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

			void				SetOrientation(orientation orientation);
			void				SetAlignment(const BAlignment& aligment);
			void				SetBorderStyle(border_style border);

			void				SetLabel(const char* label);
			void				SetLabel(KView* view, bool deletePrevious);

	virtual status_t			Perform(perform_code code, void* data);

protected:
	virtual	void				DoLayout();

private:
	// FBC padding
	virtual	void				_ReservedSeparatorView1();
	virtual	void				_ReservedSeparatorView2();
	virtual	void				_ReservedSeparatorView3();
	virtual	void				_ReservedSeparatorView4();
	virtual	void				_ReservedSeparatorView5();
	virtual	void				_ReservedSeparatorView6();
	virtual	void				_ReservedSeparatorView7();
	virtual	void				_ReservedSeparatorView8();
	virtual	void				_ReservedSeparatorView9();
	virtual	void				_ReservedSeparatorView10();

private:
			void				_Init(const char* label, KView* labelView,
									orientation orientation,
									BAlignment alignment, border_style border);

			float				_BorderSize() const;
			BRect				_MaxLabelBounds() const;

private:
			BString				fLabel;
			KView*				fLabelView;

			orientation			fOrientation;
			BAlignment			fAlignment;
			border_style		fBorder;

			uint32				_reserved[10];
};

#endif // _SEPARATOR_VIEW_H
