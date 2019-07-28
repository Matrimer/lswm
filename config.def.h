/* see LICENSE for copyright and license */

#ifndef CONFIG_H
#define CONFIG_H

/** modifiers **/
#define MOD1            Mod1Mask    /* ALT key */
#define MOD4            Mod4Mask    /* Super/Windows key */
#define CONTROL         ControlMask /* Control key */
#define SHIFT           ShiftMask   /* Shift key */

/** generic settings **/
#define MASTER_SIZE     0.52
#define SHOW_PANEL      True      /* show panel by default on exec */
#define TOP_PANEL       True      /* False means panel is on bottom */
#define PANEL_HEIGHT    18        /* 0 for no space for panel, thus no panel */
#define DEFAULT_MODE    TILE      /* initial layout/mode: TILE MONOCLE BSTACK FLOAT */
#define ATTACH_ASIDE    True      /* False means new window is master */
#define FOLLOW_WINDOW   False     /* follow the window when moved to a different desktop */
#define FOLLOW_MONITOR  False     /* follow the window when moved to a different monitor */
#define FOLLOW_MOUSE    False     /* focus the window the mouse just entered */
#define CLICK_TO_FOCUS  True      /* focus an unfocused window when clicked  */
#define FOCUS_BUTTON    Button3   /* mouse button to be used along with CLICK_TO_FOCUS */
#define BORDER_WIDTH    3         /* window border width */
#define FOCUS           "#ff950e" /* focused window border color   */
#define UNFOCUS         "#444444" /* unfocused window border color */
#define INFOCUS         "#9c3885" /* focused window border color on unfocused monitor */
#define MINWSZ          50        /* minimum window size in pixels */
#define DEFAULT_MONITOR 0         /* the monitor to focus initially */
#define DEFAULT_DESKTOP 0         /* the desktop to focus initially */
#define DESKTOPS        9         /* number of desktops - edit DESKTOPCHANGE keys to suit */

struct ml {
    int m; /* monitor that the desktop in on  */
    int d; /* desktop which properties follow */
    struct {
        int mode;  /* layout mode for desktop d of monitor m    */
        int masz;  /* incread or decrease master area in px     */
        Bool sbar; /* whether or not to show panel on desktop d */
    } dl;
};

/**
 * define initial values for each monitor and dekstop properties
 *
 * in the example below:
 * - the first desktop (0) on the first monitor (0) will have
 *   tile layout, with its master area increased by 50px and
 *   the panel will be visible.
 * - the third desktop (2) on the second monitor (1) will have
 *   grid layout, with no changes to its master area and
 *   the panel will be hidden.
 */
static const struct ml init[] = { \
    /* monitor  desktop   mode  masz  sbar   */
    {     0,       0,   { TILE,  50,  True  } },
};

/**
 * open applications to specified monitor and desktop
 * with the specified properties.
 * if monitor is negative, then current is assumed
 * if desktop is negative, then current is assumed
 */
static const AppRule rules[] = { \
    /*  class     monitor  desktop  follow  float */
    { "MPlayer",     0,       3,    True,   False },
    { "Gimp",        1,       0,    False,  True  },
};

/* helper for spawning shell commands */
#define SHCMD(cmd) {.com = (const char*[]){"/bin/sh", "-c", cmd, NULL}}

/**
 * custom commands
 * must always end with ', NULL };'
 */
static const char *termcmd[] = { "xterm", NULL };
static const char *dmenucmd[] = { "dmenu_run", NULL };


#define MONITORCHANGE(K,N) \
    {  MOD4,             K,              change_monitor, {.i = N}}, \
    {  MOD4|ShiftMask,   K,              client_to_monitor, {.i = N}},

#define DESKTOPCHANGE(K,N) \
    {  MOD1,             K,              change_desktop, {.i = N}}, \
    {  MOD1|ShiftMask,   K,              client_to_desktop, {.i = N}},

/**
 * keyboard shortcuts
 */
