#ifndef __XDGMENUREADER_H__
#define __XDGMENUREADER_H__

#include <edelib/MenuItem.h>

EDELIB_NS_USING(MenuItem)

void xdg_menu_dump_for_test_suite(void);

MenuItem *xdg_menu_load(void);
void      xdg_menu_delete(MenuItem *it);

#endif
