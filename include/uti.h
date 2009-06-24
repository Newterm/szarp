/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka MyInc
 * uti.h
 */

#ifndef _UTI_H_
#define _UTI_H_

#ifdef __cplusplus
    extern "C" {
#endif


#define J_LEFT  0
#define J_RIGHT 1


Widget 
err_dpy (char *title, Widget parent, char *msg[],int nmsg)
{
  Widget   errdial;
  Widget   label;
  Arg      wargs[10]; 
  int      n;
  XmString xmstr;

  n=0;
  XS_SA (XmNtitle,title)
  XS_SA(XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL) 

  errdial=XmCreateErrorDialog (parent, "error", wargs,n);
  XtUnmanageChild (XmMessageBoxGetChild (errdial,
		      XmDIALOG_CANCEL_BUTTON));	
  XtUnmanageChild (XmMessageBoxGetChild (errdial,
		      XmDIALOG_HELP_BUTTON));	

  label=XmMessageBoxGetChild (errdial, XmDIALOG_MESSAGE_LABEL);
     
  n=0;
  XS_SA(XmNalignment, XmALIGNMENT_BEGINNING)
     
  XtSetValues (label, wargs, n);
   
  xmstr=xs_str_array_to_xmstr (msg, nmsg);

  n=0;
  XS_SA(XmNmessageString, xmstr)   
 
  XtSetValues (errdial, wargs, n);

  XmStringFree (xmstr);
  return (errdial);
} 



Widget PlCreateFileSelectionDialog(Widget parent, String name, ArgList arglist,
                                   Cardinal argcount)
 {
  Widget w;
/*  Widget ftext; */
  Arg wargs[20];
  int n;
  n=0;
  XS_SA(XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL) 
  XS_SA(XmNfileListLabelString, XmStringCreate("Pliki",
           XmSTRING_DEFAULT_CHARSET))
  XS_SA(XmNfilterLabelString, XmStringCreate("Filtr",
           XmSTRING_DEFAULT_CHARSET))
  XS_SA(XmNapplyLabelString, XmStringCreate("Filtruj",
           XmSTRING_DEFAULT_CHARSET))
  XS_SA(XmNcancelLabelString, XmStringCreate("Zaniechaj",
           XmSTRING_DEFAULT_CHARSET))
  XS_SA(XmNhelpLabelString, XmStringCreate("Pomoc",
           XmSTRING_DEFAULT_CHARSET))
  XS_SA(XmNselectionLabelString, XmStringCreate("Wybrano",
           XmSTRING_DEFAULT_CHARSET))
  XS_SA(XmNdirListLabelString, XmStringCreate("Katalogi",
           XmSTRING_DEFAULT_CHARSET))
/*  XS_SA(XmNwidth,200)
  XS_SA(XmNheight,150)
  XS_SA(XtNx,10)
  XS_SA(XtNy,10)  */
   
  w=XmCreateFileSelectionDialog(parent, name, arglist, argcount);
  XtSetValues(w, wargs, n);

/*  ftext=XmFileSelectionBoxGetChild (w,XmDIALOG_FILTER_TEXT);
  n=0;
  XS_SA(XmNheight,50)
  XtSetValues(ftext,wargs,n); */
  return(w);
 }


Widget
PlCreateQuestionDialog(Widget parent, String name, ArgList arglist,
	Cardinal argcount, char **msg, int msgcount)
{
    Arg      wargs[10];
    Widget   label;
    XmString xmstr1, xmstr2, xmstr3, xmstr;
    Widget   w;
    
    int n=0;
    XS_SA(XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    
    xmstr1 = XmStringCreate("Tak",XmSTRING_DEFAULT_CHARSET);
    XS_SA(XmNokLabelString,xmstr1);
    
    xmstr2 = XmStringCreate("Nie",XmSTRING_DEFAULT_CHARSET);
    XS_SA(XmNcancelLabelString,xmstr2);
    
    xmstr3=XmStringCreate("Pomoc",XmSTRING_DEFAULT_CHARSET);
    XS_SA(XmNhelpLabelString,xmstr3);
    
    w = XmCreateQuestionDialog(parent, "qdial",arglist,argcount);
    XtSetValues (w,wargs,n);
    
    XmStringFree (xmstr1);
    XmStringFree (xmstr2);
    XmStringFree (xmstr3); 
    
    /* XtAddEventHandler (w, ButtonPressMask, False, tdbShowHelpEH, "quitform");*/
    /* XtUnmanageChild (XmMessageBoxGetChild (w, XmDIALOG_HELP_BUTTON));	*/
    
    label = XmMessageBoxGetChild (w, XmDIALOG_MESSAGE_LABEL);
    
    n = 0;
    XS_SA(XmNalignment, XmALIGNMENT_BEGINNING);
    XtSetValues (label, wargs, n);
    
    xmstr = xs_str_array_to_xmstr(msg, msgcount);
    
    n = 0;
    XS_SA(XmNmessageString, xmstr);
    XtSetValues(w, wargs, n);
    XmStringFree(xmstr);
    
    /* XtAddEventHandler (XmMessageBoxGetChild (w,XmDIALOG_OK_BUTTON),
     ButtonPressMask, False,
     tdbShowHelpEH, "yes");
     XtAddEventHandler (XmMessageBoxGetChild (w,XmDIALOG_CANCEL_BUTTON),
                   ButtonPressMask, False,                                                         tdbShowHelpEH, "no");
 XtAddEventHandler (XmMessageBoxGetChild (w,XmDIALOG_HELP_BUTTON),
                   ButtonPressMask, False,                                                         tdbShowHelpEH, "help"); */
    
    return (w);
}


/*----------------------------------------------------------------------------*/
/* Funkcja strform (src,dst,size,just) formatuje string src w polu dst.       */
/* Pozosta³e parametry: size-d³ugo¶æ pola do sformatowania (bez 0!),          */
/*                      just-rodzaj wyrównywania (J_LEFT lub J_RIGHT)         */
/*----------------------------------------------------------------------------*/
  
void strform (char *src, char *dst, int size, short just)
  {
  short  len;                 /*  d³ugo¶æ stringu src           */
  short  diff;                /*  ilo¶æ spacji do wstawienia    */
  short  i;              


  if ( (len=(short)strlen(src))>=size)
     {
     strncpy (dst,src,size);
     *(dst+size)='\0';
     return;
     }  

 
  
  if (just==J_LEFT)          /* string dosuniêty do lewej strony   */
     {
     strcpy (dst,src);
     for (i=len; i<size; i++)
       *(dst+i)=' ';
     *(dst+size)='\0';
     return; 
     }

  if (just==J_RIGHT)         /* string dosuniêty do prawej strony  */
     { 
     diff=size-len;
     for (i=len; i<diff; i++)
       *(dst+i)=' ';
     strcpy (dst+diff,src);
     }

  return;
  }  

#ifdef __cplusplus
    }
#endif

#endif
