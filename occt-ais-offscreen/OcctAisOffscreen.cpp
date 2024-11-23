#ifdef _WIN32
  #include <windows.h>
#endif

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Line.hxx>
#include <Image_AlienPixMap.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OSD.hxx>
#include <PrsDim_DiameterDimension.hxx>
#include <PrsDim_LengthDimension.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

#ifdef _WIN32
  #include <WNT_WClass.hxx>
  #include <WNT_Window.hxx>
#else
  #include <Xw_Window.hxx>
  #include <X11/Xlib.h>
#endif

//! Sample offscreen viewer class.
class OcctOffscreenViewer
{
public:

  //! Return view instance.
  const Handle(V3d_View)& View() const { return myView; }

  //! Return AIS context.
  const Handle(AIS_InteractiveContext)& Context() const { return myContext; }

  //! Initialize offscreen viewer.
  //! @param[in] theWinSize view dimensions
  //! @return FALSE in case of initialization error
  bool InitOffscreenViewer (const Graphic3d_Vec2i& theWinSize)
  {
    try
    {
      OCC_CATCH_SIGNALS

      // create graphic driver
      Handle(Aspect_DisplayConnection) aDispConnection = new Aspect_DisplayConnection();
      Handle(OpenGl_GraphicDriver) aDriver = new OpenGl_GraphicDriver (aDispConnection, true);
      aDriver->ChangeOptions().ffpEnable    = false;
      aDriver->ChangeOptions().swapInterval = 0;

      // create viewer and AIS context
      myViewer  = new V3d_Viewer (aDriver);
      myContext = new AIS_InteractiveContext (myViewer);

      // light sources setup
      myViewer->SetDefaultLights();
      myViewer->SetLightOn();

      // create offscreen window
      const TCollection_AsciiString aWinName ("OCCT offscreen window");
    #ifdef __ANDROID__
      Handle(Aspect_NeutralWindow) aWindow = new Aspect_NeutralWindow();
      aWindow->SetSize (theWinSize.x(), theWinSize.y());
    #elif defined(_WIN32)
      const TCollection_AsciiString aClassName ("OffscreenClass");
      Handle(WNT_WClass) aWinClass = new WNT_WClass (aClassName.ToCString(), NULL, 0); // empty callback!
      Handle(WNT_Window) aWindow   = new WNT_Window (aWinName.ToCString(), aWinClass, 0x80000000L, //WS_POPUP,
                                                     64, 64, 64, 64, Quantity_NOC_BLACK);
      aWindow->SetVirtual (true);
      aWindow->SetPos (0, 0, theWinSize.x(), theWinSize.y());
    #elif defined(__APPLE__)
      Handle(Cocoa_Window) aWindow = new Cocoa_Window (aWinName.ToCString(), 64, 64, theWinSize.x(), theWinSize.y());
    #else
      Handle(Xw_Window) aWindow = new Xw_Window (aDispConnection, aWinName.ToCString(),
                                                 64, 64, theWinSize.x(), theWinSize.y());
    #endif
      aWindow->SetVirtual (true);

      // create 3D view from offscreen window
      myView = new V3d_View (myViewer);
      myView->SetWindow (aWindow);
    }
    catch (const Standard_Failure& theErr)
    {
      Message::SendFail() << "Offscreen Viewer creation FAILED:\n" << theErr;
      return false;
    }
    return true;
  }

  //! Print information about graphics context.
  void DumpGlInfo()
  {
    TColStd_IndexedDataMapOfStringString aGlCapsDict;
    myView->DiagnosticInformation (aGlCapsDict, Graphic3d_DiagnosticInfo_Basic);
    TCollection_AsciiString anInfo = "OpenGL info:\n";
    for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter (aGlCapsDict); aValueIter.More(); aValueIter.Next())
    {
      if (!aValueIter.Value().IsEmpty())
      {
        anInfo += TCollection_AsciiString("  ") + aValueIter.Key() + ": " + aValueIter.Value() + "\n";
      }
    }
    Message::SendInfo (anInfo);
  }

private:

  Handle(V3d_Viewer) myViewer;
  Handle(V3d_View)   myView;
  Handle(AIS_InteractiveContext) myContext;

};

