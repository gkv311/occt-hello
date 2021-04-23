#include <windows.h>

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewController.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

#include <WNT_WClass.hxx>
#include <WNT_Window.hxx>

#pragma comment(lib, "TKOpenGl.lib")
#pragma comment(lib, "TKV3d.lib")
#pragma comment(lib, "TKPrim.lib")   
#pragma comment(lib, "TKTopAlgo.lib")
#pragma comment(lib, "TKBRep.lib")
#pragma comment(lib, "TKService.lib")
#pragma comment(lib, "TKMath.lib")
#pragma comment(lib, "TKernel.lib")

//! Sample single-window viewer class.
class MyViewer : public AIS_ViewController
{
public:
  //! Main constructor.
  MyViewer()
  {
    // graphic driver setup
    Handle(Aspect_DisplayConnection) aDisplay = new Aspect_DisplayConnection();
    Handle(Graphic3d_GraphicDriver) aDriver = new OpenGl_GraphicDriver (aDisplay);

    // viewer setup
    Handle(V3d_Viewer) aViewer = new V3d_Viewer (aDriver);
    aViewer->SetDefaultLights();
    aViewer->SetLightOn();

    // view setup
    myView = new V3d_View (aViewer);
    const TCollection_AsciiString aClassName ("MyWinClass");
    Handle(WNT_WClass) aWinClass = new WNT_WClass (aClassName.ToCString(), &windowProcWrapper, 0);
    Handle(WNT_Window) aWindow = new WNT_Window ("OCCT Viewer", aWinClass,  WS_OVERLAPPEDWINDOW, 
                                                 100, 100, 512, 512, Quantity_NOC_BLACK);
    ::SetWindowLongPtrW ((HWND )aWindow->NativeHandle(), GWLP_USERDATA, (LONG_PTR )this);
    myView->SetWindow (aWindow);
    myView->SetBackgroundColor (Quantity_NOC_GRAY50);
    myView->TriedronDisplay (Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, 0.1);
    myView->ChangeRenderingParams().RenderResolutionScale = 2.0f;

    // interactive context and demo scene
    myContext = new AIS_InteractiveContext (aViewer);

    TopoDS_Shape aShape = BRepPrimAPI_MakeBox (100, 100, 100).Solid();
    Handle(AIS_InteractiveObject) aShapePrs = new AIS_Shape (aShape);
    myContext->Display (aShapePrs, AIS_Shaded, 0, false);
    myView->FitAll (0.01, false);

    aWindow->Map();
    myView->Redraw();
  }

private:
  //! Window message handler.
  static LRESULT WINAPI windowProcWrapper (HWND theWnd, UINT theMsg, WPARAM theParamW, LPARAM theParamL)
  {
    MyViewer* aThis = (MyViewer* )::GetWindowLongPtrW (theWnd, GWLP_USERDATA);
    return aThis != NULL
         ? aThis->windowProc(theWnd, theMsg, theParamW, theParamL)
         : ::DefWindowProcW (theWnd, theMsg, theParamW, theParamL);
  }

