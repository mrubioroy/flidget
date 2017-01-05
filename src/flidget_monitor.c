#include "flidget.h"

void *update_monitor_data(void *workspace) {
  struct monitor_ws *ws=(struct monitor_ws*)workspace;
  char buffer[4096];
  char *data;
  ssize_t count;
  char *sdata[ws->nb_streams];
  float data_f[ws->nb_streams];
  char *junk;
  int i,j,k;
  char last_char;
  char in_data;
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
    // scan from the end to get last line
    //printf("buffer (count %d):---\n%s---\n",(int)count,buffer);
    i=count;
    in_data=0;
    data=NULL;
    while (i>=0) {
      //printf("buffer[%d]:%d\n",i,buffer[i]);
      if (i==0) {
        data=buffer;
        in_data=1;
      }
      if (buffer[i]==10) {
        if (in_data) {
          data=buffer+i+1;
          break;
        } else {
          in_data=1;
        }
      }
      if (buffer[i]==10 || buffer[i]==13) {
        buffer[i]=0;
      }
      i--;
    }
    if (!in_data) {
      printf("%s: error parsing\n",ws->label);
      continue;
    }
    //printf("\ndata:---%s---\n",data);
    i=0;
    sdata[0]=data;
    j=1;
    k=strlen(data);
    last_char=0;
    while (i<k) {
      if (data[i]==' ' && last_char!=' ') {
        last_char=data[i];
        data[i]=0;
        sdata[j]=data+i+1;
        j++;
      }
      i++;
    }
    if (j!=ws->nb_streams) {
      printf("%s: bad number of data (expected %d, found %d)\n",ws->label,ws->nb_streams,j);
      continue;
    }
    for (i=0;i<ws->nb_streams;i++) {
      data_f[i]=strtof(sdata[i],&junk);
      if (data_f[i]==0 && junk==sdata[i]) {
        printf("%s: error parsing stream %d (data:%s)\n",ws->label,i,data);
        continue;
      }
    }
    //ws->data must point to last value
    ws->data=ws->data->next;
    for (i=0;i<ws->nb_streams;i++) {
      ws->data->value[i]=data_f[i];
    }
    pthread_mutex_lock(ws->data_updated_mutex);
    ws->data_updated=1;
    pthread_mutex_unlock(ws->data_updated_mutex);
    usleep(100000);
  }
  return NULL;
}

