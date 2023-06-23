/*
 * Copyright 2001-2018 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Dörfler
 *		Jeremy Rand, jrand@magma.ca
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _DESKBAR_PRIVATE_H
#define _DESKBAR_PRIVATE_H


#ifndef kDeskbarSignature
#	define kDeskbarSignature "application/x-vnd.Be-TSKB"
#endif


static const uint32 kMsgIsAlwaysOnTop = 'gtop';
static const uint32 kMsgAlwaysOnTop = 'stop';
static const uint32 kMsgIsAutoRaise = 'grse';
static const uint32 kMsgAutoRaise = 'srse';
static const uint32 kMsgIsAutoHide = 'ghid';
static const uint32 kMsgAutoHide = 'shid';

static const uint32 kMsgAddView = 'icon';
static const uint32 kMsgAddAddOn = 'adon';
static const uint32 kMsgHasItem = 'exst';
static const uint32 kMsgGetItemInfo = 'info';
static const uint32 kMsgCountItems = 'cwnt';
static const uint32 kMsgMaxItemSize = 'mxsz';
static const uint32 kMsgRemoveItem = 'remv';
static const uint32 kMsgLocation = 'gloc';
static const uint32 kMsgIsExpanded = 'gexp';
static const uint32 kMsgSetLocation = 'sloc';
static const uint32 kMsgExpand = 'sexp';


//khidki start

#ifndef k_kDeskbarSignature
#	define k_kDeskbarSignature "application/x-vnd.Be-KRYB"
#endif


static const uint32 k_kMsgIsAlwaysOnTop = 'gtop';
static const uint32 k_kMsgAlwaysOnTop = 'stop';
static const uint32 k_kMsgIsAutoRaise = 'grse';
static const uint32 k_kMsgAutoRaise = 'srse';
static const uint32 k_kMsgIsAutoHide = 'ghid';
static const uint32 k_kMsgAutoHide = 'shid';

static const uint32 k_kMsgAddView = 'icon';
static const uint32 k_kMsgAddAddOn = 'adon';
static const uint32 k_kMsgHasItem = 'exst';
static const uint32 k_kMsgGetItemInfo = 'info';
static const uint32 k_kMsgCountItems = 'cwnt';
static const uint32 k_kMsgMaxItemSize = 'mxsz';
static const uint32 k_kMsgRemoveItem = 'remv';
static const uint32 k_kMsgLocation = 'gloc';
static const uint32 k_kMsgIsExpanded = 'gexp';
static const uint32 k_kMsgSetLocation = 'sloc';
static const uint32 k_kMsgExpand = 'sexp';
//end

#endif	/* _DESKBAR_PRIVATE_H */
