/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	K__GROUP_LAYOUT_BUILDER_H
#define	K__GROUP_LAYOUT_BUILDER_H

#include <KGroupLayout.h>
#include <KGroupView.h>
#include <List.h>

class KGroupLayoutBuilder {
public:
								KGroupLayoutBuilder(
									orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
								KGroupLayoutBuilder(KGroupLayout* layout);
								KGroupLayoutBuilder(KGroupView* view);

			KGroupLayout*		RootLayout() const;
			KGroupLayout*		TopLayout() const;
			KGroupLayoutBuilder& GetTopLayout(KGroupLayout** _layout);
			KView*				TopView() const;
			KGroupLayoutBuilder& GetTopView(KView** _view);

			KGroupLayoutBuilder& Add(KView* view);
			KGroupLayoutBuilder& Add(KView* view, float weight);
			KGroupLayoutBuilder& Add(KLayoutItem* item);
			KGroupLayoutBuilder& Add(KLayoutItem* item, float weight);

			KGroupLayoutBuilder& AddGroup(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
			KGroupLayoutBuilder& End();

			KGroupLayoutBuilder& AddGlue(float weight = 1.0f);
			KGroupLayoutBuilder& AddStrut(float size);

			KGroupLayoutBuilder& SetInsets(float left, float top, float right,
									float bottom);

								operator KGroupLayout*();

private:
			bool				_PushLayout(KGroupLayout* layout);
			void				_PopLayout();

private:
			KGroupLayout*		fRootLayout;
			BList				fLayoutStack;
};

#endif	// _GROUP_LAYOUT_BUILDER_H
