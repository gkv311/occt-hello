#include <Draw.hxx>
#include <Draw_Drawable3D.hxx>
#include <Draw_Interpretor.hxx>
#include <Draw_PluginMacro.hxx>
#include <DBRep_DrawableShape.hxx>
#include <Message.hxx>

//! Class defining static method 'Factory()' for 'DPLUGIN' macros.
class OcctDrawPlugin
{
public:
  DEFINE_STANDARD_ALLOC

  //! Add commands to Draw_Interpretor.
  static void Factory (Draw_Interpretor& theDI);
};

//! Command just printing "hello" message.
static int myhello (Draw_Interpretor& theDI,
                    int theNbArgs, const char** theArgVec)
{
  if (theNbArgs != 1)
  {
    theDI << "Syntax error - wrong number of arguments";
    return 1; // throw Tcl exception
  }

  // std::cout/std::cerr will appear in terminal,
  // but will be inaccessible to Tcl
  std::cout << "standard output\n";

  // first argument equals to command name, e.g. "myhello"
  std::cout << "command '" << theArgVec[0] << "'\n";

  // output to theDI will be accessible to Tcl
  theDI << "HELLO";
  return 0; // normal result
}

//! A dummy drawable object.
class MyDummyDrawable : public Draw_Drawable3D
{
  DEFINE_STANDARD_RTTI_INLINE(MyDummyDrawable, Draw_Drawable3D)
public:
  MyDummyDrawable() {}

  //! Draw object in axonometric viewer - not implemented.
  virtual void DrawOn (Draw_Display& theDisp) const override { (void )theDisp; }

  //! Variable dump.
  virtual void Dump (Standard_OStream& theStream) const override { theStream << "MyDummyDrawable dump"; }

  //! For whatis command.
  virtual void Whatis (Draw_Interpretor& theDI) const override { theDI << "MyDummyDrawable"; }
};

//! Command working with drawables.
static int mydrawable (Draw_Interpretor& theDI,
                       int theNbArgs, const char** theArgVec)
{
  Handle(Draw_Drawable3D) aDrawable;
  TCollection_AsciiString aName;
  for (int anArgIter = 1; anArgIter < theNbArgs; ++anArgIter)
  {
    TCollection_AsciiString anArg (theArgVec[anArgIter]);
    anArg.LowerCase();
    if (anArg == "-create"
     && aDrawable.IsNull())
    {
      aDrawable = new MyDummyDrawable();
    }
    else if (aName.IsEmpty())
    {
      aName = theArgVec[anArgIter];
    }
    else
    {
      theDI << "Syntax error at '" << theArgVec[anArgIter] << "'";
      return 1;
    }
  }
  if (aName.IsEmpty())
  {
    theDI << "Syntax error: wrong number of arguments";
    return 1;
  }

  if (aDrawable.IsNull())
  {
    // get existing drawable
    const char* aNameStr = aName.ToCString();
    aDrawable = Draw::Get (aNameStr);
    if (aDrawable.IsNull())
    {
      theDI << "Error: drawable '" << aName << "' not found";
      return 1;
    }
  }

  // print some basic info
  theDI << "DynamicType: " << aDrawable->DynamicType()->Name();
  Draw::Set (aName.ToCString(), aDrawable);

  // try handling subclasses
  if (Handle(MyDummyDrawable) aMyDraw = Handle(MyDummyDrawable)::DownCast (aDrawable))
  {
    theDI << "\nIt is my drawable!";
  }
  if (Handle(DBRep_DrawableShape) aDrawShape = Handle(DBRep_DrawableShape)::DownCast (aDrawable))
  {
    const TopoDS_Shape& aShape = aDrawShape->Shape();
    theDI << "\nShapeType: " << TopAbs::ShapeTypeToString (aShape.ShapeType());
  }

  return 0;
}

// Add commands to Draw_Interpretor.
void OcctDrawPlugin::Factory (Draw_Interpretor& theDI)
{
  // just some welcome message
  Message::SendInfo() << "Loading 'occt-draw-plugin'...\n"
    "Tip1: print 'help my*' to list commands defined by this plugin.\n"
    "Tip2: print 'testgrid' to run tests.";

  // register commands with description that could be retrieved via 'help CommandName'
  const char* aGroup = "My draw commands";
  theDI.Add ("myhello", "myhello - hello draw",
             __FILE__, myhello, aGroup);

  theDI.Add ("mydrawable", "mydrawable name [-create]",
             __FILE__, mydrawable, aGroup);
}

// Implement exported function that will be called by DRAWEXE on loading plugin
DPLUGIN(OcctDrawPlugin);
