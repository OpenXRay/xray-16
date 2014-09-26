#include "stdafx.h"
#pragma hdrstop

#include "MinimapEditor.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ExtBtn"
#pragma link "MXCtrls"
#pragma link "ElEdits"
#pragma link "ElSpin"
#pragma link "ElXPThemedControl"
#pragma link "ElACtrls"
#pragma resource "*.dfm"

TTMinimapEditor* TTMinimapEditor::form = NULL;
//.TTMinimapEditor *TMinimapEditor;
//---------------------------------------------------------------------------
__fastcall TTMinimapEditor::TTMinimapEditor(TComponent* Owner)
    : TForm(Owner)
{
}
#include "ui_main.h"

void __fastcall TTMinimapEditor::Show()
{
	if (!form){
        form 				= xr_new<TTMinimapEditor>((TComponent*)0);
//.        form->Caption 		= title;
    }

    form->ShowModal();
    UI->RedrawScene();
}

void __fastcall TTMinimapEditor::Hide()
{
	if (form){
    	form->Close();
    }
}
//---------------------------------------------------------------------------
#include "freeimage/freeimage.h"
#include "Image.h"

bool Surface_Load(LPCSTR full_name, U32Vec& data, u32& w, u32& h, u32& a);

void __fastcall TTMinimapEditor::btnLoadClick(TObject *Sender)
{
    xr_string                   fn;
    image_data.clear            ();
    
    if(EFS.GetOpenName("$app_root$", fn, false, NULL, 0))
    {
        if (Surface_Load(fn.c_str(),image_data,image_w,image_h,image_a))
        {

          u32* line         = xr_alloc<u32>(image_w);
          u32 sz_ln=sizeof(u32)*image_w;
          u32 y2 = image_w-1;
          for (int y=0; y<image_h/2; y++,y2--)
          {
              CopyMemory(line,image_data.begin()+y2*image_w,sz_ln);
              CopyMemory(image_data.begin()+y2*image_w,image_data.begin()+y*image_w,sz_ln);
              CopyMemory(image_data.begin()+y*image_w,line,sz_ln);
          }
            xr_free(line);

            LPCSTR _mark = "_[";
            if(fn.find(_mark) != fn.npos)
            {
                LPCSTR _str = fn.c_str()+fn.find(_mark);
                int cnt = sscanf(_str, "_[%f, %f]-[%f, %f]",&map_bb.min.x, &map_bb.min.y, &map_bb.max.x, &map_bb.max.y);
//                "ss_andy_05-08-07_15-24-11_#ai_test_[-150.000, -100.000]-[52.927, 50.000].tga"
                if(cnt!=4){
                   map_bb.min.x             = 0.0f;
                   map_bb.min.y             = 0.0f;
                   map_bb.max.x             = 100.0f;
                   map_bb.max.y             = 100.0f;
                   map_bb_loaded            = map_bb;

                   ApplyPoints              (true);
                  return;
                  }
                   map_bb_loaded            = map_bb;

                   ApplyPoints              (true);
                   imgPanelResize           (NULL);
            }
        }
    }
}
//---------------------------------------------------------------------------

void TTMinimapEditor::ApplyPoints(bool to_controls)
{
    static bool b_block     = false;
    if(b_block)             return;
    if(to_controls)
    {
        b_block                 = true;
       ElFloatSpinEditX1->Value = map_bb.min.x;
       ElFloatSpinEditZ1->Value = map_bb.min.y;

       ElFloatSpinEditX2->Value = map_bb.max.x;
       ElFloatSpinEditZ2->Value = map_bb.max.y;
        b_block                 = false;
   }else
   {
       map_bb.min.x             = ElFloatSpinEditX1->Value;
       map_bb.min.y             = ElFloatSpinEditZ1->Value;

       map_bb.max.x             = ElFloatSpinEditX2->Value;
       map_bb.max.y             = ElFloatSpinEditZ2->Value;
   }

   string512                    buff;
   sprintf(buff,"bound_rect = %3.3f, %3.3f, %3.3f, %3.3f",map_bb.min.x,map_bb.min.y,map_bb.max.x,map_bb.max.y);
   result_edit->ReadOnly        = false;
   result_edit->Text            = buff;
   result_edit->ReadOnly        = true;
   imgPanel->Repaint             ();
}
#include "FolderLib.h"

