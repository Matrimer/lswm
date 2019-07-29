/**
 * move the current focused client to another desktop
 *
 * add the current client as the last on the new desktop
 * then remove it from the current desktop
 */
void client_to_desktop(const Arg *arg) {
    Monitor *m = &monitors[currmonidx];
    Desktop *d = &m->desktops[m->currdeskidx], *n = NULL;
    if (arg->i == m->currdeskidx || arg->i < 0 || arg->i >= DESKTOPS || !d->curr)
        return;

    Client *c = d->curr, *p = prevclient(d->curr, d),
           *l = prevclient(m->desktops[arg->i].head, (n = &m->desktops[arg->i]));

    /* unlink current client from current desktop */
    if (d->head == c || !p)
        d->head = c->next;
    else p->next = c->next;

    c->next = NULL;
    XChangeWindowAttributes(dis, root, CWEventMask, &(XSetWindowAttributes){.do_not_propagate_mask = SubstructureNotifyMask});

    if (XUnmapWindow(dis, c->win))
        focus(d->prev, d, m);

    XChangeWindowAttributes(dis, root, CWEventMask, &(XSetWindowAttributes){.event_mask = ROOTMASK});

    if (!(c->isfloat || c->istrans) || (d->head && !d->head->next))
        tile(d, m);

    /* link client to new desktop and make it the current */
    focus(l ? (l->next = c):n->head ? (n->head->next = c):(n->head = c), n, m);

    /* I may want to edit these 3 lines */
    if (FOLLOW_WINDOW)
        change_desktop(arg);
    else desktopinfo();
}

/**
 * move the current focused client to another monitor
 *
 * add the current client as the last on the new monitor's current desktop
 * then remove it from the current monitor's current desktop
 *
 * removing the client means unlinking it and unmapping it.
 * add the client means linking it as the last client, and
 * mapping it. mapping must happen after the client has been
 * unmapped from the current monitor's current desktop.
 */
void client_to_monitor(const Arg *arg) {
    Monitor *cm = &monitors[currmonidx], *nm = NULL;
    Desktop *cd = &cm->desktops[cm->currdeskidx], *nd = NULL;
    if (arg->i == currmonidx || arg->i < 0 || arg->i >= nmonitors || !cd->curr)
        return;

    nd = &monitors[arg->i].desktops[(nm = &monitors[arg->i])->currdeskidx];
    Client *c = cd->curr, *p = prevclient(c, cd), *l = prevclient(nd->head, nd);

    /* unlink current client from current monitor's current desktop */ /* Note, Layouts here aswell */
    if (cd->head == c || !p)
        cd->head = c->next;
    else p->next = c->next;
    c->next = NULL;
    focus(cd->prev, cd, cm);
    if (!(c->isfloat || c->istrans) || (cd->head && !cd->head->next))
        tile(cd, cm);

    /* reset floating and fullscreen state */
    if (ISFFT(c)) c->isfloat = c->isfull = False;

    /* link to new monitor's current desktop */
    focus(l ? (l->next = c):nd->head ? (nd->head->next = c):(nd->head = c), nd, nm);
    tile(nd, nm);

    if (FOLLOW_MONITOR)
        change_monitor(arg);
    else desktopinfo();
}


/**
 * receive and process client messages
 *
 * check if window wants to change its state to fullscreen,
 * or if the window want to become active/focused
 *
 * to change the state of a mapped window, a client MUST
 * send a _NET_WM_STATE client message to the root window
 * message_type must be _NET_WM_STATE
 *   data.l[0] is the action to be taken
 *   data.l[1] is the property to alter three actions:
 *   - remove/unset _NET_WM_STATE_REMOVE=0
 *   - add/set _NET_WM_STATE_ADD=1,
 *   - toggle _NET_WM_STATE_TOGGLE=2
 *
 * to request to become active, a client should send a
 * message of _NET_ACTIVE_WINDOW type. when such a message
 * is received and a client holding that window exists,
 * the window becomes the current active focused window
 * on its desktop.
 */
