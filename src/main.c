#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <wayland-server.h>

#include "compositor.h"
#include "config.h"

static int
on_term_signal(int signal_number, void* data)
{
  struct wl_display* wl_display = data;

  fprintf(stderr, "caught signal %d\n", signal_number);
  wl_display_terminate(wl_display);

  return 1;
}

int
main()
{
  struct zippo_compositor* compositor;
  struct zippo_backend* backend;
  struct wl_display* wl_display;
  struct wl_event_loop* loop;
  struct wl_event_source* signals[3];

  int status = EXIT_FAILURE;

  fprintf(stderr, "zippo %s\n", VERSION);

  wl_display = wl_display_create();

  loop = wl_display_get_event_loop(wl_display);

  signals[0] =
      wl_event_loop_add_signal(loop, SIGTERM, on_term_signal, wl_display);
  signals[1] =
      wl_event_loop_add_signal(loop, SIGINT, on_term_signal, wl_display);
  signals[2] =
      wl_event_loop_add_signal(loop, SIGQUIT, on_term_signal, wl_display);

  if (!signals[0] || !signals[1] || !signals[2]) goto err_signals;

  compositor = zippo_compositor_create(wl_display);
  if (compositor == NULL) {
    fprintf(stderr, "Failed to create compositor\n");
    goto err_signals;
  }

  backend = zippo_backend_create(compositor);
  if (backend == NULL) {
    fprintf(stderr, "Failed to create backend\n");
    goto err_backend;
  }

  // inject dependencies
  compositor->backend = backend;

  // init all
  if (compositor->impl->init(compositor) != 0) {
    fprintf(stderr, "Failed to initialize\n");
    goto err_init;
  }

  wl_display_run(wl_display);

  status = EXIT_SUCCESS;

err_init:
  zippo_backend_destroy(backend);

err_backend:
  zippo_compositor_destroy(compositor);

err_signals:
  for (int i = sizeof(signals) / sizeof(signals[0]) - 1; i >= 0; i--)
    if (signals[i]) wl_event_source_remove(signals[i]);

  wl_display_destroy(wl_display);

  if (status == EXIT_SUCCESS)
    fprintf(stderr, "Exited gracefully\n");
  else
    fprintf(stderr, "Exited with err\n");

  return status;
}
