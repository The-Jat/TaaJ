/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	K__GRID_VIEW_H
#define	K__GRID_VIEW_H

#include <KGridLayout.h>
#include <Laminate.h>


class KGridView : public KView {
public:
								KGridView(float horizontal
										= B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
								KGridView(const char* name,
									float horizontal = B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
								KGridView(BMessage* from);
	virtual						~KGridView();

	virtual	void				SetLayout(KLayout* layout);
			KGridLayout*		GridLayout() const;

	static	BArchivable*		Instantiate(BMessage* from);

	virtual	status_t			Perform(perform_code d, void* arg);

private:

	// FBC padding
	virtual	void				_ReservedGridView1();
	virtual	void				_ReservedGridView2();
	virtual	void				_ReservedGridView3();
	virtual	void				_ReservedGridView4();
	virtual	void				_ReservedGridView5();
	virtual	void				_ReservedGridView6();
	virtual	void				_ReservedGridView7();
	virtual	void				_ReservedGridView8();
	virtual	void				_ReservedGridView9();
	virtual	void				_ReservedGridView10();

	// forbidden methods
								KGridView(const KGridView&);
			void				operator =(const KGridView&);

			uint32				_reserved[2];
};


#endif	// _GRID_VIEW_H