void clientmessage(XEvent *e) {
    Monitor *m = NULL;
    Desktop *d = NULL;
    Client *c = NULL;
    if (!wintoclient(e->xclient.window, &c, &d, &m))
        return;

    if (e->xclient.message_type        == netatoms[NET_WM_STATE] && (
        (unsigned)e->xclient.data.l[1] == netatoms[NET_FULLSCREEN]
     || (unsigned)e->xclient.data.l[2] == netatoms[NET_FULLSCREEN])) {
        setfullscreen(c, d, m, (e->xclient.data.l[0] == 1 || (e->xclient.data.l[0] == 2 && !c->isfull)));
        if (!(c->isfloat || c->istrans) || !d->head->next)
            tile(d, m);
    }
    else if (e->xclient.message_type == netatoms[NET_ACTIVE])
        focus(c, d, m);
}

/**
 * configure a window's size, position, border width, and stacking order.
 *
 * windows usually have a prefered size (width, height) and position (x, y),
 * and sometimes borer with (border_width) and stacking order (above, detail).
 * a configure request attempts to reconfigure those properties for a window.
 *
 * we don't really care about those values, because a tiling wm will impose
 * its own values for those properties.
 * however the requested values must be set initially for some windows,
 * otherwise the window will misbehave or even crash (see gedit, geany, gvim).
 *
 * some windows depend on the number of columns and rows to set their
 * size, and not on pixels (terminals, consoles, some editors etc).
 * normally those clients when tiled and respecting the prefered size
 * will create gaps around them (window_hints).
 * however, clients are tiled to match the wm's prefered size,
 * not respecting those prefered values.
 *
 * some windows implement window manager functions themselves.
 * that is windows explicitly steal focus, or manage subwindows,
 * or move windows around w/o the window manager's help, etc..
 * to disallow this behavior, we 'tile()' the desktop to which
 * the window that sent the configure request belongs.
 */
void configurerequest(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc = { ev->x, ev->y,  ev->width, ev->height, ev->border_width, ev->above, ev->detail };
    if (XConfigureWindow(dis, ev->window, ev->value_mask, &wc))
        XSync(dis, False);

    Monitor *m = NULL;
    Desktop *d = NULL;
    Client *c = NULL;

    if (wintoclient(ev->window, &c, &d, &m)) tile(d, m);
}


/**
 * clients receiving a WM_DELETE_WINDOW message should behave as if
 * the user selected "delete window" from a hypothetical menu and
 * also perform any confirmation dialog with the user.
 */
void deletewindow(Window w) {
    XEvent ev = { .type = ClientMessage };
    ev.xclient.window = w;
    ev.xclient.format = 32;
    ev.xclient.message_type = wmatoms[WM_PROTOCOLS];
    ev.xclient.data.l[0]    = wmatoms[WM_DELETE_WINDOW];
    ev.xclient.data.l[1]    = CurrentTime;
    XSendEvent(dis, w, False, NoEventMask, &ev);
}


/**
 * generated whenever a client application destroys a window
 *
 * a destroy notification is received when a window is being closed
 * on receival, remove the client that held that window
 */
void destroynotify(XEvent *e) {
    Monitor *m = NULL;
    Desktop *d = NULL;
    Client *c = NULL;
    if (wintoclient(e->xdestroywindow.window, &c, &d, &m))
        removeclient(c, d, m);
}

/**
 * when the mouse enters a window's borders, that window,
 * if has set notifications of such events (EnterWindowMask)
 * will notify that the pointer entered its region
 * and will get focus if FOLLOW_MOUSE is set in the config.
 */
void enternotify(XEvent *e) {
    Monitor *m = NULL; Desktop *d = NULL; Client *c = NULL, *p = NULL;

    if (!FOLLOW_MOUSE || (e->xcrossing.mode != NotifyNormal && e->xcrossing.detail == NotifyInferior)
        || !wintoclient(e->xcrossing.window, &c, &d, &m) || e->xcrossing.window == d->curr->win)
            return;

    if (m != &monitors[currmonidx])
        for (int cm = 0; cm < nmonitors; cm++)
            if (m == &monitors[cm]) change_monitor(&(Arg){.i = cm});

    if ((p = d->prev))
        XChangeWindowAttributes(dis, p->win, CWEventMask, &(XSetWindowAttributes){.do_not_propagate_mask = EnterWindowMask});
    focus(c, d, m);
    if (p) XChangeWindowAttributes(dis, p->win, CWEventMask, &(XSetWindowAttributes){.event_mask = EnterWindowMask});
}
