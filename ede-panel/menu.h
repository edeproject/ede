#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

static const char default_menu[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n\
<Menu>\n\
<!--\n\
  <Item Type=\"Exec\" Icon=\"~/xfte.png\" Exec=\"xfte\">\n\
    <Name>xFTE editor</Name>\n\
    <Name Lang=\"fi\">xFTE editori</Name>\n\
  </Item>\n\
  <Item Type=\"Divider\"/>\n\
-->\n\
  <Item Type=\"AppDir\" Dir=\"$DEFAULT_PROGRAMS_DIR\">\n\
    <Name>Programs</Name>\n\
  </Item>\n\
  <Item Type=\"AppDir\" Dir=\"~/.ede/programs\">\n\
    <Name>User Programs</Name>\n\
  </Item>\n\
  <Item Type=\"Divider\"/>\n\
    <Item Type=\"AppDir\" Dir=\"~/.ede/favourites\">\n\
    <Name>Favourites</Name>\n\
  </Item>\n\
  <Item Type=\"Exec\" Icon=\"efinder.png\" Exec=\"efinder\">\n\
    <Name>Find</Name>\n\
  </Item>\n\
  <Item Type=\"Exec\" Icon=\"ehelpbook.png\" Exec=\"ede-help\">\n\
    <Name>Help</Name>\n\
  </Item>\n\
  <Item Type=\"Exec\" Icon=\"about.png\" Exec=\"$ABOUT\">\n\
    <Name>About</Name>\n\
  </Item>\n\
  <Item Type=\"Exec\" Icon=\"erun.png\" Exec=\"ede-launch\">\n\
    <Name>Run</Name>\n\
  </Item>\n\
  <Item Type=\"Divider\"/>\n\
  <Item Type=\"FileBrowser\" Dir=\"/\">\n\
  	<Name>Quick Browser</Name>\n\
  </Item>\n\
  <Item Type=\"SubDir\">\n\
    <Name>Panel</Name>\n\
    <Item Type=\"Exec\" Icon=\"econtrol.png\" Exec=\"econtrol\">\n\
     <Name>Control Panel</Name>\n\
    </Item>\n\
    <Item Type=\"Exec\" Icon=\"emenueditor.png\" Exec=\"emenueditor\">\n\
     <Name>Menu Editor</Name>\n\
    </Item>\n\
    <Item Type=\"Exec\" Icon=\"synaptic.png\" Exec=\"einstaller\">\n\
     <Name>Install New Software</Name>\n\
    </Item>\n\
  </Item>\n\
  <Item Type=\"Divider\"/>\n\
  <Item Type=\"Exec\" Icon=\"lock.png\" Exec=\"xlock\">\n\
    <Name>Lock</Name>\n\
  </Item>\n\
  <Item Type=\"Exec\" Icon=\"logout.png\" Exec=\"ede-shutdown\">\n\
    <Name>Logout</Name>\n\
  </Item>\n\
</Menu>\n";
