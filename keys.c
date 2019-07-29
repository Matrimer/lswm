void buttonpress(XEvent *e) {
    Monitor *m = NULL;
    Desktop *d = NULL;
    Client *c = NULL;

    Bool w = wintoclient(e->xbutton.window, &c, &d, &m);

    int cm = 0;
    while (m != &monitors[cm] && cm < nmonitors)
        ++cm;
    if (w && CLICK_TO_FOCUS && e->xbutton.button == FOCUS_BUTTON && (c != d->curr || cm != currmonidx)) {
        if (cm != currmonidx)
            change_monitor(&(Arg){.i = cm});
        focus(c, d, m);
    }

    for (unsigned int i = 0; i < LENGTH(buttons); i++)
        if (CLEANMASK(buttons[i].mask) == CLEANMASK(e->xbutton.state) &&
            buttons[i].func && buttons[i].button == e->xbutton.button) {
            if (w && cm != currmonidx)
                change_monitor(&(Arg){.i = cm});
            if (w && c != d->curr)
                focus(c, d, m);
           buttons[i].func(&(buttons[i].arg));
        }
}



/**
 * register button bindings to be notified of
 * when they occur.
 * the wm listens to those button bindings and
 * calls an appropriate handler when a binding
 * occurs (see buttonpress).
 */

void grabbuttons(Client *c) {
    Monitor *cm = &monitors[currmonidx];
    unsigned int b, m, modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };

    for (m = 0; CLICK_TO_FOCUS && m < LENGTH(modifiers); m++)
        if (c != cm->desktops[cm->currdeskidx].curr)
            XGrabButton(dis, FOCUS_BUTTON, modifiers[m],
                c->win, False, BUTTONMASK, GrabModeAsync, GrabModeAsync, None, None);
        else XUngrabButton(dis, FOCUS_BUTTON, modifiers[m], c->win);

    for (b = 0, m = 0; b < LENGTH(buttons); b++, m = 0)
        while (m < LENGTH(modifiers))
        XGrabButton(dis, buttons[b].button, buttons[b].mask|modifiers[m++], c->win,
                      False, BUTTONMASK, GrabModeAsync, GrabModeAsync, None, None);
}


/**
 * register key bindings to be notified of
 * when they occur.
 * the wm listens to those key bindings and
 * calls an appropriate handler when a binding
 * occurs (see keypressed).
 */

void grabkeys(void) {
    KeyCode code;
    XUngrabKey(dis, AnyKey, AnyModifier, root);
    unsigned int k, m, modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };

    for (k = 0, m = 0; k < LENGTH(keys); k++, m = 0)
        while ((code = XKeysymToKeycode(dis, keys[k].keysym)) && m < LENGTH(modifiers))
            XGrabKey(dis, code, keys[k].mod|modifiers[m++], root, True, GrabModeAsync, GrabModeAsync);
}


/**
 * on the press of a key binding (see grabkeys)
 * call the appropriate handler
 */

void keypress(XEvent *e) {
    KeySym keysym = XkbKeycodeToKeysym(dis, e->xkey.keycode, 0, 0);
    for (unsigned int i = 0; i < LENGTH(keys); i++)
        if (keysym == keys[i].keysym && CLEANMASK(keys[i].mod) == CLEANMASK(e->xkey.state))
            if (keys[i].func)
                keys[i].func(&keys[i].arg);
}
