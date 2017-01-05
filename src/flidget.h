#include <gtk/gtk.h>
#include <pthread.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <math.h>

#define MAX_DATA 100

#define min(a,b) \
 ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })

#define max(a,b) \
 ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

struct chained_float {
  float *value;
  struct chained_float *next;
};

struct flidget_ws {
  GtkApplication *app;
  struct chained_ws *wss;
  char *config_folder;
  int nb_widgets;
  FILE *conf;
};

struct chained_ws {
  void *ws;
  char *type;
  struct chained_ws *next;
};

struct console_ws {
  GtkWidget *window;

  // view
  GtkWidget *console;

  // data source
  char *cmd;
  int ds;
  char *data;
  int data_size;
  char data_updated;
  pthread_mutex_t *data_updated_mutex;

  // conf
  char *label;
  int posX,posY,width,height;
  int data_size_max;
};

struct monitor_ws {
  GtkWidget *window;

  // view
  GtkWidget *box;
  GtkStyleContext *style;
  cairo_surface_t *db;
  pthread_mutex_t *db_mutex;
  GdkRGBA *color;
  GdkRGBA *bg_color;
  GdkRGBA **stream_colors;

  // data source
  char *cmd;
  int ds;
  struct chained_float *data;
  char data_updated;
  pthread_mutex_t *data_updated_mutex;

  // conf
  char *label;
  int posX,posY,width,height;
  unsigned int max;
  char *units;
  int nb_streams;
};

enum conf_status {INWIDGET,NEWWIDGET,ERROR,END};

enum conf_status parse_widget(struct flidget_ws *ws);
int init_console_ws(struct console_ws *ws,int nb_params,char **names,char **values);
int init_monitor_ws(struct monitor_ws *ws,int nb_params,char **names,char **values);
void activate_console(GtkApplication *app, void *workspace);
void activate_monitor(GtkApplication *app, void *workspace);
int launch_ds(char *name,char *cmd);
