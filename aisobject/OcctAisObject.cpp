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
#else
  #include <Xw_Window.hxx>
  #include <X11/Xlib.h>
#endif

#include <AIS_Animation.hxx>
#include <AIS_AnimationObject.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepBndLib.hxx>
#include <Graphic3d_ArrayOfPoints.hxx>
#include <Prs3d_Arrow.hxx>
#include <Prs3d_ArrowAspect.hxx>
#include <Prs3d_BndBox.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <Prs3d_ToolCylinder.hxx>
#include <Prs3d_ToolDisk.hxx>
#include <Prs3d_PresentationShadow.hxx>
#include <StdPrs_ShadedShape.hxx>
#include <StdPrs_WFShape.hxx>
#include <StdSelect_BRepSelectionTool.hxx>
#include <StdPrs_ToolTriangulatedShape.hxx>
#include <Select3D_SensitiveBox.hxx>
#include <Select3D_SensitivePrimitiveArray.hxx>
#include <V3d_Viewer.hxx>
#include <math_BullardGenerator.hxx>

//! Custom AIS object.
class MyAisObject : public AIS_InteractiveObject
{
  DEFINE_STANDARD_RTTI_INLINE(MyAisObject, AIS_InteractiveObject)
public:
  enum MyDispMode { MyDispMode_Main = 0, MyDispMode_Highlight = 1 };
public:
  MyAisObject();
  void SetAnimation (const Handle(AIS_Animation)& theAnim) { myAnim = theAnim; }
public:
  virtual void Compute (const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                        const Handle(Prs3d_Presentation)& thePrs,
                        const Standard_Integer theMode) override;

  virtual void ComputeSelection (const Handle(SelectMgr_Selection)& theSel,
                                 const Standard_Integer theMode) override;

  virtual bool AcceptDisplayMode (const Standard_Integer theMode) const override
  {
    return theMode == MyDispMode_Main || theMode == MyDispMode_Highlight;
  }
protected:
  Handle(AIS_Animation) myAnim;
  gp_Pnt myDragPntFrom;
};

MyAisObject::MyAisObject()
{
  //SetHilightMode (MyDispMode_Highlight);
  myDrawer->SetupOwnShadingAspect();
  myDrawer->ShadingAspect()->SetMaterial (Graphic3d_NameOfMaterial_Silver);
  myDrawer->SetWireAspect (new Prs3d_LineAspect (Quantity_NOC_GREEN, Aspect_TOL_SOLID, 2.0));
}

void MyAisObject::Compute (const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                           const Handle(Prs3d_Presentation)& thePrs,
                           const Standard_Integer theMode)
{
  const double aRadius = 100.0, aHeight = 100.0;
  TopoDS_Shape aShape = BRepPrimAPI_MakeCone (aRadius, 0.0, aHeight);
  if (theMode == MyDispMode_Main)
  {
    //StdPrs_ShadedShape::Add (thePrs, aShape, myDrawer);
    //StdPrs_WFShape::Add (thePrs, aShape, myDrawer); // add wireframe
    Prs3d_ToolCylinder aCyl (aRadius, 0.0, aHeight, 25, 25);
    Prs3d_ToolDisk aDisk (0.0, aRadius, 25, 1);
    Handle(Graphic3d_ArrayOfTriangles) aTris =
      new Graphic3d_ArrayOfTriangles (aCyl.VerticesNb() + aDisk.VerticesNb(),
      (aCyl.TrianglesNb() + aDisk.TrianglesNb()) * 3,
                                      Graphic3d_ArrayFlags_VertexNormal);
    aCyl .FillArray (aTris, gp_Trsf());
    aDisk.FillArray (aTris, gp_Trsf());
    Handle(Graphic3d_Group) aGroupTris = thePrs->NewGroup();
    aGroupTris->SetGroupPrimitivesAspect (myDrawer->ShadingAspect()->Aspect());
    aGroupTris->AddPrimitiveArray (aTris);
    aGroupTris->SetClosed (true); //

    Handle(Graphic3d_ArrayOfSegments) aSegs = new Graphic3d_ArrayOfSegments (3, 3 * 2, Graphic3d_ArrayFlags_None);
    aSegs->AddVertex (gp_Pnt (0.0, 0.0, aHeight));
    aSegs->AddVertex (gp_Pnt (0.0, -aRadius, 0.0));
    aSegs->AddVertex (gp_Pnt (0.0,  aRadius, 0.0));
    aSegs->AddEdges (1, 2);
    aSegs->AddEdges (2, 3);
    aSegs->AddEdges (3, 1);
    Handle(Graphic3d_Group) aGroupSegs = thePrs->NewGroup();
    aGroupSegs->SetGroupPrimitivesAspect (myDrawer->WireAspect()->Aspect());
    aGroupSegs->AddPrimitiveArray (aSegs);
  }
  else if (theMode == MyDispMode_Highlight)
  {
    Bnd_Box aBox;
    BRepBndLib::Add (aShape, aBox);
    Prs3d_BndBox::Add (thePrs, aBox, myDrawer);
  }
}

