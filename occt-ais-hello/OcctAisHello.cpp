#ifdef _WIN32
  #include <windows.h>
#endif

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewController.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OSD.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

#ifdef _WIN32
  #include <WNT_WClass.hxx>
  #include <WNT_Window.hxx>
#elif defined(__APPLE__)
  #include <Cocoa_Window.hxx>
#else
  #include <Xw_Window.hxx>
  #include <X11/Xlib.h>
#endif

#ifdef _MSC_VER
  #pragma comment(lib, "TKOpenGl.lib")
  #pragma comment(lib, "TKV3d.lib")
  #pragma comment(lib, "TKPrim.lib")
  #pragma comment(lib, "TKTopAlgo.lib")
  #pragma comment(lib, "TKBRep.lib")
  #pragma comment(lib, "TKService.lib")
  #pragma comment(lib, "TKMath.lib")
  #pragma comment(lib, "TKernel.lib")
#endif

//! Sample single-window viewer class.
class OcctAisHello : public AIS_ViewController
{
public:
  //! Main constructor.
  OcctAisHello()
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
  #ifdef _WIN32
    const TCollection_AsciiString aClassName ("MyWinClass");
    Handle(WNT_WClass) aWinClass = new WNT_WClass (aClassName.ToCString(), &windowProcWrapper,
                                                   CS_VREDRAW | CS_HREDRAW, 0, 0,
                                                   ::LoadCursor (NULL, IDC_ARROW));
    Handle(WNT_Window) aWindow = new WNT_Window ("OCCT Viewer", aWinClass,  WS_OVERLAPPEDWINDOW,
                                                 100, 100, 512, 512, Quantity_NOC_BLACK);
    ::SetWindowLongPtrW ((HWND )aWindow->NativeHandle(), GWLP_USERDATA, (LONG_PTR )this);
  #elif defined(__APPLE__)
    Handle(Cocoa_Window) aWindow = new Cocoa_Window ( "OCCT Viewer", 100, 100, 512, 512);
  #else
    Handle(Xw_Window) aWindow = new Xw_Window (aDisplay, "OCCT Viewer", 100, 100, 512, 512);
    Display* anXDisplay = (Display* )aDisplay->GetDisplayAspect();
    XSelectInput (anXDisplay, (Window )aWindow->NativeHandle(),
                  ExposureMask | KeyPressMask | KeyReleaseMask | FocusChangeMask | StructureNotifyMask
                | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | Button1MotionMask | Button2MotionMask | Button3MotionMask);
    Atom aDelWinAtom = aDisplay->GetAtom (Aspect_XA_DELETE_WINDOW);
    XSetWMProtocols (anXDisplay, (Window )aWindow->NativeHandle(), &aDelWinAtom, 1);
  #endif
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

  //! Return context.
  const Handle(AIS_InteractiveContext)& Context() const { return myContext; }

  //! Return view.
  const Handle(V3d_View)& View() const { return myView; }

private:
  //! Handle expose event.
  virtual void ProcessExpose() override
  {
    if (!myView.IsNull())
    {
      FlushViewEvents (myContext, myView, true);
    }
  }

  //! Handle window resize event.
  virtual void ProcessConfigure (bool theIsResized) override
  {
    if (!myView.IsNull() && theIsResized)
    {
      myView->Window()->DoResize();
      myView->MustBeResized();
      myView->Invalidate();
      FlushViewEvents (myContext, myView, true);
    }
  }

  //! Handle input.
  virtual void ProcessInput() override
  {
    if (!myView.IsNull())
    {
      ProcessExpose();
    }
  }

#ifdef _WIN32
  //! Window message handler.
  static LRESULT WINAPI windowProcWrapper (HWND theWnd, UINT theMsg, WPARAM theParamW, LPARAM theParamL)
  {
    if (theMsg == WM_CLOSE)
    {
      exit (0);
      return 0;
    }

    if (OcctAisHello* aThis = (OcctAisHello* )::GetWindowLongPtrW (theWnd, GWLP_USERDATA))
    {
      WNT_Window* aWindow = dynamic_cast<WNT_Window* >(aThis->myView->Window().get());
      MSG aMsg = { theWnd, theMsg, theParamW, theParamL };
      if (aWindow->ProcessMessage (*aThis, aMsg))
      {
        return 0;
      }
    }
    return ::DefWindowProcW (theWnd, theMsg, theParamW, theParamL);
  }
#endif
private:

  Handle(AIS_InteractiveContext) myContext;
  Handle(V3d_View) myView;
};

int main()
{
  OSD::SetSignal (false);

  OcctAisHello aViewer;
#ifdef _WIN32
  // WinAPI message loop
  for (;;)
  {
    MSG aMsg = {};
    if (GetMessageW (&aMsg, NULL, 0, 0) <= 0)
    {
      return 0;
    }
    TranslateMessage(&aMsg);
    DispatchMessageW(&aMsg);
  }
#elif defined(__APPLE__)
  /// TODO
  Message::SendFail() << "Critical error: Cocoa message loop is not implemented";
#else
  // X11 event loop
  Handle(Xw_Window) aWindow = Handle(Xw_Window)::DownCast (aViewer.View()->Window());
  Handle(Aspect_DisplayConnection) aDispConn = aViewer.View()->Viewer()->Driver()->GetDisplayConnection();
  Display* anXDisplay = (Display* )aDispConn->GetDisplayAspect();
  for (;;)
  {
    XEvent anXEvent;
    XNextEvent (anXDisplay, &anXEvent);
    aWindow->ProcessMessage (aViewer, anXEvent);
    if (anXEvent.type == ClientMessage && (Atom)anXEvent.xclient.data.l[0] == aDispConn->GetAtom(Aspect_XA_DELETE_WINDOW))
    {
      return 0; // exit when window is closed
    }
  }
#endif
  return 0;
}
