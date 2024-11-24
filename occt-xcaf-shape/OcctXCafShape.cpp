#ifdef _WIN32
  #include <windows.h>
#endif

#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewController.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OSD.hxx>
#include <OSD_Environment.hxx>
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

#include <STEPCAFControl_Controller.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TDataStd_Name.hxx>
#include <TDF_Tool.hxx>
#include <TDocStd_Application.hxx>
#include <BinXCAFDrivers.hxx>

#include <XCAFPrs.hxx>
#include <XCAFPrs_AISObject.hxx>
#include <XCAFPrs_DocumentExplorer.hxx>
#include <XCAFPrs_DocumentIdIterator.hxx>

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
  #ifdef _WIN32
    const TCollection_AsciiString aClassName ("MyWinClass");
    Handle(WNT_WClass) aWinClass = new WNT_WClass (aClassName.ToCString(), &windowProcWrapper,
                                                   CS_VREDRAW | CS_HREDRAW, 0, 0,
                                                   ::LoadCursor (NULL, IDC_ARROW));
    Handle(WNT_Window) aWindow = new WNT_Window ("OCCT Viewer - XCAF shape", aWinClass,  WS_OVERLAPPEDWINDOW,
                                                 100, 100, 512, 512, Quantity_NOC_BLACK);
    ::SetWindowLongPtrW ((HWND )aWindow->NativeHandle(), GWLP_USERDATA, (LONG_PTR )this);
  #elif defined(__APPLE__)
    Handle(Cocoa_Window) aWindow = new Cocoa_Window ( "OCCT Viewer", 100, 100, 512, 512);
  #else
    Handle(Xw_Window) aWindow = new Xw_Window (aDisplay, "OCCT Viewer - XCAF shape", 100, 100, 512, 512);
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

    aWindow->Map();
    myView->Redraw();
  }

  //! Return context.
  const Handle(AIS_InteractiveContext)& Context() const { return myContext; }

  //! Return view.
  const Handle(V3d_View)& View() const { return myView; }

public:

  //! Save XBF file.
  bool SaveXBF (const TCollection_AsciiString& theFilePath)
  {
    if (myXdeDoc.IsNull()) { return false; }
    const PCDM_StoreStatus aStatus = myXdeApp->SaveAs (myXdeDoc, TCollection_ExtendedString (theFilePath));
    if (aStatus != PCDM_SS_OK)
    {
      Message::SendFail() << "Error occurred during XBF export: " << (int )aStatus << ".\n" << theFilePath;
      return false;
    }
    return true;
  }

  //! Open XBF file.
  bool OpenXBF (const TCollection_AsciiString& theFilePath)
  {
    // create an empty XCAF document
    createXCAFApp();
    newDocument();

    const PCDM_ReaderStatus aReaderStatus = myXdeApp->Open (theFilePath, myXdeDoc);
    if (aReaderStatus != PCDM_RS_OK)
    {
      Message::SendFail() << "Error occurred during XBF import: " << (int )aReaderStatus << ".\n" << theFilePath;
      return false;
    }
    return true;
  }

  //! Open STEP file.
  bool OpenSTEP (const TCollection_AsciiString& theFilePath)
  {
    // create an empty XCAF document
    createXCAFApp();
    newDocument();

    // initialize STEP reader parameters
    STEPCAFControl_Controller::Init();
    STEPControl_Controller::Init();

    // read and translate STEP file into XCAF document
    STEPCAFControl_Reader aReader;
    OSD_Timer aTimer;
    aTimer.Start();
    try
    {
      if (aReader.ReadFile (theFilePath.ToCString()) != IFSelect_RetDone) // read model from file
      {
        Message::SendFail() << "Error occurred reading STEP file\n" << theFilePath;
        return false;
      }
      if (!aReader.Transfer (myXdeDoc)) // translate model into document
      {
        Message::SendFail() << "Error occurred transferring STEP file\n" << theFilePath;
        return false;
      }

      Message::SendInfo() << "File '" << theFilePath << "' opened in " << aTimer.ElapsedTime() << " s";
    }
    catch (Standard_Failure const& theFailure)
    {
      Message::SendFail() << "Exception raised during STEP import\n[" << theFailure.GetMessageString() << "]\n" << theFilePath;
      return false;
    }
    return true;
  }

  //! Dump XCAF document tree.
  void DumpXCafDocumentTree()
  {
    if (myXdeDoc.IsNull()) { return; }
    for (XCAFPrs_DocumentExplorer aDocExp (myXdeDoc, XCAFPrs_DocumentExplorerFlags_None); aDocExp.More(); aDocExp.Next())
    {
      //std::cout << aDocExp.Current().Id << "\n";
      //std::cout << getXCafNodePathNames (aDocExp, false, 0) << "\n";

      TCollection_AsciiString aName = getXCafNodePathNames (aDocExp, false, aDocExp.CurrentDepth());
      aName = TCollection_AsciiString (aDocExp.CurrentDepth() * 2, ' ') + aName;
      std::cout << aName << "\n";
    }
    std::cout << "\n";
  }

  //! Display XCAF document within AIS context.
  void DisplayXCafDocument (bool theToExplode)
  {
    if (myXdeDoc.IsNull()) { return; }
    for (XCAFPrs_DocumentExplorer aDocExp (myXdeDoc, XCAFPrs_DocumentExplorerFlags_None); aDocExp.More(); aDocExp.Next())
    {
      const XCAFPrs_DocumentNode& aNode = aDocExp.Current();
      if (theToExplode)
      {
        if (aNode.IsAssembly) { continue; } // handle only leaves
      }
      else
      {
        if (aDocExp.CurrentDepth() != 0) { continue; } // handle only roots
      }

      Handle(XCAFPrs_AISObject) aPrs = new XCAFPrs_AISObject (aNode.RefLabel);
      if (!aNode.Location.IsIdentity()) { aPrs->SetLocalTransformation (aNode.Location); }

      // AIS object's owner is an application-owned property; it is set to string object in this sample
      aPrs->SetOwner (new TCollection_HAsciiString (aNode.Id));

      myContext->Display (aPrs, AIS_Shaded, 0, false);
    }

    myView->FitAll (0.01, false);
    AIS_ViewController::ProcessExpose();
  }

