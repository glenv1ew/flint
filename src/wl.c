#include "wl.h"

#include <stdio.h>
#include <string.h>

#include "xdg-shell-client-protocol.h"

// registry_global fires once for every global
// object the compositor advertises. Flint only
// needs the compositor and the xdg_wm_base
// shell, so every other interface is ignored.
static void registry_global(void* data,
                             struct wl_registry* registry,
                             uint32_t name,
                             const char* interface,
                             uint32_t version) {
    (void)version;
    struct Wl* wl = data;

    if (strcmp(interface, wl_compositor_interface.name) ==
        0) {
        wl->compositor = wl_registry_bind(
            registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface,
                       xdg_wm_base_interface.name) == 0) {
        wl->wm_base = wl_registry_bind(
            registry, name, &xdg_wm_base_interface, 1);
    }
}

// registry_global_remove fires when a global
// goes away. Nothing in Flint reacts to that
// yet, but the listener struct requires it.
static void registry_global_remove(
    void* data, struct wl_registry* registry,
    uint32_t name) {
    (void)data;
    (void)registry;
    (void)name;
}

static const struct wl_registry_listener
    registry_listener = {
        .global = registry_global,
        .global_remove = registry_global_remove,
};

bool wl_init(struct Wl* wl) {
    memset(wl, 0, sizeof(*wl));

    // Connect to the compositor named by
    // WAYLAND_DISPLAY, or the default socket
    // when unset.
    wl->display = wl_display_connect(NULL);
    if (!wl->display) {
        fprintf(stderr, "wl_display_connect failed\n");
        return false;
    }

    wl->registry = wl_display_get_registry(wl->display);
    if (!wl->registry) {
        fprintf(stderr,
                "wl_display_get_registry failed\n");
        return false;
    }

    wl_registry_add_listener(wl->registry,
                              &registry_listener, wl);

    // Round-trip so the global announcements
    // handled above are dispatched before we
    // check what got bound.
    if (wl_display_roundtrip(wl->display) < 0) {
        fprintf(stderr, "wl_display_roundtrip failed\n");
        return false;
    }

    if (!wl->compositor) {
        fprintf(stderr,
                "wl_compositor global not found\n");
        return false;
    }
    if (!wl->wm_base) {
        fprintf(stderr,
                "xdg_wm_base global not found\n");
        return false;
    }

    wl->surface =
        wl_compositor_create_surface(wl->compositor);
    if (!wl->surface) {
        fprintf(stderr,
                "wl_compositor_create_surface failed\n");
        return false;
    }

    return true;
}

void wl_finish(struct Wl* wl) {
    // Reverse order of creation: surface, then
    // the two bound globals, then the registry,
    // then the connection itself.
    if (wl->surface) {
        wl_surface_destroy(wl->surface);
        wl->surface = NULL;
    }
    if (wl->wm_base) {
        xdg_wm_base_destroy(wl->wm_base);
        wl->wm_base = NULL;
    }
    if (wl->compositor) {
        wl_compositor_destroy(wl->compositor);
        wl->compositor = NULL;
    }
    if (wl->registry) {
        wl_registry_destroy(wl->registry);
        wl->registry = NULL;
    }
    if (wl->display) {
        wl_display_disconnect(wl->display);
        wl->display = NULL;
    }
}