static Key keys[] = {
    /* modifier          key            function           argument */
    {  MOD1,             XK_b,          togglepanel,       {NULL}},
    {  MOD1,             XK_BackSpace,  focusurgent,       {NULL}},
    {  MOD1,             XK_q,          killclient,        {NULL}},
    {  MOD1,             XK_n,          next_win,          {NULL}},
    {  MOD1,             XK_o,          prev_win,          {NULL}},
    {  MOD1,             XK_h,          resize_master,     {.i = -5}}, /* decrease size in px */
    {  MOD1,             XK_l,          resize_master,     {.i = +5}}, /* increase size in px */
    {  MOD1,             XK_o,          resize_stack,      {.i = -5}}, /* shrink   size in px */
    {  MOD1,             XK_g,          resize_stack,      {.i = +5}}, /* grow     size in px */
    {  MOD1|CONTROL,     XK_e,          rotate,            {.i = -1}},
    {  MOD1|CONTROL,     XK_i,          rotate,            {.i = +1}},
    {  MOD1|SHIFT,       XK_e,          rotate_filled,     {.i = -1}},
    {  MOD1|SHIFT,       XK_i,          rotate_filled,     {.i = +1}},
    {  MOD1,             XK_Tab,        last_desktop,      {NULL}},
    {  MOD1,             XK_Return,     swap_master,       {NULL}},
    {  MOD1|SHIFT,       XK_n,          move_down,         {NULL}},
    {  MOD1|SHIFT,       XK_o,          move_up,           {NULL}},
    {  MOD1|SHIFT,       XK_t,          switch_mode,       {.i = TILE}},
    {  MOD1|SHIFT,       XK_m,          switch_mode,       {.i = MONOCLE}},
    {  MOD1|SHIFT,       XK_b,          switch_mode,       {.i = BSTACK}},
    {  MOD1|SHIFT,       XK_f,          switch_mode,       {.i = FLOAT}},
    {  MOD1|CONTROL,     XK_q,          quit,              {.i = 0}}, /* quit with exit value 0 */
    {  MOD1|CONTROL,     XK_r,          quit,              {.i = 1}}, /* quit with exit value 1 */
    {  MOD1,             XK_space,      spawn,             {.com = termcmd}},
    {  MOD1,             XK_p,          spawn,             {.com = dmenucmd}},
    {  MOD4,             XK_n,          moveresize,        {.v = (int []){   0,  25,   0,   0 }}}, /* move down  */
    {  MOD4,             XK_o,          moveresize,        {.v = (int []){   0, -25,   0,   0 }}}, /* move up    */
    {  MOD4,             XK_i,          moveresize,        {.v = (int []){  25,   0,   0,   0 }}}, /* move right */
    {  MOD4,             XK_e,          moveresize,        {.v = (int []){ -25,   0,   0,   0 }}}, /* move left  */
    {  MOD4|SHIFT,       XK_n,          moveresize,        {.v = (int []){   0,   0,   0,  25 }}}, /* height grow   */
    {  MOD4|SHIFT,       XK_o,          moveresize,        {.v = (int []){   0,   0,   0, -25 }}}, /* height shrink */
    {  MOD4|SHIFT,       XK_i,          moveresize,        {.v = (int []){   0,   0,  25,   0 }}}, /* width grow    */
    {  MOD4|SHIFT,       XK_e,          moveresize,        {.v = (int []){   0,   0, -25,   0 }}}, /* width shrink  */
       DESKTOPCHANGE(    XK_F1,                             0)
       DESKTOPCHANGE(    XK_F2,                             1)
       DESKTOPCHANGE(    XK_F3,                             2)
       DESKTOPCHANGE(    XK_F4,                             3)
       DESKTOPCHANGE(    XK_F5,                             4)
       DESKTOPCHANGE(    XK_F6,                             5)
       DESKTOPCHANGE(    XK_F7,                             6)
       DESKTOPCHANGE(    XK_F8,                             7)
       DESKTOPCHANGE(    XK_F9,                             8)
       MONITORCHANGE(    XK_F1,                             0)
       MONITORCHANGE(    XK_F2,                             1)
};

/**
 * mouse shortcuts
 */
static Button buttons[] = {
    {  MOD1,    Button1,     mousemotion,   {.i = MOVE}},
    {  MOD1,    Button3,     mousemotion,   {.i = RESIZE}},
};
#endif

/* vim: set expandtab ts=4 sts=4 sw=4 : */
