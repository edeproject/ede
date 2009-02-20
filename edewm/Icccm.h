#ifndef _ICCCM_H_
#define _ICCCM_H_

class Frame;

class ICCCM
{
public:
    static void state_iconic(Frame *f);
    static void state_normal(Frame *f);
    static void state_withdrawn(Frame *f);

    static void configure(Frame *f);
    static void set_iconsizes();

    static bool get_size(Frame *f, int &w, int &h);
    static char *get_title(Frame *f);

    static bool size_hints(Frame *f);
};

#endif

