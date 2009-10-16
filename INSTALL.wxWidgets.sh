# Adam Smyk <asmyk@praterm.com.pl>
# Compilation of the wxWidgets library with wxGraphicContext feature
# $Id: INSTALL.wxWidgets.sh 135 2009.10.16Z asmyk $
#!/bin/sh

wget http://dfn.dl.sourceforge.net/project/wxwindows/wxAll/2.8.10/wxWidgets-2.8.10.tar.gz
tar xzvf wxWidgets-2.8.10.tar.gz
cd wxWidgets-2.8.10

./configure --host=i586-mingw32msvc \
        --target=i586-mingw32msvc \
        --enable-shared \
        --enable-intl \
        --enable-unicode \
	--enable-graphics_ctx \
        --with-msw \
        --with-libpng \
        --with-expat=sys \
        --with-zlib=builtin \
        --with-opengl \
        --prefix=/usr/local/i586-mingw32msvc \
        CPPFLAGS=-I/usr/local/i586-mingw32msvc/include \
        LDFLAGS="-L/usr/local/i586-mingw32msvc/lib -lgdiplus"

echo '--- src/msw/renderer.cpp	2009-03-06 13:01:15.000000000 +0100
+++ src/msw/renderer.cpp	2009-10-13 16:36:58.000000000 +0200
@@ -46,7 +46,7 @@
 #define min(a,b)            (((a) < (b)) ? (a) : (b))
 #endif
 
-#include "gdiplus.h"
+#include "Gdiplus.h"
 using namespace Gdiplus;
 #endif
 
' > renderer.patch
patch src/msw/renderer.cpp < renderer.patch

echo '--- src/msw/graphics.cpp	2008-03-06 13:01:14.000000000 +0100
+++ src/msw/graphics.cpp	2009-10-13 16:36:01.000000000 +0200
@@ -89,7 +89,7 @@
 #define min(a,b)            (((a) < (b)) ? (a) : (b))
 #endif
 
-#include "gdiplus.h"
+#include "Gdiplus.h"
 using namespace Gdiplus;
 
 class WXDLLIMPEXP_CORE wxGDIPlusPathData : public wxGraphicsPathData
@@ -521,7 +521,8 @@
 
             }
             m_penBrush = new HatchBrush(style,Color( pen.GetColour().Alpha() , pen.GetColour().Red() ,
-                pen.GetColour().Green() , pen.GetColour().Blue() ), Color::Transparent );
+//                pen.GetColour().Green() , pen.GetColour().Blue() ), Color::Transparent );
+                pen.GetColour().Green() , pen.GetColour().Blue() ), 0 );
             m_pen->SetBrush( m_penBrush );
         }
         break;
