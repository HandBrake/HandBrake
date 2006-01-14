/*

	MGroup is an abstract class from which all groups should derive.

*/

#ifndef _MGROUP_H
#define _MGROUP_H

#include "layout.h"
#include <View.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MGroup : public MView
{
	public:		MGroup();
				virtual ~MGroup();

				virtual void	reloadfont(BFont *font[]);
};

typedef struct
{
	MView	*kid;
	int		kidnum;
}  sortstruct;

#endif
