
#ifndef _MBORDER_H
#define _MBORDER_H
#include "MGroup.h"

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MBorder :  public MGroup, public BView
{
	public:
			enum {
				ROTATE_REVERSE=(int)0x80000000
			};
	
			MBorder(ulong border_type,ulong spacing,char *name=NULL,MView *kid=NULL);
			MBorder(BMessage*);
			virtual long Archive(BMessage *archive, bool deep=true) const;
			static BArchivable *Instantiate(BMessage *archive);
			~MBorder();
			virtual	minimax	layoutprefs();
			virtual BRect	layout(BRect rect);
			virtual void	Draw(BRect);
			virtual void AttachedToWindow();
			virtual void DetachedFromWindow();
			virtual void FrameResized(float width, float height);
			void			DrawBorder();
			void			SetLabel(char *);
			char           *Label();
			void			SetHighlight(int);
			void			SetHighlightColors(rgb_color color1, rgb_color color2, rgb_color color3);

	private:
			virtual void	_expansionmborder1();
			virtual void	_expansionmborder2();

			static long		_cycler(void *arg);
			void			Cycler();
			int	highlightmode;
			thread_id cycler;

			ulong bordertype;
			ulong extraspacing;
			ulong extralabelspacing;
			char  *label;
			char  *truncatedlabel;
			
			rgb_color *highlightcolors;

			uint32 _expansiondata[1];
};


enum
{
	M_NO_BORDER,
	M_RAISED_BORDER,
	M_DEPRESSED_BORDER,
	M_LABELED_BORDER,
	M_ETCHED_BORDER
};

enum
{
	M_SHOW_FULL_LABEL=	0x00000100
};
#endif
