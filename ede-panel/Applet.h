#ifndef __APPLET_H__
#define __APPLET_H__

#define EDE_PANEL_APPLET_INTERFACE_VERSION 0x01

class Fl_Widget;

enum {
	EDE_PANEL_APPLET_OPTION_RESIZABLE_H = (1 << 1),
	EDE_PANEL_APPLET_OPTION_RESIZABLE_V = (1 << 2),
	EDE_PANEL_APPLET_OPTION_ALIGN_LEFT  = (1 << 3),
	EDE_PANEL_APPLET_OPTION_ALIGN_RIGHT = (1 << 4)
};

struct AppletInfo {
	const char    *name;
	const char    *klass_name;
	const char    *version;
	const char    *icon;
	const char    *author;
	unsigned long  options;
};

typedef Fl_Widget*  (*applet_create_t)(void);
typedef void        (*applet_destroy_t)(Fl_Widget *);

typedef AppletInfo* (*applet_get_info_t)(void);
typedef void        (*applet_destroy_info_t)(AppletInfo *a);

typedef float       (*applet_version_t)(void);

/* the main macro each applet library must implement */

#define EDE_PANEL_APPLET_EXPORT(klass, aoptions, aname, aversion, aicon, aauthor) \
extern "C" Fl_Widget *ede_panel_applet_create(void) {                             \
	return new klass;                                                             \
}                                                                                 \
                                                                                  \
extern "C" void ede_panel_applet_destroy(Fl_Widget *w) {                          \
	klass *k = (klass*)w;                                                         \
	delete k;                                                                     \
}                                                                                 \
                                                                                  \
extern "C" AppletInfo *ede_panel_applet_get_info(void) {                          \
	AppletInfo *a = new AppletInfo;                                               \
	a->name = aname;                                                              \
	a->klass_name = #klass;                                                       \
	a->version = aversion;                                                        \
	a->icon = aicon;                                                              \
	a->author = aauthor;                                                          \
	a->options = aoptions;                                                        \
	return a;                                                                     \
}                                                                                 \
                                                                                  \
extern "C" void ede_panel_applet_destroy_info(AppletInfo *a) {                    \
	delete a;                                                                     \
}                                                                                 \
                                                                                  \
extern "C" int ede_panel_applet_get_iface_version(void) {                         \
	return EDE_PANEL_APPLET_INTERFACE_VERSION;                                    \
}

#endif
