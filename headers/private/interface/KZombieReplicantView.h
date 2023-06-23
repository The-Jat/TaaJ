//------------------------------------------------------------------------------
//	Copyright (c) 2001-2010, Haiku
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ZombieReplicantView.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Class for Zombie replicants
//------------------------------------------------------------------------------

#ifndef K__ZOMBIE_REPLICANT_VIEW_H
#define K__ZOMBIE_REPLICANT_VIEW_H

#include <BeBuild.h>
#include <KBox.h>


const static rgb_color kZombieColor = {220, 220, 220, 255};


// _KZombieReplicantView_ class ------------------------------------------------
class _KZombieReplicantView_ : public KBox {

public:
								_KZombieReplicantView_(BRect frame, 
									status_t error);
	virtual						~_KZombieReplicantView_();

	virtual	void				MessageReceived(BMessage*msg);

	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint);

	virtual status_t			Archive(BMessage* archive,
									bool deep = true) const;

			void				SetArchive(BMessage*);

private:
			status_t			fError;
			BMessage*			fArchive;
};

#endif /* _ZOMBIE_REPLICANT_VIEW_H */
