#include "flidget.h"

void *update_console_data(void *workspace) {
  struct console_ws *ws=(struct console_ws*)workspace;
  char buffer[4096];
  char *data;
  ssize_t count;
  while(1) {
    count=read(ws->ds,buffer,sizeof(buffer)-1);
    if (count==-1) {
      if (errno==EINTR) {
        continue;
      } else {
        perror("read");
        exit(1);
      }
    }
    if (count==0) {
      return NULL;
    }
    buffer[count]=0;
    //printf("buffer:%s-- (count:%d)\n",buffer,count);
    //printf("count:%d\n",count);
    if (count+strlen(ws->data)>ws->data_size-1 && ws->data_size<ws->data_size_max) {
      ws->data_size=min(ws->data_size+4096,ws->data_size_max);
      ws->data=realloc(ws->data,ws->data_size);
      //printf("realloc to %d\n",ws->data_size);
    }
    if (count+strlen(ws->data)>ws->data_size-1) {
      //printf("cutting\n");
      data=strdup(ws->data+count);
    } else {
      //printf("not cutting\n");
      data=strdup(ws->data);
    }
    //printf("data:%s--\n",data);
    snprintf(ws->data,ws->data_size,"%s%s",data,buffer);
    free(data);
    //printf("ws->data:%s-- (data_size:%d len:%d)\n\n",ws->data,ws->data_size,strlen(ws->data));
    //printf("data_size:%d len:%d\n\n",ws->data_size,strlen(ws->data));
    pthread_mutex_lock(ws->data_updated_mutex);
    ws->data_updated=1;
    pthread_mutex_unlock(ws->data_updated_mutex);
    usleep(100000);
  }
  return NULL;
}

int add_text(void *workspace) {
  struct console_ws *ws=(struct console_ws*)workspace;
  gtk_label_set_text(GTK_LABEL(ws->console),ws->data);
  return 0;
}

void *update_console(void *workspace) {
  struct console_ws *ws=(struct console_ws*)workspace;
  while (1) {
    if (ws->data_updated) {
      pthread_mutex_lock(ws->data_updated_mutex);
      ws->data_updated=0;
      pthread_mutex_unlock(ws->data_updated_mutex);
      // ADD DATA TO CONSOLE
      g_idle_add((GSourceFunc)add_text,(void*)ws);
    }
    usleep(100000);
  }
}

void scroll_to_bottom(GtkAdjustment *scroller_adj,void *workspace) {
  //struct console_ws *ws=(struct console_ws*)workspace;
  if (scroller_adj!=NULL) {
    gtk_adjustment_set_value(scroller_adj,gtk_adjustment_get_upper(scroller_adj));
  }
}

void activate_console(GtkApplication *app, void *workspace) {
  struct console_ws *ws=(struct console_ws*)workspace;
  pthread_t *update_console_thread;
  pthread_t *update_data_thread;
  GtkWidget *s_window;
  GtkAdjustment *scroller_adj;

  // launch data source
  ws->ds=launch_ds(ws->label,ws->cmd);

  // allocate data buffers
  ws->data_size=4096;
  ws->data=malloc(ws->data_size);
  memset(ws->data,0,ws->data_size);

  ws->data_updated_mutex=malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(ws->data_updated_mutex,NULL);
  ws->data_updated=0;

  GdkGeometry hints;
  hints.min_width = ws->width;
  hints.max_width = ws->width;
  hints.min_height = ws->height;
  hints.max_height = ws->height;

  ws->window=gtk_application_window_new(app);
  gtk_window_move(GTK_WINDOW(ws->window),ws->posX,ws->posY);
  gtk_window_set_geometry_hints(GTK_WINDOW(ws->window),NULL,&hints,(GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
  gtk_window_set_keep_below(GTK_WINDOW(ws->window),1);
  gtk_window_set_decorated(GTK_WINDOW(ws->window),0);
  gtk_widget_set_visual(ws->window,gdk_screen_get_rgba_visual(gtk_window_get_screen(GTK_WINDOW(ws->window))));
  gtk_widget_set_app_paintable(ws->window,1);

  s_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_widget_set_halign(s_window,GTK_ALIGN_START);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(s_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  scroller_adj=gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(s_window));

  //create console, set name and class
  ws->console=gtk_label_new(NULL);
  gtk_widget_set_name(ws->console,ws->label);
  gtk_style_context_add_class(gtk_widget_get_style_context(ws->console),"console");

  gtk_label_set_yalign(GTK_LABEL(ws->console),0);
  gtk_label_set_line_wrap(GTK_LABEL(ws->console),1);
  gtk_label_set_line_wrap_mode(GTK_LABEL(ws->console),PANGO_WRAP_CHAR);

  gtk_container_add(GTK_CONTAINER(s_window),ws->console);
  gtk_container_add(GTK_CONTAINER(ws->window),s_window);

  g_signal_connect(scroller_adj,"changed",G_CALLBACK(scroll_to_bottom),(void*)ws);

  update_console_thread=malloc(sizeof(pthread_t));
  pthread_create(update_console_thread,NULL,update_console,(void*)ws);
  update_data_thread=malloc(sizeof(pthread_t));
  pthread_create(update_data_thread,NULL,update_console_data,(void*)ws);

  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(ws->window),1);
  gtk_window_set_skip_pager_hint(GTK_WINDOW(ws->window),1);
  gtk_window_set_accept_focus(GTK_WINDOW(ws->window),0);
  gtk_window_stick(GTK_WINDOW(ws->window));
  gtk_window_set_type_hint(GTK_WINDOW(ws->window),GDK_WINDOW_TYPE_HINT_DOCK);

  gtk_widget_show_all(ws->window);
}