void *update_monitor(void *workspace) {
  struct monitor_ws *ws=(struct monitor_ws*)workspace;
  cairo_t *cc;
  int i,j;
  struct chained_float *datum;
  float max_value;
  char label[255];
  float width,height,x,y;
  PangoLayout *pango_layout;
  PangoFontDescription *font;
  while (1) {
    if (ws->db!=NULL && ws->data_updated) {
      pthread_mutex_lock(ws->data_updated_mutex);
      ws->data_updated=0;
      pthread_mutex_unlock(ws->data_updated_mutex);
      pthread_mutex_lock(ws->db_mutex);
      cc=cairo_create(ws->db);
      width=cairo_image_surface_get_width(ws->db);
      height=cairo_image_surface_get_height(ws->db);

      // SET BACKGROUND
      gdk_cairo_set_source_rgba(cc,ws->bg_color);
      cairo_set_operator(cc,CAIRO_OPERATOR_SOURCE);
      cairo_paint(cc);
      cairo_set_operator(cc,CAIRO_OPERATOR_OVER);

      // Determine max_value on this iteration
      max_value=1;
      if (!ws->max) {
        for (i=0;i<ws->nb_streams;i++) {
          datum=ws->data->next;
          j=0;
          while (j==0 || datum!=ws->data->next) {
            max_value=max(max_value,datum->value[i]);
            datum=datum->next;
            j++;
          }
        }
      } else {
        max_value=ws->max;
      }
      max_value=ceilf(max_value);

      // PLOT DATA
      cairo_set_line_width(cc,2);
      for (i=ws->nb_streams-1;i>=0;i--) {
        gdk_cairo_set_source_rgba(cc,ws->stream_colors[i]);
        // Plot data for stream
        //ws->data points to last value in ring
        datum=ws->data->next;
        j=0;
        while (j==0 || datum!=ws->data->next) {
          x=j*width/(MAX_DATA-1);
          y=1+(height-2)*(max_value-datum->value[i])/max_value;
          if (j==0) {
            cairo_move_to(cc,x,y);
          } else {
            cairo_line_to(cc,x,y);
          }
          //printf("j:%2d x:%4f y:%4f\n",j,j*1.0/(MAX_DATA-1),(100-datum->value)/100);
          datum=datum->next;
          j++;
        }
        cairo_set_line_join(cc,CAIRO_LINE_JOIN_ROUND);
        cairo_stroke(cc);
      }

      // WRITE TEXT
      gdk_cairo_set_source_rgba(cc,ws->color);
      gtk_style_context_get(ws->style,gtk_widget_get_state_flags(ws->window),"font",&font,NULL);
      pango_layout=pango_cairo_create_layout(cc);
      pango_layout_set_font_description(pango_layout,font);
      cairo_move_to(cc,5,5);
      snprintf(label,sizeof(label),"%s (max:%.0f%s)",ws->label,max_value,ws->units);
      pango_layout_set_text(pango_layout,label,-1);
      pango_cairo_show_layout(cc,pango_layout);

      // FLUSH & DRAW
      g_object_unref(pango_layout);
      pango_font_description_free(font);
      cairo_surface_flush(ws->db);
      g_idle_add((GSourceFunc)gtk_widget_queue_draw,(void*)ws->window);
      pthread_mutex_unlock(ws->db_mutex);
      cairo_destroy(cc);
    }
    usleep(100000);
  }
}

void configure(GtkWidget *widget, GdkEventConfigure *event, void *workspace) {
  struct monitor_ws *ws=(struct monitor_ws*)workspace;
  pthread_mutex_lock(ws->db_mutex);
  if (ws->db!=NULL) {
    cairo_surface_destroy(ws->db);
    ws->db=NULL;
  }
  ws->db=cairo_image_surface_create(
          CAIRO_FORMAT_ARGB32,
          gtk_widget_get_allocated_width(ws->box),
          gtk_widget_get_allocated_height(ws->box)
  );
  pthread_mutex_unlock(ws->db_mutex);
}

void monitor_draw(GtkWidget *widget, cairo_t *cr, void *workspace) {
  struct monitor_ws *ws=(struct monitor_ws*)workspace;
  pthread_mutex_lock(ws->db_mutex);
  if (ws->db!=NULL) {
    cairo_set_operator(cr,CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr,ws->db,0,0);
    cairo_paint(cr);
  }
  pthread_mutex_unlock(ws->db_mutex);
}

void set_graph_colors(struct monitor_ws* ws) {
  int i;
  GdkRGBA *color;
  char *color_prop=NULL;
  ws->stream_colors=(GdkRGBA**)malloc(sizeof(GdkRGBA*)*ws->nb_streams);
  for (i=0;i<ws->nb_streams;i++) {
    if (color_prop!=NULL)
      free(color_prop);
    color=(GdkRGBA*)malloc(sizeof(GdkRGBA));
    color_prop=malloc(255);
    snprintf(color_prop,255,"%s_%d",ws->label,i+1);
    if (gtk_style_context_lookup_color(ws->style,color_prop,color)) {
      //printf("%s: setting %s: %s\n",ws->label,color_prop,gdk_rgba_to_string(color));
      ws->stream_colors[i]=color;
      continue;
    }
    snprintf(color_prop,255,"graph_%d",i+1);
    if (gtk_style_context_lookup_color(ws->style,color_prop,color)) {
      //printf("%s: setting for %s: %s\n",ws->label,color_prop,gdk_rgba_to_string(color));
      ws->stream_colors[i]=color;
      continue;
    }
    gtk_style_context_get(ws->style,gtk_widget_get_state_flags(ws->box),GTK_STYLE_PROPERTY_COLOR,&color,NULL);
    //printf("%s: defaulting to css color: %s\n",ws->label,gdk_rgba_to_string(color));
    ws->stream_colors[i]=color;
  }
  if (color_prop!=NULL)
    free(color_prop);
}

