/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	K__GROUP_VIEW_H
#define	K__GROUP_VIEW_H


#include <KGroupLayout.h>
#include <Laminate.h>


class KGroupView : public KView {
public:
								KGroupView(
									orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
								KGroupView(const char* name,
									orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
								KGroupView(BMessage* from);
	virtual						~KGroupView();

	virtual	void				SetLayout(KLayout* layout);
			KGroupLayout*		GroupLayout() const;

	static	BArchivable*		Instantiate(BMessage* from);

	virtual	status_t			Perform(perform_code d, void* arg);

private:

	// FBC padding
	virtual	void				_ReservedGroupView1();
	virtual	void				_ReservedGroupView2();
	virtual	void				_ReservedGroupView3();
	virtual	void				_ReservedGroupView4();
	virtual	void				_ReservedGroupView5();
	virtual	void				_ReservedGroupView6();
	virtual	void				_ReservedGroupView7();
	virtual	void				_ReservedGroupView8();
	virtual	void				_ReservedGroupView9();
	virtual	void				_ReservedGroupView10();

	// forbidden methods
								KGroupView(const KGroupView&);
			void				operator =(const KGroupView&);

			uint32				_reserved[2];
};


#endif	// _GROUP_VIEW_H
