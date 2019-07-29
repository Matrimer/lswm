/**
 * monocle aka max aka fullscreen mode/layout
 * each window should cover all the available screen space
 */
void monocle(int x, int y, int w, int h, const Desktop *d) {
    for (Client *c = d->head; c; c = c->next)
        if (!ISFFT(c))
            XMoveResizeWindow(dis, c->win, x, y, w, h);
}
