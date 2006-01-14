
// Abstract base class for an object that can be 'divided'.
// Currently this includes MTextControl and MPopup.
// The dividable class, and the global function below, is
// used to align the left half (the label) and the right
// half (data-entry/selection) of a group of MDividable's

#ifndef _MDIVIDABLE_H
#define _MDIVIDABLE_H

#include "layout.h"

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MDividable
{
	public:
		float		labelwidth;
		MDividable *rolemodel;

	public:
		MDividable();
// work around my own bug...
#ifdef BUILDING_LIBLAYOUT
		~MDividable();
#endif
		void DivideSameAs(MDividable *);
		virtual float LabelWidth();
};

extern void IMPEXPLIBLAYOUT DivideSame(MView *, MView *, ...);

#endif