int main(int argc, const char** argv)
{
  OSD::SetSignal (false);

  // image dimensions
  Graphic3d_Vec2i aWinSize (1920, 1080);
  double aScaleRatio = 2.0;

  // create offsreen viewer
  OcctOffscreenViewer aViewer;
  if (!aViewer.InitOffscreenViewer (aWinSize))
  {
    return 1;
  }

  // setup rendering parameters
  const Handle(V3d_View)& aView = aViewer.View();
  Graphic3d_RenderingParams& aRendParams = aView->ChangeRenderingParams();
  aRendParams.Resolution = (unsigned int )(96.0 * aScaleRatio + 0.5); // text resolution
  //aRendParams.NbMsaaSamples = 4; // MSAA
  aRendParams.RenderResolutionScale = 2.0f; // SSAA as alternative to MSAA
  aViewer.DumpGlInfo();

  // display something
  aView->SetBackgroundColor (Quantity_NOC_BLACK);
  aView->TriedronDisplay (Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, aScaleRatio * 0.1);
  {
    const Handle(AIS_InteractiveContext)& aCtx = aViewer.Context();
    const Handle(Prs3d_Drawer)& aDrawer = aCtx->DefaultDrawer();
    aDrawer->ShadingAspect()->SetMaterial (Graphic3d_NameOfMaterial_Glass);
    aDrawer->SetFaceBoundaryDraw (true);

    TopoDS_Shape aShape = BRepPrimAPI_MakeCone (100, 10, 100).Solid();
    Handle(AIS_InteractiveObject) aShapePrs = new AIS_Shape (aShape);
    aCtx->Display (aShapePrs, AIS_Shaded, -1, false);

    TopTools_IndexedMapOfShape anEdges;
    TopExp::MapShapes (aShape, TopAbs_EDGE, anEdges);
    for (TopTools_IndexedMapOfShape::Iterator anEdgeIter (anEdges); anEdgeIter.More(); anEdgeIter.Next())
    {
      const TopoDS_Edge& anEdge = TopoDS::Edge (anEdgeIter.Value());
      Standard_Real aParRange[2] = {};
      Handle(Geom_Curve) aCurve = BRep_Tool::Curve (anEdge, aParRange[0], aParRange[1]);
      if (Handle(Geom_Circle) aCircle = Handle(Geom_Circle)::DownCast (aCurve))
      {
        Handle(PrsDim_DiameterDimension) aDiamDim = new PrsDim_DiameterDimension (anEdge);
        aDiamDim->SetFlyout (aCircle->Radius() + 20.0);
        aCtx->Display (aDiamDim, 0, -1, false);
      }
      else if (Handle(Geom_Line) aLine = Handle(Geom_Line)::DownCast (aCurve))
      {
        gp_Pln aPln (aLine->Value (aParRange[0]), gp::DY());
        Handle(PrsDim_LengthDimension) aLenDim = new PrsDim_LengthDimension (anEdge, aPln);
        aLenDim->SetFlyout (20.0);
        aCtx->Display (aLenDim, 0, -1, false);
      }
    }
  }

  // setup camera orientation
  aView->SetProj (V3d_TypeOfOrientation_Zup_AxoRight);
  aView->FitAll (0.01, false);

  // make a screenshot
  Image_AlienPixMap anImage;
  if (!aView->ToPixMap (anImage, aWinSize.x(), aWinSize.y()))
  {
    Message::SendFail() << "View dump FAILED";
    return 1;
  }

  // save image to file
  const char* anImageName = "image.png";
  if (!anImage.Save (anImageName))
  {
    Message::SendFail() << "Unable to save image " << anImage.Width() << "x" << anImage.Height() << "@" << Image_PixMap::ImageFormatToString(anImage.Format())
                        << " into file '" << anImageName << "'";
    return 1;
  }
  Message::SendInfo() << "Screenshot " << anImage.Width() << "x" << anImage.Height() << "@" << Image_PixMap::ImageFormatToString(anImage.Format())
                      << " saved into file '" << anImageName << "'";

  // use default application to open image
  for (int anArgIter = 1; anArgIter < argc; ++anArgIter)
  {
    if (TCollection_AsciiString::IsSameString (argv[anArgIter], "-noopen", false))
    {
      return 0;
    }
  }
#if defined(_WIN32)
  ShellExecuteW(NULL, L"open", TCollection_ExtendedString(anImageName).ToWideString(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__linux__)
  // https://www.freedesktop.org/wiki/Software/xdg-utils/
  TCollection_AsciiString aCmd = TCollection_AsciiString("xdg-open ") + anImageName;
  std::system(aCmd.ToCString());
#endif
  return 0;
}