void activate_monitor(GtkApplication *app, void *workspace) {
  struct monitor_ws *ws=(struct monitor_ws*)workspace;
  pthread_t *update_monitor_thread;
  pthread_t *update_data_thread;
  int i;
  struct chained_float **data;

  // launch data source
  ws->ds=launch_ds(ws->label,ws->cmd);

  // allocate data buffers
  data=malloc(sizeof(struct chained_float*)*MAX_DATA);
  memset(data,0,sizeof(struct chained_float*)*MAX_DATA);
  for (i=0;i<MAX_DATA;i++) {
    data[i]=malloc(sizeof(struct chained_float));
    memset(data[i],0,sizeof(struct chained_float));
    data[i]->value=malloc(sizeof(float)*ws->nb_streams);
    memset(data[i]->value,0,sizeof(float)*ws->nb_streams);
  }
  // chain buffers into a ring
  for (i=0;i<MAX_DATA;i++) {
    if (i!=MAX_DATA-1) {
      data[i]->next=data[i+1];
    } else {
      data[i]->next=data[0];
    }
  }
  ws->data=data[0];

  ws->data_updated_mutex=malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(ws->data_updated_mutex,NULL);
  ws->data_updated=0;

  ws->db_mutex=malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(ws->db_mutex,NULL);
  ws->db=NULL;

  GdkGeometry hints;
  hints.min_width = ws->width;
  hints.max_width = ws->width;
  hints.min_height = ws->height;
  hints.max_height = ws->height;

  ws->window=gtk_application_window_new(app);
  ws->box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);

  gtk_widget_set_name(ws->box,ws->label);
  ws->style=gtk_widget_get_style_context(ws->box);
  gtk_style_context_add_class(ws->style,"graph");
  set_graph_colors(ws);
  gtk_style_context_get(ws->style,gtk_widget_get_state_flags(ws->box),GTK_STYLE_PROPERTY_COLOR,&ws->color,NULL);
  gtk_style_context_get(ws->style,gtk_widget_get_state_flags(ws->box),GTK_STYLE_PROPERTY_BACKGROUND_COLOR,&ws->bg_color,NULL);

  gtk_window_move(GTK_WINDOW(ws->window),ws->posX,ws->posY);
  gtk_window_set_geometry_hints(GTK_WINDOW(ws->window),NULL,&hints,(GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
  gtk_window_set_keep_below(GTK_WINDOW(ws->window),1);
  gtk_window_set_decorated(GTK_WINDOW(ws->window),0);
  gtk_widget_set_visual(ws->window,gdk_screen_get_rgba_visual(gtk_window_get_screen(GTK_WINDOW(ws->window))));
  gtk_widget_set_app_paintable(ws->window,1);

  gtk_container_add(GTK_CONTAINER(ws->window),ws->box);

  g_signal_connect_after(ws->box,"draw",G_CALLBACK(monitor_draw),(void*)ws);
  g_signal_connect(ws->window,"configure-event",G_CALLBACK(configure),(void*)ws);

  update_monitor_thread=malloc(sizeof(pthread_t));
  pthread_create(update_monitor_thread,NULL,update_monitor,(void*)ws);
  update_data_thread=malloc(sizeof(pthread_t));
  pthread_create(update_data_thread,NULL,update_monitor_data,(void*)ws);

  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(ws->window),1);
  gtk_window_set_skip_pager_hint(GTK_WINDOW(ws->window),1);
  gtk_window_set_accept_focus(GTK_WINDOW(ws->window),0);
  gtk_window_stick(GTK_WINDOW(ws->window));
  gtk_window_set_type_hint(GTK_WINDOW(ws->window),GDK_WINDOW_TYPE_HINT_DOCK);

  gtk_widget_show_all(ws->window);
}