@@ -575,7 +576,8 @@
 
         }
         m_brush = new HatchBrush(style,Color( brush.GetColour().Alpha() , brush.GetColour().Red() ,
-            brush.GetColour().Green() , brush.GetColour().Blue() ), Color::Transparent );
+//            brush.GetColour().Green() , brush.GetColour().Blue() ), Color::Transparent );
+            brush.GetColour().Green() , brush.GetColour().Blue() ), 0 );
     }
     else
     {
@@ -647,12 +649,12 @@
         wxASSERT(interimMask.GetPixelFormat() == PixelFormat1bppIndexed);
 
         BitmapData dataMask ;
-        interimMask.LockBits(&bounds,ImageLockModeRead,
+        interimMask.LockBits(bounds,ImageLockModeRead,
             interimMask.GetPixelFormat(),&dataMask);
 
 
         BitmapData imageData ;
-        image->LockBits(&bounds,ImageLockModeWrite, PixelFormat32bppPARGB, &imageData);
+        image->LockBits(bounds,ImageLockModeWrite, PixelFormat32bppPARGB, &imageData);
 
         BYTE maskPattern = 0 ;
         BYTE maskByte = 0;
@@ -701,7 +703,7 @@
 
             m_helper = image ;
             image = NULL ;
-            m_helper->LockBits(&bounds, ImageLockModeRead,
+            m_helper->LockBits(bounds, ImageLockModeRead,
                 m_helper->GetPixelFormat(),&data);
 
             image = new Bitmap(data.Width, data.Height, data.Stride,
@@ -1227,7 +1229,7 @@
         Rect bounds(0,0,width,height);
         BitmapData data ;
 
-        interim.LockBits(&bounds, ImageLockModeRead,
+        interim.LockBits(bounds, ImageLockModeRead,
             interim.GetPixelFormat(),&data);
         
         bool hasAlpha = false;

' > graphics.patch
patch src/msw/graphics.cpp < graphics.patch
rm configure.patch graphics.patch renderer.patch

cd include
wget http://lublin.webd.pl/crayze/cpp-winapi/download/gdiplus.rar
unrar e gdiplus.rar
cp libgdiplus.a ../lib
mv GdiplusimageAttributes.h GdiplusImageAttributes.h
mv GdiplusMetaFile.h GdiplusMetafile.h

echo '--- GdiplusTypes.h	2009-10-13 22:24:11.000000000 +0200
+++ GdiplusTypes.h_new	2009-10-13 22:51:52.000000000 +0200
@@ -494,10 +494,10 @@
                           IN const RectF& a,
                           IN const RectF& b)
     {
-        REAL right = std::min(a.GetRight(), b.GetRight());
-        REAL bottom = std::min(a.GetBottom(), b.GetBottom());
-        REAL left = std::max(a.GetLeft(), b.GetLeft());
-        REAL top = std::max(a.GetTop(), b.GetTop());
+        REAL right = min(a.GetRight(), b.GetRight());
+        REAL bottom = min(a.GetBottom(), b.GetBottom());
+        REAL left = max(a.GetLeft(), b.GetLeft());
+        REAL top = max(a.GetTop(), b.GetTop());
 
         c.X = left;
         c.Y = top;
@@ -521,10 +521,10 @@
                       IN const RectF& a,
                       IN const RectF& b)
     {
-        REAL right = std::max(a.GetRight(), b.GetRight());
-        REAL bottom = std::max(a.GetBottom(), b.GetBottom());
-        REAL left = std::min(a.GetLeft(), b.GetLeft());
-        REAL top = std::min(a.GetTop(), b.GetTop());
+        REAL right = max(a.GetRight(), b.GetRight());
+        REAL bottom = max(a.GetBottom(), b.GetBottom());
+        REAL left = min(a.GetLeft(), b.GetLeft());
+        REAL top = min(a.GetTop(), b.GetTop());
 
         c.X = left;
         c.Y = top;
@@ -697,10 +697,10 @@
                           IN const Rect& a,
                           IN const Rect& b)
     {
-        INT right = std::min(a.GetRight(), b.GetRight());
-        INT bottom = std::min(a.GetBottom(), b.GetBottom());
-        INT left = std::max(a.GetLeft(), b.GetLeft());
-        INT top = std::max(a.GetTop(), b.GetTop());
+        INT right = min(a.GetRight(), b.GetRight());
+        INT bottom = min(a.GetBottom(), b.GetBottom());
+        INT left = max(a.GetLeft(), b.GetLeft());
+        INT top = max(a.GetTop(), b.GetTop());
 
         c.X = left;
         c.Y = top;
@@ -724,10 +724,10 @@
                       IN const Rect& a,
                       IN const Rect& b)
     {
-        INT right = std::max(a.GetRight(), b.GetRight());
-        INT bottom = std::max(a.GetBottom(), b.GetBottom());
-        INT left = std::min(a.GetLeft(), b.GetLeft());
-        INT top = std::min(a.GetTop(), b.GetTop());
+        INT right = max(a.GetRight(), b.GetRight());
+        INT bottom = max(a.GetBottom(), b.GetBottom());
+        INT left = min(a.GetLeft(), b.GetLeft());
+        INT top = min(a.GetTop(), b.GetTop());
 
         c.X = left;
         c.Y = top;
' > gdiplustypes.patch
patch GdiplusTypes.h < gdiplustypes.patch
 
echo '--- GdiplusMetafile.h	2009-10-13 22:24:11.000000000 +0200
+++ GdiplusMetafile.h	2009-10-13 22:49:30.000000000 +0200
@@ -346,7 +346,7 @@
         return metafileRasterizationLimitDpi;
     }
 
-    static UINT Metafile::EmfToWmfBits(
+    static UINT EmfToWmfBits(
         IN HENHMETAFILE       hemf,
         IN UINT               cbData16,
         IN LPBYTE             pData16,
' > gdiplusmetafile.patch
patch GdiplusMetafile.h < gdiplusmetafile.patch

echo '--- GdiplusStringFormat.h	2009-10-13 22:24:11.000000000 +0200
+++ GdiplusStringFormat.h	2009-10-13 22:50:35.000000000 +0200
@@ -239,7 +239,7 @@
         ));
     }
 
-    StringTrimming StringFormat::GetTrimming() const
+    StringTrimming GetTrimming() const
     {
         StringTrimming trimming;
         SetStatus(DllExports::GdipGetStringFormatTrimming(
' > gdiplusstringformat.patch
patch GdiplusStringFormat.h < gdiplusstringformat.patch
rm *.patch
cd ..
make
make install
