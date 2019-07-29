/*
 * focus another desktop
 *
 * to avoid flickering (esp. monocle mode):
 * first map the new windows
 * first the current window and then all other
 * then unmap the old windows
 * first all others then the current
 */
void change_desktop(const Arg *arg) {
    Monitor *m = &monitors[currmonidx];
    if (arg->i == m->currdeskidx || arg->i < 0 || arg->i >= DESKTOPS)
        return;
    Desktop *d = &m->desktops[(m->prevdeskidx = m->currdeskidx)], *n = &m->desktops[(m->currdeskidx = arg->i)];
    if (n->curr)
        XMapWindow(dis, n->curr->win);
    for (Client *c = n->head; c; c = c->next)
        XMapWindow(dis, c->win);
    XChangeWindowAttributes(dis, root, CWEventMask, &(XSetWindowAttributes){.do_not_propagate_mask = SubstructureNotifyMask});
    for (Client *c = d->head; c; c = c->next)
        if (c != d->curr)
            XUnmapWindow(dis, c->win);
    if (d->curr)
        XUnmapWindow(dis, d->curr->win);
    XChangeWindowAttributes(dis, root, CWEventMask, &(XSetWindowAttributes){.event_mask = ROOTMASK});
    if (n->head)
        { tile(n, m); focus(n->curr, n, m); }
    desktopinfo();
}

/**
 * focus another monitor
 */
void change_monitor(const Arg *arg) {
    if (arg->i == currmonidx || arg->i < 0 || arg->i >= nmonitors)
        return;
    Monitor *m = &monitors[currmonidx], *n = &monitors[(currmonidx = arg->i)];
    focus(m->desktops[m->currdeskidx].curr, &m->desktops[m->currdeskidx], m);
    focus(n->desktops[n->currdeskidx].curr, &n->desktops[n->currdeskidx], n);
    desktopinfo();
}


/**
 * 1. set current/active/focused and previously focused client
 *    in other words, manage curr and prev references
 * 2. restack clients
 * 3. highlight borders and set active window property
 * 4. give input focus to the current/active/focused client
 */


void focus(Client *c, Desktop *d, Monitor *m) {
    /* update references to prev and curr,
     * previously focused and currently focused clients.
     *
     * if there are no clients (!head) or the new client
     * is NULL, then delete the _NET_ACTIVE_WINDOW property
     *
     * if the new client is the prev client then
     *  - either the current client was removed
     *    and thus focus(prev) was called
     *  - or the previous from current is prev
     *    ie, two consecutive clients were focused
     *    and then prev_win() was called, to focus
     *    the previous from current client, which
     *    happens to be prev (curr == c->next).
     * (below: h:head p:prev c:curr)
     *
     * [h]->[p]->[c]->NULL   ===>   [h|p]->[c]->NULL
     *            ^ remove current
     *
     * [h]->[p]->[c]->NULL   ===>   [h]->[c]->[p]->NULL
     *       ^ prev_win swaps prev and curr
     *
     * in the first case we need to update prev reference,
     * choice here is to set it to the previous from the
     * new current client.
     * the second case is handled as any other case, the
     * current client is now the previously focused (prev = curr)
     * and the new current client is now curr (curr = c)
     *
     * references should only change when the current
     * client is different from the one given to focus.
     *
     * the new client should never be NULL, except if,
     * there is no other client on the workspace (!head).
     * prev and curr always point to different clients.
     *
     * NOTICE: remove client can remove any client,
     * not just the current (curr). Thus, if prev is
     * removed, its reference needs to be updated.
     * That is handled by removeclient() function.
     * All other reference changes for curr and prev
     * should and are handled here.
     */
    if (!d->head || !c) { /* no clients - no active window - nothing to do */
        XDeleteProperty(dis, root, netatoms[NET_ACTIVE]);
        d->curr = d->prev = NULL;
        return;
    }
    else if (d->prev == c && d->curr != c->next) {
        d->prev = prevclient((d->curr = c), d);
    }
    else if (d->curr != c) {
        d->prev = d->curr;
        d->curr = c;
    }

    /* restack clients
     *
     * stack order is based on client properties.
     * from top to bottom:
     *  - current when floating or transient
     *  - floating or trancient windows
     *  - current when tiled
     *  - current when fullscreen
     *  - fullscreen windows
     *  - tiled windows
     *
     * num of n:all fl:fullscreen ft:floating/transient windows
     */
    int n = 0, fl = 0, ft = 0;
    for (c = d->head; c; c = c->next, ++n)
        if (ISFFT(c)) {
            fl++;
            if (!c->isfull)
                ft++;
        }
    Window w[n];
    w[(d->curr->isfloat || d->curr->istrans) ? 0:ft] = d->curr->win;
    for (fl += !ISFFT(d->curr) ? 1:0, c = d->head; c; c = c->next) {
        XSetWindowBorder(dis, c->win, (c != d->curr) ? win_unfocus:(m == &monitors[currmonidx]) ? win_focus:win_infocus);
        /*
         * a window should have borders in any case, except if
         *  - the window is fullscreen
         *  - the window is not floating or transient and
         *      - the mode is MONOCLE or,
         *      - it is the only window on screen
         */
        XSetWindowBorderWidth(dis, c->win, c->isfull || (!ISFFT(c) &&
            (d->mode == MONOCLE || !d->head->next)) ? 0:BORDER_WIDTH);
        if (c != d->curr)
            w[c->isfull ? --fl:ISFFT(c) ? --ft:--n] = c->win;
        if (CLICK_TO_FOCUS || c == d->curr)
            grabbuttons(c);
    }
    XRestackWindows(dis, w, LENGTH(w));

    XSetInputFocus(dis, d->curr->win, RevertToPointerRoot, CurrentTime);
    XChangeProperty(dis, root, netatoms[NET_ACTIVE], XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&d->curr->win, 1);

    XSync(dis, False);
}



/**
 * dont give focus to any client except current.
 * some apps explicitly call XSetInputFocus (see
 * tabbed, chromium), resulting in loss of input
 * focuse (mouse/kbd) from the current focused
 * client.
 *
 * this gives focus back to the current selected
 * client, by the user, through the wm.
 */
void focusin(XEvent *e) {
    Monitor *m = &monitors[currmonidx];
    Desktop *d = &m->desktops[m->currdeskidx];
    if (d->curr && d->curr->win != e->xfocus.window)
        focus(d->curr, d, m);
}


/**
 * find and focus the first client that received an urgent hint
 * first look in the current desktop then on other desktops
 */
void focusurgent(void) {
    Monitor *m = &monitors[currmonidx];
    Client *c = NULL;
    int d = -1;
    for (c = m->desktops[m->currdeskidx].head; c && !c->isurgn; c = c->next);
    while (!c && d < DESKTOPS-1)
        for (c = m->desktops[++d].head; c && !c->isurgn; c = c->next);
    if (c) {
        if (d != -1)
            change_desktop(&(Arg){.i = d});
        focus(c, &m->desktops[m->currdeskidx], m);
    }
}


/**
 * focus the previously focused desktop
 */
void last_desktop(void) {
    change_desktop(&(Arg){.i = monitors[currmonidx].prevdeskidx});
}