  //! Window message handler.
  LRESULT WINAPI windowProc (HWND theWnd, UINT theMsg, WPARAM theParamW, LPARAM theParamL)
  {
    switch (theMsg)
    {
      case WM_CLOSE:
      {
        exit (0);
        return 0;
      }
      case WM_PAINT:
      {
        PAINTSTRUCT aPaint;
        ::BeginPaint(theWnd, &aPaint);
        ::EndPaint  (theWnd, &aPaint);
        myView->Redraw();
        break;
      }
      case WM_SIZE:
      {
        myView->MustBeResized();
        AIS_ViewController::FlushViewEvents (myContext, myView, true);
        break;
      }
      case WM_LBUTTONUP:
      case WM_MBUTTONUP:
      case WM_RBUTTONUP:
      case WM_LBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_RBUTTONDOWN:
      {
        const Graphic3d_Vec2i aPos (LOWORD(theParamL), HIWORD(theParamL));
        const Aspect_VKeyFlags aFlags = WNT_Window::MouseKeyFlagsFromEvent (theParamW);
        Aspect_VKeyMouse aButton = Aspect_VKeyMouse_NONE;
        switch (theMsg)
        {
          case WM_LBUTTONUP:
          case WM_LBUTTONDOWN:
            aButton = Aspect_VKeyMouse_LeftButton;
            break;
          case WM_MBUTTONUP:
          case WM_MBUTTONDOWN:
            aButton = Aspect_VKeyMouse_MiddleButton;
            break;
          case WM_RBUTTONUP:
          case WM_RBUTTONDOWN:
            aButton = Aspect_VKeyMouse_RightButton;
            break;
        }
        if (theMsg == WM_LBUTTONDOWN
         || theMsg == WM_MBUTTONDOWN
         || theMsg == WM_RBUTTONDOWN)
        {
          ::SetFocus  (theWnd);
          ::SetCapture(theWnd);
          AIS_ViewController::PressMouseButton (aPos, aButton, aFlags, false);
        }
        else
        {
          ::ReleaseCapture();
          AIS_ViewController::ReleaseMouseButton (aPos, aButton, aFlags, false);
        }
        AIS_ViewController::FlushViewEvents (myContext, myView, true);
        break;
      }
      case WM_MOUSEMOVE:
      {
        Graphic3d_Vec2i aPos (LOWORD(theParamL), HIWORD(theParamL));
        Aspect_VKeyMouse aButtons = WNT_Window::MouseButtonsFromEvent (theParamW);
        Aspect_VKeyFlags aFlags   = WNT_Window::MouseKeyFlagsFromEvent(theParamW);
        CURSORINFO aCursor;
        aCursor.cbSize = sizeof(aCursor);
        if (::GetCursorInfo (&aCursor) != FALSE)
        {
          POINT aCursorPnt = { aCursor.ptScreenPos.x, aCursor.ptScreenPos.y };
          if (::ScreenToClient (theWnd, &aCursorPnt))
          {
            // as we override mouse position, we need overriding also mouse state
            aPos.SetValues (aCursorPnt.x, aCursorPnt.y);
            aButtons = WNT_Window::MouseButtonsAsync();
            aFlags   = WNT_Window::MouseKeyFlagsAsync();
          }
        }

        AIS_ViewController::UpdateMousePosition (aPos, aButtons, aFlags, false);
        AIS_ViewController::FlushViewEvents (myContext, myView, true);
        break;
      }
      case WM_MOUSEWHEEL:
      {
        const int aDelta = GET_WHEEL_DELTA_WPARAM (theParamW);
        const Standard_Real aDeltaF = Standard_Real(aDelta) / Standard_Real(WHEEL_DELTA);
        const Aspect_VKeyFlags aFlags = WNT_Window::MouseKeyFlagsFromEvent (theParamW);
        Graphic3d_Vec2i aPos (int(short(LOWORD(theParamL))), int(short(HIWORD(theParamL))));
        POINT aCursorPnt = { aPos.x(), aPos.y() };
        if (::ScreenToClient (theWnd, &aCursorPnt))
        {
          aPos.SetValues (aCursorPnt.x, aCursorPnt.y);
        }

        AIS_ViewController::UpdateMouseScroll (Aspect_ScrollDelta (aPos, aDeltaF, aFlags));
        AIS_ViewController::FlushViewEvents (myContext, myView, true);
        break;
      }
      default:
      {
        return ::DefWindowProcW (theWnd, theMsg, theParamW, theParamL);
      }
    }
    return 0;
  }
private:

  Handle(AIS_InteractiveContext) myContext;
  Handle(V3d_View) myView;
};

int main()
{
  MyViewer aViewer;
  for (;;) // message loop
  {
    MSG aMsg = {};
    if (GetMessageW (&aMsg, NULL, 0, 0) <= 0)
    {
      return 0;
    }
    TranslateMessage(&aMsg);
    DispatchMessageW(&aMsg);
  }
  return 0;
}