//! Custom AIS owner.
class MyAisOwner : public SelectMgr_EntityOwner
{
  DEFINE_STANDARD_RTTI_INLINE(MyAisOwner, SelectMgr_EntityOwner)
public:
  MyAisOwner (const Handle(MyAisObject)& theObj, int thePriority = 0)
  : SelectMgr_EntityOwner (theObj, thePriority) {}

  void SetAnimation (const Handle(AIS_Animation)& theAnim) { myAnim = theAnim; }

  virtual void HilightWithColor (const Handle(PrsMgr_PresentationManager)& thePM,
                                 const Handle(Prs3d_Drawer)& theStyle,
                                 const Standard_Integer theMode) override;
  virtual void Unhilight (const Handle(PrsMgr_PresentationManager)& thePM,
                          const Standard_Integer theMode) override;

  virtual bool IsForcedHilight() const override { return true; }
  virtual bool HandleMouseClick (const Graphic3d_Vec2i& thePoint,
                                 Aspect_VKeyMouse theButton,
                                 Aspect_VKeyFlags theModifiers,
                                 bool theIsDoubleClick) override;
  virtual void SetLocation (const TopLoc_Location& theLocation) override
  {
    if (!myPrs.IsNull()) { myPrs->SetTransformation (new TopLoc_Datum3D (theLocation.Transformation())); }
  }
protected:
  Handle(Prs3d_Presentation) myPrs;
  Handle(AIS_Animation) myAnim;
};

void MyAisOwner::HilightWithColor (const Handle(PrsMgr_PresentationManager)& thePM,
                                   const Handle(Prs3d_Drawer)& theStyle,
                                   const Standard_Integer theMode)
{
  auto anObj = dynamic_cast<MyAisObject*> (mySelectable);
  if (myPrs.IsNull())
  {
    myPrs = new Prs3d_Presentation (thePM->StructureManager());
    anObj->Compute (thePM, myPrs, MyAisObject::MyDispMode_Highlight);
  }
  if (thePM->IsImmediateModeOn())
  {
    Handle(StdSelect_ViewerSelector3d) aSelector = anObj->InteractiveContext()->MainSelector();
    SelectMgr_SortCriterion aPickPnt;
    for (int aPickIter = 1; aPickIter <= aSelector->NbPicked(); ++aPickIter)
    {
      if (aSelector->Picked (aPickIter) == this)
      {
        aPickPnt = aSelector->PickedData (aPickIter);
        break;
      }
    }

    Handle(Prs3d_Presentation) aPrs = mySelectable->GetHilightPresentation (thePM);
    aPrs->Clear();
    Handle(Graphic3d_Group) aGroupPnt = aPrs->NewGroup();
    aGroupPnt->SetGroupPrimitivesAspect (theStyle->ArrowAspect()->Aspect());

    gp_Trsf aTrsfInv (mySelectable->InversedTransformation().Trsf());
    gp_Dir  aNorm (aPickPnt.Normal.x(), aPickPnt.Normal.y(), aPickPnt.Normal.z());
    Handle(Graphic3d_ArrayOfTriangles) aTris =
      Prs3d_Arrow::DrawShaded (gp_Ax1(aPickPnt.Point, aNorm).Transformed (aTrsfInv),
                               1.0, 15.0,
                               3.0, 4.0, 10);
    aGroupPnt->AddPrimitiveArray (aTris);

    aPrs->SetZLayer (Graphic3d_ZLayerId_Top);
    thePM->AddToImmediateList (aPrs);

    //Handle(Prs3d_PresentationShadow) aShadow = new Prs3d_PresentationShadow (thePM->StructureManager(), myPrs);
    //aShadow->SetZLayer (Graphic3d_ZLayerId_Top);
    //aShadow->Highlight (theStyle);
    //thePM->AddToImmediateList (aShadow);
  }
  else
  {
    myPrs->SetTransformation (mySelectable->TransformationGeom());
    myPrs->Display();
  }
}

