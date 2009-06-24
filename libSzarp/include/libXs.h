/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka xslib.d
 * libXs.h
 */

#ifdef XmVERSION /* Musi byc dolaczony Motif aby dolaczyc ten plik */
#ifndef H__LIBXS_H
#define H__LIBXS_H

/*--------------------------------------------------------------------------*/
/*  libXs.h : Header file for X-sample library                              */
/*  Version 2.0, revised 05.95                                              */
/*--------------------------------------------------------------------------*/

typedef struct _menu_struct
{
    char    *name;               /* name of the button      */
    void (* func)(Widget, XtPointer, XtPointer);   /* Callback to be invoked  */
    caddr_t data;                /* Data for the callback   */
    struct _menu_struct *sub_menu;           /* Data for submenu        */
    int     n_sub_items;         /* items in sub_menu       */
    char    *sub_menu_title;     /* Title of submenu        */
}   xs_menu_struct;

extern XmString  xs_str_array_to_xmstr(char * cs[], int n);	/* xs_str.c    */
extern XmString  xs_concat_words(int n, char * words[]);        /* xs_str.c    */
extern char     *xs_get_string_from_xmstring(XmString string);	/* xs_str.c    */
extern void      xs_wprintf (Widget wd, ...);			/* xs_str.c    */

extern Widget    xs_create_quit_button(Widget parent);		/* xs_quit.c   */

extern void      xs_create_menu_buttons(char *title, Widget menu,
			xs_menu_struct *menulist, int nitems);      /* xs_menu.c   */
extern void      xs_help_callback(Widget w, char *str[], caddr_t call_data);    /* xs_help.c   */

extern Widget    xs_create_pixmap_browser(Widget parent, char *tiles[],
			int n_tiles, void (*callback)(), caddr_t data);	    /* xs_pixmap.c */
extern Widget    xs_create_pixmap_button(Widget parent, char *pattern);	    /* xs_pixmap.c */


#define XS_SA(parname,parvalue) {XtSetArg (wargs[n],(parname),(parvalue));n++;}

#endif                      /*   do not add anything after this endif !!!    */
#endif