private:

  //! Create XCAF application instance.
  bool createXCAFApp()
  {
    if (!myXdeApp.IsNull()) { return true; }
    try
    {
      OCC_CATCH_SIGNALS
      myXdeApp = new TDocStd_Application();
      //XmlXCAFDrivers::DefineFormat (myXdeApp); // to load XML files
      BinXCAFDrivers::DefineFormat (myXdeApp); // to load XBF files
      return true;
    }
    catch (Standard_Failure const& theFailure)
    {
      Message::SendFail() << "Error in creating XCAF application " << theFailure.GetMessageString();
      return false;
    }
  }

  //! Create new document.
  void newDocument()
  {
    // close old document
    if (!myXdeDoc.IsNull())
    {
      if (myXdeDoc->HasOpenCommand()) { myXdeDoc->AbortCommand(); }
      myXdeDoc->Main().Root().ForgetAllAttributes (true);
      myXdeApp->Close (myXdeDoc);
      myXdeDoc.Nullify();
    }

    // create new document
    if (!myXdeApp.IsNull()) { myXdeApp->NewDocument (TCollection_ExtendedString ("BinXCAF"), myXdeDoc); }
    if (!myXdeDoc.IsNull()) { myXdeDoc->SetUndoLimit(10); } // set the maximum number of available "undo" actions
  }

  //! Format XCAF node's name(s) starting from parent to leaf.
  static TCollection_AsciiString getXCafNodePathNames (const XCAFPrs_DocumentExplorer& theExp,
                                                       const bool theIsInstanceName,
                                                       const int theLowerDepth = 0)
  {
    TCollection_AsciiString aPath;
    for (int aDepth = theLowerDepth; aDepth <= theExp.CurrentDepth(); ++aDepth)
    {
      const XCAFPrs_DocumentNode& aNode = theExp.Current (aDepth);
      TCollection_AsciiString aName;
      Handle(TDataStd_Name) aNodeName;
      if (theIsInstanceName)
      {
        if (aNode.Label.FindAttribute (TDataStd_Name::GetID(), aNodeName))
        {
          aName = aNodeName->Get();
        }
      }
      else
      {
        if (aNode.RefLabel.FindAttribute (TDataStd_Name::GetID(), aNodeName))
        {
          aName = aNodeName->Get();
        }
      }

      if (aName.IsEmpty()) { TDF_Tool::Entry (aNode.Label, aName); }
      if (aNode.IsAssembly) { aName += "/"; }
      aPath += aName;
    }
    return aPath;
  }

