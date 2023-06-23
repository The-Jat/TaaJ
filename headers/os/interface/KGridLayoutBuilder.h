/*
 * Copyright 2006, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	K__GRID_LAYOUT_BUILDER_H
#define	K__GRID_LAYOUT_BUILDER_H

#include <KGridView.h>

class KGridLayoutBuilder {
public:
								KGridLayoutBuilder(float horizontal
										= B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
								KGridLayoutBuilder(KGridLayout* layout);
								KGridLayoutBuilder(KGridView* view);

			KGridLayout*		GridLayout() const;
			KGridLayoutBuilder& GetGridLayout(KGridLayout** _layout);
			KView*				View() const;
			KGridLayoutBuilder&	GetView(KView** _view);

			KGridLayoutBuilder& Add(KView* view, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
			KGridLayoutBuilder& Add(KLayoutItem* item, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);

			KGridLayoutBuilder& SetColumnWeight(int32 column, float weight);
			KGridLayoutBuilder& SetRowWeight(int32 row, float weight);

			KGridLayoutBuilder& SetInsets(float left, float top, float right,
									float bottom);

								operator KGridLayout*();

private:
			KGridLayout*		fLayout;
};

#endif	// _GRID_LAYOUT_BUILDER_H