void MyAisOwner::Unhilight (const Handle(PrsMgr_PresentationManager)& thePM,
                            const Standard_Integer theMode)
{
  if (!myPrs.IsNull())
  {
    myPrs->Erase();
  }
}

bool MyAisOwner::HandleMouseClick (const Graphic3d_Vec2i& thePoint,
                                   Aspect_VKeyMouse theButton,
                                   Aspect_VKeyFlags theModifiers,
                                   bool theIsDoubleClick)
{
  {
    static math_BullardGenerator aRandGen;
    Quantity_Color aRandColor (float(aRandGen.NextInt() % 256) / 255.0f,
                               float(aRandGen.NextInt() % 256) / 255.0f,
                               float(aRandGen.NextInt() % 256) / 255.0f,
                               Quantity_TOC_sRGB);
    mySelectable->Attributes()->ShadingAspect()->SetColor(aRandColor);
    mySelectable->SynchronizeAspects();
  }

  if (!myAnim.IsNull())
  {
    static bool isFirst = true;
    isFirst = !isFirst;
    auto anObj = dynamic_cast<MyAisObject*> (mySelectable);

    gp_Trsf aTrsfTo;
    aTrsfTo.SetRotation (gp_Ax1 (gp::Origin(), gp::DX()), isFirst ? M_PI * 0.5 : -M_PI * 0.5);
    gp_Trsf aTrsfFrom = anObj->LocalTransformation();
    Handle(AIS_AnimationObject) anAnim = new AIS_AnimationObject ("MyAnim", anObj->InteractiveContext(), anObj, aTrsfFrom, aTrsfTo);
    anAnim->SetOwnDuration (2.0);

    myAnim->Clear();
    myAnim->Add (anAnim);
    myAnim->StartTimer (0.0, 1.0, true);
  }

  return true;
}

void MyAisObject::ComputeSelection (const Handle(SelectMgr_Selection)& theSel,
                                    const Standard_Integer theMode)
{
  const double aRadius = 100.0, aHeight = 100.0;
  TopoDS_Shape aShape = BRepPrimAPI_MakeCone (aRadius, 0.0, aHeight);
  Bnd_Box aBox;
  BRepBndLib::Add (aShape, aBox);
  Handle(MyAisOwner) anOwner = new MyAisOwner (this);
  anOwner->SetAnimation (myAnim);

  Handle(Graphic3d_ArrayOfTriangles) aTris = Prs3d_ToolCylinder::Create (aRadius, 0.0, aHeight, 25, 25, gp_Trsf());
  auto aSensTri = new Select3D_SensitivePrimitiveArray (anOwner);
  aSensTri->InitTriangulation (aTris->Attributes(), aTris->Indices(), TopLoc_Location());
  theSel->Add (aSensTri);

  //Handle(SelectMgr_EntityOwner) anOwner = new SelectMgr_EntityOwner (this);
  //Handle(Select3D_SensitiveBox) aSensBox = new Select3D_SensitiveBox (anOwner, aBox);
  //theSel->Add (aSensBox);
}

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
    Handle(WNT_Window) aWindow = new WNT_Window ("OCCT Viewer", aWinClass,  WS_OVERLAPPEDWINDOW,
                                                 100, 100, 512, 512, Quantity_NOC_BLACK);
    ::SetWindowLongPtrW ((HWND )aWindow->NativeHandle(), GWLP_USERDATA, (LONG_PTR )this);
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

    Handle(MyAisObject) aPrs = new MyAisObject();
    aPrs->SetAnimation (AIS_ViewController::ObjectsAnimation());
    myContext->Display (aPrs, MyAisObject::MyDispMode_Main, 0, false);
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

  Handle(AIS_InteractiveContext) myContext;
  Handle(V3d_View) myView;
};

int main()
{
  OSD::SetSignal (false);

  MyViewer aViewer;
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