private:

  //! Print some information about selected object.
  virtual void OnSelectionChanged (const Handle(AIS_InteractiveContext)& theCtx,
                                   const Handle(V3d_View)& theView) override
  {
    for (const Handle(SelectMgr_EntityOwner)& aSelIter : theCtx->Selection()->Objects())
    {
      Handle(XCAFPrs_AISObject) anXCafPrs = Handle(XCAFPrs_AISObject)::DownCast (aSelIter->Selectable());
      if (anXCafPrs.IsNull()) { continue; }

      {
        // AIS object's owner is an application-owned property; it is set to string object in this sample
        Handle(TCollection_HAsciiString) anId = Handle(TCollection_HAsciiString)::DownCast (anXCafPrs->GetOwner());
        std::cout << "Selected Id: '" << (!anId.IsNull() ? anId->String() : "") << "'\n";
      }

      {
        Handle(TDataStd_Name) aNodeName;
        if (anXCafPrs->GetLabel().FindAttribute (TDataStd_Name::GetID(), aNodeName))
        {
          std::cout << "       Name: '" << aNodeName->Get() << "'\n";
        }
      }

      {
        // print information on colored subshapes
        TopLoc_Location aLoc;
        XCAFPrs_IndexedDataMapOfShapeStyle aStyles;
        XCAFPrs::CollectStyleSettings (anXCafPrs->GetLabel(), aLoc, aStyles);
        NCollection_Map<Quantity_ColorRGBA, Quantity_ColorRGBAHasher> aColorFilter;
        std::cout << "     Colors:";
        for (XCAFPrs_IndexedDataMapOfShapeStyle::Iterator aStyleIter (aStyles); aStyleIter.More(); aStyleIter.Next())
        {
          const XCAFPrs_Style& aStyle = aStyleIter.Value();
          if (aStyle.IsSetColorSurf()
           && aColorFilter.Add (aStyle.GetColorSurfRGBA()))
          {
            std::cout << " " << Quantity_ColorRGBA::ColorToHex (aStyle.GetColorSurfRGBA());
          }
        }
        std::cout << "\n";
      }
    }
  }

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

    if (MyViewer* aThis = (MyViewer* )::GetWindowLongPtrW (theWnd, GWLP_USERDATA))
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

  Handle(AIS_InteractiveContext) myContext; //!< AIS viewer interactive context
  Handle(V3d_View)               myView;    //!< AIS view (window)

  Handle(TDocStd_Application)    myXdeApp;  //!< XDE application instance
  Handle(TDocStd_Document)       myXdeDoc;  //!< XDE document instance
};

//! Fill in array of program arguments.
static void fillAppArguments (std::vector<TCollection_AsciiString>& theArgsOut,
                              int theNbArgs, char** theArgVec)
{
#if defined(_WIN32)
  // fetch arguments using UNICODE interface
  (void )theNbArgs; (void )theArgVec;
  int aNbArgs = 0;
  wchar_t** anArgVecW = ::CommandLineToArgvW (GetCommandLineW(), &aNbArgs);
  for (int anArgIter = 0; anArgIter < aNbArgs; ++anArgIter)
  {
    theArgsOut.push_back (anArgVecW[anArgIter]);
  }
  ::LocalFree (anArgVecW);
#else
  for (int anArgIter = 0; anArgIter < theNbArgs; ++anArgIter)
  {
    theArgsOut.push_back (theArgVec[anArgIter]);
  }
#endif
}

int main (int theNbArgs, char** theArgVec)
{
  OSD::SetSignal (false);

  std::vector<TCollection_AsciiString> anArgs;
  fillAppArguments (anArgs, theNbArgs, theArgVec);

  TCollection_AsciiString aModelPath;
  if (anArgs.size() > 2)
  {
    Message::SendFail() << "Syntax error: wrong number of arguments";
    return 1;
  }

  if (anArgs.size() == 2)
  {
    aModelPath = anArgs[1];
  }
  else
  {
    OSD_Environment aVarModDir ("SAMPLE_MODELS_DIR");
    if (!aVarModDir.Value().IsEmpty())
    {
      aModelPath = aVarModDir.Value() + "/as1-oc-214.stp";
    }
    else
    {
      Message::SendFail() << "Warning: variable SAMPLE_MODELS_DIR not set";
    }
  }

  MyViewer aViewer;
  if (!aModelPath.IsEmpty())
  {
    TCollection_AsciiString aNameLower = aModelPath;
    aNameLower.LowerCase();
    if (aNameLower.EndsWith (".xbf"))
    {
      aViewer.OpenXBF (aModelPath);
    }
    else //if (aNameLower.EndsWith (".stp") || aNameLower.EndsWith (".step"))
    {
      aViewer.OpenSTEP (aModelPath);
    }

    aViewer.DumpXCafDocumentTree();
    aViewer.DisplayXCafDocument (true);
  }

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
