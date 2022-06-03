#include <Draw.hxx>
#include <Draw_Interpretor.hxx>
#include <Draw_PluginMacro.hxx>
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
static int myhello (Draw_Interpretor& theDI, int , const char** )
{
  theDI << "HELLO from OcctDrawPlugin!";
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
}

// Implement exported function that will be called by DRAWEXE on loading plugin
DPLUGIN(OcctDrawPlugin);