void DrawThumbnail(TCanvas* pCanvas, TRect& r, U32Vec& data, bool bDrawWithAlpha, int _w, int _h)
{
    pCanvas->CopyMode		= cmSrcCopy;
    Graphics::TBitmap *pBitmap = xr_new<Graphics::TBitmap>();

    pBitmap->PixelFormat 	= pf24bit; //pf32bit;
    pBitmap->Height		= _h;
    pBitmap->Width		= _w;
        
    if (bDrawWithAlpha)
    {
    	Fcolor back;
        back.set		(bgr2rgb(pCanvas->Brush->Color));  back.mul_rgb(255.f);
        for (int y = 0; y < pBitmap->Height; y++)
        {
            u32* ptr 		= (u32*)pBitmap->ScanLine[y];
            for (int x = 0; x < pBitmap->Width; x++){
                u32 src 	= data[(_h-1-y)*_w+x];
                float a		= float(color_get_A(src))/255.f;
                float inv_a	= 1.f-a;;
                u32 r		= iFloor(float(color_get_R(src))*a+back.r*inv_a);
                u32 g		= iFloor(float(color_get_G(src))*a+back.g*inv_a);
                u32 b		= iFloor(float(color_get_B(src))*a+back.b*inv_a);
                ptr[x] 		= color_rgba(r,g,b,0);
            }
        }
    }else{
        for (int y = 0; y < pBitmap->Height; y++)
        {
            u32* ptr 		= (u32*)pBitmap->ScanLine[y];
            Memory.mem_copy	(ptr,&data[(_h-1-y)*_w],_w*4);
        }
    }
    pCanvas->StretchDraw(r,pBitmap);
    xr_delete(pBitmap);
}

void __fastcall TTMinimapEditor::imgPanelPaint(TObject *Sender)
{
    if(image_data.size()==0) return;

    Irect       R;
    R.x1        = 0;
    R.y1        = 0;
    R.x2        = image_draw_size.x;
    R.y2        = image_draw_size.y;
//.    FHelper.DrawThumbnail(imgPanel->Canvas->Handle, R, &*image_data.begin(), image_w, image_h);

/*
    RECT       R1;
    R1.left        = 0;
    R1.top        = 0;
    R1.right        = image_draw_size.x;
    R1.bottom        = image_draw_size.y;
    DrawThumbnail(imgPanel->Canvas, R1, image_data, false, image_w, image_h);
*/
    float kw = (map_bb_loaded.max.x-map_bb_loaded.min.x) / (float)image_draw_size.x;
    float kh = (map_bb_loaded.max.y-map_bb_loaded.min.y) / (float)image_draw_size.y;

    TRect       R2;
    R2.Left      = iFloor((map_bb.min.x-map_bb_loaded.min.x)/kw);
    R2.Top       = iFloor((map_bb.min.y-map_bb_loaded.min.y)/kh);
    R2.Right     = R2.Left  + iFloor((map_bb.max.x-map_bb.min.x)/kw);
    R2.Bottom    = R2.Top   + iFloor((map_bb.max.y-map_bb.min.y)/kh);

    FHelper.DrawThumbnail(imgPanel->Canvas->Handle, R, &*image_data.begin(), image_w, image_h);

    imgPanel->Canvas->Pen->Color = clRed;
    imgPanel->Canvas->Pen->Style = psSolid;
    imgPanel->Canvas->Rectangle(R2);
}
//---------------------------------------------------------------------------

void __fastcall TTMinimapEditor::imgPanelResize(TObject *Sender)
{
    if(image_data.size()==0) return;
    float _k_img                = (float)image_w/(float)image_h;
    float _k_panel              = (float)imgPanel->Width/(float)imgPanel->Height;

    if(_k_img>_k_panel)
    {    //align to width
        image_draw_size.set     (imgPanel->Width, iFloor(imgPanel->Width/_k_img) );
    }else
    {    //align to height
        image_draw_size.set     (iFloor(imgPanel->Height*_k_img), imgPanel->Height);
    }
    VERIFY  (image_draw_size.x<=imgPanel->Width && image_draw_size.y<=imgPanel->Height);
}
//---------------------------------------------------------------------------

void __fastcall TTMinimapEditor::imgPanelMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    if(X>image_draw_size.x || Y>image_draw_size.y)  return;
    float kw = (map_bb_loaded.max.x-map_bb_loaded.min.x) / (float)image_draw_size.x;
    float kh = (map_bb_loaded.max.y-map_bb_loaded.min.y) / (float)image_draw_size.y;

  if( X<(image_draw_size.x/2) && Y<(image_draw_size.y/2) )
  {   //LT
        map_bb.min.x = map_bb_loaded.min.x + X*kw;
        map_bb.min.y = map_bb_loaded.min.y + Y*kh;
  }

  if( X>(image_draw_size.x/2) && Y>(image_draw_size.y/2) )
  {   //RB
        map_bb.max.x = map_bb_loaded.min.x + X*kw;
        map_bb.max.y = map_bb_loaded.min.y + Y*kh;
  }
  ApplyPoints       (true);
}
//---------------------------------------------------------------------------

void __fastcall TTMinimapEditor::ElFloatSpinEditX1Change(TObject *Sender)
{
  ApplyPoints       (false);
}
//---------------------------------------------------------------------------

