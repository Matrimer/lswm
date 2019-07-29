
/**
 * tile or common tiling aka v-stack mode/layout
 * bstack or bottom stack aka h-stack mode/layout
 */
void stack(int x, int y, int w, int h, const Desktop *d) {
    Client *c = NULL, *t = NULL; Bool b = (d->mode == BSTACK);
    int n = 0, p = 0, z = (b ? w:h), ma = (b ? h:w) * MASTER_SIZE + d->masz;

    /* count stack windows and grab first non-floating, non-fullscreen window */
    for (t = d->head; t; t = t->next)
        if (!ISFFT(t)) {
            if (c)
                ++n;
            else c = t;
        }

    /* if there is only one window (c && !n), it should cover the available screen space
     * if there is only one stack window, then we don't care about growth
     * if more than one stack windows (n > 1) adjustments may be needed.
     *
     *   - p is the num of pixels than remain when spliting the
     *       available width/height to the number of windows
     *   - z is each client's height/width
     *
     *      ----------  --.    ----------------------.
     *      |   |----| }--|--> sasz                  }--> first client will have
     *      |   | 1s |    |                          |    z+p+sasz height/width.
     *      | M |----|-.  }--> screen height (h)  ---'
     *      |   | 2s | }--|--> client height (z)    two stack clients on tile mode
     *      -----------' -'                         ::: ascii art by c00kiemon5ter
     *
     * what we do is, remove the sasz from the screen height/width and then
     * divide that space with the windows on the stack so all windows have
     * equal height/width: z = (z - sasz)/n
     *
     * sasz was left out (subtrackted), to later be added to the first client
     * height/width. before we do that, there will be cases when the num of
     * windows cannot be perfectly divided with the available screen height/width.
     * for example: 100px scr. height, and 3 stack windows: 100/3 = 33,3333..
     * so we get that remaining space and merge it to sasz: p = (z - sasz) % n + sasz
     *
     * in the end, we know each client's height/width (z), and how many pixels
     * should be added to the first stack client (p) so that it satisfies sasz,
     * and also, does not result in gaps created on the bottom of the screen.
     */
    if (c && !n)
        XMoveResizeWindow(dis, c->win, x, y, w - 2*BORDER_WIDTH, h - 2*BORDER_WIDTH);
    if (!c || !n)
        return;
    else if (n > 1) {
        p = (z - d->sasz)%n + d->sasz;
        z = (z - d->sasz)/n;
    }

    /* tile the first non-floating, non-fullscreen window to cover the master area */
    if (b) XMoveResizeWindow(dis, c->win, x, y, w - 2*BORDER_WIDTH, ma - BORDER_WIDTH);
    else   XMoveResizeWindow(dis, c->win, x, y, ma - BORDER_WIDTH, h - 2*BORDER_WIDTH);

    /* tile the next non-floating, non-fullscreen (and first) stack window adding p */
    for (c = c->next; c && ISFFT(c); c = c->next)
        ;
    int cw = (b ? h:w) - 2*BORDER_WIDTH - ma, ch = z - BORDER_WIDTH;
    if (b)
        XMoveResizeWindow(dis, c->win, x, y += ma, ch - BORDER_WIDTH + p, cw);
    else XMoveResizeWindow(dis, c->win, x += ma, y, cw, ch - BORDER_WIDTH + p);

    /* tile the rest of the non-floating, non-fullscreen stack windows */
    for (b ? (x += ch+p):(y += ch+p), c = c->next; c; c = c->next) {
        if (ISFFT(c))
            continue;
        if (b) {
            XMoveResizeWindow(dis, c->win, x, y, ch, cw); x += z;
        }
        else {
            XMoveResizeWindow(dis, c->win, x, y, cw, ch);
            y += z;
        }
    }
}


