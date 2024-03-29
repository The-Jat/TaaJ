/*
 * Copyright 2008-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Pieter Panman
 */
#ifndef DEVICESVIEW_H
#define DEVICESVIEW_H


#include <MenuField.h>
#include <MenuItem.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>
#include <TabView.h>
#include <View.h>

#include <map>

#include "Device.h"
#include "DeviceACPI.h"
#include "DevicePCI.h"
#include "DeviceSCSI.h"
#include "DeviceUSB.h"
#include "PropertyList.h"

static const uint32 kMsgRefresh				= 'refr';
static const uint32 kMsgReportCompatibility	= 'repo';
static const uint32 kMsgGenerateSysInfo		= 'sysi';
static const uint32 kMsgSelectionChanged	= 'selc';
static const uint32 kMsgOrderCategory		= 'ocat';
static const uint32 kMsgOrderConnection		= 'ocon';

typedef enum {
	ORDER_BY_CONNECTION,
	ORDER_BY_CATEGORY
} OrderByType;

typedef std::map<Category, Device*> CategoryMap;
typedef std::map<Category, Device*>::const_iterator CategoryMapIterator;

typedef std::vector<Device*> Devices;


class DevicesView : public BView {
	public:
				DevicesView();
				~DevicesView();

		virtual void CreateLayout();

		virtual void MessageReceived(BMessage* msg);
		virtual void RescanDevices();
		virtual void CreateCategoryMap();
		virtual void DeleteCategoryMap();

		virtual void DeleteDevices();
		virtual void RebuildDevicesOutline();
		virtual void AddChildrenToOutlineByConnection(Device* parent);
		virtual void AddDeviceAndChildren(device_node_cookie* node, Device* parent);
		static int   SortItemsCompare(const BListItem*, const BListItem*);

	private:
		BOutlineListView*	fDevicesOutline;
		PropertyList*		fAttributesView;
		BMenuField*			fOrderByMenu;
		Devices				fDevices;
		OrderByType			fOrderBy;
		CategoryMap			fCategoryMap;

};

#endif /* DEVICESVIEW_H */
